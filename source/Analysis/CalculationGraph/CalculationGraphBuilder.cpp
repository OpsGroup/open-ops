#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "OPS_Core/Exceptions.h"
#include "Analysis/CalculationGraph/CalculationGraphBuilder.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Shared/ParametricLinearExpressions.h"
#include "Shared/LinearExpressions.h"
#include "Shared/LoopShared.h"
//#include "PipelineStartDelays.h"
#include "Shared/Checks.h"
#include "OPS_Core/Localization.h"
#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::PlatformSettingsSpace;
using namespace OPS::Montego;
using namespace std;

namespace CalculationGraphSpace
{

OPS_DEFINE_EXCEPTION_CLASS(DependenceProcessingError, OPS::RuntimeError);


// ================================================
//          AdjacencyList
// ================================================

inline void CalculationGraphBuilder::recountAfterTrueDep(Edge& edge)
{
    Node& node = adjList[edge.to];
    if (edge.dist < node.minInDepDist)
    {
        node.minInDepDist = edge.dist;
        node.inDepNodeNum = getExprNum(edge.arc->getStartVertex().getSourceOccurrence()->getSourceExpression());
    }
    else if (edge.dist == node.minInDepDist)
    {
        // из дуг с одинаковым расстоянием выбираем последний
        node.inDepNodeNum = std::max(node.inDepNodeNum, getExprNum(edge.arc->getStartVertex().getSourceOccurrence()->getSourceExpression()));
    }

}

int CalculationGraphBuilder::getExprNum(RepriseBase* expr)
{
    while (!expr->is_a<ExpressionStatement>())
        expr = expr->getParent();
    return exprOrder[expr->cast_ptr<ExpressionStatement>()];
}

CalculationGraphBuilder::Edge& CalculationGraphBuilder::AdjacencyList::addEdge(int from, int to, DependenceGraphAbstractArc* arc)
{
    const int type = arc->getDependenceType();
    Edge* res = nullptr;

    std::vector<Edge>* list;
    switch (type) {
        case DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE:
            list = &(*this)[from].outList;
            break;
        case DependenceGraphAbstractArc::DT_ININ_DEPENDENCE:
            list = &(*this)[from].inList;
            break;
        case DependenceGraphAbstractArc::DT_ANTIDEPENDENCE:
            list = &(*this)[from].antiList;
            break;
        case DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE:
            list = &(*this)[from].trueList;
            break;
        default:
            throw DependenceProcessingError("unsupported arc type");
    }

    list->push_back(Edge(to, arc));
    return list->back();
}


// ================================================
//          Builder
// ================================================

CalculationGraphBuilder::CalculationGraphBuilder(CalculationGraph::EN_GraphCreatingMode GraphType, const PlatformSettings* settings, EN_NodeType DefaultNodeType)
    : m_PlatformSettings(settings)
{
    m_DefaultNodeType = DefaultNodeType;
}

bool CalculationGraphBuilder::createGraph(ForStatement* Loop)
{
    if (Loop == nullptr)
        return false;

	BlockStatement* LoopBody = &(Loop->getBody());
	list<CalculationGraphNode*> Generators; // необходимо для вызова, но не нужно для дальнейшего анализа в этом месте

    lastOrderNum = 0;
    if (!initializeByStatements(LoopBody, Generators))
        return false;

    std::unique_ptr<DependenceGraph> dependenceGraph(new DependenceGraph(*Loop));
    dependenceGraph->removeCounterArcs();
	dependenceGraph->refineAllArcsWithLatticeGraph();

    if (!isPipelinableOneDimLoop(Loop, dependenceGraph.get()))
        return false;
    if (!analyzeDependencies(Loop, dependenceGraph.get())) // Анализируем все зависимости по графу Лампорта
        return false;

    numerateNodes(); // Нумеруем вершины графа
    return true;
}

bool CalculationGraphBuilder::initializeByStatements(BlockStatement* GivenBlock, list<CalculationGraphNode*>& Generators)
{
    OPS_ASSERT(GivenBlock != nullptr);
	BlockStatement::Iterator itStmtIter = GivenBlock->getFirst();

	// Цикл по всем операторам расположенным в LoopBody
	for(; itStmtIter.isValid(); itStmtIter++)
	{
		// Цикл for
		if ((*itStmtIter).is_a<ForStatement>())
			if (!initializeByStatements(&itStmtIter->cast_to<ForStatement>().getBody(), Generators))
				return false;

		// Условный оператор
		if ((*itStmtIter).is_a<IfStatement>())
		{
			IfStatement* pIfStatement = (*itStmtIter).cast_ptr<IfStatement>();
			CalculationGraphNode* pConditionResult;
			list<CalculationGraphNode*> ThenBlockGenerators, ElseBlockGenerators;
			if (!parseExpression(&pIfStatement->getCondition(), pConditionResult))
				return false;
			if (!initializeByStatements(&pIfStatement->getThenBody(), ThenBlockGenerators))
				return false;
			if (!initializeByStatements(&pIfStatement->getElseBody(), ElseBlockGenerators))
				return false;

			// Добавим дугу ведущую от результата вычисленного условия к узлу If
            CalculationGraphNode* pIfNode = new CalculationGraphNode(pIfStatement, m_PlatformSettings->m_CompareDelay);
			addNode(pIfNode);
			addDirEdge(pConditionResult, pIfNode);

			// Добавим управляющие дуги в блок Then
            for (CalculationGraphNode* iterGenerator: ThenBlockGenerators)
				addDirEdge(pIfNode, iterGenerator, 0, CalculationGraphDirEdge::ST_CONTROLFLOW_00);

			// Добавим управляющие дуги в блок Else
			BasicCallExpression* pNotRepriseNode = new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT);
            CalculationGraphNode* pNotNode = new CalculationGraphNode(pNotRepriseNode, m_PlatformSettings->m_BooleanDelay);
			addNode(pNotNode);
			addDirEdge(pIfNode, pNotNode);

            for (CalculationGraphNode* iterGenerator: ElseBlockGenerators)
				addDirEdge(pNotNode, iterGenerator, 0, CalculationGraphDirEdge::ST_CONTROLFLOW_00);
		}

		// Оператор присваивания
		if ((*itStmtIter).is_a<ExpressionStatement>())
		{
			exprOrder[(*itStmtIter).cast_ptr<ExpressionStatement>()] = lastOrderNum++;
            CalculationGraphNode* pGenerator = nullptr;
			if (!parseAssignmentStatement(&(*itStmtIter), pGenerator))
				return false;
			Generators.push_back(pGenerator);
		}

	}

