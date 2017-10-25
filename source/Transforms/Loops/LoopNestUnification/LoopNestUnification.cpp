#include "Transforms/Loops/LoopNestUnification/LoopNestUnification.h"
#include "Transforms/Loops/LoopFragmentation/LoopFragmentation.h"
#include "Reprise/Reprise.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include "Shared/LabelsShared.h"
#include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Transforms::Loops;
using namespace OPS::Reprise::Editing;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Shared;

namespace OPS
{
namespace Transforms
{
namespace Loops
{

// From

//  for(i = i0; i < n; ++i)
//      for(j = j0; j < m; ++j)
//      {
//          Block;
//      }

//  to

//  for(s = 0; s < (n - i0) * (m - j0); ++s)
//  {
//      i = i0 + s / (m - j0);
//      j = j0 + s % (m - j0);
//      {
//          Block;
//      }
//  }


ForStatement* getParentLoop(ForStatement* loopPtr);
ForStatement*  getChildLoop(ForStatement* loopPtr);

ForStatement& makeLoopNestUnification(ForStatement &forStatement);



ForStatement& loopNestUnification(ForStatement &forStatement, int maxDepth)
{
    int depth = getNestDepth(&forStatement, false);

    if (depth < 2) return forStatement;
    if (depth > maxDepth) depth = maxDepth;

    ForStatement* forPtr = &forStatement;

    depth -= 1;

    for(int d = 0; d < depth; d++)
        forPtr = getChildLoop(forPtr);

    while (depth > 0)
    {
        forPtr = getParentLoop(forPtr);
        forPtr = &makeLoopNestUnification(*forPtr);
        depth--;
    }

    return *forPtr;
}


ForStatement& makeLoopNestUnification(ForStatement &forStatement)
{
    //old loops
    ReprisePtr<ForStatement> for1(&forStatement);
    ReprisePtr<ForStatement> for2(forStatement.getBody().getChild(0).cast_ptr<ForStatement>());

    BlockStatement &parentBlock = for1->getParentBlock();
    ReprisePtr<ReferenceExpression> i(&getBasicForCounter(*for1));
    ReprisePtr<ReferenceExpression> j(&getBasicForCounter(*for2));
    IntegerHelper intHelp(i->getResultType()->cast_to<BasicType>());
    Declarations &oldDecls = for1->getBody().getDeclarations();

    //new variable s
    ReprisePtr<ReferenceExpression> s(new ReferenceExpression(
                                                   createNewVariable(*i->getResultType(), parentBlock, "")));

    //(n - i0) and (m - j0)
    ReprisePtr<ExpressionBase> i0(&op( getBasicForInitExpression(*for1) ));
    ReprisePtr<ExpressionBase> j0(&op( getBasicForInitExpression(*for2) ));
    ReprisePtr<ExpressionBase> def_i(&( op(getBasicForFinalExpression(*for1)) - op(i0) ));
    ReprisePtr<ExpressionBase> def_j(&( op(getBasicForFinalExpression(*for2)) - op(j0) ));

    //new ForStatement
    ReprisePtr<ExpressionBase> s_init(&( op(s) R_AS intHelp(0)) );
    ReprisePtr<ExpressionBase> s_fin(&(  op(s) < op(def_i) * op(def_j) ));
    ReprisePtr<ExpressionBase> s_step(&( op(s) R_AS op(s) + intHelp(1) ));

    ReprisePtr<StatementBase> newForPtr(new ForStatement(s_init.get(), s_fin.get(), s_step.get()));
    replaceStatement(forStatement.cast_to<StatementBase>(), newForPtr);
    ForStatement &newFor = newForPtr->cast_to<ForStatement>();

    //set i and j counters
    ReprisePtr<ExpressionStatement> set_i(new ExpressionStatement(&( op(i) R_AS op(i0) + op(s) / op(def_j) )));
    ReprisePtr<ExpressionStatement> set_j(new ExpressionStatement(&( op(j) R_AS op(j0) + op(s) % op(def_j) )));
    newFor.getBody().addLast(set_i.get());
    newFor.getBody().addLast(set_j.get());
    updateLabel(for2, newFor.getBody().getChild(1).cast_to<StatementBase>());

    //move inner block
    newFor.getBody().addLast(for2->getBody().cast_ptr<StatementBase>());

    //move variables declarations
    Declarations::VarIterator iter = oldDecls.getFirstVar();

    while (iter.isValid())
    {
        ReprisePtr<VariableDeclaration> var(&(*iter));
        iter++;
        if (&var->getDefinedBlock() == &for1->getBody())
            var->setDefinedBlock(newFor.getBody());
    }

    return newFor;
}

}   //Loops
}   //Transforms
}   //OPS



