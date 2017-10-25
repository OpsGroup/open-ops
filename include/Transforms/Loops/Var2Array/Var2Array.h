#ifndef _VAR2ARRAY
#define _VAR2ARRAY

#include "Reprise/Expressions.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{
	bool MakeVar2ArrayTransform(OPS::Montego::DependenceGraph::ArcList::iterator Arrow, OPS::Reprise::ForStatement* pFor);
}	// Loops
}	// Transforms
}	// OPS

#endif // _VAR2ARRAY
