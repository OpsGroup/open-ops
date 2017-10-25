#include "Transforms/DataDistribution/MPI/Block/BufferCopyingBuilder.h"

#include "Shared/ExpressionHelpers.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;

BufferCopyingBuilder::BufferCopyingBuilder(
        SubroutineDeclaration& entryPoint,
        BADDParametersFamily parametersFamily,
        VariableDeclaration& distributedArrayDeclaration
    ): m_entryPoint(entryPoint), m_parametersFamily(parametersFamily), m_distributedArrayDeclaration(distributedArrayDeclaration),
    m_leftLoopIndexShifts(parametersFamily.dims.size()), m_rightLoopIndexShifts(parametersFamily.dims.size()), m_blockIndexCounters(parametersFamily.dims.size()), m_copyDirection(BufferCopyingBuilder::CD_TO_BUFFER)
{
	for(size_t loopDimensionNumber = 0; loopDimensionNumber < parametersFamily.dims.size(); ++loopDimensionNumber)
    {
        m_leftLoopIndexShifts[loopDimensionNumber] = -parametersFamily.leftOverlapping[loopDimensionNumber];
        m_rightLoopIndexShifts[loopDimensionNumber] = -parametersFamily.rightOverlapping[loopDimensionNumber];
        m_blockIndexCounters[loopDimensionNumber] = NULL;
    }
}

BufferCopyingBuilder::BufferCopyingBuilder(
        SubroutineDeclaration& entryPoint,
        BADDParametersFamily parametersFamily,
        BADDParameters generatorParameters,
        VariableDeclaration& distributedArrayDeclaration
    ): m_entryPoint(entryPoint), m_parametersFamily(parametersFamily), m_distributedArrayDeclaration(distributedArrayDeclaration),
    m_leftLoopIndexShifts(parametersFamily.dims.size()), m_rightLoopIndexShifts(parametersFamily.dims.size()), m_blockIndexCounters(parametersFamily.dims.size()), m_copyDirection(BufferCopyingBuilder::CD_FROM_BUFFER)
{
	for(size_t loopDimensionNumber = 0; loopDimensionNumber < parametersFamily.dims.size(); ++loopDimensionNumber)
    {
        m_leftLoopIndexShifts[loopDimensionNumber] = generatorParameters.a[loopDimensionNumber];
        m_rightLoopIndexShifts[loopDimensionNumber] = generatorParameters.a[loopDimensionNumber];
        m_blockIndexCounters[loopDimensionNumber] = NULL;
    }
}

bool BufferCopyingBuilder::analyseApplicability()
{
    m_analysisPerformed = true;
    return true;
}

std::string BufferCopyingBuilder::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

ReprisePtr<StatementBase> BufferCopyingBuilder::buildBufferCopying(ReprisePtr<ExpressionBase> rpBufferReference, const std::vector<VariableDeclaration*>& blockRanks)
{
    if(!m_analysisPerformed)
    {
        analyseApplicability();
    }
    OPS_ASSERT(m_errors.size() == 0);

    std::vector<VariableDeclaration*> blockIndexes(blockRanks.size());
    return buildBlockCopyingLoops(
            rpBufferReference,
            blockRanks,
            blockIndexes,
            0
        );
}

ReprisePtr<StatementBase> BufferCopyingBuilder::buildBlockCopyingLoops(
        ReprisePtr<ExpressionBase> rpBufferReference,
        const std::vector<VariableDeclaration*>& blockRanks,
        std::vector<VariableDeclaration*>& blockIndexes,
        int loopDimensionNumber
    )
{
    ReprisePtr<BlockStatement> rpResultBlock(new BlockStatement());

	if(loopDimensionNumber == int(m_parametersFamily.dims.size()))
    {
        ReprisePtr<StatementBase> rpSingleElementCopyStatement = buildSingleElementCopyStatement(
                rpBufferReference,
                blockRanks,
                blockIndexes
            );
        rpResultBlock->addLast(rpSingleElementCopyStatement.get());
    }
    else
    {
        VariableDeclaration& counter = getOrCreateCounter(loopDimensionNumber);
        std::vector<VariableDeclaration*> newBlockIndexes(blockIndexes);
        newBlockIndexes[loopDimensionNumber] = &counter;

        if(m_leftLoopIndexShifts[loopDimensionNumber] < 0)
        {
            ReprisePtr<ForStatement> rpForStatementLeft = buildLoop(m_leftLoopIndexShifts[loopDimensionNumber], 0, counter);
            rpForStatementLeft->getBody().addLast(
                    buildBlockCopyingLoops(rpBufferReference, blockRanks, newBlockIndexes, loopDimensionNumber + 1).get()
                );
            ReprisePtr<IfStatement> rpIfStatementLeft = buildIf(*blockRanks[loopDimensionNumber], BasicCallExpression::BCK_GREATER, 0);
            rpIfStatementLeft->getThenBody().addLast(rpForStatementLeft.get());
            rpResultBlock->addLast(rpIfStatementLeft.get());
        }

        ReprisePtr<ForStatement> rpForStatement = buildLoop(
                m_leftLoopIndexShifts[loopDimensionNumber] > 0 ? m_leftLoopIndexShifts[loopDimensionNumber] : 0,
                m_rightLoopIndexShifts[loopDimensionNumber] < 0 ? m_parametersFamily.d[loopDimensionNumber] + m_rightLoopIndexShifts[loopDimensionNumber] : m_parametersFamily.d[loopDimensionNumber],
                counter
            );
        rpForStatement->getBody().addLast(
                buildBlockCopyingLoops(rpBufferReference, blockRanks, newBlockIndexes, loopDimensionNumber + 1).get()
            );
        rpResultBlock->addLast(rpForStatement.get());

        if(m_rightLoopIndexShifts[loopDimensionNumber] > 0)
        {
            ReprisePtr<ForStatement> rpForStatementRight = buildLoop(
                    m_parametersFamily.d[loopDimensionNumber],
                    m_parametersFamily.d[loopDimensionNumber] + m_rightLoopIndexShifts[loopDimensionNumber],
                    counter
                );
            rpForStatementRight->getBody().addLast(
                    buildBlockCopyingLoops(rpBufferReference, blockRanks, newBlockIndexes, loopDimensionNumber + 1).get()
                );
            ReprisePtr<IfStatement> rpIfStatementRight = buildIf(
                    *blockRanks[loopDimensionNumber],
                    BasicCallExpression::BCK_LESS,
                    m_parametersFamily.dims[loopDimensionNumber] / m_parametersFamily.d[loopDimensionNumber] - 1
                );
            rpIfStatementRight->getThenBody().addLast(rpForStatementRight.get());
            rpResultBlock->addLast(rpIfStatementRight.get());
        }
    }

    return rpResultBlock;
}