	return true;
}

int CalculationGraphBuilder::getNodeOperationDelayByExpressionBase(ExpressionBase* Data) const
{
	if (Data->is_a<BasicCallExpression>())
	{
		switch(Data->cast_to<BasicCallExpression>().getKind())
		{
		case BasicCallExpression::BCK_BINARY_PLUS:
		case BasicCallExpression::BCK_BINARY_MINUS:
			return m_PlatformSettings->m_IntSumDelay;

		case BasicCallExpression::BCK_MULTIPLY:
			return m_PlatformSettings->m_IntMultiplicationDelay;

		case BasicCallExpression::BCK_DIVISION:
			return m_PlatformSettings->m_IntDivisionDelay;

		default:
            return 1; // TODO: более развернутая проверка необходима. Возможно надо будет кидать исключения.
		}
	}
	else
		return m_PlatformSettings->m_ReadingWriting; // TODO: более развернутая проверка необходима
}


// ================================================
//  Dependencies analyzation's nodes processing
// ================================================

void CalculationGraphBuilder::AdjacencyList::deleteAntidependences(int v)
{
    OPS_ASSERT(v < (int)size());

    (*this)[v].antiList.clear();
}

// Добавляет дугу в граф
CalculationGraphDirEdge* CalculationGraphBuilder::addDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistance, CalculationGraphDirEdge::EN_SignalType SignalType)
{
	return Begin->addOutDirEdge(End, DepDistance, SignalType);
}


