#include "Analysis/SymbolPredicateFinder.h"
#include "Analysis/ControlFlowGraph.h"


using namespace OPS::Reprise;

typedef ControlFlowGraph::StatementList StatementList;
typedef ControlFlowGraph::StatementVector StatementVector;
typedef ControlFlowGraph::StatementGraph StatementGraph;

struct ReversedControlFlowGraph: public OPS::NonCopyableMix, OPS::NonAssignableMix
{
		ControlFlowGraph& cfg;
		ControlFlowGraph::StatementList* operator[](const StatementBase* pStmt)
		{
			ControlFlowGraph::StatementGraph::const_iterator it(m_graph.find(const_cast<StatementBase*>(pStmt)));
			OPS_ASSERT(it != m_graph.end());
			return it->second;
		}
		ReversedControlFlowGraph(ControlFlowGraph& p_cfg): cfg(p_cfg)
		{
			for(ControlFlowGraph::StatementVector::const_iterator node = cfg.getStatementVector().begin();
				node != cfg.getStatementVector().end();
				++node)
			{
				m_graph[*node] = new ControlFlowGraph::StatementList();
			}
			for(ControlFlowGraph::StatementGraph::const_iterator edge_list = cfg.getGraph().begin();
				edge_list != cfg.getGraph().end();
				++edge_list)
			{
				for(ControlFlowGraph::StatementList::const_iterator edge_begin = edge_list->second->begin();
					edge_begin != edge_list->second->end();
					++edge_begin)
				{
					if (*edge_begin)
					{
						if (m_graph.find(*edge_begin) == m_graph.end())
							m_graph[*edge_begin] = new ControlFlowGraph::StatementList;

						m_graph[*edge_begin]->push_back(edge_list->first);
					}
				}

			}

		}
		~ReversedControlFlowGraph()
		{
			for(ControlFlowGraph::StatementGraph::iterator edge_list = m_graph.begin();
				edge_list != m_graph.end();
				++edge_list)
			{
				delete edge_list->second;
			}
		}
private: 
	ControlFlowGraph::StatementGraph m_graph;
};



typedef std::list<SymbolPredicate*> PredicateList;

SymbolPredicate* composeFromList(PredicateList list)
{
	if(list.size() == 0) return 0;
	if(list.size() == 1) return *list.begin();	
	SymbolPredicate* first = *list.begin();
	PredicateList::iterator second = ++list.begin();
	for(; second!=list.end(); ++second)
	{
		first = SymbolPredicate::createOr(first, *second);
	}
	return first;
	
}


SymbolPredicate* constructPath(const StatementBase* child, const StatementBase* parent_statement)
{	
	OPS_ASSERT(parent_statement); OPS_ASSERT(child); 
	
	const BlockStatement* child_block = &child->getParentBlock();
	OPS_ASSERT(child_block);
	//if
	if(const IfStatement* parent_if = parent_statement->cast_ptr<const IfStatement>() )
	{	
		SymbolPredicate* condition = SymbolPredicate::createExpression(&parent_if->getCondition());
		if(child_block == &(parent_if->getThenBody()))
		{
			return condition;
		}
		else
		{
			return SymbolPredicate::createNot(condition);
		}				
	}
	//while/do while
	if(const WhileStatement* parent_while = parent_statement->cast_ptr<WhileStatement>() )
	{	
		SymbolPredicate* condition = SymbolPredicate::createExpression(&parent_while->getCondition());


		if(child_block == &(parent_while->getBody()))
		{
			return condition;
		}
		else
		{
			return SymbolPredicate::createNot(condition);
		}				
	}

	//for
	if(const ForStatement* parent_for = parent_statement->cast_ptr<ForStatement>() )
	{	

		SymbolPredicate* condition = SymbolPredicate::createExpression(&parent_for->getFinalExpression());


		if(child_block == &(parent_for->getBody()))
		{
			return condition;
		}
		else
		{
			return SymbolPredicate::createNot(condition);
		}				
	}

	return SymbolPredicate::createBool(true);

}

