#include "Shared/ContextNotifier/ContextObserverBase.h"

#include "Shared/ContextNotifier/ContextNotifier.h"
#include "Shared/ContextNotifier/ContextObserverStateManager.h"

#include <OPS_Core/Helpers.h>

namespace OPS
{

namespace Shared
{

ContextObserverBase::ContextObserverBase(const std::string& observerId)
	: m_dependencies(new ContextNotifier())
	, m_stateManager(new ContextObserverStateManager(observerId))
{
}

ContextObserverBase::~ContextObserverBase()
{
}

ContextNotifier& ContextObserverBase::getDependencies() const
{
	OPS_ASSERT(m_dependencies.get() != NULL);

	return *m_dependencies;
}

ContextObserverStateManager& ContextObserverBase::getStateManager() const
{
	OPS_ASSERT(m_stateManager.get() != NULL);

	return *m_stateManager;
}

bool ContextObserverBase::isStateSyncronized(const Reprise::RepriseBase &context) const
{
	return getStateManager().hasState(context) &&
		getStateManager().getState<ContextObserverState>(context).isSyncronized();
}

void ContextObserverBase::notifyContextChanged(const Reprise::RepriseBase& context)
{
	if (isStateSyncronized(context))
	{
		getStateManager().getState<ContextObserverState>(context).setSyncronized(false);
	}

	getDependencies().notifyContextChanged(context);
}

void ContextObserverBase::syncronizeWithContextIfNeeded(
	const Reprise::RepriseBase& context)
{
	if (!isStateSyncronized(context))
	{
        syncronizeWithContext(context);

		getStateManager().getState<ContextObserverState>(context).setSyncronized(true);
	}
}

}

}
