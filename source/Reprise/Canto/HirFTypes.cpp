#include "Reprise/Canto/HirFTypes.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

using namespace OPS::Reprise;
using namespace std;

HirFBasicType::HirFBasicType(const HirFBasicKind basicKind) : m_kind(basicKind)
{
}

HirFBasicType::HirFBasicType(const HirFBasicType& basicType)
	:BasicTypeBase(basicType)
	,m_kind(basicType.m_kind)
{
}

HirFBasicType::HirFBasicKind HirFBasicType::getKind(void) const
{
	return m_kind;
}

int HirFBasicType::getSizeOf(void) const
{
	//if (m_kind == HCBK_LONG_LONG)
	//	return 8;
	//else
	//if (m_kind == HCBK_CHAR)
	//	return 1;
	//else
		return -1;
}

//		RepriseBase implementation
int HirFBasicType::getChildCount(void) const
{
	return 0;
}

RepriseBase& HirFBasicType::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("HirFBasicType::getChild()");
}

std::string HirFBasicType::dumpState(void) const
{
	return "[HirFBasicType]" + TypeBase::dumpState() + HirFBasicType::basicKindToString(m_kind, false);
}

bool HirFBasicType::isEqual(const TypeBase &type) const
{
	if (const HirFBasicType* other = type.cast_ptr<HirFBasicType>())
	{
		if (m_kind == other->m_kind)
			return TypeBase::isEqual(type);
	}
	return false;
}

std::string HirFBasicType::basicKindToString(HirFBasicKind basicKind, const bool shouldThrow)
{
	std::string state;
	switch (basicKind)
	{
	case HFBK_COMPLEX_FLOAT:
		state = "complex float";
		break;
	case HFBK_COMPLEX_DOUBLE:
		state = "complex double";
		break;
	case HFBK_COMPLEX_LONG_DOUBLE:
		state = "complex long double";
		break;

	default:
		if (shouldThrow)
			throw RepriseError(Strings::format("Unexpected basic type (%u).", basicKind));
		else
			return Strings::format("unexpected (%u).", basicKind);
	}
	return state;
}




HirFArrayType::HirFArrayType(TypeBase* baseType, HirFArrayShapeExpression* shape)
	:  m_shape(shape), m_baseType(baseType)
{
	baseType->setParent(this);
	shape->setParent(this);
}

HirFArrayType::HirFArrayType(const HirFArrayType& arrayType)
	:TypeBase(arrayType)
	,m_shape(arrayType.m_shape->clone())
	,m_baseType(arrayType.m_baseType->clone())
{
	m_baseType->setParent(this);
	m_shape->setParent(this);
}

const TypeBase& HirFArrayType::getBaseType(void) const
{
	return *m_baseType;
}

TypeBase& HirFArrayType::getBaseType(void)
{
	return *m_baseType;
}

void HirFArrayType::setBaseType(TypeBase* newTypeBase)
{
	m_baseType.reset(newTypeBase);
	m_baseType->setParent(this);
}

const HirFArrayShapeExpression& HirFArrayType::getShape() const
{
	return *(this->m_shape);
}

HirFArrayShapeExpression& HirFArrayType::getShape()
{
	return *(this->m_shape);
}

void HirFArrayType::setShape(HirFArrayShapeExpression* newShape)
{
	m_shape.reset(newShape);
	m_shape->setParent(this);
}

int HirFArrayType::getChildCount(void) const
{
	return 2;
}

RepriseBase& HirFArrayType::getChild(int index)
{
	switch(index)
	{
	case 0: return *m_baseType;
	case 1: return *m_shape;
		OPS_DEFAULT_CASE_LABEL;
	}
	throw UnexpectedChildError("HirFArrayType::::getChild()");
}

std::string HirFArrayType::dumpState(void) const
{
	string state = "[HirFArrayType]" + m_baseType->dumpState() + "[";
	state += m_shape->dumpState();
	state += "]";
	return state;
}

bool HirFArrayType::isEqual(const TypeBase &type) const
{
	if (const HirFArrayType* other = type.cast_ptr<HirFArrayType>())
	{
		if (m_shape->isEqual(*other->m_shape) &&
			m_baseType->isEqual(*other->m_baseType))
			return TypeBase::isEqual(type);
	}
	return false;
}

}
}
}
