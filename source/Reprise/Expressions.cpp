#include "Reprise/Expressions.h"

#include "Reprise/Exceptions.h"

#include "Reprise/ServiceFunctions.h"

#include <valarray>
#include <list>
#include <sstream>
#include <limits>
#include <locale>

//	TODO: Add checks in BasicLiteralExpression

//	Enter namespace
namespace OPS
{
namespace Reprise
{

//	ExpressionBase class implementation
ExpressionBase::ExpressionBase()
{
}

StatementBase* ExpressionBase::obtainParentStatement(void)
{
	ExpressionBase* current = this; 
	for (;;)
	{
		StatementBase* parentStmt = dynamic_cast<StatementBase*>(current->getParent());
		if (parentStmt != 0)
		{
			return parentStmt;
		}
		ExpressionBase* parentExpr = dynamic_cast<ExpressionBase*>(current->getParent());
		if (parentExpr != 0)
		{
			current = parentExpr;
		}
		else
		{
			return 0;
		}
	}
}

VariableDeclaration* ExpressionBase::obtainParentDeclaration(void)
{
	ExpressionBase* current = this; 
	for (;;)
	{
		VariableDeclaration* parentDecl = dynamic_cast<VariableDeclaration*>(current->getParent());
		if (parentDecl != 0)
		{
			return parentDecl;
		}
		ExpressionBase* parentExpr = dynamic_cast<ExpressionBase*>(current->getParent());
		if (parentExpr != 0)
		{
			current = parentExpr;
		}
		else
		{
			return 0;
		}
	}
}

ExpressionBase& ExpressionBase::obtainRootExpression(void)
{
	ExpressionBase* current = this; 
	for (;;)
	{
		ExpressionBase* parentExpr = dynamic_cast<ExpressionBase*>(current->getParent());
		if (parentExpr == 0)
		{
			return *current;
		}
		else
		{
			current = parentExpr;
		}
	}
}

ReprisePtr<TypeBase> ExpressionBase::getResultType(void) const
{
    return Editing::getExpressionType(*this);
}

//	LiteralExpression class implementation
LiteralExpression::LiteralExpression(void)
{
}

//	BasicLiteralExpression class implementation
BasicLiteralExpression::BasicLiteralExpression(void)
	: m_type(LT_UNDEFINED)
{
}

BasicLiteralExpression::BasicLiteralExpression(BasicLiteralExpression::LiteralTypes literalType)
	: m_type(literalType)
{
}

//		BasicLiteralExpression - Static methods
std::string BasicLiteralExpression::literalTypeToString(BasicLiteralExpression::LiteralTypes literalType, const bool shouldThrow)
{
	switch (literalType)
	{
	case LT_UNDEFINED:
		return "undefined";
	case LT_CHAR:
		return "char";
	case LT_WIDE_CHAR:
		return "wchar_t";
	case LT_INTEGER:
		return "integer";
	case LT_UNSIGNED_INTEGER:
		return "unsigned integer";
	case LT_FLOAT:
		return "float";
	case LT_BOOLEAN:
		return "boolean";
	case LT_STRING:
		return "string";
	case LT_WIDE_STRING:
		return "wstring";

	default:
		if (shouldThrow)
			throw RepriseError(Strings::format("Unexpected literal type (%u).", literalType));
		else
			return Strings::format("unexpected (%u).", literalType);
	}
}

BasicLiteralExpression* BasicLiteralExpression::createChar(const char value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_CHAR);
	newLiteral->setChar(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createWideChar(const wchar_t value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_WIDE_CHAR);
	newLiteral->setWideChar(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createInteger(const long_long_t value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_INTEGER);
	newLiteral->setInteger(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createUnsignedInteger(const unsigned_long_long_t value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_UNSIGNED_INTEGER);
	newLiteral->setUnsignedInteger(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createFloat(const double value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_FLOAT);
	newLiteral->setFloat(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createBoolean(const bool value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_BOOLEAN);
	newLiteral->setBoolean(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createString(const std::string& value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_STRING);
	newLiteral->setString(value);
	return newLiteral;
}

BasicLiteralExpression* BasicLiteralExpression::createWideString(const std::wstring& value)
{
	BasicLiteralExpression* newLiteral = new BasicLiteralExpression(LT_WIDE_STRING);
	newLiteral->setWideString(value);
	return newLiteral;
}

//	Methods
BasicLiteralExpression::LiteralTypes BasicLiteralExpression::getLiteralType(void) const
{
	return m_type;
}

std::string BasicLiteralExpression::getLiteralValueAsString(const bool shouldThrow) const
{
	switch (m_type)
	{
	case LT_UNDEFINED:
		return "(undefined)";
	case LT_CHAR:
		return Strings::format("'%c'", m_value.char_value);
	case LT_WIDE_CHAR:
		return Strings::format("L'%lc'", m_value.wide_char_value);
	case LT_INTEGER:
		return Strings::format("%" OPS_CRT_FORMAT_LONG_LONG_PREFIX "i", m_value.long_int_value);
	case LT_UNSIGNED_INTEGER:
		return Strings::format("%" OPS_CRT_FORMAT_LONG_LONG_PREFIX "u", m_value.long_unsigned_value);
	case LT_FLOAT:
		return Strings::format("%g", m_value.real_value);
	case LT_BOOLEAN:
		return m_value.bool_value ? "true" : "false";
	case LT_STRING:
		return Strings::format("\"%s\"", m_stringValue.c_str());
	case LT_WIDE_STRING:
		return Strings::format("L\"%ls\"", m_wideStringValue.c_str());

	default:
		if (shouldThrow)
			throw RepriseError(Strings::format("Unexpected literal type (%u).", m_type));
		else
			return Strings::format("unexpected (%u).", m_type);
	}
}


//		General getters
char BasicLiteralExpression::getChar(void) const
{
	if (m_type != LT_CHAR)
		throw UnexpectedLiteralError("Unexpected getting char value.");
	return m_value.char_value;
}

wchar_t BasicLiteralExpression::getWideChar(void) const
{
	return m_value.wide_char_value;
}

long_long_t BasicLiteralExpression::getInteger(void) const
{
	return m_value.long_int_value;
}

unsigned_long_long_t BasicLiteralExpression::getUnsignedInteger(void) const
{
	return m_value.long_unsigned_value;
}

double BasicLiteralExpression::getFloat(void) const
{
	return m_value.real_value;
}

bool BasicLiteralExpression::getBoolean(void) const
{
	return m_value.bool_value;
}

std::string BasicLiteralExpression::getString(void) const
{
	return m_stringValue;
}

std::wstring BasicLiteralExpression::getWideString(void) const
{
	return m_wideStringValue;
}

void BasicLiteralExpression::setChar(const char value)
{
	if (m_type == LT_CHAR || m_type == LT_UNDEFINED)
		m_value.char_value = value;
	else
		throw UnexpectedLiteralError("Unexpected setting char value.");
}

void BasicLiteralExpression::setWideChar(const wchar_t value)
{
	m_value.wide_char_value = value;
}

void BasicLiteralExpression::setInteger(const long_long_t value)
{
	m_value.long_int_value = value;
}

void BasicLiteralExpression::setUnsignedInteger(const unsigned_long_long_t value)
{
	m_value.long_unsigned_value = value;
}

void BasicLiteralExpression::setFloat(const double value)
{
	m_value.real_value = value;
}

void BasicLiteralExpression::setBoolean(const bool value)
{
	m_value.bool_value = value;
}

void BasicLiteralExpression::setString(const std::string& value)
{
	m_stringValue = value;
}

void BasicLiteralExpression::setWideString(const std::wstring& value)
{
	m_wideStringValue = value;
}

//		BasicLiteralExpression - ExpressionBase implementation
bool BasicLiteralExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<BasicLiteralExpression>())
	{
		const BasicLiteralExpression& other = exprNode.cast_to<BasicLiteralExpression>();
		if (other.m_type == m_type)
		{
			if (other.getLiteralValueAsString(false) == getLiteralValueAsString(false))
				return true;
		}
	}
	return false;
}

//		BasicLiteralExpression - RepriseBase implementation
int BasicLiteralExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& BasicLiteralExpression::getChild(int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("BasicLiteralExpression::getChild()");
}

std::string BasicLiteralExpression::dumpState(void) const
{
	std::string state = LiteralExpression::dumpState();
//	state += BasicLiteralExpression::literalTypeToString(m_type, false);
//	state += " ";
	state += getLiteralValueAsString(false);
	return state;
}

//	StrictLiteralExpression class implementation
StrictLiteralExpression::StrictLiteralExpression(void) 
	: m_type(BasicType::BT_UNDEFINED)
{
}

StrictLiteralExpression::StrictLiteralExpression(const BasicType::BasicTypes literalType)
	: m_type(literalType)
{
}

//		StrictLiteralExpression class - Static methods
#define OPS_IMPLEMENT_STRICT_LITERAL_CREATE(basicType, setter) \
	StrictLiteralExpression* newLiteral = new StrictLiteralExpression(basicType);\
	newLiteral->setter(value);\
	return newLiteral;

StrictLiteralExpression* StrictLiteralExpression::createChar(const char value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_CHAR, setChar)
}

StrictLiteralExpression* StrictLiteralExpression::createWideChar(const wchar_t value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_WIDE_CHAR, setWideChar)
}

StrictLiteralExpression* StrictLiteralExpression::createInt8(const sbyte value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_INT8, setInt8)
}

StrictLiteralExpression* StrictLiteralExpression::createInt16(const sword value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_INT16, setInt16)
}

StrictLiteralExpression* StrictLiteralExpression::createInt32(const sdword value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_INT32, setInt32)
}

