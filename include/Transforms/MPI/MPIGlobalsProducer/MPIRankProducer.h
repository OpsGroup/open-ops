#ifndef MPI_RANK_PRODUCER_H_INCLUDED
#define MPI_RANK_PRODUCER_H_INCLUDED

#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Transforms/MPI/MPIGlobalsProducer/IMPIRankProducer.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            class MPIProducerFactory;

            using OPS::Reprise::ReprisePtr;
            using OPS::Reprise::StatementBase;
            using OPS::Reprise::SubroutineDeclaration;
            using OPS::Reprise::BasicType;
            using OPS::Reprise::VariableDeclaration;

            class MPIRankProducer: public IMPIRankProducer
            {
            public:
                MPIRankProducer(SubroutineDeclaration* pEntryPoint);
                virtual ~MPIRankProducer();

                // ITransformation implementation
                virtual bool analyseApplicability();
                virtual std::string getErrorMessage();
                virtual void makeTransformation(); 

                // IMPIRankProducer implementation
                const BasicType* getRankType() const;

            private:
                MPIProducerFactory* m_pFactory;
                MPIHelper*          m_pMPIHelper;

                SubroutineDeclaration* m_pEntryPoint;
                BasicType* m_pRankType;
                VariableDeclaration* m_pRankDeclaration;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_RANK_PRODUCER_H_INCLUDED
