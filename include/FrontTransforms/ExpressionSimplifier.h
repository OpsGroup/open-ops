#ifndef EXPRESSION_OPTIMIZATION_EXPRESSION_SIMPLIFIER_H
#define EXPRESSION_OPTIMIZATION_EXPRESSION_SIMPLIFIER_H

#include "Reprise/Reprise.h"
#include "BaseOperator.h"

namespace OPS
{
namespace ExpressionSimplifier
{		
	using OPS::Reprise::ExpressionBase;
	using OPS::Reprise::BasicCallExpression;
	using OPS::Reprise::TypeCastExpression;
	using OPS::Reprise::StrictLiteralExpression;
	using OPS::ExpressionSimplifier::Calculation::BaseOperator;

	class Simplifier
	{
	public:
		Simplifier();
		~Simplifier();

		ExpressionBase* simplify(const ExpressionBase* const inputExpression);
		ExpressionBase* simplify(const BasicCallExpression* const inputExpression);
		ExpressionBase* simplify(const TypeCastExpression* const inputExpression);
		ExpressionBase* simplify(const StrictLiteralExpression* const inputExpression);

		void simplifyAndReplace(OPS::Reprise::ExpressionStatement& exprStatement);
				
	private:
		ExpressionBase* private_SimplifyBinaryAssociativeAndCommutativeOperation(BasicCallExpression*& inputExpression);
		
		typedef std::map<BasicCallExpression::BuiltinCallKind, BaseOperator*>  OperatorsMap;		
		OperatorsMap operators; 
		std::set<BasicCallExpression::BuiltinCallKind> commutativeOperators, associativeOperators;
	};
    
}
}

#endif 
