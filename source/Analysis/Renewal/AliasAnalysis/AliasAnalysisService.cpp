#include "AliasAnalysisService.h"

#include "AliasAnalysisServiceState.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/OccurrenceContainer.h"

#include <OPS_Core/Helpers.h>

#include <Reprise/Common.h>
#include <Reprise/Units.h>

#include <Shared/ContextNotifier/ContextObserverStateManager.h>

namespace OPS
{

namespace Renewal
{

AliasAnalysisService::AliasAnalysisService()
	: Shared::ContextObserverBase("AliasAnalysisService")
{
}

AliasAnalysisService::~AliasAnalysisService()
{
}

Montego::OccurrenceContainer& AliasAnalysisService::reqestOccurrenceAnalyzer(
	const Reprise::RepriseBase& context)
{
    syncronizeWithContextIfNeeded(context);

	return getOccurrenceAnalyzer(context);
}

void AliasAnalysisService::syncronizeWithContext(const Reprise::RepriseBase& context)
{
	if (!getStateManager().hasState(context))
	{
		getStateManager().resetState(context, new AliasAnalysisState());
	}

	AliasAnalysisState& state = getStateManager().getState<AliasAnalysisState>(context);

	state.occurrenceAnalyzer.reset(new Montego::OccurrenceContainer());

	{
        Reprise::ProgramUnit* program = context.findProgramUnit();

        OPS_ASSERT(program != NULL);

        state.aliasAnalyzer.reset(Montego::AliasInterface::create(*program, *state.occurrenceAnalyzer));

		static const int s_aliasAnalysisSuccess = 0;

        OPS_VERIFY(state.aliasAnalyzer->runAliasAnalysis() == s_aliasAnalysisSuccess);
	}
}

Montego::AliasInterface &AliasAnalysisService::getAliasAnalyzer(
	const Reprise::RepriseBase& context) const
{
	OPS_ASSERT(getStateManager().hasState(context));

	AliasAnalysisState& state = getStateManager().getState<AliasAnalysisState>(context);

	OPS_ASSERT(state.aliasAnalyzer.get() != NULL);

	return *state.aliasAnalyzer;
}

Montego::OccurrenceContainer &AliasAnalysisService::getOccurrenceAnalyzer(
	const Reprise::RepriseBase& context) const
{
	OPS_ASSERT(getStateManager().hasState(context));

	AliasAnalysisState& state = getStateManager().getState<AliasAnalysisState>(context);

	OPS_ASSERT(state.occurrenceAnalyzer.get() != NULL);

	return *state.occurrenceAnalyzer;
}

}

}
