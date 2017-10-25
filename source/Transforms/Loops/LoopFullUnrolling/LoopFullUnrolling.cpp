#include "Transforms/Loops/LoopFullUnrolling/LoopFullUnrolling.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/LabelsShared.h"
#include "Shared/RepriseClone.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::BlockStatement;
using OPS::Reprise::ExpressionStatement;

using OPS::Reprise::ExpressionBase;

using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::StrictLiteralExpression;


bool canApplyLoopFullUnrollingTo(ForStatement& forStatement)
{
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Shared::ExpressionHelpers;

	if (!forHeaderIsCanonized(forStatement))
	{
		return false;
	}

	ExpressionBase& n = getBasicForFinalExpression(forStatement);
	if (!n.is_a<StrictLiteralExpression>() || !IntegerHelper::isIntegerType(n.cast_to<StrictLiteralExpression>()))
	{
		return false;
	}

	return true;
}

void makeLoopFullUnrolling(ForStatement& forStatement)
{
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Shared::ExpressionHelpers;
	using namespace OPS::Transforms::Scalar;
	using namespace OPS::Shared;

	OPS_ASSERT(canApplyLoopFullUnrollingTo(forStatement));

	OPS::Reprise::Declarations& decls = forStatement.getRootBlock().getDeclarations();
	ReprisePtr<BlockStatement> body(&forStatement.getBody());
	ReprisePtr<ReferenceExpression> i(getBasicForCounter(forStatement).clone());
	ReprisePtr<ExpressionBase> n(getBasicForFinalExpression(forStatement).clone());

// 	> {
// 	> }
	BlockStatement* resultBlock = new BlockStatement();
	updateLabel(replaceStatement(forStatement, ReprisePtr<StatementBase>(resultBlock)), *resultBlock);

    IntegerHelper c(i->getResultType()->cast_to<BasicType>());

	StrictLiteralExpression& nLiteral = n->cast_to<StrictLiteralExpression>();
	ExpressionStatement* iInnitialization = NULL;
	if (IntegerHelper::isGreaterThanOrEqualToZero(nLiteral))
	{
		qword nValue = IntegerHelper::getUnsignedValue(nLiteral);
		for (qword l = 0; l < nValue; ++l)
		{
		// 	{
		// 		...
		// 		> {
		// 		>	  ...
		// 		> }
		// 		...
		// 	}
			ReprisePtr<BlockStatement> block = OPS::Shared::cloneStatement(*body, decls, decls);
			makeSubstitutionForward(*block, *i, ReprisePtr<ExpressionBase>(&c(l)), true);
			generateNewLabels(*block);
			resultBlock->addLast(block.get());
		}

	// 	{
	// 		...
	// 		{
	// 			...
	// 		}
	// 		...
	//		i = ...
	// 	}
		iInnitialization = new ExpressionStatement(&(op(i) R_AS op(n)));
	}
	else
	{
	// 	{
	//		i = 0;
	// 	}
		iInnitialization = new ExpressionStatement(&(op(i) R_AS c(0)));
	}
	resultBlock->addLast(iInnitialization);
}

}	// Loops
}	// Transforms
}	// OPS
