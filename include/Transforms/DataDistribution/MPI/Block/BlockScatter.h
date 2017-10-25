#ifndef BLOCK_SCATTER_H_INCLUDED
#define BLOCK_SCATTER_H_INCLUDED

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"
#include "Transforms/DataDistribution/MPI/IMPIDataDistributionScatter.h"

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
    using OPS::Reprise::ForStatement;
    using OPS::Reprise::ExpressionBase;
    using OPS::Reprise::ReprisePtr;
    using OPS::Reprise::StatementBase;
    using OPS::Reprise::SubroutineDeclaration;
    using OPS::Reprise::TypeBase;
    using OPS::Reprise::VariableDeclaration;

    class BlockScatter: public IMPIDataDistributionScatter, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        BlockScatter(
            BADDParametersFamily distributionParameters,
            VariableDeclaration& targetArrayDeclaration,
            StatementBase& insertionPointStatement,
            int sourceNodeNumber);

        virtual ~BlockScatter();

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

        // IMPIDataDistributionScatter implementation
        void setNewArrayDeclaration(VariableDeclaration& newArrayDeclaration) { m_pNewArrayDeclaration = &newArrayDeclaration; }

    private:
        void initRankDeclaration();

        void populateSendingBlock(BlockStatement& sendingBlock);
        VariableDeclaration& createBuffer2DimArray(BlockStatement& sendingBlock);
        ReprisePtr<StatementBase> buildBufferInitializingStatement(VariableDeclaration& buffersDeclaration, VariableDeclaration& processesCounter);
        VariableDeclaration& createMpiRequestArray(BlockStatement& sendingBlock);
        std::vector<VariableDeclaration*> createBlockRankDeclarations(BlockStatement& sendingBlock);
        BlockStatement& buildLoopNestByBlocks(BlockStatement& parent, const std::vector<VariableDeclaration*>& blockRankDeclarations, int dimension = 0);
        ReprisePtr<StatementBase> buildISendStatement(const std::vector<VariableDeclaration*>& blockRankDeclarations, VariableDeclaration& buffersDeclaration, VariableDeclaration& requestsDeclaration);
        ReprisePtr<StatementBase> buildWaitStatements(VariableDeclaration& requestsDeclaration, VariableDeclaration& statusDeclaration, VariableDeclaration& processesCounter);
        ReprisePtr<StatementBase> buildBufferFreeStatements(VariableDeclaration& buffersDeclaration, VariableDeclaration& processesCounter);

        void populateReceivingBlock(BlockStatement& receivingBlock);

        ReprisePtr<ForStatement> buildLoop(int initCounterValue, int finalCounterValue, VariableDeclaration& counter);
        ReprisePtr<ExpressionBase> buildRankByBlockRanks(const std::vector<VariableDeclaration*>& blockRankDeclarations);
        ReprisePtr<StatementBase> createMemoryAllocatingStatement(ReprisePtr<ExpressionBase> pointerExpression, int sizeInElements, BasicType& typeOfElements);
        ReprisePtr<StatementBase> createMemoryDeallocatingStatement(ReprisePtr<ExpressionBase> pointerExpression);

    private:
        MPIProducerFactory* m_pFactory;
        MPIHelper* m_pHelper;
        IMPIRankProducer* m_pRankProducer;

        BADDParametersFamily m_distributionParameters;
        VariableDeclaration& m_targetArrayDeclaration;
        VariableDeclaration* m_pNewArrayDeclaration;
        StatementBase& m_insertionPointStatement;

        VariableDeclaration* m_pRankDeclaration;
        BasicType* m_pArrayElementBasicType;

        std::list<std::string> m_errors;

        int m_sourceNodeNumber;

        SubroutineDeclaration* m_pMallocDeclaration;
        SubroutineDeclaration* m_pFreeDeclaration;

        bool m_analysisRerformed;
    };
}
}
}

#endif // BLOCK_SCATTER_H_INCLUDED
