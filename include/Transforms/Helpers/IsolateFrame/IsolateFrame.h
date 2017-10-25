#include "Transforms/Helpers/PrepareCadreDeclarations/PrepareCadreDeclarations.h"

namespace OPS
{
namespace Transforms
{
namespace Helpers
{
using namespace Reprise;

typedef RepriseList<StatementBase> StatementList;
typedef RepriseList<VariableDeclaration> VariableList;

ReprisePtr<ProgramUnit> isolateFrame(StatementBase& frame,
                                     ReplaceParams& to_replace,
                                     VariableList& localVariables);
ReprisePtr<ProgramUnit> isolateFrame(StatementList& frame,
                                     ReplaceParams& to_replace,
                                     VariableList& localVariables);

}
}
}
