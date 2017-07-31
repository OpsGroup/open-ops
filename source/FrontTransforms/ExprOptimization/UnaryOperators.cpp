#include "UnaryOperators.h"
#include "BasicTypesHelper.h"
#include "Exceptions.h"
#include <limits>

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::ExpressionSimplifier;
using namespace OPS::ExpressionSimplifier::Helpers;

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Calculation
{

ExpressionBase* UnaryPlusOperator::calculate(OPS::Reprise::BasicCallExpression*& inputOperator)
{
	if (inputOperator->getKind() != BasicCallExpression::BCK_UNARY_PLUS || inputOperator->getArgumentCount() != 1) 
		throw OPS::Exception(string("UnaryPlusOperator::calculate Error!"));
	
	// (+expr) equals (expr)
	return inputOperator->getArgument(0).clone();
}

ExpressionBase* UnaryMinusOperator::calculate(OPS::Reprise::BasicCallExpression*& inputOperator)
{
	if (inputOperator->getKind() != BasicCallExpression::BCK_UNARY_MINUS || inputOperator->getArgumentCount() != 1) 
		throw OPS::Exception(string("UnaryMinusOperator::calculate Error!"));
	
	StrictLiteralExpression * leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));
	
	if (leftArgument && BasicTypesHelper::isNumeric(leftArgument)) 
	{
	
		BasicType::BasicTypes resultType = BasicTypesHelper::getResultTypeForUnaryOperation(leftArgument->getLiteralType());
		
		BasicTypesHelper::typeCast(leftArgument, resultType);		
		switch (resultType)
		{
			case (BasicType::BT_INT32): return StrictLiteralExpression::createInt32( -(leftArgument->getInt32()) );
			case (BasicType::BT_UINT32): return StrictLiteralExpression::createUInt32( -(leftArgument->getUInt32()) );
			case (BasicType::BT_INT64): return StrictLiteralExpression::createInt64( -(leftArgument->getInt64()) );
			case (BasicType::BT_UINT64): return StrictLiteralExpression::createUInt64( -(leftArgument->getUInt64()) );

			case (BasicType::BT_FLOAT32): return StrictLiteralExpression::createFloat32( -(leftArgument->getFloat32()) );
			case (BasicType::BT_FLOAT64): return StrictLiteralExpression::createFloat64( -(leftArgument->getFloat64()) );			
			default: throw OPS::Exception(string("BinaryMultiplyOperator::calculate Error!"));		
		} 										
	} 												
	return 0;
}

ExpressionBase* TypeCastOperator::calculate(TypeCastExpression*& inputTypeCastExpression)
{
	StrictLiteralExpression *inputLiteral = dynamic_cast<StrictLiteralExpression*>(&inputTypeCastExpression->getCastArgument());
	if (!inputLiteral) return 0; // оптимизируются только литералы	

	StrictLiteralExpression *result = inputLiteral->clone();
	BasicTypesHelper::typeCast(result, dynamic_cast<BasicType*>(&inputTypeCastExpression->getCastType()));

	return result;
}


}
}
}