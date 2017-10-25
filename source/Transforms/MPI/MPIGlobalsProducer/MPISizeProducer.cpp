#include "Transforms/MPI/MPIGlobalsProducer/MPISizeProducer.h"

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

            MPISizeProducer::MPISizeProducer(SubroutineDeclaration* pEntryPoint): 
                m_pFactory(NULL),
                m_pMPIHelper(NULL),
                m_pEntryPoint(pEntryPoint),
                m_pSizeType(NULL),
                m_pSizeDeclaration(NULL),
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

                TypeBase* pType = Shared::getArgumentPointedType(m_pMPIHelper->getMPIFunctionDeclaration(MPIFunctionsHelper::MF_MPI_COMM_SIZE), 1);
                if(pType != NULL)
                {
                    m_pSizeType = pType->cast_ptr<BasicType>();
                }
            }

            MPISizeProducer::~MPISizeProducer()
            {
                delete m_pMPIHelper;
                delete m_pFactory;
            }

            bool MPISizeProducer::analyseApplicability()
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

                if (m_pSizeType == NULL)
                {
                    m_errors.push_back(_TL("Invalid type of size.", "Некорректный тип переменной size."));
                }

                m_analysisPerformed = true;

                return m_errors.size() == 0;
            }

            std::string MPISizeProducer::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }
            
            void MPISizeProducer::makeTransformation()
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

                m_pSizeDeclaration = &Editing::createNewVariable(*m_pSizeType, m_pEntryPoint->getBodyBlock(), "Size");

                pGlobalsInitBlock->addLast(m_pMPIHelper->createMPICommSizeCallStatement(*m_pSizeDeclaration).get());

                m_pSizeDeclaration->setNote(MPIHelper::SIZE_NOTE_NAME, Note::newBool(true));
            }

            const BasicType* MPISizeProducer::getSizeType() const
            {
                return m_pSizeType;
            }
        }
    }
}