StrictLiteralExpression* StrictLiteralExpression::createInt64(const sqword value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_INT64, setInt64)
}

StrictLiteralExpression* StrictLiteralExpression::createUInt8(const byte value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_UINT8, setUInt8)
}

StrictLiteralExpression* StrictLiteralExpression::createUInt16(const word value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_UINT16, setUInt16)
}

StrictLiteralExpression* StrictLiteralExpression::createUInt32(const dword value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_UINT32, setUInt32)
}

StrictLiteralExpression* StrictLiteralExpression::createUInt64(const qword value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_UINT64, setUInt64)
}


StrictLiteralExpression* StrictLiteralExpression::createFloat32(const float value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_FLOAT32, setFloat32)
}

StrictLiteralExpression* StrictLiteralExpression::createFloat64(const double value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_FLOAT64, setFloat64)
}

StrictLiteralExpression* StrictLiteralExpression::createBoolean(const bool value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_BOOLEAN, setBoolean)
}


StrictLiteralExpression* StrictLiteralExpression::createString(const std::string& value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_STRING, setString)
}

StrictLiteralExpression* StrictLiteralExpression::createWideString(const std::wstring& value)
{
	OPS_IMPLEMENT_STRICT_LITERAL_CREATE(BasicType::BT_WIDE_STRING, setWideString)
}


//		StrictLiteralExpression class - Static methods
BasicType::BasicTypes StrictLiteralExpression::getLiteralType(void) const
{
	return m_type;
}

std::string StrictLiteralExpression::getLiteralValueAsString(const bool shouldThrow) const
{
	std::string result;
	switch (m_type)
	{
		case BasicType::BT_CHAR:
			if (m_value.char_value >= 0x20)
				result = Strings::format("%c", m_value.char_value);
			else
				result = Strings::format("%i", (int)m_value.char_value);
			break;
		case BasicType::BT_WIDE_CHAR:
			if (m_value.wide_char_value >= 0x20)
				result = Strings::format("%lc", m_value.wide_char_value);
			else
				result = Strings::format("%i", (int)m_value.wide_char_value);
			break;
		case BasicType::BT_INT8:
			result = Strings::format("%i", m_value.sbyte_value);
			break;
		case BasicType::BT_INT16:
			result = Strings::format("%i", m_value.sword_value);
			break;
		case BasicType::BT_INT32:
			result = Strings::format("%i", m_value.sdword_value);
			break;
		case BasicType::BT_INT64:
			result = Strings::format("%" OPS_CRT_FORMAT_LONG_LONG_PREFIX "i", m_value.sqword_value);
			break;
		case BasicType::BT_UINT8:
			result = Strings::format("%u", m_value.byte_value);
			break;
		case BasicType::BT_UINT16:
			result = Strings::format("%u", m_value.word_value);
			break;
		case BasicType::BT_UINT32:
			result = Strings::format("%u", m_value.dword_value);
			break;
		case BasicType::BT_UINT64:
			result = Strings::format("%" OPS_CRT_FORMAT_LONG_LONG_PREFIX "u", m_value.qword_value);
			break;

		case BasicType::BT_FLOAT32:
		case BasicType::BT_FLOAT64:
		{
			std::ostringstream output;
			output.imbue(std::locale::classic());
			output.exceptions(std::ios::failbit | std::ios::badbit);
			output.setf(std::ios::boolalpha);
			output.precision(std::numeric_limits<double>::digits10 + 2);
			output << std::showpoint;

			if (m_type == BasicType::BT_FLOAT32)
			{
				output << m_value.float_value;
			}
			else if (m_type == BasicType::BT_FLOAT64)
			{
				output << m_value.double_value;
			}
			else
			{
				if (shouldThrow)
					throw RepriseError(Strings::format("Unexpected strict literal type (%u).", m_type));
				else
					return Strings::format("unexpected (%u).", m_type);
			}
			result = output.str();

			// remove trailing zeros
			if (result.find('.') != std::string::npos)
			{
				// the number has fractional part
				int pos = result.length() - 1;
				if (result[pos] == '0')
				{
					while(result[pos] == '0') pos--;
					result.resize(pos + 1);
				}
			}
			break;
		}

		case BasicType::BT_BOOLEAN:
			result = m_value.bool_value ? "true" : "false";
			break;

		case BasicType::BT_STRING:
			result = Strings::format("%s", m_stringValue.c_str());
			break;
		case BasicType::BT_WIDE_STRING:
			result = Strings::format("%ls", m_wideStringValue.c_str());
			break;
		default:
			if (shouldThrow)
				throw RepriseError(Strings::format("Unexpected strict literal type (%u).", m_type));
			else
				return Strings::format("unexpected (%u).", m_type);
	}

	return result;
}

