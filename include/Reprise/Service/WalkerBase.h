#ifndef OPS_IR_REPRISE_SERVICE_WALKERBASE_H_INCLUDED__
#define OPS_IR_REPRISE_SERVICE_WALKERBASE_H_INCLUDED__

#include "OPS_Core/Visitor.h"

#include "Reprise/Units.h"
#include "Reprise/Declarations.h"
#include "Reprise/Canto/HirCExpressions.h"
#include "Reprise/Canto/HirCStatements.h"
#include "Reprise/Canto/HirFExpressions.h"
#include "Reprise/Canto/HirCTypes.h"
#include "Reprise/Canto/HirFTypes.h"

namespace OPS
{
namespace Reprise
{
namespace Service
{

class RepriseWalkerInterface : public OPS::BaseVisitor,
	public OPS::Visitor<ProgramUnit>,
	public OPS::Visitor<TranslationUnit>,

	public OPS::Visitor<Declarations>,
	public OPS::Visitor<VariableDeclaration>,
	public OPS::Visitor<TypeDeclaration>,
	public OPS::Visitor<SubroutineDeclaration>,

	public OPS::Visitor<BlockStatement>,
	public OPS::Visitor<ForStatement>,
	public OPS::Visitor<WhileStatement>,
	public OPS::Visitor<IfStatement>,
	public OPS::Visitor<PlainCaseLabel>,
	public OPS::Visitor<PlainSwitchStatement>,
	public OPS::Visitor<GotoStatement>,
	public OPS::Visitor<ReturnStatement>,
	public OPS::Visitor<ExpressionStatement>,
	public OPS::Visitor<ASMStatement>,
	public OPS::Visitor<EmptyStatement>,

	public OPS::Visitor<BasicType>,
	public OPS::Visitor<PtrType>,
	public OPS::Visitor<TypedefType>,
	public OPS::Visitor<ArrayType>,
	public OPS::Visitor<StructMemberDescriptor>,
	public OPS::Visitor<StructType>,
	public OPS::Visitor<EnumMemberDescriptor>,
	public OPS::Visitor<EnumType>,
	public OPS::Visitor<ParameterDescriptor>,
	public OPS::Visitor<SubroutineType>,
	public OPS::Visitor<DeclaredType>,
    public OPS::Visitor<VectorType>,

	public OPS::Visitor<BasicLiteralExpression>,
	public OPS::Visitor<StrictLiteralExpression>,
	public OPS::Visitor<CompoundLiteralExpression>,
	public OPS::Visitor<ReferenceExpression>,
	public OPS::Visitor<SubroutineReferenceExpression>,
	public OPS::Visitor<StructAccessExpression>,
	public OPS::Visitor<EnumAccessExpression>,
	public OPS::Visitor<TypeCastExpression>,
	public OPS::Visitor<BasicCallExpression>,
	public OPS::Visitor<SubroutineCallExpression>,
	public OPS::Visitor<EmptyExpression>,
	public OPS::NonCopyableMix
{
};

class WalkerInterface : public RepriseWalkerInterface,
        public OPS::Visitor<Canto::HirCBasicType>,
        public OPS::Visitor<Canto::HirFBasicType>,
        public OPS::Visitor<Canto::HirFArrayType>,
        public OPS::Visitor<Canto::HirCCallExpression>,
        public OPS::Visitor<Canto::HirCVariableInitStatement>,
        public OPS::Visitor<Canto::HirBreakStatement>,
        public OPS::Visitor<Canto::HirContinueStatement>,
        public OPS::Visitor<Canto::HirFAltResultExpression>,
        public OPS::Visitor<Canto::HirFAsteriskExpression>,
        public OPS::Visitor<Canto::HirFDimensionExpression>,
        public OPS::Visitor<Canto::HirFArrayShapeExpression>,
        public OPS::Visitor<Canto::HirFArrayIndexRangeExpression>,
        public OPS::Visitor<Canto::HirFImpliedDoExpression>,
        public OPS::Visitor<Canto::HirFArgumentPairExpression>,
        public OPS::Visitor<Canto::HirFIntrinsicCallExpression>,
        public OPS::Visitor<Canto::HirFIntrinsicReferenceExpression>

{
};

class RepriseWalkerBase : public RepriseWalkerInterface
{
public:
	void visit(ProgramUnit&);
	void visit(TranslationUnit&);

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
	void visit(ASMStatement&);
	void visit(EmptyExpression&);
protected:
    virtual void unexpectedVisit(const RepriseBase& node);
};

class WalkerBase : public RepriseWalkerBase,
        public OPS::Visitor<Canto::HirCBasicType>,
        public OPS::Visitor<Canto::HirFBasicType>,
        public OPS::Visitor<Canto::HirFArrayType>,
        public OPS::Visitor<Canto::HirCCallExpression>,
        public OPS::Visitor<Canto::HirCStatementExpression>,
        public OPS::Visitor<Canto::HirCVariableInitStatement>,
        public OPS::Visitor<Canto::HirBreakStatement>,
        public OPS::Visitor<Canto::HirContinueStatement>,
        public OPS::Visitor<Canto::HirFAltResultExpression>,
        public OPS::Visitor<Canto::HirFAsteriskExpression>,
        public OPS::Visitor<Canto::HirFDimensionExpression>,
        public OPS::Visitor<Canto::HirFArrayShapeExpression>,
        public OPS::Visitor<Canto::HirFArrayIndexRangeExpression>,
        public OPS::Visitor<Canto::HirFImpliedDoExpression>,
        public OPS::Visitor<Canto::HirFArgumentPairExpression>,
        public OPS::Visitor<Canto::HirFIntrinsicCallExpression>,
        public OPS::Visitor<Canto::HirFIntrinsicReferenceExpression>
{
public:
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
