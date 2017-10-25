#include "Transforms/Loops/LoopNesting/LoopNesting.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/LabelsShared.h"
#include "Shared/RepriseClone.h"
#include "FrontTransforms/ExpressionSimplifier.h"
#include "Transforms/Loops/Canonizing/ConvertLessEqualToLess.h"

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
using namespace OPS::Shared::ExpressionHelpers;


bool canApplyLoopNestingTo(ForStatement& forStatement)
{
    if (!Reprise::Editing::forHeaderIsCanonized(forStatement))
        return canConvertLessEqualToLess(forStatement);
    else return true;
}

BlockStatement& makeLoopNesting(ForStatement& forStatement, const int h, bool flag, bool native, bool tryOptimizeBorders, bool generateTail)
{
    if (!Reprise::Editing::forHeaderIsCanonized(forStatement))
        convertLessEqualToLess(forStatement);
    Reprise::ReferenceExpression& i = Reprise::Editing::getBasicForCounter(forStatement);
    IntegerHelper c(i.getResultType()->cast_to<BasicType>());
    Reprise::ReprisePtr<Reprise::ExpressionBase> hExpr(&c(h));
    return makeLoopNesting(forStatement, *hExpr, flag, native, tryOptimizeBorders, generateTail);
}

BlockStatement& makeLoopNesting(ForStatement& forStatement, const ExpressionBase& h, bool flag, bool native, bool tryOptimizeBorders, bool generateTail)
{
	ForStatement *f1, *f2, *f3;
    if(native)
    {
        return makeLoopNestingWithNativeCounter(forStatement, h, f1, f2, f3, flag, tryOptimizeBorders, generateTail);
    }
    else
    {
        return makeLoopNesting(forStatement, h, f1, f2, f3, flag, tryOptimizeBorders, generateTail);
    }
}