std::string StrictLiteralExpression::stringToEscapedString(const std::string &src)
{
	static const char specialCharsRaw[12] = "\a\b\f\n\r\t\v\'\"\\\?";
	const std::string specialChars(specialCharsRaw);

	static const char* replacements[12] =
	{
		"\\a", "\\b", "\\f", "\\n", "\\r", "\\t", "\\v", "\\\'", "\\\"", "\\\\", "\\\?", NULL
	};

	std::string str;
	str.reserve(src.size()*2);

	for(std::string::const_iterator it = src.begin(); it < src.end(); ++it)
	{
		std::string::size_type idx = specialChars.find(*it);
		if (idx != std::string::npos)
			str.append(replacements[idx]);
		else
			str.append(1, *it);
	}
	return str;
}

std::string StrictLiteralExpression::stringToEscapedString(const std::wstring &wstr)
{
	std::stringstream ss;
	ss << std::hex;

	for(size_t i = 0; i < wstr.size(); ++i)
	{
		ss << "\\x" << wstr[i];
	}

	return ss.str();
}

bool StrictLiteralExpression::isCharacter(void) const
{
	switch (m_type)
	{
		case BasicType::BT_CHAR:
		case BasicType::BT_WIDE_CHAR:
			return true;
		default:
			return false;
	}
}

bool StrictLiteralExpression::isInteger(void) const
{
	switch (m_type)
	{
		case BasicType::BT_INT8:
		case BasicType::BT_INT16:
		case BasicType::BT_INT32:
		case BasicType::BT_INT64:
        case BasicType::BT_INT128:
		case BasicType::BT_UINT8:
		case BasicType::BT_UINT16:
		case BasicType::BT_UINT32:
		case BasicType::BT_UINT64:
        case BasicType::BT_UINT128:
		case BasicType::BT_BOOLEAN:
			return true;
		default:
			return false;
	}
}

bool StrictLiteralExpression::isFloat(void) const
{
	switch (m_type)
	{
		case BasicType::BT_FLOAT32:
		case BasicType::BT_FLOAT64:
			return true;
		default:
			return false;
	}
}

bool StrictLiteralExpression::isString(void) const
{
	switch (m_type)
	{
		case BasicType::BT_STRING:
		case BasicType::BT_WIDE_STRING:
			return true;
		default:
			return false;
	}
}


char StrictLiteralExpression::getChar(void) const
{
	return m_value.char_value;
}

wchar_t StrictLiteralExpression::getWideChar(void) const
{
	return m_value.wide_char_value;
}

sbyte StrictLiteralExpression::getInt8(void) const
{
	return m_value.sbyte_value;
}

sword StrictLiteralExpression::getInt16(void) const
{
	return m_value.sword_value;
}
sdword StrictLiteralExpression::getInt32(void) const
{
	return m_value.sdword_value;
}
sqword StrictLiteralExpression::getInt64(void) const
{
	return m_value.sqword_value;
}
byte StrictLiteralExpression::getUInt8(void) const
{
	return m_value.byte_value;
}
word StrictLiteralExpression::getUInt16(void) const
{
	return m_value.word_value;
}
dword StrictLiteralExpression::getUInt32(void) const
{
	return m_value.dword_value;
}
qword StrictLiteralExpression::getUInt64(void) const
{
	return m_value.qword_value;
}

float StrictLiteralExpression::getFloat32(void) const
{
	return m_value.float_value;
}
double StrictLiteralExpression::getFloat64(void) const
{
	return m_value.double_value;
}

bool StrictLiteralExpression::getBoolean(void) const
{
	return m_value.bool_value;
}
const std::string& StrictLiteralExpression::getString(void) const
{
	return m_stringValue;
}
const std::wstring& StrictLiteralExpression::getWideString(void) const
{
	return m_wideStringValue;
}


void StrictLiteralExpression::setChar(const char value)
{
	m_type = BasicType::BT_CHAR;
	m_value.char_value = value;
}

void StrictLiteralExpression::setWideChar(const wchar_t value)
{
	m_type = BasicType::BT_WIDE_CHAR;
	m_value.wide_char_value = value;
}

void StrictLiteralExpression::setInt8(const sbyte value)
{
	m_type = BasicType::BT_INT8;
	m_value.sbyte_value = value;
}

void StrictLiteralExpression::setInt16(const sword value)
{
	m_type = BasicType::BT_INT16;
	m_value.sword_value = value;
}

void StrictLiteralExpression::setInt32(const sdword value)
{
	m_type = BasicType::BT_INT32;
	m_value.sdword_value = value;
}

void StrictLiteralExpression::setInt64(const sqword value)
{
	m_type = BasicType::BT_INT64;
	m_value.sqword_value = value;
}

void StrictLiteralExpression::setUInt8(const byte value)
{
	m_type = BasicType::BT_UINT8;
	m_value.byte_value = value;
}

void StrictLiteralExpression::setUInt16(const word value)
{
	m_type = BasicType::BT_UINT16;
	m_value.word_value = value;
}

void StrictLiteralExpression::setUInt32(const dword value)
{
	m_type = BasicType::BT_UINT32;
	m_value.dword_value = value;
}

