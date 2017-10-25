#include "Shared/ContextNotifier/ContextObserverStateManager.h"

#include <Reprise/Common.h>
#include <Reprise/Utils.h>

namespace OPS
{

namespace Shared
{

ContextObserverStateManager::ContextObserverStateManager(const std::string& observerId)
	: m_observerId(observerId)
{
}

ContextObserverStateManager::~ContextObserverStateManager()
{
}

Reprise::ProgramUnit &ContextObserverStateManager::findProgramUnit(
	const Reprise::RepriseBase& context)
{
	Reprise::ProgramUnit* program = context.findProgramUnit();

	OPS_ASSERT(program != NULL);

	return *program;
}

bool ContextObserverStateManager::hasState(const Reprise::RepriseBase& context) const
{
	return findProgramUnit(context).hasNote(m_observerId);
}

void ContextObserverStateManager::resetState(const Reprise::RepriseBase &context,
	ContextObserverState* state)
{
	Reprise::ReprisePtr<Reprise::RepriseBase> statePtr(state);

	findProgramUnit(context).setNote(m_observerId, Reprise::Note::newReprise(statePtr));
}

}

}
