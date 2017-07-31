#ifndef FORTRAN2003_HIR_F_TYPES_H
#define FORTRAN2003_HIR_F_TYPES_H

#include "Reprise/Types.h"
#include "Reprise/Canto/HirFExpressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

///	HIR Basic type node
class HirFBasicType : public OPS::Reprise::BasicTypeBase
{
public:
	enum HirFBasicKind
	{
		HFBK_UNDEFINED = 0,

		HFBK_COMPLEX_FLOAT,
		HFBK_COMPLEX_DOUBLE,
		HFBK_COMPLEX_LONG_DOUBLE,

	};

//	Static methods
	static std::string basicKindToString(HirFBasicKind basicKind, const bool shouldThrow);

	explicit HirFBasicType(HirFBasicKind basicKind);

	HirFBasicKind getKind(void) const;

	virtual int getSizeOf(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(HirFBasicType)
private:
	explicit HirFBasicType(const HirFBasicType&);

	HirFBasicKind m_kind;
};



class HirFArrayType : public TypeBase
{
public:				
	explicit HirFArrayType(TypeBase* baseType, HirFArrayShapeExpression* shape);
		
	const TypeBase& getBaseType(void) const;
	TypeBase& getBaseType(void);
	void setBaseType(TypeBase*);

	const HirFArrayShapeExpression& getShape() const;
	HirFArrayShapeExpression& getShape();
	void setShape(HirFArrayShapeExpression* newShape);					
	
//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(HirFArrayType)
private:
	explicit HirFArrayType(const HirFArrayType&);

	ReprisePtr<HirFArrayShapeExpression> m_shape;
	ReprisePtr<TypeBase> m_baseType;
};

}
}
}

#endif 
