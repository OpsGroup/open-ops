#include <map>
#include <limits>
#include "BasicTypesHelper.h"
#include "LiteralTypesLists.h"
#include "Reprise/Reprise.h"
#include "Accessors.h"
#include "Exceptions.h"


using namespace OPS::Reprise;
using namespace std;

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Helpers
{
std::map<BasicType::BasicTypes, int> BasicTypesHelper::ranks;	
bool BasicTypesHelper::initialized = false;

void BasicTypesHelper::initializeStaticData()
{			
	ranks[BasicType::BT_BOOLEAN] = 1;
	ranks[BasicType::BT_CHAR] = 2;
	
	ranks[BasicType::BT_INT8] = 3;
	ranks[BasicType::BT_UINT8] = 3;
	
	ranks[BasicType::BT_INT16] = 4;
	ranks[BasicType::BT_UINT16] = 4;

	ranks[BasicType::BT_INT32] = 5;
	ranks[BasicType::BT_UINT32] = 5;

	ranks[BasicType::BT_INT64] = 6;
	ranks[BasicType::BT_UINT64] = 6;
	BasicTypesHelper::initialized = true;
}	

bool BasicTypesHelper::isFloat(BasicType::BasicTypes inputLiteralType) 
{	
	return (inputLiteralType == BasicType::BT_FLOAT32 || inputLiteralType == BasicType::BT_FLOAT64);
}

bool BasicTypesHelper::isFloat(const OPS::Reprise::StrictLiteralExpression *inputLiteral) 
{	
	return isFloat(inputLiteral->getLiteralType());
}

bool BasicTypesHelper::isInteger(BasicType::BasicTypes inputLiteralType) 
{
	return (!isFloat(inputLiteralType) && !isString(inputLiteralType));
}

bool BasicTypesHelper::isInteger(const StrictLiteralExpression *inputLiteral) 
{
	return isInteger(inputLiteral->getLiteralType());
}

bool BasicTypesHelper::isNumeric(BasicType::BasicTypes inputLiteralType) 
{
	return (!isString(inputLiteralType));
}

bool BasicTypesHelper::isNumeric(const StrictLiteralExpression *inputLiteral) 
{
	return isNumeric(inputLiteral->getLiteralType());
	
}

bool BasicTypesHelper::isString(BasicType::BasicTypes inputLiteralType) 
{	
	return (inputLiteralType == BasicType::BT_STRING || inputLiteralType == BasicType::BT_WIDE_STRING);
}

bool BasicTypesHelper::isString(const StrictLiteralExpression *inputLiteral) 
{
	return isString(inputLiteral->getLiteralType());
}	

bool BasicTypesHelper::isSigned(BasicType::BasicTypes inputLiteralType)
{
	return (inputLiteralType == BasicType::BT_FLOAT32 ||
			inputLiteralType == BasicType::BT_FLOAT64 ||
			inputLiteralType == BasicType::BT_INT8    ||
			inputLiteralType == BasicType::BT_INT16   ||
			inputLiteralType == BasicType::BT_INT32   ||	
			inputLiteralType == BasicType::BT_INT64);
}

bool BasicTypesHelper::isSigned(const StrictLiteralExpression *inputLiteral)
{
	return isSigned(inputLiteral->getLiteralType());
}

BasicType::BasicTypes BasicTypesHelper::getMinimumTypeForSpecificConstant(long long int value)
{	
#define isValueBelongsToSpecificTypeRange(bscType, value) \
	numeric_limits<TypeByLabel<LiteralTypesList, bscType>::Result >::min() <= value \
	&& numeric_limits<TypeByLabel<LiteralTypesList, bscType>::Result >::max() >= value
		

	if (isValueBelongsToSpecificTypeRange(BasicType::BT_CHAR, value)) return BasicType::BT_CHAR;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_INT8, value)) return BasicType::BT_INT8;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_UINT8, value)) return BasicType::BT_UINT8;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_INT16, value)) return BasicType::BT_INT16;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_UINT16, value)) return BasicType::BT_UINT16;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_INT32, value)) return BasicType::BT_INT32;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_UINT32, value)) return BasicType::BT_UINT32;
	if (isValueBelongsToSpecificTypeRange(BasicType::BT_INT64, value)) return BasicType::BT_INT64;
	return BasicType::BT_UINT64;	
}

BasicType::BasicTypes BasicTypesHelper::getMinimumTypeForSpecificConstant(long double value)
{
	if (numeric_limits<float>::min() <= value && numeric_limits<float>::max() >= value)
		return BasicType::BT_FLOAT32;
	return BasicType::BT_FLOAT64;
}

