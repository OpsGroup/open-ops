#ifndef REMOVEDEADDECLARATIONS_H
#define REMOVEDEADDECLARATIONS_H

namespace OPS
{

namespace Reprise
{
class ProgramUnit;
class TranslationUnit;
}

namespace Transformations
{
namespace Declarations
{

void removeDeadDeclarations(Reprise::ProgramUnit& program);

}
}
}

#endif // REMOVEDEADDECLARATIONS_H
