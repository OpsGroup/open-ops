#ifndef STATEMENTS_SHARED_DEEP_WALKERS_H
#define STATEMENTS_SHARED_DEEP_WALKERS_H

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ServiceFunctions.h"

namespace OPS
{
namespace Shared
{
	class CollectChildStatemetsDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		typedef std::set<OPS::Reprise::StatementBase*> StatementsContainer;

		CollectChildStatemetsDeepWalker(): OPS::Reprise::Service::DeepWalker(),
			m_statements()
		{
		}

		void visit(OPS::Reprise::ForStatement& forStatement)
		{
			m_statements.insert(&forStatement);

			OPS::Reprise::Service::DeepWalker::visit(forStatement);
		}

		void visit(OPS::Reprise::IfStatement& stmt)
		{
			m_statements.insert(&stmt);

			OPS::Reprise::Service::DeepWalker::visit(stmt);
		}

		void visit(OPS::Reprise::WhileStatement& stmt)
		{
			m_statements.insert(&stmt);

			OPS::Reprise::Service::DeepWalker::visit(stmt);
		}

		void visit(OPS::Reprise::ExpressionStatement& stmt)
		{
			m_statements.insert(&stmt);
		}

		void visit(OPS::Reprise::GotoStatement& gotoStatement)
		{
			m_statements.insert(&gotoStatement);
		}

        void visit(OPS::Reprise::Canto::HirBreakStatement& breakStatement)
		{
			m_statements.insert(&breakStatement);
		}

		void visit(OPS::Reprise::ReturnStatement& returnStatement)
		{
			m_statements.insert(&returnStatement);
		}

        void visit(OPS::Reprise::Canto::HirContinueStatement& continueStatement)
		{
			m_statements.insert(&continueStatement);
		}

		StatementsContainer getChildStatements()
		{
			return m_statements;
		}
	private:
		StatementsContainer m_statements;
	};

}
}

#endif			// STATEMENTS_SHARED_DEEP_WALKERS_H
