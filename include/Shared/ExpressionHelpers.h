#ifndef _EXPRESSION_HELPERS_H_INCLUDED_
#define _EXPRESSION_HELPERS_H_INCLUDED_

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Shared
{
namespace ExpressionHelpers
{

using OPS::Reprise::ReprisePtr;

using OPS::Reprise::BasicType;
using OPS::Reprise::TypeBase;

using OPS::Reprise::ExpressionBase;

using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::StrictLiteralExpression;
using OPS::Reprise::BasicLiteralExpression;


// Usual operators
#define DEFINE_UNARY_REPRISE_OPERATOR(_operator) \
BasicCallExpression& operator _operator(ExpressionBase& expressionBase);

#define DEFINE_BINARY_REPRISE_OPERATOR(_operator) \
BasicCallExpression& operator _operator(ExpressionBase& expressionBase1, ExpressionBase& expressionBase2);


// Binary macro operators
struct BinaryExpressionHelper
{
	BinaryExpressionHelper(BasicCallExpression::BuiltinCallKind builtinCallKind);

	BasicCallExpression::BuiltinCallKind builtinCallKind;
	ExpressionBase* expressionBase;
};

#define ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS(_operator) \
BinaryExpressionHelper operator _operator(ExpressionBase& expressionBase, BinaryExpressionHelper binaryHelper); \
BasicCallExpression& operator _operator(BinaryExpressionHelper binaryHelper, ExpressionBase& expressionBase);

#define REPRISE_BINARY_MACROOPERATOR_WITH_PRIORITY_AS(_operator, builtinCallKind) \
_operator BinaryExpressionHelper(builtinCallKind) _operator


// Unary prefix macro operators
struct UnaryExpressionHelper
{
	UnaryExpressionHelper(ExpressionBase* expressionBase);

	operator ExpressionBase&() const;

	ExpressionBase* expressionBase;
	int internalType;
};

UnaryExpressionHelper operator +(ExpressionBase& expressionBase);
UnaryExpressionHelper operator +(UnaryExpressionHelper unaryHelper);


// mapping BasicCallExpression::BuiltinCallKind to C++ structures
ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS(||)
ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS(/)

// Unary
// BCK_UNARY_PLUS
#define R_UP() \
(ExpressionBase&) + 
// BCK_UNARY_MINUS
DEFINE_UNARY_REPRISE_OPERATOR(-)

// BCK_SIZE_OF sizeof() operator - not implemented yet
BasicCallExpression& R_sizeOf(ExpressionBase& expressionBase);

// BCK_TAKE_ADDRESS &
#define R_AD() \
(ExpressionBase&) + + 
// BCK_DE_REFERENCE *
#define R_DR() \
(ExpressionBase&) + + + 

// Binary
// BCK_BINARY_PLUS +
DEFINE_BINARY_REPRISE_OPERATOR(+)
// BCK_BINARY_MINUS -
DEFINE_BINARY_REPRISE_OPERATOR(-)
// BCK_MULTIPLY *
DEFINE_BINARY_REPRISE_OPERATOR(*)
// BCK_DIVISION / 
DEFINE_BINARY_REPRISE_OPERATOR(/)
// BCK_INTEGER_DIVISION div - will be excluded from Reprise
// BCK_INTEGER_MOD mod (%)
DEFINE_BINARY_REPRISE_OPERATOR(%)

// Assign
// BCK_ASSIGN =
// WARNING: Priority of this macro operator same as operator ||.
// So you have to use additional brackets depends on situation.
#define R_AS \
REPRISE_BINARY_MACROOPERATOR_WITH_PRIORITY_AS(||, BasicCallExpression::BCK_ASSIGN) 

// Equality
// BCK_LESS <
DEFINE_BINARY_REPRISE_OPERATOR(<)
// BCK_GREATER >
DEFINE_BINARY_REPRISE_OPERATOR(>)
// BCK_LESS_EQUAL <=
DEFINE_BINARY_REPRISE_OPERATOR(<=)
// BCK_GREATER_EQUAL >=
DEFINE_BINARY_REPRISE_OPERATOR(>=)
// BCK_EQUAL ==
DEFINE_BINARY_REPRISE_OPERATOR(==)
// BCK_NOT_EQUAL !=
DEFINE_BINARY_REPRISE_OPERATOR(!=)

// Shifts
// BCK_LEFT_SHIFT <<
DEFINE_BINARY_REPRISE_OPERATOR(<<)
// BCK_RIGHT_SHIFT >>
DEFINE_BINARY_REPRISE_OPERATOR(>>)

// Logical
// BCK_LOGICAL_NOT !
DEFINE_UNARY_REPRISE_OPERATOR(!)
// BCK_LOGICAL_AND &&
DEFINE_BINARY_REPRISE_OPERATOR(&&)
// BCK_LOGICAL_OR ||
DEFINE_BINARY_REPRISE_OPERATOR(||)

// Bitwise
// BCK_BITWISE_NOT ~
DEFINE_UNARY_REPRISE_OPERATOR(~)
// BCK_BITWISE_AND &
DEFINE_BINARY_REPRISE_OPERATOR(&)
// BCK_BITWISE_OR |
DEFINE_BINARY_REPRISE_OPERATOR(|)
// BCK_BITWISE_XOR ^
DEFINE_BINARY_REPRISE_OPERATOR(^)

// Special
// BCK_ARRAY_ACCESS []
// WARNING: Priority of this macro operator same as operator /.
// So you have to use additional brackets depends on situation.
// EXAMPLE: Recommended use case for creating expression a[b] in common situation is ( a R_BK(b) ).
// But depends on situation you can write a R_BK(b).
#define R_BK(expressionBase) \
REPRISE_BINARY_MACROOPERATOR_WITH_PRIORITY_AS(/, BasicCallExpression::BCK_ARRAY_ACCESS) expressionBase 
// BCK_COMMA , - will be excluded from Reprise

#undef DEFINE_UNARY_REPRISE_OPERATOR
#undef DEFINE_BINARY_REPRISE_OPERATOR
#undef ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS


// IntegerHelper
class IntegerHelper
{
public:
	explicit IntegerHelper(BasicType::BasicTypes literalType);
    explicit IntegerHelper(const Reprise::BasicType& basicType);

	static bool isIntegerType(BasicType::BasicTypes literalType);
    static bool isIntegerType(const Reprise::BasicType& basicType);

	// StrictLiteralExpression with unsigned value helpers functions
	static bool isIntegerType(StrictLiteralExpression& strictLiteralExpression);
	static bool isGreaterThanOrEqualToZero(StrictLiteralExpression& strictLiteralExpression);
	static qword getUnsignedValue(StrictLiteralExpression& strictLiteralExpression);

	StrictLiteralExpression& operator ()(sbyte value) const;
	StrictLiteralExpression& operator ()(sword value) const;
	StrictLiteralExpression& operator ()(sdword value) const;
	StrictLiteralExpression& operator ()(sqword value) const;
	StrictLiteralExpression& operator ()(byte value) const;
	StrictLiteralExpression& operator ()(word value) const;
	StrictLiteralExpression& operator ()(dword value) const;
	StrictLiteralExpression& operator ()(qword value) const;

private:
	BasicType::BasicTypes m_literalType;
};


// Create operands helpers
template<class RepriseExpression>
ExpressionBase& op(RepriseExpression* expression)
{
	if ( expression == 0 )	
	{
		IntegerHelper ih(BasicType::BT_INT32);	
		return ih(0);
	}

	return *expression->clone();
}

inline ExpressionBase& opnc(ExpressionBase* expression)
{
	if (expression == 0)
	{
		IntegerHelper ih(BasicType::BT_INT32);
		return ih(0);
	}
	return *expression;
}

typedef ReprisePtr<ExpressionBase> ExprBasePtr;

template<class RepriseExpression>
ExpressionBase& op(ReprisePtr<RepriseExpression> expression)
{
	return *expression->clone();
}

template<class RepriseExpression>
ExpressionBase& op(RepriseExpression& expression)
{
	return *expression.clone();
}

ExpressionBase& Conditional(ExpressionBase& cond, ExpressionBase& trueVal, ExpressionBase& falseVal);

ExpressionBase& Min(ExpressionBase& e1, ExpressionBase& e2);
ExpressionBase& Max(ExpressionBase& e1, ExpressionBase& e2);

} // namespace OPS
} // namespace Reprise
} // namespace ExpressionHelpers

#endif	// _EXPRESSION_HELPERS_H_INCLUDED_
