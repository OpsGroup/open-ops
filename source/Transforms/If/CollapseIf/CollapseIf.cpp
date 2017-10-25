#include "Reprise/Statements.h"
#include "Reprise/Expressions.h"
#include "Transforms/If/CollapseIf/CollapseIf.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Transforms
{

ExpressionStatement* collapseIfToAssign(IfStatement* ifStatement)
{
    BlockStatement& thenBody = ifStatement->getThenBody();
    BlockStatement& elseBody = ifStatement->getElseBody();
    if(thenBody.isEmpty() || elseBody.isEmpty())
        return 0;

    if((thenBody.getFirst() != thenBody.getLast()) || (elseBody.getFirst() != elseBody.getLast()))
        return 0;

    if(!(thenBody.getFirst()->is_a<ExpressionStatement>()))
        return 0;
    const ExpressionStatement* exprThen = thenBody.getFirst()->cast_ptr<ExpressionStatement>();

    if(!((*(elseBody.getFirst())).is_a<ExpressionStatement>()))
        return 0;

    const ExpressionStatement* exprElse = elseBody.getFirst()->cast_ptr<ExpressionStatement>();

    if(!((exprThen->get()).is_a<BasicCallExpression>()))
        return 0;

    const BasicCallExpression* basicCallExprThen = exprThen->get().cast_ptr<BasicCallExpression>();
    if(basicCallExprThen->getKind() != BasicCallExpression::BCK_ASSIGN)
        return 0;

    if(!((exprElse->get()).is_a<BasicCallExpression>()))
        return 0;

    const BasicCallExpression* basicCallExprElse = exprElse->get().cast_ptr<BasicCallExpression>();

    if(basicCallExprElse->getKind() != BasicCallExpression::BCK_ASSIGN)
        return 0;

    const ExpressionBase* leftArgumentThen = &(basicCallExprThen->getArgument(0));
    const ExpressionBase* leftArgumentElse = &(basicCallExprElse->getArgument(0));

    if(!(leftArgumentThen->isEqual(*(leftArgumentElse))))
        return 0;

    BasicCallExpression* tmpExpr1 = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT);
    tmpExpr1->addArgument((ifStatement->getCondition().clone()));
    BasicCallExpression* tmpExpr2 = new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY);
    tmpExpr2->addArgument(tmpExpr1);
    tmpExpr2->addArgument((basicCallExprElse->getArgument(1)).clone());
    tmpExpr1 = new BasicCallExpression(BasicCallExpression(BasicCallExpression::BCK_MULTIPLY));
    tmpExpr1->addArgument(ifStatement->getCondition().clone());
    tmpExpr1->addArgument(basicCallExprThen->getArgument(1).clone());
    BasicCallExpression* tmpExpr3 = new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS);
    tmpExpr3->addArgument(tmpExpr1);
    tmpExpr3->addArgument(tmpExpr2);
    tmpExpr1 = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN);
    tmpExpr1->addArgument(leftArgumentElse->clone());
    tmpExpr1->addArgument(tmpExpr3);
    ExpressionStatement* result = new ExpressionStatement(tmpExpr1);
    return result;


}

}
}

