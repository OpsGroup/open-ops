#ifndef I_MPI_GLOBALS_PRODUCER_H_INCLUDED
#define I_MPI_GLOBALS_PRODUCER_H_INCLUDED

#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            using OPS::Transforms::ITransformation;

            class MPIProducerFactory;
            /**
                Need to be called before all MPI transformation
                Summary
                1) Find the entry point of input program      <-- Need to give the entry point in parameters
                2) Add call of MPI_Init as first executable statement of the program
                3) Add call of MPI_finalise as last executable statement of the program

                To particular information see documentation
            */
            class IMPIGlobalsProducer: public ITransformation
            {
            public:
                virtual ~IMPIGlobalsProducer() { };
            };
        }
    }
}

#endif // I_MPI_GLOBALS_PRODUCER_H_INCLUDED
