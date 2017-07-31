//  Standard includes

//  OPS includes
#include "Reprise/Lifetime.h"
#include "Reprise/Common.h"
#include "Reprise/Exceptions.h"
#include "Reprise/Units.h"
#include "Reprise/Statements.h"

//  Namespaces using

//  Defines and macros

//	Enter namespace
namespace OPS
{
namespace Reprise
{

//  Constants and enums

//  Classes

//  Functions declaration

//  Variables
static Coordinator* s_coordinator = 0;

//  Classes implementation

//  Global classes implementation
//      Coordinator - class implementation
Coordinator::Coordinator(void)
{
}

Coordinator::~Coordinator(void)
{
}

void Coordinator::init(void)
{
	if (s_coordinator != 0)
		throw RepriseError("Unexpected Coordinator init.");
	s_coordinator = new Coordinator();
}

Coordinator& Coordinator::instance(void)
{
	if (s_coordinator == 0)
	{
		s_coordinator = new Coordinator();
	}
	return *s_coordinator;
}

void Coordinator::shutdown(void)
{
	if (s_coordinator == 0)
		throw RepriseError("Unexpected Coordinator shutdown.");
	s_coordinator->cleanup();
	delete s_coordinator;
}

void Coordinator::addNode(RepriseBase* const node)
{
	OPS_ASSERT(node != 0);
	m_nodes.insert(node);
}

void Coordinator::removeNode(RepriseBase* const node)
{
	OPS_ASSERT(node != 0);
	NodesList::iterator nodeToRemove = m_nodes.find(node);
	if (nodeToRemove != m_nodes.end())
		m_nodes.erase(node);
}

std::string Coordinator::getStatistics(const unsigned indent) const
{
	std::string state;
#if OPS_REPRISE_COLLECT_CREATION_STATISTIC
	size_t memSize = 0;
	for (NodesList::const_iterator node = m_nodes.begin(); node != m_nodes.end(); ++node)
	{
		memSize += (*node)->getObjectSize();
	}
	for (unsigned i = 0; i < indent; ++i)
		state += " ";
	state += Strings::format("Nodes: %u. Memory: %u.\n", m_nodes.size(), memSize);
#else
	OPS_UNUSED(indent)
	state += "Creation statistics is disabled.";
#endif

	return state;
}


//		Coordinator - private methods
void Coordinator::cleanup(void)
{
}

//	Exit namespace
}
}
