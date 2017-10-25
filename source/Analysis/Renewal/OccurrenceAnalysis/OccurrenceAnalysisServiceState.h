#ifndef _OCCURRENCE_ANALYSIS_SERVICE_STATE_H_INCLUDED_
#define _OCCURRENCE_ANALYSIS_SERVICE_STATE_H_INCLUDED_

#include <Shared/ContextNotifier/ContextObserverState.h>

namespace OPS
{

namespace Montego
{

class OccurrenceContainer;

}

namespace Renewal
{

struct OccurrenceAnalysisServiceState
	: public Shared::ContextObserverState
{
	OccurrenceAnalysisServiceState()
		: occurrenceAnalyzer(NULL)
	{
	}

	Montego::OccurrenceContainer* occurrenceAnalyzer;
};

}

}

#endif // _OCCURRENCE_ANALYSIS_SERVICE_STATE_H_INCLUDED_
