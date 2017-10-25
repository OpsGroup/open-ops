#include "Transforms/MPI/MPIGlobalsProducer/MPIGlobalsProducer.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/SubroutinesShared.h"
#include "OPS_Core/Localization.h"
#include "MPIGlobalsProducerDeepWalkers.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using namespace OPS::Reprise;
            using namespace OPS::Shared::ExpressionHelpers;

            MPIGlobalsProducer::MPIGlobalsProducer( SubroutineDeclaration* pEntryPoint ): 
                m_pFactory(NULL),
                m_pMPIHelper(NULL),
                m_pEntryPoint(pEntryPoint),
                m_pArgcDeclaration(NULL),
                m_pArgvDeclaration(NULL),
                m_analysisPerformed(false)
            {
                m_pFactory = new MPIProducerCFactory();
                m_pMPIHelper = m_pFactory->createMPIHelper(Shared::getTranslationUnitBySubroutineDeclaration(m_pEntryPoint));

                if(m_pEntryPoint->getType().getParameterCount() == 2)
                {
                    m_pArgcDeclaration = &m_pEntryPoint->getType().getParameter(0).getAssociatedVariable();
                    m_pArgvDeclaration = &m_pEntryPoint->getType().getParameter(1).getAssociatedVariable();
                }
            }

            MPIGlobalsProducer::~MPIGlobalsProducer()
            {
                delete m_pMPIHelper;
                delete m_pFactory;
            }

            bool MPIGlobalsProducer::analyseApplicability()
            {
                m_errors.clear();

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

                if(m_pArgcDeclaration == NULL)
                {
                    m_errors.push_back(_TL("Cannot find argc.", "Не удалось найти argc."));
                }

                if (m_pArgvDeclaration == NULL)
                {
                    m_errors.push_back(_TL("Cannot find argv.", "Не удалось найти argv."));
                }

                m_analysisPerformed = true;

                return m_errors.size() == 0;
            }

            std::string MPIGlobalsProducer::getErrorMessage()
            {
                std::string error = "";

                for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
                {
                    error = error + "\n" + *it;
                }

                return error;
            }

            void MPIGlobalsProducer::makeTransformation()
            {
                if(!m_analysisPerformed)
                {
                    analyseApplicability();
                }

                OPS_ASSERT(m_errors.size() == 0);

                BlockStatement& entryBlock = m_pEntryPoint->getBodyBlock();
                BlockStatement* pGlobalsInitBlock = MPIHelper::getGlobalsInitBlock(*m_pEntryPoint);
                if(pGlobalsInitBlock == NULL)
                {
                    pGlobalsInitBlock = MPIHelper::createGlobalsInitBlock(*m_pEntryPoint);
                }

                // Insert MPI_Init call
                ReprisePtr<SubroutineCallExpression> rpMPIInitCall = m_pMPIHelper->createMPIInitCallExpression(*m_pArgcDeclaration, *m_pArgvDeclaration);
                pGlobalsInitBlock->addFirst(new ExpressionStatement(rpMPIInitCall.get()));

                // Insert MPI_Finalize call
                ReturnFinderDeepWalker rfdw;
                rfdw.visit(entryBlock);
                ReturnFinderDeepWalker::ReturnsList returnsList = rfdw.getReturnsList();
                for(ReturnFinderDeepWalker::ReturnsList::iterator it = returnsList.begin(); it != returnsList.end(); ++it)
                {
                    ReprisePtr<SubroutineCallExpression> rpMPIFinalizeCall = m_pMPIHelper->createMPIFinalizeCallExpression();
                    (*it)->getParentBlock().addBefore((*it)->getParentBlock().convertToIterator(*it), new ExpressionStatement(rpMPIFinalizeCall.get()));
                }

                ReprisePtr<SubroutineCallExpression> rpMPIFinalizeCall = m_pMPIHelper->createMPIFinalizeCallExpression();
                entryBlock.addLast(new ExpressionStatement(rpMPIFinalizeCall.get()));
            }
        }
    }
}
