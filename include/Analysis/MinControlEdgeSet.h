// Нахождение минимального множества контрольных дуг
#ifndef MIN_CONTROL_EDGES
#define MIN_CONTROL_EDGES

#include "Reprise/Reprise.h"
#include "Analysis/ControlFlowGraph.h"

typedef std::pair<const OPS::Reprise::StatementBase*, const OPS::Reprise::StatementBase*> Edge;
typedef std::vector<Edge> EdgeVector;
typedef std::map<Edge, int> EdgesMarks;
typedef std::map<int, Edge> MarksEdges;
typedef std::map<StatementBase*, int> VertexMarks;

EdgeVector FindMinControlEdgeSet(const OPS::Reprise::BlockStatement& aBlock);
EdgeVector FindMinControlEdgeSet(const ControlFlowGraph& aGraph);
void BuildMinEdgeSet(const ControlFlowGraph& aGraph, EdgeVector& minSet);
void CopyGraph(const ControlFlowGraph& lGraph, ControlFlowGraph::StatementGraph& rGraph);
void BuildReverseGraph(const ControlFlowGraph::StatementGraph& lGraph, ControlFlowGraph::StatementGraph& rGraph);
int GraphEdgesCount(const ControlFlowGraph::StatementGraph& g);
void CreateMarks(const ControlFlowGraph::StatementGraph& g, EdgesMarks& marks, EdgesMarks& wasSeen, VertexMarks& X1, VertexMarks& X2);
void ClearMarks(VertexMarks& X1, VertexMarks& X2);
void DeleteEdge(ControlFlowGraph::StatementGraph& g, const Edge& e);
void AddEdge(ControlFlowGraph::StatementGraph& g, const Edge& e);
void DepthFirstReach(ControlFlowGraph::StatementGraph& g, const OPS::Reprise::StatementBase* source, VertexMarks& X);
int DegPlus(ControlFlowGraph::StatementGraph& aGraph, const OPS::Reprise::StatementBase* pStatement);
int DegMinus(ControlFlowGraph::StatementGraph& aGraph, const OPS::Reprise::StatementBase* pStatement);
void ClearGraph(ControlFlowGraph::StatementGraph& g);
void DeleteRootBlock(ControlFlowGraph::StatementGraph& aGraph, const StatementBase* pStatement);
void markCoveredStatements(EdgeVector& coveredEdges, EdgeVector& unCoveredEdges, const OPS::Reprise::BlockStatement& aBlock);
#endif //MIN_CONTROL_EDGES
