#include "Shared/StatementsShared.h"
#include "Shared/NodesCollector.h"
#include "StatementsSharedDeepWalkers.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace OPS::Reprise;
using namespace std;

namespace OPS
{
namespace Shared
{
	bool contain(const StatementBase* const outsideStmt, const StatementBase* const insideStmt)
	{
		OPS_ASSERT( outsideStmt != NULL && insideStmt != NULL );

		if (outsideStmt == insideStmt)
			return true;
		RepriseBase* parent = insideStmt->getParent();
		while (! parent->is_a<SubroutineDeclaration>())
		{
			if (parent == outsideStmt)
				return true; 
			parent = parent->getParent();
		}
		return false;
	}

	bool swapStmts (Reprise::StatementBase* /*const*/ stmt1, Reprise::StatementBase* /*const*/ stmt2)
	{
		OPS_ASSERT( stmt1 != NULL && stmt2 != NULL );

		if (stmt1 == stmt2)
			return true; // считаем, что переставили оператор сам с собой

		// первые две проверки нельзя менять местами, так как вторая включает в себя первую и будет выдан неадекватный результат
		if ( contain(stmt1, stmt2) || contain(stmt2, stmt1) )
			return false;

        ReprisePtr<StatementBase> tmpStmt(new EmptyStatement);
        ReprisePtr<StatementBase> stmt1Ptr = OPS::Reprise::Editing::replaceStatement(*stmt1, tmpStmt);
        ReprisePtr<StatementBase> stmt2Ptr = OPS::Reprise::Editing::replaceStatement(*stmt2, stmt1Ptr);
        OPS::Reprise::Editing::replaceStatement(*tmpStmt, stmt2Ptr);
		return true;
	}

	
	set<StatementBase*> getChildStatements(StatementBase* pStatement, bool includeSourceStatement)
	{
		OPS_ASSERT(pStatement != NULL);

		CollectChildStatemetsDeepWalker deepWalker;

		pStatement->accept(deepWalker);
		CollectChildStatemetsDeepWalker::StatementsContainer childStatements = deepWalker.getChildStatements();

		if(includeSourceStatement)
		{
			childStatements.insert(pStatement);
		}

		return childStatements;
	}

    std::list<RepriseBase*> getAllParentsOf(RepriseBase& node)
    {
        std::list<RepriseBase*> result;
        RepriseBase* parent = node.getParent();
        while (parent != 0)
        {
            result.push_back(parent);
            parent = parent->getParent();
        }
        return result;
    }

    OPS::Reprise::RepriseBase* getFirstCommonParent(OPS::Reprise::RepriseBase& node1, OPS::Reprise::RepriseBase& node2)
    {
		RepriseBase* p1 = 0, *p2 = 0;
		return getFirstCommonParentEx(node1, node2, p1, p2);
    }

	OPS::Reprise::RepriseBase* getFirstCommonParentEx(Reprise::RepriseBase &node1, Reprise::RepriseBase &node2, Reprise::RepriseBase *&node1Parent, Reprise::RepriseBase *&node2Parent)
	{
		if (&node1 == &node2)
			return node1.getParent();

        if (node1.getParent() == node2.getParent())
        {
            node1Parent = &node1;
            node2Parent = &node2;
            return node1.getParent();
        }

		std::list<RepriseBase*> node1Parents = getAllParentsOf(node1);
		std::list<RepriseBase*> node2Parents = getAllParentsOf(node2);

		// Список предков у нас инвертирован, поэтому используются реверсные итераторы
		std::list<RepriseBase*>::const_reverse_iterator it1 = node1Parents.rbegin();
		std::list<RepriseBase*>::const_reverse_iterator it2 = node2Parents.rbegin();

		for (; it1 != node1Parents.rend() && it2 != node2Parents.rend(); ++it1, ++it2)
		{
			// Находим первые отличающиеся узлы
			if (*it1 != *it2)
			{
				// Если это не самое начало списка
				if (it1 != node1Parents.rbegin())
				{
					node1Parent = *it1;
					node2Parent = *it2;
					// возвращаем предыдущий элемент
					--it1;
					return *it1;
				}
				else
					return 0; // У узлов вообще нет общих предков
			}
		}
        //сюда дошли, если все родители одинаковы, но число родителей разное
        OPS_ASSERT (it1 != node1Parents.rend() || it2 != node2Parents.rend());

        if (it1 == node1Parents.rend())
		{
            if (&node1 == *it2) //node1 is parent of node2
            {
                node1Parent = 0;
                node2Parent = *(--it2);
                return &node1;
            }
            else
            {
                node1Parent = &node1;
                node2Parent = *it2;
                --it1;
                return *it1;
            }
        }
		else
		{
            if (&node2 == *it1) //node2 is parent of node1
            {
                node1Parent = *(--it1);
                node2Parent = 0;
                return &node2;
            }
            else
            {
                node2Parent = &node2;
                node1Parent = *it1;
                --it2;
                return *it2;
            }
        }
	}

