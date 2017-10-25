#ifndef BLOCK_ITERATION_H_INCLUDED
#define BLOCK_ITERATION_H_INCLUDED

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"
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
    using OPS::Reprise::ForStatement;
    using OPS::Reprise::ReprisePtr;
    using OPS::Reprise::StatementBase;
    using OPS::Reprise::SubroutineDeclaration;
    using OPS::Reprise::VariableDeclaration;

    class BlockIteration: public IMPIDataDistributionIteration, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        BlockIteration(
            BADDParametersFamily distributionParameters,
            BADDParameters generatorParameters,
            VariableDeclaration& targetArrayDeclaration,
            ForStatement& iterationStatement);

        virtual ~BlockIteration();

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

        // IMPIDataDistributionScatter implementation
        void setNewArrayDeclaration(VariableDeclaration& newArrayDeclaration) { m_pNewArrayDeclaration = &newArrayDeclaration; }

    private:
        void initRankDeclaration();
        void initBlockIndexDeclarations(BlockStatement& blockStatement);

        void populateExchangeByDimensionBlock(BlockStatement& exchangeByDimentionBlock, int dimensionNumber);
        bool isSendToLeftRequired(int dimensionNumber);
        bool isSendToRightRequired(int dimensionNumber);

        void addSendStatements(BlockStatement& sendBlock, BlockStatement& bufferFreeBlock, VariableDeclaration& bufferDeclaration, VariableDeclaration& requestDeclaration, VariableDeclaration& statusDeclaration, int sendDimensionNumber, bool isLeft);
        void addReceiveStatements(BlockStatement& receiveBlock, VariableDeclaration& bufferDeclaration, VariableDeclaration& statusDeclaration, int receiveDimensionNumber, bool isLeft);

        void addBufferCopyingStatemens(const std::vector<int>& inits, const std::vector<int> finals, VariableDeclaration& bufferDeclaration, BlockStatement& blockToAdd, bool isSend);

        int getBufferSize(const std::vector<int>& inits, const std::vector<int> finals);

        ReprisePtr<StatementBase> createMemoryAllocatingStatement(ReprisePtr<ExpressionBase> pointerExpression, int sizeInElements, BasicType& typeOfElements);
        ReprisePtr<StatementBase> createMemoryDeallocatingStatement(ReprisePtr<ExpressionBase> pointerExpression);
        ReprisePtr<StatementBase> buildWaitStatement(VariableDeclaration& requestDeclaration, VariableDeclaration& statusDeclaration);
        ReprisePtr<StatementBase> buildISendStatement(VariableDeclaration& bufferDeclaration, int bufferSize, ReprisePtr<ExpressionBase> rpRankExpression, VariableDeclaration& requestDeclaration);
        ReprisePtr<StatementBase> buildRecvStatement(VariableDeclaration& bufferDeclaration, int bufferSize, ReprisePtr<ExpressionBase> rpRankExpression, VariableDeclaration& statusDeclaration);
        ReprisePtr<ExpressionBase> buildRankExpression(int dimensionNumber, bool isLeft);

    private:
        MPIProducerFactory* m_pFactory;
        MPIHelper* m_pHelper;
        IMPIRankProducer* m_pRankProducer;

        BADDParametersFamily m_distributionParameters;
        BADDParameters m_generatorParameters;
        VariableDeclaration& m_targetArrayDeclaration;
        ForStatement& m_iterationStatement;

        VariableDeclaration* m_pNewArrayDeclaration;

        VariableDeclaration* m_pRankDeclaration;
        BasicType* m_pArrayElementBasicType;
        SubroutineDeclaration* m_pMallocDeclaration;
        SubroutineDeclaration* m_pFreeDeclaration;
        std::vector<VariableDeclaration*> m_blockIndexDeclarations;

        std::list<std::string> m_errors;
        bool m_analysisRerformed;
    };
}
}
}

#endif // BLOCK_ITERATION_H_INCLUDED
