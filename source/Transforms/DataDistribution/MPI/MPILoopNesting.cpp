#include "Transforms/DataDistribution/MPI/MPILoopNesting.h"

#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
#include "Transforms/Loops/LoopNesting/LoopNesting.h"
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

            MPILoopNesting::MPILoopNesting(ForStatement& forStatement):
                m_pFactory(NULL),
                m_pRankProducer(NULL),
                m_pSizeProducer(NULL),
                m_rpForStatement(&forStatement),
                m_pParentSubroutine(NULL),
                m_pRankDeclaration(NULL),
                m_pSizeDeclaration(NULL),
                m_errors(),
                m_analysisRerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();

                m_pParentSubroutine = Shared::getSubroutineDeclarationByStatement(m_rpForStatement.get());

                m_pRankProducer = m_pFactory->createMPIRankProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pRankProducer != NULL);

                m_pSizeProducer = m_pFactory->createMPISizeProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pSizeProducer != NULL);
            }

            bool MPILoopNesting::analyseApplicability()
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

                if(m_errors.size() == 0)
                {
                    if(!Transforms::Loops::canApplyLoopNestingTo(*m_rpForStatement))
                    {
                        m_errors.push_back(_TL("Cannot apply \"LootNesting\" transformation", "Невозможно применить преобразование \"Гнездование цикла\""));
                    }
                }

                m_analysisRerformed = true;

                return m_errors.size() == 0;
            }

            void MPILoopNesting::makeTransformation()
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

                m_pSizeDeclaration = MPIHelper::getSizeDeclaration(*m_pParentSubroutine);
                if(m_pSizeDeclaration == NULL)
                {
                    m_pSizeProducer->makeTransformation();
                    m_pSizeDeclaration = MPIHelper::getSizeDeclaration(*m_pParentSubroutine);
                }

                OPS_ASSERT(m_pSizeDeclaration != NULL);

                Declarations& declarations = m_rpForStatement->getRootBlock().getDeclarations();

                ReprisePtr<ReferenceExpression> rpSizeReference(new ReferenceExpression(*m_pSizeDeclaration));
                BlockStatement& loopNestingResult = Transforms::Loops::makeLoopNesting(*m_rpForStatement, *rpSizeReference);
                
                OPS_ASSERT(!loopNestingResult.isEmpty());
                OPS_ASSERT(loopNestingResult.getFirst()->is_a<ForStatement>());
                ForStatement& loopNestingOuterFor = loopNestingResult.getFirst()->cast_to<ForStatement>();
                
                BasicCallExpression& rank2CounterAssign = *Editing::getBasicForCounter(loopNestingOuterFor).clone() R_AS *(new ReferenceExpression(*m_pRankDeclaration));
                ExpressionStatement* pRank2CounterAssignStmt = new ExpressionStatement(&rank2CounterAssign);

                ReprisePtr<ExpressionBase> rankExpression(new ReferenceExpression(*m_pRankDeclaration));
                Transforms::Scalar::makeSubstitutionForward(loopNestingOuterFor.getBody(), Editing::getBasicForCounter(loopNestingOuterFor), rankExpression, true);

                BlockStatement& parentBlock = loopNestingOuterFor.getParentBlock();
                parentBlock.addBefore(parentBlock.convertToIterator(&loopNestingOuterFor), pRank2CounterAssignStmt);

                ReprisePtr<StatementBase> rpInnerBlock = cloneStatement(loopNestingOuterFor.getBody(), declarations, declarations);
                BlockStatement* pInnerBlock = rpInnerBlock->cast_ptr<BlockStatement>();
                parentBlock.replace(parentBlock.convertToIterator(&loopNestingOuterFor), pInnerBlock);
            }

            std::string MPILoopNesting::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }

            MPILoopNesting::~MPILoopNesting()
            {
                delete m_pRankProducer;
                delete m_pSizeProducer;
                delete m_pFactory;
            }
}
}
}
