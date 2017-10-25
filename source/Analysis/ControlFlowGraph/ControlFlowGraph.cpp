#include "Analysis/ControlFlowGraph.h"
#include "Shared/NodesCollector.h"
#include <iostream>
#include <sstream>
#include <map>

#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

ControlFlowGraph::~ControlFlowGraph()
{
	StatementGraph::iterator it(m_graph.begin());
	for(; it!=m_graph.end();++it)
	{
		try 
		{ 
			delete it->second; 
		}
		catch(...) {}
	}
	m_graph.clear();
	EdgeList::iterator jt(m_minEdgeSet.begin());
	for(; jt!=m_minEdgeSet.end(); ++jt)
	{
		try
		{
			delete it->second;
		}
		catch(...) {}
	}
	m_minEdgeSet.clear();
}

ControlFlowGraph::ControlFlowGraph(const BlockStatement &Block): m_pBlock(Block)
{
	m_statementVector.reserve(8);
	SafeStackPopper sp(m_controls, 0);
	const_cast<BlockStatement*>(&Block)->accept(*this);
	//parseStatement(Block);
}

void ControlFlowGraph::checkStatementVector()
{
	if(m_statementVector.capacity()==m_statementVector.size())
	{
		m_statementVector.reserve(m_statementVector.capacity()*2);
	}
}

const StatementBase& ControlFlowGraph::getStatementEntry(const StatementBase &stmt) const
{
	if (const WhileStatement* stmtWhile = stmt.cast_ptr<const WhileStatement>())
	{
		if (!stmtWhile->isPreCondition())
			return *firstStatement(stmtWhile->getBody());
	}
	return stmt;
}

// возвращает текущий список концов дуг управляющего графа, выходящих из statement
// создает новый список, если его еще нет
ControlFlowGraph::StatementList& ControlFlowGraph::safeRetrieveList(const StatementBase& statement)
{
	StatementGraph::const_iterator it(m_graph.find(&statement));
	if (it!=m_graph.end())
	{
		return  *it->second;
	}
	else
	{
		return *(m_graph[&statement]=new StatementList);
	}
}

const StatementBase* ControlFlowGraph::firstStatement(const StatementBase& statement) const
{
	if(statement.is_a<BlockStatement>())
	{
		const BlockStatement& block = statement.cast_to<BlockStatement>();
		if(block.isEmpty()) return &block;
		else
			return firstStatement(getStatementEntry(*block.getFirst())); 
	} else
		return &statement;
}

// Возвращает оператор, на который возможна передача управления от текущего (наружу)
const StatementBase* ControlFlowGraph::next(const StatementBase& statement) const
{
	const StatementBase* result = nextStatement(statement);
	if(result == NULL) return result;
	return firstStatement(*result);
}

const StatementBase* ControlFlowGraph::nextStatement(const StatementBase& statement) const
{
	if (statement.is_a<BlockStatement>())
	{
		if (statement.getParent() != 0 && statement.getParent()->is_a<StatementBase>())
		{
			const StatementBase& parent = statement.getParent()->cast_to<const StatementBase>();
			// Если есть предок и он является циклом, то управление передастся на него
			if (parent.is_a<ForStatement>() || parent.is_a<WhileStatement>())
				return &parent;
		}
	}

	if(statement.hasParentBlock())
	{
		//Найдем текущий оператор в списке операторов родительского блока
		const BlockStatement& parentBlock = statement.getParentBlock();
		BlockStatement::ConstIterator it = parentBlock.convertToIterator(&statement);
		// если непосредственный следующий оператор есть, возвращаем его, иначе - следующий для родительского
		if (it.isValid() && (++it).isValid()) 
		{
			return &getStatementEntry(*it);
		}
		else 
		{
			if (statement.hasParentBlock() && 
				statement.getParentBlock().is_a<BlockStatement>())
				return nextStatement(statement.getParentBlock());
			else
				return m_controls.top();
		}
	}
	else
		return m_controls.top();
}

