#ifndef OPS_LOOPS_DEPENDENCIES_ELIMINATION_H_
#define OPS_LOOPS_DEPENDENCIES_ELIMINATION_H_

#include <unordered_map>
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "Analysis/CalculationGraph/ExpressionGraph.h"
#include "Analysis/CalculationGraph/CalculationGraphBase.h"


class DependencyElimination
{
public:
    DependencyElimination(OPS::Reprise::ForStatement* loop);
    bool removeLoopIndependentOutputDependencies();

private:
    struct Edge
    {
        int to, dist;
        Edge(int num, int dist): to(num), dist(dist) {}
    };

    struct EdgeList
    {
        OPS::Reprise::RepriseBase* node;
        int inFlowCnt; // кол-во входящих дуг с 0 расстоянием

        EdgeList(OPS::Reprise::RepriseBase* node): node(node), inFlowCnt(0) {} ;
        std::vector<Edge> outList, trueList;
    };

    struct AdjacencyList: std::vector<EdgeList>
    {
        AdjacencyList() {};

        Edge* addEdge(int from, int to, OPS::Montego::DependenceGraphAbstractArc* edge);
        void deleteAntidependences(int from);
        void clearGenerator(int num);

        /// Возвращает вектор номеров вершин графа с правильной (топологической) нумерацией
        std::vector<int> topologicalSort();
        void dfs(const int v, std::vector<bool>& used, std::vector<int>& ans);
    };

    void createAdjacencyList(OPS::Montego::DependenceGraph* depGraph);
    void removeNodeLoopIndependentOutputDeps(const int from);
    void substituteAfterLoopIndependentTrueDep(const int from, OPS::Reprise::ExpressionBase* newNode);

    bool haveTrivialOutputEdge(const int node);
    int recountTrueEdgesForSubstitution(const int node);
    bool isNodeDeleted(const int nodeNum);

    // возвращает указатель на уже встроенное в AST выражение. Перед повторной подстановкой копировать.
    OPS::Reprise::ReferenceExpression* extractTempVariable(OPS::Reprise::ExpressionBase* expr, const bool toDelete);
    static bool isNestedAssignment(OPS::Reprise::ExpressionBase* generator);
    static void deleteExpression(OPS::Reprise::ExpressionBase* expr);
    static OPS::Reprise::ExpressionStatement* composeAssignStatement(OPS::Reprise::ReferenceExpression* ref, OPS::Reprise::ExpressionBase* value);
    static OPS::Reprise::ExpressionBase* getUsageFromGenerator(OPS::Reprise::ExpressionBase* generator);

    // данные члены
    AdjacencyList adjList;
    OPS::Reprise::ForStatement* loop;
};



#endif // OPS_LOOPS_DEPENDENCIES_ELIMINATION_H_
