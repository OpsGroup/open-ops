#include "Analysis/ControlFlowGraphEx.h"
#include "Shared/StatementsShared.h"

using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Analysis::ControlFlowGraphs;
using namespace OPS::Analysis::ControlFlowGraphs::SSAControlFlowGraph;




OPS::Reprise::BlockStatement& OPS::Analysis::ControlFlowGraphs::Stash::getParentBlock()
{
	if(pStatement) return pStatement->getParentBlock(); 
	else if(pExpression) 
	{
		if(pExpression->obtainParentStatement())
		{
			return pExpression->obtainParentStatement()->getParentBlock();
		}
		else
		{	if (pExpression->obtainParentDeclaration()->hasDefinedBlock()) return pExpression->obtainParentDeclaration()->getDefinedBlock();
			else throw OPS::Exception("В stash выражение без parentblock");
		}
	}
	else if(pDeclaration)
	{
			if(pDeclaration->hasDefinedBlock()) 
			{
				return pDeclaration->getDefinedBlock();
			}
			else 
			{
				throw OPS::Exception("В stash variabledeclaration без parentblock");
			}
	}
	else
	{
		throw OPS::Exception("Пустой stash без parentblock");
	}	
}

OPS::Analysis::ControlFlowGraphs::Stash::Stash() : pStatement(0), pExpression(0), pDeclaration(0)
{

}

OPS::Analysis::ControlFlowGraphs::Stash::Stash( OPS::Reprise::StatementBase* statement ) : pStatement(statement), pExpression(0),pDeclaration(0)
{

}

OPS::Analysis::ControlFlowGraphs::Stash::Stash( const Stash& lhs ) : pStatement(lhs.pStatement), pExpression(lhs.pExpression),pDeclaration(lhs.pDeclaration)
{

}

OPS::Analysis::ControlFlowGraphs::Stash::Stash( OPS::Reprise::ExpressionBase* expression ) : pStatement(0), pExpression(expression),pDeclaration(0)
{

}

OPS::Analysis::ControlFlowGraphs::Stash::Stash( OPS::Reprise::VariableDeclaration* declaration ) : pStatement(0), pExpression(0),pDeclaration(declaration)
{

}
bool OPS::Analysis::ControlFlowGraphs::Stash::isStatement() const
{
	return (pStatement != 0);
}

bool OPS::Analysis::ControlFlowGraphs::Stash::isExpression() const
{
	return (pExpression != 0);
}

bool OPS::Analysis::ControlFlowGraphs::Stash::isDeclaration() const
{
	return (pDeclaration != 0);
}

OPS::Reprise::StatementBase* OPS::Analysis::ControlFlowGraphs::Stash::getAsStatement() const
{
	return pStatement;
}

OPS::Reprise::ExpressionBase* OPS::Analysis::ControlFlowGraphs::Stash::getAsExpression() const
{
	return pExpression;
}


OPS::Reprise::VariableDeclaration* OPS::Analysis::ControlFlowGraphs::Stash::getAsDeclaration() const
{
	return pDeclaration;
}

void OPS::Analysis::ControlFlowGraphs::Stash::setStatement( OPS::Reprise::StatementBase* pS )
{
	pStatement = pS;
	pExpression = 0;
	pDeclaration = 0;
}

void OPS::Analysis::ControlFlowGraphs::Stash::setExpression( OPS::Reprise::ExpressionBase* pE )
{
	pExpression = pE;
	pStatement = 0;
	pDeclaration = 0;
}


void OPS::Analysis::ControlFlowGraphs::Stash::setDeclaration( OPS::Reprise::VariableDeclaration* pD )
{
	pExpression = 0;
	pStatement = 0;
	pDeclaration = pD;
}

bool OPS::Analysis::ControlFlowGraphs::Stash::invariant() const
{
	return ((pStatement == 0) || (pExpression == 0) || (pDeclaration == 0));
}


bool OPS::Analysis::ControlFlowGraphs::Stash::operator==( const Stash& lhs ) const
{	
	if (isStatement())
	{
		return lhs.isStatement() && (pStatement == lhs.pStatement);
	}
	else if(isExpression())
	{
		return lhs.isExpression() && (pExpression == lhs.pExpression);
	}
	else if (isDeclaration())
	{
		return lhs.isDeclaration() && (pDeclaration == lhs.pDeclaration);
	}
	else
	{
		//пустой стэш
		return lhs.isEmpty();
	}

}

