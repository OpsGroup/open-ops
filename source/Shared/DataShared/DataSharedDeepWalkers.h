#ifndef DATA_SHARED_DEEP_WALKERS_H
#define DATA_SHARED_DEEP_WALKERS_H

#include "Reprise/Service/DeepWalker.h"
#include "Shared/ExpressionOperators.h"

#include <set>

namespace OPS
{
namespace Shared
{
	class CollectVariablesDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		typedef std::set<OPS::Reprise::VariableDeclaration*> VariablesDeclarationsContainer;

	public:
		CollectVariablesDeepWalker(): m_variablesDecls()
		{
		}

		VariablesDeclarationsContainer getVariablesDecls()
		{
			return m_variablesDecls;
		}

		void visit(OPS::Reprise::ReferenceExpression& pReferenceExpr)
		{
			m_variablesDecls.insert(&(pReferenceExpr.getReference()));
		}

	private:
		VariablesDeclarationsContainer m_variablesDecls;
	};


	class CollectVarDeclarationsDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		typedef std::set<OPS::Reprise::VariableDeclaration*> VariablesDeclarationsContainer;

	public:
		CollectVarDeclarationsDeepWalker(VariablesDeclarationsContainer sourceDeclarations): m_sourceDeclarations(sourceDeclarations) {};
	
		void visit(OPS::Reprise::BlockStatement& blockStatement)
		{
			for(VariablesDeclarationsContainer::iterator it = m_sourceDeclarations.begin(); it != m_sourceDeclarations.end(); ++it)
			{
				if(&((*it)->getDefinedBlock()) == &blockStatement)
				{
					m_variablesDecls.insert(*it);
				}
			}

			OPS::Reprise::Service::DeepWalker::visit(blockStatement);
		}

		VariablesDeclarationsContainer getVariableDecls()
		{
			return m_variablesDecls;
		}
	private:
		VariablesDeclarationsContainer m_sourceDeclarations;
		VariablesDeclarationsContainer m_variablesDecls;
	};

/*
	class GeneratorsFinderDeepWalker: public OPS::Reprise::Service::DeepWalker
	{
	public:
		typedef std::list<OccurenceInfo> OccurenceInfoList;

	public:
		GeneratorsFinderDeepWalker(bool isCStyle): OPS::Reprise::Service::DeepWalker(),
			m_occurenceInfoList(), m_isCStyle(isCStyle)
		{
		}

		OccurenceInfoList getOccurenceInfoList()
		{
			return m_occurenceInfoList;
		}

		void visit(OPS::Reprise::ForStatement& forStatement)
		{
			forStatement.getBody().accept(*this);
		}

		void visit(OPS::Reprise::BasicCallExpression& callExpression)
		{
			if(callExpression.getKind() == OPS::Reprise::BasicCallExpression::BCK_ASSIGN)
			{
				OccurenceInfo occurenceInfo(callExpression.getArgument(0), m_isCStyle);
				m_occurenceInfoList.push_back(occurenceInfo);
			}

			OPS::Reprise::Service::DeepWalker::visit(callExpression);
		}

	private:
		OccurenceInfoList m_occurenceInfoList;
		bool m_isCStyle;
	};
*/
class CalculatorDeepWalker: public OPS::Reprise::Service::DeepWalker
{
public:
	CalculatorDeepWalker(): OPS::Reprise::Service::DeepWalker()	{}

	~CalculatorDeepWalker() { m_ResultStack.clear(); }

	Reprise::StrictLiteralExpression* getResult() { return (!m_ResultStack.empty())? m_ResultStack.front().get() : 0; }

	void visit(Reprise::ExpressionBase& Node)
	{
		if ((Node.is_a<Reprise::BasicCallExpression>()) || (Node.is_a<Reprise::StrictLiteralExpression>())
			 || (Node.is_a<Reprise::TypeCastExpression>()))
			m_ResultStack.clear();
	}
	
	void visit(Reprise::StrictLiteralExpression& Node)
	{
		m_ResultStack.push_back(Reprise::ReprisePtr<Reprise::StrictLiteralExpression>(Node.clone()));
	}

	void visit(Reprise::BasicCallExpression& Node);

	void visit(Reprise::TypeCastExpression& Node);

private:
	Reprise::StrictLiteralExpression* pop_last()
	{
		Reprise::StrictLiteralExpression* res = m_ResultStack.back().release();
		m_ResultStack.pop_back();
		return res;
	}

	void push_result(Reprise::StrictLiteralExpression* IntermediateResult)
	{
		m_ResultStack.push_back(Reprise::ReprisePtr<Reprise::StrictLiteralExpression>(IntermediateResult));
	}

	std::vector<Reprise::ReprisePtr<Reprise::StrictLiteralExpression> > m_ResultStack;
};

}
}

#endif // DATA_SHARED_DEEP_WALKERS_H

