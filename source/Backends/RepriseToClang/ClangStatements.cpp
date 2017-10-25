/// ClangStatements.cpp
///   Create clang statements from Reprise statements.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created:  9.08.2013

#include "ClangStatements.h"

#include "ClangExpressions.h"
#include "RepriseClangifier.h"

#include "OPS_Core/Helpers.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Expressions.h"
#include "Reprise/Statements.h"

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

#include "llvm/ADT/SmallVector.h"

//#include <string>

using namespace OPS::Backends::Clang::Internal;
using namespace OPS::Reprise;

//using namespace std;

//////////////////////////////////////////////////////////////////////
// Internal

namespace
{
	class StatementWalker : public OPS::Reprise::Service::DeepWalker
	{
	public:
		//
		StatementWalker(
			clang::ASTContext &rASTContext,
			R2CDeclNodes& rR2CDeclNodes);
		//
	// Attributes
	public:
		//
		clang::Stmt* getResultStmt() const;
		//
	// Overrides
	public:
		//
		virtual void visit(
			BlockStatement& rBlockStatement);
		//
		virtual void visit(
			ForStatement& rForStatement);
		//
		virtual void visit(
			WhileStatement& rWhileStatement);
		//
		virtual void visit(
			IfStatement& rIfStatement);
		//
		virtual void visit(
			PlainSwitchStatement& rPlainSwitchStatement);
		//
		virtual void visit(
			GotoStatement& rGotoStatement);
		//
		virtual void visit(
			ReturnStatement& rReturnStatement);
		//
		virtual void visit(
			ExpressionStatement& rExpressionStatement);
		//
		virtual void visit(
			EmptyStatement& rEmptyStatement);
		//
		virtual void visit(
			ASMStatement& rASMStatement);
		//
		virtual void visit(
			RepriseVarDeclWrap& rRepriseVarDeclWrap);
		//
	private:
		//
		clang::ASTContext &m_rASTContext;
		R2CDeclNodes& m_rR2CDeclNodes;
		const clang::TargetInfo& m_rcTargetInfo;
		clang::Stmt* m_pResultStmt;
		//std::map <std::string, clang::LabelDecl*> m_LabelStr2DeclNodes;
	};
}

StatementWalker::StatementWalker(
	clang::ASTContext &rASTContext,
	R2CDeclNodes& rR2CDeclNodes)
	: m_rASTContext(rASTContext),
		m_rR2CDeclNodes(rR2CDeclNodes),
		m_rcTargetInfo(rASTContext.getTargetInfo()),
		m_pResultStmt(0)
{
	//
}

clang::Stmt* StatementWalker::getResultStmt() const
{
	assert(m_pResultStmt != 0);

	return m_pResultStmt;
}

void StatementWalker::visit(
	BlockStatement& rBlockStatement)
{
	llvm::SmallVector <clang::Stmt*, 256> statements;

	if (!rBlockStatement.isEmpty())
	{
		bool bProceed = true;
		BlockStatement::Iterator
			i = rBlockStatement.getFirst(),
			e = rBlockStatement.getLast();
		do
		{
			if (i == e)
				bProceed = false;

			StatementBase& rStatementBase = *i;
			clang::Stmt* pStmt = getClangStmt(
				m_rASTContext, rStatementBase, m_rR2CDeclNodes);
			statements.push_back(pStmt);

			if (i != e)
				++ i;
		}
		while (bProceed);
		
	}    // if (!rBlockStatement.isEmpty())

	m_pResultStmt = new (m_rASTContext) clang::CompoundStmt(
		m_rASTContext,
		statements,
		clang::SourceLocation(),
		clang::SourceLocation());
}

void StatementWalker::visit(
	ForStatement& rForStatement)
{
	ExpressionBase& rExprInit =
		rForStatement.getInitExpression();
	ExpressionBase& rExprFinal =
		rForStatement.getFinalExpression();
	ExpressionBase& rExprStep =
		rForStatement.getStepExpression();
	BlockStatement& rStmtBody =
		rForStatement.getBody();

	clang::Expr* pExprInit = getClangExpr(
		m_rASTContext, rExprInit, m_rR2CDeclNodes, true);
	clang::Expr* pExprFinal = getClangExpr(
		m_rASTContext, rExprFinal, m_rR2CDeclNodes, true);
	clang::Expr* pExprStep = getClangExpr(
		m_rASTContext, rExprStep, m_rR2CDeclNodes, true);
	clang::Stmt* pStmtBody = getClangStmt(
		m_rASTContext, rStmtBody, m_rR2CDeclNodes);

	m_pResultStmt = new (m_rASTContext) clang::ForStmt(
		m_rASTContext,
		pExprInit, pExprFinal, 0, pExprStep, pStmtBody,
		clang::SourceLocation(),
		clang::SourceLocation(),
		clang::SourceLocation());

}    // if (const ForStatement* ...)