// for testing purposes only!
void ControlFlowGraph::PrintToCout() const
{
	StatementGraph::const_iterator it = m_graph.begin();
	for(; it != m_graph.end(); ++it)
	{
		std::cout << "next statement: " << std::endl;
		std::cout << (it->first)->dumpState() << std::endl;
		std::cout << "edges for this statement: " << std::endl;
		StatementList::const_iterator jt = (*it->second).begin();
		for(; jt != (*it->second).end(); ++jt)
		{
			if(*jt != 0)
			std::cout << (*jt)->dumpState() << std::endl;
		}
	}
}

std::string ControlFlowGraph::dumpState() const
{
	std::string graphDump;
	std::ostringstream dumper(graphDump);
	std::string edgesDump;
	std::ostringstream edgesDumper(edgesDump);
	dumper << "digraph ControlFlowGraph {" << std::endl;

	std::map<const StatementBase*, int> numbering;
	StatementVector::const_iterator pStatement = m_statementVector.begin();
	for(int nNode = 0; pStatement != m_statementVector.end(); ++pStatement)
	{
		numbering[*pStatement] = nNode;
		++nNode;
	}

	StatementGraph::const_iterator pStatementList = m_graph.begin();
	for(;pStatementList != m_graph.end(); ++pStatementList)
	{
		const StatementBase* pStatement = pStatementList->first;
		std::string typeName = typeid(pStatement).name();
		TSourceCodeLocation line = pStatement->getLocationId();
		dumper << numbering[pStatement] << " "<< "[ label = \"" << typeName << " " << line << "\"];" << std::endl;
		StatementList::const_iterator pEdge = pStatementList->second->begin();
		for(; pEdge != pStatementList->second->end(); ++pEdge)
		{
			edgesDumper << numbering[pStatement] << "->" << numbering[*pEdge] << ";" << std::endl;
		}
	}

	dumper << edgesDumper.str();
	dumper << "}" << std::endl;
	graphDump = dumper.str();
	return graphDump;
}

bool ControlFlowGraph::hasEdge(const Reprise::StatementBase* pSource, const Reprise::StatementBase* pDestination) const
{
	StatementGraph::const_iterator pAdjacentNodes = m_graph.find(pSource);
	OPS_ASSERT(pAdjacentNodes != m_graph.end());
	const StatementList* adjacentNodes = pAdjacentNodes->second;
	StatementList::const_iterator pNode = std::find(adjacentNodes->begin(), adjacentNodes->end(), pDestination);
	if(pNode != adjacentNodes->end()) 
		return true;
	else
		return false;
}

void ControlFlowGraph::buildPaths() const
{
	StatementGraph::const_iterator it = m_graph.begin();
	int nodeNumber = 0;
	for(; it != m_graph.end(); ++it, ++nodeNumber)
	{
		m_NodesNumbering[it->first] = nodeNumber;
		m_ReverseNodesNumbering[nodeNumber] = it->first;
	}
	
	int nodesCount = nodeNumber;
	for(int index = 0; index < nodesCount; ++index)
	{
		m_Paths.push_back(std::valarray<bool>(false, nodesCount));
		for(int jndex = 0; jndex < nodesCount; ++jndex)
		{
			const Reprise::StatementBase* pSource = m_ReverseNodesNumbering[index];
			const Reprise::StatementBase* pDestination = m_ReverseNodesNumbering[jndex];
			if(hasEdge(pSource, pDestination)) 
			{
				m_Paths.back()[jndex] = true;
			}
		}	
	}
	
	// Алгоритм Флойда
	for(int k = 0; k < nodesCount; ++k)
	{
		for(int i = 0; i < nodesCount; ++i)
		{
			for(int j = 0; j < nodesCount; ++j)
			{
				if(m_Paths[i][k] && m_Paths[k][j]) m_Paths[i][j] = true;
			}
		}
	}
}

