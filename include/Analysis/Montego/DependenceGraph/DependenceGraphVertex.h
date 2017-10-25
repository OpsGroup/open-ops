#ifndef _DEPENDENCE_GRAPH_VERTEX_H_INCLUDED_
#define _DEPENDENCE_GRAPH_VERTEX_H_INCLUDED_

#include "Analysis/Montego/Occurrence.h"

#include <Reprise/Reprise.h>
#include <OPS_Core/MemoryHelper.h>

namespace OPS
{
namespace Montego
{

class DependenceGraphVertex
{
public:
	DependenceGraphVertex();
	explicit DependenceGraphVertex(Reprise::StatementBase* parentStatement);
	explicit DependenceGraphVertex(const OccurrencePtr& sourceOccurrence);

public:
	bool operator ==(const DependenceGraphVertex& otherVertex) const;

public:
	Reprise::StatementBase* getParentStatement() const;
	OccurrencePtr getSourceOccurrence() const;

private:
	Reprise::StatementBase* m_parentStatement;
	OccurrencePtr m_sourceOccurrence;
};

}
}

#endif // _DEPENDENCE_GRAPH_VERTEX_H_INCLUDED_
