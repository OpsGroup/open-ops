#include "Analysis/Renewal/Occurrence/OccurrenceHelpers.h"

#include "Analysis/Montego/Occurrence.h"

#include <OPS_Core/Helpers.h>

#include <Reprise/Expressions.h>

namespace OPS
{

namespace Renewal
{

OccurrencePtr convertMontegoToRenewal(const Montego::Occurrence& montegoOccurrence)
{
	const AbstractOccurrence::Type occurrenceType = montegoOccurrence.isGenerator() ?
		AbstractOccurrence::T_GENERATOR : AbstractOccurrence::T_USSAGE;

	OPS_ASSERT(montegoOccurrence.getSourceExpression() != NULL);

	if (montegoOccurrence.is_a<Montego::BasicOccurrence>())
	{
		return OccurrencePtr(new DataAccessOccurrence(occurrenceType,
			*montegoOccurrence.getSourceExpression(),
			*montegoOccurrence.getSourceExpression()));
	}

	if (montegoOccurrence.is_a<Montego::ComplexOccurrence>())
	{
		OPS_ASSERT(montegoOccurrence.getSourceExpression()->is_a<
			Reprise::SubroutineCallExpression>());

		return OccurrencePtr(new FunctionCallOccurrence(occurrenceType,
			montegoOccurrence.getSourceExpression()->cast_to<
				Reprise::SubroutineCallExpression>()));
	}

	OPS_ASSERT(!"Can not convert Montego occurrence to Renewal Occurrence");

	return OccurrencePtr();
}

}

}
