#ifndef _I_OCCURRENCE_ANALYSIS_SERVICE_H_INCLUDED_
#define _I_OCCURRENCE_ANALYSIS_SERVICE_H_INCLUDED_

#include "Analysis/Renewal/Occurrence/Occurrences.h"

#include <OPS_Core/ServiceLocator.h>

#include <Shared/ContextNotifier/IContextObserver.h>

#include <vector>

namespace OPS
{

namespace Reprise
{

class RepriseBase;

}

namespace Renewal
{

typedef std::vector<OccurrencePtr> Occurrences;

/// Stuff for work with occurrences
class IOccurrenceAnalysisService
	: public virtual Shared::IContextObserver
{
public:
	virtual Occurrences findAllOccurrences(const Reprise::RepriseBase& rootNode) = 0;

protected:
	inline ~IOccurrenceAnalysisService() {}
};

inline IOccurrenceAnalysisService& getOccurrenceAnalysisService()
{
	return Core::ServiceLocator::instance().getService<IOccurrenceAnalysisService>();
}

}

}

#endif
