#include "include/RenameDistributionArrayVisitor.h"

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Service;

RenameDistributionArrayVisitor::RenameDistributionArrayVisitor()
{
	m_startRenameNames = false;
}

#define RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(STMT_TYPE)	\
void RenameDistributionArrayVisitor::visit(STMT_TYPE& stmt)	\
{	\
	int res = processStmt(stmt);	\
	if (res == -1)	\
	{	\
		m_startRenameNames = false;	\
		return;	\
	}	\
\
	if (res == 1)	\
		m_startRenameNames = true;	\
	DeepWalker::visit(stmt);	\
}

void RenameDistributionArrayVisitor::visit(ForStatement& stmt)
{	
	int res = processStmt(stmt);	
	if (res == -1)	
	{	
		m_startRenameNames = false;	
		return;	
	}	

	if (res == 1)	
		m_startRenameNames = true;	
	DeepWalker::visit(stmt);
}

RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(BlockStatement)

RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(WhileStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(IfStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(PlainSwitchStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(GotoStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(ReturnStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(Canto::HirBreakStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(Canto::HirContinueStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(ExpressionStatement)
RENAME_DISTIRIBUTION_ARRAY_VISITOR_VISIT_STMT(EmptyStatement)
void RenameDistributionArrayVisitor::visit(ReferenceExpression& expr)
{	
	if (&expr.getReference() == oldArrayDecl && m_startRenameNames == true)
		expr.setReference(newArrayDecl);
	DeepWalker::visit(expr);
}

int RenameDistributionArrayVisitor::processStmt(StatementBase& stmt)
{
	if (&stmt == firstStmt)
		return 1;

	if (&stmt == lastStmt)
		return -1;
	return 0;
}
