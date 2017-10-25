#include "Analysis/AbstractDepGraph.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"
#include "Shared/ProgramContext.h"
#include "Shared/LoopShared.h"
#include "Analysis/Montego/DependenceGraph/SymbolicAnalysis.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Analysis
{

AbstractOccurrence::AbstractOccurrence(Reprise::ExpressionBase *expr, int bracketCount, const LoopSet &loops)
	:m_expr(expr)
	,m_bracketCount(bracketCount)
	,m_loops(loops)
{
}

Reprise::StatementBase* AbstractOccurrence::getStatement() const
{
	return m_expr->obtainParentStatement();
}

bool AbstractOccurrence::isIncludedByForBody(Reprise::ForStatement &forStmt)
{
	return m_loops.find(&forStmt) != m_loops.end();
}

AbstractDependence::AbstractDependence(Type type, AbstractOccurrencePtr begin, AbstractOccurrencePtr end, const LoopSet &carriers)
	:m_type(type)
	,m_begin(begin)
	,m_end(end)
	,m_carriers(carriers)
{
}

ExpressionBase* AbstractDependence::getBeginExpr() const
{
	return m_begin->getExpr();
}

ExpressionBase* AbstractDependence::getEndExpr() const
{
	return m_end->getExpr();
}

StatementBase* AbstractDependence::getBeginStatement() const
{
	return m_begin->getStatement();
}

StatementBase* AbstractDependence::getEndStatement() const
{
	return m_end->getStatement();
}

AbstractOccurrence* AbstractDependence::getBeginOccurrence() const
{
	return m_begin.get();
}

AbstractOccurrence* AbstractDependence::getEndOccurrence() const
{
	return m_end.get();
}

bool AbstractDependence::isScalarVariable() const
{
	ReferenceExpression* beginRef = getBeginExpr()->cast_ptr<ReferenceExpression>();
	ReferenceExpression* endRef = getEndExpr()->cast_ptr<ReferenceExpression>();

	if (beginRef == 0 || endRef == 0)
		return false;

	if (&beginRef->getReference() != & endRef->getReference())
		return false;

	return beginRef->getReference().getType().is_a<BasicTypeBase>();
}

bool AbstractDependence::testSupp(Reprise::ForStatement &forStmt)
{
	return m_carriers.find(&forStmt) != m_carriers.end();
}

void AbstractDependence::eraseSupp(Reprise::ForStatement &forStmt)
{
	m_carriers.erase(&forStmt);
}

class CarrierPred
{
public:
	CarrierPred(ForStatement& loop):m_loop(loop) {}
	bool operator()(AbstractDependence& dep) const
	{
		return dep.testSupp(m_loop) &&
			   dep.getBeginOccurrence()->isIncludedByForBody(m_loop) &&
			   dep.getEndOccurrence()->isIncludedByForBody(m_loop);
	}
	ForStatement& m_loop;
};

AbstractDepGraph* AbstractDepGraph::getSubGraphByCarrier(Reprise::ForStatement &forStmt)
{
	return getSubGraphByPred(CarrierPred(forStmt));
}

class LoopBodyPred
{
public:
	LoopBodyPred(ForStatement& loop):m_loop(loop) {}
	bool operator()(AbstractDependence& dep) const
	{
		return dep.getBeginOccurrence()->isIncludedByForBody(m_loop) &&
			   dep.getEndOccurrence()->isIncludedByForBody(m_loop);
	}
	ForStatement& m_loop;
};

AbstractDepGraph* AbstractDepGraph::getSubGraphByLoopBody(Reprise::ForStatement &forStmt)
{
	return getSubGraphByPred(LoopBodyPred(forStmt));
}

DependenceList::iterator AbstractDepGraph::begin()
{
	return m_dependencies.begin();
}

DependenceList::iterator AbstractDepGraph::end()
{
	return m_dependencies.end();
}

int AbstractDepGraph::getDependenceCount() const
{
	return (int)m_dependencies.size();
}

bool AbstractDepGraph::isEmpty() const
{
	return m_dependencies.empty();
}

void AbstractDepGraph::append(AbstractDependencePtr dependence)
{
	m_dependencies.push_back(dependence);
}

void AbstractDepGraph::remove(AbstractDependencePtr dependence)
{
	m_dependencies.remove(dependence);
}

namespace
{
struct IsDepInSetPred
{
	IsDepInSetPred(const std::set<AbstractDependencePtr>& dependencies)
		:m_dependencies(dependencies) {}

	const std::set<AbstractDependencePtr>& m_dependencies;

	bool operator()(const AbstractDependencePtr& ptr)
	{
		return m_dependencies.find(ptr) != m_dependencies.end();
	}
};

}

void AbstractDepGraph::remove(const std::set<AbstractDependencePtr>& dependencies)
{
	m_dependencies.remove_if(IsDepInSetPred(dependencies));
}

typedef std::map<const DepGraph::OccurDesc*, AbstractOccurrencePtr> OccurDescToOccur;

AbstractOccurrencePtr findOrCreateOccurrence(OccurDescToOccur& occurMap,
											 const DepGraph::OccurDesc* occurr)
{
	OccurDescToOccur::iterator it = occurMap.find(occurr);
	if (it != occurMap.end())
		return it->second;

	LoopSet loops;
	for(int i = 0; i < occurr->loopNumb; ++i)
		loops.insert(occurr->loops[i].stmtFor);

	AbstractOccurrencePtr newOccurr(new AbstractOccurrence(occurr->pOccur, occurr->dim, loops));
	occurMap[occurr] = newOccurr;
	return newOccurr;
}

AbstractDepGraph* AbstractDepGraph::buildLamportGraph(Reprise::BlockStatement &block, bool useLatticeGraphs)
{
	using namespace ::DepGraph;

	DepGraph::LamportGraph lg;
	lg.Build(block, LMP_CONSIDER_INPUTDEP, true);
	lg.Refine(useLatticeGraphs ? RM_ELEM_LATTICE : RM_NO_REFINEMENT);

	AbstractDepGraph* graph = new AbstractDepGraph;

	OccurDescToOccur occurMap;

	LampArrowIterator it = lg.Begin();

	for(; it != lg.End(); ++it)
	{
		LampArrow& arrow = **it;

		AbstractDependence::Type type;
		switch(arrow.type)
		{
		case FLOWDEP: type = AbstractDependence::Flow; break;
		case ANTIDEP: type = AbstractDependence::Anti; break;
		case OUTPUTDEP: type = AbstractDependence::Output; break;
		case INPUTDEP: type = AbstractDependence::Input; break;
		OPS_DEFAULT_CASE_LABEL
		}

		AbstractOccurrencePtr begin = findOrCreateOccurrence(occurMap, arrow.srcOccurDesc),
								end = findOrCreateOccurrence(occurMap, arrow.depOccurDesc);
		LoopSet carriers;

		for(int i = 0; i < arrow.commonLoopNumb; ++i)
		{
			if (arrow.TestSupp(i))
				carriers.insert(arrow.srcOccurDesc->loops[i].stmtFor);
		}

		graph->append(AbstractDependencePtr( new AbstractDependence(type, begin, end, carriers) ));
	}

	return graph;
}

typedef std::map<Montego::Occurrence*, AbstractOccurrencePtr> ExprToOccur;

AbstractOccurrencePtr findOrCreateOccurrence(ExprToOccur &occurMap, const Montego::DependenceGraphVertex& v)
{
	ExprToOccur::iterator it = occurMap.find(v.getSourceOccurrence().get());
	if (it != occurMap.end())
		return it->second;

	std::list<ForStatement*> loopsList = OPS::Shared::getEmbracedLoopsNest(*v.getParentStatement());
	LoopSet loops(loopsList.begin(), loopsList.end());

	int bracketCount = 0;

	if (Montego::BasicOccurrence* basicOccurr = v.getSourceOccurrence()->cast_ptr<Montego::BasicOccurrence>())
		bracketCount = basicOccurr->getBracketCount();

	AbstractOccurrencePtr newOccurr(new AbstractOccurrence(v.getSourceOccurrence()->getSourceExpression(), bracketCount, loops));
	occurMap[v.getSourceOccurrence().get()] = newOccurr;
	return newOccurr;

}

AbstractDepGraph* AbstractDepGraph::buildMontegoGraph(Reprise::BlockStatement &block, bool useLatticeGraphs, OPS::Shared::ProgramContext* context)
{
	using namespace OPS::Montego;

	DependenceGraph dg(context, block);

	if (useLatticeGraphs)
		dg.refineAllArcsWithLatticeGraph();

	AbstractDepGraph* graph = new AbstractDepGraph;

	ExprToOccur occurMap;

	DependenceGraph::ArcList arcs = dg.getAllArcs();
	DependenceGraph::ArcList::const_iterator it = arcs.begin();
	for(; it != arcs.end(); ++it)
	{
		const DependenceGraphAbstractArc& arc = *it->get();

		if (arc.getDependenceType() == DependenceGraphAbstractArc::DT_TRIVIAL_DEPENDENCE)
			continue;

		AbstractDependence::Type type;
		switch(arc.getDependenceType())
		{
		case DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE: type = AbstractDependence::Flow; break;
		case DependenceGraphAbstractArc::DT_ANTIDEPENDENCE: type = AbstractDependence::Anti; break;
		case DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE: type = AbstractDependence::Output; break;
		case DependenceGraphAbstractArc::DT_ININ_DEPENDENCE: type = AbstractDependence::Input; break;
		OPS_DEFAULT_CASE_LABEL
		}

		AbstractOccurrencePtr begin = findOrCreateOccurrence(occurMap, arc.getStartVertex()),
								end = findOrCreateOccurrence(occurMap, arc.getEndVertex());
		LoopSet carriers;
		std::list<ForStatement*> loops = Shared::getEmbracedLoopsNest(*arc.getStartVertex().getParentStatement());
		std::vector<bool> carrierFlags;
		try
		{
			carrierFlags = Montego::findDepSupp(arc, dg);
		}
		catch(const OPS::Exception&)
		{
			carrierFlags.resize(loops.size(), true);
		}

		OPS_ASSERT(carrierFlags.size() <= loops.size());

		std::list<ForStatement*>::const_iterator itLoop = loops.begin();
		for(size_t i = 0; i < carrierFlags.size(); ++i, ++itLoop)
			if (carrierFlags[i])
				carriers.insert(*itLoop);

		AbstractDependencePtr dep(new AbstractDependence(type, begin, end, carriers));
		graph->append(dep);

		if (context)
		{
			context->getMetaInformationSafe<OPS::Montego::SymbolicAnalysis::SymbolicPredicatesMeta>().refineDependence(*dep);
		}
	}

	return graph;
}

}
}
