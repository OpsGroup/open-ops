#ifndef _DEPENDENCE_GRAPH_H_INCLUDED_
#define _DEPENDENCE_GRAPH_H_INCLUDED_

#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphVertex.h"
#include "Analysis/Montego/Occurrence.h"

#include <Reprise/Reprise.h>

#include <list>
#include <utility>

namespace OPS
{
	namespace Analysis
	{
		class InductionAnalysis;
	}

	namespace Shared
	{
		class ProgramContext;
	}

	namespace Montego
	{
		class DependenceGraph
		{
		public:

			typedef std::list<std::tr1::shared_ptr<DependenceGraphAbstractArc> > ArcList;

		public:

			explicit DependenceGraph(const Reprise::StatementBase& sourceStatement);

			DependenceGraph(std::tr1::shared_ptr<OccurrenceContainer> occurrenceContainer,
                std::tr1::shared_ptr<AliasInterface> aliasInterface,
				const Reprise::StatementBase& sourceStatement);

			DependenceGraph(Shared::ProgramContext* context,
				const Reprise::StatementBase& sourceStatement);

			~DependenceGraph();

		public:

			const Reprise::StatementBase& getSourceStatement() const;
			ArcList getAllArcs() const;

			// DependencyGraph arc selectors

			ArcList getAllArcsBetween(const Reprise::StatementBase& startStatement,
				const Reprise::StatementBase& endStatement) const;

			// DependencyGraph arc refiners

			void refineAllArcsWithLatticeGraph();
			void removeCounterArcs();
			//bool refineSelectedArcWithLatticeGraph(DependenceGraphArc arc);

			// Cache
			//TODO: need to extract from graph into external object

            std::tr1::shared_ptr<AliasInterface> getAliasInterface() const;
			//std::tr1::shared_ptr<OccurrenceContainer> getOccurrenceContainer() const;

		private:

			typedef std::list<std::tr1::shared_ptr<DependenceGraphTrueArc> >
				InternalArcList;

		private:

			DependenceGraph();

		private:

			void buildDependencyGraph();

			bool isArcNotInLatticeGraph(
					const std::tr1::shared_ptr<DependenceGraphTrueArc>& arc);

			Reprise::ExpressionBase* tryGetCounter(
				const Reprise::ForStatement& forStatement) const;
			std::list<std::pair<Reprise::ExpressionBase*, Reprise::ForStatement*> >
				findAllCounters() const;

			Analysis::InductionAnalysis& inductionAnalysis() const;

		private:

			static ArcList internalArcListToArcList(
					const InternalArcList& internalArcList);

			static Reprise::BlockStatement* tryGetDeclarationBlock(
				const OccurrencePtr& occurrence);

			static bool contains(const Reprise::BlockStatement& source,
				const Reprise::BlockStatement& target);

		private:

			const Reprise::StatementBase& m_sourceStatement;
			InternalArcList m_arcList;

			// Cache
			//TODO: need to extract from graph into external object
            std::tr1::shared_ptr<AliasInterface> m_aliasInterface;
			std::tr1::shared_ptr<OccurrenceContainer> m_occurrenceContainer;
			std::unique_ptr<Analysis::InductionAnalysis> m_inductionAnalysis;
		};
	}
}

#endif // _DEPENDENCE_GRAPH_H_INCLUDED_
