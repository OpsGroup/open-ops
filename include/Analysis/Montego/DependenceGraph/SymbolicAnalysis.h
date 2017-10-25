#ifndef SYMBOLICANALYSIS_H
#define SYMBOLICANALYSIS_H

#include "DependenceGraph.h"
#include "Analysis/AbstractDepGraph.h"
#include "Shared/ProgramContext.h"

namespace OPS
{
namespace Montego
{
namespace SymbolicAnalysis
{

typedef Reprise::ReprisePtr<Reprise::ExpressionBase> PredicatePtr;

class PredicateBuilder
{
public:
	PredicateBuilder(const Analysis::AbstractDependence& arc);
	~PredicateBuilder();

	bool isApplicable() const;

	PredicatePtr buildPredicate(int support);

	std::vector<PredicatePtr> buildPredicates();

private:

	PredicatePtr buildPredicate(int support,
							   Analysis::AbstractOccurrence* start,
							   Analysis::AbstractOccurrence* end,
							   Reprise::ExpressionBase* condition);

	const Analysis::AbstractDependence* m_arc;
	Analysis::AbstractOccurrence* m_start;
	Analysis::AbstractOccurrence* m_end;
	Reprise::ExpressionBase* m_startCondition;
	Reprise::ExpressionBase* m_endCondition;
	int m_commonLoops;
};

class SymbolicPredicatesMeta : public OPS::Shared::IMetaInformation
{
public:
	SymbolicPredicatesMeta(){}
	SymbolicPredicatesMeta(OPS::Shared::ProgramContext&){}

	typedef std::pair<PredicatePtr, bool> PredicateValuePair;
	typedef std::list<PredicateValuePair> PredicateValueList;

	PredicateValueList& getPredicates();

	void refineDependence(Analysis::AbstractDependence& dep);

	const char* getUniqueId() const;
	static const char* const UNIQUE_ID;

private:
	PredicateValueList m_predicates;
};

}
}
}

#endif // SYMBOLICANALYSIS_H
