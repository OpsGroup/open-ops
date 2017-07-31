#ifndef EXPRESSION_OPTIMIZATION_BASE_OPERATOR_H
#define EXPRESSION_OPTIMIZATION_BASE_OPERATOR_H

#include "Reprise/Reprise.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Calculation
{
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ExpressionBase;

class BaseOperator{
public:
	// любая реализация метода calculate может делать следующее:
	// 1) измененение expression
	// 2) возвращать НОВОЕ дерево либо 0.
	virtual ExpressionBase* calculate(BasicCallExpression*& expression) = 0;
	virtual ~BaseOperator() {}
};

}
}
}

#endif 
