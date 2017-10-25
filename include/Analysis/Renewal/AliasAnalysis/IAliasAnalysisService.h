#ifndef _I_ALIAS_ANALYSIS_SERVICE_H_INLUDED_
#define _I_ALIAS_ANALYSIS_SERVICE_H_INLUDED_

#include <OPS_Core/ServiceLocator.h>

#include <Shared/ContextNotifier/IContextObserver.h>

namespace OPS
{

namespace Renewal
{

/// Stuff for work with aliases
class IAliasAnalysisService
	: public virtual Shared::IContextObserver
{
protected:
	inline ~IAliasAnalysisService() {}
};

inline IAliasAnalysisService& getAliasAnalysisService()
{
	return Core::ServiceLocator::instance().getService<IAliasAnalysisService>();
}

}

}

#endif
