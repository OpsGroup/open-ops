#ifndef _CONTEXT_OBSERVER_STATE_H_INCLUDED_
#define _CONTEXT_OBSERVER_STATE_H_INCLUDED_

#include <OPS_Core/Mixins.h>

#include <Reprise/Common.h>

namespace OPS
{

namespace Shared
{

/// Base class for context observer state
class ContextObserverState
	: public Reprise::RepriseBase
{
public:
	virtual ~ContextObserverState();

public:
	bool isSyncronized() const;
	void setSyncronized(bool isSyncronized);

	// RepriseBase stuff

	virtual int getChildCount() const;
	virtual RepriseBase& getChild(int index);

	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ContextObserverState)

protected:
	ContextObserverState();

private:
	bool m_isSyncronized;
};

}

}

#endif // _CONTEXT_OBSERVER_STATE_H_INCLUDED_
