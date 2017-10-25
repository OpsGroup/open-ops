#ifndef CONVERT_STRUCT_COUNTER__
#define CONVERT_STRUCT_COUNTER__

#include "Reprise/Reprise.h"


namespace OPS
{
namespace Transforms
{

bool canChangeStructLoopCounter(OPS::Reprise::ForStatement& forStmt);

OPS::Reprise::ForStatement& changeStructLoopCounter(OPS::Reprise::ForStatement& forStmt);

}
}
#endif
