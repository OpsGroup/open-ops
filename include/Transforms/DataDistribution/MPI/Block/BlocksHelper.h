#ifndef I_MPI_DATA_DISTRIBUTION_BLOCKS_HELPER_INCLUDED
#define I_MPI_DATA_DISTRIBUTION_BLOCKS_HELPER_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/MPI/Block/BlockingDescription.h"

namespace OPS
{
    namespace Transforms
    {
        namespace DataDistribution
        {
            using OPS::Reprise::VariableDeclaration;
            using OPS::Reprise::SubroutineDeclaration;

            class BlocksHelper
            {
            public:
                static const std::string BLOCK_INDEX_NOTE_NAME;
                static const std::string OLD_EXPRESSION_NOTE_NAME;
                static const std::string PARAMETERS_FAMILY_NOTE_NAME;

                static bool findBlockIndexDeclarations(BlockingDescription& blockingDescription, SubroutineDeclaration& subroutineDeclaration, std::vector<VariableDeclaration*>& foundDeclarations);
            };
        }
    }
}

#endif // I_MPI_DATA_DISTRIBUTION_BLOCKS_HELPER_INCLUDED
