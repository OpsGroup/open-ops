#ifndef OPS_IR_REPRISE_EXPRESSIONS_H_INCLUDED__
#define OPS_IR_REPRISE_EXPRESSIONS_H_INCLUDED__

#include "Reprise/Common.h" 
#include "Reprise/Declarations.h"
#include "Reprise/Types.h"
#include "Reprise/Utils.h"

namespace OPS
{
namespace Reprise
{

OPS_DEFINE_EXCEPTION_CLASS(UnexpectedLiteralError, RepriseError)

/// Interface to replacing child expressions in node
class IReplaceChildExpression
{
public:
	virtual ~IReplaceChildExpression() {}
	virtual void replaceChildExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination) = 0;
};

///	Base class for expression nodes in IR
class ExpressionBase : public RepriseBase
{
public:
	ExpressionBase();

	///	Obtain parent statement
	/**
		Traverse parent nodes to find parent statement. 
		May return 0 if no parent statement found (variable initialization).
	*/
	StatementBase* obtainParentStatement(void);

	///	Obtain parent variable declaration
	/**
		Traverse parent nodes to find parent variable declaration. 
		May return 0 if no parent variable declaration found (expression in statement).
	*/
	VariableDeclaration* obtainParentDeclaration(void);

	///	Obtain parent statement
	/**
		Traverse parent nodes to find root expression. 
	*/
	ExpressionBase& obtainRootExpression(void);

	/// Check expressions for equality
	/**
		Returns true if expressions match and false otherwise.	
	*/
	virtual bool isEqual(const ExpressionBase& exprNode) const = 0;

	virtual ReprisePtr<TypeBase> getResultType(void) const;

	virtual ExpressionBase* clone(void) const = 0;
};

///	Base class for literal expression nodes in IR
class LiteralExpression : public ExpressionBase
{
public:
	LiteralExpression();

	virtual LiteralExpression* clone(void) const = 0;

};

///	Basic (general) literal expression node
class BasicLiteralExpression : public LiteralExpression
{
public:
	enum LiteralTypes
	{
		LT_UNDEFINED = 100,
		LT_CHAR,
		LT_WIDE_CHAR,
		LT_INTEGER,
		LT_UNSIGNED_INTEGER,
		LT_FLOAT,
		LT_BOOLEAN,
		LT_STRING,
		LT_WIDE_STRING
	};

	BasicLiteralExpression(void);
	explicit BasicLiteralExpression(LiteralTypes literalType);

//	Static methods
	static std::string literalTypeToString(LiteralTypes literalType, bool shouldThrow = true);

//		General creators
	static BasicLiteralExpression* createChar(char value);
	static BasicLiteralExpression* createWideChar(wchar_t value);
	static BasicLiteralExpression* createInteger(long_long_t value);
	static BasicLiteralExpression* createUnsignedInteger(unsigned_long_long_t value);
	static BasicLiteralExpression* createFloat(double value);
	static BasicLiteralExpression* createBoolean(bool value);
	static BasicLiteralExpression* createString(const std::string& value);
	static BasicLiteralExpression* createWideString(const std::wstring& value);

//		Special creators
//	TODO: Подумать, нужно ли вообще делать Special методы

//	Methods
	LiteralTypes getLiteralType(void) const;
	std::string getLiteralValueAsString(bool shouldThrow = true) const;

//		General getters
	char getChar(void) const;
	wchar_t getWideChar(void) const;
	long_long_t getInteger(void) const;
	unsigned_long_long_t getUnsignedInteger(void) const;
	double getFloat(void) const;
	bool getBoolean(void) const;
	std::string getString(void) const;
	std::wstring getWideString(void) const;

//		Special getters
/*
	sbyte getInt8(void) const;
	sword getInt16(void) const;
	sdword getInt32(void) const;
	long_long_t getInt64(void) const;
	byte getUInt8(void) const;
	word getUInt16(void) const;
	dword getUInt32(void) const;
	unsigned_long_long_t getUInt64(void) const;
	float getFloat32(void) const;
	double getFloat64(void) const;
*/

//		General setters
	void setChar(char value);
	void setWideChar(wchar_t value);
	void setInteger(long_long_t value);
	void setUnsignedInteger(unsigned_long_long_t value);
	void setFloat(double value);
	void setBoolean(bool value);
	void setString(const std::string& value);
	void setWideString(const std::wstring& value);

//		Special setters
/*
	void setInt8(sbyte value);
	void setInt16(sword value);
	void setInt32(sdword value);
	void setInt64(long_long_t value);
	void setUInt8(byte value);
	void setUInt16(word value);
	void setUInt32(dword value);
	void setUInt64(unsigned_long_long_t value);

	void setFloat32(float value);
	void setFloat64(double value);
*/

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(BasicLiteralExpression)

private:
//      Values
    union ValueType
    {
    //  Members
	//		Char value
		char char_value;
	//		Wide char value
		wchar_t wide_char_value;
    //      Boolean value
        bool bool_value;
    //      Long integer value
        long_long_t long_int_value;
    //      Long unsigned value
        unsigned_long_long_t long_unsigned_value;
    //      Real value
        double real_value;
    } m_value;
	std::string m_stringValue;
	std::wstring m_wideStringValue;

