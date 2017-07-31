#include <limits>
#include "Exceptions.h"
#include "BinaryOperators.h"
#include "BasicTypesHelper.h"
#include "LiteralTypesLists.h"
#include "Accessors.h"


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

static void swapArguments(BasicCallExpression *expr)
{
	OPS_ASSERT(expr->getArgumentCount() == 2);
	ReprisePtr<ExpressionBase> rightArg(&expr->getArgument(1));
	expr->replaceArgument(expr->getArgument(1), ReprisePtr<ExpressionBase>(&expr->getArgument(0)));
	expr->replaceArgument(expr->getArgument(0), rightArg);	
}

#define CalculateAndCreateStrictLiteral(LiteralsType, literal0, literal1, op, creationMethod)\
    StrictLiteralExpression::creationMethod(GETTER(BasicType::LiteralsType, literal0) op GETTER(BasicType::LiteralsType, literal1))

ExpressionBase* BinaryPlusOperator::calculate(BasicCallExpression*& inputOperator) 
{	
	if (inputOperator->getKind() != BasicCallExpression::BCK_BINARY_PLUS || inputOperator->getArgumentCount() != 2) 
		throw OPS::Exception(string("BinaryPlusOperator::calculate Error!"));
	
	StrictLiteralExpression * leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));
	StrictLiteralExpression * rightArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(1));

	if (leftArgument && rightArgument) 
	{
		if (BasicTypesHelper::isNumeric(leftArgument) && BasicTypesHelper::isNumeric(rightArgument))
		{			
			BasicType::BasicTypes resultType = BasicTypesHelper::getResultTypeForBinaryOperation(leftArgument->getLiteralType(), rightArgument->getLiteralType());
			
			BasicTypesHelper::typeCast(leftArgument, resultType);
			BasicTypesHelper::typeCast(rightArgument, resultType);
			switch (resultType)
			{
				case (BasicType::BT_INT32): return CalculateAndCreateStrictLiteral(BT_INT32, leftArgument, rightArgument, +, createInt32);
				case (BasicType::BT_UINT32): return CalculateAndCreateStrictLiteral(BT_UINT32, leftArgument, rightArgument, +, createUInt32);
				case (BasicType::BT_INT64): return CalculateAndCreateStrictLiteral(BT_INT64, leftArgument, rightArgument, +, createInt64);
				case (BasicType::BT_UINT64): return CalculateAndCreateStrictLiteral(BT_UINT64, leftArgument, rightArgument, +, createUInt64);
				case (BasicType::BT_FLOAT32): return CalculateAndCreateStrictLiteral(BT_FLOAT32, leftArgument, rightArgument, +, createFloat32);
				case (BasicType::BT_FLOAT64): return CalculateAndCreateStrictLiteral(BT_FLOAT64, leftArgument, rightArgument, +, createFloat64);
				default: throw OPS::Exception(string("BinaryPlusOperator::calculate Error!"));
			}
		}	
	} else
	{
		if (!leftArgument)
		{
			if (!rightArgument) return 0;
			swapArguments(inputOperator);
			leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));
		}
		if (BasicTypesHelper::isFloat(leftArgument)) return 0;
		if (leftArgument && BasicTypesHelper::isLiteralEqualToSpecificConstant(0, leftArgument))
			return inputOperator->getArgument(1).clone();
	}
	return 0;
}

