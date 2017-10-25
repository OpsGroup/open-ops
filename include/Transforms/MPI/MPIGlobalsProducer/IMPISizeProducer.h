#ifndef I_MPI_SIZE_PRODUCER_H_INCLUDED
#define I_MPI_SIZE_PRODUCER_H_INCLUDED

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

            class IMPISizeProducer: public ITransformation
            {
            public:
                virtual ~IMPISizeProducer() {};

                virtual const BasicType* getSizeType() const = 0;
            };
        }
    }
}

#endif // I_MPI_SIZE_PRODUCER_H_INCLUDED
