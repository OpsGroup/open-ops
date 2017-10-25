#ifndef OPS_CALCGRAPH_BUILDER_
#define OPS_CALCGRAPH_BUILDER_

#include <vector>
#include <limits.h>
#include <unordered_map>

#include "Analysis/CalculationGraph/CalculationGraphBase.h"
#include "Analysis/CalculationGraph/CalculationGraph.h"



namespace CalculationGraphSpace
{


class CalculationGraphBuilder: public ExpressionsGraph<CalculationGraphNode, CalculationGraphDirEdge>
{
public:

    explicit CalculationGraphBuilder(CalculationGraph::EN_GraphCreatingMode GraphType, const OPS::PlatformSettingsSpace::PlatformSettings* settings, EN_NodeType DefaultNodeType = NT_PIPELINED);
    bool createGraph(OPS::Reprise::ForStatement* Loop);

    /**
		\brief По деревьям выражений строится граф вычислений без дуг зависимостей (вспомогательная)
		\param GivenBlock - блок, для которого строиться граф
		\param Generators - список генераторов в этом блоке (необходимо при рекурсивных вызовах для If)
	*/
	bool initializeByStatements(OPS::Reprise::BlockStatement* GivenBlock, std::list<CalculationGraphNode*>& Generators);

    /// Анализирует граф зависимостей Лампорта и добавляет дуги (истинная зависимость)
	/// или склеивает вершины (входная зависимость).
    bool analyzeDependencies(const OPS::Reprise::ForStatement* Loop, OPS::Montego::DependenceGraph* depGraph);

    /// Добавляет дугу в граф вычислений
	CalculationGraphDirEdge* addDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistance = 0, CalculationGraphDirEdge::EN_SignalType SignalType = CalculationGraphDirEdge::ST_DATAFLOW);

	/// Определяет по Data тип операции, а следовательно и задержку
	int getNodeOperationDelayByExpressionBase(OPS::Reprise::ExpressionBase* Data) const;


private:
    struct Edge
    {
        OPS::Montego::DependenceGraphAbstractArc* arc;
        int to, dist;

        Edge(int to, OPS::Montego::DependenceGraphAbstractArc* arc): to(to), arc(arc), dist(calculateDependenceDistance(arc)) {}
    };

    struct Node
    {
        int minInDepDist, inDepNodeNum;
        bool isInit; // вершина служит лишь для инициализации
        Node(): minInDepDist(INT_MAX), isInit(false), inDepNodeNum(-1) {}

        std::vector<Edge> inList, outList, trueList, antiList;
    };

    struct AdjacencyList: std::vector<Node>
    {
        AdjacencyList(size_t size): std::vector<Node> (size) {};
        AdjacencyList() {};

        Edge& addEdge(int from, int to, OPS::Montego::DependenceGraphAbstractArc* edge);
        void deleteAntidependences(int from);
    };

    void recountAfterTrueDep(Edge& edge);
    int getExprNum(OPS::Reprise::RepriseBase* expr);
    std::unordered_map<OPS::Reprise::ExpressionStatement*, int> exprOrder; // костыль для распознавания цикл. порожденных зависимостей.
    int lastOrderNum;

    void createAdjancencyList(OPS::Montego::DependenceGraph* gr);

    /// Удалить все вершины с нулевой степенью (портит нумерацию, see numerateNodes)
    void deleteRedundantNodes();

    /// Удаляет все дуги зависимой вершины и добавляет к исходной вершине с перерасчетом дистанции
    void glueNodesAfterInInDependency(int from, int to, int dist);

    /// Склеивает вершины по истинной зависимости, подменяя вхождения второй вершины первой
    bool glueNodesAfterFlowDependency(int from, int to, const int dist);

    void processVertex(const int vertex, void (CalculationGraphBuilder::*foo)(int from, Edge& edge), std::vector<Edge> Node::* list);
    void processInputEdge(const int from, Edge& edge);
    void processTrueEdge(const int from, Edge& edge);
    void processOutputEdge(const int from, Edge& edge);
    void processAntiEdge(const int from, Edge& edge);

    const OPS::PlatformSettingsSpace::PlatformSettings* m_PlatformSettings;

    AdjacencyList adjList;
    friend CalculationGraph;
};



};

#endif //_CALCGRAPH_BUILDER_
