#ifndef OPS_SHARED_NODESCOLLECTOR_H_INCLUDED__
#define OPS_SHARED_NODESCOLLECTOR_H_INCLUDED__

#include <vector>
#include <set>
#include <list>
#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
namespace Shared
{

template<typename _NodeType,
		 typename _CollectionType = std::vector<_NodeType*> >
	class NodesCollector : public OPS::Reprise::Service::DeepWalker
{
public:
	typedef _NodeType NodeType;
	typedef _CollectionType CollectionType;

	NodesCollector() {}

	void visit(NodeType& node)
	{
		m_collection.push_back(&node);
		OPS::Reprise::Service::DeepWalker::visit(node);
	}

	CollectionType& getCollection() { return m_collection; }
	const CollectionType& getCollection() const { return m_collection; }

private:
	CollectionType m_collection;
};

template<typename NodeType>
	std::vector<NodeType*> collectNodes(OPS::Reprise::RepriseBase& rootNode)
{
	NodesCollector<NodeType> collector;
	rootNode.accept(collector);
	return collector.getCollection();
}

template<typename NodeType>
	std::list<NodeType*> collectNodesList(OPS::Reprise::RepriseBase &rootNode)
{
	NodesCollector<NodeType, std::list<NodeType*> > collector;
	rootNode.accept(collector);
	return collector.getCollection();
}

template<typename NodeType, typename Container>
	void collectNodes2(const OPS::Reprise::RepriseBase &rootNode, Container& container)
{
    if (const NodeType* node = rootNode.cast_ptr<const NodeType>())
		container.insert(container.end(), node);

	int childCount = rootNode.getChildCount();
	for(int i = 0; i < childCount; ++i)
	{
		collectNodes2<NodeType, Container>(const_cast<Reprise::RepriseBase&>(rootNode).getChild(i), container);
	}
}

template<typename NodeType, typename Container>
    void collectNodes2(OPS::Reprise::RepriseBase &rootNode, Container& container)
{
    if (NodeType* node = rootNode.cast_ptr<NodeType>())
        container.insert(container.end(), node);

    int childCount = rootNode.getChildCount();
    for(int i = 0; i < childCount; ++i)
    {
        collectNodes2<NodeType, Container>(const_cast<Reprise::RepriseBase&>(rootNode).getChild(i), container);
    }
}

}
}

#endif // OPS_SHARED_NODESCOLLECTOR_H_INCLUDED__
