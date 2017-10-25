#ifndef _CONTEXT_NOTIFIER_H_INCLUDED_
#define _CONTEXT_NOTIFIER_H_INCLUDED_

#include <OPS_Core/Mixins.h>

#include <list>

namespace OPS
{

namespace Reprise
{

class RepriseBase;

}

namespace Shared
{

class IContextObserver;

/// Context notifier
class ContextNotifier
	: public NonCopyableMix
{
public:
	ContextNotifier();

	~ContextNotifier();

public:
	/// Subscribe context observer for notifications
	void subscribe(IContextObserver& observer);
	/// Unsubscribe context observer
	void unsubscribe(IContextObserver& observer);

	/// Notify all subscribed context observers about context change
	void notifyContextChanged(const Reprise::RepriseBase& context) const;

private:
	const std::list<IContextObserver*>& observers() const;

private:
	std::list<IContextObserver*> m_observers;
};

}

}

#endif
