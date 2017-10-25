#ifndef OPS_EXPRGRAPH_H_
#define OPS_EXPRGRAPH_H_

#include "Reprise/Expressions.h"

#include <vector>


namespace CalculationGraphSpace
{

	enum EN_NodeType
	{
		NT_PIPELINED,
		NT_NOTPIPELINED,
		NT_TRANSIT,
		NT_CMP
	};

/// Класс-шаблон для описания графа вычислений в одномерном случае и в многомерном
template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge> 
class ExpressionsGraph 
{
public:

	/// Добавление вершины к графу
	void addNode(ExpressionsGraphNode* Vertex);

	/// Прототип добавления дуги
	void addDirEdge(ExpressionsGraphNode* Begin, ExpressionsGraphNode* End);

	/// Найти итератор вершину графа по выражению из программы
	typename std::vector<ExpressionsGraphNode*>::iterator findNodeIteratorByExpressionBase(OPS::Reprise::ExpressionBase* a) const;

	/// Возвращает список вершин для 
	const std::vector<ExpressionsGraphNode*>& getNodesVector() { return m_Nodes; } 

    /// Создает нумерацию для вершин графа
    void numerateNodes(const int startVertex = 1);

	/// Получить информацию о величине задержки // TODO: выяснить нужна ли она тут 
	virtual int getNodeOperationDelayByExpressionBase(OPS::Reprise::ExpressionBase* Data) const = 0;
	
	/// Прототип функции для графов наследников // TODO: выяснить нужна ли она тут 
	//virtual bool initializeByStatements(OPS::Reprise::BlockStatement* GivenBlock) = 0;

	/// Деструктор
	virtual ~ExpressionsGraph() { clear(); }

protected:
	/// Найти итератор вершину графа по вхождению (\see ExpressionBase) из программы
	typename std::vector<ExpressionsGraphNode*>::iterator findNodeIteratorByExpressionBaseUnsafe(const OPS::Reprise::ExpressionBase* a);

	/// Найти вершину графа по вхождению (\see ExpressionBase) из программы
	ExpressionsGraphNode* findNodeByExpressionBase(const OPS::Reprise::ExpressionBase* a);

//private:
	/// Очистка объекта класса
	virtual void clear();

	/// Вспомогательная процедура для разбора оператора как выражения в граф
	bool parseAssignmentStatement(OPS::Reprise::StatementBase* pStatementToParse, ExpressionsGraphNode*& Generator);

	/// Вспомогательная процедура для разбора оператора как выражения в граф
	bool parseExpression(OPS::Reprise::ExpressionBase* pExpressionToParse, ExpressionsGraphNode*& Root);

