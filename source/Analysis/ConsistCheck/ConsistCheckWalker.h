#ifndef _CONSIST_CHECK_WALKER_H_INCLUDED_
#define _CONSIST_CHECK_WALKER_H_INCLUDED_

#include "Analysis/ConsistCheck/Conditions.h"

#include <Reprise/Service/DeepWalker.h>

namespace OPS
{

namespace Analysis
{

class ConsistCheckWalker
	: public Reprise::Service::DeepWalker
{
public:
	ConsistCheckWalker(const ConditionList& conditions);

	bool getResult() const;

	virtual void visit(Reprise::ProgramUnit& );
	virtual void visit(Reprise::TranslationUnit& );

	virtual void visit(Reprise::ProgramFragment& );

	virtual void visit(Reprise::Declarations& );
	virtual void visit(Reprise::VariableDeclaration& variableDeclaration);
	virtual void visit(Reprise::TypeDeclaration& );
	virtual void visit(Reprise::SubroutineDeclaration& );

	virtual void visit(Reprise::BlockStatement& blockStatement);
	virtual void visit(Reprise::ForStatement& forStatement);
	virtual void visit(Reprise::WhileStatement& whileStatement);
	virtual void visit(Reprise::IfStatement& ifStatement);
	virtual void visit(Reprise::PlainCaseLabel& plainCaseLabel);
	virtual void visit(Reprise::PlainSwitchStatement& plainSwitchStatement);
	virtual void visit(Reprise::GotoStatement& gotoStatement);
	virtual void visit(Reprise::ReturnStatement& returnStatement);
	virtual void visit(Reprise::ExpressionStatement& expressionStatement);
	virtual void visit(Reprise::EmptyStatement& emptyStatement);

	virtual void visit(Reprise::BasicType& basicType);
	virtual void visit(Reprise::PtrType& ptrType);
	virtual void visit(Reprise::TypedefType& typedefType);
	virtual void visit(Reprise::ArrayType& arrayType);
	virtual void visit(Reprise::StructMemberDescriptor& structMemberDescriptor);
	virtual void visit(Reprise::StructType& structType);
	virtual void visit(Reprise::EnumMemberDescriptor& enumMemberDescriptor);
	virtual void visit(Reprise::EnumType& enumType);
	virtual void visit(Reprise::ParameterDescriptor& parameterDescriptor);
	virtual void visit(Reprise::SubroutineType& subroutineType);
	virtual void visit(Reprise::DeclaredType& declaredType);
	virtual void visit(Reprise::VectorType& vectorType);

	virtual void visit(Reprise::Canto::HirCBasicType& );
	virtual void visit(Reprise::Canto::HirFBasicType& );
	virtual void visit(Reprise::Canto::HirFArrayType& );

	virtual void visit(Reprise::BasicLiteralExpression& basicLiteralExpression);
	virtual void visit(Reprise::StrictLiteralExpression& strictLiteralExpression);
	virtual void visit(Reprise::CompoundLiteralExpression& compoundLiteralExpression);
	virtual void visit(Reprise::ReferenceExpression& referenceExpression);
	virtual void visit(Reprise::SubroutineReferenceExpression& subroutineReferenceExpression);
	virtual void visit(Reprise::StructAccessExpression& structAccessExpression);
	virtual void visit(Reprise::EnumAccessExpression& enumAccessExpression);
	virtual void visit(Reprise::TypeCastExpression& typeCastExpression);
	virtual void visit(Reprise::BasicCallExpression& basicCallExpression);
	virtual void visit(Reprise::SubroutineCallExpression& subroutineCallExpression);
	virtual void visit(Reprise::EmptyExpression& emptyExpression);

	virtual void visit(Reprise::Canto::HirCCallExpression& );
    virtual void visit(Reprise::Canto::HirBreakStatement& breakStatement);
    virtual void visit(Reprise::Canto::HirContinueStatement& continueStatement);
    virtual void visit(Reprise::Canto::HirFAltResultExpression& );
	virtual void visit(Reprise::Canto::HirFAsteriskExpression& );
	virtual void visit(Reprise::Canto::HirFDimensionExpression& );
	virtual void visit(Reprise::Canto::HirFArrayShapeExpression& );
	virtual void visit(Reprise::Canto::HirFArrayIndexRangeExpression& );
	virtual void visit(Reprise::Canto::HirFImpliedDoExpression& );
	virtual void visit(Reprise::Canto::HirFArgumentPairExpression& );
	virtual void visit(Reprise::Canto::HirFIntrinsicCallExpression& );
	virtual void visit(Reprise::Canto::HirFIntrinsicReferenceExpression& );

private:
	void validate(Reprise::RepriseBase& repriseObject);
	void fail();

private:
	bool m_result;
	ConditionList m_conditions;
	bool m_isFirstVisit;
};

}

}

#endif
