#include "Analysis/AliasAnalysis/DistributionValues.h"

namespace OPS
{
namespace AliasAnalysis
{	

void DistributionValues::Build(const ExprName& procCallName, ControlFlowGraph* pCfg, ExprCallEx* exprCallEx, const StatementBase* pStmt, const ExprName& exprName, const SetAbstractMemoryCell& samcSrc)
{
	//Правильная нумерация
	ProperNumbering properNumbering;
	ProperNumberingBuild(pCfg, properNumbering);	

	//Поиск достигающих определений
	StatementBase* curStmt = const_cast<StatementBase*>(pStmt);	
	ListStatementEx bufStmtOwner;
	if (PushNextStmt(curStmt, 0, pCfg, properNumbering, bufStmtOwner))
		UpdateActivationRecord(exprCallEx, exprName, samcSrc);
	while (bufStmtOwner.size()>0)
	{
		//Получение выражения
		StatementBase* pStmtNext = const_cast<StatementBase*>(bufStmtOwner.front().first);
		int StmtOwner = bufStmtOwner.front().second;
		bufStmtOwner.pop_front();

		//Поиск генератора в выражении
		DistributionWalker distributionWalker(exprName, samcSrc, procCallName, exprCallEx, m_PointersTable, m_MemManager);
		distributionWalker.Run(pStmtNext);		
		if (distributionWalker.IsExistGenerator())
			DeleteStmt(StmtOwner, bufStmtOwner);
		else
			if (PushNextStmt(pStmtNext, StmtOwner, pCfg, properNumbering, bufStmtOwner))
				UpdateActivationRecord(exprCallEx, exprName, samcSrc);
	}
}

void DistributionValues::ProperNumberingBuild(ControlFlowGraph* pCfg, ProperNumbering& properNumbering)
{
	int k = 1;
	ControlFlowGraph::StatementVector::const_iterator itCurr = pCfg->getStatementVector().begin(),
													  itEnd  = pCfg->getStatementVector().end();
	for(; itCurr != itEnd; ++itCurr)
	{		
		StatementBase* pStmtNext = const_cast<StatementBase*>(*itCurr);
		std::string sty = pStmtNext->dumpState();							//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		properNumbering[pStmtNext] = k;
		k++;
	}
}

bool DistributionValues::PushNextStmt(StatementBase* currentStmt, int currentStmtOwner, ControlFlowGraph* pCfg, ProperNumbering properNumbering, ListStatementEx& bufStmt)
{
	bool flag = false;
	int currentStmtNum = properNumbering[currentStmt];
	const ControlFlowGraph::StatementList& list = (*pCfg)[currentStmt];
	ListStatementEx tmpList;
	ControlFlowGraph::StatementList::const_iterator it = list.begin();
	for(; it != list.end(); ++it)
	{
		StatementBase* pElemList = const_cast<StatementBase*>(*it);	
		if (pElemList)
		{
			int elemListNum = properNumbering[pElemList];			
			if (elemListNum > currentStmtNum)
			{
				if (pElemList->is_a<BlockStatement>())
					tmpList.push_back(PairStatement(pElemList, elemListNum));
				else 
					tmpList.push_back(PairStatement(pElemList, currentStmtOwner));
			}
			else if (pElemList->is_a<BlockStatement>() && (currentStmtOwner == 0) && (elemListNum > 1))
			{
				//вернуть последний путь из предка
				flag = GetNextStmt(pElemList, currentStmtNum, currentStmtOwner, pCfg, properNumbering, tmpList) && flag;
			}
			if (elemListNum == 1) //конец программы
				flag = true;      //хотя бы 1 ветка доходит до конца
		}
		else flag = true;      //хотя бы 1 ветка доходит до конца
	}
	bufStmt.insert(bufStmt.begin(), tmpList.begin(), tmpList.end());
	return flag;
}

bool DistributionValues::GetNextStmt(StatementBase* pElemList, int currentStmtNum, int currentStmtOwner, ControlFlowGraph* pCfg, ProperNumbering properNumbering, ListStatementEx& tmpList)
{
	bool flag = false;
	const ControlFlowGraph::StatementList& listOwn = (*pCfg)[pElemList];			
	StatementBase* pElemOwn = const_cast<StatementBase*>(*(--listOwn.end()));
	if (pElemOwn)
	{
		int elemListOwnNum = properNumbering[pElemOwn];
		if (elemListOwnNum > currentStmtNum)
		{
			if (pElemOwn->is_a<BlockStatement>())
				tmpList.push_back(PairStatement(pElemOwn, elemListOwnNum));
			else 
				tmpList.push_back(PairStatement(pElemOwn, currentStmtOwner));
		}
		else if (pElemOwn->is_a<BlockStatement>() &&(currentStmtOwner == 0) && (elemListOwnNum > 1))
		{
			GetNextStmt(pElemOwn, currentStmtNum, currentStmtOwner, pCfg, properNumbering, tmpList);
		}
		if (elemListOwnNum == 1) //конец программы
			flag = true;      //хотя бы 1 ветка доходит до конца
	}
	else 
		flag = true;      //хотя бы 1 ветка доходит до конца
	return flag;
}

void DistributionValues::UpdateActivationRecord(ExprCallEx* exprCallEx, const ExprName& exprName, const SetAbstractMemoryCell& samcSrc)
{
	//конец программы
	ActivationRecord *pActivationRecord = exprCallEx->GetActivationRecord();
	ValuesPointerMap::const_iterator itActual, itGlobal;
	itGlobal = pActivationRecord->InParam.globalParam.find(exprName);
	itActual = pActivationRecord->InParam.actualParam.find(exprName);
	if (itGlobal != pActivationRecord->InParam.globalParam.end())
	{	//глобальная переменная
		SetAbstractMemoryCell samc_glob = pActivationRecord->OutParam.globalParam[exprName];
		samc_glob.insert(samc_glob.end(), samcSrc.begin(), samcSrc.end());
		pActivationRecord->OutParam.globalParam[exprName] = samc_glob;
	}			
	else if (itActual != pActivationRecord->InParam.actualParam.end())
	{	//фактический параметр
		SetAbstractMemoryCell samc_act = pActivationRecord->OutParam.actualParam[exprName];
		samc_act.insert(samc_act.end(), samcSrc.begin(), samcSrc.end());
		pActivationRecord->OutParam.actualParam[exprName] = samc_act;
	}
}

void DistributionValues::DeleteStmt(int currentStmtOwner, ListStatementEx& bufStmt)
{
	ListStatementEx::iterator iter = bufStmt.begin();
	while (iter != bufStmt.end())
	{
		if (iter->second == currentStmtOwner)
		{
			ListStatementEx::iterator next_iter = iter;
			next_iter++;
			bufStmt.erase(iter);
			iter = next_iter;
		}
		else iter++;
	}
}


//****************** DistributionWalker ******************
DistributionWalker::DistributionWalker(const ExprName& exprName, const SetAbstractMemoryCell& samcSrc
									  ,const ExprName& procCallName, ExprCallEx* exprCallEx
									  ,EntriesPointersTable* aPointersTable, MemoryManager* aMemManager)
	: m_ExprCallEx(exprCallEx), m_PointersTable(aPointersTable), m_MemManager(aMemManager)
{
	m_SamcSrc = samcSrc;
	m_ExprName = exprName;
	m_ProcCallName = procCallName;
	isGenerator = false;
}

bool DistributionWalker::Run(StatementBase* stmt)
{
	isGenerator= false;
	if (stmt->is_a<ExpressionStatement>()) 
		DeepWalker::visit(stmt->cast_to<ExpressionStatement>());
	else if (stmt->is_a<ForStatement>())
		visit(stmt->cast_to<ForStatement>());
	else if (stmt->is_a<WhileStatement>())
		visit(stmt->cast_to<WhileStatement>());
	else if (stmt->is_a<IfStatement>())
		visit(stmt->cast_to<IfStatement>());
	else if (stmt->is_a<SwitchStatement>())
		visit(stmt->cast_to<SwitchStatement>());
	else if (stmt->is_a<GotoStatement>())
		DeepWalker::visit(stmt->cast_to<GotoStatement>());
	else if (stmt->is_a<ReturnStatement>())
		visit(stmt->cast_to<ReturnStatement>());
	else return false;
	return true;
}

bool DistributionWalker::Parse(RepriseBase* node)
{
	isGenerator= false;
	if (node->is_a<ReferenceExpression>()) 
		visit(node->cast_to<ReferenceExpression>()); //
	else if (node->is_a<BasicCallExpression>())
		visit(node->cast_to<BasicCallExpression>()); //
	else if (node->is_a<SubroutineCallExpression>())
		visit(node->cast_to<SubroutineCallExpression>()); //
	else if (node->is_a<BasicLiteralExpression>())
		DeepWalker::visit(node->cast_to<BasicLiteralExpression>());
	else if (node->is_a<StrictLiteralExpression>())
		DeepWalker::visit(node->cast_to<StrictLiteralExpression>());
	else if (node->is_a<CompoundLiteralExpression>())
		DeepWalker::visit(node->cast_to<CompoundLiteralExpression>());
	else if (node->is_a<SubroutineReferenceExpression>())
		DeepWalker::visit(node->cast_to<SubroutineReferenceExpression>());
	else if (node->is_a<StructAccessExpression>())
		DeepWalker::visit(node->cast_to<StructAccessExpression>());
	else if (node->is_a<EnumAccessExpression>())
		DeepWalker::visit(node->cast_to<EnumAccessExpression>());
	else if (node->is_a<TypeCastExpression>())
		DeepWalker::visit(node->cast_to<TypeCastExpression>());
	else if (node->is_a<ExpressionStatement>()) 
		DeepWalker::visit(node->cast_to<ExpressionStatement>());
	else if (node->is_a<ForStatement>())
		visit(node->cast_to<ForStatement>());
	else if (node->is_a<WhileStatement>())
		visit(node->cast_to<WhileStatement>());
	else if (node->is_a<IfStatement>())
		visit(node->cast_to<IfStatement>());
	else if (node->is_a<SwitchStatement>())
		visit(node->cast_to<SwitchStatement>());
	else if (node->is_a<GotoStatement>())
		DeepWalker::visit(node->cast_to<GotoStatement>());
	else if (node->is_a<ReturnStatement>())
		visit(node->cast_to<ReturnStatement>());
	else return false;
	return true;
}

void DistributionWalker::visit(ReferenceExpression& referenceExpression)
{	
	if (m_ExprName == referenceExpression.getReference().getName())
		m_PointersTable->UnionSAMC(&referenceExpression, m_SamcSrc);
}

void DistributionWalker::visit(BasicCallExpression& basicCallExpr)
{
	if ((basicCallExpr.getKind() == BasicCallExpression::BCK_ASSIGN) && (basicCallExpr.getArgument(0).is_a<ReferenceExpression>()))
	{
		ReferenceExpression& referenceExpression = basicCallExpr.getArgument(0).cast_to<ReferenceExpression>();
		if (m_ExprName == referenceExpression.getReference().getName())
			isGenerator = true;
	}
	DeepWalker::visit(basicCallExpr);
}

void DistributionWalker::visit(SubroutineCallExpression& exprCall)
{
	ExprName subProcName = exprCall.getExplicitSubroutineDeclaration().getName();
	VariablesClassifier* pVarClassif = m_MemManager->GetVariablesClassifier();
	//глобальные переменные
	bool isModified = pVarClassif->IsModifiedVar(subProcName, m_ExprName);
	if (isModified || (pVarClassif->IsReadOnlyVar(subProcName, m_ExprName)))
	{
		CallGraphEx* pCallGraphEx = m_MemManager->GetCallGraphEx();
		ExprCallEx *subExprCallEx = pCallGraphEx->GetSubProcExprCallEx(m_ProcCallName, &exprCall);
		if(subExprCallEx)
		{
			ActivationRecord *pActivationRecord = subExprCallEx->GetActivationRecord();
			SetAbstractMemoryCell samc_glob = pActivationRecord->InParam.globalParam[m_ExprName];
			samc_glob.insert(samc_glob.end(), m_SamcSrc.begin(), m_SamcSrc.end());
			pActivationRecord->InParam.globalParam[m_ExprName] = samc_glob;
			if (isModified) isGenerator = true;
		}
	}
	DeepWalker::visit(exprCall);
}

void DistributionWalker::visit(ReturnStatement& returnStatement)
{
	ExpressionBase &expressionBase = returnStatement.getReturnExpression();
	if (expressionBase.is_a<ReferenceExpression>())
	{
		ReferenceExpression &exprData= expressionBase.cast_to<ReferenceExpression>();
		if (m_ExprName==exprData.getReference().getName())
		{
			m_PointersTable->UnionSAMC(&exprData, m_SamcSrc);
			ActivationRecord *pActivationRecord = m_ExprCallEx->GetActivationRecord();
			pActivationRecord->ProcResult.insert(pActivationRecord->ProcResult.end(), m_SamcSrc.begin(), m_SamcSrc.end());
		}
	}
}

void DistributionWalker::visit(ForStatement& forStatement)
{
	this->Parse(&forStatement.getInitExpression());	
	this->Parse(&forStatement.getFinalExpression());	
	this->Parse(&forStatement.getStepExpression());
	//тело цикла не обрабатывается
}

void DistributionWalker::visit(WhileStatement& whileStatement)
{
	this->Parse(&whileStatement.getCondition());
	//тело цикла не обрабатывается
}

void DistributionWalker::visit(IfStatement& ifStatement)
{
	this->Parse(&ifStatement.getCondition());	
	//тело if не обрабатывается
}

void DistributionWalker::visit(SwitchStatement& switchStatement)
{
	this->Parse(&switchStatement.getCondition());
	//тело не обрабатывается
}

}
}