void StrictLiteralExpression::setUInt64(const qword value)
{
	m_type = BasicType::BT_UINT64;
	m_value.qword_value = value;
}


void StrictLiteralExpression::setFloat32(const float value)
{
	m_type = BasicType::BT_FLOAT32;
	m_value.float_value = value;
}

void StrictLiteralExpression::setFloat64(const double value)
{
	m_type = BasicType::BT_FLOAT64;
	m_value.double_value = value;
}


void StrictLiteralExpression::setBoolean(const bool value)
{
	m_type = BasicType::BT_BOOLEAN;
	m_value.bool_value = value;
}

void StrictLiteralExpression::setString(const std::string& value)
{
	m_type = BasicType::BT_STRING;
	m_stringValue = value;
}

void StrictLiteralExpression::setWideString(const std::wstring& value)
{
	m_type = BasicType::BT_WIDE_STRING;
	m_wideStringValue = value;
}

void StrictLiteralExpression::setOne()
{
	switch (m_type)
	{
		case BasicType::BT_CHAR:
			m_value.char_value = 1;
			break;
		case BasicType::BT_WIDE_CHAR:
			m_value.wide_char_value = 1;
			break;
		case BasicType::BT_INT8:
			m_value.sbyte_value = 1;
			break;
		case BasicType::BT_INT16:
			m_value.sword_value = 1;
			break;
		case BasicType::BT_INT32:
			m_value.sdword_value = 1;
			break;
		case BasicType::BT_INT64:
			m_value.sqword_value = 1;
			break;
		case BasicType::BT_UINT8:
			m_value.byte_value = 1;
			break;
		case BasicType::BT_UINT16:
			m_value.word_value = 1;
			break;
		case BasicType::BT_UINT32:
			m_value.dword_value = 1;
			break;
		case BasicType::BT_UINT64:
			m_value.qword_value = 1;
			break;

		case BasicType::BT_FLOAT32:
			m_value.float_value = 1.0;
			break;
		case BasicType::BT_FLOAT64:
			m_value.double_value = 1.0;
			break;

		case BasicType::BT_BOOLEAN:
			m_value.bool_value = true;
			break;

		default:
			throw RepriseError(Strings::format("Unexpected strict literal type (%u) for setting value 1.", m_type));
	}
}


void StrictLiteralExpression::setType(BasicType::BasicTypes type)
{
	m_type = type;
}

/*
void StrictLiteralExpression::convertToChar(char value)
{
	switch (m_type)
	{
		case BasicType::BT_CHAR:
			break;
		case BasicType::BT_WIDE_CHAR:
			m_value.wide_char_value = value;
			break;
		case BasicType::BT_INT8:
			m_value.sbyte_value = static_cast<sbyte>(value);
			break;
		case BasicType::BT_INT16:
			m_value.sword_value = static_cast<sword>(value);
			break;
		case BasicType::BT_INT32:
			m_value.sdword_value = static_cast<sdword>(value);
			break;
		case BasicType::BT_INT64:
			m_value.sqword_value = static_cast<sqword>(value);
			break;
		case BasicType::BT_UINT8:
			m_value.byte_value = static_cast<byte>(value);
			break;
		case BasicType::BT_UINT16:
			m_value.word_value = static_cast<word>(value);
			break;
		case BasicType::BT_UINT32:
			m_value.dword_value = static_cast<dword>(value);
			break;
		case BasicType::BT_UINT64:
			m_value.qword_value = static_cast<qword>(value);
			break;

		case BasicType::BT_FLOAT32:
			m_value.float_value = static_cast<float>(value);
			break;
		case BasicType::BT_FLOAT64:
			m_value.double_value = static_cast<double>(value);
			break;

		case BasicType::BT_BOOLEAN:
			m_value.bool_value = value == 0 ? false : true;
			break;

		case BasicType::BT_STRING:
			throw RepriseError("Unexpected convert from char type to string type.");
			break;
		case BasicType::BT_WIDE_STRING:
			throw RepriseError("Unexpected convert from char type to wide string type.");
			break;
		default:
			throw RepriseError(Strings::format("Unexpected strict literal type (%u).", m_type));
	}
	m_type = BasicType::BT_CHAR;
}
*/


//		StrictLiteralExpression class - ExpressionBase implementation
bool StrictLiteralExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<StrictLiteralExpression>())
	{
		const StrictLiteralExpression& other = exprNode.cast_to<StrictLiteralExpression>();
		if (other.m_type == m_type)
		{
			if (other.getLiteralValueAsString(false) == getLiteralValueAsString(false))
				return true;
		}
	}
	return false;
}


//		StrictLiteralExpression class - RepriseBase implementation
int StrictLiteralExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& StrictLiteralExpression::getChild(int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("StrictLiteralExpression::getChild()");
}

std::string StrictLiteralExpression::dumpState(void) const
{
	std::string state = LiteralExpression::dumpState();
	state += BasicType::basicTypeToString(m_type, false);
	state += " ";
	state += getLiteralValueAsString(false);
	return state;
}


//	CompoundLiteralExpression class implementation
CompoundLiteralExpression::CompoundLiteralExpression(void)
{
}

CompoundLiteralExpression::CompoundLiteralExpression(const CompoundLiteralExpression& other) : 
	LiteralExpression(other)
{ 
	for (ElementsType::ConstIterator iter = other.m_elements.begin(); iter != other.m_elements.end(); ++iter)
	{
		addValue((*iter)->clone());
	}
}

CompoundLiteralExpression::~CompoundLiteralExpression(void)
{
}

int CompoundLiteralExpression::getValueCount(void) const
{
	return static_cast<int>(m_elements.size());
}

const ExpressionBase& CompoundLiteralExpression::getValue(const int index) const
{
	if (index < 0 || index >= getValueCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
			index, getValueCount()));
	return m_elements[index];
}

ExpressionBase& CompoundLiteralExpression::getValue(const int index)
{
	if (index < 0 || index >= getValueCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
		index, getValueCount()));
	return m_elements[index];
}

void CompoundLiteralExpression::addValue(ExpressionBase* const value)
{
	m_elements.add(value).setParent(this);
}

void CompoundLiteralExpression::insertValue(const int index, ExpressionBase* const value)
{
	if (index < 0 || index >= getValueCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
		index, getValueCount()));
	m_elements.insert(index, value).setParent(this);
}

