#include "Analysis/DepGraph/id.h"
#include "Analysis/DepGraph/DepGraph.h"
#include <iostream>
#include "Shared/LinearExpressions.h"

#include "OPS_Core/msc_leakcheck.h"
#include "OPS_Core/Localization.h"

namespace DepGraph
{
	namespace Id
	{
//using namespace DepGraph;
using namespace std;
/////////////// my_str /////////////////////////
void myStmtStr::print()
{
	cout <<_TL("Position ","")<< m_nCurPos <<_TL(" in statement","")<<endl;
	cout << m_pStmt->dumpState() <<endl;
}

/////////////// OccurrenceInfo /////////////////////////
OccurrenceInfo::OccurrenceInfo() 
:m_numcycles(0)
,m_dim(0)
,m_bIsLoopIndex(false)
,m_isIndexExprLinear(false)
{}

OccurrenceInfo::OccurrenceInfo(const VariableDeclaration* n1, bool gen, int LoopNum, int dim, bool bLin, ReferenceExpression * pED, bool bIsLoopIndex)
:m_oDataObject(n1)
,m_numcycles(LoopNum)
,m_dim(dim)
,m_bIsLoopIndex(bIsLoopIndex)
,m_pED(pED)
,m_generator(gen)
,m_isIndexExprLinear(bLin)
{
}


LoopInfo::LoopInfo()
:pthis(0)
,m_left(0)
,m_right(0)
,m_step(0)
,pStmtFor(0)
{
}

LoopInfo::LoopInfo(OPS::Reprise::ForStatement &forStmt)
	:m_oDataObject(0)
	,pthis(0)
	,m_left(0)
	,m_right(0)
	,m_step(0)
	,pStmtFor(0)
{
	reset(forStmt);
}

void LoopInfo::reset(OPS::Reprise::ForStatement &forStmt)
{
	pStmtFor = &forStmt;

	CallExpressionBase* pExprOper = dynamic_cast<CallExpressionBase*>(&forStmt.getInitExpression()); 

	if (pExprOper)
	{
		ReferenceExpression* pED = dynamic_cast<ReferenceExpression*>(&pExprOper->getArgument(0)); 
		if ( pED )
			m_oDataObject = &pED->getReference();
		pthis = &pExprOper->getArgument(0);
		m_left = pExprOper;
	};

	pExprOper = dynamic_cast<CallExpressionBase *>(&forStmt.getStepExpression() );
	if (pExprOper && (pExprOper->getArgumentCount() > 1) ) 
	{
		if (pExprOper->getArgument(1).is_a<CallExpressionBase>())
			m_step = pExprOper;
	}; 

	if (forStmt.getFinalExpression().is_a<CallExpressionBase>()) 
		m_right = &forStmt.getFinalExpression();
}

///////////////    id  ///////////////////////////
//id::id(StatementBase *pStmt)
//{
//	reset(pStmt);
//}


id::id(BlockStatement &rBlock)
{   
	reset(rBlock);
}

id::id(BlockStatement *pBlock)
{   
	reset(*pBlock);
}

id::id(StatementBase *pStmt1, StatementBase *pStmt2)
{
	reset(pStmt1,pStmt2);
}
/*
id::id(Statement * pFirstStmt, Statement *pSecondStmt)
// вводит интервал
// next проходит этот интервал, потом 0
{
	Statement *ptemp = pFirstStmt;
	while (ptemp->getParentStmt()) ptemp = ptemp->getParentStmt();
	Statement *ptemp1 = pFirstStmt;
	while (ptemp1->getParentStmt()) ptemp1 = ptemp1->getParentStmt();
	if ( (ptemp1 != ptemp) )
	{
		throw 1;
		return;
	}
	reset(ptemp);
	findfirst(pFirstStmt);
	//  this->pCur = psecond;
}*/

////////////////////////////////////////////////////////
id::id()
{
	clear();
}

void id::clear()
{
	m_StmtStack.clear();
	m_bFindNext = true;
	m_pFirst = 0;
	m_pEnd  = 0;
}

void id::reset()
{
	StatementBase * pStmt1= m_pFirst;
	StatementBase * pStmt2 = m_pEnd;
	if ( pStmt1->is_a<BlockStatement>() )
		reset(pStmt1->cast_to<BlockStatement>());
	else reset(pStmt1,pStmt2);
}

//void id::reset(StatementBase * pStmt) 
//{ 
//	clear(); 
//	if (!pStmt) return;
//
//	StatementBase *ptemp = pStmt;
//
//	//while ( ptemp->getParentStmt() ) 
//	//	ptemp = ptemp->getParentStmt(); 
//
//
//	if ( (!pStmt->hasParentBlock()) && ( pStmt->is_a<BlockStatement>() )) 
//	{ 
//		BlockStatement* pBlock = dynamic_cast<BlockStatement*>(pStmt); 
//		m_pFirst = pStmt; 
//		if ( pBlock != 0 ) 
//		{ 
//			m_StmtStack.push_back(myStmtStr(pBlock,1)) ; 
//			if (pBlock->isEmpty())
//				return;
//
//			StatementBase *ptemp1 = pBlock->getIntBlock(1); 
//			while ( ptemp1->isDeepViewNeed() ) 
//			{ 
//				m_StmtStack.push_back(myStmtStr(ptemp1,1)) ; 
//
//				if (!ptemp->isIntBlockValid(1))
//					break;
//
//				ptemp1 = ptemp1->getIntBlock(1); 
//				if (!( pBlock = dynamic_cast<Block*>(ptemp1) )) 
//					break;
//			} 
//		} 
//	} 
//	else 
//	{
//		m_StmtStack.push_back(myStmtStr(pStmt,1)) ; 
//		m_pFirst = pStmt; 
//	}; 
//}; 

