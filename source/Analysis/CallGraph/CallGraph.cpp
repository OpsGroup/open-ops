#include "Analysis/CallGraph.h"

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS;
using namespace OPS::Reprise;

CallGraph::CallGraph(ProgramUnit* aProgramUnit): m_CurrentNodes() 
{
	aProgramUnit->accept(*this);
}

CallGraph::CallGraph(TranslationUnit* aTranslationUnit)
{
	aTranslationUnit->accept(*this);
}

CallGraph::CallGraph(OPS::Reprise::SubroutineDeclaration *aSubroutine)
{
	SubroutineSet notVisited;

	// Начинаем с корневой функции
	notVisited.insert(aSubroutine);

	buildByBFS(notVisited);
}

CallGraph::CallGraph(OPS::Reprise::StatementBase* aStatement)
{
	// Добавляем фиктивную вершину для фрагмента программы
	Node* fragmentNode = new Node(0);
	m_Graph["__FRAGMENT__"] = fragmentNode;

	// Находим все вызовы из фрагмента
	SafeStackPopper safePopper(m_CurrentNodes, fragmentNode);
	aStatement->accept(*this);

	// дальше строим обходом в ширину
	buildByBFS(SubroutineSet());
}

CallGraph::~CallGraph(void)
{
	NodeMap::iterator it;
	for(it=m_Graph.begin();it!=m_Graph.end();++it) 
	{
		delete (*it).second;
	}
}

const CallGraph::NodeMap& CallGraph::getGraph() const
{
	return m_Graph;
}

CallGraph::Node::Node(SubroutineDeclaration *aSubProc): m_SubProc(aSubProc)
{
}

CallGraph::Node::~Node()
{
	SubProcCallMap::iterator it;
	for(it=m_calls.begin();it!=m_calls.end();++it)
	{
		delete (*it).second;
	}
}

void CallGraph::Node::addNode(const SubroutineDeclaration *aSubProc, const SubroutineCallExpression* callSite)
{
	SubProcCallMap::iterator it;
	ExprCallList* aList=0;
	if ((it=m_calls.find(aSubProc))==m_calls.end())
	{
		m_calls[aSubProc]=(aList=new ExprCallList());
	}
	else
		aList=(*it).second;
	aList->push_back(callSite);
}

const SubroutineDeclaration *CallGraph::Node::getSubProc() const
{
	return m_SubProc;
}

SubroutineDeclaration *CallGraph::Node::getSubProc()
{
	return m_SubProc;
}

const CallGraph::SubProcCallMap& CallGraph::Node::getCalls() const
{
	return m_calls;
}

std::string CallGraph::dumpState() const
{
	std::string result;
	result += "digraph CallGraph {";
	std::string nodes;
	std::string edges;

	CallGraph::NodeMap::const_iterator pNode = m_Graph.begin();
	for(; pNode != m_Graph.end(); ++pNode)
	{
		nodes += pNode->first + ";";
		nodes.push_back('\n');
		if(!(pNode->second->getCalls().empty()))
		{
			CallGraph::SubProcCallMap::const_iterator pCall = pNode->second->getCalls().begin();
			for(; pCall != pNode->second->getCalls().end(); ++pCall)
			{
				edges += pNode->first;
				edges += " -> ";
				edges += pCall->first->getName();
				edges += ";";
				edges.push_back('\n');
			}
		}
		edges.push_back('\n');
	}
	result += nodes;
	result += edges;

	result += "}";
	return result;
}

