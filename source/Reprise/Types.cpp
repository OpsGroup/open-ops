#include "Reprise/Types.h"

#include "Reprise/Exceptions.h"
#include "Reprise/Declarations.h"
#include "Reprise/Expressions.h"

//	TODO: Add incomplete TypeStruct check
//	TODO: Add incomplete TypeEnum check

//	Enter namespace
namespace OPS
{
namespace Reprise
{
//	Constants
static const int INCOMPLETE_ARRAY_SIZE = 0;
static const int VAR_SIZE_ARRAY_SIZE = -1;

static const int NO_BITS_LIMIT = 0;
static const int VAR_BITS_LIMIT = -1;


//	TypeBase class implementation
TypeBase::TypeBase(void) : m_const(false), m_volatile(false)
{
}

TypeBase::TypeBase(const TypeBase& typeBase)
	: RepriseBase(typeBase), m_const(typeBase.m_const), m_volatile(typeBase.m_volatile)
{
	setParent(typeBase.getParent());
}

bool TypeBase::isConst(void) const
{
	return m_const;
}

bool TypeBase::isVolatile(void) const
{
	return m_volatile;
}

void TypeBase::setConst(const bool constQualified)
{
	m_const = constQualified;
}

void TypeBase::setVolatile(const bool volatileQualified)
{
	m_volatile = volatileQualified;
}

bool TypeBase::isFullType(void) const
{
	return true;
}

bool TypeBase::isEqual(const TypeBase &type) const
{
	return type.m_const == m_const && type.m_volatile == m_volatile;
}

//		TypeBase - RepriseBase implementation
std::string TypeBase::dumpState(void) const
{
	std::string state = RepriseBase::dumpState();
	if (m_const)
		state += "const ";
	if (m_volatile)
		state += "volatile ";
	return state;
}

BasicTypeBase::BasicTypeBase()
{
}

BasicTypeBase::BasicTypeBase(const BasicTypeBase& other) : TypeBase(other)
{
}


//	BasicType class implementation
BasicType::BasicType(BasicType::BasicTypes kind) : m_kind(kind)
{
}

BasicType::BasicType(const BasicType& basicType) : BasicTypeBase(basicType),
	m_kind(basicType.m_kind)
{
}

/*
#define IMPLEMENT_BASIC_TYPE_RETURN(x) \
	static BasicType temp(x);\
	return &temp;
*/
#define IMPLEMENT_BASIC_TYPE_RETURN(x) return new BasicType(x);


//		BasicType - static methods
BasicType* BasicType::basicType(const BasicTypes type)
{
	IMPLEMENT_BASIC_TYPE_RETURN(type)
}

BasicType* BasicType::charType(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_CHAR)
}

BasicType* BasicType::wideCharType(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_WIDE_CHAR)
}

BasicType* BasicType::int8Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_INT8)
}

BasicType* BasicType::int16Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_INT16)
}

BasicType* BasicType::int32Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_INT32)
}

BasicType* BasicType::int64Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_INT64)
}

BasicType* BasicType::int128Type(void)
{
		IMPLEMENT_BASIC_TYPE_RETURN(BT_INT128)
}

BasicType* BasicType::uint8Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_UINT8)
}

BasicType* BasicType::uint16Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_UINT16)
}

BasicType* BasicType::uint32Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_UINT32)
}

BasicType* BasicType::uint64Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_UINT64)
}

BasicType* BasicType::uint128Type(void)
{
		IMPLEMENT_BASIC_TYPE_RETURN(BT_UINT128)
}

BasicType* BasicType::float32Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_FLOAT32)
}

BasicType* BasicType::float64Type(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_FLOAT64)
}


BasicType* BasicType::booleanType(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_BOOLEAN)
}

BasicType* BasicType::voidType(void)
{
	IMPLEMENT_BASIC_TYPE_RETURN(BT_VOID)
}


BasicType::BasicTypes BasicType::getKind(void) const
{
	return m_kind;
}

int BasicType::getSizeOf(void) const
{
	switch (m_kind)
	{
	case BT_CHAR:
		return 1;
	case BT_WIDE_CHAR:
		return 2;
	case BT_INT8:
	case BT_UINT8:
		return 1;
	case BT_INT16:
	case BT_UINT16:
		return 2;
	case BT_INT32:
	case BT_UINT32:
		return 4;
	case BT_INT64:
	case BT_UINT64:
		return 8;
		case BT_INT128:
		case BT_UINT128:
				return 16;

	case BT_FLOAT32:
		return 4;
	case BT_FLOAT64:
		return 8;

	case BT_BOOLEAN:
		return 4;

	case BT_VOID:
		return 0;
	default:
		throw RepriseError(Strings::format("Unexpected kind (%u).", m_kind));
	}
}

//		BasicType - RepriseBase implementation
int BasicType::getChildCount(void) const
{
	return 0;
}

RepriseBase& BasicType::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("BasicType::getChild()");
}

std::string BasicType::dumpState(void) const
{
	return "[BasicType]" + TypeBase::dumpState() + basicTypeToString(m_kind, false);
}

