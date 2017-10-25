#ifndef I_MPI_SINGLE_ACCESS_AREA_PRODUCER_H_INCLUDED
#define I_MPI_SINGLE_ACCESS_AREA_PRODUCER_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::VariableDeclaration;

            // Интерфейс для создания области единичного доступа
            class IMPISingleAccessAreaProducer: public ITransformation
            {
            public:
                typedef std::list<VariableDeclaration*> VariableDeclarationContainer;

                virtual void setDeclarationsToBcast(VariableDeclarationContainer declarations) = 0;

                virtual ~IMPISingleAccessAreaProducer() {};
            };
        }
    }
}

#endif // I_MPI_SINGLE_ACCESS_AREA_PRODUCER_H_INCLUDED
