#ifndef EXPRESSION_OPTIMIZATION_BASIC_TYPES_HELPER_H
#define EXPRESSION_OPTIMIZATION_BASIC_TYPES_HELPER_H

// TODO: надо изучить правила преобразования строковых литералов (typeCast)

#include <map>
#include "Reprise/Reprise.h"
#include "LiteralTypesLists.h"
#include "Accessors.h"

namespace OPS
{
namespace ExpressionSimplifier
{
namespace Helpers
{
using OPS::Reprise::BasicType;
using OPS::Reprise::StrictLiteralExpression;

	
/* Platform dependent(used in integer promotions) */
#define INTEGER_TYPE BasicType::BT_INT32 
#define UNSIGNED_INTEGER_TYPE BasicType::BT_UINT32

class BasicTypesHelper
{
	static std::map<BasicType::BasicTypes, int> ranks;	

	static bool initialized;
	static void initializeStaticData();
public:	
	BasicTypesHelper() 
	{
		if (!initialized) BasicTypesHelper::initializeStaticData();
	}

	static bool isFloat(BasicType::BasicTypes inputType);
	static bool isFloat(const StrictLiteralExpression *inputLiteral);

	static bool isInteger(BasicType::BasicTypes inputType);
	static bool isInteger(const StrictLiteralExpression *inputLiteral);
	
	static bool isNumeric(BasicType::BasicTypes inputType);		
	static bool isNumeric(const StrictLiteralExpression *inputLiteral);		
	
	static bool isString(BasicType::BasicTypes inputType);		
	static bool isString(const StrictLiteralExpression *inputLiteral);

	static bool isSigned(BasicType::BasicTypes inputType);
	static bool isSigned(const StrictLiteralExpression *inputLiteral);

	// getValueAs приводит значение литерала к выбранному арифметическому типу и возвращает его
	template<int ResultTypeLabel>
	static typename TypeByLabel<LiteralTypesList, ResultTypeLabel>::Result 
		getValueAs(const StrictLiteralExpression *inputLiteral)
	{
		OPS_ASSERT(isNumeric(inputLiteral));
        typedef typename TypeByLabel<LiteralTypesList, ResultTypeLabel>::Result ResultType;

#define CASE_BY_LITERAL_TYPE_RETURN_VALUE(bscType, inputLiteral) case (bscType): return (ResultType)GETTER(bscType , inputLiteral);
		
		switch(inputLiteral->getLiteralType())
		{
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_BOOLEAN, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_CHAR, inputLiteral);				
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_WIDE_CHAR, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_FLOAT32, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_FLOAT64, inputLiteral);			
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_INT8, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_INT16, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_INT32, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_INT64, inputLiteral);				
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_UINT8, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_UINT16, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_UINT32, inputLiteral);
			CASE_BY_LITERAL_TYPE_RETURN_VALUE(BasicType::BT_UINT64, inputLiteral);				
			default: throw OPS::Exception(std::string("Bad literal was passed to getValueAsFloat function"));
		}
	}

	static BasicType::BasicTypes getMinimumTypeForSpecificConstant(long long int value);
	static BasicType::BasicTypes getMinimumTypeForSpecificConstant(long double value);

	static int getRankOfNumericType(BasicType::BasicTypes type);		
	static bool isLiteralEqualToSpecificConstant(int number, const StrictLiteralExpression *expression);

	// Standard C conversion for BinaryOperation
	static BasicType::BasicTypes getResultTypeForUnaryOperation(BasicType::BasicTypes type);
	static BasicType::BasicTypes getResultTypeForBinaryOperation(BasicType::BasicTypes leftType, BasicType::BasicTypes rightType);

	// C type conversion 
	static void typeCast(StrictLiteralExpression *inputLiteral, BasicType::BasicTypes type_to);
	static void typeCast(StrictLiteralExpression *inputLiteral, const BasicType *type_to);
};

}
}
}

#endif 
