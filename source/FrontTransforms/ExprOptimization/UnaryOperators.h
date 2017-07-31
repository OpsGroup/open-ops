#ifndef EXPRESSION_OPTIMIZATION_UNARY_OPERATORS_H
#define EXPRESSION_OPTIMIZATION_UNARY_OPERATORS_H

#include "Reprise/Reprise.h"
#include "FrontTransforms/BaseOperator.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Calculation
{	
using OPS::Reprise::ExpressionBase;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::TypeCastExpression;

class UnaryPlusOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class UnaryMinusOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class LogicalNotOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class BitwiseNotOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

/* Spefific TypeCast operator */
class TypeCastOperator 
{
public:
	ExpressionBase* calculate(TypeCastExpression*&);
};
}
}
}

#endif
