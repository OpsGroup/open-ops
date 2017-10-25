#ifndef _TEMPARRAYS
#define _TEMPARRAYS

#include "Reprise/Expressions.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{
	//bool MakeTempArrayTrunsform(DepGraph::LampArrow* Arrow, DepGraph::IndOccurContainer& occurList, OPS::Reprise::ForStatement* pFor);
	bool MakeTempArrayTransform(OPS::Montego::DependenceGraph::ArcList::iterator Arrow, OPS::Reprise::ForStatement* pFor);
}	// Loops
}	// Transforms
}	// OPS

#endif // _TEMPARRAYS