ReprisePtr<IfStatement> BufferCopyingBuilder::buildIf(VariableDeclaration& leftOperand, BasicCallExpression::BuiltinCallKind operatorKind, int rightOperand)
{
    IntegerHelper ih(BasicType::BT_INT32);
    ReprisePtr<IfStatement> rpResultIf(new IfStatement(
            new BasicCallExpression(operatorKind, new ReferenceExpression(leftOperand), &ih(rightOperand))
        ));
    return rpResultIf;
}

ReprisePtr<ForStatement> BufferCopyingBuilder::buildLoop(int initCounterValue, int finalCounterValue, VariableDeclaration& counter)
{
    IntegerHelper ih(BasicType::BT_INT32);

    ReprisePtr<ExpressionBase> rpCounterReference(new ReferenceExpression(counter));
    ReprisePtr<ForStatement> rpForStatement(new ForStatement());
    rpForStatement->setInitExpression(&(
            op(rpCounterReference) R_AS ih(initCounterValue)
        ));
    rpForStatement->setFinalExpression(&(
            op(rpCounterReference) < ih(finalCounterValue)
        ));
    rpForStatement->setStepExpression(&(
            op(rpCounterReference) R_AS op(rpCounterReference) + ih(1)
        ));

    return rpForStatement;
}

ReprisePtr<StatementBase> BufferCopyingBuilder::buildSingleElementCopyStatement(
        ReprisePtr<ExpressionBase> rpBufferReference,
        const std::vector<VariableDeclaration*>& blockRanks,
        std::vector<VariableDeclaration*>& blockIndexes
    )
{
    IntegerHelper ih(BasicType::BT_INT32);

    ReprisePtr<ExpressionBase> rpDistributedArrayExpression(new ReferenceExpression(m_distributedArrayDeclaration));
	for(size_t dimensionNumber = 0; dimensionNumber < m_parametersFamily.dims.size(); ++dimensionNumber)
    {
        ReprisePtr<ExpressionBase> rpDistributedArrayIndex(&(
                *(new ReferenceExpression(*blockRanks[dimensionNumber])) * ih(m_parametersFamily.d[dimensionNumber]) + *(new ReferenceExpression(*blockIndexes[dimensionNumber]))
            ));
        rpDistributedArrayExpression.reset(&(
                op(rpDistributedArrayExpression) R_BK (op(rpDistributedArrayIndex))
            ));
    }
    ReprisePtr<ExpressionBase> rpBufferIndexExpression(&(
            *(new ReferenceExpression(*blockIndexes[0])) + ih(m_parametersFamily.leftOverlapping[0])
        ));
    if(m_parametersFamily.leftOverlapping[0] > 0)
    {
        rpBufferIndexExpression.reset(&(op(rpBufferIndexExpression) + ih(m_parametersFamily.leftOverlapping[0])));
    }
	for(size_t dimensionNumber = 1; dimensionNumber < m_parametersFamily.dims.size(); ++dimensionNumber)
    {
        rpBufferIndexExpression.reset(&(
                op(rpBufferIndexExpression)
                    *
                ih(m_parametersFamily.dims[dimensionNumber] + m_parametersFamily.leftOverlapping[dimensionNumber] + m_parametersFamily.rightOverlapping[dimensionNumber])
                    +
                *(new ReferenceExpression(*m_blockIndexCounters[dimensionNumber]))
            ));
        if(m_parametersFamily.leftOverlapping[dimensionNumber] > 0)
        {
            rpBufferIndexExpression.reset(&(op(rpBufferIndexExpression) + ih(m_parametersFamily.leftOverlapping[dimensionNumber])));
        }
    }
    ReprisePtr<ExpressionBase> rpBufferExpression(&( op(rpBufferReference) R_BK(op(rpBufferIndexExpression)) ));

    if(m_copyDirection == CD_TO_BUFFER)
    {
        return ReprisePtr<StatementBase>(new ExpressionStatement(&(
                op(rpBufferExpression) R_AS op(rpDistributedArrayExpression)
            )));
    }

    return ReprisePtr<StatementBase>(new ExpressionStatement(&(
            op(rpDistributedArrayExpression) R_AS op(rpBufferExpression)
        )));
}

VariableDeclaration& BufferCopyingBuilder::getOrCreateCounter(int loopDimensionNumber)
{
    if(m_blockIndexCounters[loopDimensionNumber] == NULL)
    {
        m_blockIndexCounters[loopDimensionNumber] = &Editing::createNewVariable(*BasicType::int32Type(), m_entryPoint.getBodyBlock(), "_block_index_counter_");
    }
    return *m_blockIndexCounters[loopDimensionNumber];
}

}
}
}