void CompoundLiteralExpression::replaceValue(ExpressionBase& oldValue, ReprisePtr<ExpressionBase> value)
{
	for (ElementsType::Iterator it = m_elements.begin(); it != m_elements.end(); ++it)
	{
		if (*it == &oldValue)
		{
			m_elements.replace(it, value.get());
			(*it)->setParent(this);
		}
	}
}

//		ExpressionBase implementation
bool CompoundLiteralExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<CompoundLiteralExpression>())
	{
		const CompoundLiteralExpression& other = exprNode.cast_to<CompoundLiteralExpression>();
		if (other.m_elements.size() == m_elements.size())
		{
			for (int index = 0; index < m_elements.size(); ++index)
			{
				if (!m_elements[index].isEqual(other.m_elements[index]))
					return false;
			}
			return true;
		}
	}
	return false;
}

//		CompoundLiteralExpression class - RepriseBase implementation
int CompoundLiteralExpression::getChildCount(void) const
{
	return getValueCount();
}

RepriseBase& CompoundLiteralExpression::getChild(const int index)
{
	return getValue(index);
}

std::string CompoundLiteralExpression::dumpState(void) const
{
	std::string state = LiteralExpression::dumpState();
	state += "{ ";
	if (getValueCount() > 0)
	{
	}
	for (ElementsType::ConstIterator it = m_elements.begin(); it != m_elements.end(); ++it)
	{
		state += (it != m_elements.begin() ? ", " : "") + (*it)->dumpState();
	}
	state += "\n}";
	return state;
}

//	ReferenceExpression class implementation
ReferenceExpression::ReferenceExpression(VariableDeclaration& declaration)
	: m_declaration(&declaration)
{
}

const VariableDeclaration& ReferenceExpression::getReference(void) const
{
	return *m_declaration;
}

VariableDeclaration& ReferenceExpression::getReference(void)
{
	return *m_declaration;
}

void ReferenceExpression::setReference(VariableDeclaration* reference)
{
	OPS_ASSERT(reference != 0)
	m_declaration.reset(reference);
}

//		ExpressionBase implementation
bool ReferenceExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<ReferenceExpression>())
	{
		const ReferenceExpression& other = exprNode.cast_to<ReferenceExpression>();
		if (other.m_declaration.get() == m_declaration.get())
			return true;
	}
	return false;
}

//		ReferenceExpression - RepriseBase implementation
int ReferenceExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& ReferenceExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	throw RepriseError("ReferenceExpression::getChild()");
}

int ReferenceExpression::getLinkCount(void) const
{
	return 1;
}

RepriseBase& ReferenceExpression::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("ReferenceExpression::getLink()");
	OPS_ASSERT(m_declaration.get() != 0)
	return *m_declaration;
}

std::string ReferenceExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += m_declaration->getName();
	return state;
}

//	SubroutineReferenceExpression class implementation
SubroutineReferenceExpression::SubroutineReferenceExpression(SubroutineDeclaration& declaration)
	: m_declaration(&declaration)
{
}

const SubroutineDeclaration& SubroutineReferenceExpression::getReference(void) const
{
	return *m_declaration;
}

SubroutineDeclaration& SubroutineReferenceExpression::getReference(void)
{
	return *m_declaration;
}

void SubroutineReferenceExpression::setReference(SubroutineDeclaration* newSubroutine)
{
	OPS_ASSERT(newSubroutine != 0)
	m_declaration.reset(newSubroutine);
}

//		ExpressionBase implementation
bool SubroutineReferenceExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<SubroutineReferenceExpression>())
	{
		const SubroutineReferenceExpression& other = exprNode.cast_to<SubroutineReferenceExpression>();
		if (other.m_declaration.get() == m_declaration.get())
			return true;
	}
	return false;
}

//		SubroutineReferenceExpression - RepriseBase implementation
int SubroutineReferenceExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& SubroutineReferenceExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	throw RepriseError("SubroutineReferenceExpression::getChild()");
}

int SubroutineReferenceExpression::getLinkCount(void) const
{
	return 1;
}

RepriseBase& SubroutineReferenceExpression::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("SubroutineReferenceExpression::getLink()");
	OPS_ASSERT(m_declaration.get() != 0)
	return *m_declaration;
}

std::string SubroutineReferenceExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += m_declaration->getName();
	return state;
}

//	StructAccessExpression class implementation
StructAccessExpression::StructAccessExpression(ExpressionBase& structPointer, StructMemberDescriptor& member)
	: m_structPointer(&structPointer), m_member(&member)
{
	m_structPointer->setParent(this);
}

StructAccessExpression::StructAccessExpression(const StructAccessExpression& other) : ExpressionBase(other),
	m_structPointer(other.m_structPointer->clone()), m_member(other.m_member)
{
	m_structPointer->setParent(this);
}

const ExpressionBase& StructAccessExpression::getStructPointerExpression(void) const
{
	return *m_structPointer;
}

ExpressionBase& StructAccessExpression::getStructPointerExpression(void)
{
	return *m_structPointer;
}

void StructAccessExpression::setStructPointerExpression(ReprisePtr<ExpressionBase> expression)
{
	m_structPointer = expression;
	m_structPointer->setParent(this);
}

const StructMemberDescriptor& StructAccessExpression::getMember(void) const
{
	return *m_member;
}

StructMemberDescriptor& StructAccessExpression::getMember(void)
{
	return *m_member;
}

void StructAccessExpression::setMember(StructMemberDescriptor* member)
{
	OPS_ASSERT(member != 0)
	m_member.reset(member);
}


//		ExpressionBase implementation
bool StructAccessExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<StructAccessExpression>())
	{
		const StructAccessExpression& other = exprNode.cast_to<StructAccessExpression>();
        if (other.m_structPointer->isEqual(*m_structPointer) && other.m_member.get() == m_member.get())
			return true;
	}
	return false;
}

//		StructAccessExpression - RepriseBase implementation
int StructAccessExpression::getChildCount(void) const
{
	return 1;
}

RepriseBase& StructAccessExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	return *m_structPointer;
}

int StructAccessExpression::getLinkCount(void) const
{
	return 1;
}

RepriseBase& StructAccessExpression::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("StructAccessExpression::getLink()");
	OPS_ASSERT(m_member.get() != 0)
	return *m_member;
}

std::string StructAccessExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += m_structPointer->dumpState() + "." + m_member->getName();
	return state;
}

//	EnumAccessExpression class implementation
EnumAccessExpression::EnumAccessExpression(EnumMemberDescriptor& member)
: m_member(&member)
{
}

const EnumMemberDescriptor& EnumAccessExpression::getMember(void) const
{
	return *m_member;
}

