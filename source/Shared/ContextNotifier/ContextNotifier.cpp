#include "Shared/ContextNotifier/ContextNotifier.h"

#include "Shared/ContextNotifier/IContextObserver.h"

#include "OPS_Core/Helpers.h"

#if defined (_MSC_VER) && _MSC_VER > 1600
	#include "Reprise/Common.h"    // this include breaks MSVS 2008 build
#endif

#include <algorithm>
#include <functional>

#ifdef __GNUC__
	#include <tr1/functional>
#endif

namespace OPS
{

namespace Shared
{

ContextNotifier::ContextNotifier()
{
}

ContextNotifier::~ContextNotifier()
{
}

const std::list<IContextObserver*>& ContextNotifier::observers() const
{
	return m_observers;
}

namespace
{

bool contains(const std::list<IContextObserver*>& observers,
	const IContextObserver& observer)
{
	return std::find(observers.begin(), observers.end(), &observer) != observers.end();
}

}

void ContextNotifier::subscribe(IContextObserver& observer)
{
	OPS_ASSERT(!contains(observers(), observer));

	m_observers.push_back(&observer);
}

void ContextNotifier::unsubscribe(IContextObserver& observer)
{
	OPS_ASSERT(contains(observers(), observer));

	m_observers.remove(&observer);
}

void ContextNotifier::notifyContextChanged(const Reprise::RepriseBase& context) const
{
	using namespace std::tr1::placeholders;

	using std::tr1::bind;
	using std::tr1::ref;

	std::for_each(observers().begin(), observers().end(),
		bind(&IContextObserver::notifyContextChanged, _1, ref(context)));
}

}

}