	// Данные-члены
	std::string m_strGraphName;
	std::vector<ExpressionsGraphNode*> m_Nodes; // набор всех вершин
	EN_NodeType m_DefaultNodeType;
};

template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
void ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::addNode(ExpressionsGraphNode* Vertex)
{
	m_Nodes.push_back(Vertex);
}


template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
void ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::numerateNodes(const int startVertex)
{
	int iCurrentNodeNumber = startVertex;
    for (ExpressionsGraphNode* node: m_Nodes)
        node->setNodeNumber(iCurrentNodeNumber++);
}


template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
typename std::vector<ExpressionsGraphNode*>::iterator ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::findNodeIteratorByExpressionBase(OPS::Reprise::ExpressionBase* a) const
{
	return findNodeIteratorByExpressionBaseUnsafe(a);
}

template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
typename std::vector<ExpressionsGraphNode*>::iterator ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::findNodeIteratorByExpressionBaseUnsafe(const OPS::Reprise::ExpressionBase* a)
{
	using namespace OPS::Reprise;
	typename std::vector<ExpressionsGraphNode*>::iterator itCurrentNode = m_Nodes.begin(),
												itLastNode = m_Nodes.end();
	// Перебираем все вершины
	for(;itCurrentNode!=itLastNode; itCurrentNode++)
	{
		typename std::list<RepriseBase*>::iterator itCurrentData = (*itCurrentNode)->m_Data.begin();
		for(;itCurrentData != (*itCurrentNode)->m_Data.end(); itCurrentData++)
		{
			ExpressionBase* curData;
			if ((*itCurrentData)->is_a<ExpressionBase>())
			{
				curData = (*itCurrentData)->cast_ptr<ExpressionBase>();

				// Проверка для операций, констант, скалярных переменных и пр простых наследников ExpressionBase 
				if (curData == a) 
				{ // нашли!
					return itCurrentNode; 
				}

				// Проверка для операции обращения к элементу массива
				if (curData->is_a<BasicCallExpression>())
				{
					BasicCallExpression* pCurrentBasicCall = curData->cast_ptr<BasicCallExpression>();
					if (pCurrentBasicCall->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
					{
						if (&(pCurrentBasicCall->getArgument(0)) == a) 
						{ // нашли по вхождению имени!
							return itCurrentNode; 
						}
					}
				}
			}
		}
	}

	return itLastNode;
}

template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
ExpressionsGraphNode* ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::findNodeByExpressionBase(const OPS::Reprise::ExpressionBase* a)
{ 
	typename std::vector<ExpressionsGraphNode*>::iterator it = findNodeIteratorByExpressionBaseUnsafe(a);
	if (it == m_Nodes.end())
		return 0;
	else 
		return *it;
}

// Разбирает оператор присваивания для создания вершин графа. 
template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
bool ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::parseAssignmentStatement(OPS::Reprise::StatementBase* pStatementToParse, ExpressionsGraphNode*& Generator)
{
	using namespace OPS::Reprise;

	ExpressionBase *pCurrentData = 0, *pPreviousData = 0;
	ExpressionsGraphNode *pCurrentNode = 0;

	pCurrentData = 0;

	// Обход дерева выражения в глубину для оператора присваивания
	if (pStatementToParse->is_a<ExpressionStatement>())
	{
		ExpressionBase& Expr = pStatementToParse->cast_to<ExpressionStatement>().get();

		if (Expr.is_a<BasicCallExpression>())
		{
			BasicCallExpression& BExpr = Expr.cast_to<BasicCallExpression>();
			if(BExpr.getKind() == BasicCallExpression::BCK_ASSIGN)
			{
				// Левая часть оператора присваивания
				pCurrentData = &(BExpr.getArgument(0));

				// Вносим в граф вершину ссответсвующую записи в переменную стоящую в левой части оператора присваивания. 
				// Множественные присваивания могут не работать! 
				pCurrentNode = new ExpressionsGraphNode(pCurrentData, getNodeOperationDelayByExpressionBase(pCurrentData), m_DefaultNodeType);
				addNode(pCurrentNode);

				// Правая часть оператора присваивания
				pPreviousData = pCurrentData;
				pCurrentData = &(BExpr.getArgument(1));
			}
			else 
			{
				OPS::Console* const pConsole = &OPS::getOutputConsole(m_strGraphName);
				pConsole->log(OPS::Console::LEVEL_ERROR, "Not an assign operator will not be parsed to graph!");
				return true; // TODO: разобраться почему истина?
			}
		}
		else
		{
			OPS::Console* const pConsole = &OPS::getOutputConsole(m_strGraphName);
			pConsole->log(OPS::Console::LEVEL_ERROR, "Not an assign operator will not be parsed to graph!");
			return true; // TODO: разобраться почему истина?
		}
	}
	OPS_UNUSED(pPreviousData);

	if (pCurrentData != 0)
	{
		Generator = pCurrentNode;
		if (parseExpression(pCurrentData, pCurrentNode))
		{
			addDirEdge(pCurrentNode, Generator);
			return true;
		}
	}

	return false;
}

// Разбирает оператор присваивания для создания вершин графа. 
template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
bool ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::parseExpression(OPS::Reprise::ExpressionBase* pExpressionToParse, ExpressionsGraphNode*& Root)
{

	using namespace OPS::Reprise;

	ExpressionBase *pCurrentData, *pPreviousData = 0;
	ExpressionsGraphNode *pCurrentNode (0),*pPreviousNode (0);

	pCurrentData = pExpressionToParse;

	bool bFirstEntry(true), bExit(false);
	// Правая часть оператора присваивания (этот подалгоритм выполняется только,
	// если pStatementToParse - оператор присваивания)

	while (!bExit)
	{	
		if ((pCurrentData->is_a<BasicCallExpression>()) || (pCurrentData->is_a<LiteralExpression>()) || (pCurrentData->is_a<ReferenceExpression>()))
		{
			// Строим граф вычислений по выражениям в операторе
			if (!bFirstEntry)
				pPreviousNode = pCurrentNode;

			pCurrentNode = findNodeByExpressionBase(pCurrentData);
			if (pCurrentNode == 0)
			{
				pCurrentNode = new ExpressionsGraphNode(pCurrentData, getNodeOperationDelayByExpressionBase(pCurrentData), m_DefaultNodeType);
				addNode(pCurrentNode);
			}
			
			if (!bFirstEntry)
				if (pCurrentData == pPreviousData->getParent())
					addDirEdge(pPreviousNode, pCurrentNode);
				
			
			// Рассмотрим вызовы переменных и константы
			if ((pCurrentData->is_a<LiteralExpression>()) || (pCurrentData->is_a<ReferenceExpression>()))
			{
				if (pCurrentData == pExpressionToParse) // выражение pExpressionToParse состояло из одной лишь константы 
					bExit = true;
				else
				{
					pPreviousData = pCurrentData;
					pCurrentData = &(pCurrentData->getParent()->cast_to<ExpressionBase>());
				}
				continue;
			}

			// Рассмотрим все операции, включая обращение к элементу массива
			if (pCurrentData->is_a<BasicCallExpression>())
			{
				BasicCallExpression& CurrentBasicCall = pCurrentData->cast_to<BasicCallExpression>();
				//Движение по дереву в зависимости от типа текущей вершины
				switch (CurrentBasicCall.getKind())
				{
				case BasicCallExpression::BCK_UNARY_PLUS:
				case BasicCallExpression::BCK_UNARY_MINUS: 
				case BasicCallExpression::BCK_LOGICAL_NOT:
				case BasicCallExpression::BCK_BITWISE_NOT:
					{
						if (pPreviousData != &(CurrentBasicCall.getArgument(0)))
						{
							// Шаг в глубину
							pPreviousData = pCurrentData;
							pCurrentData = &(CurrentBasicCall.getArgument(0));
						}
						else
						{
							// Шаг на поверхность
							if (pCurrentData == pExpressionToParse) // достигли корня дерева обойдя все поддеревья 
							{
								bExit = true;
								break;
							}
							pPreviousData = pCurrentData;
							pCurrentData = &(CurrentBasicCall.getParent()->cast_to<ExpressionBase>());
						}
						break;
					}
				case BasicCallExpression::BCK_BINARY_PLUS:
				case BasicCallExpression::BCK_BINARY_MINUS:
				case BasicCallExpression::BCK_MULTIPLY:
				case BasicCallExpression::BCK_DIVISION:
				case BasicCallExpression::BCK_INTEGER_DIVISION:
				case BasicCallExpression::BCK_INTEGER_MOD: 
				case BasicCallExpression::BCK_GREATER:
				case BasicCallExpression::BCK_GREATER_EQUAL:
				case BasicCallExpression::BCK_LESS:
				case BasicCallExpression::BCK_LESS_EQUAL:
				case BasicCallExpression::BCK_EQUAL:
				case BasicCallExpression::BCK_NOT_EQUAL:
				case BasicCallExpression::BCK_LOGICAL_AND:
				case BasicCallExpression::BCK_LOGICAL_OR:
				case BasicCallExpression::BCK_BITWISE_AND:		
				case BasicCallExpression::BCK_BITWISE_OR:		
				case BasicCallExpression::BCK_BITWISE_XOR:
					{
						if (pPreviousData == &(CurrentBasicCall.getArgument(1)))
						{
							// Пришли справа
							if (pCurrentData == pExpressionToParse) // достигли корня дерева обойдя все поддеревья 
							{
								bExit = true;
								break;
							}
							pPreviousData = pCurrentData;
							pCurrentData = &(CurrentBasicCall.getParent()->cast_to<ExpressionBase>());
						}
						else
						{
							// Пришли либо сверху, либо слева
							if (pPreviousData == &(CurrentBasicCall.getArgument(0)))
							{
								// Пришли слева - идем в глубину по правой дуге
								pPreviousData = pCurrentData;
								pCurrentData = &(CurrentBasicCall.getArgument(1));
							}
							else
							{
								// Пришли сверху - идем в глубину по левой дуге
								pPreviousData = pCurrentData;
								pCurrentData = &(CurrentBasicCall.getArgument(0));
							}
						}
						break;
					}
				case BasicCallExpression::BCK_ARRAY_ACCESS: 
					{
						if (pCurrentData == pExpressionToParse) // выражение pExpressionToParse состояло только из одного обращения к массиву
							bExit = true;
						else
						{
							pPreviousData = pCurrentData;
							pCurrentData = &(CurrentBasicCall.getParent()->cast_to<ExpressionBase>());
							//if (pCurrentData == pExpressionToParse) // достигли корня дерева обойдя все поддеревья 
							//	bExit = true;
						}
						break;
					}
				case BasicCallExpression::BCK_ASSIGN:
					{
						/* TODO: Здесь должен быть случай множественного присваивания!
						// Вернулись к присваиванию, значит создаем последнюю дугу
						// от левой части присваивания к правой ...
						pCurrentData = &(CurrentBasicCall.getArgument(0));
						pCurrentNode = findNodeByExpressionBase(pCurrentData);
						pPreviousNode = findNodeByExpressionBase(pPreviousData);						
						addDirEdge(pPreviousNode, pCurrentNode);
						// ... и выход
						pCurrentData = 0;
						*/
						if (pCurrentData == pExpressionToParse) // достигли корня дерева обойдя все поддеревья 
							bExit = true;
						break;
					}
				default:
					{
						OPS::Console* const pConsole = &OPS::getOutputConsole(m_strGraphName);
						pConsole->log(OPS::Console::LEVEL_ERROR, "During CalculationGraph construction BasicCallExpression node of unexpected kind was found!");
						return false;
					}
				} // end of switch

			} // if BasicCallexpression ?
		}
		else
		{
			OPS::Console* const pConsole = &OPS::getOutputConsole(m_strGraphName);
			pConsole->log(OPS::Console::LEVEL_ERROR, "During CalculationGraph construction ExpressionBase node of unexpected type was found!");
			return false;
		}

		bFirstEntry = false;
	} // end while

	Root = findNodeByExpressionBase(pCurrentData);
	return true;
}

template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
void ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::addDirEdge(ExpressionsGraphNode* Begin, ExpressionsGraphNode* End)
{
	Begin->addOutDirEdge(End);
	//End->addInDirEdge(Begin); - не нужно!!!
}

template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
void ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::clear() 
{
	m_Nodes.clear();
}


//template <class ExpressionsGraphNode, class ExpressionsGraphDirEdge>
//bool ExpressionsGraph<ExpressionsGraphNode, ExpressionsGraphDirEdge>::initializeByStatements(BlockStatement* LoopBody)
//{
//		
//	StatementBase::Iterator itStmtIter = LoopBody->getFirst();
//
//	// Цикл по всем операторам расположенным в LoopBody
//	for(; itStmtIter.isValid() == 1; itStmtIter++)
//	{
//		if (!parseAssignmentStatement(&(*itStmtIter)))
//		{
//			return false;
//		}
//	}
//
//	return true;
//}
}

#endif                      //_EXPRGRAPH_H_