void CalculationGraphBuilder::deleteRedundantNodes()
{
    for (CalculationGraphNode*& it: m_Nodes)
        if (it->m_InDirEdges.size() == 0 && it->m_OutDirEdges.size() == 0)
        {
            delete it;
            it = nullptr;
        }
    m_Nodes.erase(std::remove(m_Nodes.begin(), m_Nodes.end(), nullptr), m_Nodes.end());
}

bool CalculationGraphBuilder::glueNodesAfterFlowDependency(int from, int to, const int dist)
{
    OPS_ASSERT(from < (int)m_Nodes.size() && to <= (int)m_Nodes.size());

    if (dist < 0)
        return !(dist < -1);

	//Получим начало дуги (генератор!!!)
	CalculationGraphNode *pGeneratorNode = m_Nodes[from];
	// Получим конец дуги (использование!!!)
	CalculationGraphNode *pUseNode = m_Nodes[to];

	// Проверка корректности
	int inEdgeCount = 0;
    CalculationGraphDirEdge *pDataFlowInDirEdge = nullptr;
    for (CalculationGraphDirEdge* iter: pGeneratorNode->m_InDirEdges)
		if (iter->getSignalType() == CalculationGraphDirEdge::ST_DATAFLOW && iter->getDirEdgeType() != CalculationGraphDirEdge::DET_INITIALIZE)
        {
			++inEdgeCount;
			pDataFlowInDirEdge = iter;
		}

	if (inEdgeCount != 1)
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Incorrect number of DirEdges sink to generator " + pGeneratorNode->getStringRepresentation() + "!",""));
		return false;
	}

	// Будем строить эту DATAFLOW дугу не от генератора, а
	// от предыдущей вершины, которая должна быть одна
	CalculationGraphNode *pBeginOfTheDirEdge = pDataFlowInDirEdge->getBeginUnsafe();

	// Если такая дуга уже есть (например оператор a[i] = a[i-1]), то ничего делать не надо.
	if (pUseNode == pBeginOfTheDirEdge)
		return true;

	// Проверка корректности
	if (pUseNode->m_InDirEdges.size() != 0)
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
		pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Too many DirEdges sink to use!",""));
		return false;
	}

	// дуги исходящие из pUseNode (чтение) и склееных с ней
	// удаляем или переведим в дуги инициализации (see toInit)
	// и создадим набор новых дуг порожденных этой зависимостью
	for(auto iter = pUseNode->m_OutDirEdges.begin(); iter != pUseNode->m_OutDirEdges.end();)
	{
		CalculationGraphDirEdge* pDirEdgeFromUse = *iter;
        if (pDirEdgeFromUse->m_DirEdgeType == CalculationGraphDirEdge::DET_INITIALIZE)
        {
            ++iter;
            continue; // дуги инициализации не учитываются, пропускаем
        }

       // Находим конец новой дуги
		CalculationGraphNode *pEndOfTheDirEdge = pDirEdgeFromUse->getEndUnsafe();
		// Добавляем дугу в Граф Вычислений
		CalculationGraphDirEdge* edge = addDirEdge(pBeginOfTheDirEdge, pEndOfTheDirEdge, dist + pDirEdgeFromUse->getDependenceDistance() + pDataFlowInDirEdge->getDependenceDistance());

        if (dist > 0)
        {
            if (pDirEdgeFromUse->m_InitializationDirEdge == nullptr)
            {
                pDirEdgeFromUse->convertDirEdgeToInitDirEdge(); // Переводим в дугу инициализации
                edge->m_InitializationDirEdge = pDirEdgeFromUse;
                ++iter;
            }
            else
            {
                edge->m_InitializationDirEdge = pDirEdgeFromUse->m_InitializationDirEdge; // указываем дугу инициализации, связанную тем же входом.
                iter = pDirEdgeFromUse->deleteOccurrences().second;
            }
        }
        else // подстановка вершины
        {
            CalculationGraphDirEdge* pDataFlowInitDirEdge = pDataFlowInDirEdge->m_InitializationDirEdge;
            if (pDataFlowInitDirEdge != nullptr)
            {
		        // Копируем дугу инициализации
		        edge->m_InitializationDirEdge = addDirEdge(pDataFlowInitDirEdge->getBeginUnsafe(), pEndOfTheDirEdge, 0);
                edge->m_InitializationDirEdge->convertDirEdgeToInitDirEdge();
            }
            iter = pDirEdgeFromUse->deleteOccurrences().second;
	    }
    }

	return true;
}

