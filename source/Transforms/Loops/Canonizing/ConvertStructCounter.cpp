#include "Reprise/Reprise.h"
#include "Transforms/Loops/Canonizing/ConvertStructCounter.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Shared/Checks.h"

#include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Transforms;
using namespace OPS::Shared;

namespace OPS
{
namespace Transforms
{
using namespace OPS::Shared::ExpressionHelpers;


bool checkConsistency(ForStatement& fragment)
{
    Checks::CompositionCheckObjects acceptableObjects;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_BlockStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_ForStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_WhileStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_IfStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_ExpressionStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_EmptyStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_SwitchStatement;

    return makeCompositionCheck(fragment, acceptableObjects);
}

bool canChangeStructLoopCounter(ForStatement& forStmt)
{
    if (!checkConsistency(forStmt))
    {
        return false;
    }
    ExpressionBase& stepExpr = forStmt.getStepExpression();
    if (!stepExpr.is_a<BasicCallExpression>())
    {
        return false;
    }
    BasicCallExpression& stepAssignExpr = stepExpr.cast_to<BasicCallExpression>();
    if (stepAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN)
    {
        return false;
    }
    ExpressionBase& leftStepAssignExpr = stepAssignExpr.getArgument(0);
    if (!leftStepAssignExpr.is_a<StructAccessExpression>())
    {
        return false;
    }

    ExpressionBase& pointerOfLeftStepAssignExpr = leftStepAssignExpr.cast_to<StructAccessExpression>().getStructPointerExpression();
    if (!pointerOfLeftStepAssignExpr.is_a<ReferenceExpression>())
    {
        return false;
    }
    VariableDeclaration& structDeclaration = pointerOfLeftStepAssignExpr.cast_to<ReferenceExpression>().getReference();
    if (!structDeclaration.getType().is_a<DeclaredType>())
    {
        return false;
    }
    if (!structDeclaration.getType().cast_to<DeclaredType>().getDeclaration().getType().is_a<StructType>())
    {
        return false;
    }
    return true;
}


StructAccessExpression* getCounterStruct (ForStatement& forStmt)
{
    ExpressionBase& stepExpr = forStmt.getStepExpression();
    BasicCallExpression& stepAssignExpr = stepExpr.cast_to<BasicCallExpression>();
    ExpressionBase& leftStepAssignExpr = stepAssignExpr.getArgument(0);
    return leftStepAssignExpr.cast_ptr<StructAccessExpression>();
}

OPS::Reprise::ForStatement& changeStructLoopCounter(OPS::Reprise::ForStatement& forStmt)
{
    OPS_ASSERT(canChangeStructLoopCounter(forStmt));
    StructAccessExpression* counter = getCounterStruct(forStmt);
    TypeBase& type = counter->getMember().getType();
    BlockStatement& parent = forStmt.getParentBlock();
    VariableDeclaration& declaration = OPS::Reprise::Editing::createNewVariable(type, parent,"nC");
    ReprisePtr<ReferenceExpression> rpTmpVariable(new ReferenceExpression(declaration));

    ReprisePtr<ExpressionStatement> rpStructAssign(new ExpressionStatement(&(op(counter) R_AS op(rpTmpVariable))));
    parent.addAfter(parent.convertToIterator(&forStmt),rpStructAssign.get());

    OPS::Transforms::Scalar::makeSubstitutionForward(forStmt, *counter, rpTmpVariable, true);

    return forStmt;
}



}

}
