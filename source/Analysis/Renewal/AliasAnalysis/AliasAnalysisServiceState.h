#ifndef _ALIAS_ANALYSIS_SERVICE_STATE_H_INCLUDED_
#define _ALIAS_ANALYSIS_SERVICE_STATE_H_INCLUDED_

#include <Shared/ContextNotifier/ContextObserverState.h>

#include <memory>

namespace OPS
{

namespace Montego
{

class AliasInterface;
class OccurrenceContainer;

}

namespace Renewal
{

struct AliasAnalysisState
	: public Shared::ContextObserverState
{
	std::unique_ptr<Montego::AliasInterface> aliasAnalyzer;
	std::unique_ptr<Montego::OccurrenceContainer> occurrenceAnalyzer;
};

}

}

#endif // _ALIAS_ANALYSIS_SERVICE_STATE_H_INCLUDED_
