#ifndef OPS_IR_REPRISE_SERVICE_DEEPWALKER_H_INCLUDED__
#define OPS_IR_REPRISE_SERVICE_DEEPWALKER_H_INCLUDED__

#include "OPS_Core/Visitor.h"

#include "Reprise/Service/WalkerBase.h"
#include "Reprise/ProgramFragment.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

/**
	Reprise "Deep Walker"
	Base class to make Reprise passes. Visits every node in Reprise tree. You should override some
	of visit() methods and add logic.
*/
class RepriseDeepWalker : public WalkerBase,
                   public OPS::Visitor<ProgramFragment>
{
public:

	void visit(ProgramUnit&);
	void visit(TranslationUnit&);

	void visit(ProgramFragment&);

	void visit(Declarations&);
	void visit(VariableDeclaration&);
	void visit(TypeDeclaration&);
	void visit(SubroutineDeclaration&);

	void visit(BlockStatement&);
	void visit(ForStatement&);
	void visit(WhileStatement&);
	void visit(IfStatement&);
	void visit(PlainCaseLabel&);
	void visit(PlainSwitchStatement&);
	void visit(GotoStatement&);
	void visit(ReturnStatement&);
	void visit(ExpressionStatement&);
	void visit(ASMStatement&);
	void visit(EmptyStatement&);

	void visit(BasicType&);
	void visit(PtrType&);
	void visit(TypedefType&);
	void visit(ArrayType&);
	void visit(StructMemberDescriptor& structMember);
	void visit(StructType&);
	void visit(EnumMemberDescriptor&);
	void visit(EnumType&);
	void visit(ParameterDescriptor&);
	void visit(SubroutineType&);
	void visit(DeclaredType&);
	void visit(VectorType&);

	void visit(BasicLiteralExpression&);
	void visit(StrictLiteralExpression&);
	void visit(CompoundLiteralExpression&);
	void visit(ReferenceExpression&);
	void visit(SubroutineReferenceExpression&);
	void visit(StructAccessExpression&);
	void visit(EnumAccessExpression&);
	void visit(TypeCastExpression&);
	void visit(BasicCallExpression&);
	void visit(SubroutineCallExpression&);
	void visit(EmptyExpression&);
};

class DeepWalker : public RepriseDeepWalker
{
public:
	using RepriseDeepWalker::visit;

	void visit(Canto::HirCBasicType&);
	void visit(Canto::HirFBasicType&);
	void visit(Canto::HirFArrayType&);

	void visit(Canto::HirCCallExpression&);
    void visit(Canto::HirCStatementExpression&);
	void visit(Canto::HirCVariableInitStatement&);
	void visit(Canto::HirBreakStatement&);
	void visit(Canto::HirContinueStatement&);

	void visit(Canto::HirFAltResultExpression&);
	void visit(Canto::HirFAsteriskExpression&);
	void visit(Canto::HirFDimensionExpression&);
	void visit(Canto::HirFArrayShapeExpression&);
	void visit(Canto::HirFArrayIndexRangeExpression&);
	void visit(Canto::HirFImpliedDoExpression&);
	void visit(Canto::HirFArgumentPairExpression&);
	void visit(Canto::HirFIntrinsicCallExpression&);
	void visit(Canto::HirFIntrinsicReferenceExpression&);
};

}
}
}

#endif
