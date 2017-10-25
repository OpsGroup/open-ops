#ifndef I_MPI_RANK_PRODUCER_H_INCLUDED
#define I_MPI_RANK_PRODUCER_H_INCLUDED

#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using OPS::Reprise::BasicType;
            using OPS::Transforms::ITransformation;

            class IMPIRankProducer: public ITransformation
            {
            public:
                virtual ~IMPIRankProducer() {};

                virtual const BasicType* getRankType() const = 0;
            };
        }
    }
}

#endif // I_MPI_RANK_PRODUCER_H_INCLUDED
