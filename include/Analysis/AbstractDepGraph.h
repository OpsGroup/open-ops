#ifndef ABSTRACTDEPGRAPH_H
#define ABSTRACTDEPGRAPH_H

#include <list>
#include "Reprise/Statements.h"
#include "OPS_Core/MemoryHelper.h"

namespace OPS
{
namespace Shared { class ProgramContext; }

namespace Analysis
{

typedef std::set<Reprise::ForStatement*> LoopSet;

class AbstractOccurrence
{
public:
	AbstractOccurrence(Reprise::ExpressionBase* expr, int bracketCount, const LoopSet& loops);

	Reprise::ExpressionBase* getExpr() const { return m_expr; }
	Reprise::StatementBase* getStatement() const;

	int getBracketCount() const { return m_bracketCount; }

	bool isIncludedByForBody(Reprise::ForStatement& forStmt);

private:
	Reprise::ExpressionBase* m_expr;
	int m_bracketCount;
	LoopSet m_loops;
};

typedef std::tr1::shared_ptr<AbstractOccurrence> AbstractOccurrencePtr;

class AbstractDependence
{
public:
	enum Type
	{
		Flow,
		Anti,
		Output,
		Input
	};

	AbstractDependence(Type type, AbstractOccurrencePtr begin, AbstractOccurrencePtr end, const LoopSet& carriers);

	Type getType() const { return m_type; }

	Reprise::ExpressionBase* getBeginExpr() const;
	Reprise::ExpressionBase* getEndExpr() const;

	Reprise::StatementBase* getBeginStatement() const;
	Reprise::StatementBase* getEndStatement() const;

	AbstractOccurrence* getBeginOccurrence() const;
	AbstractOccurrence* getEndOccurrence() const;

	bool isScalarVariable() const;
	bool isSelfStatementDependence() const { return getBeginStatement() == getEndStatement(); }

	bool testSupp(Reprise::ForStatement& forStmt);
	void eraseSupp(Reprise::ForStatement& forStmt);

private:
	Type m_type;
	AbstractOccurrencePtr m_begin, m_end;
	LoopSet m_carriers;
};

typedef std::tr1::shared_ptr<AbstractDependence> AbstractDependencePtr;
typedef std::list<AbstractDependencePtr> DependenceList;

class AbstractDepGraph
{
public:

	static AbstractDepGraph* buildLamportGraph(Reprise::BlockStatement& block, bool useLatticeGraphs);
	static AbstractDepGraph* buildMontegoGraph(Reprise::BlockStatement& block, bool useLatticeGraphs, OPS::Shared::ProgramContext* context = 0);

	template<typename Pred>
		AbstractDepGraph* getSubGraphByPred(Pred pred)
	{
		AbstractDepGraph* subGraph = new AbstractDepGraph;

		for(DependenceList::iterator it = begin(); it != end(); ++it)
		{
			if (pred(**it))
				subGraph->append(*it);
		}

		return subGraph;
	}

	AbstractDepGraph* getSubGraphByCarrier(Reprise::ForStatement& forStmt);
	AbstractDepGraph* getSubGraphByLoopBody(Reprise::ForStatement& forStmt);

	DependenceList::iterator begin();
	DependenceList::iterator end();

	int getDependenceCount() const;
	bool isEmpty() const;

	void append(AbstractDependencePtr dependence);
	void remove(AbstractDependencePtr dependence);
	void remove(const std::set<AbstractDependencePtr>& dependencies);

private:
	DependenceList m_dependencies;
};

}
}

#endif // ABSTRACTDEPGRAPH_H
