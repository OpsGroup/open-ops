#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

#include "Analysis/ControlFlowGraph.h"
#include "Analysis/InductionVariables/InductionVariables.h"
#include "Analysis/LatticeGraph/ElemLatticeGraph.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/DependenceGraph/IndexAnalysis.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Reprise/Service/DeepWalker.h"
#include "Shared/StatementsShared.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Analysis/ExpressionOrder.h"
#include "Shared/ProgramContext.h"

#include <algorithm>

using namespace OPS::Reprise;

using std::list;
using std::map;
using std::pair;
using std::tr1::shared_ptr;
using std::vector;

namespace OPS
{
namespace Montego
{

namespace
{
	bool isOccurrenceCounter(const OccurrencePtr& occurrence,
		const list<pair<ExpressionBase*, ForStatement*> >& counters)
	{
		if (!occurrence->is_a<BasicOccurrence>())
		{
			return false;
		}

		OPS_ASSERT(occurrence->getSourceExpression() != 0);
		OPS_ASSERT(occurrence->getParentStatement() != 0);

		typedef list<pair<ExpressionBase*, ForStatement*> >::const_iterator
			CountersIterator;

		for (CountersIterator it = counters.begin(); it != counters.end(); ++it)
		{
			OPS_ASSERT(it->first != 0);
			OPS_ASSERT(it->second != 0);

			if (occurrence->getSourceExpression()->isEqual(*it->first))
			{
				if (!occurrence->isGenerator() ||
					occurrence->getParentStatement() == it->second)
				{
					return true;
				}
			}
		}

		return false;
	}
}

DependenceGraph::DependenceGraph(const StatementBase& sourceStatement)
	: m_sourceStatement(sourceStatement)
	, m_aliasInterface()
	, m_occurrenceContainer()
{
	buildDependencyGraph();
}

DependenceGraph::DependenceGraph(
	shared_ptr<OccurrenceContainer> occurrenceContainer,
    shared_ptr<AliasInterface> aliasInterface, const StatementBase &sourceStatement)
	: m_sourceStatement(sourceStatement)
	, m_aliasInterface(aliasInterface)
	, m_occurrenceContainer(occurrenceContainer)
{
	buildDependencyGraph();
}

DependenceGraph::DependenceGraph(Shared::ProgramContext *context,
	const StatementBase &sourceStatement)
	: m_sourceStatement(sourceStatement)
	, m_aliasInterface()
	, m_occurrenceContainer()
{
	if (context != 0)
	{
		m_occurrenceContainer =
			context->getMetaInformationSafe<OccurrenceContainerMeta>().getContainer();
		m_aliasInterface =
			context->getMetaInformationSafe<AliasInterfaceMeta>().getInterface();
	}

	buildDependencyGraph();
}

DependenceGraph::~DependenceGraph()
{
}

shared_ptr<AliasInterface> DependenceGraph::getAliasInterface() const
{
	return m_aliasInterface;
}

/*
std::tr1::shared_ptr<OccurrenceContainer> DependenceGraph::getOccurrenceContainer() const
{
	return m_occurrenceContainer;
}
*/

Analysis::InductionAnalysis& DependenceGraph::inductionAnalysis() const
{
	OPS_ASSERT(m_inductionAnalysis.get() != 0);

	return *m_inductionAnalysis;
}

#define USE_INDUCTION_ANALYSIS 0

void DependenceGraph::buildDependencyGraph()
{
	OPS_ASSERT(m_arcList.size() == 0);

	// perform alias analysis

	//TODO: optimize
	ProgramUnit* programUnit = m_sourceStatement.findProgramUnit();
    
	OPS_ASSERT(programUnit != 0);

	if (m_occurrenceContainer == 0)
	{
		m_occurrenceContainer.reset(new OccurrenceContainer(*programUnit));
	}

	//TODO: wtf? extract away! use OPS_ASSERT instead!
	CanonicalFormChecker canonicalChecker(*m_occurrenceContainer);
	if (!canonicalChecker.isFragmentCanonical(*programUnit))
	{
		throw OPS::RuntimeError("The program is not in canonical form(" +
			canonicalChecker.getReason() + ")");
	}
	
	if (m_aliasInterface == 0)
	{
        m_aliasInterface.reset(AliasInterface::create(*programUnit, *m_occurrenceContainer));
	}

	if (!m_aliasInterface->wasBuilt())
	{
        if (1 == m_aliasInterface->runAliasAnalysis())
		{
			// wtf? program already in canonical form..
			//TODO: use OPS_ASSERT instead!
			throw OPS::RuntimeError(
				"Alias analysis failed: the program is not in canonical form");
		}
	}

	vector<vector<OccurrencePtrWithGeneratorInformation> > aliases = m_aliasInterface->getOccursByAlias(
		const_cast<StatementBase&>(m_sourceStatement));

	// find parent block

	BlockStatement* block = 0;
	if (m_sourceStatement.is_a<BlockStatement>())
	{
		block = const_cast<BlockStatement*>(
			m_sourceStatement.cast_ptr<BlockStatement>());
	}
	else
	{
		OPS_ASSERT(m_sourceStatement.hasParentBlock());

		block = const_cast<BlockStatement*>(&m_sourceStatement.getParentBlock());
	}

	OPS_ASSERT(block != 0);

	// perform induction variable analysis

	if (m_inductionAnalysis.get() == 0)
	{
#if USE_INDUCTION_ANALYSIS
		m_inductionAnalysis.reset(new Analysis::InductionAnalysis(*block));
#endif
	}

	// build ControlFlowGraph
	//TODO: refine ControlFlowGraph building for special cases (bug)

	// refine block for correct ControlFlowGraph building for the fragment
	block = Shared::getOuterSimpleBlock(block);

	map<BlockStatement*, shared_ptr<ControlFlowGraph> > controlFlowGraphCache;

	controlFlowGraphCache[block] =
			shared_ptr<ControlFlowGraph>(new ControlFlowGraph(*block));

	//build InformationDependencyGraph

	typedef vector<vector<OccurrencePtrWithGeneratorInformation> >::const_iterator AliasGroupConstIterator;
	typedef vector<OccurrencePtrWithGeneratorInformation>::const_iterator AliasConstIterator;

	for (AliasGroupConstIterator gIter = aliases.begin(); gIter != aliases.end(); ++gIter)
	{
		for (AliasConstIterator aIter = gIter->begin(); aIter != gIter->end(); ++aIter)
		{
			for (AliasConstIterator aIter2 = gIter->begin(); aIter2 != gIter->end();
				++aIter2)
			{
                const OccurrencePtrWithGeneratorInformation occurrenceWithInf = *aIter;
                const OccurrencePtrWithGeneratorInformation occurrence2WithInf = *aIter2;
				const OccurrencePtr occurrence = occurrenceWithInf.occurrencePtr;
				const OccurrencePtr occurrence2 = occurrence2WithInf.occurrencePtr;

				OPS_ASSERT(occurrence.get() != 0);
				OPS_ASSERT(occurrence2.get() != 0);

				BlockStatement* blockToObserve = block;

				BlockStatement* block1 = tryGetDeclarationBlock(occurrence);
				BlockStatement* block2 = tryGetDeclarationBlock(occurrence2);

				if (block1 != 0 && block2 != 0 && contains(*block, *block1) &&
					contains(*block, *block2))
				{
					if (contains(*block1, *block2))
					{
						blockToObserve = block1;
					}
					else
					{
						blockToObserve = block2;
					}

					if (controlFlowGraphCache.find(blockToObserve) ==
						controlFlowGraphCache.end())
					{
						OPS_ASSERT(blockToObserve != 0);

						controlFlowGraphCache[blockToObserve] =
							shared_ptr<ControlFlowGraph>(
								new ControlFlowGraph(*blockToObserve));
					}
				}

				//TODO: investigate
// 				if (occurence->isEqual(*occurence2))
// 				{
// 					continue;
// 				}

				StatementBase* parentStatement = occurrence->getParentStatement();
				StatementBase* parentStatement2 = occurrence2->getParentStatement();

				OPS_ASSERT(parentStatement != 0);
				OPS_ASSERT(parentStatement2 != 0);

				//TODO: implement correctly
// 				if (parentStatement == parentStatement2)
// 				{
// 					if (!occurence->isGenerator() && occurence2->isGenerator())
// 					{
// 						m_arcList.push_back(DependencyGraphArc(
// 							DependencyGraphVertex(occurence),
// 							DependencyGraphVertex(occurence2),
// 							DependencyGraphArc::AT_TRIVIAL_DEPENDENCE));
// 					}
// 				}
				if (controlFlowGraphCache[blockToObserve]->hasPath(parentStatement,
						parentStatement2) ||
					parentStatement == parentStatement2 &&
					Analysis::getExpressionOrder(*occurrence->getSourceExpression(),
                        *occurrence2->getSourceExpression()) > 0)
				{
                    std::list<DependenceGraphAbstractArc::DependenceType> dependenceTypes;

					if (occurrenceWithInf.isGenerator && occurrence2WithInf.isGenerator)
						dependenceTypes.push_back(DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE);
                    if (occurrenceWithInf.isGenerator && occurrence2WithInf.isUsage)
                        dependenceTypes.push_back(DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE);
                    if (occurrenceWithInf.isUsage && occurrence2WithInf.isGenerator)
                        dependenceTypes.push_back(DependenceGraphAbstractArc::DT_ANTIDEPENDENCE);
                    if (occurrenceWithInf.isUsage && occurrence2WithInf.isUsage)
                        dependenceTypes.push_back(DependenceGraphAbstractArc::DT_ININ_DEPENDENCE);

					std::list<DependenceGraphAbstractArc::DependenceType>::iterator it;
					for(it = dependenceTypes.begin(); it != dependenceTypes.end(); ++it)
					{
						DependenceGraphAbstractArc::DependenceType dependenceType = *it;
					#if USE_INDUCTION_ANALYSIS
						Analysis::InductionDescription* description =
							inductionAnalysis().getInductionDescription(
								occurrence->getSourceExpression());
						Analysis::InductionDescription* description2 =
							inductionAnalysis().getInductionDescription(
								occurrence2->getSourceExpression());

						OPS_ASSERT(description != 0);
						OPS_ASSERT(description2 != 0);

						if (dependenceType ==
								DependenceGraphAbstractArc::DT_ININ_DEPENDENCE &&
							description->equals(description2))
						{
							continue;
						}
					#endif
						m_arcList.push_back(shared_ptr<DependenceGraphTrueArc>(
							new DependenceGraphTrueArc(occurrence, occurrence2,
							dependenceType, m_sourceStatement)));
					}
				}
			}
		}
	}
}

DependenceGraph::ArcList DependenceGraph::internalArcListToArcList(
	const DependenceGraph::InternalArcList& internalArcList)
{
	ArcList arcList;

	typedef InternalArcList::const_iterator InternalListIterator;

	for (InternalListIterator iter = internalArcList.begin();
		 iter != internalArcList.end(); ++iter)
	{
		arcList.push_back(*iter);
	}

	return arcList;
}

BlockStatement* DependenceGraph::tryGetDeclarationBlock(
	const OccurrencePtr& occurrence)
{
	ExpressionBase* expression = occurrence->getSourceExpression();

	OPS_ASSERT(expression != 0);

	if (ReferenceExpression* refExpr = expression->cast_ptr<ReferenceExpression>())
	{
		if (refExpr->getReference().hasDefinedBlock())
			return &refExpr->getReference().getDefinedBlock();
	}

	return 0;
}

bool DependenceGraph::contains(const BlockStatement& source,
	const BlockStatement& target)
{
    if (&source == &target)
        return false;

    return OPS::Shared::contain(&source, &target);
}

const StatementBase& DependenceGraph::getSourceStatement() const
{
	return m_sourceStatement;
}

DependenceGraph::ArcList DependenceGraph::getAllArcs() const
{
	return internalArcListToArcList(m_arcList);
}

namespace
{
	class IsArcBetween
	{
	public:

