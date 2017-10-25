#include "Analysis/AliasAnalysis/ProcAnalyzer.h"

namespace OPS
{
namespace AliasAnalysis
{

void ProcAnalyzer::Run(ExprName procName, ExprCallEx *exprCallEx)
{
	Initialize(exprCallEx);
	GirthGraph(procName, exprCallEx);
}

void ProcAnalyzer::Initialize(ExprCallEx* exprCallEx)
{
	m_MemManager->SetBlock(const_cast<BlockStatement*>(exprCallEx->GetProcBlock()));		
}

void ProcAnalyzer::GirthGraph(const ExprName& procName, ExprCallEx* exprCallEx)
{ 
	ControlFlowGraph* pCfg = new ControlFlowGraph(*exprCallEx->GetProcBlock());
	StatementBase* pStmtFirst = const_cast<StatementBase*>(*(pCfg->getStatementVector().begin()));
	if (pStmtFirst->is_a<BlockStatement>())
		pStmtFirst = const_cast<StatementBase*>(*(++(pCfg->getStatementVector().begin())));		
	m_ExprAnalyzer->Initialize(procName, exprCallEx, pStmtFirst, pCfg, exprCallEx->GetActivationRecord()->InParam.globalParam, exprCallEx->GetActivationRecord()->InParam.actualParam);
	ControlFlowGraph::StatementVector::const_iterator itCurr = pCfg->getStatementVector().begin(),
													  itEnd  = pCfg->getStatementVector().end();
	for(; itCurr != itEnd; ++itCurr)
	{
		StatementBase* pStmtNext = const_cast<StatementBase*>(*itCurr);

		ProcWalker procWalker(m_MemManager);
		if (procWalker.Run(pStmtNext))
		{			
			if (procWalker.isExistCall())
			{
				ExprName resultName = "";
				if (procWalker.getResultExprData())
					resultName = procWalker.getResultExprData()->getReference().getName();
				SetAbstractMemoryCell samcResult = this->Parse(procName, exprCallEx, pStmtNext, pCfg, procWalker.getSubCallExpr(), resultName);
				if (procWalker.getOffset()!=0)
				{
					SetAbstractMemoryCell temp = m_MemManager->GetSAMCExprValue(samcResult, procWalker.getOffset());
					m_ExprAnalyzer->AddEntriesPointers(procWalker.getResultExprData(), temp);
				}
				else
					if (procWalker.getResultExprData())
						m_ExprAnalyzer->AddEntriesPointers(procWalker.getResultExprData(), samcResult);
			}
			else
				m_ExprAnalyzer->Parse(procName, exprCallEx, pStmtNext, pCfg);
		}
	}
	delete pCfg;        
}

SetAbstractMemoryCell ProcAnalyzer::Parse(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg, SubroutineCallExpression* subExprCall, const ExprName& resultName)
{
	SetAbstractMemoryCell samcResult;
	ExprName subProcName = subExprCall->getExplicitSubroutineDeclaration().getName();
	ExprCallEx *subExprCallEx = m_CallGraphEx->GetSubProcExprCallEx(procName, subExprCall);
	if (subExprCallEx && subExprCallEx->GetProcBlock() && subExprCallEx->GetPointersTable())
	{
		VariablesList convertParam = m_MemManager->GetVariablesClassifier()->GetConvertParam(subProcName);
		SetParam(exprCallEx, subExprCallEx, subExprCall, convertParam);			
		Run(subProcName, subExprCallEx);
		ValuesPointerMap modifActualParam;
		CheckParam(resultName, subExprCallEx->GetActivationRecord()->OutParam.globalParam, modifActualParam);
		m_ExprAnalyzer->Initialize(procName, exprCallEx, pStmt, pCfg, subExprCallEx->GetActivationRecord()->OutParam.globalParam, modifActualParam);
		samcResult = subExprCallEx->GetActivationRecord()->ProcResult;
	}	
	return samcResult;
}

void ProcAnalyzer::SetParam(ExprCallEx* exprCallEx, ExprCallEx* subExprCallEx, SubroutineCallExpression* subExprCall, const VariablesList& convertParam)
{	
	int paramsCount = subExprCall->getArgumentCount();
	if ((int)convertParam.size() == paramsCount)
	{		
		for (int i = 0; i < paramsCount; i++)
			if ((subExprCall->getArgument(i).is_a<ReferenceExpression>()) &&
				(m_MemManager->GetVariablesClassifier()->IsPointer(subExprCall->getArgument(i).cast_to<ReferenceExpression>())))
			{
					subExprCallEx->GetActivationRecord()->InParam.actualParam[convertParam[i]] = 
							exprCallEx->GetPointersTable()->GetSAMC(subExprCall->getArgument(i).cast_ptr<ReferenceExpression>());
			}
	}
}

void ProcAnalyzer::CheckParam(const ExprName& resultName, ValuesPointerMap& globalParam, ValuesPointerMap& actualParam)
{
	ValuesPointerMap::iterator itGlob = globalParam.find(resultName);
	if (itGlob != globalParam.end())
		globalParam.erase(itGlob);
	ValuesPointerMap::iterator itAct = actualParam.find(resultName);
	if (itAct != actualParam.end())
		actualParam.erase(itAct);
}

BlockStatement* ProcAnalyzer::GetSubProcBody(const ExprName& subProcName, TranslationUnit* aTranslationUnit)
{
	for (Declarations::SubrIterator iter = aTranslationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		if (iter->getName() == subProcName 
			&& iter->hasImplementation())
		{
			return &iter->getBodyBlock();						
		}
	}
	return NULL;
}

//****************** ProcWalker ******************

ProcWalker::ProcWalker(MemoryManager* aMemManager)
	: m_MemManager(aMemManager)
{
	m_IsCall = false;
	m_CallExpr = 0;
	m_ResultExpr = 0;
	m_Offset = 0;
}

bool ProcWalker::Run(StatementBase* statementBase)
{
	if (statementBase->is_a<ExpressionStatement>())
		DeepWalker::visit(statementBase->cast_to<ExpressionStatement>());
	else if (statementBase->is_a<ForStatement>())
		DeepWalker::visit(statementBase->cast_to<ForStatement>());
	else if (statementBase->is_a<WhileStatement>())
		DeepWalker::visit(statementBase->cast_to<WhileStatement>());
	else if (statementBase->is_a<IfStatement>())
		DeepWalker::visit(statementBase->cast_to<IfStatement>());
	else if (statementBase->is_a<SwitchStatement>())
		DeepWalker::visit(statementBase->cast_to<SwitchStatement>());
	else if (statementBase->is_a<GotoStatement>())
		DeepWalker::visit(statementBase->cast_to<GotoStatement>());
	else if (statementBase->is_a<ReturnStatement>())
		DeepWalker::visit(statementBase->cast_to<ReturnStatement>());
	else return false;
	return true;
}

void ProcWalker::visit(SubroutineCallExpression& callExpr)
{
	m_CallExpr = &callExpr;
	m_IsCall = true;
}

void ProcWalker::visit(BasicCallExpression& basicCallExpression)
{ 
	if (!HasTemplateSimpleAssign(basicCallExpression))
		if (!HasTemplateAssignFuncPlusImm(basicCallExpression))
			DeepWalker::visit(basicCallExpression);
}

bool ProcWalker::HasTemplateSimpleAssign(BasicCallExpression& basicCallExpression)
{	
	if ((basicCallExpression.getKind() == BasicCallExpression::BCK_ASSIGN) &&
	    (basicCallExpression.getArgument(0).is_a<ReferenceExpression>()) && 
		(basicCallExpression.getArgument(1).is_a<SubroutineCallExpression>()))
	{
			if (m_MemManager->GetVariablesClassifier()->IsPointer(basicCallExpression.getArgument(0).cast_to<ReferenceExpression>()))
			{				
				m_ResultExpr = basicCallExpression.getArgument(0).cast_ptr<ReferenceExpression>();
				m_CallExpr = basicCallExpression.getArgument(1).cast_ptr<SubroutineCallExpression>();
				m_IsCall = true;
				return true;
			}
	}
	return false;
}

bool ProcWalker::HasTemplateAssignFuncPlusImm(BasicCallExpression& basicCallExpression)
{
	BasicCallExpression::BuiltinCallKind callKind = basicCallExpression.getKind();
	if (callKind == BasicCallExpression::BCK_ASSIGN)
	{
		if ( basicCallExpression.getArgument(0).is_a<ReferenceExpression>() && 
			 basicCallExpression.getArgument(1).is_a<BasicCallExpression>())
		{
			if (m_MemManager->GetVariablesClassifier()->IsPointer(basicCallExpression.getArgument(0).cast_to<ReferenceExpression>()))
			{
				BasicCallExpression& basicCallExprRight = basicCallExpression.getArgument(1).cast_to<BasicCallExpression>();
				BasicCallExpression::BuiltinCallKind callKindRight = basicCallExprRight.getKind();
				if (basicCallExprRight.getArgumentCount() == 2)
				{
					if ((callKindRight == BasicCallExpression::BCK_BINARY_PLUS) || (callKindRight == BasicCallExpression::BCK_BINARY_MINUS))
					{
						if ((((basicCallExprRight.getArgument(0).is_a<SubroutineCallExpression>()) && (basicCallExprRight.getArgument(1).is_a<LiteralExpression>())) ||
							((basicCallExprRight.getArgument(1).is_a<SubroutineCallExpression>()) && (basicCallExprRight.getArgument(0).is_a<LiteralExpression>())))
							)
						{
							SubroutineCallExpression* pSubCallExpression;
							LiteralExpression* pLiteralExpression;
							if ((basicCallExprRight.getArgument(0).is_a<SubroutineCallExpression>()) && (basicCallExprRight.getArgument(1).is_a<LiteralExpression>()))
							{
								pSubCallExpression = basicCallExprRight.getArgument(0).cast_ptr<SubroutineCallExpression>();
								pLiteralExpression = basicCallExprRight.getArgument(1).cast_ptr<LiteralExpression>();
							}
							else 
							{
								pSubCallExpression = basicCallExprRight.getArgument(1).cast_ptr<SubroutineCallExpression>();
								pLiteralExpression = basicCallExprRight.getArgument(0).cast_ptr<LiteralExpression>();
							}
							m_ResultExpr = basicCallExpression.getArgument(0).cast_ptr<ReferenceExpression>();							
							m_CallExpr = pSubCallExpression;
							char iAction = (callKindRight == BasicCallExpression::BCK_BINARY_PLUS)? 1: -1;
							m_Offset = m_MemManager->GetIntLiteralValue(pLiteralExpression, iAction);											
							m_IsCall = true;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

//****************** SearchProcCall ******************

void SearchProcCall::visit(SubroutineCallExpression& callExpr)
{	
	if (callExpr.getExplicitSubroutineDeclaration().getName() == m_ProcName)
		m_CallExpr = &callExpr;
}
		
}
}
