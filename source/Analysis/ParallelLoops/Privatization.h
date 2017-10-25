#pragma once

#include "Analysis/AbstractDepGraph.h"
#include "Analysis/ControlFlowGraph.h"

namespace OPS
{
namespace Analysis
{
namespace ParallelLoops
{

class PrivatizationHelper
{
public:
    typedef std::set<const OPS::Reprise::VariableDeclaration*> VariableSet;

	PrivatizationHelper(OPS::Reprise::StatementBase& stmt, AbstractDepGraph& depGraph);
    VariableSet analyse();
    void removeDependenciesByVariables(AbstractDepGraph& graph, const VariableSet& variables);

private:
    typedef std::set<OPS::Reprise::ReferenceExpression*> ReferenceSet;

	struct VariableReferences
	{
		std::set<AbstractDependencePtr> dependencies;
		ReferenceSet definitions;
		ReferenceSet uses;
		bool valid;

		VariableReferences():valid(true) {}

		void addDependency(const AbstractDependencePtr& dep);
	};

	typedef std::map<const OPS::Reprise::VariableDeclaration*, VariableReferences> Var2References;

	Var2References m_var2refs;
    VariableSet m_pointers;
	ControlFlowGraphExpr m_cfg;
	StatementBase* m_rootStmt;

	void registerExpr(const OPS::Reprise::ExpressionBase&, const AbstractDependencePtr& dep);
    VariableSet runDefUseAnalysis();
    VariableSet runPointerAnalysis();
};

}
}
}