void CalculationGraphBuilder::glueNodesAfterInInDependency(int from, int to, int dist)
{
    OPS_ASSERT(from < (int)m_Nodes.size() && to <= (int)m_Nodes.size());
	CalculationGraphNode *pFirstUseNode = m_Nodes[from];
	CalculationGraphNode *pSecondUseNode  = m_Nodes[to];

	if (pFirstUseNode == nullptr || pSecondUseNode == nullptr || pFirstUseNode == pSecondUseNode )
		return; // эта зависимость не связывает вершины графа вычислений

	// Склейка вершин (этих)
    for (RepriseBase* itCurrentExpressionBase: pSecondUseNode->m_Data)
        pFirstUseNode->m_Data.push_back(itCurrentExpressionBase);

	// копирование дуг
	for (CalculationGraphDirEdge* itCurrentDirEdge: pSecondUseNode->m_InDirEdges)
    {
        OPS_ASSERT(itCurrentDirEdge->getDependenceDistance() - dist >= 0);
		CalculationGraphDirEdge* edge = pFirstUseNode->addInDirEdge(itCurrentDirEdge->getBeginUnsafe(), itCurrentDirEdge->getDependenceDistance() - dist);
        edge->m_DirEdgeType = itCurrentDirEdge->m_DirEdgeType;
    }

    for (CalculationGraphDirEdge* itCurrentDirEdge: pSecondUseNode->m_OutDirEdges)
	{
        OPS_ASSERT(itCurrentDirEdge->getDependenceDistance() + dist >= 0);
		CalculationGraphDirEdge* edge = pFirstUseNode->addOutDirEdge(itCurrentDirEdge->getEndUnsafe(), itCurrentDirEdge->getDependenceDistance() + dist);
        edge->m_DirEdgeType = itCurrentDirEdge->m_DirEdgeType;
	}

	// Удаляем второе использование (pSecondUseNode) как вершину графа из списка
    pSecondUseNode->clear();
}


// ================================================
//          Dependencies analyzation
// ================================================

