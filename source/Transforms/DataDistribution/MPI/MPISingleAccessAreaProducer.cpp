#include "Transforms/DataDistribution/MPI/MPISingleAccessAreaProducer.h"

#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
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

            MPISingleAccessAreaProducer::MPISingleAccessAreaProducer(StatementBase& initialisationSatatement, ReprisePtr<ExpressionBase> rpShiftExpression):
                m_rpInitialisationSatatement(&initialisationSatatement),
                m_rpShiftExpression(rpShiftExpression),
                m_pRankDeclaration(NULL),
                m_pSizeDeclaration(NULL),
                m_errors(),
                m_analysisRerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();

                m_pParentSubroutine = Shared::getSubroutineDeclarationByStatement(m_rpInitialisationSatatement.get());

                m_pHelper = m_pFactory->createMPIHelper(Shared::getTranslationUnit(m_rpInitialisationSatatement.get()));
                OPS_ASSERT(m_pHelper != NULL);

                m_pRankProducer = m_pFactory->createMPIRankProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pRankProducer != NULL);

                m_pSizeProducer = m_pFactory->createMPISizeProducer(m_pParentSubroutine);
                OPS_ASSERT(m_pSizeProducer != NULL);
            }

            bool MPISingleAccessAreaProducer::analyseApplicability()
            {
                OPS_ASSERT(m_pFactory != NULL);
                m_errors.clear();

                OPS_ASSERT(m_pHelper != NULL);
                if(!m_pHelper->validate())
                {
                    m_errors.push_back(_TL("MPI library is not included or not correct.", "Библиотека MPI не подключена или некорректна."));
                }

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

                // Все объявления, для к-х нужно делать Bcast
                // должны быть базовым типом или массивом базового типа
                for (VariableDeclarationContainer::iterator it = m_declarationsToBcast.begin(); it != m_declarationsToBcast.end(); ++it)
                {
                    VariableDeclaration* pDeclaration = *it;
                    OPS_ASSERT(pDeclaration != NULL);

                    if (pDeclaration->getType().is_a<BasicType>()) continue;
                    
                    if (pDeclaration->getType().is_a<ArrayType>())
                    {
                        TypeBase* pType = Shared::getArrayElementBasicType(&(pDeclaration->getType()));
                        OPS_ASSERT(pType != NULL);

                        if(pType->is_a<BasicType>()) continue;
                    }
                    
                    m_errors.push_back(_TL("Uncorrect type of variable ", "Некорректный тип переменной ") + pDeclaration->getName());
                }

                m_analysisRerformed = true;

                return m_errors.size() == 0;
            }

            void MPISingleAccessAreaProducer::makeTransformation()
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

                ReprisePtr<BlockStatement> rpResultBlock(new BlockStatement());

                IntegerHelper ih(m_pRankProducer->getRankType()->getKind());
                ReprisePtr<IfStatement> rpNewIfStatement(new IfStatement(
                    new BasicCallExpression(
                        BasicCallExpression::BCK_EQUAL, 
                        new ReferenceExpression(*m_pRankDeclaration), 
                        &(op(*m_rpShiftExpression) % *(new ReferenceExpression(*m_pSizeDeclaration)))
                    )
                ));
                rpResultBlock->addLast(rpNewIfStatement.get());

                Declarations& declarations = m_rpInitialisationSatatement->getRootBlock().getDeclarations();
                rpNewIfStatement->getThenBody().addFirst(cloneStatement(*m_rpInitialisationSatatement, declarations, declarations).get());
                
                m_rpInitialisationSatatement->getParentBlock().replace(m_rpInitialisationSatatement->getParentBlock().convertToIterator(m_rpInitialisationSatatement.get()), rpResultBlock.get());

                for (VariableDeclarationContainer::iterator it = m_declarationsToBcast.begin(); it != m_declarationsToBcast.end(); ++it)
                {
                    VariableDeclaration* pDeclaration = *(it);

                    if(pDeclaration == NULL) continue;

                    ReprisePtr<ExpressionBase> rpBufferExpression(new ReferenceExpression(*pDeclaration));
                    
                    int dimention = Shared::getTypeDimension(pDeclaration->getType());
                    IntegerHelper ih(BasicType::BT_INT32);
                    for (int i = 0; i < dimention; ++i)
                    {
                        rpBufferExpression.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, rpBufferExpression.get(), &ih(0)));
                    }

                    rpBufferExpression.reset(&(R_AD()*rpBufferExpression));

                    TypeBase* pElementType = Shared::getArrayElementBasicType(&(pDeclaration->getType()));
                    OPS_ASSERT(pElementType == NULL || pElementType->is_a<BasicType>());

                    IntegerHelper ihCount(Shared::getArgumentPointedType(m_pHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_BCAST), 1)->cast_to<BasicType>());
                    rpResultBlock->addLast(m_pHelper->createMPIBcastStatement(
                        *rpBufferExpression,
                        ihCount(Shared::getTypeSize(pDeclaration->getType())),
                        *m_pHelper->createMPIBasicTypeCode(m_pFactory->createMPIBasicTypesHelper()->getMPIBasicTypeByRepriseBasicType(pElementType->cast_to<BasicType>().getKind())),
                        op(*m_rpShiftExpression) % *(new ReferenceExpression(*m_pSizeDeclaration)),
                        *m_pHelper->createMPICommCode(MPICommsHelper::MC_MPI_COMM_WORLD)
                    ).get());
                }
            }

            std::string MPISingleAccessAreaProducer::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }

            MPISingleAccessAreaProducer::~MPISingleAccessAreaProducer()
            {
                delete m_pHelper;
                delete m_pRankProducer;
                delete m_pSizeProducer;
                delete m_pFactory;
            }

            void MPISingleAccessAreaProducer::setDeclarationsToBcast(MPISingleAccessAreaProducer::VariableDeclarationContainer declarations)
            {
                m_declarationsToBcast = declarations;
            }
}
}
}
