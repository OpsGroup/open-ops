#ifndef OPS_SHARED_LINEAREXPRESSIONSUTILS_H_INCLUDED__
#define OPS_SHARED_LINEAREXPRESSIONSUTILS_H_INCLUDED__

#include "Shared/LinearExpressions.h"

namespace OPS
{
namespace Shared
{

CanonicalLinearExpression& operator+=(CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand);
CanonicalLinearExpression& operator+=(CanonicalLinearExpression& expr, const CanonicalLinearExpression& other);
CanonicalLinearExpression& operator-=(CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand);
CanonicalLinearExpression& operator-=(CanonicalLinearExpression& expr, const CanonicalLinearExpression& other);

CanonicalLinearExpression operator+(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand);
CanonicalLinearExpression operator-(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient freeSummand);
CanonicalLinearExpression operator+(const CanonicalLinearExpression& expr, const CanonicalLinearExpression& other);
CanonicalLinearExpression operator-(const CanonicalLinearExpression& expr, const CanonicalLinearExpression& other);
CanonicalLinearExpression operator*(const CanonicalLinearExpression& expr, CanonicalLinearExpression::Coefficient mult);

bool operator==(const CanonicalLinearExpression& left, const CanonicalLinearExpression& right);
bool operator!=(const CanonicalLinearExpression& left, const CanonicalLinearExpression& right);

/// Строит дерево Reprise соответствующее линейному выражению
Reprise::ReprisePtr<Reprise::ExpressionBase> canonicalLinearExpressionToReprise(const CanonicalLinearExpression& expr);

/// Строит дерево Reprise соответствующее линейному выражению без свободного коэффициента
Reprise::ReprisePtr<Reprise::ExpressionBase> summandsMapToReprise(const CanonicalLinearExpression::SummandsMap& expr);

std::string canonicalLinearExpressionToString(const CanonicalLinearExpression& expr);

}
}

#endif // OPS_SHARED_LINEAREXPRESSIONSUTILS_H_INCLUDED__
