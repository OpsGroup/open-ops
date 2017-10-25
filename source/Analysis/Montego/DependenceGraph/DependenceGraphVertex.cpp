#include "Analysis/Montego/DependenceGraph/DependenceGraphVertex.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

DependenceGraphVertex::DependenceGraphVertex()
	: m_parentStatement(0)
{
}

DependenceGraphVertex::DependenceGraphVertex(StatementBase* parentStatement)
	: m_parentStatement(parentStatement)
{
}

DependenceGraphVertex::DependenceGraphVertex(const OccurrencePtr& sourceOccurrence)
	: m_sourceOccurrence(sourceOccurrence)
{
	OPS_ASSERT(m_sourceOccurrence.get() != 0);

	m_parentStatement = m_sourceOccurrence->getParentStatement();
}

bool DependenceGraphVertex::operator ==(const DependenceGraphVertex& otherVertex) const
{
	if (m_sourceOccurrence.get() == 0 && otherVertex.m_sourceOccurrence.get() != 0 ||
		m_sourceOccurrence.get() != 0 && otherVertex.m_sourceOccurrence.get() == 0)
	{
		return false;
	}

	if (m_sourceOccurrence.get() == 0 && otherVertex.m_sourceOccurrence.get() == 0)
	{
		return m_parentStatement == otherVertex.m_parentStatement;
	}

	OPS_ASSERT(m_sourceOccurrence.get() != 0);
	OPS_ASSERT(otherVertex.m_sourceOccurrence.get() != 0);

	return m_sourceOccurrence->isEqual(*otherVertex.m_sourceOccurrence) &&
		m_parentStatement == otherVertex.m_parentStatement;
}

StatementBase* DependenceGraphVertex::getParentStatement() const
{
	return m_parentStatement;
}

OccurrencePtr DependenceGraphVertex::getSourceOccurrence() const
{
	return m_sourceOccurrence;
}

}
}
