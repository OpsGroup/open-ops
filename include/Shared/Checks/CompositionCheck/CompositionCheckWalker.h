#ifndef _COMPOSITION_CHECK_H_WALKER_INCLUDED_
#define _COMPOSITION_CHECK_H_WALKER_INCLUDED_

#include "Shared/Checks/CompositionCheck/CompositionCheckObjects.h"

#include "Reprise/Reprise.h"
#include "Reprise/Canto/HirCTypes.h"
#include "Reprise/Canto/HirFExpressions.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ProgramFragment.h"

namespace OPS
{
namespace Shared
{
namespace Checks
{

using OPS::Reprise::Service::DeepWalker;

using OPS::Reprise::ProgramUnit;
using OPS::Reprise::TranslationUnit;

using OPS::Reprise::ProgramFragment;

using OPS::Reprise::Declarations;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::TypeDeclaration;
using OPS::Reprise::SubroutineDeclaration;

using OPS::Reprise::BlockStatement;
using OPS::Reprise::ForStatement;
using OPS::Reprise::WhileStatement;
using OPS::Reprise::IfStatement;
using OPS::Reprise::GotoStatement;
using OPS::Reprise::ReturnStatement;
using OPS::Reprise::ExpressionStatement;
using OPS::Reprise::EmptyStatement;

using OPS::Reprise::Canto::HirCBasicType;
using OPS::Reprise::BasicType;
using OPS::Reprise::PtrType;
using OPS::Reprise::ArrayType;
using OPS::Reprise::StructMemberDescriptor;
using OPS::Reprise::StructType;
using OPS::Reprise::EnumMemberDescriptor;
using OPS::Reprise::EnumType;
using OPS::Reprise::ParameterDescriptor;
using OPS::Reprise::SubroutineType;
using OPS::Reprise::DeclaredType;

using OPS::Reprise::BasicLiteralExpression;
using OPS::Reprise::StrictLiteralExpression;
using OPS::Reprise::CompoundLiteralExpression;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::SubroutineReferenceExpression;
using OPS::Reprise::StructAccessExpression;
using OPS::Reprise::EnumAccessExpression;
using OPS::Reprise::TypeCastExpression;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::SubroutineCallExpression;
using OPS::Reprise::EmptyExpression;
using OPS::Reprise::Canto::HirCCallExpression;
using OPS::Reprise::Canto::HirBreakStatement;
using OPS::Reprise::Canto::HirContinueStatement;


class CompositionCheckWalker: public DeepWalker
{
public:
	CompositionCheckWalker(CompositionCheckObjects acceptableObjects, bool firstVisitCallIsInternal = false);

	bool compositionCheckResult() const;

	void visit(ProgramUnit& repriseObject);
	void visit(TranslationUnit& repriseObject);

	void visit(ProgramFragment& repriseObject);

	void visit(Declarations& repriseObject);
	void visit(VariableDeclaration& repriseObject);
	void visit(TypeDeclaration& repriseObject);
	void visit(SubroutineDeclaration& repriseObject);

	void visit(BlockStatement& repriseObject);
	void visit(ForStatement& repriseObject);
	void visit(WhileStatement& repriseObject);
	void visit(IfStatement& repriseObject);
	void visit(GotoStatement& repriseObject);
	void visit(ReturnStatement& repriseObject);
	void visit(ExpressionStatement& repriseObject);
	void visit(EmptyStatement& repriseObject);

	void visit(HirCBasicType& repriseObject);
	void visit(BasicType& repriseObject);
	void visit(PtrType& repriseObject);
	void visit(ArrayType& repriseObject);
	void visit(Reprise::VectorType& repriseObject);
	void visit(StructMemberDescriptor& repriseObject);
	void visit(StructType& repriseObject);
	void visit(EnumMemberDescriptor& repriseObject);
	void visit(EnumType& repriseObject);
	void visit(ParameterDescriptor& repriseObject);
	void visit(SubroutineType& repriseObject);
	void visit(DeclaredType& repriseObject);

	void visit(BasicLiteralExpression& repriseObject);
	void visit(StrictLiteralExpression& repriseObject);
	void visit(CompoundLiteralExpression& repriseObject);
	void visit(ReferenceExpression& repriseObject);
	void visit(SubroutineReferenceExpression& repriseObject);
	void visit(StructAccessExpression& repriseObject);
	void visit(EnumAccessExpression& repriseObject);
	void visit(TypeCastExpression& repriseObject);
	void visit(BasicCallExpression& repriseObject);
	void visit(SubroutineCallExpression& repriseObject);
	void visit(EmptyExpression& repriseObject);
	void visit(HirCCallExpression& repriseObject);
    void visit(HirBreakStatement& repriseObject);
    void visit(HirContinueStatement& repriseObject);
private:
	bool m_compositionCheckResult;
	CompositionCheckObjects m_acceptableObjects;
	bool m_visitCallIsInternal;
};

}	// OPS
}	// Shared
}	// Checks

#endif	// _COMPOSITION_CHECK_H_WALKER_INCLUDED_
