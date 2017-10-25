#ifndef MPI_DISTRIBUTED_INDEX_BUILDER_H_INCLUDED
#define MPI_DISTRIBUTED_INDEX_BUILDER_H_INCLUDED

#include "OPS_Core/MemoryHelper.h"
#include "Shared/LinearExpressions.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::ExpressionBase;
            using OPS::Reprise::VariableDeclaration;
            using OPS::Reprise::ReprisePtr;
            using OPS::Shared::ParametricLinearExpression;

            class DistributedIndexBuilder
            {
            public:
                DistributedIndexBuilder(
                        ExpressionBase& sourceExpression,
                        BADDParametersFamily& parametersFamily,
                        const std::vector<VariableDeclaration*>& oldCounters
                    );

                bool analyseApplicability();
                std::string getErrorMessage();
                ReprisePtr<ExpressionBase> buildDistributedIndex(const std::vector<VariableDeclaration*>& blockCounters);

            private:
                ReprisePtr<ExpressionBase> getBlockIndex(ParametricLinearExpression& parametricLinearExpression, const std::vector<VariableDeclaration*>& blockCounters);
                int getOldCounterIndexForDimension(ParametricLinearExpression& parametricLinearExpression);
                int getElementsInDimension(int dimensionNumber);

            private:
                ExpressionBase& m_sourceExpression;
                BADDParametersFamily m_parametersFamily;
                std::vector<VariableDeclaration*> m_oldCounters;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_DISTRIBUTED_INDEX_BUILDER_H_INCLUDED
