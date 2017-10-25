#ifndef I_MPI_DATA_DISTRIBUTION_BLOCKING_DESCRIPTION_INCLUDED
#define I_MPI_DATA_DISTRIBUTION_BLOCKING_DESCRIPTION_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/MPI/Block/BlockingDescription.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::VariableDeclaration;

            class BlockingDescription
            {
            public:
                typedef std::vector<int> Dimensions;

            public:
                BlockingDescription(int processorsCount, Dimensions dimensions);
                BlockingDescription(const std::vector<int> blockWidths, const std::vector<int> iterationCounts);

                int getProcessorsCount();
                Dimensions& getDimensions();

                int getBlocksCount(int dimensionNumber);

                std::string getBlockingDescriptionDimentionString(int dimensionNumber);

            private:
                int m_processorsCount;
                Dimensions m_dimensions;
            };
        }
    }
}

#endif // I_MPI_DATA_DISTRIBUTION_BLOCKING_DESCRIPTION_INCLUDED
