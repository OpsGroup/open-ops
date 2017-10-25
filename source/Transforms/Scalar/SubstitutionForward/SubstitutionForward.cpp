#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "SubstitutionForwardWalker.h"


#include <list>

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using namespace OPS::Reprise::Editing;


template<class ExpressionType>
static void makeSubstitutionForwardImplementation(ExpressionType& to, ExpressionBase& expressionInstead, ReprisePtr<ExpressionBase> expressionWhat,
	bool changeLeftPartOfAssign)
{
	SubstitutionForwardWalker substitutionForwardWalker(expressionInstead, changeLeftPartOfAssign);
	to.accept(substitutionForwardWalker);
	std::list<ExpressionBase*> expressionsForReplacement(substitutionForwardWalker.getExpressionsForReplacement());
	for (std::list<ExpressionBase*>::iterator i = expressionsForReplacement.begin(); i != expressionsForReplacement.end(); ++i)
	{
		replaceExpression(**i, ReprisePtr<ExpressionBase>(expressionWhat->clone()));
	}
	expressionWhat.release();
}

void makeSubstitutionForward(StatementBase& statementTo, ExpressionBase& expressionInstead, ReprisePtr<ExpressionBase> expressionWhat,
	bool changeLeftPartOfAssign /*= false*/)
{
	makeSubstitutionForwardImplementation<StatementBase>(statementTo, expressionInstead, expressionWhat, changeLeftPartOfAssign);
}

void makeSubstitutionForward(ExpressionBase& expressionTo, ExpressionBase& expressionInstead, ReprisePtr<ExpressionBase> expressionWhat,
	bool changeLeftPartOfAssign /*= false*/)
{
	makeSubstitutionForwardImplementation<ExpressionBase>(expressionTo, expressionInstead, expressionWhat, changeLeftPartOfAssign);
}

} // Scalar
} // Transforms
} // OPS
