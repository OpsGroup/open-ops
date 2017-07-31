#ifndef EXPRESSION_OPTIMIZATION_BINARY_OPERATIONS_H
#define EXPRESSION_OPTIMIZATION_BINARY_OPERATIONS_H

#include "Reprise/Reprise.h"
#include "FrontTransforms/BaseOperator.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Calculation
{

using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ExpressionBase;

class BinaryPlusOperator : public BaseOperator
{	
public:		
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class BinaryMinusOperator : public BaseOperator
{	
public:		
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class MultiplyOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class DivisionOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class IntegerDivisionOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class IntegerModOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};
	

/* Equality operators */
class LessOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class GreaterOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class LessEqualOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class GreaterEqualOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class EqualOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class NotEqualOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};
	
/* Shift operators */

class LeftShiftOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class RightShiftOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

/* Logical */

class LogicalAndOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class LogicalOrOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};


/* Bitwise */

class BitwiseAndOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};	

class BitwiseOrOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};

class BitwiseXorOperator : public BaseOperator
{	
public:	
	virtual ExpressionBase* calculate(BasicCallExpression*&);
};
		
}
}
}

#endif