	LiteralTypes m_type;
};

///	Strict (specialized for types) literal expression node
class StrictLiteralExpression : public LiteralExpression
{
public:
	StrictLiteralExpression(void);
	explicit StrictLiteralExpression(BasicType::BasicTypes literalType);

//	Static methods
//		Сreators
	static StrictLiteralExpression* createChar(char value);
	static StrictLiteralExpression* createWideChar(wchar_t value);
	static StrictLiteralExpression* createInt8(sbyte value);
	static StrictLiteralExpression* createInt16(sword value);
	static StrictLiteralExpression* createInt32(sdword value);
	static StrictLiteralExpression* createInt64(sqword value);
	static StrictLiteralExpression* createUInt8(byte value);
	static StrictLiteralExpression* createUInt16(word value);
	static StrictLiteralExpression* createUInt32(dword value);
	static StrictLiteralExpression* createUInt64(qword value);

	static StrictLiteralExpression* createFloat32(float value);
	static StrictLiteralExpression* createFloat64(double value);

	static StrictLiteralExpression* createBoolean(bool value);

	static StrictLiteralExpression* createString(const std::string& value);
	static StrictLiteralExpression* createWideString(const std::wstring& value);

//	Methods
	BasicType::BasicTypes getLiteralType(void) const;
	std::string getLiteralValueAsString(bool shouldThrow = true) const;

	static std::string stringToEscapedString(const std::string& str);
	static std::string stringToEscapedString(const std::wstring& wstr);

	char getChar(void) const;
	wchar_t getWideChar(void) const;

//	Testers
	bool isCharacter(void) const;
	bool isInteger(void) const;
	bool isFloat(void) const;
	bool isString(void) const;

//	Getters
	sbyte getInt8(void) const;
	sword getInt16(void) const;
	sdword getInt32(void) const;
	sqword getInt64(void) const;
	byte getUInt8(void) const;
	word getUInt16(void) const;
	dword getUInt32(void) const;
	qword getUInt64(void) const;

	float getFloat32(void) const;
	double getFloat64(void) const;

	bool getBoolean(void) const;
	const std::string& getString(void) const;
	const std::wstring& getWideString(void) const;

//	Setters
	void setChar(char value);
	void setWideChar(wchar_t value);

	void setInt8(sbyte value);
	void setInt16(sword value);
	void setInt32(sdword value);
	void setInt64(sqword value);
	void setUInt8(byte value);
	void setUInt16(word value);
	void setUInt32(dword value);
	void setUInt64(qword value);

	void setFloat32(float value);
	void setFloat64(double value);

	void setBoolean(bool value);
	void setString(const std::string& value);
	void setWideString(const std::wstring& value);

	void setOne();

	void setType(BasicType::BasicTypes type);
/*
	void convertToChar(char value);
	void convertToWideChar(wchar_t value);
	void convertToInt8(sbyte value);
	void convertToInt16(sword value);
	void convertToInt32(sdword value);
	void convertToInt64(sqword value);
	void convertToUInt8(byte value);
	void convertToUInt16(word value);
	void convertToUInt32(dword value);
	void convertToUInt64(qword value);

	void convertToFloat32(float value);
	void convertToFloat64(double value);

	void convertToBoolean(bool value);
	void convertToString(const std::string& value);
	void convertToWideString(const std::wstring& value);
*/

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(StrictLiteralExpression)

private:
	//      Values
	union ValueType
	{
		//  Members
		//		Char value
		char char_value;
		//		Wide char value
		wchar_t wide_char_value;
		//      Boolean value
		bool bool_value;
		//		Int8
		sbyte sbyte_value;
		//		Int16
		sword sword_value;
		//		Int32
		sdword sdword_value;
		//		Int64
		sqword sqword_value;
		//      UInt8
		byte byte_value;
		//      UInt16
		word word_value;
		//      UInt32
		dword dword_value;
		//      UInt64
		qword qword_value;
		//      Float value
		float float_value;
		//      Double value
		double double_value;
	} m_value;
	std::string m_stringValue;
	std::wstring m_wideStringValue;

