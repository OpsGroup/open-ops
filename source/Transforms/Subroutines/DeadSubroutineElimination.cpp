#include "Transforms/Subroutines/DeadSubroutineElimination.h"
#include "Analysis/CallGraph.h"

namespace OPS
{
namespace Transforms
{
namespace Subroutines
{

int removeDeadSubroutines(OPS::Reprise::SubroutineDeclaration &mainSubroutine)
{
	CallGraph callGraph(&mainSubroutine);

	std::set<OPS::Reprise::SubroutineDeclaration*> usedSubroutines;

	for(CallGraph::NodeMap::const_iterator it = callGraph.getGraph().begin();
										   it != callGraph.getGraph().end(); ++it)
	{
		usedSubroutines.insert(it->second->getSubProc());
	}

	return removeDeadSubroutines(usedSubroutines);
}

class SubroutineRemover : public OPS::Reprise::Service::DeepWalker
{
public:
	SubroutineRemover(const std::set<OPS::Reprise::SubroutineDeclaration*>& exceptList)
		:m_exceptList(exceptList)
		,m_removedCount(0) {}

	void visit(OPS::Reprise::Declarations& declarations);
	void visit(OPS::Reprise::BlockStatement&) { /* optimization */ }

	int getRemovedCount() const { return m_removedCount; }

private:
	const std::set<OPS::Reprise::SubroutineDeclaration*> m_exceptList;
	int m_removedCount;
};

void SubroutineRemover::visit(OPS::Reprise::Declarations &declarations)
{
	OPS::Reprise::Declarations::SubrIterator it = declarations.getFirstSubr();
	while(it.isValid())
	{
		OPS::Reprise::SubroutineDeclaration* subDecl = &*it;

		if (!subDecl->hasImplementation() && subDecl->hasDefinition())
			subDecl = &subDecl->getDefinition();

		if (m_exceptList.find(subDecl) == m_exceptList.end())
		{
			OPS::Reprise::Declarations::Iterator tmpIt = it.base();
			it++;
			declarations.erase(tmpIt);
			m_removedCount++;
		}
		else
		{
			it++;
		}
	}

	DeepWalker::visit(declarations);
}

int removeDeadSubroutines(const std::set<OPS::Reprise::SubroutineDeclaration*>& usedSubroutines)
{
	if (usedSubroutines.empty())
	{
		throw OPS::RuntimeError("removeDeadSubroutines: trying to remove all subroutines from program");
	}

	OPS::Reprise::RepriseBase* unit = (*usedSubroutines.begin())->findProgramUnit();

	if (unit == 0)
	{
		unit = (*usedSubroutines.begin())->findTranslationUnit();
		if (unit == 0)
			throw OPS::RuntimeError("removeDeadSubroutines: program or translation unit not found");
	}

	SubroutineRemover removerWalker(usedSubroutines);
	unit->accept(removerWalker);
	return removerWalker.getRemovedCount();
}

}
}
}
