#ifndef MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED
#define MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"
#include "Transforms/ITransformation.h"
#include "Transforms/DataDistribution/MPI/IMPIDataDistributionIteration.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Transforms::MPIProducer::MPIProducerFactory;
            using OPS::Transforms::MPIProducer::MPIHelper;
            using OPS::Transforms::MPIProducer::IMPIRankProducer;
            using OPS::Reprise::BasicType;
            using OPS::Reprise::BlockStatement;
            using OPS::Reprise::ExpressionBase;
            using OPS::Reprise::ReprisePtr;
            using OPS::Reprise::StatementBase;
            using OPS::Reprise::ForStatement;
            using OPS::Reprise::SubroutineDeclaration;
            using OPS::Reprise::TypeBase;
            using OPS::Reprise::VariableDeclaration;

            class MPIDataDistributionIteration: public IMPIDataDistributionIteration, public OPS::NonCopyableMix, public OPS::NonAssignableMix
            {
            public:
                // Constructor
                MPIDataDistributionIteration(
                    BADDParametersFamily distributionParameters, 
                    BADDParameters generatorParameters,
                    VariableDeclaration& targetArrayDeclaration, 
                    ForStatement& iterationStatement, 
                    int leadingDimention);

                // Destructor
                ~MPIDataDistributionIteration();

                // ITransformation implementation
                virtual bool analyseApplicability();
                virtual std::string getErrorMessage();
                virtual void makeTransformation();

                // IMPIDataDistributionIteration implementation
                virtual void setNewArrayDeclaration(VariableDeclaration& newArrayDeclaration);

            private:
                typedef std::vector<int> NeightbourInfo;
                struct NeightbourSendInfo
                {
                    NeightbourInfo m_neightbourInfo;
                    int m_elementsInBlockToSendCount;
                    int m_elementsToSendCount;
                    int m_rankOffset;
                };
                struct NeightbourRankSendInfo
                {
                    std::list<NeightbourSendInfo> m_neightbourInfos;
                    int m_elementsToSendCount;
                    int m_rankOffset;
                    VariableDeclaration* m_bufferDeclaration;
                    VariableDeclaration* m_pRequestDeclaration;
                };
                std::list<NeightbourInfo> getAllNeightbours();
                std::list<NeightbourInfo> getNeightboursByDimention(int dimention, int dimentionsCount, const std::list<NeightbourInfo>& oldNeightbours);
                std::list<NeightbourSendInfo> getAllNeightbourSendInfo();
                std::list<NeightbourRankSendInfo> getAllNeightbourRankSendInfo();
                int getRankOffset(const NeightbourInfo& neightbour);
                int getElementsToSendCount(const NeightbourInfo& neightbour);
                int getStartIndex(int dimention, const NeightbourInfo& neightbour);
                int getEndIndex(int dimention, const NeightbourInfo& neightbour);
                int getReceivingStartIndex(int dimention, const NeightbourInfo& neightbour);
                int getReceivingEndIndex(int dimention, const NeightbourInfo& neightbour);
                int getBlockIndexInEmbedding(BADDParametersFamily::MultyIndex blockMultyIndex);
                int getElementsInCellInEmbeddingCount();
                ReprisePtr<ExpressionBase> getSendingBufferArrayIndex(
                    const std::vector<VariableDeclaration*>& cellIndexes,
                    const std::vector<VariableDeclaration*>& partsIndexes,
                    const BADDParametersFamily::MultyIndex& blockMultyIndex,
                    const MPIDataDistributionIteration::NeightbourSendInfo& neightbourSendInfo
                    );
                ReprisePtr<ExpressionBase> getSourceArrayIndex(
                    const std::vector<VariableDeclaration*>& cellIndexes,
                    const std::vector<VariableDeclaration*>& partsIndexes,
                    const BADDParametersFamily::MultyIndex& blockMultyIndex
                    );

                ReprisePtr<ExpressionBase> getTargetArrayIndex(
                    VariableDeclaration& blockIndex,
                    const std::vector<VariableDeclaration*>& partsIndexes
                    );
                ReprisePtr<ExpressionBase> getReceivingBufferArrayIndex(
                    VariableDeclaration& blockIndex,
                    const std::vector<VariableDeclaration*>& partsIndexes,
                    const MPIDataDistributionIteration::NeightbourSendInfo& neightbourSendInfo
                    );

                std::list<NeightbourRankSendInfo> addSendStatements(ReprisePtr<BlockStatement> rpOutputBlock, VariableDeclaration& status);
                void addReceiveStatements(ReprisePtr<BlockStatement> rpOutputBlock);

                ReprisePtr<StatementBase> createMemoryAllocatingStatement(VariableDeclaration& variableDeclaration, int sizeInElements, BasicType& typeOfElements);
                ReprisePtr<StatementBase> createMemoryDeallocatingStatement(VariableDeclaration& variableDeclaration);
                ReprisePtr<StatementBase> createMPIISendStatement(ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& dest, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request);
                ReprisePtr<StatementBase> createMPIIRecvStatement(ExpressionBase& buf, ExpressionBase& count, ExpressionBase& datatype, ExpressionBase& source, ExpressionBase& tag, ExpressionBase& comm, ExpressionBase& request);
                ReprisePtr<StatementBase> createMPIWaitStatement(ExpressionBase& request, ExpressionBase& status);

            private:
                MPIProducerFactory* m_pFactory;
                MPIHelper* m_pHelper;
                IMPIRankProducer* m_pRankProducer;
                BADDParameters m_generatorParameters;
                BADDParametersFamily m_distributionParameters;

                VariableDeclaration& m_targetArrayDeclaration;
                VariableDeclaration* m_pNewArrayDeclaration;
                ForStatement& m_iterationStatement;
                int m_leadingDimention;

                VariableDeclaration* m_pRankDeclaration;
                BasicType* m_pArrayElementBasicType;

                SubroutineDeclaration* m_pMallocDeclaration;
                SubroutineDeclaration* m_pFreeDeclaration;

                std::list<std::string> m_errors;
                bool m_analysisRerformed;
            };
        }
    }
}

#endif  // MPI_DATA_DISTRIBUTION_ITERATION_H_INCLUDED