void CalculationGraphBuilder::createAdjancencyList(DependenceGraph* dependenceGraph)
{
    OPS_ASSERT(dependenceGraph != nullptr);
    adjList = AdjacencyList(m_Nodes.size());

    // создаем хеш для быстрого поиска вершины по выражению Reprise
    // все вершины должны иметь только одно сопоставленное вхождение (see getIRNode)
    std::unordered_map<const ExpressionBase*, CalculationGraphNode*> hash;
    hash.reserve(m_Nodes.size());
    for (CalculationGraphNode *node: m_Nodes)
        hash.insert(make_pair((const ExpressionBase*)node->m_Data.front(), node));

    for (auto it: dependenceGraph->getAllArcs())
    {
        auto start = hash.find(it->getStartVertex().getSourceOccurrence()->getSourceExpression());
        auto end = hash.find(it->getEndVertex().getSourceOccurrence()->getSourceExpression());
        if (start == hash.end() || end == hash.end()) // Дуга не принадлежит телу цикла
            continue;

        const int from = (*start).second->getNodeNumber()-1;
        const int to = (*end).second->getNodeNumber()-1;
        Edge& edge = adjList.addEdge(from, to, it.get());
        if (it->getDependenceType() == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
            recountAfterTrueDep(edge);

        if (edge.dist == 0 && !isArrayAccess(m_Nodes[edge.to]->getIRNode()))
            if (getExprNum(m_Nodes[from]->m_Data.front()) >= getExprNum(m_Nodes[edge.to]->m_Data.front()))
                edge.dist = 1;

        if (edge.dist < -1)
            throw DependenceProcessingError("unsupported dependence");
    }
}

bool CalculationGraphBuilder::analyzeDependencies(const ForStatement* Loop, DependenceGraph* depGraph)
{
    OPS_ASSERT(Loop != nullptr);
    OPS_ASSERT(depGraph != nullptr);

    numerateNodes();
    try {
        createAdjancencyList(depGraph);

        for (int cur=0; cur < (int)m_Nodes.size(); ++cur)
            processVertex(cur, &CalculationGraphBuilder::processTrueEdge, &Node::trueList);
        for (int cur=0; cur < (int)m_Nodes.size(); ++cur)
            processVertex(cur, &CalculationGraphBuilder::processOutputEdge, &Node::outList);
        for (int cur=0; cur < (int)m_Nodes.size(); ++cur)
            processVertex(cur, &CalculationGraphBuilder::processAntiEdge, &Node::antiList);
        for (int cur=0; cur < (int)m_Nodes.size(); ++cur)
            processVertex(cur, &CalculationGraphBuilder::processInputEdge, &Node::inList);
    }
    catch (DependenceProcessingError error)
    {
        return false;
    }

    // После выполнения алгоритма могли остаться неиспользуемые узлы
    deleteRedundantNodes();
    return true;
}

void CalculationGraphBuilder::processVertex(const int vertex, void (CalculationGraphBuilder::*foo)(int from, Edge& edge), std::vector<Edge> Node::* list)
{
    OPS_ASSERT(foo != nullptr && list != nullptr);

    for (Edge& edge: adjList[vertex].*list)
        (this->*foo)(vertex, edge);
}

void CalculationGraphBuilder::processInputEdge(const int from, Edge& edge)
{
    OPS_ASSERT(edge.arc->getDependenceType() == DependenceGraphAbstractArc::DT_ININ_DEPENDENCE);
    OPS_ASSERT(edge.dist >= -1);

    // Из вершины инициализации всегда должна исходить хотя бы одна дуга инициализации с 0 расстоянием.
    // Если склеем вершину учавствующую в вычисениях, и вершину, используемую лишь для инициализации,
    // и при расстоянии м\у ними >0, это условие нарушится
    if (adjList[from].isInit != adjList[edge.to].isInit && edge.dist > 0)
        return;

    if (from != edge.to)
        glueNodesAfterInInDependency(from, edge.to, edge.dist);
}

void CalculationGraphBuilder::processTrueEdge(const int from, Edge& edge)
{
    OPS_ASSERT(edge.dist >= 0);
    OPS_ASSERT(edge.arc->getDependenceType() == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE);

    if (edge.dist == adjList[edge.to].minInDepDist && adjList[edge.to].inDepNodeNum == getExprNum(m_Nodes[from]->m_Data.front()))
    {
        if (!glueNodesAfterFlowDependency(from, edge.to, edge.dist)) // склейка + init дуги
            throw DependenceProcessingError("incorrect glue of true dependence");
        adjList[edge.to].isInit = true;
        adjList.deleteAntidependences(edge.to);
    }
}

void CalculationGraphBuilder::processOutputEdge(const int from, Edge& edge)
{
    OPS_ASSERT(edge.dist >= 0);
    OPS_ASSERT(edge.arc->getDependenceType() == DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE);

    if (from != edge.to)
        addDirEdge(m_Nodes[from], m_Nodes[edge.to], edge.dist, CalculationGraphDirEdge::ST_CONTROL_DELAY);
}


void CalculationGraphBuilder::processAntiEdge(const int from, Edge& edge)
{
    OPS_ASSERT(edge.arc->getDependenceType() == DependenceGraphAbstractArc::DT_ANTIDEPENDENCE);
    if (from != edge.to)
        addDirEdge(m_Nodes[from], m_Nodes[edge.to], edge.dist, CalculationGraphDirEdge::ST_CONTROL_DELAY);
}


};
