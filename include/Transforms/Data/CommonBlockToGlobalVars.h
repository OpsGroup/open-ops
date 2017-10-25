#ifndef COMMONBLOCKTOGLOBALVARS_H
#define COMMONBLOCKTOGLOBALVARS_H

#include "Reprise/Units.h"
#include "Reprise/Declarations.h"

namespace OPS
{
namespace Transforms
{

/// Convert single common block
bool convertCommonBlockToGlobalVariables(Reprise::VariableDeclaration& commonBlockVariable);

/// Convert all common blocks declared in fragment
void convertAllCommonBlocksToGlobalVariables(Reprise::RepriseBase& node);

}
}

#endif // COMMONBLOCKTOGLOBALVARS_H
