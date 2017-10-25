#ifndef _I_CONTEXT_OBSERVER_H_INCLUDED_
#define _I_CONTEXT_OBSERVER_H_INCLUDED_

namespace OPS
{

namespace Reprise
{

class RepriseBase;

}

namespace Shared
{

/// Context observer interface
class IContextObserver
{
public:
	/// Notify context observer about context change
	virtual void notifyContextChanged(const Reprise::RepriseBase& context) = 0;

protected:
	inline ~IContextObserver() {}
};

}

}

#endif
