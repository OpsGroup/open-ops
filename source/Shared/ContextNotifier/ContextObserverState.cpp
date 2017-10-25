#include "Shared/ContextNotifier/ContextObserverState.h"

#include <Reprise/Exceptions.h>

namespace OPS
{

namespace Shared
{

ContextObserverState::ContextObserverState()
	: m_isSyncronized(false)
{
}

ContextObserverState::~ContextObserverState()
{
}

int ContextObserverState::getChildCount() const
{
	return 0;
}

Reprise::RepriseBase& ContextObserverState::getChild(int )
{
	throw Reprise::UnexpectedChildError("ContextObserverState::getChild()");
}

bool ContextObserverState::isSyncronized() const
{
	return m_isSyncronized;
}

void ContextObserverState::setSyncronized(bool isSyncronized)
{
	if (m_isSyncronized != isSyncronized)
	{
		m_isSyncronized = isSyncronized;
	}
}

}

}
