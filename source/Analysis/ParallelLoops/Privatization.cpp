#include "Privatization.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Shared/StatementsShared.h"
#include <iterator>

namespace OPS
{
namespace Analysis
{
namespace ParallelLoops
{

using namespace OPS::Reprise;

void PrivatizationHelper::VariableReferences::addDependency(const AbstractDependencePtr &dep)
{
	if (dependencies.find(dep) == dependencies.end())
	{
		dependencies.insert(dep);

		ReferenceExpression* begin = dep->getBeginExpr()->cast_ptr<ReferenceExpression>();
		ReferenceExpression* end = dep->getEndExpr()->cast_ptr<ReferenceExpression>();

		if (begin == 0 || end == 0 || &begin->getReference() != &end->getReference())
		{
			valid = false;
			return;
		}

		switch(dep->getType())
		{
		case AbstractDependence::Flow: definitions.insert(begin); uses.insert(end); break;
		case AbstractDependence::Anti: uses.insert(begin); definitions.insert(end); break;
		case AbstractDependence::Output: definitions.insert(begin); definitions.insert(end); break;
		case AbstractDependence::Input: uses.insert(begin); uses.insert(end); break;
		}
	}
}

PrivatizationHelper::PrivatizationHelper(StatementBase &stmt, AbstractDepGraph &depGraph)
	:m_rootStmt(&stmt)
{
	ControlFlowGraphExprBuilder cfgBuilder(false);
	cfgBuilder.build(stmt, m_cfg);

    // находим все внутренние переменные-указатели
	SubroutineDeclaration& subroutine = stmt.getRootBlock().getParent()->cast_to<SubroutineDeclaration>();
    Declarations::VarIterator itVar = subroutine.getDeclarations().getFirstVar();
    for(; itVar.isValid(); ++itVar)
    {
        if (itVar->hasDefinedBlock() &&
			Shared::contain(&stmt, &itVar->getDefinedBlock()) &&
            Editing::desugarType(itVar->getType()).is_a<PtrType>())
        {
            m_pointers.insert(&*itVar);
        }
    }

    // находим все зависимости по скалярным переменным
	DependenceList::iterator it = depGraph.begin();
	for(; it != depGraph.end(); ++it)
	{
		AbstractDependencePtr& dep = *it;
		registerExpr(*dep->getBeginExpr(), dep);
		registerExpr(*dep->getEndExpr(), dep);
	}
}

void PrivatizationHelper::registerExpr(const OPS::Reprise::ExpressionBase& expr, const AbstractDependencePtr& dep)
{
	if (const ReferenceExpression* ref = expr.cast_ptr<const ReferenceExpression>())
	{
        const TypeBase& realType = Editing::desugarType(ref->getReference().getType());
        if (realType.is_a<BasicType>() || m_pointers.find(&ref->getReference()) != m_pointers.end())
			m_var2refs[&ref->getReference()].addDependency(dep);
	}
    if (const BasicCallExpression* bce = expr.cast_ptr<const BasicCallExpression>())
    {
        if (bce->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        {
            registerExpr(bce->getArgument(0), dep);
        }
    }
}

namespace {

struct StmtDefUse
{
	typedef std::set<const VariableDeclaration*> Variables;
	Variables uses;
	Variables defs;
	Variables up;
};

}

PrivatizationHelper::VariableSet PrivatizationHelper::analyse()
{
    VariableSet vars = runDefUseAnalysis();
    VariableSet ptrs = runPointerAnalysis();
    vars.insert(ptrs.begin(), ptrs.end());
	return vars;
}

PrivatizationHelper::VariableSet PrivatizationHelper::runDefUseAnalysis()
{
	std::map<const ControlFlowGraphExpr::BasicBlock*, StmtDefUse> graph;
	std::set<const VariableDeclaration*> testedVars;

	// заполнить use(x) и def(x)
	{
		Var2References::const_iterator it = m_var2refs.begin();
		for(; it != m_var2refs.end(); ++it)
		{
			if (it->second.valid && !it->second.definitions.empty())
			{
				ReferenceSet::const_iterator itUse = it->second.uses.begin();
				for(; itUse != it->second.uses.end(); ++itUse)
				{
					ControlFlowGraphExpr::BasicBlock* block = m_cfg.findBlockOfExpr(*itUse);
					OPS_ASSERT(block != 0);
					graph[block].uses.insert(it->first);
				}
				ReferenceSet::const_iterator itDef = it->second.definitions.begin();
				for(; itDef != it->second.definitions.end(); ++itDef)
				{
					ControlFlowGraphExpr::BasicBlock* block = m_cfg.findBlockOfExpr(*itDef);
					OPS_ASSERT(block != 0);
					graph[block].defs.insert(it->first);
				}
				testedVars.insert(it->first);
			}
		}
	}

	if (graph.empty())
		return std::set<const VariableDeclaration*>();

	// решить уравнение

	bool changed = false;

	do
	{
		changed = false;
		ControlFlowGraphExpr::BasicBlockList::const_iterator itBlock = m_cfg.getBlocks().begin();
		for(; itBlock != m_cfg.getBlocks().end(); ++itBlock)
		{
			ControlFlowGraphExpr::BasicBlock& basicBlock = **itBlock;
			StmtDefUse& node = graph[&basicBlock];
			StmtDefUse::Variables newUp = node.uses;
			StmtDefUse::Variables succUp;

			ControlFlowGraphExpr::BasicBlockList::const_iterator itSucc = basicBlock.getOutBlocks().begin();
			for(; itSucc != basicBlock.getOutBlocks().end(); ++itSucc)
			{
				StmtDefUse& succDefUse = graph[*itSucc];
				succUp.insert(succDefUse.up.begin(), succDefUse.up.end());
			}

			std::set_difference(succUp.begin(), succUp.end(), node.defs.begin(), node.defs.end(),
				std::inserter(newUp, newUp.end()));

			if (newUp != node.up)
			{
				changed = true;
				node.up.swap(newUp);
			}
		}
	}
	while(changed == true);

	std::set<const VariableDeclaration*> decls;
	StmtDefUse& rootNode = graph[&m_cfg.getEntry()];

	std::set_difference(testedVars.begin(), testedVars.end(), rootNode.up.begin(), rootNode.up.end(),
						std::inserter(decls, decls.end()));

	return decls;
}

PrivatizationHelper::VariableSet PrivatizationHelper::runPointerAnalysis()
{
    VariableSet result;

	OPS::Shared::ProgramContext* context = 
		OPS::Shared::ProgramContext::getFromProgram(*m_rootStmt->findProgramUnit());

	OPS::Montego::OccurrenceContainerMeta* ocMeta = context->getMetaInformation<OPS::Montego::OccurrenceContainerMeta>();
	OPS::Montego::AliasInterfaceMeta* aiMeta = context->getMetaInformation<OPS::Montego::AliasInterfaceMeta>();

	if (ocMeta == 0 || aiMeta == 0)
        return result;

	OPS::Montego::OccurrenceContainer* occurContainer = ocMeta->getContainer().get();
	OPS::Montego::AliasInterface* aliasInterface = aiMeta->getInterface().get();

	if (!aliasInterface->wasBuilt())
        return result;

	BlockStatement* rootBlock = 0;

	if (m_rootStmt->is_a<ForStatement>())
		rootBlock = &m_rootStmt->cast_to<ForStatement>().getBody();
	else if (m_rootStmt->is_a<BlockStatement>())
		rootBlock = &m_rootStmt->cast_to<BlockStatement>();
	else
		return result;

    for(VariableSet::const_iterator it = m_pointers.begin(); it != m_pointers.end(); ++it)
	{
        typedef std::list<Montego::BasicOccurrencePtr> OccurrenceList;
        OccurrenceList varOccurs = occurContainer->getAllBasicOccurrencesOf(**it);
        OccurrenceList::iterator itOccur = varOccurs.begin();

        bool isPrivatizable = true;
        for(; itOccur != varOccurs.end() && isPrivatizable; ++itOccur)
        {
			isPrivatizable = aliasInterface->isOccurrencePrivate(*rootBlock, *itOccur);
        }

        if (isPrivatizable)
            result.insert(*it);
	}
    return result;
}

void PrivatizationHelper::removeDependenciesByVariables(AbstractDepGraph &graph, const VariableSet &variables)
{
    VariableSet::const_iterator itVar = variables.begin();
	for(; itVar != variables.end(); ++itVar)
	{
		Var2References::const_iterator it = m_var2refs.find(*itVar);
		if (it != m_var2refs.end())
		{
			graph.remove(it->second.dependencies);
		}
	}
}

}
}
}