bool BasicType::isEqual(const TypeBase &type) const
{
	if (const BasicType* other = type.cast_ptr<BasicType>())
	{
		if (other->m_kind == m_kind)
			return TypeBase::isEqual(type);
	}
	return false;
}

std::string BasicType::basicTypeToString(const BasicType::BasicTypes basicType, const bool shouldThrow)
{
	std::string state;
	switch (basicType)
	{
	case BT_CHAR:
		state = "char";
		break;
	case BT_WIDE_CHAR:
		state = "wchar_t";
		break;
	case BT_INT8:
		state = "int8";
		break;
	case BT_UINT8:
		state = "uint8";
		break;
	case BT_INT16:
		state = "int16";
		break;
	case BT_UINT16:
		state = "uint16";
		break;
	case BT_INT32:
		state = "int32";
		break;
	case BT_UINT32:
		state = "uint32";
		break;
	case BT_INT64:
		state = "int64";
		break;
	case BT_UINT64:
		state = "uint64";
		break;
		case BT_INT128:
				state = "int128";
				break;
		case BT_UINT128:
				state = "uint128";
				break;

	case BT_FLOAT32:
		state = "float32";
		break;
	case BT_FLOAT64:
		state = "float64";
		break;

	case BT_BOOLEAN:
		state = "bool";
		break;

	case BT_STRING:
		state = "string";
		break;
	case BT_WIDE_STRING:
		state = "wstring";
		break;

	case BT_VOID:
		state = "void";
		break;
	default:
		if (shouldThrow)
			throw RepriseError(Strings::format("Unexpected basic type (%u).", basicType));
		else
			return Strings::format("unexpected (%u).", basicType);
	}
	return state;
}

//	PtrType class implementation
PtrType::PtrType(TypeBase* const pointedType) :
	m_pointedType(pointedType), m_restrict(false)
{
	OPS_ASSERT(pointedType != 0)
	m_pointedType->setParent(this);
}


PtrType::PtrType(ReprisePtr<TypeBase> pointedType) :
	m_pointedType(pointedType), m_restrict(false)
{
//	OPS_ASSERT(pointedType != 0)
	m_pointedType->setParent(this);
}

PtrType::PtrType(const PtrType& other) : TypeBase(other), m_pointedType(other.m_pointedType->clone()), m_restrict(other.m_restrict)
{
	m_pointedType->setParent(this);
}

const TypeBase& PtrType::getPointedType(void) const
{
	OPS_ASSERT(m_pointedType.get() != 0)
	return *m_pointedType;
}

TypeBase& PtrType::getPointedType(void)
{
	OPS_ASSERT(m_pointedType.get() != 0)
	return *m_pointedType;
}

void PtrType::setPointedType(ReprisePtr<TypeBase> typeBase)
{
	m_pointedType = typeBase;
	m_pointedType->setParent(this);
}

void PtrType::setPointedType(TypeBase* typeBase)
{
	m_pointedType.reset(typeBase);
	m_pointedType->setParent(this);
}

bool PtrType::isRestrict(void) const
{
	return m_restrict;
}

void PtrType::setRestrict(const bool restrictQualifier)
{
	preNotify(BaseListener::CT_MODIFY);
	m_restrict = restrictQualifier;
	postNotify(BaseListener::CT_MODIFY);
}

//		PtrType - RepriseBase implementation
int PtrType::getChildCount(void) const
{
	return 1;
}

RepriseBase& PtrType::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("PtrType::getChild");
	if (m_pointedType.get() == 0)
		throw UnexpectedChildError("Unexpected getting of null pointer at PtrType::getChild().");
	return *m_pointedType;
}

std::string PtrType::dumpState(void) const
{
	OPS_ASSERT(m_pointedType.get() != 0)
	return "[PtrType]" + TypeBase::dumpState() + Strings::format("(%s * %s)", m_pointedType->dumpState().c_str(),
		m_restrict ? "restrict" : "");
}

bool PtrType::isEqual(const TypeBase &type) const
{
	if (const PtrType* other = type.cast_ptr<PtrType>())
	{
		if (m_restrict == other->m_restrict &&
			m_pointedType->isEqual(*other->m_pointedType))
			return TypeBase::isEqual(type);
	}
	return false;
}

//	TypedefType class implementation
TypedefType::TypedefType(TypeBase* const baseType)
	: m_baseType(baseType)
{
	OPS_ASSERT(baseType != 0)
	m_baseType->setParent(this);
}

TypedefType::TypedefType(const TypedefType& other) : TypeBase(other), m_baseType(other.m_baseType->clone())
{
	m_baseType->setParent(this);
}

const TypeBase& TypedefType::getBaseType(void) const
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

TypeBase& TypedefType::getBaseType(void)
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

void TypedefType::setBaseType(TypeBase* typeBase)
{
	m_baseType.reset(typeBase);
	m_baseType->setParent(this);
}

//		TypedefType - RepriseBase implementation
int TypedefType::getChildCount(void) const
{
	return 1;
}

