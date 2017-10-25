#ifndef _LOOP_FULL_UNROLLING_H_INCLUDED_
#define _LOOP_FULL_UNROLLING_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::ForStatement;


bool canApplyLoopFullUnrollingTo(ForStatement& forStatement);

void makeLoopFullUnrolling(ForStatement& forStatement);

}	// Loops
}	// Transforms
}	// OPS

#endif	// _LOOP_FULL_UNROLLING_H_INCLUDED_
