#ifndef MPI_GLOBALS_C_PRODUCER_H_INCLUDED
#define MPI_GLOBALS_C_PRODUCER_H_INCLUDED

#include <list>

#include "Transforms/MPI/MPIGlobalsProducer/IMPIGlobalsProducer.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using OPS::Reprise::SubroutineDeclaration;

            class MPIGlobalsProducer: public IMPIGlobalsProducer
            {
            public:
                // Constructor
                MPIGlobalsProducer(SubroutineDeclaration* pEntryPoint);

                // Destructor
                ~MPIGlobalsProducer();

                // ITransformation implementation
                virtual bool analyseApplicability();
                virtual std::string getErrorMessage();
                virtual void makeTransformation();

            private:
                MPIProducerFactory* m_pFactory;
                MPIHelper*          m_pMPIHelper;

                OPS::Reprise::SubroutineDeclaration* m_pEntryPoint;

                OPS::Reprise::VariableDeclaration* m_pArgcDeclaration;
                OPS::Reprise::VariableDeclaration* m_pArgvDeclaration;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_GLOBALS_C_PRODUCER_H_INCLUDED