ControlFlowGraph::NodesNumbering::const_iterator ControlFlowGraph::findVertice(const StatementBase& stmt) const
{
    NodesNumbering::const_iterator p = m_NodesNumbering.find(&stmt);
    const StatementBase* s = &stmt;
    while (p == m_NodesNumbering.end())
    {
        if (s->is_a<BlockStatement>())
        {
            BlockStatement::ConstIterator it = s->cast_to<BlockStatement>().getFirst();
            if (it.isValid()) s = &*it;
            else break;
        }
        else break;
        p = m_NodesNumbering.find(s);
    }
    return p;
}

bool ControlFlowGraph::hasPath(const Reprise::StatementBase* source, const Reprise::StatementBase* destination) const
{
	if(!pathsBuilt()) buildPaths();
    NodesNumbering::const_iterator pSource = findVertice(*source);
    NodesNumbering::const_iterator pDestination = findVertice(*destination);
	// проверка, что все аргументы - это действительно узлы графа
	if (pSource == m_NodesNumbering.end())
		throw OPS::RuntimeError("ControlFlowGraph::hasPath: source statement was not found.");
	if (pDestination == m_NodesNumbering.end())
		throw OPS::RuntimeError("ControlFlowGraph::hasPath: destination statement was not found.");
	int sourcePaths = pSource->second;
	int destinationNumber = pDestination->second;
	return m_Paths[sourcePaths][destinationNumber];
}

ControlFlowGraph::EdgeList ControlFlowGraph::getInEdges(const StatementBase &stmt) const
{
	EdgeList list;

	// получаем список всех операторов внутри stmt
	std::set<const StatementBase*> internalStatementsSet;
	OPS::Shared::collectNodes2<const StatementBase>(stmt, internalStatementsSet);

	// перебираем все операторы
	StatementGraph::const_iterator outIt = m_graph.begin();
	for(; outIt != m_graph.end(); ++outIt)
	{
		// если оператор находится снаружи stmt
		if (internalStatementsSet.find(outIt->first) == internalStatementsSet.end())
		{
			// перебираем все операторы в которые ведет управление
			StatementList::const_iterator inIt = outIt->second->begin();
			for(; inIt != outIt->second->end(); ++inIt)
			{
				// если управление ведет внутрь stmt - заносим в список
				if (internalStatementsSet.find(*inIt) != internalStatementsSet.end())
				{
					list.push_back(std::make_pair(outIt->first, *inIt));
					break;
				}
			}
		}
	}

	return list;
}

ControlFlowGraph::EdgeList ControlFlowGraph::getOutEdges(const StatementBase &stmt) const
{
	EdgeList list;

	typedef std::set<const StatementBase*> StatementSet;

	// получаем список операторов внутри stmt
	StatementSet internalStatamentsSet;
	OPS::Shared::collectNodes2<const StatementBase>(stmt, internalStatamentsSet);

	// перебираем операторы внутри stmt
	for(StatementSet::const_iterator it = internalStatamentsSet.begin();
		it != internalStatamentsSet.end(); ++it)
	{
		if((*it)->is_a<BlockStatement>()) continue;
		// получаем список исходящих дуг
		const StatementList& outEdges = (*this)[*it];

		for(StatementList::const_iterator outIt = outEdges.begin(); outIt != outEdges.end(); ++outIt)
		{
			// проверяем, ведет ли дуга наружу stmt
			if (internalStatamentsSet.find(*outIt) == internalStatamentsSet.end())
				list.push_back(std::make_pair(*it, *outIt));
		}
	}

	return list;
}

const StatementBase* ControlFlowGraph::preVisit(StatementBase& statement, StatementList*& l)
{
	checkStatementVector();
	m_statementVector.push_back(&statement);
	l = &safeRetrieveList(statement);
	const StatementBase* nextStmt=next(statement);
	return nextStmt;
}

