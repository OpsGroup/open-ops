#ifndef IF_DISTRIBUTION_DEEP_WALKER
#define IF_DISTRIBUTION_DEEP_WALKER

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Reprise.h"

#include <list>

namespace OPS
{
namespace Transforms
{
	/*
		Deep walker that looks trough first part of pIfStatement(see constructor) till pBorderStatement(see constructor) exclusively.
		Also visit conditional expression of pIfStatement.

		Checks for side effect that could be possible when we use IfDistribution transformation without introducing a new variable.

		Checks for:
		1) Labeled statements(excluding pIfStatement)
		2) Function call
	*/

	class SideEffectCheckDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		SideEffectCheckDeepWalker(OPS::Reprise::IfStatement *pIfStatement, OPS::Reprise::StatementBase* pBorderStatement): OPS::Reprise::Service::DeepWalker() ,
			m_sideEffectFlag(false),
			m_pIfStatement(pIfStatement),
			m_pBorderStatement(pBorderStatement)
		{
		}

		bool getSideEffectFlag()
		{
			return m_sideEffectFlag;
		}

		void visit(OPS::Reprise::BlockStatement& blockStatement)
		{
			if(blockStatement.getParent() == m_pIfStatement)
			{
				OPS::Reprise::BlockStatement::Iterator borderIterator = blockStatement.convertToIterator(m_pBorderStatement);
				for(OPS::Reprise::BlockStatement::Iterator it = blockStatement.getFirst(); it.isValid() && it != borderIterator; ++it)
				{
					it->accept(*this);
				}
			}
			else
			{
				OPS::Reprise::Service::DeepWalker::visit(blockStatement);
			}
			
			m_sideEffectFlag = m_sideEffectFlag || blockStatement.hasLabel(); 
		}

		void visit(OPS::Reprise::ForStatement& forStatement)
		{
			OPS::Reprise::Service::DeepWalker::visit(forStatement);
			
			m_sideEffectFlag = m_sideEffectFlag || forStatement.hasLabel(); 
		}

		void visit(OPS::Reprise::WhileStatement& whileStatement)
		{
			OPS::Reprise::Service::DeepWalker::visit(whileStatement);
			
			m_sideEffectFlag = m_sideEffectFlag || whileStatement.hasLabel();
		}

		void visit(OPS::Reprise::IfStatement& ifStatement)
		{
			OPS::Reprise::Service::DeepWalker::visit(ifStatement);
			
			if(m_pIfStatement != &ifStatement)
			{
				m_sideEffectFlag = m_sideEffectFlag || ifStatement.hasLabel(); 
			}
		}

		void visit(OPS::Reprise::ExpressionStatement& expressionStatement)
		{
			OPS::Reprise::Service::DeepWalker::visit(expressionStatement);
			
			m_sideEffectFlag = m_sideEffectFlag || expressionStatement.hasLabel(); 
		}
		
		void visit(OPS::Reprise::EmptyStatement& emptyStatement)
		{
			m_sideEffectFlag = m_sideEffectFlag || emptyStatement.hasLabel(); 
		}

		void visit(OPS::Reprise::SubroutineCallExpression& subroutineCallExpression)
		{
			// TODO: add check for side effect
			m_sideEffectFlag = true;
		}

	private:
		bool m_sideEffectFlag;
		OPS::Reprise::IfStatement*   m_pIfStatement;
		OPS::Reprise::StatementBase* m_pBorderStatement;
	};

	/*
		Deep walker that collect all statements in first part of then branch of conditional operator(pIfStatement) 
		till border statement(pBorderStatement).
		*/

	class FirstPartDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		typedef std::set<OPS::Reprise::StatementBase*> StatementsContainer;

	public:
		FirstPartDeepWalker(OPS::Reprise::IfStatement *pIfStatement, OPS::Reprise::StatementBase* pBorderStatement): OPS::Reprise::Service::DeepWalker(), 
			m_firstStatements(),
			m_pIfStatement(pIfStatement),
			m_pBorderStatement(pBorderStatement)
		{
		}

		StatementsContainer getFristStatements()
		{
			return m_firstStatements;
		}

		void visit(OPS::Reprise::BlockStatement& blockStatement)
		{
			if(blockStatement.getParent() == m_pIfStatement)
			{
				OPS::Reprise::BlockStatement::Iterator borderIterator = blockStatement.convertToIterator(m_pBorderStatement);
				for(OPS::Reprise::BlockStatement::Iterator it = blockStatement.getFirst(); it.isValid() && it != borderIterator; ++it)
				{
					it->accept(*this);
				}
			}
			else
			{
				OPS::Reprise::Service::DeepWalker::visit(blockStatement);
			}
		}

		void visit(OPS::Reprise::ForStatement& forStatement)
		{
			m_firstStatements.insert(&forStatement);

			OPS::Reprise::Service::DeepWalker::visit(forStatement);
		}

		void visit(OPS::Reprise::WhileStatement& whileStatement)
		{
			m_firstStatements.insert(&whileStatement);

			OPS::Reprise::Service::DeepWalker::visit(whileStatement);
		}

		void visit(OPS::Reprise::IfStatement& ifStatement)
		{
			m_firstStatements.insert(&ifStatement);

			OPS::Reprise::Service::DeepWalker::visit(ifStatement);
		}

		void visit(OPS::Reprise::ExpressionStatement& expressionStatement)
		{
			m_firstStatements.insert(&expressionStatement);

			OPS::Reprise::Service::DeepWalker::visit(expressionStatement);
		}

	private:
		StatementsContainer               m_firstStatements;
		OPS::Reprise::IfStatement*   m_pIfStatement;
		OPS::Reprise::StatementBase* m_pBorderStatement;
	};
}
}

#endif // IF_DISTRIBUTION_DEEP_WALKER
