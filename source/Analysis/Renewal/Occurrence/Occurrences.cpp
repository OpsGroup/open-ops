#include "Analysis/Renewal/Occurrence/Occurrences.h"

#include <OPS_Core/Helpers.h>

namespace OPS
{

namespace Renewal
{

AbstractOccurrence::AbstractOccurrence(AbstractOccurrence::Type type)
	: m_type(type)
{
}

AbstractOccurrence::~AbstractOccurrence()
{
}

AbstractOccurrence::Type AbstractOccurrence::getType() const
{
	return m_type;
}

bool AbstractOccurrence::equals(const AbstractOccurrence& other) const
{
	return &getOccurrenceNode() == &other.getOccurrenceNode() &&
	&getMemoryAccessNode() == &other.getMemoryAccessNode();
}

//////////////////////////////////////////////////////////////////////////////////////////

DataAccessOccurrence::DataAccessOccurrence(Type type,
	const Reprise::ExpressionBase& occurrenceNode,
	const Reprise::ExpressionBase& memoryAccessNode)
	: AbstractOccurrence(type)
	, m_occurrenceNode(&const_cast<Reprise::ExpressionBase&>(occurrenceNode))
	, m_memoryAccessNode(&const_cast<Reprise::ExpressionBase&>(memoryAccessNode))
{
}

DataAccessOccurrence::~DataAccessOccurrence()
{
}

const Reprise::ExpressionBase& DataAccessOccurrence::getOccurrenceNode() const
{
	OPS_ASSERT(!m_occurrenceNode.expired());

	return *m_occurrenceNode;
}

const Reprise::ExpressionBase& DataAccessOccurrence::getMemoryAccessNode() const
{
	OPS_ASSERT(!m_memoryAccessNode.expired());

	return *m_memoryAccessNode;
}

//////////////////////////////////////////////////////////////////////////////////////////

FunctionCallOccurrence::FunctionCallOccurrence(Type type,
	const Reprise::SubroutineCallExpression& subroutineCallNode)
	: AbstractOccurrence(type)
	, m_subroutineCallNode(&const_cast<Reprise::SubroutineCallExpression&>(
		subroutineCallNode))
{
}

FunctionCallOccurrence::~FunctionCallOccurrence()
{
}

const Reprise::SubroutineCallExpression& FunctionCallOccurrence::getOccurrenceNode() const
{
	return getMemoryAccessNode();
}

const Reprise::SubroutineCallExpression&
	FunctionCallOccurrence::getMemoryAccessNode() const
{
	OPS_ASSERT(!m_subroutineCallNode.expired());

	return *m_subroutineCallNode;
}

}

}
