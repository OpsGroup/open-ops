#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

void RepriseDeepWalker::visit(ProgramUnit& program)
{
	for (int index = 0; index < program.getUnitCount(); ++index)
	{
		program.getUnit(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(TranslationUnit& unit)
{
	unit.getGlobals().accept(*this);
}

void RepriseDeepWalker::visit(ProgramFragment& fragment)
{
	if (!fragment.isEmpty())
	{
		BlockStatement::Iterator iter = fragment.getFirstIterator();

		// обход [fragment.getFirst(), fragment.getLast())
		for ( ; iter != fragment.getLastIterator() && iter.isValid(); )
		{
			iter->accept(*this);
			iter.goNext();
		}

		// проход по fragment.getLast()
		iter->accept(*this);
	}
}

void RepriseDeepWalker::visit(Declarations& declarations)
{
	for(Declarations::Iterator it = declarations.getFirst(); it.isValid(); ++it)
	{
		it->accept(*this);
	}
}

void RepriseDeepWalker::visit(VariableDeclaration& variable)
{
	variable.getType().accept(*this);
	variable.getInitExpression().accept(*this);
}

void RepriseDeepWalker::visit(TypeDeclaration& type)
{
	type.getType().accept(*this);
}

void RepriseDeepWalker::visit(SubroutineDeclaration& subroutine)
{
	subroutine.getType().accept(*this);
	if (subroutine.hasImplementation())
	{
		subroutine.getDeclarations().accept(*this);
		subroutine.getBodyBlock().accept(*this);
	}
}

void RepriseDeepWalker::visit(BlockStatement& block)
{
	for (BlockStatement::Iterator iter = block.getFirst(); iter.isValid(); iter.goNext())
	{
		iter->accept(*this);
	}
}

void RepriseDeepWalker::visit(ForStatement& forStmt)
{
	forStmt.getInitExpression().accept(*this);
	forStmt.getFinalExpression().accept(*this);
	forStmt.getStepExpression().accept(*this);
	forStmt.getBody().accept(*this);
}

void RepriseDeepWalker::visit(WhileStatement& whileStmt)
{
	whileStmt.getCondition().accept(*this);
	whileStmt.getBody().accept(*this);
}

void RepriseDeepWalker::visit(IfStatement& ifStmt)
{
	ifStmt.getCondition().accept(*this);
	ifStmt.getThenBody().accept(*this);
	ifStmt.getElseBody().accept(*this);
}

void RepriseDeepWalker::visit(PlainCaseLabel& caseLabel)
{
	OPS_UNUSED(caseLabel);
}

void RepriseDeepWalker::visit(PlainSwitchStatement& switchStmt)
{
	switchStmt.getCondition().accept(*this);
	switchStmt.getBody().accept(*this);

	for(int i = 0; i < switchStmt.getLabelCount(); ++i)
	{
		switchStmt.getLabel(i).accept(*this);
	}
}

void RepriseDeepWalker::visit(GotoStatement&)
{
}

void RepriseDeepWalker::visit(ReturnStatement& returnStmt)
{
	returnStmt.getReturnExpression().accept(*this);
}

void RepriseDeepWalker::visit(ExpressionStatement& expr)
{
	expr.get().accept(*this);
}

void RepriseDeepWalker::visit(ASMStatement&)
{
}

void RepriseDeepWalker::visit(EmptyStatement&)
{
}


void DeepWalker::visit(Canto::HirCBasicType&)
{
}

void DeepWalker::visit(Canto::HirFBasicType&)
{
}

void DeepWalker::visit(Canto::HirFArrayType& arrayType)
{
	arrayType.getBaseType().accept(*this);
	arrayType.getShape().accept(*this);
}

void RepriseDeepWalker::visit(BasicType&)
{
}

void RepriseDeepWalker::visit(PtrType& ptr)
{
	ptr.getPointedType().accept(*this);
}

void RepriseDeepWalker::visit(TypedefType& typeDef)
{
	typeDef.getBaseType().accept(*this);
}

void RepriseDeepWalker::visit(ArrayType& arrayType)
{
	arrayType.getBaseType().accept(*this);
}

void RepriseDeepWalker::visit(StructMemberDescriptor& structMember)
{
	structMember.getType().accept(*this);
}

void RepriseDeepWalker::visit(StructType& structType)
{
	for (int index = 0; index < structType.getMemberCount(); ++index)
	{
		structType.getMember(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(EnumMemberDescriptor&)
{
}

void RepriseDeepWalker::visit(EnumType& enumType)
{
	for (int index = 0; index < enumType.getMemberCount(); ++index)
	{
		enumType.getMember(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(ParameterDescriptor& parameterDescr)
{
	parameterDescr.getType().accept(*this);
}

void RepriseDeepWalker::visit(SubroutineType& subroutineType)
{
	subroutineType.getReturnType().accept(*this);
	for (int index = 0; index < subroutineType.getParameterCount(); ++index)
	{
		subroutineType.getParameter(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(DeclaredType&)
{
}

void RepriseDeepWalker::visit(VectorType& vectorType)
{
    vectorType.getBaseType().accept(*this);
}

void RepriseDeepWalker::visit(BasicLiteralExpression&)
{
}

void RepriseDeepWalker::visit(StrictLiteralExpression&)
{
}

void RepriseDeepWalker::visit(CompoundLiteralExpression& literalExpr)
{
	for (int index = 0; index < literalExpr.getValueCount(); ++index)
	{
		literalExpr.getValue(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(ReferenceExpression&)
{
}

void RepriseDeepWalker::visit(SubroutineReferenceExpression&)
{
}

void RepriseDeepWalker::visit(StructAccessExpression& structAccess)
{
	structAccess.getStructPointerExpression().accept(*this);
}

void RepriseDeepWalker::visit(EnumAccessExpression&)
{
}

void RepriseDeepWalker::visit(TypeCastExpression& typeCast)
{
	typeCast.getCastType().accept(*this);
	typeCast.getCastArgument().accept(*this);
}

void RepriseDeepWalker::visit(BasicCallExpression& basic)
{
	for (int index = 0; index < basic.getArgumentCount(); ++index)
	{
		basic.getArgument(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(SubroutineCallExpression& subroutine)
{
	subroutine.getCallExpression().accept(*this);
	for (int index = 0; index < subroutine.getArgumentCount(); ++index)
	{
		subroutine.getArgument(index).accept(*this);
	}
}

void RepriseDeepWalker::visit(EmptyExpression&)
{
}

void DeepWalker::visit(Canto::HirCCallExpression& basic)
{
	for (int index = 0; index < basic.getArgumentCount(); ++index)
	{
		basic.getArgument(index).accept(*this);
	}
}

void DeepWalker::visit(Canto::HirCStatementExpression& stmtExpr)
{
    stmtExpr.getSubStatements().accept(*this);
}

void DeepWalker::visit(Canto::HirCVariableInitStatement& varInit)
{
    varInit.getInitExpression().accept(*this);
}

void DeepWalker::visit(Canto::HirBreakStatement&)
{
}

void DeepWalker::visit(Canto::HirContinueStatement&)
{
}

void DeepWalker::visit(Canto::HirFAltResultExpression&)
{
}

void DeepWalker::visit(Canto::HirFAsteriskExpression&)
{
}

void DeepWalker::visit(Canto::HirFDimensionExpression& dimensionExpr)
{
	dimensionExpr.getLowerBound().accept(*this);
	dimensionExpr.getUpperBound().accept(*this);
}

void DeepWalker::visit(Canto::HirFArrayShapeExpression& shapeExpr)
{
	int rank = shapeExpr.getRank();
	for(int i = 0; i < rank; ++i)
	{
		shapeExpr.getDimension(i).accept(*this);
	}
}

void DeepWalker::visit(Canto::HirFArrayIndexRangeExpression& indexRangeExpr)
{
	for(int index = 0; index < indexRangeExpr.getArgumentCount(); ++index)
	{
		indexRangeExpr.getArgument(index).accept(*this);
	}
}

void DeepWalker::visit(Canto::HirFImpliedDoExpression& doExpr)
{
	doExpr.getInitExpression().accept(*this);
	doExpr.getFinalExpression().accept(*this);
	doExpr.getStepExpression().accept(*this);
	doExpr.getBodyExpression().accept(*this);
}

void DeepWalker::visit(Canto::HirFArgumentPairExpression& argPairExpr)
{
	argPairExpr.getValue().accept(*this);
}

void DeepWalker::visit(Canto::HirFIntrinsicCallExpression& callExpr)
{
	for (int index = 0; index < callExpr.getArgumentCount(); ++index)
	{
		callExpr.getArgument(index).accept(*this);
	}
}

void DeepWalker::visit(Canto::HirFIntrinsicReferenceExpression&)
{
}

}
}
}
