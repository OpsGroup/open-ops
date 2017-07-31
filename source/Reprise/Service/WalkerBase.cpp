#include <typeinfo>
#include "Reprise/Service/WalkerBase.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

void RepriseWalkerBase::visit(ProgramUnit& program)
{
	unexpectedVisit(program);
}

void RepriseWalkerBase::visit(TranslationUnit& unit)
{
	unexpectedVisit(unit);
}

void RepriseWalkerBase::visit(Declarations& declarations)
{
	unexpectedVisit(declarations);
}

void RepriseWalkerBase::visit(VariableDeclaration& variable)
{
	unexpectedVisit(variable);
}

void RepriseWalkerBase::visit(TypeDeclaration& type)
{
	unexpectedVisit(type);
}

void RepriseWalkerBase::visit(SubroutineDeclaration& subroutine)
{
	unexpectedVisit(subroutine);
}

void RepriseWalkerBase::visit(BlockStatement& block)
{
	unexpectedVisit(block);
}

void RepriseWalkerBase::visit(ForStatement& forStmt)
{
	unexpectedVisit(forStmt);
}

void RepriseWalkerBase::visit(WhileStatement& whileStmt)
{
	unexpectedVisit(whileStmt);
}

void RepriseWalkerBase::visit(IfStatement& ifStmt)
{
	unexpectedVisit(ifStmt);
}

void RepriseWalkerBase::visit(PlainCaseLabel& caseLabel)
{
	unexpectedVisit(caseLabel);
}

void RepriseWalkerBase::visit(PlainSwitchStatement& switchStmt)
{
	unexpectedVisit(switchStmt);
}

void RepriseWalkerBase::visit(GotoStatement& gotoStmt)
{
	unexpectedVisit(gotoStmt);
}

void RepriseWalkerBase::visit(ReturnStatement& returnStmt)
{
	unexpectedVisit(returnStmt);
}

void RepriseWalkerBase::visit(ExpressionStatement& expr)
{
	unexpectedVisit(expr);
}

void RepriseWalkerBase::visit(ASMStatement& asmStatement)
{
	unexpectedVisit(asmStatement);
}

void RepriseWalkerBase::visit(EmptyStatement& emptyStmt)
{
	unexpectedVisit(emptyStmt);
}

void WalkerBase::visit(Canto::HirCBasicType& type)
{
	unexpectedVisit(type);
}

void WalkerBase::visit(Canto::HirFBasicType& type)
{
	unexpectedVisit(type);
}

void WalkerBase::visit(Canto::HirFArrayType& arrayType)
{
	unexpectedVisit(arrayType);
}

void RepriseWalkerBase::visit(BasicType& type)
{
	unexpectedVisit(type);
}

void RepriseWalkerBase::visit(PtrType& ptr)
{
	unexpectedVisit(ptr);
}

void RepriseWalkerBase::visit(TypedefType& typeDef)
{
	unexpectedVisit(typeDef);
}

void RepriseWalkerBase::visit(ArrayType& arrayType)
{
	unexpectedVisit(arrayType);
}

void RepriseWalkerBase::visit(StructMemberDescriptor& structMember)
{
	unexpectedVisit(structMember);
}

void RepriseWalkerBase::visit(StructType& structType)
{
	unexpectedVisit(structType);
}

void RepriseWalkerBase::visit(EnumMemberDescriptor& enumMemberDesc)
{
	unexpectedVisit(enumMemberDesc);
}

void RepriseWalkerBase::visit(EnumType& enumType)
{
	unexpectedVisit(enumType);
}

void RepriseWalkerBase::visit(ParameterDescriptor& parameterDescr)
{
	unexpectedVisit(parameterDescr);
}

void RepriseWalkerBase::visit(SubroutineType& subroutineType)
{
	unexpectedVisit(subroutineType);
}

void RepriseWalkerBase::visit(DeclaredType& declaredType)
{
	unexpectedVisit(declaredType);
}

void RepriseWalkerBase::visit(VectorType& vectorType)
{
    unexpectedVisit(vectorType);
}

void RepriseWalkerBase::visit(BasicLiteralExpression& literalExpr)
{
	unexpectedVisit(literalExpr);
}

void RepriseWalkerBase::visit(StrictLiteralExpression& literalExpr)
{
	unexpectedVisit(literalExpr);
}

void RepriseWalkerBase::visit(CompoundLiteralExpression& literalExpr)
{
	unexpectedVisit(literalExpr);
}

void RepriseWalkerBase::visit(ReferenceExpression& referenceExpr)
{
	unexpectedVisit(referenceExpr);
}

void RepriseWalkerBase::visit(SubroutineReferenceExpression& subroutineReferenceExpr)
{
	unexpectedVisit(subroutineReferenceExpr);
}

void RepriseWalkerBase::visit(StructAccessExpression& structAccess)
{
	unexpectedVisit(structAccess);
}

void RepriseWalkerBase::visit(EnumAccessExpression& enumAccess)
{
	unexpectedVisit(enumAccess);
}

void RepriseWalkerBase::visit(TypeCastExpression& typeCast)
{
	unexpectedVisit(typeCast);
}

void RepriseWalkerBase::visit(BasicCallExpression& basic)
{
	unexpectedVisit(basic);
}

void RepriseWalkerBase::visit(SubroutineCallExpression& subroutine)
{
	unexpectedVisit(subroutine);
}

void RepriseWalkerBase::visit(EmptyExpression& empty)
{
	unexpectedVisit(empty);
}

void WalkerBase::visit(Canto::HirCCallExpression& basic)
{
	unexpectedVisit(basic);
}

void WalkerBase::visit(Canto::HirCStatementExpression& stmtExpr)
{
    unexpectedVisit(stmtExpr);
}

void WalkerBase::visit(Canto::HirCVariableInitStatement& variableInit)
{
    unexpectedVisit(variableInit);
}

void WalkerBase::visit(Canto::HirBreakStatement& breakStmt)
{
    unexpectedVisit(breakStmt);
}

void WalkerBase::visit(Canto::HirContinueStatement& continueStmt)
{
    unexpectedVisit(continueStmt);
}


void WalkerBase::visit(Canto::HirFAltResultExpression& expr)
{
	unexpectedVisit(expr);
}

void WalkerBase::visit(Canto::HirFAsteriskExpression& expr)
{
	unexpectedVisit(expr);
}

void WalkerBase::visit(Canto::HirFDimensionExpression& expr)
{
	unexpectedVisit(expr);
}

void WalkerBase::visit(Canto::HirFArrayShapeExpression& shapeExpr)
{
	unexpectedVisit(shapeExpr);
}

void WalkerBase::visit(Canto::HirFArrayIndexRangeExpression& indexRangeExpr)
{
	unexpectedVisit(indexRangeExpr);
}

void WalkerBase::visit(Canto::HirFImpliedDoExpression& doExpr)
{
	unexpectedVisit(doExpr);
}

void WalkerBase::visit(Canto::HirFArgumentPairExpression& argPairExpr)
{
	unexpectedVisit(argPairExpr);
}

void WalkerBase::visit(Canto::HirFIntrinsicCallExpression& callExpr)
{
	unexpectedVisit(callExpr);
}

void WalkerBase::visit(Canto::HirFIntrinsicReferenceExpression& refExpr)
{
	unexpectedVisit(refExpr);
}

void RepriseWalkerBase::unexpectedVisit(const RepriseBase &node)
{
    throw OPS::RuntimeError(std::string("Unexpected node type in RepriseWalkerBase: ") + typeid(node).name());
}

}
}
}