	BasicType::BasicTypes m_type;
};

/// Compound literal expression node
class CompoundLiteralExpression : public LiteralExpression
{
public:
	CompoundLiteralExpression(void);
	~CompoundLiteralExpression(void);

	int getValueCount(void) const;
	const ExpressionBase& getValue(int index) const;
	ExpressionBase& getValue(int index);

	void addValue(ExpressionBase* value);
	void insertValue(int index, ExpressionBase* value);
	void replaceValue(ExpressionBase& oldValue, ReprisePtr<ExpressionBase> value);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(CompoundLiteralExpression)

protected:
	CompoundLiteralExpression(const CompoundLiteralExpression& other);

private:
	typedef RepriseList<ExpressionBase> ElementsType;
	ElementsType m_elements;
};

///	Reference to variable declaration
class ReferenceExpression : public ExpressionBase
{
public:
	explicit ReferenceExpression(VariableDeclaration& declaration);

	const VariableDeclaration& getReference(void) const;
	VariableDeclaration& getReference(void);
	void setReference(VariableDeclaration* reference);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ReferenceExpression)

private:
	RepriseWeakPtr<VariableDeclaration> m_declaration;
};

///	Reference to subroutine declaration
class SubroutineReferenceExpression : public ExpressionBase
{
public:
	explicit SubroutineReferenceExpression(SubroutineDeclaration& declaration);

	const SubroutineDeclaration& getReference(void) const;
	SubroutineDeclaration& getReference(void);

	void setReference(SubroutineDeclaration* newSubroutine);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SubroutineReferenceExpression)

private:
	RepriseWeakPtr<SubroutineDeclaration> m_declaration;
};

///	Reference to struct member
class StructAccessExpression : public ExpressionBase
{
public:
	StructAccessExpression(ExpressionBase& structPointer, StructMemberDescriptor& member);

	const ExpressionBase& getStructPointerExpression(void) const;
	ExpressionBase& getStructPointerExpression(void);
	void setStructPointerExpression(ReprisePtr<ExpressionBase> expression);

	const StructMemberDescriptor& getMember(void) const;
	StructMemberDescriptor& getMember(void);
	void setMember(StructMemberDescriptor* member);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(StructAccessExpression)

protected:
	StructAccessExpression(const StructAccessExpression& other);

private:
	ReprisePtr<ExpressionBase> m_structPointer;
	RepriseWeakPtr<StructMemberDescriptor> m_member;
};

///	Reference to enum member
class EnumAccessExpression : public ExpressionBase
{
public:
	explicit EnumAccessExpression(EnumMemberDescriptor& member);

	const EnumMemberDescriptor& getMember(void) const;
	EnumMemberDescriptor& getMember(void);
	void setMember(EnumMemberDescriptor* member);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(EnumAccessExpression)

private:
	RepriseWeakPtr<EnumMemberDescriptor> m_member;
};

///	Type cast expression node
//	TODO: Implicit TypeCast?
class TypeCastExpression : public ExpressionBase
{
public:
	TypeCastExpression(TypeBase* cast_to, ExpressionBase* argument, bool implicit = false);

	const TypeBase& getCastType(void) const;
	TypeBase& getCastType(void);
	void setCastType(TypeBase*);

	const ExpressionBase& getCastArgument(void) const;
	ExpressionBase& getCastArgument(void);
	void setCastArgument(ReprisePtr<ExpressionBase> argumentExpression);

	bool isImplicit(void) const;
	void setImplicit(bool implicit);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(TypeCastExpression)

protected:
	TypeCastExpression(const TypeCastExpression& other);

private:
	ReprisePtr<TypeBase> m_cast_to;
	ReprisePtr<ExpressionBase> m_argument;
	bool m_implicit;
};