ExpressionBase* BinaryMinusOperator::calculate(BasicCallExpression*& inputOperator) 
{	
	if (inputOperator->getKind() != BasicCallExpression::BCK_BINARY_MINUS || inputOperator->getArgumentCount() != 2) 
		throw OPS::Exception(string("BinaryMinusOperator::calculate Error!"));
	
	StrictLiteralExpression * leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));
	StrictLiteralExpression * rightArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(1));

	if (leftArgument && rightArgument) 
	{
		if (BasicTypesHelper::isNumeric(leftArgument) && BasicTypesHelper::isNumeric(rightArgument))
		{			
			BasicType::BasicTypes resultType = BasicTypesHelper::getResultTypeForBinaryOperation(leftArgument->getLiteralType(), rightArgument->getLiteralType());
			
			BasicTypesHelper::typeCast(leftArgument, resultType);
			BasicTypesHelper::typeCast(rightArgument, resultType);
			switch (resultType)
			{
				case (BasicType::BT_INT32): return CalculateAndCreateStrictLiteral(BT_INT32, leftArgument, rightArgument, -, createInt32);
				case (BasicType::BT_UINT32): return CalculateAndCreateStrictLiteral(BT_UINT32, leftArgument, rightArgument, -, createUInt32);
				case (BasicType::BT_INT64): return CalculateAndCreateStrictLiteral(BT_INT64, leftArgument, rightArgument, -, createInt64);
				case (BasicType::BT_UINT64): return CalculateAndCreateStrictLiteral(BT_UINT64, leftArgument, rightArgument, -, createUInt64);
				case (BasicType::BT_FLOAT32): return CalculateAndCreateStrictLiteral(BT_FLOAT32, leftArgument, rightArgument, -, createFloat32);
				case (BasicType::BT_FLOAT64): return CalculateAndCreateStrictLiteral(BT_FLOAT64, leftArgument, rightArgument, -, createFloat64);
				default: throw OPS::Exception(string("BinaryMinusOperator::calculate Error!"));		
			}								
		}	
	} else
	{
		if (!leftArgument) 
		{
			if (!rightArgument) return 0;
			if (BasicTypesHelper::isInteger(rightArgument) && BasicTypesHelper::isLiteralEqualToSpecificConstant(0, rightArgument))
				return leftArgument;
			return 0;
		}
	}																	
	return 0;
}

ExpressionBase* MultiplyOperator::calculate(BasicCallExpression*& inputOperator) 
{	
	if (inputOperator->getKind() != BasicCallExpression::BCK_MULTIPLY || inputOperator->getArgumentCount() != 2) 
		throw OPS::Exception(string("BinaryMultiplyOperator::calculate Error!"));
	
	StrictLiteralExpression * leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));
	StrictLiteralExpression * rightArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(1));

	if (leftArgument && rightArgument) 
	{
		if (BasicTypesHelper::isNumeric(leftArgument) && BasicTypesHelper::isNumeric(rightArgument))
		{			
			BasicType::BasicTypes resultType = BasicTypesHelper::getResultTypeForBinaryOperation(leftArgument->getLiteralType(), rightArgument->getLiteralType());
			
			BasicTypesHelper::typeCast(leftArgument, resultType);
			BasicTypesHelper::typeCast(rightArgument, resultType);
			switch (resultType)
			{
				case (BasicType::BT_INT32): return CalculateAndCreateStrictLiteral(BT_INT32, leftArgument, rightArgument, *, createInt32);
				case (BasicType::BT_UINT32): return CalculateAndCreateStrictLiteral(BT_UINT32, leftArgument, rightArgument, *, createUInt32);
				case (BasicType::BT_INT64): return CalculateAndCreateStrictLiteral(BT_INT64, leftArgument, rightArgument, *, createInt64);
				case (BasicType::BT_UINT64): return CalculateAndCreateStrictLiteral(BT_UINT64, leftArgument, rightArgument, *, createUInt64);
				case (BasicType::BT_FLOAT32): return CalculateAndCreateStrictLiteral(BT_FLOAT32, leftArgument, rightArgument, *, createFloat32);
				case (BasicType::BT_FLOAT64): return CalculateAndCreateStrictLiteral(BT_FLOAT64, leftArgument, rightArgument, *, createFloat64);
				default: throw OPS::Exception(string("BinaryMultiplyOperator::calculate Error!"));		
			}								
		}	
	} else
	{
		if (!leftArgument)
		{
			if (!rightArgument) return 0;
			swapArguments(inputOperator);
			leftArgument = dynamic_cast<StrictLiteralExpression*>(&inputOperator->getArgument(0));			
		}
		if (BasicTypesHelper::isFloat(leftArgument)) return 0;
		if (leftArgument && BasicTypesHelper::isLiteralEqualToSpecificConstant(1, leftArgument))
			return inputOperator->getArgument(1).clone();
		if (leftArgument && BasicTypesHelper::isLiteralEqualToSpecificConstant(0, leftArgument))
			return StrictLiteralExpression::createInt32(0);
	}																	
	return 0;
}

}
}
}