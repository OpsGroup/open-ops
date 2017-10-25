#ifndef ANALYSIS_DEADDECLARATIONS_H
#define ANALYSIS_DEADDECLARATIONS_H

#include <list>
#include "Reprise/Declarations.h"

namespace OPS
{
namespace Analysis
{

void obtainUsedDeclarations(const std::list<OPS::Reprise::DeclarationBase*>& rootDeclarations,
							std::set<OPS::Reprise::DeclarationBase*>& usedDeclarations);

void obtainUsedDeclarations(OPS::Reprise::DeclarationBase* rootDeclaration,
                            std::set<OPS::Reprise::DeclarationBase*>& usedDeclarations);

}
}

#endif // ANALYSIS_DEADDECLARATIONS_H
