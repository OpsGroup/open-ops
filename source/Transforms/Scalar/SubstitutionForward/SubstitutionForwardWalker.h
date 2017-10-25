#ifndef _SUBSTITUTION_FORWARD_WALKER_H_INCLUDED_
#define _SUBSTITUTION_FORWARD_WALKER_H_INCLUDED_

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"

#include <list>

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using OPS::Reprise::Service::DeepWalker;

using OPS::Reprise::ExpressionBase;

using OPS::Reprise::StrictLiteralExpression;
using OPS::Reprise::CompoundLiteralExpression;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::SubroutineReferenceExpression;
using OPS::Reprise::StructAccessExpression;
using OPS::Reprise::EnumAccessExpression;
using OPS::Reprise::TypeCastExpression;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::SubroutineCallExpression;
using OPS::Reprise::EmptyExpression;


class SubstitutionForwardWalker: public DeepWalker
{
public:
	SubstitutionForwardWalker(ExpressionBase& expressionInstead, bool changeLeftPartOfAssign);

	void visit(StrictLiteralExpression& repriseObject);
	void visit(CompoundLiteralExpression& repriseObject);
	void visit(ReferenceExpression& repriseObject);
	void visit(SubroutineReferenceExpression& repriseObject);
	void visit(StructAccessExpression& repriseObject);
	void visit(EnumAccessExpression& repriseObject);
	void visit(TypeCastExpression& repriseObject);
	void visit(BasicCallExpression& repriseObject);
	void visit(SubroutineCallExpression& repriseObject);
	void visit(EmptyExpression& repriseObject);

	std::list<ExpressionBase*> getExpressionsForReplacement();

private:
	template<class ExpressionType>
	void addToListForReplacementIfSuitable(ExpressionType& to)
	{
		if (to.isEqual(m_expressionInstead))
		{
			m_expressionsForReplacement.push_back(&to);
		}
	}

private:
	ExpressionBase& m_expressionInstead;
	bool m_changeLeftPartOfAssign;
	std::list<ExpressionBase*> m_expressionsForReplacement;
};

}	// Scalar
}	// Transforms
}	// OPS

#endif	// _SUBSTITUTION_FORWARD_WALKER_H_INCLUDED_
