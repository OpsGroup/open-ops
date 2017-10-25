#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"

#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/Checks.h"
#include "Shared/LoopShared.h"
#include "Shared/StatementsShared.h"
#include "Shared/SubroutinesShared.h"
#include "OPS_Core/Localization.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using namespace OPS::Reprise;
            using namespace OPS::Shared::ExpressionHelpers;

            MPIRankProducer::MPIRankProducer(SubroutineDeclaration* pEntryPoint): 
                m_pFactory(NULL),
                m_pMPIHelper(NULL),
                m_pEntryPoint(pEntryPoint),
                m_pRankType(NULL),
                m_pRankDeclaration(NULL),
                m_analysisPerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();

                TranslationUnit* pTranslationUnit = Shared::getTranslationUnitBySubroutineDeclaration(m_pEntryPoint);
                if(pTranslationUnit == NULL)
                {
                    return;
                }

                m_pMPIHelper = m_pFactory->createMPIHelper(pTranslationUnit);
                if(!m_pMPIHelper->validate())
                {
                    return;
                }

                TypeBase* pType = Shared::getArgumentPointedType(m_pMPIHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_COMM_RANK), 1);
                if(pType != NULL)
                {
                    m_pRankType = pType->cast_ptr<BasicType>();
                }
            }

            MPIRankProducer::~MPIRankProducer()
            {
                delete m_pMPIHelper;
                delete m_pFactory;
            }

            bool MPIRankProducer::analyseApplicability()
            {
                OPS_ASSERT(m_pMPIHelper != NULL);
                if(!m_pMPIHelper->validate())
                {
                    m_errors.push_back(_TL("MPI library is not included or not correct.", "Библиотека MPI не подключена или некорректна."));
                }

                if(m_pEntryPoint == NULL)
                {
                    m_errors.push_back(_TL("Entry point is not specified.", "Не указана точка входа."));
                }
                else
                {
                    if(!m_pEntryPoint->hasImplementation())
                    {
                        m_errors.push_back( _TL("Entry point without implementation.", "Точка входа должна иметь тело."));
                    }
                }

                if (m_pRankType == NULL)
                {
                    m_errors.push_back(_TL("Invalid type of rank.", "Некорректный тип переменной rank."));
                }

                m_analysisPerformed = true;

                return m_errors.size() == 0;
            }

            std::string MPIRankProducer::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }
            
            void MPIRankProducer::makeTransformation()
            {
                if(!m_analysisPerformed)
                {
                    analyseApplicability();
                }

                OPS_ASSERT(m_errors.size() == 0);

                BlockStatement* pGlobalsInitBlock = MPIHelper::getGlobalsInitBlock(*m_pEntryPoint);
                if(pGlobalsInitBlock == NULL)
                {
                    pGlobalsInitBlock = MPIHelper::createGlobalsInitBlock(*m_pEntryPoint);
                }
                
                m_pRankDeclaration = &Editing::createNewVariable(*m_pRankType, m_pEntryPoint->getBodyBlock(), "Rank");

                pGlobalsInitBlock->addLast(m_pMPIHelper->createMPICommRankCallStatement(*m_pRankDeclaration).get());

                m_pRankDeclaration->setNote(MPIHelper::RANK_NOTE_NAME, Note::newBool(true));
            }

            const BasicType* MPIRankProducer::getRankType() const
            {
                return m_pRankType;
            }
        }
    }
}
