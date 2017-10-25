#ifndef MPI_BLOCK_INDEXES_PRODUCER_H_INCLUDED
#define MPI_BLOCK_INDEXES_PRODUCER_H_INCLUDED

#include "Transforms/DataDistribution/MPI/Block/BlockingDescription.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using Reprise::VariableDeclaration;
            using Reprise::BlockStatement;
            using Reprise::ReprisePtr;
            using Reprise::SubroutineDeclaration;

            class BlockIndexesProducer: ITransformation
            {
            public:
                BlockIndexesProducer(SubroutineDeclaration& entryPoint, BlockingDescription blockingDescription);

                // ITransformation implementation
                virtual bool analyseApplicability();
                virtual std::string getErrorMessage();
                virtual void makeTransformation();

                std::vector<VariableDeclaration*> getBlockRanks();

            private:

                void declareIndexes(VariableDeclaration& rankDeclaration);
                void initializeIndexes(VariableDeclaration& rankDeclaration);
                void initializeIndex(VariableDeclaration& rankDeclaration, VariableDeclaration* pBlockIndexDeclaration, ReprisePtr<BlockStatement> rpIndexInitBlock, int dimensionNumber);

            private:
                SubroutineDeclaration* m_pEntryPoint;
                BlockingDescription m_blockingDescription;

                std::vector<VariableDeclaration*> m_blockRanks;

                bool m_analysisPerformed;
                std::list<std::string> m_errors;
            };
        }
    }
}

#endif // MPI_BLOCK_INDEXES_PRODUCER_H_INCLUDED
