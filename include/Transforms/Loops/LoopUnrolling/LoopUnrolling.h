#ifndef _LOOP_UNROLLING_H_INCLUDED_
#define _LOOP_UNROLLING_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::ForStatement;


bool canApplyLoopUnrollingTo(ForStatement& forStatement, qword);

void makeLoopUnrolling(ForStatement& forStatement, qword h);

}	// Loops
}	// Transforms
}	// OPS

#endif	// _LOOP_UNROLLING_H_INCLUDED_