int BasicTypesHelper::getRankOfNumericType(BasicType::BasicTypes type)
{
	if (!initialized) BasicTypesHelper::initializeStaticData();
	return ranks[type];	
}

bool BasicTypesHelper::isLiteralEqualToSpecificConstant(int number, const StrictLiteralExpression *expression)
{	
	switch (expression->getLiteralType())
	{
		case (BasicType::BT_CHAR): return number == GETTER(BasicType::BT_CHAR, expression);
		case (BasicType::BT_BOOLEAN): return (number != 0) == GETTER(BasicType::BT_BOOLEAN, expression);

		case (BasicType::BT_INT8): return number == GETTER(BasicType::BT_INT8, expression);
		case (BasicType::BT_INT16): return number == GETTER(BasicType::BT_INT16, expression);
		case (BasicType::BT_INT32): return number == GETTER(BasicType::BT_INT32, expression);
		case (BasicType::BT_INT64): return number == GETTER(BasicType::BT_INT64, expression);

		case (BasicType::BT_UINT8): return number == GETTER(BasicType::BT_UINT8, expression);
		case (BasicType::BT_UINT16): return number == GETTER(BasicType::BT_UINT16, expression);
		case (BasicType::BT_UINT32): return number == GETTER(BasicType::BT_UINT32, expression);
		case (BasicType::BT_UINT64): return number == GETTER(BasicType::BT_UINT64, expression);

		case (BasicType::BT_FLOAT32): return abs(GETTER(BasicType::BT_FLOAT32, expression) - number) < numeric_limits<float>::epsilon();  
		case (BasicType::BT_FLOAT64): return abs(GETTER(BasicType::BT_FLOAT32, expression) - number) < numeric_limits<double>::epsilon();  
		default: throw OPS::ExpressionSimplifier::ArgumentError(
					 string("Error was occured in isLiteralEqualToSpecificConstant method! Argument can't be compared with specific number!"));	
	};	
}

void BasicTypesHelper::typeCast(StrictLiteralExpression* inputLiteral, BasicType::BasicTypes type_to)
{	
	BasicType::BasicTypes type_from = inputLiteral->getLiteralType();

	if (type_to == type_from) return;
		
	// надо изучить правила преобразования строковых литералов 
	if (BasicTypesHelper::isString(type_from) || BasicTypesHelper::isString(type_to))
		throw OPS::ExpressionSimplifier::ArgumentError(string("TypeCastOperator::calculate Error!")); 
	
	
	OPS_ASSERT(BasicTypesHelper::isNumeric(type_from) && BasicTypesHelper::isNumeric(type_to));

#define TRY_CONVERT_TYPE(typelabel, type, getMethod)\
	if (type_from == typelabel)\
	{\
        type oldValue = inputLiteral->getMethod();\
		switch(type_to)\
		{\
		case (BasicType::BT_BOOLEAN): { inputLiteral->setBoolean(oldValue != 0); return; break; }\
		case (BasicType::BT_CHAR): { inputLiteral->setChar(static_cast<char>(oldValue));  return; break; }\
		case (BasicType::BT_WIDE_CHAR): { inputLiteral->setWideChar(static_cast<wchar_t>(oldValue)); return; break;}\
		case (BasicType::BT_FLOAT32): { inputLiteral->setFloat32(static_cast<float>(oldValue)); return; break; }\
		case (BasicType::BT_FLOAT64): { inputLiteral->setFloat64(static_cast<double>(oldValue)); return; break; }\
		case (BasicType::BT_INT8): { inputLiteral->setInt8(static_cast<sbyte>(oldValue)); return; break; }\
		case (BasicType::BT_INT16): { inputLiteral->setInt16(static_cast<sword>(oldValue)); return; break; }\
		case (BasicType::BT_INT32): { inputLiteral->setInt32(static_cast<sdword>(oldValue)); return; break; }\
		case (BasicType::BT_INT64): { inputLiteral->setInt64(static_cast<sqword>(oldValue)); return; break; }\
		case (BasicType::BT_UINT8): { inputLiteral->setUInt8(static_cast<byte>(oldValue)); return; break; }\
		case (BasicType::BT_UINT16): { inputLiteral->setUInt16(static_cast<word>(oldValue)); return; break; }\
		case (BasicType::BT_UINT32): { inputLiteral->setUInt32(static_cast<dword>(oldValue)); return; break; }\
		case (BasicType::BT_UINT64): { inputLiteral->setUInt64(static_cast<qword>(oldValue)); return; break; }\
		default: throw OPS::ExpressionSimplifier::ArgumentError(string("TypeCastOperator::calculate Error!"));\
		}\
	}

	TRY_CONVERT_TYPE(BasicType::BT_BOOLEAN, bool, getBoolean);
	
	TRY_CONVERT_TYPE(BasicType::BT_CHAR, char, getChar);
	TRY_CONVERT_TYPE(BasicType::BT_WIDE_CHAR, wchar_t, getWideChar);

	TRY_CONVERT_TYPE(BasicType::BT_FLOAT32, float, getFloat32);
	TRY_CONVERT_TYPE(BasicType::BT_FLOAT64, double, getFloat64);

	TRY_CONVERT_TYPE(BasicType::BT_INT8, sbyte, getInt8);
	TRY_CONVERT_TYPE(BasicType::BT_INT16, sword, getInt16);
	TRY_CONVERT_TYPE(BasicType::BT_INT32, sdword, getInt32);
	TRY_CONVERT_TYPE(BasicType::BT_INT64, sqword, getInt64);

	TRY_CONVERT_TYPE(BasicType::BT_UINT8, byte, getUInt8);
	TRY_CONVERT_TYPE(BasicType::BT_UINT16, word, getUInt16);
	TRY_CONVERT_TYPE(BasicType::BT_UINT32, dword, getUInt32);
	TRY_CONVERT_TYPE(BasicType::BT_UINT64, qword, getUInt64);
	
	throw OPS::ExpressionSimplifier::ArgumentError(string("TypeCastOperator::calculate Error!"));
}

