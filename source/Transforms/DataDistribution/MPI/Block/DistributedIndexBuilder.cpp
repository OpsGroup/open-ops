#include "Transforms/DataDistribution/MPI/Block/DistributedIndexBuilder.h"

#include "Shared/ExpressionHelpers.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace OPS::Reprise;
using namespace OPS::Shared;
using namespace OPS::Shared::ExpressionHelpers;

DistributedIndexBuilder::DistributedIndexBuilder(ExpressionBase& sourceExpression, BADDParametersFamily& parametersFamily, const std::vector<VariableDeclaration*>& oldCounters) :
    m_sourceExpression(sourceExpression),
    m_parametersFamily(parametersFamily),
    m_oldCounters(oldCounters)
{

}

bool DistributedIndexBuilder::analyseApplicability()
{
    m_analysisPerformed = true;
    return true;
}

ReprisePtr<ExpressionBase> DistributedIndexBuilder::buildDistributedIndex(const std::vector<VariableDeclaration*>& blockCounters)
{
    if(!m_analysisPerformed)
    {
        analyseApplicability();
    }
    OPS_ASSERT(m_errors.size() == 0);

    OPS_ASSERT(blockCounters.size() == m_oldCounters.size());

    IntegerHelper indexIh(BasicType::BT_INT32);
    LinearExpressionMatrix linearExpressionMatrix(&m_sourceExpression);
    std::tr1::shared_ptr<ParametricLinearExpression> spPle(ParametricLinearExpression::createByListOfVariables(linearExpressionMatrix[m_parametersFamily.dims.size() - 1].convert2RepriseExpression(), m_oldCounters));
    ReprisePtr<ExpressionBase> rpResult = getBlockIndex(linearExpressionMatrix[m_parametersFamily.dims.size() - 1], blockCounters);
    for(int dimensionNumber = m_parametersFamily.dims.size() - 2; dimensionNumber >= 0; --dimensionNumber)
    {
        spPle.reset(ParametricLinearExpression::createByListOfVariables(linearExpressionMatrix[dimensionNumber].convert2RepriseExpression(), m_oldCounters));
        ReprisePtr<ExpressionBase> blockIndex = getBlockIndex(*spPle, blockCounters);
        rpResult.reset(&(
                op(blockIndex) * indexIh(getElementsInDimension(dimensionNumber)) + op(rpResult)
            ));
    }

    return rpResult;
}

std::string DistributedIndexBuilder::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

ReprisePtr<ExpressionBase> DistributedIndexBuilder::getBlockIndex(ParametricLinearExpression& parametricLinearExpression, const std::vector<VariableDeclaration*>& blockCounters)
{
    int oldCounterIndex = getOldCounterIndexForDimension(parametricLinearExpression);
    if(oldCounterIndex == -1)
    {
        return ReprisePtr<ExpressionBase>(parametricLinearExpression.convert2RepriseExpression());
    }

    OPS_ASSERT(blockCounters[oldCounterIndex] != NULL);
    return ReprisePtr<ExpressionBase>(&(
            op(new ReferenceExpression(*blockCounters[oldCounterIndex])) + op(parametricLinearExpression.getFreeCoefficient())
        ));
}

int DistributedIndexBuilder::getOldCounterIndexForDimension(ParametricLinearExpression& parametricLinearExpression)
{
	for(size_t oldCountersIndex = 0; oldCountersIndex < m_oldCounters.size(); ++oldCountersIndex)
    {
        if(parametricLinearExpression.getCoefficientAsInteger(m_oldCounters[oldCountersIndex]) == 1)
        {
            return oldCountersIndex;
        }
    }

    return -1;
}

int DistributedIndexBuilder::getElementsInDimension(int dimensionNumber)
{
    int result = 1;
	for(size_t i = dimensionNumber + 1; i < m_parametersFamily.dims.size(); ++i)
    {
        result = result * (m_parametersFamily.leftOverlapping[i] + m_parametersFamily.d[i] + m_parametersFamily.rightOverlapping[i]);
    }
    return result;
}

}
}
}
