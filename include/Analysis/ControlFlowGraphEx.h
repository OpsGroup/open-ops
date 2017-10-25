#ifndef OPS_IR_REPRISE_CONTROL_FLOW_GRAPH_EX_H_INCLUDED__
#define OPS_IR_REPRISE_CONTROL_FLOW_GRAPH_EX_H_INCLUDED__

#include <map>
#include <list>
#include <vector>
#include <stack>


#include "Reprise/Reprise.h"

namespace OPS
{
	namespace Analysis
	{
		namespace ControlFlowGraphs
		{
			// Вершины управляющего графа - линейные блоки - векторы элементов, 
			// которые могут быть как операторами, так и выражениями, и объявлениями переменных
			// Stash - класс для представления элементов линейного блока
			// хранит указатель на выражение или на оператор
			class Stash
			{
			private:
				OPS::Reprise::StatementBase* pStatement;
				OPS::Reprise::ExpressionBase* pExpression;
				Reprise::VariableDeclaration* pDeclaration;

			public:
				Stash();
				OPS::Reprise::BlockStatement& getParentBlock();
				Stash(OPS::Reprise::StatementBase* statement);
				Stash(OPS::Reprise::ExpressionBase* expression);
				Stash(OPS::Reprise::VariableDeclaration* declaration);
				Stash(const Stash& lhs);

				bool isStatement() const;

				bool isExpression() const;

				bool isDeclaration() const;

				OPS::Reprise::StatementBase* getAsStatement() const;

				OPS::Reprise::ExpressionBase* getAsExpression() const;

				OPS::Reprise::VariableDeclaration* getAsDeclaration() const;

				void setStatement(OPS::Reprise::StatementBase* pS);

				void setExpression(OPS::Reprise::ExpressionBase* pE);

				void setDeclaration(OPS::Reprise::VariableDeclaration* pD);

				bool invariant() const;

				bool isEmpty() const;

				bool operator==(const Stash& lhs) const;

			};

			namespace SSAControlFlowGraph 
			{
				typedef std::vector<Stash> StatementVector;                                     //
				typedef std::list<StatementVector*> StatementVectorList;	                    //
				typedef std::map<StatementVector*, StatementVectorList*> StatementGraph;        // граф, хранящийся списками дуг
				typedef std::map<StatementVector*, StatementVector*> StatementTree;             //
				typedef std::map<StatementVector* , std::map<StatementVector*, int > > CFGraph; // матрица инцидентности
			}

			class ControlFlowGraphEx : public OPS::NonAssignableMix, public OPS::NonCopyableMix, public OPS::Reprise::IntrusivePointerBase
			{
			private:
				SSAControlFlowGraph::StatementVectorList m_blocks; // список всех линейных блоков
				SSAControlFlowGraph::StatementGraph m_blocksSucc;  // граф по линейным блокам
				SSAControlFlowGraph::StatementGraph m_blocksPred;  // граф по линейным блокам с обращёнными дугами
				SSAControlFlowGraph::CFGraph m_cfGraph;            // матрица инцидентности
				OPS::Reprise::BlockStatement* m_block;			   // блок, граф которого представляется
			public:
				explicit ControlFlowGraphEx(OPS::Reprise::BlockStatement &aBlock);
				virtual ~ControlFlowGraphEx();
				OPS::Reprise::BlockStatement& rootBlock();
				SSAControlFlowGraph::StatementVectorList& getSubblocks(); //список всех линейных блоков
				SSAControlFlowGraph::StatementGraph& getSucc();       //граф потомков
				SSAControlFlowGraph::StatementGraph& getPred();       //граф предков
				SSAControlFlowGraph::CFGraph& getCFGraph();              //матрица инцидентности
				int getBlockNum(SSAControlFlowGraph::StatementVector* blk);               //номер блока в списке блоков
				SSAControlFlowGraph::StatementVector* getBlock(Stash element); //получить блок по элементу;


				/*кидает эксепшн, если num > blocks.size()*/
				SSAControlFlowGraph::StatementVector* getBlockByNum(unsigned int num);             //блок по его номеру в списке блоков, TODO: начиная с нуля

				//найти блок, в котором есть даннный statement/expression
				SSAControlFlowGraph::StatementVector* findMatch(OPS::Reprise::StatementBase* sMatch) 
				{
					SSAControlFlowGraph::StatementVectorList::iterator match = findMatch(Stash(sMatch));
					if( match != m_blocks.end()) return *match;
					else return 0;

				}
				SSAControlFlowGraph::StatementVector* findMatch(OPS::Reprise::ExpressionBase* sMatch)
				{
					SSAControlFlowGraph::StatementVectorList::iterator match = findMatch(Stash(sMatch));
					if( match != m_blocks.end()) return *match;
					else return 0;
				}
			private:
				void clear(); //очистить выделенную память
				bool ext(Stash currStash);
				OPS::Reprise::StatementBase* findIfOrSwitch(Stash currStash);	          // возвращает if или switch, содержащие текущий оператор
				OPS::Reprise::StatementBase* findWhileOrFor(Stash currStash);           // -----//---- while или for
				OPS::Reprise::VariableDeclaration* findFirstDeclarationInBlock(OPS::Reprise::BlockStatement*);
				Stash intoBlock(Reprise::BlockStatement*);
				Stash next(Stash runner);                              // возвращает следующий по потоку исполнения оператор
				Stash skip(Stash runner);                              // 
				SSAControlFlowGraph::StatementVectorList::iterator findMatch(Stash sMatch);    // указывает на блок в списке, в котором есть stmtMatch
				void rewriteLinks(SSAControlFlowGraph::StatementVector* what, SSAControlFlowGraph::StatementVector* withWhat); //переписывает ссылки предков блока при разбиении его на две части
				SSAControlFlowGraph::StatementVector* splitIfNeeded(Stash sMatch);             // разбивает блок на два по оператору, если он стоит внутри блока
				void buildBlocks();                                       // строит блоки и граф потомков
				SSAControlFlowGraph::StatementVector* buildBlock(Stash);             //
				void buildCFGraph();                                      // строит м-цу инцидентности блоков
				void buildReverseCFGraph();                               // строит граф предков
				bool isControlStash(Stash runner);						  //не точка ли это ветвления?
				SSAControlFlowGraph::StatementVector branches(Stash runner); //ветвления элемента (ветки if'а, switch'а и тд)

			};


		}
	}
}

#endif //OPS_IR_REPRISE_CONTROL_FLOW_GRAPH_EX_H_INCLUDED__