EnumMemberDescriptor& EnumAccessExpression::getMember(void)
{
	return *m_member;
}

void EnumAccessExpression::setMember(EnumMemberDescriptor* member)
{
	OPS_ASSERT(member != 0)
	m_member.reset(member);
}

//		ExpressionBase implementation
bool EnumAccessExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<EnumAccessExpression>())
	{
		const EnumAccessExpression& other = exprNode.cast_to<EnumAccessExpression>();
		if (other.m_member.get() == m_member.get())
			return true;
	}
	return false;
}

//		EnumAccessExpression - RepriseBase implementation
int EnumAccessExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& EnumAccessExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	throw RepriseError("SubroutineReferenceExpression::getChild()");
}

int EnumAccessExpression::getLinkCount(void) const
{
	return 1;
}

RepriseBase& EnumAccessExpression::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("EnumAccessExpression::getLink()");
	OPS_ASSERT(m_member.get() != 0)
	return *m_member;
}

std::string EnumAccessExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += m_member->getName();
	return state;
}

//	TypeCastExpression class implementation
TypeCastExpression::TypeCastExpression(TypeBase* cast_to, ExpressionBase* argument, const bool implicit)
	: m_cast_to(cast_to), m_argument(argument), m_implicit(implicit)
{
	m_cast_to->setParent(this);
	m_argument->setParent(this);
}

TypeCastExpression::TypeCastExpression(const TypeCastExpression& other) : ExpressionBase(other), 
	m_cast_to(other.m_cast_to->clone()), m_argument(other.m_argument->clone()), m_implicit(other.m_implicit)
{
	m_cast_to->setParent(this);
	m_argument->setParent(this);
}

const TypeBase& TypeCastExpression::getCastType(void) const
{
	return *m_cast_to;
}

TypeBase& TypeCastExpression::getCastType(void)
{
	return *m_cast_to;
}

void TypeCastExpression::setCastType(TypeBase* typeBase)
{
	m_cast_to.reset(typeBase);
	m_cast_to->setParent(this);
}

const ExpressionBase& TypeCastExpression::getCastArgument(void) const
{
	return *m_argument;
}

ExpressionBase& TypeCastExpression::getCastArgument(void)
{
	return *m_argument;
}

void TypeCastExpression::setCastArgument(ReprisePtr<ExpressionBase> argumentExpression)
{
	m_argument = argumentExpression;
	m_argument->setParent(this);
}

bool TypeCastExpression::isImplicit(void) const
{
	return m_implicit;
}

void TypeCastExpression::setImplicit(const bool implicit)
{
	m_implicit = implicit;
}

//		ExpressionBase implementation
bool TypeCastExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<TypeCastExpression>())
	{
		const TypeCastExpression& other = exprNode.cast_to<TypeCastExpression>();
		if (other.m_cast_to.get() == m_cast_to.get() && other.m_argument.get() == m_argument.get() && other.m_implicit == m_implicit)
			return true;
	}
	return false;
}

//		TypeCastExpression - RepriseBase implementation
int TypeCastExpression::getChildCount(void) const
{
	return 2;
}

RepriseBase& TypeCastExpression::getChild(const int index)
{
	if (index == 0)
		return *m_cast_to;
	else
	if (index == 1)
		return *m_argument;
	else
		throw UnexpectedChildError(Strings::format(
			"Unexpected getting of child (%i) at TypeCastExpression::getChild().", index));
}

std::string TypeCastExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += "((" + m_cast_to->dumpState() + ")" + m_argument->dumpState() + ")";
	return state;
}

//	CallExpressionBase class implementation
CallExpressionBase::CallExpressionBase(void)
{
}

CallExpressionBase::~CallExpressionBase(void)
{
}

CallExpressionBase::CallExpressionBase(const CallExpressionBase& other) : ExpressionBase(other)
{
	for (ArgumentsType::const_iterator it = other.m_arguments.begin(); it != other.m_arguments.end(); ++it)
	{
		addArgument((*it)->clone());
	}
}

int CallExpressionBase::getArgumentCount(void) const
{
	return static_cast<int>(m_arguments.size());
}

const ExpressionBase& CallExpressionBase::getArgument(const int index) const
{
	if (index < 0 || index >= getArgumentCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
		index, getArgumentCount()));
	return *m_arguments[index];
}

ExpressionBase& CallExpressionBase::getArgument(const int index)
{
	if (index < 0 || index >= getArgumentCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
		index, getArgumentCount()));
	return *m_arguments[index];
}

void CallExpressionBase::addArgument(ExpressionBase* const argument)
{
	m_arguments.push_back(ReprisePtr<ExpressionBase>(argument));
	argument->setParent(this);
}

void CallExpressionBase::insertArgument(const int index, ExpressionBase* const argument)
{
	if (index < 0 || index >= getArgumentCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
		index, getArgumentCount()));
	m_arguments.insert(m_arguments.begin() + index, ReprisePtr<ExpressionBase>(argument));
	argument->setParent(this);
}

void CallExpressionBase::setArgument(const int index, ExpressionBase* const argument)
{
	if (index < 0 || index >= getArgumentCount())
		throw RepriseError(Strings::format("Setting unexpected value (%i), value count (%i).",
		index, getArgumentCount()));
	m_arguments[index] = ReprisePtr<ExpressionBase>(argument);
	argument->setParent(this);
}

void CallExpressionBase::replaceArgument(ExpressionBase& sourceArgument, ReprisePtr<ExpressionBase> destinationArgument)
{
	for (ArgumentsType::iterator argument = m_arguments.begin(); argument != m_arguments.end(); ++argument)
	{
		if (argument->get() == &sourceArgument)
		{
			*argument = destinationArgument;
			(*argument)->setParent(this);
			return;
		}
	}
	throw RepriseError(Strings::format("Unexpected replace argument (%p) in CallExpressionBase.", &sourceArgument));
}

void CallExpressionBase::removeArguments()
{
	m_arguments.clear();
}

//		ExpressionBase implementation
bool CallExpressionBase::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<CallExpressionBase>())
	{
		const CallExpressionBase& other = exprNode.cast_to<CallExpressionBase>();
		if (other.m_arguments.size() == m_arguments.size())
		{
			for (size_t index = 0; index < m_arguments.size(); ++index)
			{
				if (!other.m_arguments[index]->isEqual(*m_arguments[index]))
					return false;
			}
			return true;
		}
	}
	return false;
}

