#include "Transforms/Helpers/PrepareCadreDeclarations/PrepareCadreDeclarations.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/StatementsShared.h"

namespace OPS
{
namespace Transforms
{
namespace Helpers
{
using namespace Reprise;


class ReplaceWalker: public OPS::Reprise::Service::DeepWalker
{
    ReplaceMap* mMap;


public:
    using OPS::Reprise::Service::DeepWalker::visit;

    ReplaceWalker(ReplaceMap& Map):mMap(&Map)
    {

    }


    virtual void visit(ReferenceExpression& visited_node)
    {
        if( mMap->find(&visited_node.getReference()) != mMap->end())
        {
            VariableDeclaration* new_decl = (*mMap)[ &visited_node.getReference()];
            visited_node.setReference(new_decl);
        }
    }
};



//check if declaration is either 1-dimension array or a scalar
//that scalars has 1 for new dimension
//that no special declarators are involved
//that there is no init expressions

bool checkReplaceConsistency(ReplaceParams& to_replace, bool strict)
{
    for (ReplaceParams::iterator var = to_replace.begin();
         var!=to_replace.end();
         ++var)
    {
        const VariableDeclaration* vdec = var->first;
        size_t size = var->second;
        if(vdec->getType().is_a<ArrayType>())
        {
        }
        else if (strict)
        {
            OPS_ASSERT(size == 1);
        }
    }
    return true;
}


ReplaceMap prepareCadreDeclaraions(ReplaceParams& to_replace)
{
    OPS_ASSERT(checkReplaceConsistency(to_replace, false));
    ReplaceMap to_return;
    for (ReplaceParams::iterator var = to_replace.begin();
         var!=to_replace.end();
         ++var)
    {
        const VariableDeclaration* vdec = var->first;
        size_t size = var->second;
        std::string newName = vdec->getName();
        TypeBase* newType = vdec->getType().clone();
        if(vdec->getType().is_a<ArrayType>())
        {
            ArrayType* newArrayType = newType->cast_ptr<ArrayType>();
            newArrayType->setElementCount(size);

        }
        VariableDeclaration* newVar = new VariableDeclaration(newType, newName ) ;
        to_return[vdec]=newVar;
    }
    return to_return;

}

void insertDeclarations(Declarations& where, ReplaceMap& to_replace)
{
    for (ReplaceMap::reverse_iterator var = to_replace.rbegin();
         var!=to_replace.rend();
         ++var)
    {
        VariableDeclaration* vdec = var->second;
        where.addFirst(vdec);
    }
}

void makeReplace(BlockStatement& block, ReplaceMap& to_replace)
{
    ReplaceWalker walker(to_replace);
    walker.visit(block);
}

Declarations& getGlobals(BlockStatement& block)
{
    RepriseBase* root = block.getParent();
    while (root)
    {
        if(root->is_a<TranslationUnit>())
            break;
        root = root->getParent();
    }
    OPS_ASSERT(root);
    TranslationUnit* unit = root->cast_ptr<TranslationUnit>();
    return unit->getGlobals();

}

void replaceCadreVariables(BlockStatement& block, ReplaceMap& to_replace)
{
    Declarations& decl = getGlobals(block);
    insertDeclarations(decl, to_replace);
    makeReplace(block, to_replace);
}

}
}
}
