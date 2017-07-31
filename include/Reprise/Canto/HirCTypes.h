#ifndef OPS_IR_REPRISE_CANTO_HIRCTYPES_H_INCLUDED__
#define OPS_IR_REPRISE_CANTO_HIRCTYPES_H_INCLUDED__


#include "Reprise/Types.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

///	HIR Basic type node
class HirCBasicType : public OPS::Reprise::BasicTypeBase
{
public:
	enum HirCBasicKind
	{
		HCBK_UNDEFINED = 0,
		HCBK_CHAR,
		HCBK_WIDE_CHAR,
		HCBK_SCHAR,
		HCBK_SHORT,
		HCBK_INT,
		HCBK_LONG,
		HCBK_LONG_LONG,
        HCBK_INT128,
		HCBK_UCHAR,
		HCBK_USHORT,
		HCBK_UINT,
		HCBK_ULONG,
		HCBK_ULONG_LONG,
        HCBK_UINT128,

		HCBK_FLOAT,
		HCBK_DOUBLE,
		HCBK_LONG_DOUBLE,

		HCBK_COMPLEX_FLOAT,
		HCBK_COMPLEX_DOUBLE,
		HCBK_COMPLEX_LONG_DOUBLE,

		HCBK_BOOL,

		HCBK_VOID
	};

//	Static methods
	static std::string basicKindToString(HirCBasicKind basicKind, const bool shouldThrow);

	explicit HirCBasicType(HirCBasicKind basicKind);

	HirCBasicKind getKind(void) const;

	virtual int getSizeOf(void) const;

//		ClonableMix implementation
	virtual HirCBasicType* clone(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
private:
	HirCBasicKind m_kind;
};


}
}
}

#endif                      // OPS_IR_REPRISE_CANTO_HIRCTYPES_H_INCLUDED__