	bool contain(const RepriseBase& outside, const RepriseBase& inside)
	{
		if (&outside == &inside)
			return true;

		RepriseBase* parent = inside.getParent();
		while (parent != 0)
		{
			if (parent == &outside)
				return true;
			parent = parent->getParent();
		}
		return false;
	}

    BlockStatement* getSurroundingBlock(StatementBase* currentStatement)
    {
        RepriseBase* surroundingStatement = currentStatement->getParent();
        while(!(surroundingStatement == NULL || surroundingStatement->is_a<BlockStatement>()))
        {
            surroundingStatement = surroundingStatement->getParent();
        }
        if(surroundingStatement != NULL)
        {
            return &(surroundingStatement->cast_to<BlockStatement>());
        } else return NULL;
    }


	namespace {
		typedef std::vector<BlockStatement*> BlockList;
		typedef std::vector<int> Blocks;
		typedef std::pair<BlockStatement*, Blocks> Node;
		typedef std::vector<Node> BlockGraph;

		struct NodeSearcher 
		{
			BlockStatement* pKey;
		public:
			NodeSearcher(BlockStatement* key): pKey(key) {}
			bool operator()(const Node& n)
			{
				return n.first == pKey;
			}
		};
		
		bool hasSurroundingBlock(StatementBase* currentStatement)
		{
			RepriseBase* surroundingStatement = currentStatement->getParent();
			while(!(surroundingStatement == NULL || surroundingStatement->is_a<BlockStatement>()))
			{
				surroundingStatement = surroundingStatement->getParent();
			}
			if(surroundingStatement != NULL)
			{
				return true;
			} else return false;
		}

		StatementBase* getNextOperator(StatementBase* stmt)
		{
			StatementBase* result = NULL;
            if(stmt->is_a<Canto::HirBreakStatement>() || stmt->is_a<Canto::HirContinueStatement>())
			{
				// найдем первый вышележащий оператор цикла или switch
				StatementBase* currentStatement = stmt;
				while(!(currentStatement->is_a<WhileStatement>() || 
					currentStatement->is_a<ForStatement>() ||
                    currentStatement->is_a<PlainSwitchStatement>()))
				{
					OPS_ASSERT(currentStatement->getParent()->is_a<StatementBase>());
					currentStatement = &(currentStatement->getParent()->cast_to<StatementBase>());
					if(currentStatement == NULL)
					{
						throw RepriseError("Unexpected parent block for statement");
					}
				}
				result = currentStatement;
			}
			if(stmt->is_a<GotoStatement>())
			{
				// вернем блок, содержащий оператор,
				// на который осуществляется переход
				GotoStatement* gotoStmt = &(stmt->cast_to<GotoStatement>());
				StatementBase* targetStmt = gotoStmt->getPointedStatement();
				result = targetStmt;
			}
			if(stmt->is_a<ReturnStatement>())
			{
				// вернем самый вышележащий блок
				result = &(stmt->getRootBlock());
			}
			OPS_ASSERT(result != NULL);
			return result;
		}

