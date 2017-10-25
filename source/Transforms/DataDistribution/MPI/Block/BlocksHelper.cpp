#include "Transforms/DataDistribution/MPI/Block/BlocksHelper.h"

#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    using namespace OPS::Reprise;
    using namespace OPS::Reprise::Service;

    class BlockIndexDeclarationFinderDeepWalker: public DeepWalker
    {
    public:
        BlockIndexDeclarationFinderDeepWalker(const std::string& searchString): DeepWalker(), m_searchString(searchString), m_pBlockIndexDeclaration(NULL)
        {
        }

        void visit(VariableDeclaration& variableDeclaration)
        {
            if(variableDeclaration.hasNote(BlocksHelper::BLOCK_INDEX_NOTE_NAME) && variableDeclaration.getNote(BlocksHelper::BLOCK_INDEX_NOTE_NAME).getString() == m_searchString)
            {
                m_pBlockIndexDeclaration = &variableDeclaration;
            }
        }

        VariableDeclaration* getBlockIndexDeclaration()
        {
            return m_pBlockIndexDeclaration;
        }

    private:
        const std::string& m_searchString;
        VariableDeclaration* m_pBlockIndexDeclaration;
    };

    const std::string BlocksHelper::BLOCK_INDEX_NOTE_NAME = "BLOCK_INDEX_NOTE_NAME";
    const std::string BlocksHelper::OLD_EXPRESSION_NOTE_NAME = "OLD_EXPRESSION_NOTE_NAME";
    const std::string BlocksHelper::PARAMETERS_FAMILY_NOTE_NAME = "PARAMETERS_FAMILY_NOTE_NAME";

    bool BlocksHelper::findBlockIndexDeclarations(BlockingDescription& blockingDescription, SubroutineDeclaration& subroutineDeclaration, std::vector<VariableDeclaration*>& foundDeclarations)
    {
        int dimensionsCount = blockingDescription.getDimensions().size();
        foundDeclarations.resize(dimensionsCount);

        for(int dimensionNumber = 0; dimensionNumber < dimensionsCount; ++dimensionNumber)
        {
            BlockIndexDeclarationFinderDeepWalker dw(blockingDescription.getBlockingDescriptionDimentionString(dimensionNumber));
            subroutineDeclaration.accept(dw);

            if(dw.getBlockIndexDeclaration() == NULL)
            {
                return false;
            }

            foundDeclarations[dimensionNumber] = dw.getBlockIndexDeclaration();
        }

        return true;
    }
}
}
}
