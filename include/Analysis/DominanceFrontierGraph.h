#ifndef OPS_REPRISE_DOMINANCEFRONTIERGRAPH_H__
#define OPS_REPRISE_DOMINANCEFRONTIERGRAPH_H__

#include "Analysis/ControlFlowGraphEx.h"


namespace OPS
{
namespace Analysis
{
namespace ControlFlowGraphs
{
namespace DominanceFrontierGraph
{
	class DominanceFrontier :  public OPS::NonAssignableMix, public OPS::NonCopyableMix, public Reprise::IntrusivePointerBase
	{
	protected:
		SSAControlFlowGraph::StatementGraph doms; //доминаторы подблока
		SSAControlFlowGraph::StatementGraph domF; //границы доминации подблока
		SSAControlFlowGraph::StatementGraph domFInverse; //инвертированный граф границ доминации
		SSAControlFlowGraph::CFGraph incDom;
		SSAControlFlowGraph::StatementGraph domTree;
		SSAControlFlowGraph::StatementTree domTreeInverse;
		ControlFlowGraphEx* m_cfgex;
		bool m_ownCFGex;

	public:
		SSAControlFlowGraph::StatementGraph& getDoms() {return doms;};
		SSAControlFlowGraph::StatementGraph& getDomTree() {return domTree; };
		SSAControlFlowGraph::StatementTree& getDomTreeInverse() {return domTreeInverse; };
		SSAControlFlowGraph::StatementGraph& getDomF() {return domF;};
		ControlFlowGraphEx& getCFGex() {return *m_cfgex;};

		explicit DominanceFrontier(OPS::Reprise::BlockStatement &aBlock);
		explicit DominanceFrontier(ControlFlowGraphEx &aCFGex);

		~DominanceFrontier();
	private:
		void buildIncDom(); //построить м-цу инцидентности блоков
		void buildReverseDom();//построить граф предков
		void buildDoms();
		void buildDomf();
		void buildReverseDomF();

	};
}
}
}
}

#endif