		void inverseEdges(BlockGraph& straight, BlockGraph& reverse)
		{
			size_t nCurrentNode = 0;
			for(; nCurrentNode != straight.size(); nCurrentNode++)
			{
				reverse.push_back(Node(straight[nCurrentNode].first, Blocks()));
				for(size_t nNode = 0; nNode != straight.size(); nNode++)
				{
					Blocks& blocks = straight[nNode].second;
					if(std::find(blocks.begin(), blocks.end(), nCurrentNode) != blocks.end())
					{
						reverse.back().second.push_back(nNode);
					}
				}
			}
		}

		void DFS(BlockGraph& blocks, std::vector<bool>& marks, int nStart = 0)
		{
			if(marks[nStart]) 
			{
				return; 
			}
			else 
			{
				marks[nStart] = true;
				Blocks& currentEdges = blocks[nStart].second;
				for(size_t nEdge = 0; nEdge != currentEdges.size(); ++nEdge)
				{
					DFS(blocks, marks, currentEdges[nEdge]);
				}
			}
		}

		// ищет в графе блок, в котором находится данный оператор
		int findGraphNearestStatementBlock(StatementBase* stmt, BlockGraph* blocks)
		{
			std::list<BlockStatement*> outerBlocks;
			StatementBase* statement = stmt;
			BlockStatement* currentBlock = NULL;
			if(!statement->is_a<BlockStatement>())
			{
				currentBlock = &(stmt->getParentBlock());
			} else {
				currentBlock = &(statement->cast_to<BlockStatement>());
			}
			
			outerBlocks.push_back(currentBlock);

			while(hasSurroundingBlock(currentBlock))
			{
	            currentBlock = getSurroundingBlock(currentBlock);
				outerBlocks.push_back(currentBlock);
			}
            for(int nCurrent = 0; nCurrent < (int)blocks->size(); nCurrent++)
			{
				BlockStatement* blockToFind = (*blocks)[nCurrent].first;
				if(std::find(outerBlocks.begin(), outerBlocks.end(), blockToFind) != outerBlocks.end())
				{
					return nCurrent;
				}
			}
			return blocks->size();
		}
	}