std::string CallExpressionBase::dumpStateHelper(const std::string& format) const
{
	//std::valarray<const char*> valargs;
	//std::list<std::string> tempStrings;
	//valargs.resize(m_arguments.size());
	//for (size_t arg = 0; arg < m_arguments.size(); ++arg)
	//{
	//	tempStrings.push_front(m_arguments[arg]->dumpState());
	//	valargs[arg] = tempStrings.begin()->c_str();
	//}
	//return Strings::format(format.c_str(), &valargs[0]);
	switch (m_arguments.size())
	{
	case 0:
		return "";
	case 1:
		return Strings::format(format.c_str(), m_arguments[0]->dumpState().c_str());
	case 2:
		return Strings::format(format.c_str(), 
			m_arguments[0]->dumpState().c_str(), 
			m_arguments[1]->dumpState().c_str());
	case 3:
		return Strings::format(format.c_str(), 
			m_arguments[0]->dumpState().c_str(), 
			m_arguments[1]->dumpState().c_str(), 
			m_arguments[2]->dumpState().c_str());
	case 4:
		return Strings::format(format.c_str(), 
			m_arguments[0]->dumpState().c_str(), 
			m_arguments[1]->dumpState().c_str(), 
			m_arguments[2]->dumpState().c_str(), 
			m_arguments[3]->dumpState().c_str());
	case 5:
		return Strings::format(format.c_str(), 
			m_arguments[0]->dumpState().c_str(), 
			m_arguments[1]->dumpState().c_str(), 
			m_arguments[2]->dumpState().c_str(), 
			m_arguments[3]->dumpState().c_str(), 
			m_arguments[4]->dumpState().c_str());
		OPS_DEFAULT_CASE_LABEL
	}
	return "";
}

//	CallExpressionBase class implementation
std::string BasicCallExpression::builtinCallKindToString(BasicCallExpression::BuiltinCallKind kind)
{
	switch (kind)
	{
		//	Unary
	case BCK_UNARY_PLUS:
		return "+";
	case BCK_UNARY_MINUS:
		return "-";

	case BCK_SIZE_OF:			// sizeof() operator
		return "sizeof()";

	case BCK_TAKE_ADDRESS:		// &
		return "&";
	case BCK_DE_REFERENCE:		// *
		return "*";

			//	Binary
	case BCK_BINARY_PLUS:		// +
		return "+";
	case BCK_BINARY_MINUS:		// -
		return "-";
	case BCK_MULTIPLY:			// *
		return "*";
	case BCK_DIVISION:			// / 
		return "/";
	case BCK_INTEGER_DIVISION:	// div
		return "div";
	case BCK_INTEGER_MOD:		// mod (%)
		return "mod";

			//	Assign
	case BCK_ASSIGN:				// =
		return "=";

			//	Equality
	case BCK_LESS:				// <
		return "<";
	case BCK_GREATER:			// >
		return ">";
	case BCK_LESS_EQUAL:		// <=
		return "<=";
	case BCK_GREATER_EQUAL:		// >=
		return ">=";
	case BCK_EQUAL:				// ==
		return "==";
	case BCK_NOT_EQUAL:			// !=
		return "!=";

			//	Shifts
	case BCK_LEFT_SHIFT:			// <<
		return "<<";
	case BCK_RIGHT_SHIFT:		// >>
		return ">>";

			//	Logical
	case BCK_LOGICAL_NOT:		// !
		return "!";
	case BCK_LOGICAL_AND:		// &&
		return "&&";
	case BCK_LOGICAL_OR:			// ||
		return "||";

			//	Bitwise
	case BCK_BITWISE_NOT:		// ~
		return "~";
	case BCK_BITWISE_AND:		// &
		return "&";
	case BCK_BITWISE_OR:			// |
		return "|";
	case BCK_BITWISE_XOR:		// ^
		return "^";
		//	Special
	case BCK_ARRAY_ACCESS:		// []
		return "[]";
	case BCK_COMMA:
		return ",";
	case BCK_CONDITIONAL:
		return "?:";

		OPS_DEFAULT_CASE_LABEL
	}
	return "(unexpected) BasicCallExpression::builtinCallKindToString";
}

std::string BasicCallExpression::dumpKindFormat(BuiltinCallKind kind)
{
	switch (kind)
	{
		//	Unary
	case BCK_UNARY_PLUS:
		return "+(%s)";
	case BCK_UNARY_MINUS:
		return "-(%s)";

	case BCK_SIZE_OF:			// sizeof() operator
		return "sizeof(%s)";

	case BCK_TAKE_ADDRESS:		// &
		return "&(%s)";
	case BCK_DE_REFERENCE:		// *
		return "*(%s)";

			//	Binary
	case BCK_BINARY_PLUS:		// +
		return "(%s)+(%s)";
	case BCK_BINARY_MINUS:		// -
		return "(%s)-(%s)";
	case BCK_MULTIPLY:			// *
		return "(%s)*(%s)";
	case BCK_DIVISION:			// / 
		return "(%s)/(%s)";
	case BCK_INTEGER_DIVISION:	// div
		return "(%s)div(%s)";
	case BCK_INTEGER_MOD:		// mod (%)
		return "(%s)mod(%s)";

			//	Assign
	case BCK_ASSIGN:				// =
		return "(%s)=(%s)";

			//	Equality
	case BCK_LESS:				// <
		return "(%s)<(%s)";
	case BCK_GREATER:			// >
		return "(%s)>(%s)";
	case BCK_LESS_EQUAL:		// <=
		return "(%s)<=(%s)";
	case BCK_GREATER_EQUAL:		// >=
		return "(%s)>=(%s)";
	case BCK_EQUAL:				// ==
		return "(%s)==(%s)";
	case BCK_NOT_EQUAL:			// !=
		return "(%s)!=(%s)";

			//	Shifts
	case BCK_LEFT_SHIFT:			// <<
		return "(%s)<<(%s)";
	case BCK_RIGHT_SHIFT:		// >>
		return "(%s)>>(%s)";

			//	Logical
	case BCK_LOGICAL_NOT:		// !
		return "!(%s)";
	case BCK_LOGICAL_AND:		// &&
		return "(%s)&&(%s)";
	case BCK_LOGICAL_OR:			// ||
		return "(%s)||(%s)";

			//	Bitwise
	case BCK_BITWISE_NOT:		// ~
		return "~(%s)";
	case BCK_BITWISE_AND:		// &
		return "(%s)&(%s)";
	case BCK_BITWISE_OR:			// |
		return "(%s)|(%s)";
	case BCK_BITWISE_XOR:		// ^
		return "(%s)^(%s)";
		//	Special
	case BCK_ARRAY_ACCESS:		// []
		return "(%s)[(%s)]";
	case BCK_COMMA:
		return "(%s, %s)";
	case BCK_CONDITIONAL:
		return "(%s)?(%s):(%s)";

		OPS_DEFAULT_CASE_LABEL
	}
	return "(unexpected) BasicCallExpression::builtinCallKindToString";
}