			//=============== переделать!=============
void id::reset(BlockStatement &rBlock)
{
	clear();
	if (!(&rBlock)) return; // эммм???

	// запоминаем начальный оператор
	m_pFirst = &rBlock;
	m_StmtStack.push_back(myStmtStr(m_pFirst,1)) ;

	// если блок пустой - выходим
	if (rBlock.isEmpty())
		return;

	// Берем первый оператор блока
	StatementBase *ptemp1 = &(*rBlock.getFirst());

	// Пока есть что обрабатывать
	while (  ptemp1 )
	{
		// Не заходим внутрь операторов break, continue, return, goto, expr, empty
        if (ptemp1->is_a<ReturnStatement>() || ptemp1->is_a<GotoStatement>()
			|| ptemp1->is_a<ExpressionStatement>() || ptemp1->is_a<EmptyStatement>())
			break;
	
		// не заходим внутрь пустых блоков
		if (ptemp1->is_a<BlockStatement>() && ptemp1->cast_to<BlockStatement>().isEmpty())
			break;

		// Запоминаем текущий оператор
		m_StmtStack.push_back(myStmtStr(ptemp1,1)) ;

		if (ptemp1->is_a<BlockStatement>())
		{
			// Берем первый оператор внутри блока
			ptemp1 = &(*ptemp1->cast_to<BlockStatement>().getFirst());
		}
		else if (ptemp1->is_a<ForStatement>() ||
				 ptemp1->is_a<WhileStatement>() ||
				 ptemp1->is_a<IfStatement>())
		{
			// оставляем все как есть
		}
		else if (ptemp1->is_a<PlainSwitchStatement>())
		{
			// берем блок под первым case
			ptemp1 = &ptemp1->cast_to<PlainSwitchStatement>().getBody();
		}
		else
			ptemp1 = 0;


		// если текущий оператор не блок, то продолжаем цикл
		if ( !(ptemp1->is_a<BlockStatement>()) )  break;    
	};

}

void id::reset(StatementBase *pStmt1, StatementBase *pStmt2)
{
	clear();
	if (!pStmt1) return;

	// берем начальный оператор
	StatementBase *ptemp = pStmt1;

	//while (ptemp->getParentStmt() ) ptemp = ptemp->getParentStmt();

	// берем блок, которому принадлежит начальный оператор
	ptemp = &ptemp->getRootBlock();

	if ( ptemp )
	{
		// ресетим ид этим блоком
		reset(ptemp->cast_to<BlockStatement>());

		// запоминаем первый оператор как начальный
		m_pFirst = pStmt1;
		// а второй как конечный
		m_pEnd = pStmt2;

		// если первый оператор блока и есть начальный оператор,
		// то выходим
		if (getThisOper() == pStmt1) 
			return; 

		// иначе итерируемся пока не дойдем до начального оператора
		while ( nextL() != pStmt1 )  
		{
			// cout << getThisOper()->dumpState() << endl;
		};
	}
	else throw 100; 
}

void id::resetForSSA(StatementBase *pStmt1, StatementBase *pStmt2)
{
	// мне кажется, что эта функция по смыслу не отличается от предыдущей
	clear();
	if (!pStmt1) return;

	StatementBase *ptemp = pStmt1;

	ptemp = &ptemp->getParentBlock();

	//cout << ptemp->dumpState() << endl;

	if ( ptemp )
	{
		reset(ptemp->cast_to<BlockStatement>());
		m_pFirst = pStmt1;

		if (getThisOper() == pStmt1) 
		{ m_pEnd = pStmt2; return; };


		while ( nextL() != pStmt1 )  
		{
			//			cout << getThisOper()->dumpState() << endl;
		};
		m_pEnd = pStmt2;
	}
	else throw 100; 
}

///////////////    next  //////////////////////////////
StatementBase * id::next()
{
	// если конечный оператор не задан, то вызываем функцию nextL
	if (!m_pEnd)
	{
		return nextL();
		//		if ( m_StmtStack.size() )
		//		{
		//			myStmtStr strTemp ( m_StmtStack.back() );
		//			m_StmtStack.pop_back();
		//			Statement *pStmt = strTemp.m_pStmt;
		//			int nCurPos = strTemp.m_nCurPos + 1;
		//			
		//			if ( pStmt->isIntBlockValid(nCurPos) )
		//			{
		//				m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
		//				pStmt = pStmt->getIntBlock(nCurPos);
		//				while (pStmt->isDeepViewNeed())
		//				{
		//					m_StmtStack.push_back(myStmtStr(pStmt,1));
		//					Block *pBlock;
		//					if (!( pBlock = dynamic_cast<Block*>(pStmt) )) break;
		//					pStmt = pStmt->getIntBlock(1);
		//				};
		//				return pStmt;
		//			}
		//			else return next();
		//		}
		//		else 	return 0;
	}
	else
	{
		// если искать дальше (что это значит???)
		if (m_bFindNext)
		{
			// искатьдальше если текущий оператор не последний
			m_bFindNext = getThisOper() != m_pEnd;
			nextL();
			return getThisOper();
		}
		else  return 0;
	};
}
									// исправить!
StatementBase * id::nextL()
{
	if (!m_StmtStack.empty())
	{
		myStmtStr strTemp ( m_StmtStack.back() );
		m_StmtStack.pop_back();
		StatementBase *pStmt = strTemp.m_pStmt;

		int nCurPos = strTemp.m_nCurPos + 1;
		GetTypeVisitor visitor;
		pStmt->accept(visitor);
		switch(visitor.m_typeOfNode)
		{
		case GetTypeVisitor::NK_BlockStatement:
			{

			BlockStatement* temp = &pStmt->cast_to<BlockStatement>();
			if (nCurPos>0 && nCurPos<=temp->getChildCount())
			{
				m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
				int count = 1;
				for(BlockStatement::Iterator it = temp->getFirst(); it.isValid(); it.goNext())
				{
					if (count == nCurPos)
					{
						pStmt = &(*it);
						break;
					}
					else count++;
				}
				while (pStmt)
				{
                    if (pStmt->is_a<ReturnStatement>() || pStmt->is_a<GotoStatement>()
						|| pStmt->is_a<ExpressionStatement>() || pStmt->is_a<EmptyStatement>())
						break;

					if (pStmt->is_a<BlockStatement>() && pStmt->cast_to<BlockStatement>().isEmpty())
						break;
					m_StmtStack.push_back(myStmtStr(pStmt,1));
					BlockStatement *pBlock = dynamic_cast<BlockStatement*>(pStmt) ;
					if (!pBlock) break;
					pStmt = &(*pBlock->getFirst());
				};
				return pStmt;
			}
			else return nextL();
			}
		case GetTypeVisitor::NK_ForStatement:
			{

			ForStatement* pFor = &pStmt->cast_to<ForStatement>();
			if (nCurPos==1 || nCurPos==2)
			{
				m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
				if (nCurPos == 2)
					pStmt = &pFor->getBody();

				while (pStmt)
				{
                    if (pStmt->is_a<ReturnStatement>() || pStmt->is_a<GotoStatement>()
						|| pStmt->is_a<ExpressionStatement>() || pStmt->is_a<EmptyStatement>())
						break;

					if (pStmt->is_a<BlockStatement>() && pStmt->cast_to<BlockStatement>().isEmpty())
						break;
					m_StmtStack.push_back(myStmtStr(pStmt,1));
					BlockStatement *pBlock = dynamic_cast<BlockStatement*>(pStmt);
					if (!pBlock  ) break;
					pStmt = &(*pBlock->getFirst());
				};
				return pStmt;
			}
			else return nextL();
			}
		case GetTypeVisitor::NK_IfStatement:
			{

			IfStatement* pIf = &pStmt->cast_to<IfStatement>();
			if (nCurPos==1 || nCurPos==2 || nCurPos == 3)
			{
				m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
				if (nCurPos == 2)
					pStmt = &pIf->getThenBody();
				if (nCurPos == 3)
					pStmt = &pIf->getElseBody();

				while (pStmt)
				{
                    if (pStmt->is_a<ReturnStatement>() || pStmt->is_a<GotoStatement>()
						|| pStmt->is_a<ExpressionStatement>() || pStmt->is_a<EmptyStatement>())
						break;

					if (pStmt->is_a<BlockStatement>() && pStmt->cast_to<BlockStatement>().isEmpty())
						break;
					m_StmtStack.push_back(myStmtStr(pStmt,1));
					BlockStatement *pBlock = dynamic_cast<BlockStatement*>(pStmt);
					if (!pBlock ) break;
					pStmt = &(*pBlock->getFirst());
				};
				return pStmt;
			}
			else return nextL();
			}
		case GetTypeVisitor::NK_WhileStatement:
			{

			WhileStatement* pWhile = &pStmt->cast_to<WhileStatement>();
			if (nCurPos==1 || nCurPos==2)
			{
				m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
				if (nCurPos == 2)
					pStmt = &pWhile->getBody();

				while (pStmt)
				{
                    if (pStmt->is_a<ReturnStatement>() || pStmt->is_a<GotoStatement>()
						|| pStmt->is_a<ExpressionStatement>() || pStmt->is_a<EmptyStatement>())
						break;

					if (pStmt->is_a<BlockStatement>() && pStmt->cast_to<BlockStatement>().isEmpty())
						break;
					m_StmtStack.push_back(myStmtStr(pStmt,1));
					BlockStatement *pBlock = dynamic_cast<BlockStatement*>(pStmt);
					if (!pBlock ) break;
					pStmt = &(*pBlock->getFirst());
				};
				return pStmt;
			}
			else return nextL();
			}
		default:
			return nextL();
		}
		
	}
	else 	return 0;
}
// (NB! исправить: default body может и не быть! )
bool isIntBlockValid(StatementBase *pStmt,int nPos)
{
	GetTypeVisitor visitor;
	pStmt->accept(visitor);
	switch(visitor.m_typeOfNode)
	{
	case GetTypeVisitor::NK_BlockStatement:
		if (nPos >= 0 && nPos < pStmt->cast_to<BlockStatement>().getChildCount())  return true;
		else return false;
	case GetTypeVisitor::NK_ForStatement:
		if ( (nPos>=1)&&(nPos<=2) ) return true;
		else return false;
	case GetTypeVisitor::NK_IfStatement:
		if ( (nPos>=1)&&(nPos<=3) ) return true;
		else return false;
	case GetTypeVisitor::NK_WhileStatement:
		if ( (nPos>=1)&&(nPos<=2) ) return true;
		else return false;
	
	default:
		return false;
	}
}
StatementBase * getIntBlock(StatementBase* pStmt,int nPos)
{
	GetTypeVisitor visitor;
	pStmt->accept(visitor);
	switch(visitor.m_typeOfNode)
	{
		case GetTypeVisitor::NK_BlockStatement:
		{
			int count = 0;
			BlockStatement* temp = &pStmt->cast_to<BlockStatement>();
			for(BlockStatement::Iterator it = temp->getFirst(); it.isValid(); it.goNext())
			{
				if (count == nPos)
				{
					return &(*it);
				}
				else count++;
			}
			return 0;
		}
		case GetTypeVisitor::NK_ForStatement:
			if (nPos == 1)
				return pStmt;
			if (nPos == 2)
				return &(pStmt->cast_to<ForStatement>().getBody());
			return 0;
		case GetTypeVisitor::NK_IfStatement:
			if (nPos == 1)
				return pStmt;
			if (nPos == 2)
				return &(pStmt->cast_to<IfStatement>().getThenBody());
			if (nPos == 3)
				return &(pStmt->cast_to<IfStatement>().getElseBody());
			return 0;
		case GetTypeVisitor::NK_WhileStatement:
			if (nPos == 1)
				return pStmt;
			if (nPos == 2)
				return &(pStmt->cast_to<WhileStatement>().getBody());
			return 0;
		default:
			return 0;
	}
}
bool isDeepViewNeed(StatementBase* pStmt)
{
	GetTypeVisitor visitor;
	pStmt->accept(visitor);
	switch(visitor.m_typeOfNode)
	{
	case GetTypeVisitor::NK_BlockStatement:
		return pStmt->cast_to<BlockStatement>().getChildCount() > 0;
	case GetTypeVisitor::NK_ForStatement:
		return true;
	case GetTypeVisitor::NK_IfStatement:
		return true;
	case GetTypeVisitor::NK_WhileStatement:
		return true;

	default:
		return false;
	}
}
int getMaxInt(StatementBase* pStmt)
{
	GetTypeVisitor visitor;
	pStmt->accept(visitor);
	switch(visitor.m_typeOfNode)
	{
	case GetTypeVisitor::NK_BlockStatement:
		return pStmt->cast_to<BlockStatement>().getChildCount() - 1;
	case GetTypeVisitor::NK_ForStatement:
		return 2;
	case GetTypeVisitor::NK_IfStatement:
		return 3;
	case GetTypeVisitor::NK_WhileStatement:
		return 2;
	default:
		return 1;
	}
}
									// =================== Исправить! ====================
StatementBase * id::prev()
{
	if ( m_StmtStack.size() )
	{
		myStmtStr strTemp ( m_StmtStack.back() );
		m_StmtStack.pop_back();
		StatementBase *pStmt = strTemp.m_pStmt;
		int nCurPos = strTemp.m_nCurPos - 1;

		if ( isIntBlockValid(pStmt,nCurPos) )
		{
			m_StmtStack.push_back(myStmtStr(pStmt,nCurPos));
			pStmt = getIntBlock(pStmt,nCurPos);
			while (isDeepViewNeed(pStmt))
			{
				m_StmtStack.push_back(myStmtStr(pStmt, getMaxInt(pStmt) ));
				//Block *pBlock;
				//if (!( pBlock = dynamic_cast<Block*>(pStmt) )) break;
				pStmt = getIntBlock(pStmt,getMaxInt(pStmt));

			};
			return pStmt;
		}
		else return prev();
	}
	else 	return 0;

}
///////////////////////////////////////////////////////////////////////////////

StatementBase * id::getThisOper()
{
	// если конечный оператор не задан
	if (!m_pEnd)
	{
		// если стек операторов пуст, то возвращаем ноль
		if (m_StmtStack.empty())
			return 0;

		// иначе копируем верхний оператор со стека
		myStmtStr strTemp ( m_StmtStack.back() );

		if (strTemp.m_pStmt->is_a<BlockStatement>())
		{
			// проверяем, что не вышли за границы
			if (strTemp.m_nCurPos > 0 && strTemp.m_nCurPos <= strTemp.m_pStmt->getChildCount())
			{
				// приводим к типу блока
				BlockStatement& temp = strTemp.m_pStmt->cast_to<BlockStatement>();

				// находим вложеный оператор с нужным номером
				int count = 1;
				for(BlockStatement::Iterator it = temp.getFirst(); it.isValid(); it.goNext(), ++count)
				{
					if (count == strTemp.m_nCurPos)
						return &(*it);
				}
			}
		}
		else if (strTemp.m_pStmt->is_a<ForStatement>())
		{
			if (strTemp.m_nCurPos == 1) 
				return strTemp.m_pStmt;
			if (strTemp.m_nCurPos == 2)
				return &strTemp.m_pStmt->cast_to<ForStatement>().getBody();		
			else return 0;
		}
		else if (strTemp.m_pStmt->is_a<IfStatement>())
		{
			if (strTemp.m_nCurPos == 1) 
				return strTemp.m_pStmt;
			if (strTemp.m_nCurPos == 2)
				return &strTemp.m_pStmt->cast_to<IfStatement>().getThenBody();
			if (strTemp.m_nCurPos == 3)
				return &strTemp.m_pStmt->cast_to<IfStatement>().getElseBody();
			else return 0;
		}
		else if (strTemp.m_pStmt->is_a<WhileStatement>())
		{
			if (strTemp.m_nCurPos == 1) 
				return strTemp.m_pStmt;
			if (strTemp.m_nCurPos == 2)
				return &strTemp.m_pStmt->cast_to<WhileStatement>().getBody();
			else return 0;
		}

		return 0;
	}
	else
	{
		if (m_bFindNext)
		{
			if (m_StmtStack.empty())
				return 0;

			myStmtStr strTemp ( m_StmtStack.back() );

			if (strTemp.m_pStmt->is_a<BlockStatement>())
			{
				if (strTemp.m_nCurPos>0 && strTemp.m_nCurPos<=strTemp.m_pStmt->getChildCount())
				{
					BlockStatement& temp = strTemp.m_pStmt->cast_to<BlockStatement>();

					int count = 1;
					for(BlockStatement::Iterator it = temp.getFirst(); it.isValid(); it.goNext(), ++count)
					{
						if (count == strTemp.m_nCurPos)
							return &(*it);
					}
				}
			}
			else if (strTemp.m_pStmt->is_a<ForStatement>())
			{
				if (strTemp.m_nCurPos == 1) 
					return strTemp.m_pStmt;
				if (strTemp.m_nCurPos == 2)
					return &strTemp.m_pStmt->cast_to<ForStatement>().getBody();		
				else return 0;
			}
			else if (strTemp.m_pStmt->is_a<IfStatement>())
			{
				if (strTemp.m_nCurPos == 1) 
					return strTemp.m_pStmt;
				if (strTemp.m_nCurPos == 2)
					return &strTemp.m_pStmt->cast_to<IfStatement>().getThenBody();
				if (strTemp.m_nCurPos == 3)
					return &strTemp.m_pStmt->cast_to<IfStatement>().getElseBody();
				else return 0;
			}
			else if (strTemp.m_pStmt->is_a<WhileStatement>())
			{
				if (strTemp.m_nCurPos == 1) 
					return strTemp.m_pStmt;
				if (strTemp.m_nCurPos == 2)
					return &strTemp.m_pStmt->cast_to<WhileStatement>().getBody();		
				else return 0;
			}
		}

		return 0;
	};
}


void id::print()
{
	if (m_pFirst) 	cout << _TL("First ","") << m_pFirst->dumpState() << endl;
	if (m_pEnd) cout << _TL("Last","") << m_pEnd->dumpState() << endl;
	for(unsigned int i = 0; i<m_StmtStack.size(); i++ )
	{
		cout << i << endl; 
		m_StmtStack[i].print();
	}
}

void getOuterLoopsList(StatementBase& stmt, std::vector<LoopInfo>& loops)
{
	loops.clear();

	StatementBase * pStmt = &stmt;

	while (pStmt)
	{
		if (pStmt->is_a<ForStatement>())
		{
			LoopInfo li(pStmt->cast_to<ForStatement>());
			if (li.pthis != 0)
				loops.insert(loops.begin(), li);
		};

		pStmt = dynamic_cast<StatementBase*>(pStmt->getParent()); 
	};
}

void id::getOuterLoopsList(std::vector<LoopInfo> & A, bool bGoTop)
{
	A.clear(); 
	LoopInfo a;

	if (bGoTop) 
	{ 
		if (getThisOper())
			Id::getOuterLoopsList(*getThisOper(), A);
	} 
	else
	{
		for(unsigned int i = 0; i<m_StmtStack.size(); i++ ) 
		{ 
			StatementBase * pStmt = m_StmtStack[i].m_pStmt; 
			if (ForStatement * pFor = dynamic_cast<ForStatement*>(pStmt))
			{
				LoopInfo li(*pFor);
				if (li.pthis != 0)
					A.push_back(li);
			}; 
		}; 
	}
}

int id::initWithLoopBody(StatementBase * pLoop)
{
	if ( pLoop->is_a<ForStatement>() )
	{
		reset(pLoop->cast_to<ForStatement>().getBody()); 
		return 0;
	}
	else return 1;
}


bool ExprManagement::findSubExpr(ExpressionBase * pExpr, ExpressionBase * pSource)
{
	if(!pSource->is_a<ReferenceExpression>())
		return false;
	GetTypeVisitor visitor;
	pExpr->accept(visitor);
	switch(visitor.m_typeOfNode)
	{
		case GetTypeVisitor::NK_BasicCallExpression:
		{
			BasicCallExpression * ptemp = dynamic_cast<BasicCallExpression*>(pExpr);
			if ( ptemp )
			{
				if(ptemp->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
				{
					return findSubExpr( &ptemp->getArgument(0), pSource);
				}
				if(ptemp->getArgumentCount()==2)
					return( findSubExpr( &ptemp->getArgument(0), pSource)  || 
						findSubExpr( &ptemp->getArgument(1), pSource)  
					) ;
				if(ptemp->getArgumentCount()==1)
					return findSubExpr( &ptemp->getArgument(0), pSource);
			};
		};
		break;
		case GetTypeVisitor::NK_SubroutineCallExpression:
		return false;
		break;
		case GetTypeVisitor::NK_ReferenceExpression:
		{
			const ReferenceExpression *ptemp = dynamic_cast<const ReferenceExpression*>(pExpr) ;
			if (ptemp )
			{
				return ptemp->isEqual(*pSource);
			}
		};
		break;
		OPS_DEFAULT_CASE_LABEL;
	};
	return false;
}


ReprisePtr<ExpressionBase> getCoef(ExpressionBase& source, VariableDeclaration& itNO)
{
	std::vector<VariableDeclaration*> indexes;
	indexes.push_back(&itNO);

	std::unique_ptr<OPS::Shared::ParametricLinearExpression> expr (
		OPS::Shared::ParametricLinearExpression::createByListOfVariables(&source, indexes));
	
	return expr->getCoefficient(&itNO);
}

ReprisePtr<ExpressionBase> getFreeCoef(ExpressionBase& source, const std::vector<VariableDeclaration*>& expBase)
{
	std::unique_ptr<OPS::Shared::ParametricLinearExpression> expr (
		OPS::Shared::ParametricLinearExpression::createByListOfVariables(&source, expBase));
	
	return expr->getFreeCoefficient();

}

bool checkLinByBases(ExpressionBase * pSource, std::vector<ExpressionBase*>& pExpBase)
{
		std::vector<VariableDeclaration*> vInd;
		std::vector<ExpressionBase*>::iterator it = pExpBase.begin();
		for (; it!=pExpBase.end(); it++)
		{
			if (*it == 0)
				throw OPS::RuntimeError("checkLinByBases: base expression is NULL");
			if (!(*it)->is_a<ReferenceExpression>())
				return false;
			vInd.push_back( &((*it)->cast_to<ReferenceExpression>().getReference()) );
		}
        return OPS::Shared::isLinear(vInd, pSource);
	
}

}
}
