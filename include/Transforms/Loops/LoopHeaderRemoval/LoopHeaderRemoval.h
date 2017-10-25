#ifndef _LOOP_HEADER_REMOVAL_
#define _LOOP_HEADER_REMOVAL_

#include "Reprise/Reprise.h"

namespace OPS 
{
namespace Transforms
{
namespace Loops
{
	using OPS::Reprise::ForStatement;
	
	bool canApplyLoopHeaderRemovalTo(ForStatement& forStatement);
	void makeLoopHeaderRemoval(ForStatement& forStatement);

} //Loops
} //Transforms
} //OPS
#endif  //_LOOP_HEADER_REMOVAL_
