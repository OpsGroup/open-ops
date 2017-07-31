#ifndef OPS_REPRISE_TESTMOD_NODECOLLECTOR_INCLUDED_H__
#define OPS_REPRISE_TESTMOD_NODECOLLECTOR_INCLUDED_H__

#include "OPS_Core/Mixins.h"
#include "Reprise/Reprise.h"

template <typename NodeType>
class Collector : OPS::NonCopyableMix
{
public:
	typedef NodeType* TNodeTypePtr;
	typedef std::vector<TNodeTypePtr> TNodes;

	inline explicit Collector(ProgramUnit& program) : m_program(program)
	{
	}

	TNodes m_node;

private:
	void collect(RepriseBase& reprise);

	ProgramUnit* m_program;
};


#endif	// OPS_REPRISE_TESTMOD_NODECOLLECTOR_INCLUDED_H__