void CallGraph::removeExcessNodes(OPS::Reprise::ProgramUnit* aProgramUnit)
{
	std::set<std::string> excessNodes;
	// узлы со степенью исхода, большей нуля
	std::set<std::string> rPlusNodes;
	// узлы со степенью захода, большей нуля
	std::set<std::string> rMinusNodes;
	NodeMap::iterator pNode = m_Graph.begin();
	// находим вершины с ненулевыми степенями
	for(; pNode != m_Graph.end(); ++pNode)
	{
		SubProcCallMap::iterator pCallee = pNode->second->m_calls.begin();
		for(; pCallee != pNode->second->m_calls.end(); ++pCallee)
		{
			const SubroutineDeclaration* pSubProc = pCallee->first;
			std::string calleeName = pSubProc->getName();
			rPlusNodes.insert(pNode->first);
			rMinusNodes.insert(calleeName);
		}
	}

	// находим вершины с нулевыми степенями и не содержащиеся в заданной 
	// единице трансляции
	for(pNode = m_Graph.begin(); pNode != m_Graph.end(); ++pNode)
	{
		if(rPlusNodes.find(pNode->first) == rPlusNodes.end() && 
			rMinusNodes.find(pNode->first) == rMinusNodes.end())
		{
			SubroutineDeclaration* pSubProc = pNode->second->m_SubProc;
			
			SourceCodeManager& codeManager = RepriseContext::defaultContext().getSourceCodeManager();
			SourceCodeLocation location1 = codeManager.getLocation(*pSubProc);
			bool isInLocalFile = false;
			for(int nTranslationUnit = 0; nTranslationUnit != aProgramUnit->getUnitCount(); nTranslationUnit++)
			{
				TranslationUnit* aTranslationUnit = &(aProgramUnit->getUnit(nTranslationUnit));
				std::string fileName = 	aTranslationUnit->getSourceFilename();
				int fileId = codeManager.getFileId(fileName.c_str());
				if(location1.FileId == fileId)
				{
					isInLocalFile = true;
				}
			}
			if(!isInLocalFile)
			{
				excessNodes.insert(pNode->first);
			}
		}
	}

	// заполняем новый граф без ненужных вершин
	NodeMap newGraph;
	for(pNode = m_Graph.begin(); pNode != m_Graph.end(); ++pNode)
	{
		if(excessNodes.find(pNode->first) == excessNodes.end())
		{
			Node* newNode = new Node(pNode->second->m_SubProc);
			newGraph[pNode->first] = newNode;
			SubProcCallMap::iterator pCallee = pNode->second->m_calls.begin();
			for(; pCallee != pNode->second->m_calls.end(); ++pCallee)
			{
				if(excessNodes.find(pCallee->first->getName()) == excessNodes.end())
				{
					ExprCallList::iterator pExprCall = pCallee->second->begin();
					for(; pExprCall != pCallee->second->end(); ++pExprCall)
					{
						newNode->addNode(pCallee->first, *pExprCall);
					}
				}
			}
		}
	}
	this->m_Graph.swap(newGraph);
}

void CallGraph::visit(OPS::Reprise::Declarations& decls)
{
	// обход всех подпрограмм
	typedef std::list<Node*> NodeList;
	NodeList newNodes;

	for (Declarations::SubrIterator iter = decls.getFirstSubr(); iter.isValid(); ++iter)
	{
		SubroutineDeclaration& subProcDeclaration = *iter;

		NodeMap::iterator itExisting = m_Graph.find(subProcDeclaration.getName());

		if (itExisting == m_Graph.end())
		{
			Node* newNode = new Node(&subProcDeclaration);
			m_Graph.insert(std::make_pair(subProcDeclaration.getName(), newNode));
			newNodes.push_back(newNode);
		}
		else if (subProcDeclaration.hasImplementation())
		{
			newNodes.remove(itExisting->second);
			delete itExisting->second;			
			itExisting->second = new Node(&subProcDeclaration);
			newNodes.push_back(itExisting->second);
		}
	}

	for (NodeList::const_iterator it = newNodes.begin(); it != newNodes.end(); ++it)
	{
		SafeStackPopper safePopper(m_CurrentNodes, *it);
		(*it)->getSubProc()->accept(*this);
	}
}

void CallGraph::visit(OPS::Reprise::SubroutineCallExpression& exprCall)
{
	// по вызову подпрограммы получим ее объявление
	if(exprCall.hasExplicitSubroutineDeclaration()) {
		const SubroutineDeclaration* pDecl = 
			&(exprCall.getExplicitSubroutineDeclaration());
		if(pDecl)
		{
			m_CurrentNodes.top()->addNode(pDecl, &exprCall);
		}
	}

	DeepWalker::visit(exprCall);
}

void CallGraph::buildByBFS(CallGraph::SubroutineSet notVisited)
{
	// Пока есть непосещенные функции
	do
	{
		// Посещаем все еще непосещенные функции
		for(SubroutineSet::iterator it = notVisited.begin(); it != notVisited.end(); ++it)
		{
			Node* newNode = new Node(*it);
			m_Graph[(*it)->getName()] = newNode;

			SafeStackPopper safePopper(m_CurrentNodes, newNode);
			if ((*it)->hasDefinition())
				(*it)->getDefinition().accept(*this);
			else
				(*it)->accept(*this);
		}

		notVisited.clear();

		// Ищем новые вершины в графе
		for(NodeMap::iterator itNode = m_Graph.begin(); itNode != m_Graph.end(); ++itNode)
		{
			const SubProcCallMap& callMap = itNode->second->getCalls();

			for(SubProcCallMap::const_iterator itCall = callMap.begin(); itCall != callMap.end(); ++itCall)
			{
				if (m_Graph.find(itCall->first->getName()) == m_Graph.end())
				{
					// в графе еще нет вершины, соответствующей этой функции
					notVisited.insert(const_cast<SubroutineDeclaration*>(itCall->first));
				}
			}
		}
	}
	while(!notVisited.empty());
}