bool OPS::Analysis::ControlFlowGraphs::Stash::isEmpty() const
{
	return (pDeclaration == 0) && (pExpression == 0) && (pStatement == 0);
}


OPS::Reprise::BlockStatement& OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::rootBlock()
{
	return *m_block;
}

SSAControlFlowGraph::StatementVectorList& OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getSubblocks()
{
	return m_blocks;
}

SSAControlFlowGraph::StatementGraph& OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getSucc()
{
	return m_blocksSucc;
}

SSAControlFlowGraph::StatementGraph& OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getPred()
{
	return m_blocksPred;
}

SSAControlFlowGraph::CFGraph& OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getCFGraph()
{
	return m_cfGraph;
}

SSAControlFlowGraph::StatementVector* OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getBlockByNum( unsigned int num )
{
	if(num < m_blocks.size() && num >= 0)
	{
		StatementVectorList::iterator block = m_blocks.begin();
		for( ;num > 0; num--)	++block;
		return *block;
	}
	else
	{
		throw OPS::Exception("CFGEx.getBlockByNum failed - num>number of blocks");
	}
}

OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::ControlFlowGraphEx( OPS::Reprise::BlockStatement &aBlock )
{
	try
	{
		m_block = &aBlock;
		buildBlocks();
		buildCFGraph();	
		buildReverseCFGraph();

	}
	catch(...)
	{
		clear(); throw;
	}
}

OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::~ControlFlowGraphEx()
{
	clear();
}

void OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::clear()
{
	for(StatementVectorList::iterator i = m_blocks.begin(); i != m_blocks.end(); ++i)
	{
		delete *i;
	}
}

VariableDeclaration* OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::findFirstDeclarationInBlock( BlockStatement* blk)
{	
	BlockStatement* defblock = blk;
	OPS_ASSERT(defblock);
	for (Declarations::VarIterator iter = defblock->getDeclarations().getFirstVar(); iter.isValid(); ++iter)
	{
		if (iter->hasDefinedBlock() && (defblock == &iter->getDefinedBlock()) )
		{
			return &*iter; //возвращаем первое объявление в блоке
		}
	}	
	return 0;
}

//вернуть первый оператор блока, либо, если блок пустой, то первый оператор после блока
OPS::Analysis::ControlFlowGraphs::Stash OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::intoBlock( BlockStatement* bs )
{
	VariableDeclaration* tmp = findFirstDeclarationInBlock(bs);
	if(tmp)
	{
		return Stash(tmp);
	}
	if(!bs->isEmpty())
	{
		if( bs->getFirst()->is_a<BlockStatement>() )
		{
			return intoBlock(&bs->getFirst()->cast_to<BlockStatement>());
		}
		else 
		{
			return skip(Stash(&*bs->getFirst()));
		}

	}
	else
	{
		return skip(Stash(bs));
	}
}

void OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::buildBlocks()
{
	Stash runner = intoBlock(m_block); //первая декларация/объявление блока
	buildBlock(runner);
}

SSAControlFlowGraph::StatementVector* OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::buildBlock( Stash runner )
{
	if(runner.isEmpty()) return 0; //пустой блок, т.е. выход из рассматриваемого участка
	bool pushed = false;

	StatementVector* running_block = splitIfNeeded(runner);

	if(running_block)
	{
		return running_block;
	}
	else
	{
		running_block = new StatementVector();
	}

	bool first_runner_set = false;
	Stash first_runner;

	while(!runner.isEmpty())
	{
		if( ! (runner.isStatement() && runner.getAsStatement()->is_a<BlockStatement>())) //блок закончился
		{
			running_block->push_back(runner);
			if(!first_runner_set)
			{
				first_runner = runner;
				first_runner_set = true;
			}
		}
		if(isControlStash(runner))
		{
			//костыль. похорошему надо пустые блоки удалять и обновлять графы			
			StatementVector tmp_branches = branches(runner);
			if( (running_block->size() == 0) && (tmp_branches.size() == 1) )
			{
				StatementVector::iterator i = tmp_branches.begin();
				return buildBlock(*i);
			}

			m_blocks.push_back(running_block);
			pushed = true;

			m_blocksSucc[running_block] = new StatementVectorList();
			for(StatementVector::iterator i = tmp_branches.begin(); i!= tmp_branches.end(); ++i)
			{
				m_blocksSucc[running_block]->push_back(buildBlock(*i));
				;
			}
			break;
		}
		else
		{
			runner = next(runner);			
		}
	}		
	if(!pushed)
	{
		m_blocks.push_back(running_block);
		m_blocksSucc[running_block] = new StatementVectorList();
	}
	//костыль	
	if(first_runner_set)
		running_block = *findMatch(first_runner);

	return running_block;
}



