#include "Transforms/Loops/LoopInterchange/LoopInterchange.h"
#include "Analysis/LoopsInterchange/LoopsInterchangeAnalysis.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Transforms
{
namespace Loops
{

//на вход подается внешний цикл тесного гнезда
bool canApplyLoopInterchangeTo(ForStatement& forStatement)
{
    return OPS::Analysis::LoopsInterchange::isInterchangable(forStatement);
}


//на вход подается внешний цикл тесного гнезда
void makeLoopInterchange(ForStatement& forStatement)
{
    ForStatement* innerFor = forStatement.getBody().getFirst()->cast_ptr<ForStatement>();
    OPS_ASSERT(innerFor != 0);
    ReprisePtr<ExpressionBase> initExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getInitExpression(), ReprisePtr<ExpressionBase>(&innerFor->getInitExpression()));
    ReprisePtr<ExpressionBase> finalExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getFinalExpression(), ReprisePtr<ExpressionBase>(&innerFor->getFinalExpression()));
    ReprisePtr<ExpressionBase> stepExpr = OPS::Reprise::Editing::replaceExpression(
        forStatement.getStepExpression(), ReprisePtr<ExpressionBase>(&innerFor->getStepExpression()));
    
    OPS::Reprise::Editing::replaceExpression(innerFor->getInitExpression(), initExpr);
    OPS::Reprise::Editing::replaceExpression(innerFor->getFinalExpression(), finalExpr);
    OPS::Reprise::Editing::replaceExpression(innerFor->getStepExpression(), stepExpr);
}

}	// Loops
}	// Transforms
}	// OPS



