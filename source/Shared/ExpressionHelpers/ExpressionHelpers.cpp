#include "Shared/ExpressionHelpers.h"

namespace OPS
{
namespace Shared
{
namespace ExpressionHelpers
{

static BasicCallExpression& unaryRepriseOperatorImplementation(BasicCallExpression::BuiltinCallKind builtinCallKind,
	ExpressionBase& expressionBase)
{
	BasicCallExpression* basicCallExpression = new BasicCallExpression(builtinCallKind);
	basicCallExpression->addArgument(&expressionBase);

	return *basicCallExpression;
}

static BasicCallExpression& binaryRepriseOperatorImplementation(BasicCallExpression::BuiltinCallKind builtinCallKind,
	ExpressionBase& expressionBase1, ExpressionBase& expressionBase2)
{
	BasicCallExpression* basicCallExpression = new BasicCallExpression(builtinCallKind);
	basicCallExpression->addArgument(&expressionBase1);
	basicCallExpression->addArgument(&expressionBase2);

	return *basicCallExpression;
}


// Usual operators
#define UNARY_REPRISE_OPERATOR(_operator, builtinCallKind) \
BasicCallExpression& operator _operator(ExpressionBase& expressionBase) \
{ \
	return unaryRepriseOperatorImplementation(builtinCallKind, expressionBase); \
}

#define BINARY_REPRISE_OPERATOR(_operator, builtinCallKind) \
BasicCallExpression& operator _operator(ExpressionBase& expressionBase1, ExpressionBase& expressionBase2) \
{ \
	return binaryRepriseOperatorImplementation(builtinCallKind, expressionBase1, expressionBase2); \
}


// Binary macro operators
BinaryExpressionHelper::BinaryExpressionHelper(BasicCallExpression::BuiltinCallKind builtinCallKind)
: builtinCallKind(builtinCallKind)
, expressionBase(NULL)
{
}

#define ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS_IMPL(_operator) \
BinaryExpressionHelper operator _operator(ExpressionBase& expressionBase, BinaryExpressionHelper binaryHelper) \
{ \
	binaryHelper.expressionBase = &expressionBase; \
 \
	return binaryHelper; \
} \
 \
BasicCallExpression& operator _operator(BinaryExpressionHelper binaryHelper, ExpressionBase& expressionBase) \
{ \
	OPS_ASSERT(binaryHelper.expressionBase != NULL); \
 \
	return binaryRepriseOperatorImplementation(binaryHelper.builtinCallKind, *binaryHelper.expressionBase, expressionBase); \
}


// Unary macro operators
UnaryExpressionHelper::UnaryExpressionHelper(ExpressionBase* expressionBase)
: expressionBase(expressionBase)
, internalType(0)
{
	OPS_ASSERT(expressionBase != NULL);
}

UnaryExpressionHelper::operator ExpressionBase&() const
{
	static const BasicCallExpression::BuiltinCallKind s_callKindMap[] =
	{
		BasicCallExpression::BCK_UNARY_PLUS,
		BasicCallExpression::BCK_TAKE_ADDRESS,
		BasicCallExpression::BCK_DE_REFERENCE
	};
	static const int s_callKindCount = 3;

	OPS_ASSERT(0 <= internalType && internalType < s_callKindCount);
	OPS_ASSERT(expressionBase);
	
	return unaryRepriseOperatorImplementation(s_callKindMap[internalType], *expressionBase);
}

UnaryExpressionHelper operator +(ExpressionBase& expressionBase)
{
	return UnaryExpressionHelper(&expressionBase);
}

UnaryExpressionHelper operator +(UnaryExpressionHelper unaryHelper)
{
	++unaryHelper.internalType;
	return unaryHelper;
}


// mapping BasicCallExpression::BuiltinCallKind to C++ structures
ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS_IMPL(||)
ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS_IMPL(/)

// Unary
// BCK_UNARY_PLUS - implementation in header
// BCK_UNARY_MINUS
UNARY_REPRISE_OPERATOR(-, BasicCallExpression::BCK_UNARY_MINUS)

// BCK_SIZE_OF sizeof() operator - not implemented yet
BasicCallExpression& R_sizeOf(ExpressionBase& expressionBase)
{
	return unaryRepriseOperatorImplementation(BasicCallExpression::BCK_SIZE_OF, expressionBase);
}

// BCK_TAKE_ADDRESS & - implementation in header
// BCK_DE_REFERENCE * - implementation in header

// Binary
// BCK_BINARY_PLUS +
BINARY_REPRISE_OPERATOR(+, BasicCallExpression::BCK_BINARY_PLUS)
// BCK_BINARY_MINUS -
BINARY_REPRISE_OPERATOR(-, BasicCallExpression::BCK_BINARY_MINUS)
// BCK_MULTIPLY *
BINARY_REPRISE_OPERATOR(*, BasicCallExpression::BCK_MULTIPLY)
// BCK_DIVISION /
BINARY_REPRISE_OPERATOR(/, BasicCallExpression::BCK_DIVISION)
// BCK_INTEGER_DIVISION div - will be excluded from Reprise
// BCK_INTEGER_MOD mod (%)
BINARY_REPRISE_OPERATOR(%, BasicCallExpression::BCK_INTEGER_MOD)

// Assign
// BCK_ASSIGN = - implementation in header

// Equality
// BCK_LESS <
BINARY_REPRISE_OPERATOR(<, BasicCallExpression::BCK_LESS)
// BCK_GREATER >
BINARY_REPRISE_OPERATOR(>, BasicCallExpression::BCK_GREATER)
// BCK_LESS_EQUAL <=
BINARY_REPRISE_OPERATOR(<=, BasicCallExpression::BCK_LESS_EQUAL)
// BCK_GREATER_EQUAL >=
BINARY_REPRISE_OPERATOR(>=, BasicCallExpression::BCK_GREATER_EQUAL)
// BCK_EQUAL ==
BINARY_REPRISE_OPERATOR(==, BasicCallExpression::BCK_EQUAL)
// BCK_NOT_EQUAL !=
BINARY_REPRISE_OPERATOR(!=, BasicCallExpression::BCK_NOT_EQUAL)

// Shifts
// BCK_LEFT_SHIFT <<
BINARY_REPRISE_OPERATOR(<<, BasicCallExpression::BCK_LEFT_SHIFT)
// BCK_RIGHT_SHIFT >>
BINARY_REPRISE_OPERATOR(>>, BasicCallExpression::BCK_RIGHT_SHIFT)

// Logical
// BCK_LOGICAL_NOT !
UNARY_REPRISE_OPERATOR(!, BasicCallExpression::BCK_LOGICAL_NOT)
// BCK_LOGICAL_AND &&
BINARY_REPRISE_OPERATOR(&&, BasicCallExpression::BCK_LOGICAL_AND)
// BCK_LOGICAL_OR ||
BINARY_REPRISE_OPERATOR(||, BasicCallExpression::BCK_LOGICAL_OR)

// Bitwise
// BCK_BITWISE_NOT ~
UNARY_REPRISE_OPERATOR(~, BasicCallExpression::BCK_BITWISE_NOT)
// BCK_BITWISE_AND &
BINARY_REPRISE_OPERATOR(&, BasicCallExpression::BCK_BITWISE_AND)
// BCK_BITWISE_OR |
BINARY_REPRISE_OPERATOR(|, BasicCallExpression::BCK_BITWISE_OR)
// BCK_BITWISE_XOR ^
BINARY_REPRISE_OPERATOR(^, BasicCallExpression::BCK_BITWISE_XOR)

// Special
// BCK_ARRAY_ACCESS [] - implementation in header
// BCK_COMMA , - will be excluded from Reprise

#undef UNARY_REPRISE_OPERATOR
#undef BINARY_REPRISE_OPERATOR
#undef ALLOW_REPRISE_BINARY_MACROOPERATORS_WITH_SAME_PRIORITY_AS_IMPL


// IntegerHelper
IntegerHelper::IntegerHelper(BasicType::BasicTypes literalType)
: m_literalType(literalType)
{
	OPS_ASSERT(isIntegerType(m_literalType));
}

IntegerHelper::IntegerHelper(const Reprise::BasicType &basicType)
{
    OPS_ASSERT(isIntegerType(basicType));
    m_literalType = basicType.getKind();
}

bool IntegerHelper::isIntegerType(BasicType::BasicTypes literalType)
{
	return BasicType::BT_INT8 <= literalType && literalType <= BasicType::BT_UINT64;
}

bool IntegerHelper::isIntegerType(const Reprise::BasicType &basicType)
{
    return isIntegerType(basicType.getKind());
}

bool IntegerHelper::isIntegerType(StrictLiteralExpression& strictLiteralExpression)
{
	return isIntegerType(strictLiteralExpression.getLiteralType());
}

bool IntegerHelper::isGreaterThanOrEqualToZero(StrictLiteralExpression& strictLiteralExpression)
{
	OPS_ASSERT(isIntegerType(strictLiteralExpression));
	
	switch (strictLiteralExpression.getLiteralType())
	{
		case BasicType::BT_INT8:
		{
			if (strictLiteralExpression.getInt8() >= 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		case BasicType::BT_INT16:
		{
			if (strictLiteralExpression.getInt16() >= 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		case BasicType::BT_INT32:
		{
			if (strictLiteralExpression.getInt32() >= 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		case BasicType::BT_INT64:
		{
			if (strictLiteralExpression.getInt64() >= 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		case BasicType::BT_UINT8:
		case BasicType::BT_UINT16:
		case BasicType::BT_UINT32:
		case BasicType::BT_UINT64:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

qword IntegerHelper::getUnsignedValue(StrictLiteralExpression& strictLiteralExpression)
{
	OPS_ASSERT(isGreaterThanOrEqualToZero(strictLiteralExpression));

	switch (strictLiteralExpression.getLiteralType())
	{
		case BasicType::BT_INT8:
		{
			return static_cast<qword>(strictLiteralExpression.getInt8());
		}
		case BasicType::BT_INT16:
		{
			return static_cast<qword>(strictLiteralExpression.getInt16());
		}
		case BasicType::BT_INT32:
		{
			return static_cast<qword>(strictLiteralExpression.getInt32());
		}
		case BasicType::BT_INT64:
		{
			return static_cast<qword>(strictLiteralExpression.getInt64());
		}
		case BasicType::BT_UINT8:
		{
			return static_cast<qword>(strictLiteralExpression.getUInt8());
		}
		case BasicType::BT_UINT16:
		{
			return static_cast<qword>(strictLiteralExpression.getUInt16());
		}
		case BasicType::BT_UINT32:
		{
			return static_cast<qword>(strictLiteralExpression.getUInt32());
		}
		case BasicType::BT_UINT64:
		{
			return strictLiteralExpression.getUInt64();
		}
		default:
		{
			return 0;
		}
	}
}

template<class integerType>
static StrictLiteralExpression& integerOperatorBracketsImplementation(integerType value, BasicType::BasicTypes literalType)
{
	switch (literalType)
	{
		case BasicType::BT_INT8:
		{
			return *StrictLiteralExpression::createInt8(static_cast<sbyte>(value));
		}
		case BasicType::BT_INT16:
		{
			return *StrictLiteralExpression::createInt16(static_cast<sword>(value));
		}
		case BasicType::BT_INT32:
		{
			return *StrictLiteralExpression::createInt32(static_cast<sdword>(value));
		}
		case BasicType::BT_INT64:
		{
			return *StrictLiteralExpression::createInt64(static_cast<sqword>(value));
		}
		case BasicType::BT_UINT8:
		{
			return *StrictLiteralExpression::createUInt8(static_cast<byte>(value));
		}
		case BasicType::BT_UINT16:
		{
			return *StrictLiteralExpression::createUInt16(static_cast<word>(value));
		}
		case BasicType::BT_UINT32:
		{
			return *StrictLiteralExpression::createUInt32(static_cast<dword>(value));
		}
		case BasicType::BT_UINT64:
		{
			return *StrictLiteralExpression::createUInt64(static_cast<qword>(value));
		}
		default:
		{
			OPS_ASSERT(!"IntegerHelper: unknown type");
			return *StrictLiteralExpression::createInt8(-13);
		}
	}
}

StrictLiteralExpression& IntegerHelper::operator ()(sbyte value) const
{
	return integerOperatorBracketsImplementation<sbyte>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(sword value) const
{
	return integerOperatorBracketsImplementation<sword>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(sdword value) const
{
	return integerOperatorBracketsImplementation<sdword>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(sqword value) const
{
	return integerOperatorBracketsImplementation<sqword>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(byte value) const
{
	return integerOperatorBracketsImplementation<byte>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(word value) const
{
	return integerOperatorBracketsImplementation<word>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(dword value) const
{
	return integerOperatorBracketsImplementation<dword>(value, m_literalType);
}

StrictLiteralExpression& IntegerHelper::operator ()(qword value) const
{
	return integerOperatorBracketsImplementation<qword>(value, m_literalType);
}

Reprise::ExpressionBase &Conditional(Reprise::ExpressionBase &cond, Reprise::ExpressionBase &trueVal, Reprise::ExpressionBase &falseVal)
{
    BasicCallExpression* basicCallExpression = new BasicCallExpression(BasicCallExpression::BCK_CONDITIONAL);
    basicCallExpression->addArgument(&cond);
    basicCallExpression->addArgument(&trueVal);
    basicCallExpression->addArgument(&falseVal);
    return *basicCallExpression;
}

ExpressionBase& Min(ExpressionBase& e1, ExpressionBase& e2)
{
	return Conditional(e1 > e2, op(e2), op(e1));
}

ExpressionBase& Max(ExpressionBase& e1, ExpressionBase& e2)
{
	return Conditional(e1 > e2, op(e1), op(e2));
}


} // namespace OPS
} // namespace Reprise
} // namespace ExpressionHelpers
