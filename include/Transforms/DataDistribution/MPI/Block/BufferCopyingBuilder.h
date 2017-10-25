#ifndef MPI_BUFFER_COPYING_BUILDER_H_INCLUDED
#define MPI_BUFFER_COPYING_BUILDER_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::BasicCallExpression;
            using OPS::Reprise::ExpressionBase;
            using OPS::Reprise::ForStatement;
            using OPS::Reprise::IfStatement;
            using OPS::Reprise::ReprisePtr;
            using OPS::Reprise::StatementBase;
            using OPS::Reprise::SubroutineDeclaration;
            using OPS::Reprise::VariableDeclaration;

            class BufferCopyingBuilder
            {
            public:
                BufferCopyingBuilder(
                        SubroutineDeclaration& entryPoint,
                        BADDParametersFamily m_parametersFamily,
                        VariableDeclaration& m_distributedArrayDeclaration
                    );
                BufferCopyingBuilder(
                        SubroutineDeclaration& entryPoint,
                        BADDParametersFamily m_parametersFamily,
                        BADDParameters generatorParameters,
                        VariableDeclaration& m_distributedArrayDeclaration
                    );

                bool analyseApplicability();
                std::string getErrorMessage();
                ReprisePtr<StatementBase> buildBufferCopying(ReprisePtr<ExpressionBase> rpBufferReference, const std::vector<VariableDeclaration*>& blockRanks);

            private:
                enum CopyDirection
                {
                    CD_TO_BUFFER = 0,
                    CD_FROM_BUFFER = 1
                };

            private:
                ReprisePtr<StatementBase> buildBlockCopyingLoops(
                        ReprisePtr<ExpressionBase> rpBufferReference,
                        const std::vector<VariableDeclaration*>& blockRanks,
                        std::vector<VariableDeclaration*>& blockIndexes,
                        int loopDimensionNumber
                    );
                ReprisePtr<StatementBase> buildSingleElementCopyStatement(
                        ReprisePtr<ExpressionBase> rpBufferReference,
                        const std::vector<VariableDeclaration*>& blockRanks,
                        std::vector<VariableDeclaration*>& blockIndexes
                    );
                ReprisePtr<IfStatement> buildIf(VariableDeclaration& leftOperand, BasicCallExpression::BuiltinCallKind operatorKind, int rightOperand);
                ReprisePtr<ForStatement> buildLoop(int initCounterValue, int finalCounterValue, VariableDeclaration& counter);
                VariableDeclaration& getOrCreateCounter(int loopDimensionNumber);

            private:
                SubroutineDeclaration& m_entryPoint;
                BADDParametersFamily m_parametersFamily;
                VariableDeclaration& m_distributedArrayDeclaration;

                std::vector<int> m_leftLoopIndexShifts;
                std::vector<int> m_rightLoopIndexShifts;
                std::vector<VariableDeclaration*> m_blockIndexCounters;
                CopyDirection m_copyDirection;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_BUFFER_COPYING_BUILDER_H_INCLUDED