void BasicTypesHelper::typeCast(StrictLiteralExpression* inputLiteral, const BasicType *resultType)
{
	return BasicTypesHelper::typeCast(inputLiteral, resultType->getKind());
}


// для арифметических типов
BasicType::BasicTypes BasicTypesHelper::getResultTypeForUnaryOperation(BasicType::BasicTypes type)
{
	OPS_ASSERT(BasicTypesHelper::isNumeric(type));

	if (type == BasicType::BT_FLOAT64) return BasicType::BT_FLOAT64;
	if (type == BasicType::BT_FLOAT32) return BasicType::BT_FLOAT32;
	
	/* Integer promotions */
	OPS_ASSERT(BasicTypesHelper::isInteger(type));
	
	int rankOfInt = BasicTypesHelper::getRankOfNumericType(BasicType::BT_INT32);
	if (BasicTypesHelper::getRankOfNumericType(type) < rankOfInt)
		type = BasicType::BT_INT32;

	return type;	
}
BasicType::BasicTypes BasicTypesHelper::getResultTypeForBinaryOperation(BasicType::BasicTypes leftType, BasicType::BasicTypes rightType)
{
	OPS_ASSERT(BasicTypesHelper::isNumeric(leftType));
	OPS_ASSERT(BasicTypesHelper::isNumeric(rightType));

	if (leftType == BasicType::BT_FLOAT64 || rightType == BasicType::BT_FLOAT64)
		return BasicType::BT_FLOAT64;

	if (leftType == BasicType::BT_FLOAT32 || rightType == BasicType::BT_FLOAT32)
		return BasicType::BT_FLOAT64;

	/* Integer promotions */
	OPS_ASSERT(BasicTypesHelper::isInteger(leftType) && BasicTypesHelper::isInteger(rightType));
	
	int rankOfInt = BasicTypesHelper::getRankOfNumericType(BasicType::BT_INT32);
	if (BasicTypesHelper::getRankOfNumericType(leftType) < rankOfInt)
		leftType = INTEGER_TYPE;
	if (BasicTypesHelper::getRankOfNumericType(rightType) < rankOfInt)
		rightType = INTEGER_TYPE;
	
	
	if (leftType != rightType)
	{
		if (BasicTypesHelper::isSigned(leftType) == BasicTypesHelper::isSigned(rightType))
		{
			if (BasicTypesHelper::getRankOfNumericType(leftType) <
				BasicTypesHelper::getRankOfNumericType(rightType)) leftType = rightType;
			else rightType = leftType;
		}
		else
		{
			if (!BasicTypesHelper::isSigned(leftType))
			{
				if (BasicTypesHelper::getRankOfNumericType(leftType) >= BasicTypesHelper::getRankOfNumericType(rightType))
					rightType = leftType;
				else leftType = rightType;
			} else
			{
				if (BasicTypesHelper::getRankOfNumericType(leftType) < BasicTypesHelper::getRankOfNumericType(rightType))
					leftType = rightType;
				else rightType = leftType;
			}
		}
	}
	return leftType;			
}


}
}
}
