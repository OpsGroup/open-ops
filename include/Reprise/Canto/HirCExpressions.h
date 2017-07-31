#ifndef OPS_IR_REPRISE_CANTO_HIRCEXPRESSIONS_H_INCLUDED__
#define OPS_IR_REPRISE_CANTO_HIRCEXPRESSIONS_H_INCLUDED__

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Reprise
{
namespace Canto
{

///	HIR Call expression node
class HirCCallExpression : public OPS::Reprise::CallExpressionBase
{
public:
	enum HirCOperatorKind
	{
		//	Unary
		HIRC_UNARY_PLUS,
		HIRC_UNARY_MINUS,

		HIRC_SIZE_OF,					// sizeof() operator

		HIRC_TAKE_ADDRESS,				// &
		HIRC_DE_REFERENCE,				// *
	
		HIRC_PREFIX_PLUS_PLUS,			// ++()
		HIRC_POSTFIX_PLUS_PLUS,			// ()++
		HIRC_PREFIX_MINUS_MINUS,		// --()
		HIRC_POSTFIX_MINUS_MINUS,		// ()--

		//	Binary
		HIRC_BINARY_PLUS,				// +
		HIRC_BINARY_MINUS,				// -
		HIRC_MULTIPLY,					// *
		HIRC_DIVISION,					// / 
//		HIRC_INTEGER_DIVISION,			// div
		HIRC_INTEGER_MOD,				// mod (%)

		//	Assign
		HIRC_ASSIGN,					// =

		HIRC_PLUS_ASSIGN,				// +=
		HIRC_MINUS_ASSIGN,				// -=
		HIRC_MULTIPLY_ASSIGN,			// *=
		HIRC_DIVISION_ASSIGN,			// /=
		HIRC_MOD_ASSIGN,				// %=
		HIRC_LSHIFT_ASSIGN,				// <<=
		HIRC_RSHIFT_ASSIGN,				// >>=

		HIRC_BAND_ASSIGN,				// &=
		HIRC_BOR_ASSIGN,				// |=
		HIRC_BXOR_ASSIGN,				// ^=

		//	Equality
		HIRC_LESS,						// <
		HIRC_GREATER,					// >
		HIRC_LESS_EQUAL,				// <=
		HIRC_GREATER_EQUAL,				// >=
		HIRC_EQUAL,						// ==
		HIRC_NOT_EQUAL,					// !=

		//	Shifts
		HIRC_LEFT_SHIFT,				// <<
		HIRC_RIGHT_SHIFT,				// >>

		//	Logical
		HIRC_LOGICAL_NOT,				// !
		HIRC_LOGICAL_AND,				// &&
		HIRC_LOGICAL_OR,				// ||

		//	Bitwise
		HIRC_BITWISE_NOT,				// ~
		HIRC_BITWISE_AND,				// &
		HIRC_BITWISE_OR,				// |
		HIRC_BITWISE_XOR,				// ^

		//	Special
		HIRC_ARRAY_ACCESS,				// []
		HIRC_COMMA,						// ,
		HIRC_CONDITIONAL				// ? :
	};

	//	Static methods
	static std::string callKindToString(HirCOperatorKind operatorKind);

	explicit HirCCallExpression(HirCOperatorKind operatorKind);

	HirCOperatorKind getKind(void) const;
//	std::string getPlainString(void) const;

	// ++(), ()++, --(), ()--
	bool isUnaryComplexAssign() const;
	// arg1 op= arg2
	bool isBinaryComplexAssign() const;

//		ExpressionBase implementation
	virtual bool isEqual(const ExpressionBase& exprNode) const;

//		ClonableMix implementation
	virtual HirCCallExpression* clone(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
protected:
	static std::string dumpKindFormat(HirCOperatorKind kind);

private:
	HirCOperatorKind m_operatorKind;
};

class HirCStatementExpression : public OPS::Reprise::ExpressionBase
{
public:
    explicit HirCStatementExpression(BlockStatement* subStatements);

    const BlockStatement& getSubStatements() const;
    BlockStatement& getSubStatements();
    void setSubStatements(BlockStatement* subStatements);

//  ExpressionBase implementation
    virtual bool isEqual(const ExpressionBase& exprNode) const;

//  ClonableMix implementation
    virtual HirCStatementExpression* clone(void) const;

//  RepriseBase implementation
    virtual int getChildCount(void) const;
    virtual RepriseBase& getChild(int index);
    virtual std::string dumpState(void) const;

    OPS_DEFINE_VISITABLE()
    OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
    ReprisePtr<BlockStatement> m_subStatements;
};


}
}
}

#endif                      // OPS_IR_REPRISE_CANTO_HIRCEXPRESSIONS_H_INCLUDED__