	BlockStatement* getOuterSimpleBlock(BlockStatement* block)
	{
		BlockGraph outerBlocks;
		BlockStatement* currentBlock = block;
		outerBlocks.push_back(Node(currentBlock, Blocks()));
		// соберем все охватывающие блоки для заданного (вплоть до тела подпрограммы)
		// добавим их в граф
		while(hasSurroundingBlock(currentBlock))
		{
            currentBlock = getSurroundingBlock(currentBlock);
			outerBlocks.push_back(Node(currentBlock, Blocks()));
		}
		// теперь блоки отсортированы по порядку вложенности (от внутреннего к внешним)
		
		// соберем все операторы goto внутри наружного блока, который могут 
		// вызывать "неожиданные" передачи управления внутрь указанного
		BlockStatement* upperBlock = outerBlocks.rbegin()->first;
		
		typedef Shared::NodesCollector< Reprise::GotoStatement > GotoCollector;
		GotoCollector gotoCollector;
		upperBlock->accept(gotoCollector);
		GotoCollector::CollectionType& gotos = gotoCollector.getCollection();
		
		// каждый такой оператор означает дугу на графе блоков
		GotoCollector::CollectionType::iterator pGoto = gotos.begin();
		for(; pGoto != gotos.end(); ++pGoto)
		{
			GotoStatement* gotoStmt = *pGoto;
			size_t nSource = findGraphNearestStatementBlock(gotoStmt, &outerBlocks);
			StatementBase* pointedStmt = gotoStmt->getPointedStatement();
			size_t nTarget = findGraphNearestStatementBlock(pointedStmt, &outerBlocks);
			OPS_ASSERT(nSource != outerBlocks.size());
			OPS_ASSERT(nTarget != outerBlocks.size());
			//BlockGraph::iterator pSource = std::find_if(outerBlocks.begin(), outerBlocks.end(), NodeSearcher(sourceBlock));
			//BlockGraph::iterator pTarget = std::find_if(outerBlocks.begin(), outerBlocks.end(), NodeSearcher(targetBlock));
			//OPS_ASSERT(pSource != outerBlocks.end());
			//OPS_ASSERT(pTarget != outerBlocks.end());
			//pSource->second.push_back(targetBlock);
			outerBlocks[nSource].second.push_back(nTarget);
		}
		
		// для каждого блока соберем все операторы внутри него,
		// которые могут вызывать "неожиданные" передачи управления
		// наружу
		// break, continue, goto, return
		// каждый такой оператор означает дугу на графе блоков
		BlockGraph::iterator pCurrentBlock = outerBlocks.begin();

        typedef Shared::NodesCollector<Canto::HirBreakStatement> BreakCollector;
        typedef Shared::NodesCollector<Canto::HirContinueStatement> ContinueCollector;
		typedef Shared::NodesCollector<ReturnStatement> ReturnCollector;

		for(size_t nCurrentBlock = 0; pCurrentBlock != outerBlocks.end(); ++pCurrentBlock, ++nCurrentBlock)
		{
			BlockStatement* currentBlock = pCurrentBlock->first;
			BreakCollector breaks;
			ContinueCollector continues;
			ReturnCollector returns;
			GotoCollector gotos;

			Blocks newEdges;
			currentBlock->accept(breaks);
			currentBlock->accept(continues);
			currentBlock->accept(returns);
			currentBlock->accept(gotos);

			std::vector<StatementBase*> trickyStatements;
			trickyStatements.insert(trickyStatements.end(), breaks.getCollection().begin(), breaks.getCollection().end());
			trickyStatements.insert(trickyStatements.end(), continues.getCollection().begin(), continues.getCollection().end());
			trickyStatements.insert(trickyStatements.end(), returns.getCollection().begin(), returns.getCollection().end());
			trickyStatements.insert(trickyStatements.end(), gotos.getCollection().begin(), gotos.getCollection().end());

			std::vector<StatementBase*>::iterator pStmt = trickyStatements.begin();
			for(; pStmt != trickyStatements.end(); ++pStmt)
			{
				StatementBase* stmt = getNextOperator(*pStmt);
				size_t nTarget = findGraphNearestStatementBlock(stmt, &outerBlocks);
				OPS_ASSERT(nTarget != outerBlocks.size());
				//BlockGraph::iterator pTarget = std::find_if(outerBlocks.begin(), outerBlocks.end(), NodeSearcher(block));
				//OPS_ASSERT(pTarget != outerBlocks.end());
				if(nTarget > nCurrentBlock)
				{
					pCurrentBlock->second.push_back(nTarget);
				}
			}
		}		

		// найдем максимальный блок, который достижим из данного по найденным дугам,
		// или из которого достижим данный

		// пометки блоков о посещении при поиске в глубину
		std::vector<bool> forwardMarks(outerBlocks.size(), false);
		// пометки блоков о посещении при поиске в глубину
		// на графе с обращенными дугами
		std::vector<bool> backwardMarks(outerBlocks.size(), false);

		BlockGraph backwardOuterBlocks;
		inverseEdges(outerBlocks, backwardOuterBlocks);
		
		// поиск в глубину, начиная с самого глубоко вложенного блока
		DFS(outerBlocks, forwardMarks);
		DFS(backwardOuterBlocks, backwardMarks);

		BlockGraph::reverse_iterator pBlock = outerBlocks.rbegin();
		std::vector<bool>::reverse_iterator pForwardMark = forwardMarks.rbegin();
		std::vector<bool>::reverse_iterator pBackwardMark = backwardMarks.rbegin();
		for(; pBlock != outerBlocks.rend(); ++pBlock, ++pForwardMark, ++pBackwardMark)
		{
			if(*pForwardMark || *pBackwardMark) break;
		}
		
		if(pBlock != outerBlocks.rend())
		{
			return pBlock->first;
		} else return block;
	}

} // end namespace Shared
} // end namespace OPS
