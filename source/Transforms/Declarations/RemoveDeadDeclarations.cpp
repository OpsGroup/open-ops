#include "Transforms/Declarations/RemoveDeadDeclarations.h"
#include "Reprise/Units.h"
#include "Analysis/DeadDeclarations.h"
#include "Reprise/Declarations.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Transformations
{
namespace Declarations
{

void removeDeadDeclarations(Reprise::ProgramUnit& program)
{
    // Find main subroutine
    Reprise::SubroutineDeclaration* subMain = 0;
    int unitCount = program.getUnitCount();
    for(int i = 0; i < unitCount && subMain == 0; ++i)
    {
         subMain = program.getUnit(i).getGlobals().findSubroutine("main");
    }

    std::list<DeclarationBase*> rootDeclarations;

    if (subMain != 0)
    {
        rootDeclarations.push_back(subMain);
    }
    else
    {
        for(int i = 0; i < unitCount; ++i)
        {
            Reprise::Declarations& decls = program.getUnit(i).getGlobals();
            for(Reprise::Declarations::Iterator it = decls.getFirst(); it != decls.getLast(); ++it)
            {
                if (it->getKind() == DeclarationBase::DK_SUBROUTINE ||
                    it->getKind() == DeclarationBase::DK_VARIABLE)
                {
                    rootDeclarations.push_back(&*it);
                }
            }
        }
    }

    std::set<DeclarationBase*> usedDeclarations;
    OPS::Analysis::obtainUsedDeclarations(rootDeclarations, usedDeclarations);

    for(int i = 0; i < unitCount; ++i)
    {
        Reprise::Declarations& decls = program.getUnit(i).getGlobals();
        for(Reprise::Declarations::Iterator it = decls.getFirst(); it.isValid();)
        {
            if (usedDeclarations.find(&*it) == usedDeclarations.end())
            {
                Reprise::Declarations::Iterator tmp = it++;
                decls.erase(tmp);
            }
            else
            {
                it++;
            }
        }
    }
}

}
}
}