bool OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::isControlStash( Stash runner )
{
	if(runner.isEmpty()) return false;
	if(runner.isDeclaration()) return false;
	if(runner.isExpression() ) 
	{
		OPS_ASSERT(runner.getAsExpression()->getParent());
		if(runner.getAsExpression()->getParent()->is_a<Reprise::ForStatement>()) 
		{
			ForStatement& fstmt = runner.getAsExpression()->getParent()->cast_to<Reprise::ForStatement>();
			if( &fstmt.getInitExpression() == runner.getAsExpression() )
				return false;			
			else
				return true;
		}
		else
		{
			return true;	
		}
	}
	if(runner.isStatement())
	{
		if(  (runner.getAsStatement()->is_a<Reprise::ReturnStatement>()) ||
			(runner.getAsStatement()->is_a<Reprise::GotoStatement>()) ||
            (runner.getAsStatement()->is_a<Reprise::BlockStatement>()) )
		{
			return true;
		}
		return false;

	}
	return true;
}

//возвращает следующий элемент в потоке управления
OPS::Analysis::ControlFlowGraphs::Stash OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::next( Stash runner )
{
	if(runner.isEmpty()) return Stash();
	if(runner.isDeclaration())
	{
		//возвращаем следующее объявление или первый элемент блока.
		VariableDeclaration* runnerdec = runner.getAsDeclaration();
		BlockStatement* defblock = 0;
		if(runnerdec->hasDefinedBlock()) defblock = &runnerdec->getDefinedBlock();
		bool found_one = false;
		OPS_ASSERT(defblock);
		for (Declarations::VarIterator iter = defblock->getDeclarations().getFirstVar(); iter.isValid(); ++iter)
		{
			if (found_one)
			{
				if(iter->hasDefinedBlock() && (defblock == &iter->getDefinedBlock()) )
				{
					return skip(Stash(&*iter)); //возвращаем следующее объявление
				}
			}

			if (iter == defblock->getDeclarations().getLastVar()) //последнее объявление, возвращаем первый элемент блока
			{
				if(!defblock->isEmpty())
					return skip(Stash(&*defblock->getFirst()));
				else return skip(Stash(defblock));
			}


			if (&*iter == runnerdec)
			{
				found_one = true;					
			}
		}
		OPS_ASSERT(0);
	}
	if(runner.isExpression() ) 
	{
		OPS_ASSERT(runner.getAsExpression()->getParent());
		if(runner.getAsExpression()->getParent()->is_a<Reprise::ForStatement>()) 
		{
			ForStatement& fstmt = runner.getAsExpression()->getParent()->cast_to<Reprise::ForStatement>();
			if( &fstmt.getFinalExpression() == runner.getAsExpression() )
			{
				OPS_ASSERT(0);
			}
			else if( &fstmt.getInitExpression() == runner.getAsExpression() ) 
			{
				return Stash(&fstmt.getFinalExpression());
			}
			else if( &fstmt.getStepExpression() == runner.getAsExpression() ) 
			{
				return Stash(&fstmt.getFinalExpression());
			}
		}
		else
		{
			//if, while, case
			OPS_ASSERT(0);	
		}
	}
	if(runner.isStatement())
	{
		if(  (runner.getAsStatement()->is_a<Reprise::ReturnStatement>()) ||
            (runner.getAsStatement()->is_a<Reprise::GotoStatement>()))
		{
			OPS_ASSERT(0);
		}
		//TODO: SWITCH
		else
		{
			//текущий оператор - последний
			if(runner.getAsStatement() == &*runner.getParentBlock().getLast()) return Stash(&runner.getParentBlock());
			//текущий оператор - не последний, нужен следующий
			BlockStatement::Iterator i = runner.getParentBlock().convertToIterator(runner.getAsStatement());
			if(i.isValid())
			{
				++i;
				if(i->is_a<BlockStatement>())
				{
					return intoBlock(i->cast_ptr<BlockStatement>());
				}
				else
				{
					return skip(Stash(&*i));
				}
			}

			OPS_ASSERT(0);
		}

	}
	throw OPS::StateError("Unexpected ControlFlowGraphEx state");
	//	return Stash();	
}