		IsArcBetween(const StatementBase& startStatement, const StatementBase& endStatement)
			: m_startStatement(startStatement)
			, m_endStatement(endStatement)
		{
		}

		bool operator ()(const shared_ptr<DependenceGraphTrueArc>& arc) const
		{
			OPS_ASSERT(arc.get() != 0);

			const DependenceGraphVertex statrtVertex = arc->getStartVertex();
			const DependenceGraphVertex endVertex = arc->getEndVertex();

			return statrtVertex.getParentStatement() == &m_startStatement &&
				endVertex.getParentStatement() == &m_endStatement;
		}

	private:

		const StatementBase& m_startStatement;
		const StatementBase& m_endStatement;
	};
}

DependenceGraph::ArcList DependenceGraph::getAllArcsBetween(
	const StatementBase& startStatement, const StatementBase& endStatement) const
{
	typedef InternalArcList::const_iterator ArcListConstIterator;

	ArcList arcList;

	ArcListConstIterator iter = std::find_if(m_arcList.begin(), m_arcList.end(),
		IsArcBetween(startStatement, endStatement));

	while (iter != m_arcList.end())
	{
		arcList.push_back(*iter);

		iter = std::find_if(++iter, m_arcList.end(),
			IsArcBetween(startStatement, endStatement));
	}

	return arcList;
}

bool DependenceGraph::isArcNotInLatticeGraph(
	const shared_ptr<DependenceGraphTrueArc>& arc)
{
	OPS_ASSERT(arc.get() != 0);

	typedef InternalArcList::const_iterator ArcListConstIterator;

	ArcListConstIterator iter = std::find(m_arcList.begin(), m_arcList.end(), arc);

	if (iter == m_arcList.end())
	{
		return false;
	}

	//TODO: should be DependencyGraph method
	return !testDependenceWithLatticeGraph(*arc, *this, false);
}

void DependenceGraph::refineAllArcsWithLatticeGraph()
{
	//TODO: should be the LatticeGraph method
	if (!testApplicability(const_cast<StatementBase&>(m_sourceStatement),
		*m_occurrenceContainer, *m_aliasInterface))
	{
		return;
	}

	InternalArcList arcsAbuotToBeDeleted;

	typedef InternalArcList::const_iterator ArcListConstIterator;

	for (ArcListConstIterator iter = m_arcList.begin(); iter != m_arcList.end(); ++iter)
	{
		if (isArcNotInLatticeGraph(*iter))
		{
			arcsAbuotToBeDeleted.push_back(*iter);
		}
	}

	for (ArcListConstIterator iter = arcsAbuotToBeDeleted.begin();
		iter != arcsAbuotToBeDeleted.end(); ++iter)
	{
		m_arcList.remove(*iter);
	}
}

void DependenceGraph::removeCounterArcs()
{
	const list<pair<ExpressionBase*, ForStatement*> > counters = findAllCounters();

	typedef InternalArcList::const_iterator ArcListConstIterator;

	InternalArcList arcsAbuotToBeDeleted;

	for (ArcListConstIterator iter = m_arcList.begin(); iter != m_arcList.end(); ++iter)
	{
		DependenceGraphTrueArc& arc = **iter;

		if (isOccurrenceCounter(arc.getStartVertex().getSourceOccurrence(), counters) &&
			isOccurrenceCounter(arc.getEndVertex().getSourceOccurrence(), counters))
		{
			arcsAbuotToBeDeleted.push_back(*iter);
		}
	}

	for (ArcListConstIterator iter = arcsAbuotToBeDeleted.begin();
		iter != arcsAbuotToBeDeleted.end(); ++iter)
	{
		m_arcList.remove(*iter);
	}
}

/*
bool DependenceGraph::refineSelectedArcWithLatticeGraph(DependenceGraphArc arc)
{
	//TODO: should be the LatticeGraph method
	if (!testApplicability(*m_sourceStatement, *m_occurrenceContainer,
		*m_aliasInterface))
	{
		return false;
	}

	if (isArcNotInLatticeGraph(arc))
	{
		m_arcList.remove(arc);

		return true;
	}

	return false;
}
*/

namespace
{
	class ForFinder
		: public Service::DeepWalker
	{
	public:

		void visit(ForStatement& forStatement)
		{
			m_forStatements.push_back(&forStatement);

			Service::DeepWalker::visit(forStatement);
		}

		std::list<ForStatement*> forStatements() const
		{
			return m_forStatements;
		}

	private:

		list<ForStatement*> m_forStatements;
	};
}

list<pair<ExpressionBase*, ForStatement*> > DependenceGraph::findAllCounters() const
{
	ForFinder forFinder;

	const_cast<StatementBase&>(m_sourceStatement).accept(forFinder);

	typedef list<ForStatement*>::const_iterator ConstIterator;

	list<pair<ExpressionBase*, ForStatement*> > counters;

	const list<ForStatement*> forStatements = forFinder.forStatements();

	for (ConstIterator it = forStatements.begin(); it != forStatements.end(); ++it)
	{
		OPS_ASSERT(*it != 0);

		ExpressionBase* expectedCounter = tryGetCounter(**it);

		if (expectedCounter != 0)
		{
			counters.push_back(pair<ExpressionBase*, ForStatement*>(
				expectedCounter, *it));
		}
	}

	return counters;
}

bool isExpressionIn(const ExpressionBase& expression,
	const vector<OccurrencePtr>& occurrences)
{
	typedef std::vector<OccurrencePtr>::const_iterator ConstIterator;

	for (ConstIterator it = occurrences.begin(); it != occurrences.end(); ++it)
	{
		OPS_ASSERT(it->get() != 0);
		OPS_ASSERT((*it)->getSourceExpression() != 0);

		if (expression.isEqual(*(*it)->getSourceExpression()))
		{
			return true;
		}
	}

	return false;
}

ExpressionBase* DependenceGraph::tryGetCounter(
	const ForStatement& forStatement) const
{
	const ExpressionBase& initExpr = forStatement.getInitExpression();

	if (!initExpr.is_a<BasicCallExpression>())
	{
		return 0;
	}
	const BasicCallExpression& assign = initExpr.cast_to<BasicCallExpression>();

	if (assign.getKind() != BasicCallExpression::BCK_ASSIGN)
	{
		return 0;
	}

	const ExpressionBase& expectedCounter = assign.getArgument(0);

	if (!expectedCounter.is_a<ReferenceExpression>())
	{
		return 0;
	}

	const vector<OccurrencePtr> finalExpressionOccurrences =
		m_occurrenceContainer->getAllOccurrencesIn(
			const_cast<ExpressionBase*>(&forStatement.getFinalExpression()));

	if (!isExpressionIn(expectedCounter, finalExpressionOccurrences))
	{
		return 0;
	}

	const vector<OccurrencePtr> stepExpressionOccurrences =
		m_occurrenceContainer->getAllOccurrencesIn(
			const_cast<ExpressionBase*>(&forStatement.getStepExpression()));

	if (!isExpressionIn(expectedCounter, stepExpressionOccurrences))
	{
		return 0;
	}

	return const_cast<ExpressionBase*>(&expectedCounter);
}

}
}