///	Call expression base node in IR
class CallExpressionBase : public ExpressionBase
{
public:
	int getArgumentCount(void) const;
	const ExpressionBase& getArgument(int index) const;
	ExpressionBase& getArgument(int index);

	void addArgument(ExpressionBase* argument);
	void insertArgument(int index, ExpressionBase* argument);
	void setArgument(int index, ExpressionBase* argument);
	void replaceArgument(ExpressionBase& sourceArgument, ReprisePtr<ExpressionBase> destinationArgument);
	void removeArguments();

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

protected:
	CallExpressionBase(void);
	CallExpressionBase(const CallExpressionBase& other);
	~CallExpressionBase(void);

	std::string dumpStateHelper(const std::string& format) const;

	typedef std::vector<ReprisePtr<ExpressionBase> > ArgumentsType;
	ArgumentsType m_arguments;
};

///	Basic call expression class
class BasicCallExpression : public CallExpressionBase
{
public:
	enum BuiltinCallKind
	{
		//	Unary
		BCK_UNARY_PLUS,
		BCK_UNARY_MINUS,

		BCK_SIZE_OF,			// sizeof() operator

		BCK_TAKE_ADDRESS,		// &
		BCK_DE_REFERENCE,		// *

		//	Binary
		BCK_BINARY_PLUS,		// +
		BCK_BINARY_MINUS,		// -
		BCK_MULTIPLY,			// *
		BCK_DIVISION,			// / 
		BCK_INTEGER_DIVISION,	// div
		BCK_INTEGER_MOD,		// mod (%)

		//	Assign
		BCK_ASSIGN,				// =

		//	Equality
		BCK_LESS,				// <
		BCK_GREATER,			// >
		BCK_LESS_EQUAL,			// <=
		BCK_GREATER_EQUAL,		// >=
		BCK_EQUAL,				// ==
		BCK_NOT_EQUAL,			// !=

		//	Shifts
		BCK_LEFT_SHIFT,			// <<
		BCK_RIGHT_SHIFT,		// >>

		//	Logical
		BCK_LOGICAL_NOT,		// !
		BCK_LOGICAL_AND,		// &&
		BCK_LOGICAL_OR,			// ||

		//	Bitwise
		BCK_BITWISE_NOT,		// ~
		BCK_BITWISE_AND,		// &
		BCK_BITWISE_OR,			// |
		BCK_BITWISE_XOR,		// ^

		//	Special
		BCK_ARRAY_ACCESS,		// []
		BCK_COMMA,				// ,
		BCK_CONDITIONAL		// ? :
	};

//	Static methods
	static std::string builtinCallKindToString(BuiltinCallKind kind);

	explicit BasicCallExpression(BuiltinCallKind builtinCall);
	BasicCallExpression(BuiltinCallKind builtinCall, ExpressionBase* argument);
	BasicCallExpression(BuiltinCallKind builtinCall, ExpressionBase* leftArg, ExpressionBase* rightArg);

	BuiltinCallKind getKind(void) const;
	void setKind(BuiltinCallKind kind);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(BasicCallExpression)

protected:
	static std::string dumpKindFormat(BuiltinCallKind kind);

private:
	BuiltinCallKind m_builtinCall;
};

///	Subroutine call expression 
class SubroutineCallExpression : public CallExpressionBase
{
public:
	explicit SubroutineCallExpression(ExpressionBase* callExpression);

	const ExpressionBase& getCallExpression(void) const;
	ExpressionBase& getCallExpression(void);
	void setCallExpression(ReprisePtr<ExpressionBase> callExpression);

	bool hasExplicitSubroutineDeclaration(void) const;
	const SubroutineDeclaration& getExplicitSubroutineDeclaration(void) const;
	SubroutineDeclaration& getExplicitSubroutineDeclaration(void);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SubroutineCallExpression)

protected:
	SubroutineCallExpression(const SubroutineCallExpression& other);

private:
	ReprisePtr<ExpressionBase> m_callExpression;
};

///	Empty expression
class EmptyExpression : public ExpressionBase
{
public:
	EmptyExpression(void);
//	Static public methods
	static EmptyExpression* empty(void);

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		ExpressionBase implementation

//		RepriseBase members
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(EmptyExpression)
};

}
}

#endif                      // OPS_IR_REPRISE_EXPRESSIONS_H_INCLUDED__
