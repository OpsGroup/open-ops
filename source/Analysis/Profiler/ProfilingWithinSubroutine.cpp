#include "ProfilingWithinSubroutine.h"
#include "Analysis/DepGraph/DepGraphEx/LoopHelpers.h"
#include "Shared/PlatformSettings.h"

namespace OPS
{
namespace Profiling
{
using namespace OPS::Reprise;
using namespace OPS::Shared;
using namespace DepGraph;
using namespace DepGraph::Id;

// ProfilingResults class implementation

ProfilingResults::ProfilingResults()
{
	CObT.clear();
};

ProfilingResults::ProfilingResults(const CounterOperationsByType& aCObT)
{
	CObT = aCObT;
};

void ProfilingResults::Add(int CallKind, OPS::Reprise::BasicType::BasicTypes basicTypes)
{
  MapTypeCounter &MTC = CObT[CallKind];
	if (MTC.find(basicTypes)!=MTC.end())
		MTC[basicTypes]+=1;
	else
		MTC[basicTypes]=1;
};

void ProfilingResults::Sum(ProfilingResults &psOWN) 
{
  CounterOperationsByType::iterator itCObT;
  MapTypeCounter::iterator itMTC;
	for (itCObT=psOWN.CObT.begin(); itCObT!=psOWN.CObT.end(); itCObT++)
	{
    MapTypeCounter &thisMTC = this->CObT[itCObT->first];
    MapTypeCounter &ownMTC = itCObT->second;
		for (itMTC=ownMTC.begin(); itMTC!=ownMTC.end(); itMTC++)
		{				
			if(thisMTC.find(itMTC->first)!=thisMTC.end())
				thisMTC[itMTC->first]+=itMTC->second;
			else
				thisMTC[itMTC->first]=itMTC->second;
		}
	}
};

void ProfilingResults::Mult(long_long_t m) 
{		
  CounterOperationsByType::iterator itCObT;
  MapTypeCounter::iterator itMTC;
	for (itCObT=CObT.begin(); itCObT!=CObT.end(); itCObT++)
	{
    MapTypeCounter &MTC = itCObT->second;
		for (itMTC=MTC.begin(); itMTC!=MTC.end(); itMTC++)
			itMTC->second *= m;
	}
}; 

double ProfilingResults::Count() 
{
	double Counter = 0;
  CounterOperationsByType::iterator itCObT;
  MapTypeCounter::iterator itMTC;
	for (itCObT=CObT.begin(); itCObT!=CObT.end(); itCObT++)
	{
    MapTypeCounter &MTC = itCObT->second;
		for (itMTC=MTC.begin(); itMTC!=MTC.end(); itMTC++)
			Counter += itMTC->second;
	}
	return Counter;
};	


// StatementCounter class implementation

StatementCounter::StatementCounter()
{ 
	m_mapStmtCounter.clear(); 
}

void StatementCounter::SetProfilingResults(OPS::Reprise::StatementBase *sb, CounterOperationsByType &cobt)
{ 
	m_mapStmtCounter[sb]=cobt; 
};	

CounterOperationsByType StatementCounter::GetProfilingResults(OPS::Reprise::StatementBase *sb)
{ 
  CounterOperationsByType tmp;
	return ((m_mapStmtCounter.find(sb)!=m_mapStmtCounter.end())? m_mapStmtCounter[sb] : tmp); 
};

StatementCounter::MapStatementCounter& StatementCounter::GetMapStatementCounter() 
{	
	return m_mapStmtCounter; 
}


// ProcedureCounter class implementation

ProcedureCounter::ProcedureCounter() 
{ 
	m_mapProcCounter.clear(); 
}	

void ProcedureCounter::SetProfilingResults(OPS::Reprise::SubroutineDeclaration *sd, CounterOperationsByType& cobt)
{ 
	m_mapProcCounter[sd]=cobt; 
};

CounterOperationsByType ProcedureCounter::GetProfilingResults(OPS::Reprise::SubroutineDeclaration *sd)
{ 
  CounterOperationsByType tmp;
	return ((m_mapProcCounter.find(sd)!=m_mapProcCounter.end())? m_mapProcCounter[sd] : tmp); 
};

ProcedureCounter::MapProcedureCounter& ProcedureCounter::GetMapProcedureCounter() 
{
	return m_mapProcCounter; 
}


// WithinSubroutineAnalysis class implementation

WithinSubroutineAnalysis::WithinSubroutineAnalysis(RepriseBase* node, ProfilingResults* profRes, ProfilingMode pms
																									,StatementCounter* scFull, StatementCounter* scOH, ProcedureCounter* pc
																									,StatementLimitsIteration* sliIn, StatementLimitsIteration* sliOut)
	: m_ProfilingMode(pms), p_StmtCounterFull(scFull), p_StmtCounterOnlyHeaders(scOH)
	 ,p_ProcCounter(pc), p_InStmtLimIter(sliIn), p_OutStmtLimIter(sliOut), p_ProfilingResults(profRes)
{
	Run(node);
};	

WithinSubroutineAnalysis::WithinSubroutineAnalysis(RepriseBase* node, ProfilingResults* profRes, const WithinSubroutineAnalysis* wsa)
  : m_ProfilingMode(wsa->m_ProfilingMode) ,p_StmtCounterFull(wsa->p_StmtCounterFull), p_StmtCounterOnlyHeaders(wsa->p_StmtCounterOnlyHeaders)
	 ,p_ProcCounter(wsa->p_ProcCounter), p_InStmtLimIter(wsa->p_InStmtLimIter), p_OutStmtLimIter(wsa->p_OutStmtLimIter)
  ,p_ProfilingResults(profRes)
{
	Run(node);
};	

void WithinSubroutineAnalysis::Run(OPS::Reprise::RepriseBase* repriseBase)
{
	repriseBase->accept(*this);
}

//--- Expressions ---

void WithinSubroutineAnalysis::visit(OPS::Reprise::BasicLiteralExpression& basicLiteralExpression)
{
    p_ProfilingResults->Add(PlatformSettings::OCK_CONST, GetBasicTypes(basicLiteralExpression.getResultType().get()));
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::StrictLiteralExpression& strictLiteralExpression)
{
	p_ProfilingResults->Add(PlatformSettings::OCK_CONST, strictLiteralExpression.getLiteralType());
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::ReferenceExpression& referenceExpression)
{
	p_ProfilingResults->Add(PlatformSettings::OCK_READ, GetBasicTypes(&referenceExpression.getReference().getType()));
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::StructAccessExpression& structAccessExpression)
{
	p_ProfilingResults->Add(PlatformSettings::OCK_READ, GetBasicTypes(&structAccessExpression.getMember().getType()));
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::EnumAccessExpression& enumAccessExpression)
{	
    p_ProfilingResults->Add(PlatformSettings::OCK_CONST, GetBasicTypes(enumAccessExpression.getResultType().get()));
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::BasicCallExpression& basicCallExpression)
{	
	if (basicCallExpression.getKind() == BasicCallExpression::BCK_ASSIGN)
	{
		p_ProfilingResults->Add(basicCallExpression.getKind(), GetTypes(&basicCallExpression.getArgument(0)));
		ParseLeftExpression(basicCallExpression.getArgument(0));
	}
	else
	{
		if (basicCallExpression.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			AddReadForDynamicArray(basicCallExpression.getArgument(0));			
		p_ProfilingResults->Add(basicCallExpression.getKind(), GetTypes(&basicCallExpression));
		Run(&basicCallExpression.getArgument(0));
	}
	for (int i=1; i<basicCallExpression.getArgumentCount(); i++)
		Run(&basicCallExpression.getArgument(i));
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::SubroutineCallExpression& callExpr)
{
	// Сложность вызова ()
	p_ProfilingResults->Add(PlatformSettings::OCK_CALL, BasicType::BT_UNDEFINED);
	
	// Сложность передачи аргументов
	for (int i=0; i<callExpr.getArgumentCount(); i++)
	{
		p_ProfilingResults->Add(PlatformSettings::OCK_ARGUMENT, GetTypes(&callExpr.getArgument(i)));
	}

	// Учет сложности вызываемой функции
	if (m_ProfilingMode.prfWithSubroutineCalls)
	{
		ProfilingResults pr(p_ProcCounter->GetProfilingResults(&callExpr.getExplicitSubroutineDeclaration()));
		p_ProfilingResults->Sum(pr);
	}
}

//--- Statements ---

void WithinSubroutineAnalysis::visit(OPS::Reprise::ExpressionStatement& exprStmt)
{
	WithinSubroutineAnalysis wsa(&exprStmt.get(), p_ProfilingResults, this);
	p_StmtCounterFull->SetProfilingResults(&exprStmt, p_ProfilingResults->CObT);
	p_StmtCounterOnlyHeaders->SetProfilingResults(&exprStmt, p_ProfilingResults->CObT);
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::BlockStatement& blockStmt)
{
	BlockStatement::Iterator it = blockStmt.getFirst();
	for(; it.isValid(); ++it)
	{
		StatementBase& stmtCurrent = *it;
		ProfilingResults profResStmt;
		WithinSubroutineAnalysis wsa(&stmtCurrent, &profResStmt, this);
	}
	it = blockStmt.getFirst();
	for(; it.isValid(); ++it)
	{
		StatementBase& stmtCurrent = *it;
		ProfilingResults profResStmt;
		profResStmt = p_StmtCounterFull->GetProfilingResults(&stmtCurrent);		
		p_ProfilingResults->Sum(profResStmt);
	}
	p_StmtCounterFull->SetProfilingResults(&blockStmt, p_ProfilingResults->CObT);
	p_StmtCounterOnlyHeaders->SetProfilingResults(&blockStmt, p_ProfilingResults->CObT);
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::ForStatement& forStmt)
{
	long_long_t iterCount = GetCountIterations(forStmt);

	//сложность заголовка цикла	
	WithinSubroutineAnalysis wsaInit(&forStmt.getInitExpression(), p_ProfilingResults, this);	

	ProfilingResults profResFinal;
	WithinSubroutineAnalysis wsaFinal(&forStmt.getFinalExpression(), &profResFinal, this);
	profResFinal.Mult(iterCount);

	ProfilingResults profResStep;
	WithinSubroutineAnalysis wsaStep(&forStmt.getStepExpression(), &profResStep, this);
	profResStep.Mult(iterCount);

	//сложность тела цикла
	ProfilingResults profResBody;
	WithinSubroutineAnalysis wsaBody(&forStmt.getBody(), &profResBody, this);
	profResBody.Mult(iterCount);

	p_ProfilingResults->Sum(profResFinal);
	p_ProfilingResults->Sum(profResStep);
	
	//сложность только заголовка цикла
	p_StmtCounterOnlyHeaders->SetProfilingResults(&forStmt, p_ProfilingResults->CObT);

	//сложность цикла = сложность заголовка + сложность тела
	p_ProfilingResults->Sum(profResBody);
	p_StmtCounterFull->SetProfilingResults(&forStmt, p_ProfilingResults->CObT);	
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::WhileStatement& whileStmt)
{
	long_long_t iterCount = UserDialog(&whileStmt);

	//сложность заголовка цикла
	WithinSubroutineAnalysis wsaCondition(&whileStmt.getCondition(), p_ProfilingResults, this);
	p_ProfilingResults->Mult(iterCount);
	p_StmtCounterOnlyHeaders->SetProfilingResults(&whileStmt, p_ProfilingResults->CObT);

	//сложность тела цикла	
	ProfilingResults profResBody;	
	WithinSubroutineAnalysis wsaBody(&whileStmt.getBody(), &profResBody, this);
	profResBody.Mult(iterCount);
	
	//сложность цикла = сложность заголовка + сложность тела
	p_ProfilingResults->Sum(profResBody);
	p_StmtCounterFull->SetProfilingResults(&whileStmt, p_ProfilingResults->CObT);
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::IfStatement& ifStmt)
{
	//сложность условия
	WithinSubroutineAnalysis wsaCondition(&ifStmt.getCondition(), p_ProfilingResults, this);
	p_StmtCounterOnlyHeaders->SetProfilingResults(&ifStmt, p_ProfilingResults->CObT);

	//сложность ветки then
	ProfilingResults profResThen;
	WithinSubroutineAnalysis wsaThen(&ifStmt.getThenBody(), &profResThen, this);

	//сложность ветки else
	ProfilingResults profResElse;
	WithinSubroutineAnalysis wsaElse(&ifStmt.getElseBody(), &profResElse, this);

	//в худшем случае
	if (profResThen.Count() > profResElse.Count())
		p_ProfilingResults->Sum(profResThen);
	else
		p_ProfilingResults->Sum(profResElse);
	p_StmtCounterFull->SetProfilingResults(&ifStmt, p_ProfilingResults->CObT);
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::GotoStatement& gotoStatement)
{
	long_long_t iterCount = UserDialog(&gotoStatement);
	if (iterCount > 1)
	{
		StatementList SL = GetStatementListForGoto(gotoStatement);
		StatementList::iterator itSL;
		for (itSL=SL.begin(); itSL!=SL.end(); itSL++)
		{
			StatementBase *sb = *itSL;
			
			ProfilingResults prFull;
			prFull = p_StmtCounterFull->GetProfilingResults(sb);
			prFull.Mult(iterCount);
			p_StmtCounterFull->SetProfilingResults(sb, prFull.CObT);

			ProfilingResults prOH;
			prOH = p_StmtCounterOnlyHeaders->GetProfilingResults(sb);
			prOH.Mult(iterCount);
			p_StmtCounterOnlyHeaders->SetProfilingResults(sb, prOH.CObT);
		}		
	}	
}

void WithinSubroutineAnalysis::visit(OPS::Reprise::ReturnStatement& returnStmt)
{
	WithinSubroutineAnalysis wsaReturn(&returnStmt.getReturnExpression(), p_ProfilingResults, this);
	p_ProfilingResults->Add(BasicCallExpression::BCK_ASSIGN, GetTypes(&returnStmt.getReturnExpression()));
	p_StmtCounterOnlyHeaders->SetProfilingResults(&returnStmt, p_ProfilingResults->CObT);
	p_StmtCounterFull->SetProfilingResults(&returnStmt, p_ProfilingResults->CObT);
}


long_long_t WithinSubroutineAnalysis::GetCountIterations(OPS::Reprise::ForStatement& forStmt)
{
	long_long_t iterCount = 1;
	bool isCalc = 0;
	try
	{
		id  index(&forStmt, &forStmt);
		std::vector<LoopInfo> loops;
		index.getOuterLoopsList(loops, false);

		int num = -1;
		for (int k=0; k<(int)loops.size(); k++)
			if (loops[k].pStmtFor==&forStmt)
			{
				num=k;
				break;
			}

		if (num>-1)
		{			
			LoopDescr indLoops(loops[num]);		
			if (indLoops.getDwBound().isConstant() &&
				indLoops.getUpBound().isConstant() &&
				indLoops.getStep().isConstant())
			{
				long_long_t l = (long_long_t)indLoops.getDwBound().getFreeSummand();
				long_long_t u = (long_long_t)indLoops.getUpBound().getFreeSummand();
				long_long_t s = (long_long_t)indLoops.getStep().getFreeSummand();
				iterCount = (u - l)/s;
				if (iterCount < 0)
					iterCount = 1;
				else
					isCalc = 1;
			}
		}
	}
	catch(const OPS::Exception)
	{
		isCalc = 0;		
	}
	if (!isCalc)
		iterCount = UserDialog(&forStmt);
	return iterCount;
}

long_long_t WithinSubroutineAnalysis::UserDialog(OPS::Reprise::StatementBase* stmt)
{
	long_long_t iterCount = 1;
	if ((p_InStmtLimIter!=0) && ((*p_InStmtLimIter).find(stmt)!=(*p_InStmtLimIter).end()))
	{
		//если он задан пользователем
		StatementLimitsIteration::iterator it=(*p_InStmtLimIter).find(stmt);	
		iterCount = it->second.min + (it->second.max - it->second.min)/2;
	}
	else
		if (p_OutStmtLimIter!=0)
		{	//добавить в невычисляемые
			LimitsIteration limIter;
			(*p_OutStmtLimIter)[stmt]=limIter;			
		}
	return iterCount;
}

BasicType::BasicTypes WithinSubroutineAnalysis::GetBasicTypes(TypeBase* typeBase)
{
	if (typeBase->is_a<BasicType>())
	{
		BasicType& basicType = typeBase->cast_to<BasicType>();
		return basicType.getKind();
	}
	else if (typeBase->is_a<PtrType>())
	{
		return BasicType::BT_VOID;
	}
	else if (typeBase->is_a<ArrayType>())
	{
		ArrayType& arrayType = typeBase->cast_to<ArrayType>();
		return GetBasicTypes(&arrayType.getBaseType());
	}
	else if (typeBase->is_a<VectorType>())
	{
		VectorType& vectorType = typeBase->cast_to<VectorType>();
		return GetBasicTypes(&vectorType.getBaseType());
	}
	return BasicType::BT_INT32;
}

BasicType::BasicTypes WithinSubroutineAnalysis::GetTypes(ExpressionBase* e)
{
	try
	{
		if (e->is_a<BasicCallExpression>())
		{
			int curType, maxType;
			maxType = 0;
			BasicCallExpression *bce = e->cast_ptr<BasicCallExpression>();
			for (int i=0; i<bce->getArgumentCount(); i++)
			{
                curType=GetBasicTypes(bce->getArgument(i).getResultType().get());
				if (maxType<curType)
					maxType=curType;
			}
			return (BasicType::BasicTypes)maxType;
		}
		else
		{
            return GetBasicTypes(e->getResultType().get());
		}
	}
	catch(const OPS::Exception)
	{
		return BasicType::BT_INT32;
	}
}

void WithinSubroutineAnalysis::ParseLeftExpression(OPS::Reprise::ExpressionBase& expr)
{	
	if (!expr.is_a<ReferenceExpression>())
	{
		if (expr.is_a<BasicCallExpression>())
		{
			BasicCallExpression& bce = expr.cast_to<BasicCallExpression>();
			if (bce.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			{
				AddReadForDynamicArray(bce.getArgument(0));
				for (int i=1; i<bce.getArgumentCount(); i++)
					Run(&bce.getArgument(i));
			}
			else Run(&expr);
		}
		else Run(&expr);
	}
}

void WithinSubroutineAnalysis::AddReadForDynamicArray(OPS::Reprise::ExpressionBase& arrayExprBase)
{
	if (arrayExprBase.is_a<ReferenceExpression>())
	{
		ReferenceExpression& arrayRefExpr = arrayExprBase.cast_to<ReferenceExpression>();
		TypeBase& arrayTypeBase = arrayRefExpr.getReference().getType();
		if (arrayTypeBase.is_a<PtrType>())
		{
			PtrType& arrayPtrType = arrayTypeBase.cast_to<PtrType>();
			p_ProfilingResults->Add(PlatformSettings::OCK_READ, GetBasicTypes(&arrayPtrType.getPointedType()));
		}
	}
}

WithinSubroutineAnalysis::StatementList WithinSubroutineAnalysis::GetStatementListForGoto(OPS::Reprise::GotoStatement& gotoStatement)
{
	StatementList stmtList;
	StatementBase* pointedStmt = gotoStatement.getPointedStatement();
	BlockStatement* blockStmtGoto = &gotoStatement.getRootBlock();
	BlockStatement* blockStmtPointed = &pointedStmt->getRootBlock();
	StatementList slBlock;
	slBlock.push_back(blockStmtGoto);
	while (blockStmtGoto!=blockStmtPointed)
	{
		if (blockStmtGoto->hasParentBlock())
		{
			blockStmtGoto = &blockStmtGoto->getParentBlock();
			slBlock.push_back(blockStmtGoto);
		}
		else
			break;		
	}

	StatementList::iterator itSLBlock;
	for (itSLBlock=slBlock.begin(); itSLBlock!=slBlock.end(); itSLBlock++)
	{
		BlockStatement* block = dynamic_cast<BlockStatement*>(*itSLBlock);
		BlockStatement::Iterator FirstStmtIt=block->getFirst();
		BlockStatement::Iterator LastStmtIt=block->getLast();
		if (block==&pointedStmt->getRootBlock())
		{
			BlockStatement::Iterator it = block->getFirst();
			for(; it.isValid(); ++it)
			{
				StatementBase& stmtCurrent = *it;
				if (pointedStmt == &stmtCurrent)
				{
					FirstStmtIt = it;
					break;
				}
			}
		}
		if (block==&gotoStatement.getRootBlock())
		{
			BlockStatement::Iterator it = block->getFirst();
			for(; it.isValid(); ++it)
			{
				StatementBase& stmtCurrent = *it;
				if (&gotoStatement == &stmtCurrent)
				{
					LastStmtIt = it;
					break;
				}
			}
		}
		BlockStatement::Iterator itStmt=FirstStmtIt;
		for (; itStmt!=LastStmtIt; itStmt++)
		{
			StatementBase& stmtCurrent = *itStmt;
			stmtList.push_back(&stmtCurrent);
		}
	}
	return stmtList;
}

}
}
