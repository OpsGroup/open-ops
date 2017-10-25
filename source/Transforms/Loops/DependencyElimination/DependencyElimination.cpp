#include <algorithm>
#include <unordered_map>
#include <stack>

#include "OPS_Core/Exceptions.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"
#include "Transforms/Loops/DependencyElimination/DependencyElimination.h"
#include "OPS_Core/msc_leakcheck.h" //контроль утечек памяти должен находиться в конце всех include !!!

using namespace OPS::Reprise;
using namespace OPS::Montego;
using namespace std;


// ================================================
//          AdjacencyList
// ================================================

DependencyElimination::Edge* DependencyElimination::AdjacencyList::addEdge(int from, int to, DependenceGraphAbstractArc* arc)
{
    std::vector<Edge>* list = nullptr;
    const int type = arc->getDependenceType();

    switch (type)
    {
        case DependenceGraphAbstractArc::DT_OUTPUT_DEPENDENCE:
            list = &(*this)[from].outList;
            break;
        case DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE:
            list = &(*this)[from].trueList;
            break;
        default:
            break;
    }
    if (list == nullptr)
        return nullptr;


    list->push_back(Edge(to, CalculationGraphSpace::calculateDependenceDistance(arc)));
    if (type == DependenceGraphAbstractArc::DT_TRUE_DEPENDENCE)
        (*this)[to].inFlowCnt += list->back().dist == 0;
    return &list->back();
}

void DependencyElimination::AdjacencyList::dfs(const int from, vector<bool>& used, vector<int>& ans)
{
	used[from] = true;

    for (Edge edge: (*this)[from].outList)
        if (!used[edge.to] && edge.dist == 0)
            dfs(edge.to, used, ans);
	ans.push_back (from);
}



vector<int> DependencyElimination::AdjacencyList::topologicalSort()
{
    const int n = (int)this->size();
    vector<bool> used(n, false);
    vector<int> ans;
    ans.reserve(n);

    for (int i=0; i<n; ++i)
		if (!used[i])
			dfs (i, used, ans);

    reverse(ans.begin(), ans.end());
    return ans;
}

void DependencyElimination::AdjacencyList::clearGenerator(int num)
{
    EdgeList& edges = (*this)[num];
    edges.outList.clear();
    edges.trueList.clear();
}


// ================================================
//          class DependencyElimination
// ================================================

DependencyElimination::DependencyElimination(ForStatement* loop): loop(loop)
{
    OPS_ASSERT(loop != nullptr);

    std::unique_ptr<DependenceGraph> dependenceGraph(new DependenceGraph(*loop));
    dependenceGraph->removeCounterArcs();
	dependenceGraph->refineAllArcsWithLatticeGraph();

    createAdjacencyList(dependenceGraph.get());
}

void DependencyElimination::createAdjacencyList(OPS::Montego::DependenceGraph* dependenceGraph)
{
    OPS_ASSERT(dependenceGraph != nullptr);
    auto arcs = dependenceGraph->getAllArcs();

    unordered_map<RepriseBase*, int> hash; // сопостовление узлу AST индекса массива
    const int invalidIndex = -1;
    for (auto it: arcs)
    {
        hash.insert(make_pair(it->getStartVertex().getSourceOccurrence()->getSourceExpression(), invalidIndex));
        hash.insert(make_pair(it->getEndVertex().getSourceOccurrence()->getSourceExpression(), invalidIndex));
    }

    adjList.clear();
    adjList.reserve(hash.size());

    // нумерация
    for (pair<RepriseBase* const, int>& it: hash)
    {
        adjList.push_back(EdgeList(it.first));
        it.second = (int)adjList.size()-1;
    }

    // добавляем дуги
    for (auto it: arcs)
    {
        const int from = hash[it->getStartVertex().getSourceOccurrence()->getSourceExpression()];
        const int to = hash[it->getEndVertex().getSourceOccurrence()->getSourceExpression()];
        adjList.addEdge(from, to, it.get());
    }
}

bool DependencyElimination::removeLoopIndependentOutputDependencies()
{
    OPS_ASSERT(loop != nullptr);

    vector<int> topologicalOrder = adjList.topologicalSort();
    for (int curNode: topologicalOrder)
        if (!isNodeDeleted(curNode) && !adjList[curNode].outList.empty())
            removeNodeLoopIndependentOutputDeps(curNode);

    return true;
}