void StatementWalker::visit(
	WhileStatement& rWhileStatement)
{
	const bool cbPreCondition =
		rWhileStatement.isPreCondition();
	ExpressionBase& rExpressionCond =
		rWhileStatement.getCondition();
	BlockStatement& rBlockBody =
		rWhileStatement.getBody();

	clang::Expr* pExprCond = getClangExpr(
		m_rASTContext, rExpressionCond, m_rR2CDeclNodes, true);
	clang::Stmt* pStmtBody = getClangStmt(
		m_rASTContext, rBlockBody, m_rR2CDeclNodes);

	if (cbPreCondition)
		m_pResultStmt = new (m_rASTContext) clang::WhileStmt(
			m_rASTContext,
			0,
			pExprCond,
			pStmtBody,
			clang::SourceLocation());
	else
		m_pResultStmt = new (m_rASTContext) clang::DoStmt(
			pStmtBody,
			pExprCond,
			clang::SourceLocation(),
			clang::SourceLocation(),
			clang::SourceLocation());
}

void StatementWalker::visit(
	IfStatement& rIfStatement)
{
	ExpressionBase& rExpressionCond =
		rIfStatement.getCondition();
	BlockStatement& rBlockThen =
		rIfStatement.getThenBody();
	BlockStatement& rBlockElse =
		rIfStatement.getElseBody();

	clang::Expr* pExprCond = getClangExpr(
		m_rASTContext, rExpressionCond, m_rR2CDeclNodes, true);
	clang::Stmt* pStmtThen = getClangStmt(
		m_rASTContext, rBlockThen, m_rR2CDeclNodes);
	clang::Stmt* pStmtElse =
		(rBlockElse.isEmpty() ?
		0 :
		getClangStmt(m_rASTContext, rBlockElse, m_rR2CDeclNodes));

	clang::IfStmt* pIfStmt = new (m_rASTContext) clang::IfStmt(
				m_rASTContext,
				clang::SourceLocation(),
				0,
				pExprCond,
				pStmtThen,
				clang::SourceLocation(),
				pStmtElse);
/*
	if (rIfStatement.hasLabel())
	{
		std::string labelName = rIfStatement.getLabel();
		clang::IdentifierInfo& rIdentifierInfo = m_rASTContext.Idents.get(labelName);
		clang::DeclContext* pDeclContext = m_rASTContext.getTranslationUnitDecl();
		clang::LabelDecl* pLabelDecl = clang::LabelDecl::Create(
					m_rASTContext,
					pDeclContext,
					clang::SourceLocation(),
					&rIdentifierInfo);

		pDeclContext->addDecl(pLabelDecl);
		m_LabelStr2DeclNodes.insert(make_pair(labelName, pLabelDecl));
		m_pResultStmt = new (m_rASTContext) clang::LabelStmt(
					clang::SourceLocation(),
					pLabelDecl,
					pIfStmt);
	}
	else
	{
	*/
		m_pResultStmt = pIfStmt;
	//}

	// m_pResultStmt = handleLabel(repriseStmt.hasLabel, clangStmt*);
}

void StatementWalker::visit(
	PlainSwitchStatement& rPlainSwitchStatement)
{
	ExpressionBase& rExpressionCond =
		rPlainSwitchStatement.getCondition();

	clang::Expr* pExprCond = getClangExpr(
		m_rASTContext, rExpressionCond, m_rR2CDeclNodes, true);

	clang::SwitchStmt* pSwitchStmt = new (m_rASTContext) clang::SwitchStmt(
		m_rASTContext,
		0,	// Reprise does not support variable declaration inside switch condition
		pExprCond);

	llvm::SmallVector <clang::Stmt*, 256> statements;

	for (int i = 0; i < rPlainSwitchStatement.getLabelCount(); i++) {
		clang::SwitchCase* pCaseStmt;

		PlainCaseLabel& rCaseLabel =
			rPlainSwitchStatement.getLabel(i);
		StatementBase& rCasedStatement =
			rCaseLabel.getStatement();

		clang::Stmt* pCasedStmt = getClangStmt(
			m_rASTContext, rCasedStatement, m_rR2CDeclNodes);

		if (!rCaseLabel.isDefault()) {
			StrictLiteralExpression* pLhsValue =
				StrictLiteralExpression::createInt32(rCaseLabel.getValue());

			clang::Expr* pExprLhs = getClangExpr(
				m_rASTContext, *pLhsValue, m_rR2CDeclNodes);
			delete pLhsValue;

			clang::CaseStmt* pCaseStmt_tmp = new (m_rASTContext) clang::CaseStmt(
				pExprLhs,
				0, // Reprise only supports integer values as case conditions
				clang::SourceLocation(),
				clang::SourceLocation(),
				clang::SourceLocation());

			pCaseStmt_tmp->setSubStmt(pCasedStmt);

			pCaseStmt = pCaseStmt_tmp;
		}
		else { // default node
			pCaseStmt = new (m_rASTContext) clang::DefaultStmt(
				clang::SourceLocation(),
				clang::SourceLocation(),
				pCasedStmt);
		}

		statements.push_back(pCaseStmt);
		pSwitchStmt->addSwitchCase(pCaseStmt);
	} // for

	clang::CompoundStmt* pBodyBlock = new (m_rASTContext) clang::CompoundStmt(
		m_rASTContext,
		statements,
		clang::SourceLocation(),
		clang::SourceLocation());
	pSwitchStmt->setBody(pBodyBlock);
	m_pResultStmt = pSwitchStmt;
}

