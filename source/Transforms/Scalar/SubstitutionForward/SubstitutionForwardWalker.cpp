#include "SubstitutionForwardWalker.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

SubstitutionForwardWalker::SubstitutionForwardWalker(ExpressionBase& expressionInstead, bool changeLeftPartOfAssign)
: m_expressionInstead(expressionInstead)
, m_changeLeftPartOfAssign(changeLeftPartOfAssign)
{
}

void SubstitutionForwardWalker::visit(StrictLiteralExpression& repriseObject)
{
	addToListForReplacementIfSuitable<StrictLiteralExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(CompoundLiteralExpression& repriseObject)
{
	addToListForReplacementIfSuitable<CompoundLiteralExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(ReferenceExpression& repriseObject)
{
	addToListForReplacementIfSuitable<ReferenceExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(SubroutineReferenceExpression& repriseObject)
{
	addToListForReplacementIfSuitable<SubroutineReferenceExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(StructAccessExpression& repriseObject)
{
	addToListForReplacementIfSuitable<StructAccessExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(EnumAccessExpression& repriseObject)
{
	addToListForReplacementIfSuitable<EnumAccessExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(TypeCastExpression& repriseObject)
{
	addToListForReplacementIfSuitable<TypeCastExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(BasicCallExpression& repriseObject)
{
	if (!m_changeLeftPartOfAssign && repriseObject.getKind() == BasicCallExpression::BCK_ASSIGN)
	{
		OPS_ASSERT(repriseObject.getArgumentCount() == 2);
		repriseObject.getArgument(1).accept(*this);
	}
	else
	{
		addToListForReplacementIfSuitable<BasicCallExpression>(repriseObject);
		DeepWalker::visit(repriseObject);
	}
}

void SubstitutionForwardWalker::visit(SubroutineCallExpression& repriseObject)
{
	addToListForReplacementIfSuitable<SubroutineCallExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

void SubstitutionForwardWalker::visit(EmptyExpression& repriseObject)
{
	addToListForReplacementIfSuitable<EmptyExpression>(repriseObject);
	DeepWalker::visit(repriseObject);
}

std::list<ExpressionBase*> SubstitutionForwardWalker::getExpressionsForReplacement()
{
	return m_expressionsForReplacement;
}

}	// Scalar
}	// Transforms
}	// OPS
