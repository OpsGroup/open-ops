#include "Transforms/DataDistribution/MPI/Block/BlockLoopNesting.h"

#include "Reprise/Service/DeepWalker.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
#include "Transforms/Loops/LoopNesting/LoopNesting.h"
#include "Transforms/DataDistribution/MPI/Block/DistributedIndexBuilder.h"
#include "Transforms/DataDistribution/MPI/Block/BlocksHelper.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/RepriseClone.h"
#include "OPS_Core/Localization.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
            using namespace OPS::Transforms::MPIProducer;
            using namespace OPS::Reprise;
            using namespace OPS::Shared;
            using namespace OPS::Shared::ExpressionHelpers;
            using namespace OPS::Reprise::Service;

            class ReplacedExpressionsFinderDeepWalker: public DeepWalker
            {
            public:
                ReplacedExpressionsFinderDeepWalker(): DeepWalker()
                {
                }

                void visit(BasicCallExpression& expression)
                {
                    if(expression.hasNote(BlocksHelper::OLD_EXPRESSION_NOTE_NAME) && expression.getNote(BlocksHelper::OLD_EXPRESSION_NOTE_NAME).getKind() == Note::NK_REPRISE
                        && expression.hasNote(BlocksHelper::PARAMETERS_FAMILY_NOTE_NAME) && expression.getNote(BlocksHelper::PARAMETERS_FAMILY_NOTE_NAME).getKind() == Note::NK_STRING)
                    {
                        ReprisePtr<RepriseBase> rpOldExpr = expression.getNote(BlocksHelper::OLD_EXPRESSION_NOTE_NAME).getReprisePtr();
                        std::string paramsString = expression.getNote(BlocksHelper::PARAMETERS_FAMILY_NOTE_NAME).getString();
                        BADDParametersFamily dummyFamily;
                        if(rpOldExpr.get()->is_a<BasicCallExpression>() && BADDParametersFamily::Parse(paramsString, dummyFamily))
                        {
                            m_parameters[&expression] = dummyFamily;
                            m_oldExpressions[&expression] = rpOldExpr.get()->cast_ptr<BasicCallExpression>();
                        }
                    }
                    DeepWalker::visit(expression);
                }

                std::map<BasicCallExpression*, BasicCallExpression*> getOldExpressions()
                {
                    return m_oldExpressions;
                }

                std::map<BasicCallExpression*, BADDParametersFamily> getParameters()
                {
                    return m_parameters;
                }

            private:
                std::map<BasicCallExpression*, BADDParametersFamily> m_parameters;
                std::map<BasicCallExpression*, BasicCallExpression*> m_oldExpressions;
            };

            BlockLoopNesting::BlockLoopNesting(ForStatement& outerForStatement, const std::vector<int>& blockWidths, const std::vector<int>& iterationCounts):
                m_rpOuterForStatement(&outerForStatement),
                m_blockWidths(blockWidths),
                m_iterationCounts(iterationCounts),
                m_errors(),
                m_analysisRerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();

                m_pParentSubroutine = Shared::getSubroutineDeclarationByStatement(m_rpOuterForStatement.get());

                m_pRankProducer = m_pFactory->createMPIRankProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pRankProducer != NULL);

                m_pSizeProducer = m_pFactory->createMPISizeProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pSizeProducer != NULL);
            }

            bool BlockLoopNesting::analyseApplicability()
            {
                OPS_ASSERT(m_pFactory != NULL);
                m_errors.clear();

                OPS_ASSERT(m_pRankProducer != NULL);
                if(!m_pRankProducer->analyseApplicability())
                {
                    m_errors.push_back(m_pRankProducer->getErrorMessage());
                }

                OPS_ASSERT(m_pSizeProducer != NULL);
                if(!m_pSizeProducer->analyseApplicability())
                {
                    m_errors.push_back(m_pSizeProducer->getErrorMessage());
                }

                m_pLoopNestBlocking = new Loops::LoopNestBlocking(*m_rpOuterForStatement, m_blockWidths.size(), m_blockWidths, true);
                if(!m_pLoopNestBlocking->analyseApplicability())
                {
                    m_errors.push_back(m_pLoopNestBlocking->getErrorMessage());
                }

                BlockingDescription bd(m_blockWidths, m_iterationCounts);
                m_pBlockIndexesProducer = new BlockIndexesProducer(*m_pParentSubroutine, bd);

                m_analysisRerformed = true;

                return m_errors.size() == 0;
            }

            void BlockLoopNesting::makeTransformation()
            {
                if(!m_analysisRerformed)
                {
                    analyseApplicability();
                }
                OPS_ASSERT(m_errors.size() == 0);

                m_pRankDeclaration = MPIHelper::getRankDeclaration(*m_pParentSubroutine);
                if(m_pRankDeclaration == NULL)
                {
                    m_pRankProducer->makeTransformation();
                    m_pRankDeclaration = MPIHelper::getRankDeclaration(*m_pParentSubroutine);
                }
                OPS_ASSERT(m_pRankDeclaration != NULL);

                m_pBlockIndexesProducer->makeTransformation();
                std::vector<VariableDeclaration*> blockRanks = m_pBlockIndexesProducer->getBlockRanks();

                m_pLoopNestBlocking->makeTransformation();
                std::vector<VariableDeclaration*> counters = m_pLoopNestBlocking->getCounters();
                std::vector<ForStatement*> nestLoops = m_pLoopNestBlocking->getResultNestLoops();

                ReprisePtr<BlockStatement> rpRankToCounterAssignBlock(new BlockStatement());
				for(size_t loopDimensionNumber = 0; loopDimensionNumber < m_blockWidths.size(); ++loopDimensionNumber)
                {
                    BasicCallExpression& rank2CounterAssign = *(new ReferenceExpression(*counters[loopDimensionNumber])) R_AS *(new ReferenceExpression(*blockRanks[loopDimensionNumber]));
                    rpRankToCounterAssignBlock->addLast(new ExpressionStatement(&rank2CounterAssign));
                }

                BlockStatement& parentBlock = nestLoops[0]->getParentBlock();
                parentBlock.addBefore(parentBlock.convertToIterator(nestLoops[0]), rpRankToCounterAssignBlock.get());

                ReprisePtr<StatementBase> rpInnerBlock(nestLoops[m_blockWidths.size()]->clone());
                parentBlock.replace(parentBlock.convertToIterator(nestLoops[0]), rpInnerBlock.get());

                std::vector<VariableDeclaration*> blockCounters(m_blockWidths.size());
				for(size_t loopDimension = 0; loopDimension < m_blockWidths.size(); ++loopDimension)
                {
                    blockCounters[loopDimension] = counters[loopDimension + m_blockWidths.size()];
                }
                ReplacedExpressionsFinderDeepWalker dw;
                rpInnerBlock->accept(dw);
                std::map<BasicCallExpression*, BADDParametersFamily> parameters = dw.getParameters();
                std::map<BasicCallExpression*, BasicCallExpression*> oldExpressions = dw.getOldExpressions();
                for(std::map<BasicCallExpression*, BasicCallExpression*>::iterator it = oldExpressions.begin(); it != oldExpressions.end(); ++it)
                {
                    BasicCallExpression& currentExpression = *(it->first);
                    BasicCallExpression& oldExpression = *(it->second);
                    BADDParametersFamily parametersFamily = parameters[&currentExpression];
                    DistributedIndexBuilder dib(oldExpression, parametersFamily, m_pLoopNestBlocking->getOldCounters());
                    currentExpression.setArgument(1, dib.buildDistributedIndex(blockCounters).get());
                }
            }

            std::string BlockLoopNesting::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }

            BlockLoopNesting::~BlockLoopNesting()
            {
                delete m_pBlockIndexesProducer;
                delete m_pLoopNestBlocking;
                delete m_pRankProducer;
                delete m_pSizeProducer;
                delete m_pFactory;
            }
}
}
}