void ControlFlowGraph::visit(OPS::Reprise::BlockStatement& block)
{
	checkStatementVector();
	StatementList* list = NULL;
	RepriseBase& genericParent = *(block.getParent());
	if(genericParent.is_a<ForStatement>() || 
		genericParent.is_a<WhileStatement>())
	{
		m_statementVector.push_back(&block);
		list = &safeRetrieveList(block);
	} else {
		if(genericParent.is_a<SubroutineDeclaration>())
		{
			m_statementVector.push_back(&block);
			list = &safeRetrieveList(block);

			BlockStatement::ConstIterator it(block.getFirst());
			// поставим дугу из текущего оператора (тело функции)
			// на первый оператор тела
			// если конечно тело не пусто
			if(!block.isEmpty())
			{
				list->push_back(&getStatementEntry(*firstStatement(*it)));
			}
			list = NULL;
		}
	}
	const StatementBase* nextStmt = next(block);
	SafeStackPopper sp(m_controls, nextStmt);
	if (!block.isEmpty())
	{
		DeepWalker::visit(block);
	}
	else
	{
		// блок пуст, внутри нет передач управления
	}
	// следующий по тексту оператор
	// если нужно указать
	if(list != NULL && block.isEmpty())
	{
		list->push_back(nextStmt);
	}
}
void ControlFlowGraph::visit(OPS::Reprise::ForStatement& forStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(forStatement, list);
	SafeStackPopper sp(m_controls, nextStmt);
	SafeStackPopper spb(m_breaks, nextStmt);
	SafeStackPopper spc(m_continues, &getStatementEntry(*firstStatement(forStatement)));
	// тело цикла
	list->push_back(&getStatementEntry(*firstStatement(forStatement.getBody())));
	DeepWalker::visit(forStatement);
	// следующий по тексту оператор
	list->push_back(nextStmt);
}
void ControlFlowGraph::visit(OPS::Reprise::WhileStatement& whileStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(whileStatement, list);
	SafeStackPopper sp(m_controls, nextStmt);
	SafeStackPopper spb(m_breaks, nextStmt);
	SafeStackPopper spc(m_continues, &getStatementEntry(*firstStatement(whileStatement)));
	// тело цикла
	list->push_back(&getStatementEntry(*firstStatement(whileStatement.getBody())));
	DeepWalker::visit(whileStatement);
	// следующий по тексту оператор
	list->push_back(nextStmt);
}
void ControlFlowGraph::visit(OPS::Reprise::IfStatement& ifStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(ifStatement, list);
	SafeStackPopper sp(m_controls, nextStmt);
	if(!ifStatement.getThenBody().isEmpty())
		list->push_back(&getStatementEntry(*firstStatement(*(ifStatement.getThenBody().getFirst()))));
	if(!ifStatement.getElseBody().isEmpty())
	{
		list->push_back(&getStatementEntry(*firstStatement(*(ifStatement.getElseBody().getFirst()))));
	} else {
		// если блок else пуст, поставим дугу из if на следующий оператор
		// вместо такой дуги из пустого блока
		list->push_back(nextStmt);
	}
	
	DeepWalker::visit(ifStatement);
}

void ControlFlowGraph::visit(OPS::Reprise::PlainSwitchStatement& switchStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(switchStatement, list);
	SafeStackPopper sp(m_controls, nextStmt);
	std::set<const StatementBase*> cases;
	bool hasDefault = false;
	for(int i = 0; i < switchStatement.getLabelCount(); ++i)
	{
		PlainCaseLabel* pLabel = &(switchStatement.getLabel(i));
		cases.insert(&getStatementEntry(*firstStatement(pLabel->getStatement())));
		if(pLabel->isDefault()) hasDefault = true;
	}
	list->insert(list->end(), cases.begin(), cases.end());
	// если нет метки по умолчанию, то управление может
	// передаваться еще и следующему оператору
	if(!hasDefault) 
	{
		list->push_back(nextStmt);
	}

	DeepWalker::visit(switchStatement);
}

