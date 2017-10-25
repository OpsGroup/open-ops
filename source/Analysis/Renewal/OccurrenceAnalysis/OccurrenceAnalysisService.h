#ifndef _OCCURRENCE_ANALYSIS_SERVICE_H_INCLUDED_
#define _OCCURRENCE_ANALYSIS_SERVICE_H_INCLUDED_

#include "Analysis/Renewal/OccurrenceAnalysis/IOccurrenceAnalysisService.h"

#include <Shared/ContextNotifier/ContextObserverBase.h>

#include <memory>

namespace OPS
{

namespace Montego
{

class OccurrenceContainer;

}

namespace Renewal
{

class OccurrenceAnalysisService
	: public Shared::ContextObserverBase
	, public IOccurrenceAnalysisService
{
public:
	OccurrenceAnalysisService();

	~OccurrenceAnalysisService();

public:
	virtual Occurrences findAllOccurrences(const Reprise::RepriseBase& rootNode);

protected:
    virtual void syncronizeWithContext(const Reprise::RepriseBase& context);

private:
	Montego::OccurrenceContainer& getOccurrenceAnalyzer(
		const Reprise::RepriseBase& context) const;
};

}

}

#endif