//пропускает незначимые элементы потока управления (пустые операторы, закрывающие скобки etc
OPS::Analysis::ControlFlowGraphs::Stash OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::skip( Stash runner )
{
	if(runner.isEmpty()) return runner;
	if(runner.isDeclaration())
	{
		return runner;
	}
	if(runner.isExpression() ) 
	{
		return runner;
	}
	if(runner.isStatement())
	{
		if (runner.getAsStatement()->is_a<Reprise::BlockStatement>()) 
		{
			StatementBase* parent;
			if( dynamic_cast<StatementBase*> (runner.getAsStatement()->getParent()) )
			{
				parent = &runner.getAsStatement()->getParent()->cast_to<StatementBase>();
			}
			else
			{
				return Stash();
			}
			if(ext(Stash(parent))) return Stash();

			if(parent->is_a<ForStatement>())
			{
				ForStatement* parentd= &parent->cast_to<ForStatement>();
				return Stash(&parentd->getStepExpression());
			}
			else if (parent->is_a<WhileStatement>())
			{				
				WhileStatement* parentd = &parent->cast_to<WhileStatement>();
				//while
				if(parentd->isPreCondition())
				{
					return Stash(&parentd->getCondition());			
				}
				//do while
				else
				{
					return intoBlock(&parentd->getBody());
				}
			}
			else if (parent->is_a<IfStatement>())
			{
				return next(Stash(parent));
			}
			else if(parent->is_a<BlockStatement>())
			{
				return next(runner.getAsStatement());
			}
			else
			{
				OPS_ASSERT(0);
			}

		}//blockstatement
		if (runner.getAsStatement()->is_a<Reprise::EmptyStatement>()) 
		{
			return skip(next(runner));
		}
		if (runner.getAsStatement()->is_a<Reprise::IfStatement>()) 
		{
			IfStatement* runnerd= &runner.getAsStatement()->cast_to<IfStatement>();
			return skip(Stash(&runnerd->getCondition()));
		}
		if (runner.getAsStatement()->is_a<Reprise::ForStatement>()) 
		{
			ForStatement* runnerd= &runner.getAsStatement()->cast_to<ForStatement>();
			return skip(Stash(&runnerd->getInitExpression()));			
		}
		if (runner.getAsStatement()->is_a<Reprise::WhileStatement>()) 
		{
			WhileStatement* runnerd= &runner.getAsStatement()->cast_to<WhileStatement>();
			if(runnerd->isPreCondition())
			{
				return skip(Stash(&runnerd->getCondition()));
			}
			else
			{
				return skip(intoBlock(&runnerd->getBody()));
			}
		}

		else return runner;

	}
	throw OPS::StateError("Unexpected ControlFlowGraphEx state");
	//	return Stash();		
}

bool OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::ext( Stash currStash )
{
	if(currStash.getAsStatement() == m_block)
		return false;
	return !Shared::contain(m_block, &currStash.getParentBlock());
}

void OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::rewriteLinks(StatementVector* what, StatementVector* withWhat)
{
	for(StatementGraph::iterator pred = m_blocksSucc.begin();
		pred != m_blocksSucc.end();
		++pred )
	{
		for( StatementVectorList::iterator succ = pred->second->begin();
			 succ != pred->second->end();
			 ++succ)			 
		{
			if(*succ == what) 
				*succ = withWhat;
		}
	}
}

SSAControlFlowGraph::StatementVector* OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::splitIfNeeded( Stash sMatch )
{
	StatementVectorList::iterator to_split = findMatch(sMatch);
	if(to_split == m_blocks.end())
		return 0;
	else
	{
		if( *((*to_split)->begin()) == sMatch)
		{
			return *to_split;
		}
		else
		{
			StatementVector::iterator split_place = std::find( (*to_split)->begin(), (*to_split)->end(), sMatch);

			StatementVector* new_vector = new StatementVector((*to_split)->begin(), split_place );
			(*to_split)->erase((*to_split)->begin(), split_place);

			rewriteLinks(*to_split, new_vector);			
			
			m_blocksSucc[new_vector] = new StatementVectorList(); 			
			m_blocksSucc[new_vector]->push_back(*to_split);			

			m_blocks.push_back(*to_split);
			StatementVector* to_return = *to_split;
			*to_split = new_vector;			

			return to_return;

		}
	}
}

SSAControlFlowGraph::StatementVectorList::iterator OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::findMatch( Stash sMatch )
{
	StatementVectorList::iterator i;
	for(i = m_blocks.begin(); i != m_blocks.end(); ++i)
	{
		if(std::find((*i)->begin(), (*i)->end(), sMatch) != (*i)->end()) return i;
	}
	return i;
}

