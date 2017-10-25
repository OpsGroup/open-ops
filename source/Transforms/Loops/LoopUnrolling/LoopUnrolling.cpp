#include "Transforms/Loops/LoopUnrolling/LoopUnrolling.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/LabelsShared.h"
#include "Shared/RepriseClone.h"

#include <string>

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::VariableDeclaration;

using OPS::Reprise::BlockStatement;
using OPS::Reprise::ExpressionStatement;

using OPS::Reprise::ExpressionBase;

using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::StrictLiteralExpression;


bool canApplyLoopUnrollingTo(ForStatement& forStatement, qword)
{
	using namespace OPS::Reprise::Editing;

	return forHeaderIsCanonized(forStatement);
}

void makeLoopUnrolling(ForStatement& forStatement, qword h)
{
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Shared::ExpressionHelpers;
	using namespace OPS::Transforms::Scalar;
	using namespace OPS::Shared;

	OPS_ASSERT(canApplyLoopUnrollingTo(forStatement, h));

	OPS::Reprise::Declarations& decls = forStatement.getRootBlock().getDeclarations();
	ReprisePtr<BlockStatement> body(&forStatement.getBody());
	ReprisePtr<ReferenceExpression> i(getBasicForCounter(forStatement).clone());
	const std::string iName = i->getReference().getName();
	ReprisePtr<ExpressionBase> n(getBasicForFinalExpression(forStatement).clone());

// 	> {
// 	> }
	BlockStatement* resultBlock = new BlockStatement();
	updateLabel(replaceStatement(forStatement, ReprisePtr<StatementBase>(resultBlock)), *resultBlock);

// 	{
// 		> int i1;
// 		> int i2;
// 	}
	ReprisePtr<ReferenceExpression> i1(new ReferenceExpression(createNewVariable(*i->getResultType(),
		*resultBlock, iName)));
	ReprisePtr<ReferenceExpression> i2(new ReferenceExpression(createNewVariable(*i->getResultType(),
		*resultBlock, iName)));

    IntegerHelper c(i->getResultType().get()->cast_to<BasicType>());

// 	{
// 		...
// 		> for (i1 = ...)
// 		> {
// 		> }
// 	}
	ForStatement* forStatement1 = new ForStatement(&(op(i1) R_AS c(0)), &(op(i1) < op(n)),
		&(op(i1) R_AS op(i1) + c(h)));
	resultBlock->addLast(forStatement1);
	BlockStatement* body1 = new BlockStatement();
	forStatement1->setBody(body1);

	for (qword l = 0; l < h; ++l)
	{
	// 	{
	// 		...
	// 		for (i1 = ...)
	// 		{
	// 			...
	// 			> {
	// 			>	  ...
	// 			> }
	// 			...
	// 		}
	// 	}
		ReprisePtr<BlockStatement> block = OPS::Shared::cloneStatement(*body, decls, decls);
		makeSubstitutionForward(*block, *i, ReprisePtr<ExpressionBase>(&(op(i) + c(l))), true);
		generateNewLabels(*block);
		body1->addLast(block.get());
	}

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			...
// 		}
// 		> for (i2 = ...)
// 		> {
// 		>	 ...
// 		> }
//	}
	ForStatement* forStatement2 = new ForStatement(&(op(i2) R_AS c(0)), &(op(i2) < op(n) - (op(n) / c(h)) * c(h)),
		&(op(i2) R_AS op(i2) + c(1)));
	resultBlock->addLast(forStatement2);
	ReprisePtr<BlockStatement> body2 = OPS::Shared::cloneStatement(*body, decls, decls);
	makeSubstitutionForward(*body2, *i, ReprisePtr<ExpressionBase>(&((op(n) / c(h)) * c(h) + op(i2))), true);
	generateNewLabels(*body2);
	forStatement2->setBody(body2.get());

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			...
// 		}
// 		for (i2 = ...)
// 		{
// 			...
//		}
//		> i = ...;
//	}
	ExpressionStatement* iInnitialization = new ExpressionStatement(&(op(i) R_AS op(n)));
	resultBlock->addLast(iInnitialization);
}

}	// Loops
}	// Transforms
}	// OPS
