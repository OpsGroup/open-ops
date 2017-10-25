#include "OccurrenceAnalysisService.h"

#include "../AliasAnalysis/IOccurrenceAnalyzerHolder.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Renewal/AliasAnalysis/IAliasAnalysisService.h"
#include "Analysis/Renewal/Occurrence/OccurrenceHelpers.h"
#include "OccurrenceAnalysisServiceState.h"

#include <OPS_Core/Helpers.h>

#include <Shared/ContextNotifier/ContextNotifier.h>
#include <Shared/ContextNotifier/ContextObserverStateManager.h>

namespace OPS
{

namespace Renewal
{

OccurrenceAnalysisService::OccurrenceAnalysisService()
	: Shared::ContextObserverBase("OccurrenceAnalysisService")
{
	getDependencies().subscribe(getAliasAnalysisService());
}

OccurrenceAnalysisService::~OccurrenceAnalysisService()
{
}

void OccurrenceAnalysisService::syncronizeWithContext(const Reprise::RepriseBase& context)
{
	if (!getStateManager().hasState(context))
	{
		getStateManager().resetState(context, new OccurrenceAnalysisServiceState());
	}

	OccurrenceAnalysisServiceState& state =
		getStateManager().getState<OccurrenceAnalysisServiceState>(context);

	{
		Internals::IOccurrenceAnalyzerHolder* const occurrenceAnalyzerHolder =
			dynamic_cast<Internals::IOccurrenceAnalyzerHolder*>(
				&getAliasAnalysisService());

		OPS_ASSERT(occurrenceAnalyzerHolder != NULL);

		state.occurrenceAnalyzer =
			&occurrenceAnalyzerHolder->reqestOccurrenceAnalyzer(context);
	}
}

Montego::OccurrenceContainer &OccurrenceAnalysisService::getOccurrenceAnalyzer(
	const Reprise::RepriseBase& context) const
{
	OPS_ASSERT(getStateManager().hasState(context));

	OccurrenceAnalysisServiceState& state =
		getStateManager().getState<OccurrenceAnalysisServiceState>(context);

	OPS_ASSERT(state.occurrenceAnalyzer != NULL);

	return *state.occurrenceAnalyzer;
}

Occurrences OccurrenceAnalysisService::findAllOccurrences(const RepriseBase& rootNode)
{
    syncronizeWithContextIfNeeded(rootNode);

	typedef std::vector<Montego::OccurrencePtr> MontegoOccurrences;

	const MontegoOccurrences montegoOccurrences =
		getOccurrenceAnalyzer(rootNode).getAllOccurrencesIn(
			&const_cast<Reprise::RepriseBase&>(rootNode));

	Occurrences renewalOccurrences(montegoOccurrences.size());

	{
		MontegoOccurrences::const_iterator montegoIt = montegoOccurrences.begin();
		Occurrences::iterator renewalIt = renewalOccurrences.begin();

		while (montegoIt != montegoOccurrences.end())
		{
			*renewalIt++ = convertMontegoToRenewal(**montegoIt++);
		}
	}

	return renewalOccurrences;
}

}

}