BlockStatement& makeLoopNesting(ForStatement& forStatement, const ExpressionBase& h,
                ForStatement*& resForStatement1, ForStatement*& resForStatement2, ForStatement*& resForStatement3,
                                bool outerBound, bool tryOptimizeBorders, bool generateTail)
{
	using namespace OPS::Reprise::Editing;
	using namespace OPS::Transforms::Scalar;
	using namespace OPS::Shared;
    if (!canApplyLoopNestingTo(forStatement))
        throw OPS::RuntimeError("Loop Nesting is not applicable!");

    if (!Reprise::Editing::forHeaderIsCanonized(forStatement))
        convertLessEqualToLess(forStatement);

	OPS::Reprise::Declarations& declarations = forStatement.getRootBlock().getDeclarations();
	ReprisePtr<BlockStatement> body(&forStatement.getBody());
	ReprisePtr<ReferenceExpression> i(getBasicForCounter(forStatement).clone());
	const std::string iName = i->getReference().getName();
	ReprisePtr<ExpressionBase> n(getBasicForFinalExpression(forStatement).clone());

    // 	> {
    // 	> }
    BlockStatement* resultBlock = new BlockStatement();
    updateLabel(replaceStatement(forStatement, ReprisePtr<StatementBase>(resultBlock)), *resultBlock);
    ReprisePtr<ReferenceExpression> i1(new ReferenceExpression(
        createNewVariable(*i->getResultType(), *resultBlock, iName)));
    ReprisePtr<ReferenceExpression> i2(new ReferenceExpression(createNewVariable(*i->getResultType(),
        *resultBlock, iName)));
    ReprisePtr<ReferenceExpression> i3(new ReferenceExpression(createNewVariable(*i->getResultType(),
        *resultBlock, iName)));

    ReprisePtr<ExpressionBase> f1lim, f2lim, f3lim, subst, subst3;

    IntegerHelper c(i->getResultType()->cast_to<BasicType>());

    subst3 = ReprisePtr<ExpressionBase>(&( op(h)*(op(n)/op(h)) + op(i3) ));
    if (outerBound == true)
    {
        //h - это граница внешнего цикла
        if (generateTail)
        {
            f1lim =  ReprisePtr<ExpressionBase>(&op(h));
            f2lim = ReprisePtr<ExpressionBase>(&( op(n) / op(h) ));
            f3lim = ReprisePtr<ExpressionBase>(&( op(n) % op(h) ));
            subst = ReprisePtr<ExpressionBase>(&( op(i1)*(op(n)/op(h)) + op(i2) ));
        }
        else
        {
            f1lim =  ReprisePtr<ExpressionBase>(&op(h));
            f2lim = ReprisePtr<ExpressionBase>(&( (op(i1)+c(1))*op(n)/op(h) - op(i1)*op(n)/op(h) ));
            subst = ReprisePtr<ExpressionBase>(&( (op(i1)*op(n))/op(h) + op(i2) ));
        }
    }
    else
    {
        //h - граница внутреннего цикла
        if (generateTail)
        {
            f1lim = ReprisePtr<ExpressionBase>(&(op(n) / op(h)));;
            f2lim = ReprisePtr<ExpressionBase>(&op(h));
            f3lim = ReprisePtr<ExpressionBase>(&( op(n) % op(h) ));
        }
        else
        {
            f1lim = ReprisePtr<ExpressionBase>(&( (op(n)+op(h)-   c(1)) / op(h) ));;
            f2lim = ReprisePtr<ExpressionBase>(&( Min( op(h), op(n)-op(i1)*op(h) ) ));
        }
        subst = ReprisePtr<ExpressionBase>(&( op(i1)*op(h) + op(i2) ));
    }

	//if (tryOptimizeBorders == true)
	//{
	//	f1n.reset(simplifier.simplify(f1n.get()));
	//	f2n.reset(simplifier.simplify(f2n.get()));
	//}


// 	{
// 		> int i1;
// 		> for (i1 = ...)
// 		> {
// 		> }
// 	}
    ForStatement* forStatement1 = new ForStatement(&(op(i1) R_AS c(0)),
                                                   &(op(i1) < op(f1lim) ),
                                                   &(op(i1) R_AS op(i1) + c(1)));
	resultBlock->addLast(forStatement1);
	resForStatement1 = forStatement1;

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 		    > int i2;
// 			> for (i2 = ...)
// 			> {
// 			>	 ...
// 			> }
// 		}
// 	}
    ForStatement* forStatement2 = new ForStatement(&(op(i2) R_AS c(0)),
                                                   &(op(i2) < op(f2lim)),
                                                   &(op(i2) R_AS op(i2) + c(1)));
    forStatement1->getBody().addLast(forStatement2);
	ReprisePtr<BlockStatement> body2 = OPS::Shared::cloneStatement(*body, declarations, declarations);
    makeSubstitutionForward(*body2, *i, subst, true);
	generateNewLabels(*body2);
	forStatement2->setBody(body2.get());
	resForStatement2 = forStatement2;

// 	{
// 		...
// 		for (i1 = ...)
// 		{
// 			...
// 		}
// 		> int i3;
// 		> for (i3 = ...)
// 		> {
// 		>	 ...
// 		> }
//	}
    if (generateTail )
    {
        ForStatement* forStatement3 = new ForStatement(&(op(i3) R_AS c(0)), &(op(i3) < op(f3lim)),
            &(op(i3) R_AS op(i3) + c(1)));
        resultBlock->addLast(forStatement3);
        ReprisePtr<BlockStatement> body3 = OPS::Shared::cloneStatement(*body, declarations, declarations);
        makeSubstitutionForward(*body3, *i, subst3, true);
        generateNewLabels(*body3);
        forStatement3->setBody(body3.get());
        resForStatement3 = forStatement3;
    }

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


BlockStatement& makeLoopNestingWithNativeCounter(ForStatement& forStatement, const ExpressionBase& h,
                ForStatement*& outerForStatement, ForStatement*& innerForStatement, ForStatement*& restForStatement, bool outerBound, bool tryOptimizeBorders, bool generateTail)
{
    using namespace OPS::Reprise::Editing;
    using namespace OPS::Shared::ExpressionHelpers;
    using namespace OPS::Transforms::Scalar;
    using namespace OPS::Shared;

    if (!canApplyLoopNestingTo(forStatement))
        throw OPS::RuntimeError("Loop Nesting is not applicable!");

    if (!Reprise::Editing::forHeaderIsCanonized(forStatement))
        convertLessEqualToLess(forStatement);

    OPS::Reprise::Declarations& declarations = forStatement.getRootBlock().getDeclarations();
    ReprisePtr<BlockStatement> body(&forStatement.getBody());
    ReprisePtr<ReferenceExpression> i(getBasicForCounter(forStatement).clone());
    const std::string iName = i->getReference().getName();
    ReprisePtr<ExpressionBase> n(getBasicForFinalExpression(forStatement).clone());


    // 	> {
    // 	> }
   BlockStatement* resultBlock = new BlockStatement();

    // 	{
    // 		> int i1;
    // 		> int i2;
    // 		> int i3;
    // 	}
   updateLabel(replaceStatement(forStatement, ReprisePtr<StatementBase>(resultBlock)), *resultBlock);
   ReprisePtr<ReferenceExpression> new_counter(new ReferenceExpression(createNewVariable(*i->getResultType(),
            *resultBlock, iName)));


    ReprisePtr<ExpressionBase> f1cnt, f2cnt, f3cnt;



    f1cnt = new_counter;
    f2cnt = i;
    f3cnt = i;

    ReprisePtr<ExpressionBase> f1init, f2init, f3init;
    ReprisePtr<ExpressionBase> f1lim, f2lim, f3lim;

    IntegerHelper c(i->getResultType()->cast_to<BasicType>());

    if (outerBound == true)
    {
        //h - это граница внешнего цикла
        f1init = ReprisePtr<ExpressionBase>(&c(0));
        f2init = ReprisePtr<ExpressionBase>(&(op(f1cnt) * op(n) / op(h)));
        if (generateTail)
        {
            f1lim =  ReprisePtr<ExpressionBase>(&op(h));
            f2lim = ReprisePtr<ExpressionBase>(&( (op(f1cnt) + c(1)) * op(n) / op(h)));
        }
        else
        {
            f1lim =  ReprisePtr<ExpressionBase>(&(op(h) + c(1)));
            f2lim = ReprisePtr<ExpressionBase>(&( Min((op(f1cnt) + c(1)) * op(n) / op(h), op(n)) ));
        }
    }
    else
    {
        //h - граница внутреннего цикла
        f1init = ReprisePtr<ExpressionBase>(&c(0));
        f2init = ReprisePtr<ExpressionBase>(&(op(f1cnt) *  op(h)));
        if (generateTail)
        {
            f1lim = ReprisePtr<ExpressionBase>(&(op(n) / op(h)));;
            f2lim = ReprisePtr<ExpressionBase>(&( (op(f1cnt) + c(1)) * op(h)));

            f3init = ReprisePtr<ExpressionBase>(&(op(h) * ( op(n) / op(h)) ));
            f3lim = ReprisePtr<ExpressionBase>(&(op(n)));
        }
        else
        {
            f1lim = ReprisePtr<ExpressionBase>(&((op(n)+op(h)-c(1)) / op(h)));;
            f2lim = ReprisePtr<ExpressionBase>(&( Min((op(f1cnt) + c(1)) * op(h), op(n)) ));
        }
    }

    //if (tryOptimizeBorders == true)
    //{
    //	f1n.reset(simplifier.simplify(f1n.get()));
    //	f2n.reset(simplifier.simplify(f2n.get()));
    //}






// 	{
// 		...
// 		> for (i1 = ...)
// 		> {
// 		> }
// 	}
    ForStatement* forStatement1 = new ForStatement(&(op(f1cnt) R_AS op(f1init)), &(op(f1cnt) < op(f1lim)),
        &(op(f1cnt) R_AS op(f1cnt) + c(1)));
    resultBlock->addLast(forStatement1);
    BlockStatement* body1 = new BlockStatement();
    forStatement1->setBody(body1);
    outerForStatement = forStatement1;

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
    ForStatement* forStatement2 = new ForStatement(&(op(f2cnt) R_AS op(f2init)), &(op(f2cnt) < op(f2lim)),
        &(op(f2cnt) R_AS op(f2cnt) + c(1)));
    body1->addLast(forStatement2);
    ReprisePtr<BlockStatement> body2 = OPS::Shared::cloneStatement(*body, declarations, declarations);
    generateNewLabels(*body2);
    forStatement2->setBody(body2.get());
    innerForStatement = forStatement2;

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
    if (generateTail && outerBound == false)
    {
        ForStatement* forStatement3 = new ForStatement(&(op(f3cnt) R_AS op(f3init)), &(op(f3cnt) < op(f3lim)),
            &(op(f3cnt) R_AS op(f3cnt) + c(1)));
        resultBlock->addLast(forStatement3);
        ReprisePtr<BlockStatement> body3 = OPS::Shared::cloneStatement(*body, declarations, declarations);
        generateNewLabels(*body3);
        forStatement3->setBody(body3.get());
        restForStatement = forStatement3;
    }

    return *resultBlock;
}



}	// Loops
}	// Transforms
}	// OPS