void ControlFlowGraph::visit(OPS::Reprise::Canto::HirBreakStatement& breakStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(breakStatement, list);
	OPS_UNUSED(nextStmt);
	// передача управления осуществляется на оператор, следующий за внутренним циклом
	list->push_back(m_breaks.top());
}
void ControlFlowGraph::visit(OPS::Reprise::Canto::HirContinueStatement& continueStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(continueStatement, list);
	OPS_UNUSED(nextStmt);
	// передача управления осуществляется на сам оператор внутреннего цикла
	list->push_back(m_continues.top());
}
void ControlFlowGraph::visit(OPS::Reprise::GotoStatement& gotoStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(gotoStatement, list);
	OPS_UNUSED(nextStmt);
	// передача управления возможна только на помеченный меткой оператор
	list->push_back(&getStatementEntry(*firstStatement(*(gotoStatement.getPointedStatement()))));
}
void ControlFlowGraph::visit(OPS::Reprise::EmptyStatement& emptyStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(emptyStatement, list);
	list->push_back(nextStmt);
}

void ControlFlowGraph::visit(OPS::Reprise::ReturnStatement& returnStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(returnStatement, list);
	OPS_UNUSED(nextStmt);
	list->push_back(0);
}

void ControlFlowGraph::visit(OPS::Reprise::ExpressionStatement& expressionStatement)
{
	StatementList* list = NULL;
	const StatementBase* nextStmt = preVisit(expressionStatement, list);
	list->push_back(nextStmt);
}

ControlFlowGraphExpr::ControlFlowGraphExpr()
{
}

ControlFlowGraphExpr::~ControlFlowGraphExpr()
{
    clear();
}

void ControlFlowGraphExpr::clear()
{
    BasicBlockList::iterator it = m_allBlocks.begin(),
                             itEnd = m_allBlocks.end();
    for(; it != itEnd; ++it)
        delete *it;
    m_allBlocks.clear();
}

void ControlFlowGraphExpr::addBasicBlock(BasicBlock *block)
{
    m_allBlocks.push_back(block);
}

ControlFlowGraphExpr::BasicBlock* ControlFlowGraphExpr::findBlockOfExpr(const ExpressionBase* expr)
{
	ExpressionBase& rootExpr = const_cast<ExpressionBase*>(expr)->obtainRootExpression();
	size_t numBlocks = m_allBlocks.size();

	for(size_t i = 0; i < numBlocks; ++i)
	{
		BasicBlock& block = *m_allBlocks[i];
		for(size_t j = 0; j < block.size(); ++j)
		{
			if (block[j] == &rootExpr)
				return &block;
		}
	}
	return 0;
}

ControlFlowGraphExprBuilder::ControlFlowGraphExprBuilder(bool mergeBlocks)
    :m_graph(0)
    ,m_entryBlock(0)
    ,m_exitBlock(0)
	,m_mergeBlocks(mergeBlocks)
{
}

void ControlFlowGraphExprBuilder::build(StatementBase& stmt, ControlFlowGraphExpr &graph)
{
    m_graph = &graph;
    m_graph->clear();

	buildBaseGraph(stmt);
	if (m_mergeBlocks)
		mergeBlocks();
    buildGraph();

    m_exprToBlock.clear();
}

void ControlFlowGraphExprBuilder::buildBaseGraph(StatementBase &firstStmt)
{
    m_entryBlock = new BasicBlock;
    m_exitBlock = new BasicBlock;
    m_currentBlock = m_entryBlock;
    m_current = CurrentPos(&firstStmt, true);

    while(m_current.stmt != 0)
    {
        m_current.stmt->accept(*this);

        if (m_current.stmt == 0 &&
            !m_altStmts.empty())
        {
            m_currentBlock = m_altStmts.back().first;
            m_current = m_altStmts.back().second;
            m_altStmts.pop_back();
        }
    }
}

