#include "Reprise/Reprise.h"
#include "Transforms/Loops/Canonizing/ConvertLessEqualToLess.h"
#include "Shared/ExpressionHelpers.h"
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

bool canConvertLessEqualToLess(OPS::Reprise::ForStatement& forStat)
{
    OPS::Reprise::ExpressionBase& finalExp = forStat.getFinalExpression();
    if (!finalExp.is_a<OPS::Reprise::BasicCallExpression>())
    {
        return false;
    }
    OPS::Reprise::BasicCallExpression& signPriority = finalExp.cast_to<OPS::Reprise::BasicCallExpression>();
    if (signPriority.getKind() !=  OPS::Reprise::BasicCallExpression::BCK_LESS_EQUAL)
    {
        return false;
    }
    return true;
}

OPS::Reprise::ForStatement&  convertLessEqualToLess(OPS::Reprise::ForStatement& forStmt)
{
    OPS_ASSERT(canConvertLessEqualToLess(forStmt));
    OPS::Reprise::ExpressionBase& finalExp = forStmt.getFinalExpression();
    OPS::Reprise::BasicCallExpression& signPriority = finalExp.cast_to<OPS::Reprise::BasicCallExpression>();
    signPriority.setKind(OPS::Reprise::BasicCallExpression::BCK_LESS);
    OPS::Reprise::ExpressionBase& rightPart = signPriority.getArgument(1);

    OPS::Reprise::ReprisePtr<OPS::Reprise::TypeBase> type = OPS::Reprise::Editing::getExpressionType(rightPart);
    if (type->is_a<OPS::Reprise::BasicType>() &&
        OPS::Shared::ExpressionHelpers::IntegerHelper::isIntegerType(type->cast_to<OPS::Reprise::BasicType>()))
    {
        OPS::Shared::ExpressionHelpers::IntegerHelper ih(type->cast_to<OPS::Reprise::BasicType>());
        signPriority.setArgument(1, &(op(rightPart) + ih(1)));
    }
    else
    {
        OPS::Shared::ExpressionHelpers::IntegerHelper ihInt32(OPS::Reprise::BasicType::BT_INT32);
        signPriority.setArgument(1, &(op(rightPart) + ihInt32(1)));
    }
    return forStmt;
}


}

}
