#ifndef OPS_REPRISE_LIFETIME_H_INCLUDED__
#define OPS_REPRISE_LIFETIME_H_INCLUDED__

#include "OPS_Core/Mixins.h"
#include <set>
#include <list>
#include <string>

//	Enter namespaces
namespace OPS
{
namespace Reprise
{

class RepriseBase;

class Coordinator : OPS::NonCopyableMix
{
public:
	static void init(void);
	static Coordinator& instance(void);
	static void shutdown(void);

	void addNode(RepriseBase* node);
	void removeNode(RepriseBase* node);

	std::string getStatistics(unsigned indent = 0) const;

private:
	typedef std::set<RepriseBase*> NodesList;

	Coordinator(void);
	~Coordinator(void);

	void cleanup(void);

	NodesList m_nodes;
};

//	Exit namespaces
}
}


#endif                      // OPS_REPRISE_LIFETIME_H_INCLUDED__
