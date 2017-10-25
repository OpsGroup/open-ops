#ifndef FRAMES_GRAPH_H_INCLUDED
#define FRAMES_GRAPH_H_INCLUDED

#include <map>
#include <set>

#include "Analysis/Frames/Frame.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			using std::map;
			using std::set;

			class FramesGraph
			{
			public:
				typedef set<Frame*> FramesCollection;
				typedef map<Frame*, FramesCollection > AccesssibilityMap;

			public:
				FramesGraph();

				void addNode(Frame* pFrame);
				void addEdge(Frame* pBegin, Frame* pEnd);

				void setFirstExecutableNode(Frame* pFirstExecutableNode);
				void setLastExecutableNodes(FramesCollection& lastExecutableNodes);

				Frame* getFirstExecutableNode();
				FramesCollection getLastExecutableNodes();

				FramesCollection getNodes();
				AccesssibilityMap getAccesssibilityMap();

			private:
				AccesssibilityMap m_accesssibilityMap;

				Frame* m_pFirstExecutableNode;
				FramesCollection m_lastExecutableNodes;
			};
		}
	}
}

#endif // FRAMES_GRAPH_H_INCLUDED