void DependencyElimination::removeNodeLoopIndependentOutputDeps(const int nodeNum)
{
    OPS_ASSERT(!isNodeDeleted(nodeNum));
    ExpressionBase* generator = adjList[nodeNum].node->cast_ptr<ExpressionBase>();
    ExpressionBase* usage = getUsageFromGenerator(generator);

    const int edgeCnt = recountTrueEdgesForSubstitution(nodeNum);
    const bool deleteOldAssign = haveTrivialOutputEdge(nodeNum);

    if (generator == nullptr || !CalculationGraphSpace::isArrayAccess(generator) || isNestedAssignment(generator))
        return;

    if (edgeCnt != 0)
    {
        const bool sideEffects = Editing::hasSideEffects(*usage);
        ExpressionBase* valueForSubstitution = (edgeCnt == 1 && deleteOldAssign && !sideEffects)
            ? usage
            : extractTempVariable(usage, deleteOldAssign);
        substituteAfterLoopIndependentTrueDep(nodeNum, valueForSubstitution);
    }

    if (deleteOldAssign) // удаляем изначальное присваивание
    {
        deleteExpression(generator);
        adjList[nodeNum].node = nullptr;
    }
}

ReferenceExpression* DependencyElimination::extractTempVariable(ExpressionBase* usage, const bool toDelete)
{
    VariableDeclaration& extractedVar = Editing::createNewVariable(*usage->getResultType().get(), loop->getBody());
    ReferenceExpression* refToNewVar = new ReferenceExpression(extractedVar);
    RepriseBase* assign = usage->getParent();

    // вставляем в дерево перед исходным выражением
    StatementBase* statement = usage->obtainParentStatement();
    BlockStatement* block = &statement->getParentBlock();
    StatementBase* stBase = composeAssignStatement(refToNewVar, usage);
    block->addBefore(block->convertToIterator(statement), stBase);

    if (!toDelete)
        Editing::replaceExpression(assign->getChild(1).cast_to<ExpressionBase>(), ReprisePtr<ExpressionBase>(refToNewVar->clone()));
    return refToNewVar;
}

void DependencyElimination::substituteAfterLoopIndependentTrueDep(const int srcNode, ExpressionBase* newNode)
{
    for (Edge& edge: adjList[srcNode].trueList)
    {
        if (edge.dist != 0)
            continue;
        OPS_ASSERT(edge.to != srcNode); // true edge не может быть петлей
        const int trivialInTrueCnt = adjList[edge.to].inFlowCnt;
        if (trivialInTrueCnt != 0 || isNodeDeleted(edge.to))
            continue;

        ExpressionBase* dst = adjList[edge.to].node->cast_ptr<ExpressionBase>();
        Editing::replaceExpression(*dst, ReprisePtr<ExpressionBase>(newNode->clone()));
        adjList[edge.to].node = nullptr;
    }
}


// ================================================
//          helper methods
// ================================================

bool DependencyElimination::haveTrivialOutputEdge(const int node)
{
    OPS_ASSERT(node >= 0 && node < (int)adjList.size());

    for (Edge& it: adjList[node].outList)
    if (it.dist == 0)
        return true;
    return false;
}

int DependencyElimination::recountTrueEdgesForSubstitution(const int node)
{
    OPS_ASSERT(node >= 0 && node < (int)adjList.size());

    int edgesToSubstitute = 0;
    for (Edge& edge: adjList[node].trueList)
        if (edge.dist == 0 && adjList[edge.to].inFlowCnt-- == 1)
            ++edgesToSubstitute;

    return edgesToSubstitute;
}

bool DependencyElimination::isNestedAssignment(ExpressionBase* generator)
{
    return generator->getParent()->getParent()->is_a<ExpressionBase>();
}

void DependencyElimination::deleteExpression(ExpressionBase* expr)
{
    StatementBase* statement = expr->obtainParentStatement();
    BlockStatement* block = &statement->getParentBlock();

    block->erase(block->convertToIterator(statement));
}

bool DependencyElimination::isNodeDeleted(const int nodeNum)
{
    return adjList[nodeNum].node == nullptr;
}

ExpressionStatement* DependencyElimination::composeAssignStatement(ReferenceExpression* ref, ExpressionBase* value)
{
    BasicCallExpression* callExpr = new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, ref, value);
    return new ExpressionStatement(callExpr);
}

ExpressionBase* DependencyElimination::getUsageFromGenerator(ExpressionBase* generator)
{
    BasicCallExpression* baseAssign = generator->getParent()->cast_ptr<BasicCallExpression>();
    OPS_ASSERT(baseAssign != nullptr && baseAssign->getKind() == BasicCallExpression::BCK_ASSIGN);

    ExpressionBase* usage = baseAssign->getArgument(1).cast_ptr<ExpressionBase>();
    OPS_ASSERT(usage != nullptr);

    return usage;
}

