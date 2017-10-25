#include "Analysis/Frames/FramesGraph.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			FramesGraph::FramesGraph()
				:m_accesssibilityMap()
			{
			}

			void FramesGraph::addNode( Frame* pFrame )
			{
				OPS_ASSERT(pFrame != NULL);

				m_accesssibilityMap[pFrame];
			}

			void FramesGraph::addEdge( Frame* pBegin, Frame* pEnd )
			{
				OPS_ASSERT(pBegin != NULL);
				OPS_ASSERT(pEnd != NULL);

				m_accesssibilityMap[pBegin];
				m_accesssibilityMap[pEnd];

				m_accesssibilityMap[pBegin].insert(pEnd);
			}

			FramesGraph::FramesCollection FramesGraph::getNodes()
			{
				FramesGraph::FramesCollection result;

				for (AccesssibilityMap::iterator it = m_accesssibilityMap.begin(); it != m_accesssibilityMap.end(); ++it)
				{
					result.insert(it->first);
				}

				return result;
			}

			FramesGraph::AccesssibilityMap FramesGraph::getAccesssibilityMap()
			{
				return m_accesssibilityMap;
			}

			void FramesGraph::setFirstExecutableNode(Frame* pFirstExecutableNode)
			{
				OPS_ASSERT(m_accesssibilityMap.find(pFirstExecutableNode) != m_accesssibilityMap.end());
				m_pFirstExecutableNode = pFirstExecutableNode;
			}

			void FramesGraph::setLastExecutableNodes(FramesGraph::FramesCollection& lastExecutableNodes)
			{
				m_lastExecutableNodes.clear();
				for (FramesCollection::iterator it = lastExecutableNodes.begin(); it != lastExecutableNodes.end(); ++it)
				{
					OPS_ASSERT(m_accesssibilityMap.find(*it) != m_accesssibilityMap.end());
					m_lastExecutableNodes.insert(*it);
				}
			}

			Frame* FramesGraph::getFirstExecutableNode()
			{
				return m_pFirstExecutableNode;
			}

			FramesGraph::FramesCollection FramesGraph::getLastExecutableNodes()
			{
				return m_lastExecutableNodes;
			}
		}
	}
}
