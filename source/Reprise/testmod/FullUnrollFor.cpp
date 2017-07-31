#include "Tests.h"
#include "Reprise/Canto.h"

using namespace OPS;
using namespace OPS::Reprise;

void fullUnrollFor(ForStatement& forStatement)
{
	if (forStatement.getInitExpression().is_a<Canto::HirCCallExpression>())
	{
		Canto::HirCCallExpression& callExpression = forStatement.getInitExpression().cast_to<Canto::HirCCallExpression>();
		if (callExpression.getKind() == Canto::HirCCallExpression::HIRC_ASSIGN)
		{
//			ExpressionBase 
//			callExpression.getArgument(0)
		}
		else
			throw StateError("Unexpected init expression. Must be simple assign.");
	}
	else
		throw StateError("Unexpected init expression. Must be assign.");
}