void ControlFlowGraphExprBuilder::mergeBlocks()
{
    ExprToBlockMap::iterator it = m_exprToBlock.begin(),
                             itEnd = m_exprToBlock.end();
    for(; it != itEnd; ++it)
    {
        BasicBlock* block = it->second;

        while(block->getOutBlocks().size() == 1)
        {
            BasicBlock* nextBlock = block->getOutBlocks().front();
            if (nextBlock != m_exitBlock &&
                nextBlock->getInBlocks().size() == 1)
            {
                block->insert(block->end(), nextBlock->begin(), nextBlock->end());
                nextBlock->clear();
                block->getOutBlocks().swap(nextBlock->getOutBlocks());
                nextBlock->getOutBlocks().clear();

                for(size_t i = 0; i < block->getOutBlocks().size(); ++i)
                {
                    BasicBlock* nnBlock = block->getOutBlocks()[i];
                    ControlFlowGraphExpr::BasicBlockList::iterator jt =
                        std::find(nnBlock->getInBlocks().begin(), nnBlock->getInBlocks().end(), nextBlock);
                    *jt = block;
                }
            }
            else
                break;
        }
    }
}

void ControlFlowGraphExprBuilder::buildGraph()
{
    m_graph->addBasicBlock(m_entryBlock);

    ExprToBlockMap::iterator it = m_exprToBlock.begin(),
                             itEnd = m_exprToBlock.end();
    for(; it != itEnd; ++it)
    {
        BasicBlock* block = it->second;
        if (!block->empty())
            m_graph->addBasicBlock(block);
        else
            delete block;
    }

    m_graph->addBasicBlock(m_exitBlock);

    m_exprToBlock.clear();
}

void ControlFlowGraphExprBuilder::visit(ExpressionStatement& exprStmt)
{
    if (visitExpr(exprStmt.get()))
        m_current = nextInBlock(exprStmt);
    else
        m_current = CurrentPos();
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::BlockStatement& blockStmt)
{
    if (m_current.fromOutside == false || blockStmt.isEmpty())
    {
        RepriseBase* parent = blockStmt.getParent();
        if (StatementBase* parentStmt = parent->cast_ptr<StatementBase>())
        {
            if (parentStmt->is_a<BlockStatement>())
            {
                m_current = nextInBlock(blockStmt);
            }
            else if (ForStatement* parentFor = parentStmt->cast_ptr<ForStatement>())
            {
                m_current = CurrentPos(parentFor, false);
            }
            else if (IfStatement* parentIf = parentStmt->cast_ptr<IfStatement>())
            {
                m_current = nextInBlock(*parentIf);
            }
            else if (WhileStatement* parentWhile = parentStmt->cast_ptr<WhileStatement>())
            {
                m_current = CurrentPos(parentWhile, false);
            }
            else if (PlainSwitchStatement* parentSwitch = parentStmt->cast_ptr<PlainSwitchStatement>())
            {
                m_current = nextInBlock(*parentSwitch);
            }
        }
        else
        {
            visitExit();
            m_current = CurrentPos();
        }
    }
    else
    {
        m_current = CurrentPos(&*blockStmt.getFirst(), true);
    }
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::ForStatement& forStmt)
{
    if (m_current.fromOutside)
    {
        // пришли снаружи
        // посещаем инициализацию, потом условие
        if (!visitExpr(forStmt.getInitExpression()) ||
            !visitExpr(forStmt.getFinalExpression()))
        {
            m_current = CurrentPos();
            return;
        }
    }
    else
    {
        // пришли изнутри
        // посещаем инкремент
        if (!visitExpr(forStmt.getStepExpression()) ||
            !visitExpr(forStmt.getFinalExpression()))
        {
            m_current = CurrentPos();
            return;
        }
    }

    // ветвимся
    saveAlternative(nextInBlock(forStmt));
    m_current = CurrentPos(&forStmt.getBody(), true);
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::WhileStatement& whileStmt)
{
    if (whileStmt.isPreCondition())
    {
        // while do
        // посещаем условие
        if (!visitExpr(whileStmt.getCondition()))
        {
            m_current = CurrentPos();
            return;
        }
        // ветвимся
        saveAlternative(nextInBlock(whileStmt));
        m_current = CurrentPos(&whileStmt.getBody());
    }
    else
    {
        // do while
        if (m_current.fromOutside)
        {
            // идем в тело цикла
            m_current = CurrentPos(&whileStmt.getBody(), true);
        }
        else
        {
            // посещаем условие
            if (!visitExpr(whileStmt.getCondition()))
            {
                m_current = CurrentPos();
                return;
            }
            // ветвимся
            saveAlternative(nextInBlock(whileStmt));
            m_current = CurrentPos(&whileStmt.getBody());
        }
    }
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::IfStatement& ifStmt)
{
    if (visitExpr(ifStmt.getCondition()))
    {
        saveAlternative(CurrentPos(&ifStmt.getElseBody(), true));
        m_current = CurrentPos(&ifStmt.getThenBody(), true);
    }
    else
    {
        m_current = CurrentPos();
    }
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::PlainSwitchStatement& switchStmt)
{
    // посещаем условие
    if (!visitExpr(switchStmt.getCondition()))
    {
        m_current = CurrentPos();
        return;
    }

    // ветвимся
    const int caseCount = switchStmt.getLabelCount();
    StatementBase* defaultStmt = 0;
    for(int iCase = 0; iCase < caseCount; ++iCase)
    {
        PlainCaseLabel& label = switchStmt.getLabel(iCase);
        if (!label.isDefault())
            saveAlternative(CurrentPos(&label.getStatement(), true));
        else
        {
            OPS_ASSERT(defaultStmt == 0);
            defaultStmt = &label.getStatement();
        }
    }

    if (defaultStmt != 0)
        m_current = CurrentPos(defaultStmt, true);
    else
        m_current = nextInBlock(switchStmt);
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::Canto::HirBreakStatement&)
{
    OPS_ASSERT(0);
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::Canto::HirContinueStatement&)
{
    OPS_ASSERT(0);
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::GotoStatement& gotoStmt)
{
    m_current = CurrentPos(gotoStmt.getPointedStatement(), true);
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::ReturnStatement& returnStmt)
{
    // посещаем выражение
    if (visitExpr(returnStmt.getReturnExpression()))
        visitExit();
    m_current = CurrentPos();
}

