#include "Reprise/Canto/HirCUtils.h"

#include <ctype.h>

namespace OPS
{
namespace Reprise
{
namespace Canto
{
enum IntegerSuffix
{
	IS_ERROR,
	IS_NONE,
	IS_U,
	IS_L,
	IS_UL,
	IS_LL,
	IS_ULL
};

static IntegerSuffix parseIntegerSuffix(const std::string& integerConstant);
static unsigned decodeDigit(char ch);

// Black arts with sing arithmetic. Disabling warning. This code needs review.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4146)
#endif
ReprisePtr<StrictLiteralExpression> HirCdecodeLiteral(const std::string& literal_arg)
{
	std::string literal(literal_arg);
	ReprisePtr<StrictLiteralExpression> result(new StrictLiteralExpression());
	if (literal.empty())
		return result;
	if (literal.find('e') != std::string::npos ||
		literal.find('E') != std::string::npos ||
		literal.find('p') != std::string::npos ||
		literal.find('P') != std::string::npos ||
		literal.find('.') != std::string::npos)
	//	Floating point constant
	{

	}
	else
	{
		bool negative = false;
		if (literal[0] == '-')
		{
			negative = true;
			literal = literal.substr(1);
			if (literal.empty())
				return result;
		}
		IntegerSuffix suffix = parseIntegerSuffix(literal);
		if (suffix == IS_ERROR)
			return result;
		for (;;)
		{
			if (literal.empty())
				break;
			if (toupper(literal[literal.size() - 1]) == 'U' || toupper(literal[literal.size() - 1]) == 'L')
			{
				literal.erase(literal.size() - 1);
				continue;
			}
			break;
		}
		unsigned long long decimal_value = 0;
		unsigned base = 0;
		std::string decimal = literal;
		if (literal[0] == '0')
		{
			// Octal or hex
			if (literal.size() >= 2 && (literal[1] == 'x' || literal[1] == 'X'))
			{
				//	hex
				decimal = literal.substr(2);
				base = 16;
			}
			else
			{
				//	oct
				decimal = literal.substr(1);
				base = 8;
			}
		}
		else
		{
			decimal = literal;
			base = 10;
		}
		for (size_t index = 0; index < decimal.size(); ++index)
		{
			if (decodeDigit(decimal[index]) >= base)
				return result;
		}

		for (;;)
		{
			if (decimal.empty())
				break;
			if (!isdigit(decimal[0]))
				return result;
			const unsigned digit = decodeDigit(decimal[0]);
			if (digit == 0xFF)
				return result;
			unsigned long long old_decimal_value = decimal_value;
			decimal_value = decimal_value * base + digit;
			if (old_decimal_value > decimal_value)
				return result;
			decimal = decimal.substr(1);
		}

		switch (suffix)
		{
		case IS_NONE:
		case IS_L:
			if (negative)
			{
                if (decimal_value <= 2147483648u)
					result->setInt32(-static_cast<sdword>(decimal_value));
				else
					result->setInt64(-static_cast<sqword>(decimal_value));
				return result;
			}
			if (decimal_value <= 2147483647)
				result->setInt32(static_cast<sdword>(decimal_value));
			else
				result->setInt64(static_cast<sqword>(decimal_value));
			return result;
		case IS_U:
		case IS_UL:
			if (negative)
			{
                if (decimal_value <= 2147483648u)
					result->setUInt32(-static_cast<dword>(decimal_value));
				else
					result->setUInt64(-decimal_value);
				return result;
			}
			if (decimal_value <= 2147483647)
				result->setUInt32(static_cast<dword>(decimal_value));
			else
				result->setUInt64(decimal_value);
			return result;
		case IS_LL:
			if (negative)
				result->setInt64(-decimal_value);
			else
				result->setInt64(decimal_value);
			return result;
		case IS_ULL:
			if (negative)
				result->setUInt64(-decimal_value);
			else
				result->setUInt64(decimal_value);
			return result;
			OPS_DEFAULT_CASE_LABEL
		}
	}
	return result;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif


//	Local functions
IntegerSuffix parseIntegerSuffix(const std::string& integerConstant)
{
	std::string integerSuffix = integerConstant; 
	for (;;)
	{
		if (integerSuffix.empty())
			return IS_NONE;
		if (isdigit(integerSuffix[0]) || toupper(integerSuffix[0]) == 'X')
			integerSuffix = integerSuffix.substr(1);
		else
			break;
	}
	bool suf_u = false;
	bool suf_l = false;
	bool suf_ll = false;
	if (toupper(integerSuffix[0]) == 'U')
	{
		suf_u = true;
		integerSuffix = integerSuffix.substr(1);
		if (integerSuffix.size() == 2 && (integerSuffix == "ll" || integerSuffix == "LL"))
		{
			suf_ll = true;
			integerSuffix.substr(2);
		}
		else
		if (integerSuffix.size() == 1 && toupper(integerSuffix[0]) == 'L')
		{
			suf_l = true;
			integerSuffix.substr(1);
		}
		if (integerSuffix.empty())
		{
			if (suf_l)
				return IS_UL;
			if (suf_ll)
				return IS_ULL;
			return IS_U;
		}
		else
			return IS_ERROR;
	}
	if (toupper(integerSuffix[0]) == 'L')
	{
		if (integerSuffix.size() >= 2 && toupper(integerSuffix[1]) == 'L')
		{
			suf_ll = true;
			integerSuffix = integerSuffix.substr(2);
			if (integerSuffix.empty())
				return IS_LL;
			if (toupper(integerSuffix[0]) == 'U')
				return IS_ULL;
			return IS_ERROR;
		}
		else
		{
			if (integerSuffix.size() == 1)
				return IS_L;
			if (integerSuffix.size() == 2 && toupper(integerSuffix[1]) == 'U')
				return IS_UL;
			return IS_ERROR;
		}
	}
	return IS_ERROR;	
}

unsigned decodeDigit(const char ch)
{
	switch (toupper(ch))
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return ch - '0';
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return ch - 'A' + 10;
	default:
		return 0xFF;
	}
}

}
}
}
