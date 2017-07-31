#include "Reprise/Canto/HirCTypes.h"
#include "Reprise/Exceptions.h"
#include "Reprise/Expressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{


HirCBasicType::HirCBasicType(const HirCBasicKind basicKind) : m_kind(basicKind)
{
}

HirCBasicType::HirCBasicKind HirCBasicType::getKind(void) const
{
	return m_kind;
}

int HirCBasicType::getSizeOf(void) const
{
	if (m_kind == HCBK_LONG_LONG)
		return 8;
	else
	if (m_kind == HCBK_CHAR)
		return 1;
	else
		return -1;
}

HirCBasicType* HirCBasicType::clone(void) const
{
	// TODO: Make this in proper way
	HirCBasicType* result = new HirCBasicType(m_kind);
	result->setVolatile(isVolatile());
	result->setConst(isConst());
	return result;
}

//		RepriseBase implementation
int HirCBasicType::getChildCount(void) const
{
	return 0;
}

RepriseBase& HirCBasicType::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("HirCBasicType::getChild()");
}

std::string HirCBasicType::dumpState(void) const
{
	return "[HirCBasicType]" + TypeBase::dumpState() + HirCBasicType::basicKindToString(m_kind, false);
}

bool HirCBasicType::isEqual(const TypeBase &type) const
{
	if (const HirCBasicType* other = type.cast_ptr<HirCBasicType>())
	{
		if (m_kind == other->m_kind)
			return TypeBase::isEqual(type);
	}
	return false;
}

std::string HirCBasicType::basicKindToString(HirCBasicKind basicKind, const bool shouldThrow)
{
	std::string state;
	switch (basicKind)
	{
	case HCBK_CHAR:
		state = "char";
		break;
	case HCBK_WIDE_CHAR:
		state = "wchar_t";
		break;
	case HCBK_SCHAR:
		state = "signed char";
		break;
	case HCBK_SHORT:
		state = "short";
		break;
	case HCBK_INT:
		state = "int";
		break;
	case HCBK_LONG:
		state = "long";
		break;
	case HCBK_LONG_LONG:
		state = "long long";
		break;
    case HCBK_INT128:
        state = "__int128";
        break;
	case HCBK_UCHAR:
		state = "unsigned char";
		break;
	case HCBK_USHORT:
		state = "unsigned short";
		break;
	case HCBK_UINT:
		state = "unsigned int";
		break;
	case HCBK_ULONG:
		state = "unsigned long";
		break;
	case HCBK_ULONG_LONG:
		state = "unsigned long long";
		break;
    case HCBK_UINT128:
        state = "unsigned __int128";
        break;

	case HCBK_FLOAT:
		state = "float";
		break;
	case HCBK_DOUBLE:
		state = "double";
		break;
	case HCBK_LONG_DOUBLE:
		state = "long double";
		break;

	case HCBK_BOOL:
		state = "_Bool";
		break;

	case HCBK_VOID:
		state = "void";
		break;

	default:
		if (shouldThrow)
			throw RepriseError(Strings::format("Unexpected basic type (%u).", basicKind));
		else
			return Strings::format("unexpected (%u).", basicKind);
	}
	return state;
}


}
}
}