void ControlFlowGraphExprBuilder::visit(OPS::Reprise::EmptyStatement& empty)
{
    m_current = nextInBlock(empty);
}

void ControlFlowGraphExprBuilder::saveAlternative(const CurrentPos &pos)
{
    m_altStmts.push_back(std::make_pair(m_currentBlock, pos));
}

bool ControlFlowGraphExprBuilder::visitExpr(ExpressionBase& expr)
{
    ExprToBlockMap::iterator it = m_exprToBlock.find(&expr);
    BasicBlock* targetBlock = 0;
    const bool targetExists = it != m_exprToBlock.end();
    if (targetExists)
    {
        targetBlock = it->second;
    }
    else
    {
        targetBlock = new BasicBlock;
        targetBlock->push_back(&expr);
        m_exprToBlock.insert(std::make_pair(&expr, targetBlock));
    }

    if (m_currentBlock != 0)
    {
        m_currentBlock->getOutBlocks().push_back(targetBlock);
        targetBlock->getInBlocks().push_back(m_currentBlock);
    }
    m_currentBlock = targetBlock;
    return !targetExists;
}

void ControlFlowGraphExprBuilder::visitExit()
{
    m_currentBlock->getOutBlocks().push_back(m_exitBlock);
    m_exitBlock->getInBlocks().push_back(m_currentBlock);
    m_currentBlock = m_exitBlock;
}

ControlFlowGraphExprBuilder::CurrentPos ControlFlowGraphExprBuilder::nextInBlock(StatementBase& stmt)
{
    BlockStatement& parentBlock = stmt.getParentBlock();
    BlockStatement::Iterator it = parentBlock.convertToIterator(&stmt);
    it++;
    if (it.isValid())
    {
        return CurrentPos(&*it, true);
    }
    else
    {
        return CurrentPos(&parentBlock, false);
    }
}
