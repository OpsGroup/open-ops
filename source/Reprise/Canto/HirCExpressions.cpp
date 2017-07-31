#include "Reprise/Canto/HirCExpressions.h"
#include "Reprise/Statements.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

HirCCallExpression::HirCCallExpression(const HirCCallExpression::HirCOperatorKind operatorKind) 
	: m_operatorKind(operatorKind)
{
}

std::string HirCCallExpression::callKindToString(const HirCOperatorKind operatorKind)
{
	switch (operatorKind)
	{
	//	Unary
	case HIRC_UNARY_PLUS:
		return "+()";
	case HIRC_UNARY_MINUS:
		return "-()";

	case HIRC_SIZE_OF:			// sizeof() operator
		return "sizeof()";

	case HIRC_TAKE_ADDRESS:		// &
		return "&()";
	case HIRC_DE_REFERENCE:		// *
		return "*()";

	case HIRC_PREFIX_PLUS_PLUS:
		return "++()";
	case HIRC_POSTFIX_PLUS_PLUS:
		return "()++";
	case HIRC_PREFIX_MINUS_MINUS:
		return "--()";
	case HIRC_POSTFIX_MINUS_MINUS:
		return "()--";

		//	Binary
	case HIRC_BINARY_PLUS:		// +
		return "()+()";
	case HIRC_BINARY_MINUS:		// -
		return "()-()";
	case HIRC_MULTIPLY:			// *
		return "()*()";
	case HIRC_DIVISION:			// / 
		return "()/()";
//	case BCK_INTEGER_DIVISION:	// div
//		return "div";
	case HIRC_INTEGER_MOD:		// mod (%)
		return "mod";

	//	Assign
	case HIRC_ASSIGN:				// =
		return "=";

	case HIRC_PLUS_ASSIGN:				// +=
		return "+=";
	case HIRC_MINUS_ASSIGN:				// -=
		return "-=";
	case HIRC_MULTIPLY_ASSIGN:			// *=
		return "*=";
	case HIRC_DIVISION_ASSIGN:			// /=
		return "/=";
	case HIRC_MOD_ASSIGN:				// %=
		return "%=";
	case HIRC_LSHIFT_ASSIGN:				// <<=
		return "<<=";
	case HIRC_RSHIFT_ASSIGN:				// >>=
		return ">>=";

	case HIRC_BAND_ASSIGN:				// &=
		return "&=";
	case HIRC_BOR_ASSIGN:				// |=
		return "|=";
	case HIRC_BXOR_ASSIGN:				// ^=
		return "^=";

	//	Equality
	case HIRC_LESS:				// <
		return "<";
	case HIRC_GREATER:			// >
		return ">";
	case HIRC_LESS_EQUAL:		// <=
		return "<=";
	case HIRC_GREATER_EQUAL:		// >=
		return ">=";
	case HIRC_EQUAL:				// ==
		return "==";
	case HIRC_NOT_EQUAL:			// !=
		return "!=";

		//	Shifts
	case HIRC_LEFT_SHIFT:			// <<
		return "<<";
	case HIRC_RIGHT_SHIFT:		// >>
		return ">>";

		//	Logical
	case HIRC_LOGICAL_NOT:		// !
		return "!";
	case HIRC_LOGICAL_AND:		// &&
		return "&&";
	case HIRC_LOGICAL_OR:			// ||
		return "||";

		//	Bitwise
	case HIRC_BITWISE_NOT:		// ~
		return "~";
	case HIRC_BITWISE_AND:		// &
		return "&";
	case HIRC_BITWISE_OR:			// |
		return "|";
	case HIRC_BITWISE_XOR:		// ^
		return "^";
		//	Special
	case HIRC_ARRAY_ACCESS:		// []
		return "[]";
	case HIRC_COMMA:			// ,
		return ", ";
	case HIRC_CONDITIONAL:		// ? :
		return "?:";

		OPS_DEFAULT_CASE_LABEL
	}
	return "(unexpected) HirCCallExpression::callKindToString";
}