BasicCallExpression::BasicCallExpression(const BasicCallExpression::BuiltinCallKind builtinCall)
	: m_builtinCall(builtinCall)
{
}

BasicCallExpression::BasicCallExpression(BuiltinCallKind builtinCall, ExpressionBase* argument)
	: m_builtinCall(builtinCall)
{
	addArgument(argument);
}

BasicCallExpression::BasicCallExpression(BuiltinCallKind builtinCall, ExpressionBase* leftArg, ExpressionBase* rightArg)
	: m_builtinCall(builtinCall)
{
	addArgument(leftArg);
	addArgument(rightArg);
}

BasicCallExpression::BuiltinCallKind BasicCallExpression::getKind(void) const
{
	return m_builtinCall;
}

void BasicCallExpression::setKind(BuiltinCallKind kind)
{
	m_builtinCall = kind;
}

//		ExpressionBase implementation
bool BasicCallExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<BasicCallExpression>())
	{
		const BasicCallExpression& other = exprNode.cast_to<BasicCallExpression>();
		if (other.m_builtinCall == m_builtinCall && CallExpressionBase::isEqual(other))
		{
			return true;
		}
	}
	return false;
}

//		BasicCallExpression - RepriseBase implementation
int BasicCallExpression::getChildCount(void) const
{
	return getArgumentCount();
}

RepriseBase& BasicCallExpression::getChild(const int index)
{
	return getArgument(index);
}

std::string BasicCallExpression::dumpState(void) const
{
	std::string state = CallExpressionBase::dumpState();
	if (m_builtinCall != BCK_ARRAY_ACCESS)
		state += dumpStateHelper(dumpKindFormat(m_builtinCall));
	else
	{
		state += getArgument(0).dumpState() + "[";
		for(int i = 1; i < getArgumentCount(); ++i)
		{
			if (i != 1)
				state += ",";
			state += getArgument(i).dumpState();
		}
		state += "]";
	}
	return state;
}

//	SubroutineCallExpression class implementation
SubroutineCallExpression::SubroutineCallExpression(ExpressionBase* const callExpression) 
	: m_callExpression(callExpression)
{
	OPS_ASSERT(callExpression != 0);
	m_callExpression->setParent(this);
}

SubroutineCallExpression::SubroutineCallExpression(const SubroutineCallExpression& other) : CallExpressionBase(other),
	m_callExpression(other.m_callExpression->clone())
{
	m_callExpression->setParent(this);
}


const ExpressionBase& SubroutineCallExpression::getCallExpression(void) const
{
	return *m_callExpression;
}

ExpressionBase& SubroutineCallExpression::getCallExpression(void)
{
	return *m_callExpression;
}

void SubroutineCallExpression::setCallExpression(ReprisePtr<ExpressionBase> callExpression)
{
	m_callExpression = callExpression;
	m_callExpression->setParent(this);
}

bool SubroutineCallExpression::hasExplicitSubroutineDeclaration(void) const
{
	if (m_callExpression.get() != 0)
		if (m_callExpression->is_a<SubroutineReferenceExpression>())
		{
			if (dynamic_cast<const SubroutineDeclaration*>(&m_callExpression->cast_to<SubroutineReferenceExpression>().getReference()) != 0)
				return true;
		}
	return false;
}

const SubroutineDeclaration& SubroutineCallExpression::getExplicitSubroutineDeclaration(void) const
{
	if (hasExplicitSubroutineDeclaration())
	{
		return *dynamic_cast<const SubroutineDeclaration*>(&m_callExpression->cast_to<SubroutineReferenceExpression>().getReference());
	}
	else
		throw StateError("Unexpected getting of subroutine declaration.");
}

SubroutineDeclaration& SubroutineCallExpression::getExplicitSubroutineDeclaration(void)
{
	if (hasExplicitSubroutineDeclaration())
	{
		return *dynamic_cast<SubroutineDeclaration*>(&m_callExpression->cast_to<SubroutineReferenceExpression>().getReference());
	}
	else
		throw StateError("Unexpected getting of subroutine declaration.");
}

//		ExpressionBase implementation
bool SubroutineCallExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<SubroutineCallExpression>())
	{
		const SubroutineCallExpression& other = exprNode.cast_to<SubroutineCallExpression>();
		if (other.m_callExpression.get() == m_callExpression.get() && CallExpressionBase::isEqual(other))
		{
			return true;
		}
	}
	return false;
}

//	SubroutineCallExpression - RepriseBase implementation
int SubroutineCallExpression::getChildCount(void) const
{
	return getArgumentCount() + 1;
}

RepriseBase& SubroutineCallExpression::getChild(int index)
{
	if (index < 0 || index >= getChildCount())
		throw RepriseError(Strings::format("Getting unexpected value (%i), value count (%i).",
			index, getChildCount()));
	if (index == 0)
		return *m_callExpression;
	else
		return getArgument(index - 1);
}

std::string SubroutineCallExpression::dumpState(void) const
{
	std::string state = CallExpressionBase::dumpState();
	state += "(" + m_callExpression->dumpState() + ")(";
	for (ArgumentsType::const_iterator arg = m_arguments.begin(); arg != m_arguments.end(); ++arg)
	{
		state += ((arg != m_arguments.begin()) ? "," : "") + (*arg)->dumpState();
	}
	state += ")";
	return state;
}


//	EmptyExpression class implementation
EmptyExpression::EmptyExpression(void)
{
}

EmptyExpression* EmptyExpression::empty(void)
{
	return new EmptyExpression();
}

//		ExpressionBase implementation
bool EmptyExpression::isEqual(const ExpressionBase& exprNode) const
{
	if (exprNode.is_a<EmptyExpression>())
	{
		return true;
	}
	return false;
}

//		EmptyExpression - RepriseBase implementation
int EmptyExpression::getChildCount(void) const
{
	return 0;
}

RepriseBase& EmptyExpression::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("EmptyExpression::getChild()");
}

std::string EmptyExpression::dumpState(void) const
{
	std::string state = ExpressionBase::dumpState();
	state += "(Empty expression)";
	return state;
}


}
}