SSAControlFlowGraph::StatementVector OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::branches( Stash runner )
{
	StatementVector to_return;
	if(runner.isEmpty()) return StatementVector();
	if(runner.isDeclaration()) return StatementVector(1, next(runner));
	if(runner.isExpression() ) 
	{
		OPS_ASSERT(runner.getAsExpression()->getParent());
		if(runner.getAsExpression()->getParent()->is_a<Reprise::ForStatement>()) 
		{
			ForStatement& fstmt = runner.getAsExpression()->getParent()->cast_to<Reprise::ForStatement>();
			if( &fstmt.getFinalExpression() == runner.getAsExpression() )
			{
				to_return.push_back(intoBlock(&fstmt.getBody()));
				to_return.push_back(next(Stash(&fstmt)));				

			}
			else if( &fstmt.getStepExpression() == runner.getAsExpression() )
			{
				to_return.push_back(Stash(&fstmt.getFinalExpression()));
			}
		}
		else if(runner.getAsExpression()->getParent()->is_a<Reprise::WhileStatement>()) 
		{
			WhileStatement& wstmt = runner.getAsExpression()->getParent()->cast_to<Reprise::WhileStatement>();
			to_return.push_back(intoBlock(&wstmt.getBody()));
			to_return.push_back(next(Stash(&wstmt)));			
		}
		else if(runner.getAsExpression()->getParent()->is_a<Reprise::IfStatement>()) 
		{
			IfStatement& ifstmt = runner.getAsExpression()->getParent()->cast_to<Reprise::IfStatement>();
			to_return.push_back(intoBlock(&ifstmt.getThenBody()));
			to_return.push_back(intoBlock(&ifstmt.getElseBody()));
		}		
		else
		{
			OPS_ASSERT(0);
		}
	}
	if(runner.isStatement())
	{
		if(runner.getAsStatement()->is_a<Reprise::ReturnStatement>())
		{
			return StatementVector();
		}
		else if(runner.getAsStatement()->is_a<BlockStatement>())
		{
			to_return.push_back( skip (runner.getAsStatement()) );
		}
		else if (runner.getAsStatement()->is_a<Reprise::GotoStatement>())
		{
			GotoStatement* gts = runner.getAsStatement()->cast_ptr<GotoStatement>();
			OPS_ASSERT(gts->getPointedStatement());
			to_return.push_back(skip(gts->getPointedStatement()));
		}
		else 
		{
			OPS_ASSERT(0);
		}
	}
	return to_return;
}

void OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::buildCFGraph()
{
	for(StatementVectorList::iterator i = m_blocks.begin(); i != m_blocks.end(); ++i)
	{
		for(StatementVectorList::iterator j = m_blocks.begin(); j != m_blocks.end(); ++j)
		{
			if(std::find(m_blocksSucc[*i]->begin(), m_blocksSucc[*i]->end(), *j)!= m_blocksSucc[*i]->end())
				m_cfGraph[*i][*j] = 1;
			else
				m_cfGraph[*i][*j] = 0;

		}
	}	
}

void OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::buildReverseCFGraph()
{
	for(StatementVectorList::iterator i = m_blocks.begin(); i != m_blocks.end(); ++i)
	{
		m_blocksPred[*i] = new StatementVectorList();
		for(StatementVectorList::iterator j = m_blocks.begin(); j != m_blocks.end(); ++j)
		{
			if(m_cfGraph[*j][*i])
				m_blocksPred[*i]->push_back(*j);
		}
	}	
}

SSAControlFlowGraph::StatementVector* OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getBlock( Stash element ) /*получить блок по элементу */
{
	for(StatementVectorList::iterator block = m_blocks.begin();
		block != m_blocks.end();
		++block)
	{
		for(StatementVector::iterator oper = (*block)->begin();
			oper != (*block)->end();
			++oper)
		{
			if(*oper == element) return *block;
		}
	}
	return 0;
}

int OPS::Analysis::ControlFlowGraphs::ControlFlowGraphEx::getBlockNum( SSAControlFlowGraph::StatementVector* blk )
{
	if(blk == 0) return 0;
	int i = 1;
	for(StatementVectorList::iterator current_block = m_blocks.begin();
		current_block != m_blocks.end();
		++current_block)
	{
		if(blk == *current_block) return i;
		i++;
	}
	return -1;
}