RepriseBase& TypedefType::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("TypedefType::getChild");
	if (m_baseType.get() == 0)
		throw UnexpectedChildError("Unexpected getting of null pointer at TypedefType::getChild().");
	return *m_baseType;
}

bool TypedefType::isEqual(const TypeBase &type) const
{
	if (const TypedefType* other = type.cast_ptr<TypedefType>())
	{
		if (m_baseType->isEqual(*other->m_baseType))
			return TypeBase::isEqual(type);
	}
	return false;
}

std::string TypedefType::dumpState(void) const
{
	OPS_ASSERT(m_baseType.get() != 0)
	return "[TypedefType]" + TypeBase::dumpState() + "(" + m_baseType->dumpState() + ")";
}

//	ArrayType class implementation
ArrayType::ArrayType(TypeBase* const baseType) :
	m_elements(INCOMPLETE_ARRAY_SIZE), m_baseType(baseType)
{
	OPS_ASSERT(baseType != 0)
	m_baseType->setParent(this);
}

ArrayType::ArrayType(int elements, TypeBase* const baseType) :
	m_elements(elements), m_baseType(baseType)
{
	OPS_ASSERT(baseType != 0)
	m_baseType->setParent(this);
}

ArrayType::ArrayType(ExpressionBase* countExpression, TypeBase* baseType, int elements)
	: m_elements(elements)
{
	m_baseType.reset(baseType);
	m_baseType->setParent(this);
	m_countExpr.reset(countExpression);
	m_countExpr->setParent(this);
}

ArrayType::ArrayType(const ArrayType& other) : TypeBase(other)
	, m_elements(other.m_elements)
	, m_baseType(other.m_baseType->clone())
{
	m_baseType->setParent(this);
	if (other.hasCountExpression())
	{
		m_countExpr.reset(other.m_countExpr->clone());
		m_countExpr->setParent(this);
	}
}

int ArrayType::getElementCount(void) const
{
	return m_elements;
}

void ArrayType::setElementCount(const int count)
{
	m_elements = count;
}

bool ArrayType::hasCountExpression(void) const
{
	return m_countExpr.get() != 0;
}

const ExpressionBase& ArrayType::getCountExpression(void) const
{
	OPS_ASSERT(hasCountExpression())
	OPS_ASSERT(m_countExpr.get() != 0)
	return *m_countExpr;
}

ExpressionBase& ArrayType::getCountExpression(void)
{
	OPS_ASSERT(hasCountExpression())
	OPS_ASSERT(m_countExpr.get() != 0)
	return *m_countExpr;
}

void ArrayType::setCountExpression(ExpressionBase* countExpression)
{
	m_elements = VAR_SIZE_ARRAY_SIZE;
	m_countExpr.reset(countExpression);
	m_countExpr->setParent(this);
}

const TypeBase& ArrayType::getBaseType(void) const
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

TypeBase& ArrayType::getBaseType(void)
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

void ArrayType::setBaseType(TypeBase* typeBase)
{
	m_baseType.reset(typeBase);
	m_baseType->setParent(this);
}

bool ArrayType::isFullType(void) const
{
	return m_elements != INCOMPLETE_ARRAY_SIZE;
}

bool ArrayType::isEqual(const TypeBase &type) const
{
	if (const ArrayType* other = type.cast_ptr<ArrayType>())
	{
		if (m_elements == other->m_elements &&
			m_baseType->isEqual(*other->m_baseType) &&
			hasCountExpression() == other->hasCountExpression())
		{
			if (hasCountExpression())
			{
				if (m_countExpr->isEqual(*other->m_countExpr))
					return TypeBase::isEqual(type);
			}
			else
				return TypeBase::isEqual(type);
		}
	}
	return false;
}

