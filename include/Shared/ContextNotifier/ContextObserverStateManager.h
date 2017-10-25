#ifndef _CONTEXT_OBSERVER_STATE_MANAGER_H_INCLUDED_
#define _CONTEXT_OBSERVER_STATE_MANAGER_H_INCLUDED_

#include "Shared/ContextNotifier/ContextObserverState.h"

#include <OPS_Core/Helpers.h>

#include <Reprise/Common.h>
#include <Reprise/Units.h>

#include <string>

namespace OPS
{

namespace Shared
{

/// Manager for context observer states
class ContextObserverStateManager
{
public:
	ContextObserverStateManager(const std::string& observerId);

	~ContextObserverStateManager();

public:
	/// Get state of specified constext
	template<class StateClass>
	StateClass& getState(const Reprise::RepriseBase& context) const
	{
		OPS_ASSERT(hasState(context));

		Reprise::Note& note = findProgramUnit(context).getNote(m_observerId);

		OPS_ASSERT(note.getReprise().is_a<StateClass>());

		return note.getReprise().cast_to<StateClass>();
	}

	/// Check if state exists for specified context
	bool hasState(const Reprise::RepriseBase& context) const;

	/// Reset State for specified context
	void resetState(const Reprise::RepriseBase& context, ContextObserverState* state);

private:
	static Reprise::ProgramUnit& findProgramUnit(const Reprise::RepriseBase& context);

private:
	const std::string m_observerId;
};

}

}

#endif // _CONTEXT_OBSERVER_STATE_MANAGER_H_INCLUDED_
