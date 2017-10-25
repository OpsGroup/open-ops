#include "Analysis/AliasAnalysis/CallGraphEx.h"

namespace OPS
{
namespace AliasAnalysis
{
CallGraphEx::CallGraphEx(TranslationUnit* aTranslationUnit): m_TranslationUnit(aTranslationUnit)
{
	build();	
	for (Declarations::SubrIterator iter = m_TranslationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		if (iter->asSubroutine().getName()=="main")
		{
			m_ExprCallExMain = new ExprCallEx(NULL, &iter->getBodyBlock());
		}
	}
}

CallGraphEx::~CallGraphEx(void)
{
	NodeMap::iterator it;
	for(it=m_Graph.begin();it!=m_Graph.end();++it) 
	{
		delete (*it).second;
	}
}

void CallGraphEx::build()
{
	// обход всех подпрограмм
	for (Declarations::SubrIterator iter = m_TranslationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		buildSubProc(*iter);
	}
}

void CallGraphEx::buildSubProc(SubroutineDeclaration& pSubProcDeclaration)
{
	Node *procNode=new Node(&pSubProcDeclaration);
	m_Graph[pSubProcDeclaration.getName()] = procNode;

	// рекурсивный вызов для вложенных подпрограмм
	for (Declarations::SubrIterator iter = pSubProcDeclaration.getBodyBlock().getDeclarations().getFirstSubr(); iter.isValid(); ++iter)
	{
		buildSubProc(*iter);
	}
	parseBlock(procNode, pSubProcDeclaration.getBodyBlock());
}

void CallGraphEx::parseBlock(Node* procNode, const BlockStatement& aBlock)
{
	// отлавливаем вызовы подпрограмм
	{

		BlockStatement::ConstIterator it= aBlock.getFirst();
		for(;it.isValid();++it)
		{
			//
			// цикл по всем операторам, в которых может быть вызов подпрограмм
			//
			// цикл по всем операторам блока
			if((*it).is_a<BlockStatement>())
			{
				parseBlock(procNode, static_cast<const BlockStatement&>(*it));
			}
			// самый легкий случай - оператор-выражение
			if((*it).is_a<ExpressionStatement>())
			{
				parseExpression(procNode, &(static_cast<const ExpressionStatement*>(&*it))->get());
			}
			else
				if((*it).is_a<ForStatement>())
				{
					const ForStatement* stmtFor=static_cast<const ForStatement*>(&*it);
					parseExpression(procNode, &stmtFor->getInitExpression());
					parseExpression(procNode, &stmtFor->getFinalExpression());
					parseExpression(procNode, &stmtFor->getStepExpression());
					parseBlock(procNode, stmtFor->getBody());
				}
				else 
					if((*it).is_a<IfStatement>())
					{
						const IfStatement* stmtIf=static_cast<const IfStatement*>(&*it);
						parseExpression(procNode, &stmtIf->getCondition());
						parseBlock(procNode, stmtIf->getThenBody());
						parseBlock(procNode, stmtIf->getElseBody());
					}
					else
						if((*it).is_a<WhileStatement>())
						{
							const WhileStatement* stmtWhile = static_cast<const WhileStatement*>(&*it);
							parseExpression(procNode, &stmtWhile->getCondition());
							parseBlock(procNode, stmtWhile->getBody());
						}
						else
							if((*it).is_a<ReturnStatement>())
							{
								const ReturnStatement* stmtReturn=static_cast<const ReturnStatement*>(&*it);
								parseExpression(procNode, &stmtReturn->getReturnExpression());
							}
							else
								if((*it).is_a<EmptyStatement>())
								{
									// do nothing
								}
								else
								{
									//OPS::OPSAssert(false, _TL("Not implemented for this kind of statement yet",""));
								}
		}
	}
}

const CallGraphEx::NodeMap& CallGraphEx::getGraph() const
{
	return m_Graph;
}

CallGraphEx::Node::Node(SubroutineDeclaration* aSubProc): m_SubProc(aSubProc)
{
}

CallGraphEx::Node::~Node(void)
{
	SubProcCallMap::iterator it;
	for(it=m_calls.begin();it!=m_calls.end();++it)
	{
		ExprCallExList::iterator it_EC;
		for(it_EC=(*it).second->begin();it_EC!=(*it).second->end();++it_EC)
		{
			delete (*it_EC);
		}
		delete (*it).second;
	}
}

void CallGraphEx::Node::addNode(const SubroutineDeclaration* aSubProc, const SubroutineCallExpression* callSite)
{
	SubProcCallMap::iterator it;
	ExprCallExList* aList=0;
	if ((it=m_calls.find(aSubProc))==m_calls.end())
	{
		m_calls[aSubProc]=(aList=new ExprCallExList());
	}
	else
		aList=(*it).second;
	ExprCallEx *aExprCallEx = new ExprCallEx(callSite, const_cast<BlockStatement*>(&aSubProc->getBodyBlock()));
	aList->push_back(aExprCallEx);
}

const SubroutineDeclaration* CallGraphEx::Node::getSubProc() const
{
	return m_SubProc;
}

SubroutineDeclaration* CallGraphEx::Node::getSubProc()
{
	return m_SubProc;
}

const CallGraphEx::SubProcCallMap& CallGraphEx::Node::getCalls() const
{
	return m_calls;
}

void CallGraphEx::parseExpression(CallGraphEx::Node* pNode, const ExpressionBase* expression)
{
	if(expression)
	{
		// вызов функции надо занести в список вызовов...
		if(expression->is_a<SubroutineCallExpression>())
		{
			const SubroutineCallExpression* exprCall = static_cast<const SubroutineCallExpression*>(expression);
			// нашли выражение-вызов подпрограммы
			
			// по вызову подпрограммы получим ее объявление
			if(exprCall->hasExplicitSubroutineDeclaration()) {
				const SubroutineDeclaration* pDecl = 
					&exprCall->getExplicitSubroutineDeclaration();
				if(pDecl)
				{
					pNode->addNode(pDecl, exprCall);
				}
			}
		}
		// для выражений-операций надо рекурсивно обрабатывать их подвыражения
		if (expression->is_a<CallExpressionBase>())
		{
			const CallExpressionBase* exprCall = static_cast<const CallExpressionBase*>(expression);
			int paramCount = exprCall->getArgumentCount();
			// вызываем себя рекурсивно для каждого своего аргумента
			for(int paramIndex=0; paramIndex < paramCount; ++paramIndex)
			{
				parseExpression(pNode, &exprCall->getArgument(paramIndex));
			}
		}
	}
}

ExprCallEx* CallGraphEx::GetSubProcExprCallEx(const ExprName& procName, const SubroutineCallExpression* exprCall)
{
	if (!this->getGraph().empty())
	{
		typedef CallGraphEx::SubProcCallMap::const_iterator Spcm_Ci;
		CallGraphEx::NodeMap nodeMap = this->getGraph();			
		CallGraphEx::SubProcCallMap vCalls = nodeMap[procName]->getCalls();
		for (Spcm_Ci q=vCalls.begin(); q!=vCalls.end(); ++q)
		{
			ExprCallExList::const_iterator it=q->second->begin();
			for (; it!=q->second->end(); ++it)
			{
				if (exprCall == (*it)->GetExprCall())
					return const_cast<ExprCallEx*>(*it);
			}
		}		
	}
	return NULL;
}

ExprCallEx* CallGraphEx::GetSubProcExprCallEx(const SubroutineCallExpression* exprCall)
{
	if (!this->getGraph().empty())
	{
		typedef CallGraphEx::SubProcCallMap::const_iterator Spcm_Ci;
		NodeMap::iterator it;
		for(it=m_Graph.begin();it!=m_Graph.end();++it) 
		{
			CallGraphEx::SubProcCallMap vCalls = it->second->getCalls();
			for (Spcm_Ci q=vCalls.begin(); q!=vCalls.end(); ++q)
			{
				ExprCallExList::const_iterator it=q->second->begin();
				for (; it!=q->second->end(); ++it)
				{
					if (exprCall == (*it)->GetExprCall())
						return const_cast<ExprCallEx*>(*it);
				}
			}
		}
	}
	return NULL;
}

} // end namespace AliasAnalysis
} // end namespace OPS