//	ArrayType - RepriseBase implementation
int ArrayType::getChildCount(void) const
{
	if (hasCountExpression())
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

RepriseBase& ArrayType::getChild(const int index)
{
	switch (index)
	{
	case 0:
	{
		if (m_baseType.get() == 0)
			throw UnexpectedChildError("Unexpected getting of null pointer at ArrayType::getChild().");
		return *m_baseType;
	}
	case 1:
	{
		if (m_countExpr.get() == 0)
			throw UnexpectedChildError("Unexpected getting of null pointer at ArrayType::getChild().");
		return *m_countExpr;
	}
	default:
		throw UnexpectedChildError("ArrayType::getChild");
	}
}

std::string ArrayType::dumpState(void) const
{
	if (hasCountExpression())
	{
		return "[ArrayType]" + TypeBase::dumpState() + Strings::format("([%s] of %s)", m_countExpr->dumpState().c_str(), m_baseType->dumpState().c_str());
	}
	else
	{
		if (isFullType())
			return "[ArrayType]" + TypeBase::dumpState() + Strings::format("([%i] of %s)", m_elements, m_baseType->dumpState().c_str());
		else
			return "[ArrayType]" + TypeBase::dumpState() + Strings::format("([] of %s)", m_baseType->dumpState().c_str());
	}
}

//	VectorType class implementation
VectorType::VectorType(int elements, TypeBase* const baseType) :
		m_elements(elements), m_baseType(baseType)
{
	OPS_ASSERT(baseType != 0)
	m_baseType->setParent(this);
}

VectorType::VectorType(const VectorType& other) : TypeBase(other)
	, m_elements(other.m_elements)
	, m_baseType(other.m_baseType->clone())
{
	m_baseType->setParent(this);
}

int VectorType::getElementCount(void) const
{
	return m_elements;
}

void VectorType::setElementCount(const int count)
{
	m_elements = count;
}

const TypeBase& VectorType::getBaseType(void) const
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

TypeBase& VectorType::getBaseType(void)
{
	OPS_ASSERT(m_baseType.get() != 0)
	return *m_baseType;
}

void VectorType::setBaseType(TypeBase* typeBase)
{
	m_baseType.reset(typeBase);
	m_baseType->setParent(this);
}

bool VectorType::isEqual(const TypeBase &type) const
{
	if (const VectorType* other = type.cast_ptr<VectorType>())
	{
		if (m_elements == other->m_elements &&
						m_baseType->isEqual(*other->m_baseType))
		{
			return TypeBase::isEqual(type);
		}
	}
	return false;
}

//	VectorType - RepriseBase implementation
int VectorType::getChildCount(void) const
{
	return 1;
}

RepriseBase& VectorType::getChild(const int index)
{
	switch (index)
	{
	case 0:
	{
		if (m_baseType.get() == 0)
			throw UnexpectedChildError("Unexpected getting of null pointer at VectorType::getChild().");
		return *m_baseType;
	}
	default:
		throw UnexpectedChildError("ArrayType::getChild");
	}
}

std::string VectorType::dumpState(void) const
{
	return "[VectorType]" + TypeBase::dumpState() + Strings::format("([%i] of %s)", m_elements, m_baseType->dumpState().c_str());
}

//	StructMemberDescriptor class implementation
StructMemberDescriptor::StructMemberDescriptor(const std::string& name, TypeBase* const memberType)
	: m_name(name), m_memberType(memberType), m_bitsLimit(NO_BITS_LIMIT)
{
	OPS_ASSERT(memberType != 0)
	m_memberType->setParent(this);
}

StructMemberDescriptor::StructMemberDescriptor(const std::string& name, TypeBase* const memberType, int bitsLimit)
	: m_name(name), m_memberType(memberType), m_bitsLimit(bitsLimit)
{
	OPS_ASSERT(memberType != 0)
	m_memberType->setParent(this);
}

StructMemberDescriptor::StructMemberDescriptor(const std::string& name, TypeBase* memberType, ExpressionBase* limitExpr)
	: m_name(name), m_memberType(memberType), m_bitsLimit(VAR_BITS_LIMIT), m_limitExpr(limitExpr)
{
	OPS_ASSERT(memberType != 0)
	OPS_ASSERT(limitExpr != 0)
	m_memberType->setParent(this);
	m_limitExpr->setParent(this);
}

std::string StructMemberDescriptor::getName(void) const
{
	return m_name;
}

const TypeBase& StructMemberDescriptor::getType(void) const
{
	OPS_ASSERT(m_memberType.get() != 0)
	return *m_memberType;
}

TypeBase& StructMemberDescriptor::getType(void)
{
	OPS_ASSERT(m_memberType.get() != 0)
	return *m_memberType;
}

void StructMemberDescriptor::setType(TypeBase* typeBase)
{
	m_memberType.reset(typeBase);
	m_memberType->setParent(this);
}

int StructMemberDescriptor::getBitsLimit(void) const
{
	return m_bitsLimit;
}

bool StructMemberDescriptor::hasLimitExpression(void) const
{
	return m_bitsLimit == VAR_BITS_LIMIT;
}

const ExpressionBase& StructMemberDescriptor::getLimitExpression(void) const
{
	OPS_ASSERT(hasLimitExpression())
	return *m_limitExpr;
}

ExpressionBase& StructMemberDescriptor::getLimitExpression(void)
{
	OPS_ASSERT(hasLimitExpression())
	return *m_limitExpr;
}

void StructMemberDescriptor::setLimitExpression(ExpressionBase* countExpression)
{
	OPS_ASSERT(countExpression != 0)
	m_bitsLimit = VAR_BITS_LIMIT;
	m_limitExpr.reset(countExpression);
	m_limitExpr->setParent(this);
}

//	StructMemberDescriptor - RepriseBase implementation
int StructMemberDescriptor::getChildCount(void) const
{
	if (hasLimitExpression())
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

RepriseBase& StructMemberDescriptor::getChild(const int index)
{
	switch (index)
	{
	case 0:
		if (m_memberType.get() == 0)
			throw UnexpectedChildError("Unexpected getting of null pointer type at StructMemberDescriptor::getChild().");
		return *m_memberType;
	case 1:
		if (m_limitExpr.get() == 0)
			throw UnexpectedChildError("Unexpected getting of null pointer limit expression at StructMemberDescriptor::getChild().");
		return *m_limitExpr;
	default:
		throw UnexpectedChildError("Unexpected getting of child at StructMemberDescriptor::getChild().");
	}
}

std::string StructMemberDescriptor::dumpState(void) const
{
	std::string member = "[StructMemberDescriptor]" + RepriseBase::dumpState() +
			m_memberType->dumpState() + " " + m_name;
	if (m_bitsLimit == VAR_BITS_LIMIT)
	{
		member += Strings::format(" : %s", m_limitExpr->dumpState().c_str());
	}
	else
	if (m_bitsLimit > 0)
	{
		member += Strings::format(" : %i", m_bitsLimit);
	}
	return member;
}

//	StructType implementation
StructType::StructType(bool unionFlag /* = false*/) : m_incomplete(false), m_union(unionFlag)
{
}

StructType::StructType(const StructType& other) : TypeBase(other)
	, m_incomplete(other.m_incomplete)
	, m_union(other.m_union)
{
	for (MembersType::ConstIterator it = other.m_members.begin(); it != other.m_members.end(); ++it)
	{
		if ((*it)->hasLimitExpression())
		{
			addMember(new StructMemberDescriptor((*it)->getName(), (*it)->getType().clone(), (*it)->getLimitExpression().clone()));
		}
		else
		{
			addMember(new StructMemberDescriptor((*it)->getName(), (*it)->getType().clone(), (*it)->getBitsLimit()));
		}
	}
}

bool StructType::isUnion(void) const
{
	return m_union;
}

int StructType::getMemberCount(void) const
{
	return m_members.size();
}

const StructMemberDescriptor& StructType::getMember(const int index) const
{
	if (index < 0 || index >= getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	return m_members[index];
}

StructMemberDescriptor& StructType::getMember(const int index)
{
	if (index < 0 || index >= getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	return m_members[index];
}

void StructType::addMember(StructMemberDescriptor* const descriptor)
{
	m_members.add(descriptor).setParent(this);
}

void StructType::insertMember(const int index, StructMemberDescriptor* const descriptor)
{
	if (index < 0 || index > getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	m_members.insert(index, descriptor).setParent(this);
}

void StructType::setIncomplete(const bool isIncomplete)
{
	m_incomplete = isIncomplete;
}

//		StructType - TypeBase overriding
void StructType::setConst(const bool constQualified)
{
	TypeBase::setConst(constQualified);
	for (MembersType::Iterator it = m_members.begin(); it != m_members.end(); ++it)
	{
		(*it)->getType().setConst(constQualified);
	}
}

void StructType::setVolatile(const bool volatileQualified)
{
	TypeBase::setVolatile(volatileQualified);
	for (MembersType::Iterator it = m_members.begin(); it != m_members.end(); ++it)
	{
		(*it)->getType().setVolatile(volatileQualified);
	}
}

bool StructType::isFullType(void) const
{
	return !m_incomplete;
}

bool StructType::isEqual(const TypeBase &type) const
{
	if (const StructType* other = type.cast_ptr<StructType>())
	{
		if (m_incomplete == other->m_incomplete &&
			m_union == other->m_union &&
			m_members.size() == other->m_members.size())
		{
			for(int i = 0; i < m_members.size(); ++i)
			{
				const StructMemberDescriptor& member = m_members[i];
				const StructMemberDescriptor& otherMember = other->m_members[i];

				if (member.getName() != otherMember.getName() ||
					member.getBitsLimit() != otherMember.getBitsLimit() ||
					!member.getType().isEqual(otherMember.getType()) ||
					member.hasLimitExpression() != otherMember.hasLimitExpression())
					return false;

				if (member.hasLimitExpression() &&
					!member.getLimitExpression().isEqual(otherMember.getLimitExpression()))
					return false;
			}
			return TypeBase::isEqual(type);
		}
	}
	return false;
}

//		StructType - RepriseBase implementation
int StructType::getChildCount(void) const
{
	return getMemberCount();
}

RepriseBase& StructType::getChild(const int index)
{
	return getMember(index);
}

std::string StructType::dumpState(void) const
{
	std::string state = "[StructType]" + TypeBase::dumpState();
	state += (m_union ? "union" : "struct") + std::string("\n");
	for (MembersType::ConstIterator it = m_members.begin(); it != m_members.end(); ++it)
	{
		state += "\t" + (*it)->dumpState() + "\n";
	}
	if (m_incomplete)
		state += "incomplete";
	return state;
}

//	EnumMemberDescriptor class implementation
EnumMemberDescriptor::EnumMemberDescriptor(const std::string& name, const int value)
	: m_name(name), m_value(value)
{
}

std::string EnumMemberDescriptor::getName(void) const
{
	return m_name;
}

int EnumMemberDescriptor::getValue(void) const
{
	return m_value;
}

EnumType& EnumMemberDescriptor::getEnum()
{
	if (getParent() == 0)
		throw RepriseError("Unexpected NULL parent in enum member descriptor");
	EnumType* parentEnum = getParent()->cast_ptr<EnumType>();
	if (parentEnum == 0)
		throw RepriseError("Unexpected parent in enum member descriptor");
	return *parentEnum;
}

//		EnumMemberDescriptor - RepriseBase implementation
int EnumMemberDescriptor::getChildCount(void) const
{
	return 0;
}

RepriseBase& EnumMemberDescriptor::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("EnumMemberDescriptor::getChild()");
}

std::string EnumMemberDescriptor::dumpState(void) const
{
	std::string state = "[EnumMemberDescriptor]" + RepriseBase::dumpState();
	state += Strings::format("%s = %i", m_name.c_str(), m_value);
	return state;
}

//	EnumType class implementation
EnumType::EnumType(bool incomplete /* = false*/) : m_incomplete(incomplete)
{
}

EnumType::EnumType(const EnumType& other) : TypeBase(other), m_incomplete(other.m_incomplete)
{
	for (MembersType::ConstIterator it = other.m_members.begin(); it != other.m_members.end(); ++it)
	{
		addMember(new EnumMemberDescriptor((*it)->getName(), (*it)->getValue()));
	}
}

int EnumType::getMemberCount(void) const
{
	return m_members.size();
}

const EnumMemberDescriptor& EnumType::getMember(const int index) const
{
	if (index < 0 || index > getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	return m_members[index];
}

EnumMemberDescriptor& EnumType::getMember(const int index)
{
	if (index < 0 || index > getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	return m_members[index];
}

EnumMemberDescriptor* EnumType::getMember(const std::string& name)
{
    for (RepriseList<EnumMemberDescriptor>::Iterator it = m_members.begin(); it != m_members.end(); ++it)
    {
        EnumMemberDescriptor* mem = *it;
        if (mem->getName() == name) return mem;
    }
    return NULL;
}

const EnumMemberDescriptor* EnumType::getMember(const std::string& name) const
{
    for (RepriseList<EnumMemberDescriptor>::ConstIterator it = m_members.begin(); it != m_members.end(); ++it)
    {
        const EnumMemberDescriptor* mem = *it;
        if (mem->getName() == name) return mem;
    }
    return NULL;
}

void EnumType::addMember(EnumMemberDescriptor* const descriptor)
{
	m_members.add(descriptor).setParent(this);
}

void EnumType::insertMember(const int index, EnumMemberDescriptor* const descriptor)
{
	if (index < 0 || index > getMemberCount())
		throw RepriseError(Strings::format("Getting unexpected member (%i), member count (%i).",
			index, getMemberCount()));
	m_members.insert(index, descriptor).setParent(this);
}

void EnumType::setIncomplete(const bool incomplete)
{
	m_incomplete = incomplete;
}

//		EnumType - TypeBase overriding
bool EnumType::isFullType(void) const
{
	return !m_incomplete;
}

bool EnumType::isEqual(const TypeBase &type) const
{
	if (const EnumType* other = type.cast_ptr<EnumType>())
	{
		if (m_incomplete == other->m_incomplete &&
			m_members.size() == other->m_members.size())
		{
			for(int i = 0; i < m_members.size(); ++i)
			{
				const EnumMemberDescriptor& member = m_members[i];
				const EnumMemberDescriptor& otherMember = other->m_members[i];

				if (member.getName() != otherMember.getName() ||
					member.getValue() != otherMember.getValue())
					return false;
			}
			return TypeBase::isEqual(type);
		}
	}
	return false;
}

//		EnumType - RepriseBase implementation
int EnumType::getChildCount(void) const
{
	return getMemberCount();
}

RepriseBase& EnumType::getChild(int index)
{
	return getMember(index);
}

std::string EnumType::dumpState(void) const
{
	std::string state = "[EnumType]" + TypeBase::dumpState();
	state += "enum\n";
	for (MembersType::ConstIterator it = m_members.begin(); it != m_members.end(); ++it)
	{
		state += "\t" + (*it)->dumpState() + "\n";
	}
	return state;
}

//	ParameterDescriptor class implementation
ParameterDescriptor::ParameterDescriptor(TypeBase* const paramType)
	: m_type(paramType), m_transitKind(TK_VALUE), m_optional(false)
{
	m_type->setParent(this);
}

ParameterDescriptor::ParameterDescriptor(const std::string& name, TypeBase* const paramType)
	: m_name(name), m_type(paramType), m_transitKind(TK_VALUE), m_optional(false)
{
	m_type->setParent(this);
}

SubroutineType& ParameterDescriptor::getParentSubroutine(void) const
{
	SubroutineType* parent = dynamic_cast<SubroutineType*>(getParent());
	if (parent != 0)
	{
		return *parent;
	}
	else
		throw RepriseError("ParameterDescriptor::getParentSubroutine()");
}

std::string ParameterDescriptor::getName(void) const
{
	return m_name;
}

const TypeBase& ParameterDescriptor::getType(void) const
{
	return *m_type;
}

TypeBase& ParameterDescriptor::getType(void)
{
	return *m_type;
}

void ParameterDescriptor::setType(TypeBase* typeBase)
{
	m_type.reset(typeBase);
	m_type->setParent(this);
}

ParameterDescriptor::TransitKind ParameterDescriptor::getTransitKind(void) const
{
	return m_transitKind;
}

void ParameterDescriptor::setTransitKind(const TransitKind kind)
{
	m_transitKind = kind;
}

bool ParameterDescriptor::isOptional() const
{
	return m_optional;
}

void ParameterDescriptor::setOptional(const bool isOptional)
{
	m_optional = isOptional;
}

bool ParameterDescriptor::hasAssociatedVariable() const
{
	return const_cast<ParameterDescriptor*>(this)->getAssociatedVariableHelper() != 0;
}

const VariableDeclaration& ParameterDescriptor::getAssociatedVariable() const
{
	VariableDeclaration* variable = const_cast<ParameterDescriptor*>(this)->getAssociatedVariableHelper();
	if (variable == 0)
		throw RepriseError("Unexpected getting associated variable.");
	return *variable;
}

VariableDeclaration& ParameterDescriptor::getAssociatedVariable()
{
	VariableDeclaration* variable = const_cast<ParameterDescriptor*>(this)->getAssociatedVariableHelper();
	if (variable == 0)
		throw RepriseError("Unexpected getting associated variable.");
	return *variable;
}

VariableDeclaration* ParameterDescriptor::getAssociatedVariableHelper(void)
{
	SubroutineType* subroutine = getParent()->cast_ptr<SubroutineType>();
	if (subroutine == 0)
		return 0;
	SubroutineDeclaration* subDecl = subroutine->getParent()->cast_ptr<SubroutineDeclaration>();
	if (subDecl == 0)
		return 0;
	if (!subDecl->hasImplementation())
		return 0;
	Declarations& declarations = subDecl->getDeclarations();
	if (declarations.isEmpty())
		return 0;
	for (Declarations::VarIterator iter = declarations.getFirstVar(); iter.isValid(); ++iter)
	{
		if (iter->hasParameterReference())
		{
			if (&iter->getParameterReference() == this)
			{
				return &*iter;
			}
		}
	}
	return 0;
}

//		ParameterDescriptor - RepriseBase implementation
int ParameterDescriptor::getChildCount(void) const
{
	return 1;
}

RepriseBase& ParameterDescriptor::getChild(const int index)
{
	OPS_UNUSED(index)
	return *m_type;
}

std::string ParameterDescriptor::dumpState(void) const
{
	std::string state = "[ParameterDescriptor]" + RepriseBase::dumpState();
	state += m_type->dumpState();
	if (!m_name.empty())
		state += " " + m_name;
	return state;
}

std::string ParameterDescriptor::transitKindToString(TransitKind kind)
{
	switch (kind)
	{
	case TK_VALUE:
		return "value";
	case TK_REFERENCE:
		return "ref";
	case TK_OUT:
		return "out";
	OPS_DEFAULT_CASE_LABEL
	}
	return "(unknown)";
}


//	SubroutineType class implementation
std::string SubroutineType::getCallingText(const CallingKind kind)
{
	switch (kind)
	{
	case CK_DEFAULT:
		return "";
	case CK_FASTCALL:
		return "__fastcall";
	case CK_STDCALL:
		return "__stdcall";
	case CK_CDECL:
		return "__cdecl";
	case CK_THISCALL:
		return "__thiscall";
	case CK_PASCAL:
		return "__pascal";
		OPS_DEFAULT_CASE_LABEL
	}
	return "(unexpected)";
}

SubroutineType::SubroutineType(TypeBase* const returnType)
	: m_callingKind(CK_DEFAULT), m_isVarArg(false), m_isArgsKnown(true), m_returnType(returnType)
{
	m_returnType->setParent(this);
}

SubroutineType::SubroutineType(TypeBase* const returnType,
	const CallingKind callingKind, const bool varArg, const bool argsKnown)
	: m_callingKind(callingKind), m_isVarArg(varArg), m_isArgsKnown(argsKnown), m_returnType(returnType)
{
	m_returnType->setParent(this);
}

SubroutineType::SubroutineType(const SubroutineType& other) : TypeBase(other)
	, m_callingKind(other.m_callingKind)
	, m_isVarArg(other.m_isVarArg)
	, m_isArgsKnown(other.m_isArgsKnown)
	, m_returnType(other.m_returnType->clone())
{
	m_returnType->setParent(this);
	for (ParametersType::ConstIterator it = other.m_parameters.begin(); it != other.m_parameters.end(); ++it)
	{
		addParameter(new ParameterDescriptor((*it)->getName(), (*it)->getType().clone()));
	}
}

SubroutineType::CallingKind SubroutineType::getCallingKind(void) const
{
	return m_callingKind;
}

void SubroutineType::setCallingKind(const CallingKind kind)
{
	m_callingKind = kind;
}

bool SubroutineType::isVarArg(void) const
{
	return m_isVarArg;
}

void SubroutineType::setVarArg(const bool isVarArg)
{
	m_isVarArg = isVarArg;
}


bool SubroutineType::isArgsKnown(void) const
{
	return m_isArgsKnown;
}

void SubroutineType::setArgsKnown(const bool argsKnown)
{
	m_isArgsKnown = argsKnown;
}

int SubroutineType::getParameterCount(void) const
{
	return static_cast<int>(m_parameters.size());
}

const ParameterDescriptor& SubroutineType::getParameter(const int index) const
{
	return m_parameters[index];
}

ParameterDescriptor& SubroutineType::getParameter(const int index)
{
	return m_parameters[index];
}

void SubroutineType::addParameter(ParameterDescriptor* const descriptor)
{
	m_parameters.add(descriptor).setParent(this);
}

void SubroutineType::insertParameter(const int index, ParameterDescriptor* const descriptor)
{
	if (index < 0 || index > getParameterCount())
		throw RepriseError(Strings::format("Getting unexpected parameter (%i), parameter count (%i).",
			index, getParameterCount()));
	m_parameters.insert(index, descriptor).setParent(this);
}

const TypeBase& SubroutineType::getReturnType(void) const
{
	return *m_returnType;
}

TypeBase& SubroutineType::getReturnType(void)
{
	return *m_returnType;
}

void SubroutineType::setReturnType(TypeBase* typeBase)
{
	m_returnType.reset(typeBase);
	m_returnType->setParent(this);
}

bool SubroutineType::isEqual(const TypeBase &type) const
{
	if (const SubroutineType* other = type.cast_ptr<SubroutineType>())
	{
		if (m_callingKind == other->m_callingKind &&
			m_isVarArg == other->m_isVarArg &&
			m_isArgsKnown == other->m_isArgsKnown &&
			m_parameters.size() == other->m_parameters.size() &&
			m_returnType->isEqual(*other->m_returnType))
		{
			for(int i = 0; i < m_parameters.size(); ++i)
			{
				const ParameterDescriptor& descriptor = m_parameters[i];
				const ParameterDescriptor& otherDescriptor = other->m_parameters[i];

				if (descriptor.getName() != otherDescriptor.getName() ||
					!descriptor.getType().isEqual(otherDescriptor.getType()) ||
					descriptor.isOptional() != otherDescriptor.isOptional() ||
					descriptor.getTransitKind() != otherDescriptor.getTransitKind())
					return false;
			}
			return TypeBase::isEqual(type);
		}
	}
	return false;
}

//		SubroutineType - RepriseBase implementation
int SubroutineType::getChildCount(void) const
{
	return getParameterCount() + 1;
}

RepriseBase& SubroutineType::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("SubroutineType::getChild");
	if (index == 0)
		return *m_returnType;
	else
	if (index <= getChildCount() - 1)
		return m_parameters[index - 1];
	throw UnexpectedChildError("SubroutineType::getChild");
}

std::string SubroutineType::dumpState(void) const
{
	std::string state = "[SubroutineType]" + TypeBase::dumpState();
	state += m_returnType->dumpState() + " " + SubroutineType::getCallingText(m_callingKind)
		+ " (";
	if (m_parameters.size() > 0)
	{
		ParametersType::ConstIterator it = m_parameters.begin();
		state += (*it)->dumpState();
		++it;
		for (; it != m_parameters.end(); ++it)
		{
			state += ", " + (*it)->dumpState();
		}
		if (m_isVarArg)
		{
			state += ", ...";
		}
	}
	else
		if (m_isVarArg)
		{
			state += "...";
		}
	state += ")";
	return state;
}

//	DeclaredType class implementation
DeclaredType::DeclaredType(TypeDeclaration& typeDeclaration)
	: m_typeDeclaration(&typeDeclaration)
{
}

DeclaredType::DeclaredType(const DeclaredType& other) : TypeBase(other), m_typeDeclaration(other.m_typeDeclaration)
{
}

const TypeDeclaration& DeclaredType::getDeclaration(void) const
{
	return *m_typeDeclaration;
}

TypeDeclaration& DeclaredType::getDeclaration(void)
{
	return *m_typeDeclaration;
}

void DeclaredType::setDeclaration(TypeDeclaration* typeDecl)
{
	m_typeDeclaration.reset(typeDecl);
}

//		DeclaredType - RepriseBase implementation
int DeclaredType::getChildCount(void) const
{
	return 0;
}

RepriseBase& DeclaredType::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("DeclaredType::getChild");
}

int DeclaredType::getLinkCount(void) const
{
	return 1;
}

RepriseBase& DeclaredType::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("DeclaredType::getLink()");
	OPS_ASSERT(m_typeDeclaration.get() != 0)
	return *m_typeDeclaration;
}

std::string DeclaredType::dumpState(void) const
{
	std::string state = "[DeclaredType]" + TypeBase::dumpState();
	state += "Declared type: " + m_typeDeclaration->getName();
	return state;
}

bool DeclaredType::isEqual(const TypeBase &type) const
{
	if (const DeclaredType* other = type.cast_ptr<DeclaredType>())
	{
		if (m_typeDeclaration.get() == other->m_typeDeclaration.get())
			return TypeBase::isEqual(type);
	}
	return false;
}


//	Exit namespace
}
}
