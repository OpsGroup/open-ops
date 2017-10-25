#include "Transforms/DataDistribution/MPI/Block/BlockIndexesProducer.h"

#include "Shared/ExpressionHelpers.h"
#include "Transforms/DataDistribution/MPI/Block/BlocksHelper.h"
#include "Transforms/MPI/Utils/MPIHelper.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace OPS::Reprise;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Transforms::MPIProducer;

BlockIndexesProducer::BlockIndexesProducer(SubroutineDeclaration& entryPoint, BlockingDescription blockingDescription) :
    m_pEntryPoint(&entryPoint),
    m_blockingDescription(blockingDescription)
{

}

bool BlockIndexesProducer::analyseApplicability()
{
    m_analysisPerformed = true;
    return true;
}

void BlockIndexesProducer::makeTransformation()
{
    if(!m_analysisPerformed)
    {
        analyseApplicability();
    }
    OPS_ASSERT(m_errors.size() == 0);

    m_blockRanks.resize(m_blockingDescription.getDimensions().size());

    VariableDeclaration* pRankDeclaration = MPIHelper::getRankDeclaration(*m_pEntryPoint);
    OPS_ASSERT(pRankDeclaration != NULL);

    declareIndexes(*pRankDeclaration);
    initializeIndexes(*pRankDeclaration);
}

std::string BlockIndexesProducer::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

void BlockIndexesProducer::declareIndexes(VariableDeclaration& rankDeclaration)
{
    int dimensionsCount = m_blockingDescription.getDimensions().size();

    for(int dimensionNumber = 0; dimensionNumber < dimensionsCount; ++dimensionNumber)
    {
        m_blockRanks[dimensionNumber] = &Editing::createNewVariable(rankDeclaration.getType(), m_pEntryPoint->getBodyBlock(), "BlockIndex" + dimensionNumber);
        m_blockRanks[dimensionNumber]->setNote(BlocksHelper::BLOCK_INDEX_NOTE_NAME, Note::newString(m_blockingDescription.getBlockingDescriptionDimentionString(dimensionNumber)));
    }
}

void BlockIndexesProducer::initializeIndexes(VariableDeclaration& rankDeclaration)
{
    ReprisePtr<BlockStatement> rpIndexInitBlock(new BlockStatement());

    int dimensionsCount = m_blockingDescription.getDimensions().size();
    for(int dimensionNumber = 0; dimensionNumber < dimensionsCount; ++dimensionNumber)
    {
        initializeIndex(rankDeclaration, m_blockRanks[dimensionNumber], rpIndexInitBlock, dimensionNumber);
    }

    BlockStatement* pGlobalsInitBlock = MPIHelper::getGlobalsInitBlock(*m_pEntryPoint);
    OPS_ASSERT(pGlobalsInitBlock != NULL);

    BlockStatement& parentBlock = pGlobalsInitBlock->getParentBlock();
    parentBlock.addAfter(
            parentBlock.convertToIterator(pGlobalsInitBlock),
                rpIndexInitBlock.get()
        );
}

void BlockIndexesProducer::initializeIndex(VariableDeclaration& rankDeclaration, VariableDeclaration* pBlockIndexDeclaration, ReprisePtr<BlockStatement> rpIndexInitBlock, int dimensionNumber)
{
    int currentDimensionBlocksCount = m_blockingDescription.getBlocksCount(dimensionNumber);
    int nextDimensionBlocksCount= m_blockingDescription.getBlocksCount(dimensionNumber + 1);

    OPS_ASSERT(rankDeclaration.getType().is_a<BasicType>());
    IntegerHelper indexIh(rankDeclaration.getType().cast_to<BasicType>());

    ReprisePtr<ExpressionBase> rpAssgnExpression(&(
            op(new ReferenceExpression(*pBlockIndexDeclaration))
                R_AS
            (op(new ReferenceExpression(rankDeclaration)) % indexIh(currentDimensionBlocksCount)) / indexIh(nextDimensionBlocksCount)
        ));
    rpIndexInitBlock->addLast(new ExpressionStatement(rpAssgnExpression.get()));
}

std::vector<VariableDeclaration*> BlockIndexesProducer::getBlockRanks()
{
    return m_blockRanks;
}

}
}
}
