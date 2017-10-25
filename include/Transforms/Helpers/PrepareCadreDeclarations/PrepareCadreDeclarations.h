#pragma once
#include "Reprise/Statements.h"

namespace OPS
{
namespace Transforms
{
namespace Helpers
{



typedef std::map<const Reprise::VariableDeclaration*, int> ReplaceParams;
typedef std::map<const Reprise::VariableDeclaration*, Reprise::VariableDeclaration*> ReplaceMap;


ReplaceMap prepareCadreDeclaraions(ReplaceParams& to_replace);
void replaceCadreVariables(Reprise::BlockStatement& block, ReplaceMap& to_replace);

}
}
}
