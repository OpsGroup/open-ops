#ifndef OPS_SHARED_EXPRESSIONOPERATOR_H_INCLUDED__
#define OPS_SHARED_EXPRESSIONOPERATOR_H_INCLUDED__

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Shared
{

	/// Describes priority level for all operators that influence calculations.
	/// For example, operators [], ->, . are part of ExpressionVariable and out of calculations leve
	enum EN_Operator_Precedence_Level
	{
		OPL_UNDEFINED = 0,
		OPL_OUT_OF_CONSIDERATION,
		OPL_POSTFIX,		// []; операции ++, -- не входят в репрайз
		OPL_PREFIX,			// &, *, size_of - не логические и не арифметические унарные операции,
							// которые будут входить в SymbolicDescription (???), а не PriorityMonomial
		OPL_UNARY,			// +, - , ~, ! 
		OPL_MULTIPLICATIVE, // *,/,%
		OPL_ADDITIVE,		// +, - 
		OPL_SHIFTS,			// >>, <<
		OPL_RELATIONAL,		// <, >, >=, <=
		OPL_EQUALITY,		// ==, !=
		OPL_BITWISE_AND,	// &
		OPL_BITWISE_XOR,	// ^
		OPL_BITWISE_OR,		// |
		OPL_AND,			// &&
		OPL_OR,				// ||
		OPL_CONDITIONAL,	// ? :
		OPL_COMMA			// ,

	};

	/// Определяет приоритет операции
	EN_Operator_Precedence_Level getOperatorPrecedency(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind);

	/// Сравнивает приоритет операций. OPL_PREFIX == OPL_UNARY
	bool isPrecedent(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind1, 
								Reprise::BasicCallExpression::BuiltinCallKind OperatorKind2);

	/// Определяет ращрешено ли применять дистрибутивный закон к данной паре операций
	bool allowDistributiveLaw(Reprise::BasicCallExpression::BuiltinCallKind OuterOperatorKind, 
								Reprise::BasicCallExpression::BuiltinCallKind InnerOperatorKind);

	/** 
		Возвращает местность (арность) операции 
		\result
		0 - если с операцией не корректная, 
		от 1 до 3 - означает местность (арность) операции, 
		4 - бесконечная арность операции запятая (,)
	*/ 
	unsigned getOperatorArity(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind);

	/// Определяет ассоциативна ли операция
	bool isOperatorAssociative(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind);


} // end namespace Shared
} // end namespace OPS

#endif