SymbolPredicate* constructPredicate(ReversedControlFlowGraph& rcfg, const StatementBase* begin, const StatementBase* current, ControlFlowGraph::StatementVector path, bool skip_first = true)
{	
	//если дошли до нужного оператора, заканчиваем рекурсию
	if (!skip_first && current == begin)
		return SymbolPredicate::createBool(true);

	//если дошли до начала блока, заканчиваем рекурсию
	if(current == &rcfg.cfg.rootBlock()) 
		return 0;

	//если попали в цикл, отматываем назад до ближайшего разветвления
	if(std::find(path.begin(), path.end(), current) != path.end())
		return 0;

	path.push_back(current);

	std::list<SymbolPredicate*> parent_preds;

	StatementList* parents = rcfg[ current ];
	for(StatementList::iterator current_parent = parents->begin();
		current_parent != parents->end();
		++current_parent)
	{
		SymbolPredicate* parent_predicate = constructPredicate(rcfg, begin, *current_parent, path, false);
		if(parent_predicate)
		{
			SymbolPredicate* path_predicate = constructPath(current, *current_parent);
			SymbolPredicate* parent_n_path;
			
			//если предикат перехода тождественно истинный, мы его автоматически упрощаем
			if(path_predicate->getAsBool() && (path_predicate->getAsBool()->getValue() == true)) 
			{
				delete path_predicate;
				parent_n_path = parent_predicate;				
			}
			else if (parent_predicate->getAsBool() && (parent_predicate->getAsBool()->getValue() == true))
			{
				delete parent_predicate;
				parent_n_path = path_predicate;			
			}
			else
			{
				parent_n_path = SymbolPredicate::createAnd(parent_predicate, path_predicate);
			}
				
			parent_preds.push_back(parent_n_path);
		}
	}

	return composeFromList(parent_preds);

}


SymbolPredicate* OPS::getPathPredicates(const OPS::Reprise::StatementBase& end, const OPS::Reprise::BlockStatement& start_block)
{
	return getPathPredicates(start_block, end, start_block);
}

SymbolPredicate* OPS::getPathPredicates(const OPS::Reprise::StatementBase& begin, const OPS::Reprise::StatementBase& end, const OPS::Reprise::BlockStatement& enclosing_block)
{	
	if (&end == &enclosing_block) 
	{
		throw OPS::ArgumentError("end statement must not be enclosing block");
	}

	ControlFlowGraph cfg(enclosing_block);
	ReversedControlFlowGraph rcfg(cfg);
	SymbolPredicate* result = constructPredicate(rcfg, &begin, &end, StatementVector());
	//если пути нет
	if (!result) 
		result = SymbolPredicate::createBool(false);
	return result;

}




SymbolPredicateExpression* OPS::SymbolPredicate::createExpression(const ExpressionBase* expr)
{
	return new SymbolPredicateExpression(expr);
}
SymbolPredicateLogic* OPS::SymbolPredicate::createAnd(SymbolPredicate* first, SymbolPredicate* second)
{
	return new SymbolPredicateLogic(SymbolPredicateLogic::PLT_AND, first, second);
}
SymbolPredicateLogic* OPS::SymbolPredicate::createOr(SymbolPredicate* first, SymbolPredicate* second)
{
	return new SymbolPredicateLogic(SymbolPredicateLogic::PLT_OR, first, second);
}
SymbolPredicateLogic* OPS::SymbolPredicate::createNot(SymbolPredicate* first)
{
	return new SymbolPredicateLogic(SymbolPredicateLogic::PLT_NOT, first);
}
SymbolPredicateBool* OPS::SymbolPredicate::createBool(bool m_value)
{
	return new SymbolPredicateBool(m_value);
}

ExpressionBase* SymbolPredicateLogic::outToExpression() const
{
	switch(m_plt)
	{
	case PLT_AND:
		return new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_AND, m_first->outToExpression(), m_second->outToExpression());
	case PLT_OR:
		return new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_OR, m_first->outToExpression(), m_second->outToExpression());
	case PLT_NOT:
		return new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT, m_first->outToExpression());
	OPS_DEFAULT_CASE_LABEL;
	}
	return 0;
}

ExpressionBase* SymbolPredicateBool::outToExpression() const
{
	return StrictLiteralExpression::createBoolean(m_value);
}
