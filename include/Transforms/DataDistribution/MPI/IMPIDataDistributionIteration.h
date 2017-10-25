#ifndef I_MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED
#define I_MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::VariableDeclaration;

            // Интерфейс для итерационного переразмещения данных
            class IMPIDataDistributionIteration: public ITransformation
            {
            public:
                // Задает объявление массива, который будет содержать размещенные данные.
                // Должен быть вызван до вызова метода makeTransformation.
                virtual void setNewArrayDeclaration(VariableDeclaration& newArrayDeclaration) = 0;

                virtual ~IMPIDataDistributionIteration() {};
            };
        }
    }
}

#endif // I_MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED
