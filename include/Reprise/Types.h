#ifndef OPS_IR_REPRISE_TYPES_H_INCLUDED__
#define OPS_IR_REPRISE_TYPES_H_INCLUDED__

// TODO: Clone as * method
// TODO: Add initializers
// TODO: Add recursive structure type check
// TODO: Add incomplete types


#include "Reprise/Common.h"
#include "Reprise/Utils.h"


namespace OPS
{
namespace Reprise
{

class ExpressionBase;
class VariableDeclaration;

///	Base class for Types in IR
class TypeBase : public RepriseBase
{
public:
	bool isConst(void) const;

	bool isVolatile(void) const;

	virtual void setConst(bool constQualified);

	virtual void setVolatile(bool volatileQualified);

	virtual bool isFullType(void) const;

	virtual TypeBase* clone(void) const = 0;

	virtual bool isEqual(const TypeBase& type) const = 0;

//		RepriseBase implementation
	virtual std::string dumpState(void) const;

protected:
	TypeBase(void);
	explicit TypeBase(const TypeBase& typeBase);

private:
	bool m_const;
	bool m_volatile;
};

///	Base class for basic types in IR
class BasicTypeBase : public TypeBase
{
public:
	virtual int getSizeOf(void) const = 0;

protected:
	BasicTypeBase();
	BasicTypeBase(const BasicTypeBase& other);
};

///	Basic type
class BasicType : public BasicTypeBase
{
public:
	enum BasicTypes
	{
		BT_UNDEFINED = 0,
		BT_CHAR,
		BT_WIDE_CHAR,
		BT_INT8,
		BT_INT16,
		BT_INT32,
		BT_INT64,
		BT_INT128,
		BT_UINT8,
		BT_UINT16,
		BT_UINT32,
		BT_UINT64,
		BT_UINT128,

		BT_FLOAT32,
		BT_FLOAT64,
		//		LongDouble,		// long double unsupported
		//		Decimal,

		BT_BOOLEAN,

		BT_STRING,
		BT_WIDE_STRING,

		BT_VOID
	};

	static BasicType* basicType(BasicTypes type);

	static BasicType* charType(void);
	static BasicType* wideCharType(void);
	static BasicType* int8Type(void);
	static BasicType* int16Type(void);
	static BasicType* int32Type(void);
	static BasicType* int64Type(void);
	static BasicType* int128Type(void);
	static BasicType* uint8Type(void);
	static BasicType* uint16Type(void);
	static BasicType* uint32Type(void);
	static BasicType* uint64Type(void);
	static BasicType* uint128Type(void);

	static BasicType* float32Type(void);
	static BasicType* float64Type(void);
	static BasicType* booleanType(void);
	static BasicType* voidType(void);

	BasicTypes getKind(void) const;

	virtual int getSizeOf(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//	Static members
	static std::string basicTypeToString(BasicTypes basicType, bool shouldThrow = true);

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(BasicType)

private:
	explicit BasicType(BasicTypes kind);
	explicit BasicType(const BasicType& basicType);

	BasicTypes m_kind; 
};
/*
class UnifiedBasicType : public BasicTypeBase
{
public:


	virtual int getSizeOf(void) const;

private:
	bool m_bigEndian;
	int m_bits;
	int m_abiAlignment;
	int m_preferredAlignment;
};
*/

///	Pointer type
class PtrType : public TypeBase
{
public:
	explicit PtrType(TypeBase* pointedType);
	explicit PtrType(ReprisePtr<TypeBase> pointedType);

	const TypeBase& getPointedType(void) const;
	TypeBase& getPointedType(void);
	void setPointedType(ReprisePtr<TypeBase>);
	void setPointedType(TypeBase* typeBase);

	bool isRestrict(void) const;
	void setRestrict(bool restrictQualifier);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(PtrType)

protected:
	PtrType(const PtrType& other);
	OPS_DEFINE_LISTENABLE()
private:
	ReprisePtr<TypeBase> m_pointedType;
	bool m_restrict;
};

///	Typedef type
class TypedefType : public TypeBase
{
public:
	explicit TypedefType(TypeBase* baseType);

	const TypeBase& getBaseType(void) const;
	TypeBase& getBaseType(void);
	void setBaseType(TypeBase*);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(TypedefType)

protected:
	TypedefType(const TypedefType& other);

private:
	ReprisePtr<TypeBase> m_baseType;
};

///	Array type
class ArrayType : public TypeBase
{
public:
	explicit ArrayType(TypeBase* baseType);
	ArrayType(int elements, TypeBase* baseType);
	ArrayType(ExpressionBase* countExpression, TypeBase* baseType, int elements = -1);

	int getElementCount(void) const;
	void setElementCount(int count);

	bool hasCountExpression(void) const;
	const ExpressionBase& getCountExpression(void) const;
	ExpressionBase& getCountExpression(void);
	void setCountExpression(ExpressionBase* countExpression);

	const TypeBase& getBaseType(void) const;
	TypeBase& getBaseType(void);
	void setBaseType(TypeBase*);

//		TypeBase overriding
	virtual bool isFullType(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ArrayType)

protected:
	ArrayType(const ArrayType& other);

private:
	int m_elements;
	ReprisePtr<ExpressionBase> m_countExpr;
	ReprisePtr<TypeBase> m_baseType;
};

///	Vector type
class VectorType : public TypeBase
{
public:
	VectorType(int elements, TypeBase* baseType);

	int getElementCount(void) const;
	void setElementCount(int count);

	const TypeBase& getBaseType(void) const;
	TypeBase& getBaseType(void);
	void setBaseType(TypeBase*);

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(VectorType)

protected:
	VectorType(const VectorType& other);

private:
	int m_elements;
	ReprisePtr<TypeBase> m_baseType;
};

///	Struct member descriptor
class StructMemberDescriptor : public RepriseBase
{
public:
	StructMemberDescriptor(const std::string& name, TypeBase* memberType);
	StructMemberDescriptor(const std::string& name, TypeBase* memberType, int bitsLimit);
	StructMemberDescriptor(const std::string& name, TypeBase* memberType, ExpressionBase* limitExpr);

	std::string getName(void) const;

	const TypeBase& getType(void) const;
	TypeBase& getType(void);
	void setType(TypeBase*);

	int getBitsLimit(void) const;

	bool hasLimitExpression(void) const;
	const ExpressionBase& getLimitExpression(void) const;
	ExpressionBase& getLimitExpression(void);
	void setLimitExpression(ExpressionBase* countExpression);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	OPS_DEFINE_CLONABLE_INTERFACE(StructMemberDescriptor)

	std::string m_name;
	ReprisePtr<TypeBase> m_memberType;
	int m_bitsLimit;
	ReprisePtr<ExpressionBase> m_limitExpr;
};

///	Struct/Union type
class StructType : public TypeBase
{
public:
	explicit StructType(bool unionFlag = false);

	bool isUnion(void) const;

	int getMemberCount(void) const;

	const StructMemberDescriptor& getMember(int index) const;

	StructMemberDescriptor& getMember(int index);

	void addMember(StructMemberDescriptor* descriptor);

	void insertMember(int index, StructMemberDescriptor* descriptor);

/*
	There are empty structs.
		EXAMPLE: struct A {};
	And there are incomplete structs.
		EXAMPLE: struct A;
*/
	void setIncomplete(bool isIncomplete);

//		TypeBase overriding
	virtual void setConst(bool constQualified);
	virtual void setVolatile(bool volatileQualified);
	virtual bool isFullType(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(StructType)

protected:
	StructType(const StructType& other);

private:
	typedef RepriseList<StructMemberDescriptor> MembersType;

	bool m_incomplete;
	bool m_union;
	MembersType m_members;
};

class EnumType;

///	Enum member descriptor
class EnumMemberDescriptor : public RepriseBase
{
public:
	EnumMemberDescriptor(const std::string& name, int value);

	std::string getName(void) const;

	int getValue(void) const;

	EnumType& getEnum();

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	OPS_DEFINE_CLONABLE_INTERFACE(EnumMemberDescriptor)

	std::string m_name;
	int	m_value;
};

///	Enum type
class EnumType : public TypeBase
{
public:
	explicit EnumType(bool incomplete = false);

	int getMemberCount(void) const;

	const EnumMemberDescriptor& getMember(int index) const;

	EnumMemberDescriptor& getMember(int index);

    EnumMemberDescriptor* getMember(const std::string& name); //return 0, if not found

    const EnumMemberDescriptor* getMember(const std::string& name) const; //return 0, if not found

	void addMember(EnumMemberDescriptor* descriptor);

	void insertMember(int index, EnumMemberDescriptor* descriptor);

/*
	There are empty enums.
		EXAMPLE: enum A {};
	And there are incomplete enums.
		EXAMPLE: enum A;
*/
	void setIncomplete(bool incomplete);

//		TypeBase overriding
	virtual bool isFullType(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(EnumType)

protected:
	EnumType(const EnumType& other);

private:
	typedef RepriseList<EnumMemberDescriptor> MembersType;
	
	bool m_incomplete;
	MembersType m_members;
};

class SubroutineType;

///	Parameter descriptor
class ParameterDescriptor : public RepriseBase
{
public:
	enum TransitKind
	{
		TK_VALUE,
		TK_REFERENCE,
		TK_OUT
	};

	explicit ParameterDescriptor(TypeBase* paramType);
	ParameterDescriptor(const std::string& name, TypeBase* paramType);

	SubroutineType& getParentSubroutine(void) const;

	std::string getName(void) const;

	const TypeBase& getType(void) const;
	TypeBase& getType(void);
	void setType(TypeBase*);

	TransitKind getTransitKind(void) const;
	void setTransitKind(const TransitKind kind);

	bool isOptional() const;
	void setOptional(bool isOptional);

	bool hasAssociatedVariable() const;
	const VariableDeclaration& getAssociatedVariable() const;
	VariableDeclaration& getAssociatedVariable();

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	static std::string transitKindToString(TransitKind kind);

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	OPS_DEFINE_CLONABLE_INTERFACE(ParameterDescriptor)

	VariableDeclaration* getAssociatedVariableHelper(void);

	std::string m_name;
	ReprisePtr<TypeBase> m_type;
	TransitKind m_transitKind;
	bool m_optional;
};

/*
declaration:
	declaration-specifiers init-declarator-list_opt ;
declaration-specifiers:
	storage-class-specifier declaration-specifiers_opt
	type-specifier declaration-specifiers_opt
	type-qualifier declaration-specifiers_opt
	function-specifier declaration-specifiers_opt
init-declarator-list:
	init-declarator
	init-declarator-list , init-declarator
init-declarator:
	declarator
	declarator = initializer

storage-class-specifier:
	typedef
	extern
	static
	auto
	register

function-specifier:
	inline
*/

class SubroutineBody;

///	Subroutine type
class SubroutineType : public TypeBase
{
public:
	enum CallingKind
	{
		CK_DEFAULT,		//	default call
		CK_FASTCALL,	//	fastcall
		CK_STDCALL,		//	stdcall
		CK_CDECL,		//	C call
		CK_THISCALL,	//	C++ this call
		CK_PASCAL		//	pascal call
	};

//	Static methods
	static std::string getCallingText(CallingKind kind);

//	Constructors/destructor
	explicit SubroutineType(TypeBase* returnType);
	SubroutineType(TypeBase* returnType, CallingKind callingKind, bool varArg, bool argsKnown);

//	Methods
	CallingKind getCallingKind(void) const;
	void setCallingKind(const CallingKind kind);

	bool isVarArg(void) const;
	void setVarArg(bool isVarArg);

	bool isArgsKnown(void) const;
	void setArgsKnown(bool argsKnown);

	int getParameterCount(void) const;
	const ParameterDescriptor& getParameter(int index) const;
	ParameterDescriptor& getParameter(int index);
	void addParameter(ParameterDescriptor* descriptor);
	void insertParameter(int index, ParameterDescriptor* descriptor);

	const TypeBase& getReturnType(void) const;
	TypeBase& getReturnType(void);
	void setReturnType(TypeBase*);

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SubroutineType)

protected:
	SubroutineType(const SubroutineType& other);

private:
	typedef RepriseList<ParameterDescriptor> ParametersType;

	CallingKind m_callingKind;
	bool m_isVarArg;
	bool m_isArgsKnown;
	ParametersType m_parameters;
	ReprisePtr<TypeBase> m_returnType;
};

class TypeDeclaration;

///	Declared type
class DeclaredType : public TypeBase
{
public:
	explicit DeclaredType(TypeDeclaration& typeDeclaration);

	const TypeDeclaration& getDeclaration(void) const;
	TypeDeclaration& getDeclaration(void);
	void setDeclaration(TypeDeclaration* typeDecl);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);

	virtual std::string dumpState(void) const;

//		TypeBase implementation
	virtual bool isEqual(const TypeBase &type) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(DeclaredType)

protected:
	DeclaredType(const DeclaredType& other);

private:
	RepriseWeakPtr<TypeDeclaration> m_typeDeclaration;
};

}
}

#endif                      // OPS_IR_REPRISE_TYPES_H_INCLUDED__
