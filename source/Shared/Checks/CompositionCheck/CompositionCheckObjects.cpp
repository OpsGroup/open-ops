#include "Shared/Checks/CompositionCheck/CompositionCheckObjects.h"
#include "OPS_Core/Helpers.h"

namespace OPS
{
namespace Shared
{
namespace Checks
{

CompositionCheckObjects::CompositionCheckObjects()
{
}

CompositionCheckObjects& CompositionCheckObjects::operator << (CompositionCheckObjectTypes object)
{
	OPS_ASSERT(object < CCOT_ObjectsCount);
	m_objects.insert(object);

	return *this;
}

bool CompositionCheckObjects::contains(CompositionCheckObjectTypes object) const
{
	OPS_ASSERT(object < CCOT_ObjectsCount);

	return m_objects.find(object) != m_objects.end();
}

}	// OPS
}	// Shared
}	// Checks
