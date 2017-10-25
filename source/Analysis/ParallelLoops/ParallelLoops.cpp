#include "Analysis/ParallelLoops.h"
#include "Analysers.h"
#include "Shared/NodesCollector.h"
#include "Analysis/ControlFlowGraph.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"

namespace OPS
{
namespace Analysis
{

using namespace OPS::Reprise;

namespace ParallelLoops
{

/// Проверяет, что цикл имеет 1 вход и 1 выход
bool isBasicBlockLoop(const ControlFlowGraph& cfGraph, ForStatement& loop)
{
	ControlFlowGraph::EdgeList inEdges = cfGraph.getInEdges(loop);
	ControlFlowGraph::EdgeList outEdges = cfGraph.getOutEdges(loop);

	// нужно чтобы все входящие дуги входили в оператор цикла
	ControlFlowGraph::EdgeList::const_iterator it = inEdges.begin();
	for(; it != inEdges.end(); ++it)
	{
		if (it->second != &loop)
			return false;
	}

	// а все исходящие дуги выходили из оператора цикла
	it = outEdges.begin();
	for(; it != outEdges.end(); ++it)
	{
		if (it->first != &loop)
			return false;
	}

	return true;
}

bool isStdSafeSubroutine(SubroutineDeclaration& decl)
{
	const char* safeSubroutines[] = {"malloc", "calloc", "realloc", "free", "posix_memalign", "sqrt"};
	for(size_t i = 0; i < sizeof(safeSubroutines)/sizeof(safeSubroutines[0]); ++i)
	{
		if (decl.getName() == safeSubroutines[i])
			return true;
	}
	return false;
}

std::list<CallExpressionBase*> getDirtyCallList(RepriseBase& node)
{
	std::list<CallExpressionBase*> dirtyCalls;

	// Проверяем, что не вызываются обычные функции с побочными эффектами
	{
		std::vector<SubroutineCallExpression*> calls =
				OPS::Shared::collectNodes<SubroutineCallExpression>(node);

		for(size_t i = 0; i < calls.size(); ++i)
		{
			if (!calls[i]->hasExplicitSubroutineDeclaration())
			{
				dirtyCalls.push_back(calls[i]);
			}
			else
			{
				SubroutineDeclaration* subDecl = &calls[i]->getExplicitSubroutineDeclaration();
				if (subDecl->hasDefinition())
					subDecl = &subDecl->getDefinition();
				if (!subDecl->hasImplementation())
				{
					if (!isStdSafeSubroutine(*subDecl))
						dirtyCalls.push_back(calls[i]);
				}
			}
		}
	}

	// Проверяем, что не вызываются встроенные фортрановские функции
	{
		using namespace OPS::Reprise::Canto;

		std::vector<HirFIntrinsicCallExpression*> instrinsicCalls =
				OPS::Shared::collectNodes<HirFIntrinsicCallExpression>(node);

		for(size_t i = 0; i < instrinsicCalls.size(); ++i)
		{
			HirFIntrinsicCallExpression::IntrinsicKind kind = instrinsicCalls[i]->getKind();
			if (kind == HirFIntrinsicCallExpression::IK_WRITE ||
				kind == HirFIntrinsicCallExpression::IK_READ)
			{
				dirtyCalls.push_back(instrinsicCalls[i]);
			}
		}
	}

	return dirtyCalls;
}

PrivatizationCondition::PrivatizationCondition(const VariableList &privateVariables)
	:m_privateVariables(privateVariables)
{
}

const PrivatizationCondition::VariableList& PrivatizationCondition::getPrivateVariables() const
{
	return m_privateVariables;
}

std::string PrivatizationCondition::dump() const
{
	std::string str = "Privatization of ";
	for(VariableList::const_iterator it = m_privateVariables.begin(); it != m_privateVariables.end(); ++it)
	{
		str += (*it)->getName() + ", ";
	}
	str += " is requred";
	return str;
}

ReductionCondition::ReductionCondition(Reprise::VariableDeclaration &variable, Reprise::BasicCallExpression::BuiltinCallKind callKind)
	:m_variable(&variable)
	,m_callKind(callKind)
{
}

Reprise::VariableDeclaration& ReductionCondition::getVariable() const
{
	return *m_variable;
}

Reprise::BasicCallExpression::BuiltinCallKind ReductionCondition::getOperation() const
{
	return m_callKind;
}

std::string ReductionCondition::dump() const
{
	std::string str = "Reduction of \""
			+ m_variable->getName() + "\" by \""
			+ OPS::Reprise::BasicCallExpression::builtinCallKindToString(m_callKind)
			+ "\" is required.";
	return str;
}

std::string PreliminarySendCondition::dump() const
{
	return "Preliminary data distribution is required.";
}

std::string StripMiningCondition::dump() const
{
	return "Strip Mining is required.";
}

DirtyCallsObstacle::DirtyCallsObstacle(const LoopParallelInfo &loopInfo, const CallList &calls)
	:m_loopInfo(loopInfo)
	,m_calls(calls)
{
}

std::string DirtyCallsObstacle::dump() const
{
	std::string callNames;
	for(CallList::const_iterator it = m_calls.begin(); it != m_calls.end(); ++it)
	{
		if (it != m_calls.begin())
			callNames += ", ";
		callNames += (*it)->dumpState();
	}

	return "Loop body contains dirty function call(s): " + callNames;
}

std::string BadControlFlowObstacle::dump() const
{
	return "Loop has more than one entry or exit point";
}

std::string NonBasicForObstacle::dump() const
{
	return "Loop is not basic";
}

std::string NonConstantBoundsObstacle::dump() const
{
	return "Loop has non constant bound(s)";
}

std::string ForbiddenStatementsObstacle::dump() const
{
	return "Loop body contains forbidden statement(s)";
}

DependenceObstacle::DependenceObstacle(AbstractDependencePtr dependence)
	:m_dependence(dependence)
{
}

std::string DependenceObstacle::getDependenceType() const
{
	switch(m_dependence->getType())
	{
	case AbstractDependence::Flow: return "flow";
	case AbstractDependence::Anti: return "anti";
	case AbstractDependence::Output: return "output";
	case AbstractDependence::Input: return "input";
	default: return "";
	}
}

CarriedDependenceObstacle::CarriedDependenceObstacle(AbstractDependencePtr dependence)
	:DependenceObstacle(dependence)
{
}

ExpressionBase* getOccurenceExpression(ReferenceExpression* occur)
{
	ExpressionBase* expr = occur;

	while(expr->getParent()->is_a<BasicCallExpression>())
	{
		BasicCallExpression* basicCall = static_cast<BasicCallExpression*>(expr->getParent());
		if (basicCall->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			expr = basicCall;
		}
		else
		{
			break;
		}
	}

	return expr;
}

std::string CarriedDependenceObstacle::dump() const
{
	return "Loop has carried " + getDependenceType() + " dependence from " +
			from()->dumpState() + " to " +
			to()->dumpState();
}

BackwardDependenceObstacle::BackwardDependenceObstacle(AbstractDependencePtr dependence)
	:DependenceObstacle(dependence)
{
}

std::string BackwardDependenceObstacle::dump() const
{
	return "Loop has backward dependence from " +
			from()->dumpState() + " to " +
			to()->dumpState();
}

TargetParallelInfo::TargetParallelInfo(Target target)
	:m_target(target)
{
}

LoopParallelInfo::LoopParallelInfo(Reprise::ForStatement& loop)
	:m_loop(&loop)
	,m_basicBlock(false)
{
}

std::tr1::shared_ptr<LoopParallelInfo> LoopParallelInfo::create(Reprise::ForStatement &loop, ControlFlowGraph &cfGraph)
{
	std::tr1::shared_ptr<LoopParallelInfo> info( new LoopParallelInfo(loop) );

	info->m_dirtyCalls = getDirtyCallList(loop);
	info->m_basicBlock = isBasicBlockLoop(cfGraph, loop);

	return info;
}

bool LoopParallelInfo::isBasicBlock() const
{
	return m_basicBlock;
}

int LoopParallelInfo::getTargetCount() const
{
	return m_targetToConditionsMap.size();
}

const TargetParallelInfo& LoopParallelInfo::getTargetInfo(Target target) const
{
	std::tr1::shared_ptr<TargetParallelInfo> info = getTargetInfoPtr(target);

	if (info.get() != 0)
		return *info;
	else
		throw OPS::RuntimeError("LoopParallelInfo::getTargetInfo : invalid target");
}

std::tr1::shared_ptr<TargetParallelInfo> LoopParallelInfo::getTargetInfoPtr(Target target) const
{
	TargetToConditionListMap::const_iterator it = m_targetToConditionsMap.find(target);

	if (it != m_targetToConditionsMap.end())
		return it->second;
	else
		return std::tr1::shared_ptr<TargetParallelInfo>();
}

void LoopParallelInfo::addTargetInfo(std::tr1::shared_ptr<TargetParallelInfo> targetInfo)
{
	m_targetToConditionsMap[targetInfo->getTarget()] = targetInfo;
}

ParallelLoopsMarker::ParallelLoopsMarker(Target target)
	:m_targets(target)
	,m_useLatticeGraphs(true)
	,m_useMontegoDepGraph(true)
{
}

ParallelLoopsMarker::ParallelLoopsMarker(unsigned int targets)
	:m_targets(targets)
	,m_useLatticeGraphs(true)
	,m_useMontegoDepGraph(true)
{
}

void ParallelLoopsMarker::setUseLatticeGraphs(bool use)
{
	m_useLatticeGraphs = use;
}

void ParallelLoopsMarker::setUseMontegoDepGraph(bool use)
{
	m_useMontegoDepGraph = use;
}

std::tr1::shared_ptr<LoopParallelInfo> ParallelLoopsMarker::markLoop(Reprise::ForStatement &forStmt, OPS::Shared::ProgramContext *context) const
{
	std::list<Reprise::ForStatement*> loops;
	loops.push_back(&forStmt);
	std::tr1::shared_ptr<LoopToMarksMap> results = markLoops(forStmt.getRootBlock(), loops, context);
	if (results.get())
	{
		return (*results)[&forStmt];
	}
	else
	{
		return std::tr1::shared_ptr<LoopParallelInfo>();
	}
}

std::tr1::shared_ptr<LoopToMarksMap> ParallelLoopsMarker::markLoops(Reprise::BlockStatement &block, OPS::Shared::ProgramContext *context) const
{
	return markLoops(block, OPS::Shared::collectNodesList<OPS::Reprise::ForStatement>(block), context);
}

std::tr1::shared_ptr<LoopToMarksMap> ParallelLoopsMarker::markLoops(Reprise::SubroutineDeclaration &subroutine, OPS::Shared::ProgramContext *context) const
{
	if (subroutine.hasImplementation())
	{
		return markLoops(subroutine.getBodyBlock(), OPS::Shared::collectNodesList<Reprise::ForStatement>(subroutine.getBodyBlock()), context);
	}
	return std::tr1::shared_ptr<LoopToMarksMap>(new LoopToMarksMap);
}

std::unique_ptr<LoopToMarksMap> ParallelLoopsMarker::markLoops(ProgramUnit &program, Shared::ProgramContext *context) const
{
	std::unique_ptr<LoopToMarksMap> result(new LoopToMarksMap);

	for(int iUnit = 0; iUnit < program.getUnitCount(); ++iUnit)
	{
		Declarations::SubrIterator subrIt = program.getUnit(iUnit).getGlobals().getFirstSubr();
		for(; subrIt.isValid(); ++subrIt)
		{
			if (subrIt->hasImplementation())
			{
				std::tr1::shared_ptr<LoopToMarksMap> loops = markLoops(*subrIt, context);
				if (loops.get() != 0 && !loops->empty())
					result->insert(loops->begin(), loops->end());
			}
		}
	}
	return result;
}

/*
std::unique_ptr<ResultsResults> ParallelLoopsMarker::markLoops(const LoopList& forList) const
{
	std::unique_ptr<ResultsResults> result(new ResultsResults);

	// разбить все циклы по функциям
	typedef std::map<BlockStatement*, LoopList> BlockToLoopListMap;
	BlockToLoopListMap rootToLoops;

	for(LoopList::const_iterator it = forList.begin(); it != forList.end(); ++it)
	{
		rootToLoops[(*it)->getRootBlock());
	}

	// для каждой функции вызвать анализатор
	for(BlockToLoopListMap::const_iterator it = rootToLoops.begin(); it != rootToLoops.end(); ++it)
	{
	}

	return result;
}
*/

std::tr1::shared_ptr<LoopToMarksMap> ParallelLoopsMarker::markLoops(Reprise::BlockStatement &parentBlock, const LoopList &loops, OPS::Shared::ProgramContext *context) const
{
	std::tr1::shared_ptr<LoopToMarksMap> result(new LoopToMarksMap);

	// Построить для блока CFG и DepGraph
	std::unique_ptr<AbstractDepGraph> depGraph(m_useMontegoDepGraph
											 ? AbstractDepGraph::buildMontegoGraph(parentBlock, m_useLatticeGraphs, context)
											 : AbstractDepGraph::buildLamportGraph(parentBlock, m_useLatticeGraphs));

	ControlFlowGraph controlGraph(parentBlock);

	for(LoopList::const_iterator it = loops.begin(); it != loops.end(); ++it)
	{
		result->insert(std::make_pair(*it, LoopParallelInfo::create(**it, controlGraph)));
	}

	typedef std::vector<LoopAnalyserBase*> Analysers;

	Analysers analysers;
	if ((m_targets & SISM) != 0)
		analysers.push_back(new SIMDAnalyser(*depGraph, SIMDAnalyser::SharedMemory));
	if ((m_targets & SIDM) != 0)
		analysers.push_back(new SIMDAnalyser(*depGraph, SIMDAnalyser::DistributedMemory));
	if ((m_targets & MISM) != 0)
		analysers.push_back(new MISMAnalyser(*depGraph));
	if ((m_targets & MIDM) != 0)
		analysers.push_back(new MIDMAnalyser(*depGraph));

	// Для каждого цикла вызвать
	for(LoopList::const_iterator it = loops.begin(); it != loops.end(); ++it)
	{
		LoopParallelInfo& info = *(*result)[*it];
		for(size_t i = 0; i < analysers.size(); ++i)
		{
			std::tr1::shared_ptr<TargetParallelInfo> targetInfo = analysers[i]->analyse(info);
			info.addTargetInfo(targetInfo);
		}
	}

	for(size_t i = 0; i < analysers.size(); ++i)
		delete analysers[i];

	return result;
}
}
}

namespace Stage
{
using namespace OPS::Analysis::ParallelLoops;

FindParallelLoopsPass::FindParallelLoopsPass(bool useMontegoGraph, bool useLatticeGraph)
	:m_useMontegoGraph(useMontegoGraph)
	,m_useLatticeGraph(useLatticeGraph)
{
}

bool FindParallelLoopsPass::run()
{
	ParallelLoopsMarker marker(MISM);
	marker.setUseLatticeGraphs(m_useLatticeGraph);
	marker.setUseMontegoDepGraph(m_useMontegoGraph);

	std::unique_ptr<LoopToMarksMap> loops = marker.markLoops(workContext().program());
	workContext().addService<LoopToMarksMap>(loops.release());
	return false;
}

AnalysisUsage FindParallelLoopsPass::getAnalysisUsage() const
{
	return AnalysisUsage().addRequired<AliasCanonicalForm>();
}

}
}
