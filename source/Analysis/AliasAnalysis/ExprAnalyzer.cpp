#include "Analysis/AliasAnalysis/ExprAnalyzer.h"

namespace OPS
{
namespace AliasAnalysis
{

void ExprAnalyzer::Initialize(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg, 
							  const ValuesPointerMap& globalParam, const ValuesPointerMap& actualParam)
{
	SetVarParam(procName, exprCallEx, pStmt, pCfg);
	//Глобальные переменные
	ValuesPointerMap::const_iterator itCurrGlob = globalParam.begin();
	for(; itCurrGlob != globalParam.end(); ++itCurrGlob)
	{
		ExprName exprName = itCurrGlob->first;
		if (m_ExprCallEx->GetProcBlock()->getChildCount()>0)
			m_DistributionValues->Build(procName, m_pCfg, exprCallEx, m_pStmt, exprName, itCurrGlob->second);
	}		
	//Фактические параметры
	ValuesPointerMap::const_iterator itCurrAct = actualParam.begin();
	for(; itCurrAct != actualParam.end(); ++itCurrAct)		
	{
		ExprName exprName = itCurrAct->first;
		if (m_ExprCallEx->GetProcBlock()->getChildCount()>0)
			m_DistributionValues->Build(procName, m_pCfg, exprCallEx, m_pStmt, exprName, itCurrAct->second);
	}
}

bool ExprAnalyzer::Parse(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg)
{
	SetVarParam(procName, exprCallEx, pStmt, pCfg);
	ParseAssign(*pStmt);
	if (m_isExprPointer)
		AddEntriesPointers(m_exprData, m_samc);			
	return m_isExprPointer;
}

void ExprAnalyzer::AddEntriesPointers(ReferenceExpression* exprData, SetAbstractMemoryCell& m_samc)
{
	if (exprData)
	{
		m_PointersTable->SetSAMC(exprData, m_samc);
		m_DistributionValues->Build(m_ProcName, m_pCfg, m_ExprCallEx, m_pStmt, exprData->getReference().getName(), m_samc);
	}
}

void ExprAnalyzer::SetVarParam(const ExprName& procName, ExprCallEx* exprCallEx, StatementBase* pStmt, ControlFlowGraph* pCfg)
{
	m_exprData = NULL;
	m_samc.clear();
	m_isExprPointer = false;
	m_pStmt = pStmt;
	m_pCfg = pCfg;
	m_ExprCallEx = exprCallEx;
	m_ProcName = procName;
	m_PointersTable = exprCallEx->GetPointersTable();
	m_DistributionValues->SetPointsTo(m_PointersTable);		
}

void ExprAnalyzer::ParseAssign(RepriseBase& node)
{
	ExprWalker exprWalker(m_MemManager);
	if (exprWalker.Run(node))
	{
		if (exprWalker.isExprPointer() && exprWalker.getExprLeft() && exprWalker.getExprRight())
		{
			m_isExprPointer = exprWalker.isExprPointer();
			m_exprData = exprWalker.getExprLeft();										//запись левой части
			RightExprWalker rightExprWalker(m_MemManager, m_PointersTable, &m_samc);    //разбор правой части
			rightExprWalker.Run(exprWalker.getExprRight());
		}
	}
}

//****************** ExprWalker ******************

ExprWalker::ExprWalker(MemoryManager* aMemManager)
	: m_MemManager(aMemManager)
{
	m_IsExprPointer = false;
	m_ExprLeft = 0;
	m_ExprRight = 0;
}

bool ExprWalker::Run(RepriseBase& node)
{
	if (node.is_a<ReferenceExpression>()) 
		DeepWalker::visit(node.cast_to<ReferenceExpression>()); 
	else if (node.is_a<BasicCallExpression>())
		visit(node.cast_to<BasicCallExpression>()); //
	else if (node.is_a<SubroutineCallExpression>())
		DeepWalker::visit(node.cast_to<SubroutineCallExpression>()); 
	else if (node.is_a<BasicLiteralExpression>())
		DeepWalker::visit(node.cast_to<BasicLiteralExpression>());
	else if (node.is_a<StrictLiteralExpression>())
		DeepWalker::visit(node.cast_to<StrictLiteralExpression>());
	else if (node.is_a<CompoundLiteralExpression>())
		DeepWalker::visit(node.cast_to<CompoundLiteralExpression>());
	else if (node.is_a<SubroutineReferenceExpression>())
		DeepWalker::visit(node.cast_to<SubroutineReferenceExpression>());
	else if (node.is_a<StructAccessExpression>())
		DeepWalker::visit(node.cast_to<StructAccessExpression>());
	else if (node.is_a<EnumAccessExpression>())
		DeepWalker::visit(node.cast_to<EnumAccessExpression>());
	else if (node.is_a<TypeCastExpression>())
		DeepWalker::visit(node.cast_to<TypeCastExpression>());
	else if (node.is_a<ExpressionStatement>()) 
		DeepWalker::visit(node.cast_to<ExpressionStatement>());
	else if (node.is_a<ForStatement>())
		visit(node.cast_to<ForStatement>());
	else if (node.is_a<WhileStatement>())
		visit(node.cast_to<WhileStatement>());
	else if (node.is_a<IfStatement>())
		visit(node.cast_to<IfStatement>());
	else if (node.is_a<SwitchStatement>())
		visit(node.cast_to<SwitchStatement>());
	else if (node.is_a<GotoStatement>())
		DeepWalker::visit(node.cast_to<GotoStatement>());
	else if (node.is_a<ReturnStatement>())
		DeepWalker::visit(node.cast_to<ReturnStatement>());
	else return false;
	return true;
}

void ExprWalker::visit(BasicCallExpression& basicCallExpression)
{ 
	if ((basicCallExpression.getKind() == BasicCallExpression::BCK_ASSIGN) &&
	    (basicCallExpression.getArgument(0).is_a<ReferenceExpression>()))
	{			
		m_IsExprPointer = m_MemManager->GetVariablesClassifier()->IsPointer(basicCallExpression.getArgument(0).cast_to<ReferenceExpression>());
		if (m_IsExprPointer) //анализ будет только если левая часть указатель
		{			
			m_ExprLeft=basicCallExpression.getArgument(0).cast_ptr<ReferenceExpression>();
			m_ExprRight=&(basicCallExpression.getArgument(1));
		}		
	}
}

void ExprWalker::visit(ForStatement& forStatement)
{
	Run(forStatement.getInitExpression());	
	Run(forStatement.getFinalExpression());	
	Run(forStatement.getStepExpression());
	//тело цикла не обрабатывается
}

void ExprWalker::visit(WhileStatement& whileStatement)
{
	Run(whileStatement.getCondition());
	//тело цикла не обрабатывается
}

void ExprWalker::visit(IfStatement& ifStatement)
{
	Run(ifStatement.getCondition());	
	//тело if не обрабатывается
}

void ExprWalker::visit(SwitchStatement& switchStatement)
{
	Run(switchStatement.getCondition());
	//тело не обрабатывается
}


//****************** RightExprWalker ******************

bool RightExprWalker::Run(ExpressionBase* expressionBase)
{
	if (expressionBase->is_a<ReferenceExpression>()) 
		visit(expressionBase->cast_to<ReferenceExpression>());
	else if (expressionBase->is_a<BasicCallExpression>())
		visit(expressionBase->cast_to<BasicCallExpression>());
	else if (expressionBase->is_a<BasicLiteralExpression>())
		DeepWalker::visit(expressionBase->cast_to<BasicLiteralExpression>());
	else if (expressionBase->is_a<StrictLiteralExpression>())
		DeepWalker::visit(expressionBase->cast_to<StrictLiteralExpression>());
	else if (expressionBase->is_a<CompoundLiteralExpression>())
		DeepWalker::visit(expressionBase->cast_to<CompoundLiteralExpression>());
	else if (expressionBase->is_a<SubroutineReferenceExpression>())
		DeepWalker::visit(expressionBase->cast_to<SubroutineReferenceExpression>());
	else if (expressionBase->is_a<StructAccessExpression>())
		DeepWalker::visit(expressionBase->cast_to<StructAccessExpression>());
	else if (expressionBase->is_a<EnumAccessExpression>())
		DeepWalker::visit(expressionBase->cast_to<EnumAccessExpression>());
	else if (expressionBase->is_a<TypeCastExpression>())
		DeepWalker::visit(expressionBase->cast_to<TypeCastExpression>());
	else if (expressionBase->is_a<SubroutineCallExpression>())
		DeepWalker::visit(expressionBase->cast_to<SubroutineCallExpression>());
	else return false;
	return true;
}

void RightExprWalker::visit(ReferenceExpression& referenceExpression)
{ 
	TypeBase &typeData = referenceExpression.getReference().getType();
	if (typeData.is_a<BasicType>() || typeData.is_a<ArrayType>())
		m_SAMC->push_back(AbstractMemoryCell(referenceExpression.getReference().getName(),0));
	else 
		if (typeData.is_a<PtrType>())
			*m_SAMC = m_PointersTable->GetSAMC(&referenceExpression);
}

void RightExprWalker::visit(BasicCallExpression& basicCallExpression)
{	
	if (!HasTemplateSimpleAssign(basicCallExpression))
		if (!HasTemplateArrayAssign(basicCallExpression))
			if (!HasTemplateAssignPtrOprConst(basicCallExpression))
				DeepWalker::visit(basicCallExpression);
}

bool RightExprWalker::HasTemplateSimpleAssign(BasicCallExpression& basicCallExpression)
{	
	if ((basicCallExpression.getArgumentCount() == 1) &&
	    (basicCallExpression.getArgument(0).is_a<ReferenceExpression>()))
	{
		ReferenceExpression& exprData = basicCallExpression.getArgument(0).cast_to<ReferenceExpression>();
		switch (basicCallExpression.getKind())
		{
			case BasicCallExpression::BCK_TAKE_ADDRESS:			// &
				{
					m_SAMC->push_back(AbstractMemoryCell(exprData.getReference().getName(),0));
				}
				break;

			case BasicCallExpression::BCK_DE_REFERENCE:			// *
				{
					// m_SAMC 
				}
				break;
			default: ;
		}
		return true;
	}
	return false;
}

bool RightExprWalker::HasTemplateArrayAssign(BasicCallExpression& basicCallExpression)
{	
	if ((basicCallExpression.getArgumentCount() == 1) &&
	    (basicCallExpression.getArgument(0).is_a<BasicCallExpression>()))
	{
		BasicCallExpression& basicCallExprArray = basicCallExpression.getArgument(0).cast_to<BasicCallExpression>();
		if ((basicCallExprArray.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS) &&
			(basicCallExprArray.getArgument(0).is_a<ReferenceExpression>()))
		{
			ReferenceExpression& arrayData = basicCallExprArray.getArgument(0).cast_to<ReferenceExpression>();
			if (basicCallExpression.getKind() == BasicCallExpression::BCK_TAKE_ADDRESS)							
			{	
				long_long_t offset_array = m_MemManager->GetArrayOffset(basicCallExprArray);
				m_SAMC->push_back(AbstractMemoryCell(arrayData.getReference().getName(),offset_array));
				return true;
			}
		}
	}
	return false;
}

bool RightExprWalker::HasTemplateAssignPtrOprConst(BasicCallExpression& basicCallExpression)
{	
	if (basicCallExpression.getArgumentCount() == 2)
	{
		BasicCallExpression::BuiltinCallKind callKind = basicCallExpression.getKind();
		if (callKind == BasicCallExpression::BCK_BINARY_PLUS ||	
			callKind == BasicCallExpression::BCK_BINARY_MINUS)
		{
			char iAction = (callKind == BasicCallExpression::BCK_BINARY_PLUS)? 1: -1;

			if ((((basicCallExpression.getArgument(0).is_a<ReferenceExpression>()) && (basicCallExpression.getArgument(1).is_a<LiteralExpression>())) ||
				 ((basicCallExpression.getArgument(1).is_a<ReferenceExpression>()) && (basicCallExpression.getArgument(0).is_a<LiteralExpression>())))
				)
			{
				ReferenceExpression* exprData;
				LiteralExpression* exprLit;
				if ((basicCallExpression.getArgument(0).is_a<ReferenceExpression>()) && (basicCallExpression.getArgument(1).is_a<LiteralExpression>()))
				{
					exprData = basicCallExpression.getArgument(0).cast_ptr<ReferenceExpression>();
					exprLit  = basicCallExpression.getArgument(1).cast_ptr<LiteralExpression>();
				}
				else 
				{
					exprData = basicCallExpression.getArgument(1).cast_ptr<ReferenceExpression>();
					exprLit  = basicCallExpression.getArgument(0).cast_ptr<LiteralExpression>();
				}
				SetAbstractMemoryCell samcTmp = m_PointersTable->GetSAMC(exprData);
				long_long_t offset = m_MemManager->GetIntLiteralValue(exprLit, iAction);				
				*m_SAMC = m_MemManager->GetSAMCExprValue(samcTmp, offset);								
				return true;
			}
		}
	}
	return false;
}
		
} // end namespace AliasAnalysis
} // end namespace OPS
