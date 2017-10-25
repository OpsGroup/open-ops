#include "Transforms/DataDistribution/MPI/Block/BlockingDescription.h"

#include <sstream>

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

BlockingDescription::BlockingDescription(int processorsCount, BlockingDescription::Dimensions dimensions) :
    m_processorsCount(processorsCount),
    m_dimensions(dimensions)
{

}

BlockingDescription::BlockingDescription(const std::vector<int> blockWidths, const std::vector<int> iterationCounts)
{
    OPS_ASSERT(blockWidths.size() == iterationCounts.size());

    m_processorsCount = 1;
    m_dimensions.resize(blockWidths.size());

	for(size_t dimensionNumber = 0; dimensionNumber < blockWidths.size(); ++dimensionNumber)
    {
        m_dimensions[dimensionNumber] = iterationCounts[dimensionNumber] / blockWidths[dimensionNumber];
        m_processorsCount = m_processorsCount * m_dimensions[dimensionNumber];
    }
}

int BlockingDescription::getProcessorsCount()
{
    return m_processorsCount;
}

BlockingDescription::Dimensions& BlockingDescription::getDimensions()
{
    return m_dimensions;
}

int BlockingDescription::getBlocksCount(int dimensionNumber)
{
    int result = 1;

	for(size_t i = dimensionNumber; i < m_dimensions.size(); ++i)
    {
        result = result * m_dimensions[i];
    }

    return result;
}

std::string BlockingDescription::getBlockingDescriptionDimentionString(int dimensionNumber)
{
    std::stringstream ss;
    ss<<m_processorsCount<<"|";
	for(size_t i = 0; i < m_dimensions.size(); ++i)
    {
        ss<<m_dimensions[i]<<",";
    }
    ss<<"|"<<dimensionNumber;

    return ss.str();
}

}
}
}