std::string HirCCallExpression::dumpKindFormat(HirCOperatorKind kind)
{
	switch (kind)
	{
	//	Unary
	case HIRC_UNARY_PLUS:
		return "+(%s)";
	case HIRC_UNARY_MINUS:
		return "-(%s)";

	case HIRC_SIZE_OF:			// sizeof() operator
		return "sizeof(%s)";

	case HIRC_TAKE_ADDRESS:		// &
		return "&(%s)";
	case HIRC_DE_REFERENCE:		// *
		return "*(%s)";

	case HIRC_PREFIX_PLUS_PLUS:
		return "++(%s)";
	case HIRC_POSTFIX_PLUS_PLUS:
		return "(%s)++";
	case HIRC_PREFIX_MINUS_MINUS:
		return "--(%s)";
	case HIRC_POSTFIX_MINUS_MINUS:
		return "(%s)--";

		//	Binary
	case HIRC_BINARY_PLUS:		// +
		return "(%s)+(%s)";
	case HIRC_BINARY_MINUS:		// -
		return "(%s)-(%s)";
	case HIRC_MULTIPLY:			// *
		return "(%s)*(%s)";
	case HIRC_DIVISION:			// / 
		return "(%s)/(%s)";
//	case BCK_INTEGER_DIVISION:	// div
//		return "div";
	case HIRC_INTEGER_MOD:		// mod (%)
		return "(%s)%%(%s)";

	//	Assign
	case HIRC_ASSIGN:				// =
		return "=";

	case HIRC_PLUS_ASSIGN:				// +=
		return "(%s)+=(%s)";
	case HIRC_MINUS_ASSIGN:				// -=
		return "(%s)-=(%s)";
	case HIRC_MULTIPLY_ASSIGN:			// *=
		return "(%s)*=(%s)";
	case HIRC_DIVISION_ASSIGN:			// /=
		return "(%s)/=(%s)";
	case HIRC_MOD_ASSIGN:				// %=
		return "(%s)%%=(%s)";
	case HIRC_LSHIFT_ASSIGN:				// <<=
		return "(%s)<<=(%s)";
	case HIRC_RSHIFT_ASSIGN:				// >>=
		return "(%s)>>=(%s)";

	case HIRC_BAND_ASSIGN:				// &=
		return "(%s)&=(%s)";
	case HIRC_BOR_ASSIGN:				// |=
		return "(%s)|=(%s)";
	case HIRC_BXOR_ASSIGN:				// ^=
		return "(%s)^=(%s)";

	//	Equality
	case HIRC_LESS:				// <
		return "(%s)<(%s)";
	case HIRC_GREATER:			// >
		return "(%s)>(%s)";
	case HIRC_LESS_EQUAL:		// <=
		return "(%s)<=(%s)";
	case HIRC_GREATER_EQUAL:		// >=
		return "(%s)>=(%s)";
	case HIRC_EQUAL:				// ==
		return "(%s)==(%s)";
	case HIRC_NOT_EQUAL:			// !=
		return "(%s)!=(%s)";

		//	Shifts
	case HIRC_LEFT_SHIFT:			// <<
		return "(%s)<<(%s)";
	case HIRC_RIGHT_SHIFT:		// >>
		return "(%s)>>(%s)";

		//	Logical
	case HIRC_LOGICAL_NOT:		// !
		return "!(%s)";
	case HIRC_LOGICAL_AND:		// &&
		return "(%s)&&(%s)";
	case HIRC_LOGICAL_OR:			// ||
		return "(%s)||(%s)";

		//	Bitwise
	case HIRC_BITWISE_NOT:		// ~
		return "~(%s)";
	case HIRC_BITWISE_AND:		// &
		return "(%s)&(%s)";
	case HIRC_BITWISE_OR:			// |
		return "(%s)|(%s)";
	case HIRC_BITWISE_XOR:		// ^
		return "(%s)^(%s)";
		//	Special
	case HIRC_ARRAY_ACCESS:		// []
		return "(%s)[(%s)]";
	case HIRC_COMMA:			// ,
		return "(%s), (%s)";
	case HIRC_CONDITIONAL:		// ? :
		return "(%s)?(%s):(%s)";

		OPS_DEFAULT_CASE_LABEL
	}
	return "(Unexpcted dumpKindFormat)";
}

HirCCallExpression::HirCOperatorKind HirCCallExpression::getKind(void) const
{
	return m_operatorKind;
}

bool HirCCallExpression::isUnaryComplexAssign() const
{
	switch (m_operatorKind)
	{
	case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:			// ++()
	case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:		// ()++
	case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:		// --()
	case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:		// ()--
		return true;
	default:
		return false;
	}
}

bool HirCCallExpression::isBinaryComplexAssign() const
{
	switch (m_operatorKind)
	{
	case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
	case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
	case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
	case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
	case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
	case HirCCallExpression::HIRC_LSHIFT_ASSIGN:			// <<=
	case HirCCallExpression::HIRC_RSHIFT_ASSIGN:			// >>=
	case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
	case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
	case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
		return true;
	default:
		return false;
	}
}

//		ExpressionBase implementation
bool HirCCallExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<HirCCallExpression>())
	{
		const HirCCallExpression& other = exprNode.cast_to<HirCCallExpression>();
		if (other.m_operatorKind == m_operatorKind && CallExpressionBase::isEqual(other))
		{
			return true;
		}
	}
	return false;
}

//		ClonableMix implementation
HirCCallExpression* HirCCallExpression::clone(void) const
{
	HirCCallExpression* callExpression = new HirCCallExpression(m_operatorKind);
	for (ArgumentsType::const_iterator iter = m_arguments.begin(); iter != m_arguments.end(); ++iter)
	{
		callExpression->addArgument((*iter)->clone());
	}
	callExpression->acquireNotes(*this);
	return callExpression;
}

//		RepriseBase implementation
int HirCCallExpression::getChildCount(void) const
{
	return getArgumentCount();
}

RepriseBase& HirCCallExpression::getChild(const int index)
{
	return getArgument(index);
}

std::string HirCCallExpression::dumpState(void) const
{
	std::string state = CallExpressionBase::dumpState();
	state += dumpStateHelper(dumpKindFormat(m_operatorKind));
	return state;
}

HirCStatementExpression::HirCStatementExpression(BlockStatement *subStatements)
    :m_subStatements(subStatements)
{
    m_subStatements->setParent(this);
}

const BlockStatement& HirCStatementExpression::getSubStatements() const
{
    return *m_subStatements;
}

BlockStatement& HirCStatementExpression::getSubStatements()
{
    return *m_subStatements;
}

void HirCStatementExpression::setSubStatements(BlockStatement *subStatements)
{
    m_subStatements.reset(subStatements);
    m_subStatements->setParent(this);
}

bool HirCStatementExpression::isEqual(const ExpressionBase &exprNode) const
{
    throw OPS::StateError("HirCStatementExpression::isEqual is not implemented");
    return false;
}

HirCStatementExpression* HirCStatementExpression::clone() const
{
    return new HirCStatementExpression(m_subStatements->clone());
}

int HirCStatementExpression::getChildCount() const
{
    return 1;
}

RepriseBase& HirCStatementExpression::getChild(int index)
{
    return *m_subStatements;
}

std::string HirCStatementExpression::dumpState() const
{
    return "(" + m_subStatements->dumpState() + ")";
}

}
}
}
