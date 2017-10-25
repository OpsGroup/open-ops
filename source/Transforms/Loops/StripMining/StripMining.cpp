#include "Transforms/Loops/LoopNesting/LoopNesting.h"
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

using OPS::Reprise::ReferenceExpression;


bool canApplyStripMiningTo(ForStatement& forStatement, ReprisePtr<ExpressionBase>)
{
	using namespace OPS::Reprise::Editing;

	return forHeaderIsCanonized(forStatement);
}

BlockStatement& makeStripMining(ForStatement& forStatement, ReprisePtr<ExpressionBase> h)
{
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Shared::ExpressionHelpers;
	using namespace OPS::Transforms::Scalar;
	using namespace OPS::Shared;

	if (!canApplyStripMiningTo(forStatement, h))
		throw OPS::RuntimeError("Strip Mining is not applicable!");

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
// 		> int i3;
// 	}
	ReprisePtr<ReferenceExpression> i1(new ReferenceExpression(createNewVariable(*i->getResultType(),
		*resultBlock, iName)));
	ReprisePtr<ReferenceExpression> i2(new ReferenceExpression(createNewVariable(*i->getResultType(),
		*resultBlock, iName)));
	ReprisePtr<ReferenceExpression> i3(new ReferenceExpression(createNewVariable(*i->getResultType(),
		*resultBlock, iName)));

    IntegerHelper c(i->getResultType()->cast_to<BasicType>());

// 	{
// 		...
// 		> for (i1 = ...)
// 		> {
// 		> }
// 	}
	ForStatement* forStatement1 = new ForStatement(&(op(i1) R_AS c(0)), &(op(i1) < op(n) / op(h)),
		&(op(i1) R_AS op(i1) + c(1)));
	resultBlock->addLast(forStatement1);
	BlockStatement* body1 = new BlockStatement();
	forStatement1->setBody(body1);

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			> for (i2 = ...)
// 			> {
// 			>	 ...
// 			> }
// 		}
// 	}
	ForStatement* forStatement2 = new ForStatement(&(op(i2) R_AS c(0)), &(op(i2) < op(h)),
		&(op(i2) R_AS op(i2) + c(1)));
	body1->addLast(forStatement2);
	ReprisePtr<BlockStatement> body2 = OPS::Shared::cloneStatement(*body, decls, decls);
	makeSubstitutionForward(*body2, *i, ReprisePtr<ExpressionBase>(&(op(i1) * op(h) + op(i2))), true);
	generateNewLabels(*body2);
	forStatement2->setBody(body2.get());

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			...
// 		}
// 		> for (i3 = ...)
// 		> {
// 		>	 ...
// 		> }
//	}
	ForStatement* forStatement3 = new ForStatement(&(op(i3) R_AS c(0)), &(op(i3) < op(n) - (op(n) / op(h)) * op(h)),
		&(op(i3) R_AS op(i3) + c(1)));
	resultBlock->addLast(forStatement3);
	ReprisePtr<BlockStatement> body3 = OPS::Shared::cloneStatement(*body, decls, decls);
	makeSubstitutionForward(*body3, *i, ReprisePtr<ExpressionBase>(&((op(n) / op(h)) * op(h) + op(i3))), true);
	generateNewLabels(*body3);
	forStatement3->setBody(body3.get());

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			...
// 		}
// 		for (i3 = ...)
// 		{
// 			...
//		}
//		> i = ...;
//	}
	ExpressionStatement* iInnitialization = new ExpressionStatement(&(op(i) R_AS op(n)));
	resultBlock->addLast(iInnitialization);

    return *resultBlock;
}

}	// Loops
}	// Transforms
}	// OPS
