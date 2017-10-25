#ifndef MPI_SIZE_PRODUCER_H_INCLUDED
#define MPI_SIZE_PRODUCER_H_INCLUDED

#include "Transforms/MPI/MPIGlobalsProducer/IMPISizeProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using OPS::Reprise::BasicType;
            using OPS::Reprise::ReprisePtr;
            using OPS::Reprise::StatementBase;
            using OPS::Reprise::SubroutineDeclaration;
            using OPS::Reprise::VariableDeclaration;
            using OPS::Transforms::ITransformation;

            class MPIProducerFactory;

            class MPISizeProducer: public IMPISizeProducer
            {
            public:
                // Constructor
                MPISizeProducer(SubroutineDeclaration* pEntryPoint);

                // Destructor
                virtual ~MPISizeProducer();

                // ITransformation implementation
                virtual bool analyseApplicability();
                virtual std::string getErrorMessage();
                virtual void makeTransformation();

                // IMPISizeProducer implementation
                const BasicType* getSizeType() const;

            protected:
                MPIProducerFactory* m_pFactory;
                MPIHelper*          m_pMPIHelper;

                SubroutineDeclaration* m_pEntryPoint;
                BasicType* m_pSizeType;
                VariableDeclaration* m_pSizeDeclaration;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_SIZE_PRODUCER_H_INCLUDED
