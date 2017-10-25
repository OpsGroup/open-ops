#ifndef _CONTEXT_OBSERVER_BASE_H_INCLUDED_
#define _CONTEXT_OBSERVER_BASE_H_INCLUDED_

#include "Shared/ContextNotifier/IContextObserver.h"

#include <memory>
#include <string>

namespace OPS
{

namespace Reprise
{

class RepriseBase;

}

namespace Shared
{

class ContextNotifier;
class ContextObserverStateManager;

/// Context observer base implementation
class ContextObserverBase
	: public virtual IContextObserver
{
public:
	virtual ~ContextObserverBase();

public:
	virtual void notifyContextChanged(const Reprise::RepriseBase& context);

protected:
	ContextObserverBase(const std::string& observerId);

protected:
	/// Dependent observers
	ContextNotifier& getDependencies() const;
	/// State manager
	ContextObserverStateManager& getStateManager() const;

    /// Call syncronizeWithContext if state for specified context is not syncronized
    void syncronizeWithContextIfNeeded(const Reprise::RepriseBase& context);
	/// Override for state syncronization
    virtual void syncronizeWithContext(const Reprise::RepriseBase& context) = 0;

private:
	bool isStateSyncronized(const Reprise::RepriseBase& context) const;

private:
	std::unique_ptr<ContextNotifier> m_dependencies;
	std::unique_ptr<ContextObserverStateManager> m_stateManager;
};

}

}

#endif
