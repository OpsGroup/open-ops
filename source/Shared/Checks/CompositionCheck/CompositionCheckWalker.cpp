#include "Shared/Checks/CompositionCheck/CompositionCheckWalker.h"

namespace OPS
{
namespace Shared
{
namespace Checks
{

CompositionCheckWalker::CompositionCheckWalker(CompositionCheckObjects acceptableObjects, bool firstVisitCallIsInternal /* = false */)
: m_compositionCheckResult(true)
, m_acceptableObjects(acceptableObjects)
, m_visitCallIsInternal(firstVisitCallIsInternal)
{
}

bool CompositionCheckWalker::compositionCheckResult() const
{
	return m_compositionCheckResult;
}

#define CHECK_FOR_A_PRESENCE(CCO_Object) \
if (m_visitCallIsInternal && !m_acceptableObjects.contains(CCO_Object)) \
{ \
	m_compositionCheckResult = false; \
	return; \
}

#define STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL() \
if (!m_compositionCheckResult) \
{ \
	return; \
} \
if (!m_visitCallIsInternal) \
{ \
	m_visitCallIsInternal = true; \
} \
DeepWalker::visit(repriseObject);

#define CHECK_STATEMENT_FOR_A_LABEL() \
if (m_visitCallIsInternal && !m_acceptableObjects.contains(CompositionCheckObjects::CCOT_Label) && repriseObject.hasLabel()) \
{ \
	m_compositionCheckResult = false; \
	return; \
}

void CompositionCheckWalker::visit(ProgramUnit& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(TranslationUnit& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ProgramFragment& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}


void CompositionCheckWalker::visit(Declarations& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(VariableDeclaration& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(TypeDeclaration& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(SubroutineDeclaration& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(BlockStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_BlockStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ForStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_ForStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(WhileStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_WhileStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(IfStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_IfStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(GotoStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_GotoStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ReturnStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_ReturnStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(HirBreakStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_BreakStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(HirContinueStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_ContinueStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ExpressionStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_ExpressionStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(EmptyStatement& repriseObject)
{
	CHECK_STATEMENT_FOR_A_LABEL();
	CHECK_FOR_A_PRESENCE(CompositionCheckObjects::CCOT_EmptyStatement);
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(HirCBasicType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(BasicType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(PtrType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ArrayType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(Reprise::VectorType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(StructMemberDescriptor& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(StructType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(EnumMemberDescriptor& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(EnumType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ParameterDescriptor& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(SubroutineType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(DeclaredType& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(BasicLiteralExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(StrictLiteralExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(CompoundLiteralExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(ReferenceExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(SubroutineReferenceExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(StructAccessExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(EnumAccessExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(TypeCastExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(BasicCallExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(SubroutineCallExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(EmptyExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

void CompositionCheckWalker::visit(HirCCallExpression& repriseObject)
{
	STOP_IF_CHECK_FAILED_AND_MARK_OTHER_VISIT_CALLS_AS_INTERNAL();
}

}	// OPS
}	// Shared
}	// Checks
