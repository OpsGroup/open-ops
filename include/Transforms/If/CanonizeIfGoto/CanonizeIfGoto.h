#ifndef CANONIZE_IF_GOTO
#define CANONIZE_IF_GOTO

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{

bool canCanonizeIfGoto(OPS::Reprise::IfStatement& ifStatement);

void canonizeIfGoto(OPS::Reprise::IfStatement& ifStatement);

}
}
#endif