void StatementWalker::visit(
	GotoStatement& rGotoStatement)
{
/*	std::string labelName = rGotoStatement.getPointedStatement()->getLabel();
	std::map <std::string, clang::LabelDecl*>::iterator i =
		m_LabelStr2DeclNodes.find(labelName);
	if (i != m_LabelStr2DeclNodes.end())
		m_pResultStmt = new (m_rASTContext) clang::GotoStmt(
					i->second,
					clang::SourceLocation(),
					clang::SourceLocation());
	else
		OPS_ASSERT(false &&
				   "Unexpected label name");
*/
}

void StatementWalker::visit(
	ReturnStatement& rReturnStatement)
{
	ExpressionBase& rExpressionRet =
		rReturnStatement.getReturnExpression();

	if (rExpressionRet.is_a <EmptyExpression> ())
		m_pResultStmt = new (m_rASTContext) clang::ReturnStmt(
			clang::SourceLocation());
	else
	{
		clang::Expr* pExprRet = getClangExpr(
			m_rASTContext, rExpressionRet, m_rR2CDeclNodes);

		m_pResultStmt = new (m_rASTContext) clang::ReturnStmt(
			clang::SourceLocation(),
			pExprRet,
			0);
	}
}

void StatementWalker::visit(
	ExpressionStatement& rExpressionStatement)
{
	ExpressionBase& rExpressionBase =
		rExpressionStatement.get();
		
	m_pResultStmt = getClangExpr(
		m_rASTContext, rExpressionBase, m_rR2CDeclNodes, true);

}

void StatementWalker::visit(
	EmptyStatement& rEmptyStatement)
{
	m_pResultStmt = new (m_rASTContext) clang::NullStmt(
		clang::SourceLocation());
}

void StatementWalker::visit(
	ASMStatement& rASMStatement)
{
	llvm::StringRef* pAsmString = new llvm::StringRef(rASMStatement.getASMString());

	clang::StringLiteral* pAsmStringLiteral = clang::StringLiteral::Create(
		m_rASTContext, // ??????
		*pAsmString,
		clang::StringLiteral::StringKind::Ascii,
		false, // pascal (?)
		clang::QualType(),
		clang::SourceLocation());

	switch(rASMStatement.getASMType()) {
	case ASMStatement::InlineASMType::ASMTP_MS:
		m_pResultStmt = new (m_rASTContext) clang::MSAsmStmt(
							m_rASTContext,
							clang::SourceLocation(),
							clang::SourceLocation(),
							true,  // Reprise supports simple asm() only
							false, // volatility. Default value??
							llvm::NoneType::None,     // isn't stored in Reprise
							0, 0,  // num of inputs and outputs. Reprise does not support these
							llvm::NoneType::None, llvm::NoneType::None,  // aren't stored in Reprise
							*pAsmString,
							llvm::NoneType::None,	   // clobbers?
							clang::SourceLocation());
		break;
	case ASMStatement::InlineASMType::ASMTP_GCC:
		m_pResultStmt = new (m_rASTContext) clang::GCCAsmStmt(
							m_rASTContext,
							clang::SourceLocation(),
							true,  // Reprise supports simple asm() only
							false, // volatility. Default value??
							0, 0,  // num of inputs and outputs. Reprise does not support these
							0, 0,  // names, constraints?
							0,     // expressions?
							pAsmStringLiteral,
							0, 0,  // clobbers?
							clang::SourceLocation());
		break;
	}
}

void StatementWalker::visit(
	RepriseVarDeclWrap& rRepriseVarDeclWrap)
{
	// контекст формируется в другом visitorе, толку
	// от объявления переменных в блоке мало
}


//////////////////////////////////////////////////////////////////////
// getClangStmt()

clang::Stmt* OPS::Backends::Clang::Internal::getClangStmt(
	clang::ASTContext &rASTContext,
	StatementBase& rStatementBase,
	R2CDeclNodes& rR2CDeclNodes)
{
	StatementWalker walker(rASTContext, rR2CDeclNodes);
	rStatementBase.accept(walker);
	clang::Stmt* pResultStmt = walker.getResultStmt();

	return pResultStmt;
}

// End of File
