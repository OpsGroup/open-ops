#include "Analysis/MinControlEdgeSet.h"
#include <iterator>

using namespace OPS::Reprise;

EdgeVector FindMinControlEdgeSet(const BlockStatement& aBlock)
{
  EdgeVector minControlEdgeSet;
  minControlEdgeSet.reserve(50);
  ControlFlowGraph blockGraph(aBlock);
  BuildMinEdgeSet(blockGraph, minControlEdgeSet);
  return minControlEdgeSet;
}

void markAllCoveredEdges(EdgeVector& coveredEdges, EdgeVector& unCoveredEdges, ControlFlowGraph& aGraph);

void markCoveredStatements(EdgeVector& coveredEdges, EdgeVector& unCoveredEdges, const OPS::Reprise::BlockStatement& aBlock)
{
	ControlFlowGraph blockGraph(aBlock);
	markAllCoveredEdges(coveredEdges, unCoveredEdges, blockGraph);
}

EdgeVector FindMinControlEdgeSet(const ControlFlowGraph& aGraph)
{
	EdgeVector minControlEdgeSet;
	minControlEdgeSet.reserve(50);
	BuildMinEdgeSet(aGraph, minControlEdgeSet);
	return minControlEdgeSet;
}

void DeleteStatement(ControlFlowGraph::StatementGraph& g, StatementBase* root)
{
	if(g.find(root) != g.end()) {
		delete g[root];
		g[root] = NULL;
		ControlFlowGraph::StatementGraph::iterator it = g.find(root);
		g.erase(it);
	}
	ControlFlowGraph::StatementGraph::iterator it = g.begin();
	for(; it!=g.end(); ++it)
	{
		ControlFlowGraph::StatementList::iterator jt = it->second->begin();
		while(jt != it->second->end())
		{
			if(*jt == root)
			{
				ControlFlowGraph::StatementList::iterator xt = jt;
				++jt;
				it->second->erase(xt);
			} else { ++jt; }
		}
	}
	//bool hasGaps = true;
	/*while(hasGaps)
	{
		ControlFlowGraph::StatementGraph::iterator it = g.begin();
		while(it != g.end() && (!it->second->empty()))
		{
			++it;
		}
		if(it != g.end()) {
			ControlFlowGraph::StatementGraph::iterator xt = g.find(it->first);
			g.erase(xt);
		} else hasGaps = false;
	}*/
}

void RemoveDuplicate(ControlFlowGraph::StatementGraph& g)
{
	ControlFlowGraph::StatementGraph::iterator it = g.begin();
	for(; it!=g.end(); ++it)
	{
		if(it->second != NULL && ! it->second->empty()) {
			ControlFlowGraph::StatementList::iterator last = std::unique(it->second->begin(), it->second->end());
			it->second->erase(last, it->second->end());
		}
	}
}

void ClearGraph(ControlFlowGraph::StatementGraph& g)
{
	// удалим все дуги, связанные с 0 - они не нужны
	DeleteStatement(g, NULL);
	RemoveDuplicate(g);
}

void BuildMinEdgeSet(const ControlFlowGraph& aGraph, EdgeVector& minSet)
{
	ControlFlowGraph::StatementGraph g;
	// скопировать aGraph в g, из g будут по ходу работы временно удаляться ребра
	CopyGraph(aGraph, g);
	DeleteRootBlock(g, &aGraph.rootBlock());
	ClearGraph(g);
	ControlFlowGraph::StatementGraph r;
	// построим граф с обращенными дугами
	BuildReverseGraph(g, r);
	// нужно найти источник и сток или получить сверху
	// источник
	const StatementBase* source = NULL;
	// источник - первый оператор блока, по которому строится граф (блок должен быть телом процедуры)
	source = &(*(const_cast<Reprise::BlockStatement*>(&aGraph.rootBlock())->getFirst()));
	//// источник - блок, по которому строится граф
	////source = &aGraph.rootBlock();
	// сток
	// за сток возьмем последний оператор блока
	const StatementBase* target = &(*(const_cast<Reprise::BlockStatement*>(&aGraph.rootBlock())->getLast()));
	//// за сток возьмем тот же блок - лучше не стоит
	////const StatementBase* target = &aGraph.rootBlock();
	int edgesCount = GraphEdgesCount(g);
	//int reverseEdgesCount = GraphEdgesCount(r);
	// создадим массив пометок ребер графа
	EdgesMarks marks;
	EdgesMarks wasSeen;
	VertexMarks X1;
	VertexMarks X2;
	//все дуги не просмотрены
	//пометить все дуги 0, все вершины 0
	CreateMarks(g, marks, wasSeen, X1, X2);
	int m = 0;
	for(int i = 0; i < edgesCount; ++i)
	{	
		//найти дугу w с максимальной меткой среди еще не просмотренных
	    int maxMark = 0; // меток меньше этой быть не может
		EdgesMarks::iterator pMaxEdge = marks.begin();
		for(EdgesMarks::iterator it = marks.begin(); it!=marks.end(); ++it)
		{
			if(wasSeen[it->first]==0) { // ищем максимальную среди не просмотренных
				if(it->second > maxMark)  // ненавижу условия в стиле С!!!
				{
					maxMark = it->second;
					pMaxEdge = it;
				}
			}
		}
		bool ended = (wasSeen[pMaxEdge->first] !=0);
		if(ended) break;
		++m;
		//удалить w из g и из r
		Edge deletedEdge(pMaxEdge->first);
		Edge reverseEdge(deletedEdge.second, deletedEdge.first);
		DeleteEdge(g, deletedEdge);
		DeleteEdge(r, reverseEdge);		
		//найти X1(w), X2(w)
		// пометим все вершины, достижимые из источника, пометки храним в X1
		DepthFirstReach(g, source, X1);
		// пометим все вершины, из которых достижим сток (то есть которые достижимы из стока на обратном графе)
		// пометки храним в X2
		DepthFirstReach(r, target, X2);
		//найти v: v<w, пометить их меткой m
		bool wasAnyMarked = false;
		bool wasOnlySeenPoorMarked = true;
		EdgesMarks::iterator wt = wasSeen.begin();
		for(EdgesMarks::iterator jt = marks.begin(); jt!=marks.end(); ++jt, ++wt)
		{
			if(jt->first.first == deletedEdge.first && jt->first.second == deletedEdge.second) continue;
			StatementBase* s1 = const_cast<StatementBase*>(jt->first.first);
			bool isFromX1 = (X1[s1]==0 && s1 != source);
			StatementBase* s2 = const_cast<StatementBase*>(jt->first.second);
			bool isFromX2 = (X2[s2]==0 && s2 != target);
			bool isLess = isFromX1 || isFromX2;
			if(isLess) {
				bool wasSeen = (wt->second != 0);
				jt->second = m;
				wasAnyMarked = true;
				EdgeVector::iterator edgeS = std::find(minSet.begin(), minSet.end(), jt->first);
				bool isFromSolution = (edgeS != minSet.end());
				if (isFromSolution || !wasSeen) wasOnlySeenPoorMarked = false;
			}
			
		}
		//если w нужно добавить в minSet, то добавить
		if(!wasAnyMarked || wasOnlySeenPoorMarked) 
		{
			minSet.push_back(deletedEdge);
		}
		//вернуть w в g и обратную - в r
		AddEdge(g, deletedEdge);
		AddEdge(r, reverseEdge);
		// очистим метки X1 и X2
		ClearMarks(X1,X2);
		wasSeen[deletedEdge] = 1;
	}
}

void markAllCoveredEdges(EdgeVector& coveredEdges, EdgeVector& uncoveredEdges, ControlFlowGraph& aGraph)
{
	ControlFlowGraph::StatementGraph g;
	// скопировать aGraph в g, из g будут по ходу работы временно удаляться ребра
	CopyGraph(aGraph, g);
	DeleteRootBlock(g, &aGraph.rootBlock());
	ClearGraph(g);
	ControlFlowGraph::StatementGraph r;
	// построим граф с обращенными дугами
	BuildReverseGraph(g, r);
	// нужно найти источник и сток или получить сверху
	// источник
	const StatementBase* source = NULL;
	// источник - первый оператор блока, по которому строится граф (блок должен быть телом процедуры)
	source = &(*(const_cast<Reprise::BlockStatement*>(&aGraph.rootBlock())->getFirst()));
	const_cast<StatementBase*>(source)->setNote("Covered", Note::newBool(true));
	//// источник - блок, по которому строится граф
	////source = &aGraph.rootBlock();
	// сток
	// за сток возьмем последний оператор блока
	const StatementBase* target = &(*(const_cast<Reprise::BlockStatement*>(&aGraph.rootBlock())->getLast()));
	const_cast<StatementBase*>(target)->setNote("Covered", Note::newBool(true));
	//// за сток возьмем тот же блок - лучше не стоит
	////const StatementBase* target = &aGraph.rootBlock();
	int edgesCount = coveredEdges.size();
	OPS_UNUSED(edgesCount);
	//int reverseEdgesCount = GraphEdgesCount(r);
	// создадим массив пометок ребер графа
	EdgesMarks marks;
	EdgesMarks wasSeen;
	VertexMarks X1;
	VertexMarks X2;
	//все дуги не просмотрены
	//пометить все дуги 0, все вершины 0
	CreateMarks(g, marks, wasSeen, X1, X2);
	// TODO: сначала удаляем из графа все непокрытые дуги 
	// минимального множества контрольных дуг
	// затем для каждой вершины графа проверяем,
	// достижима ли она из источника и достижим ли из нее сток
	// если да, помечаем ее как пройденную
	for(size_t i = 0; i < uncoveredEdges.size(); ++i)
	{
		Edge edgeToDelete(uncoveredEdges[i].first, uncoveredEdges[i].second);
		Edge reverseEdgeToDelete(edgeToDelete.second, edgeToDelete.first);
		DeleteEdge(g, edgeToDelete);
		DeleteEdge(r, reverseEdgeToDelete);
	}

	//for(int i = 0; i < edgesCount; ++i)
	//{	
		//Edge deletedEdge(coveredEdges[i]);
		//Edge reverseEdge(deletedEdge.second, deletedEdge.first);
		//DeleteEdge(g, deletedEdge);
		//DeleteEdge(r, reverseEdge);		
		//найти X1(w), X2(w)
		// пометим все вершины, достижимые из источника, пометки храним в X1
		DepthFirstReach(g, source, X1);
		// пометим все вершины, из которых достижим сток (то есть которые достижимы из стока на обратном графе)
		// пометки храним в X2
		DepthFirstReach(r, target, X2);
		//найти v: v<w, пометить их меткой m
		// теперь найдем среди всех дуг графа те, которые "меньше" w
		ControlFlowGraph::StatementGraph::iterator it = g.begin();
		//EdgesMarks::iterator wt = wasSeen.begin();
		// для каждой вершины графа
		for(; it != g.end(); ++it)
		{
			//ControlFlowGraph::StatementList::iterator jt = it->second->begin();
			//for(; jt != it->second->end(); ++jt)
			//{
				StatementBase* edgeSource =  const_cast<StatementBase*>(it->first);
				//StatementBase* edgeDestination = const_cast<StatementBase*>(*jt);
				//if(edgeSource == deletedEdge.first && edgeDestination == deletedEdge.second) continue;
				bool isFromX1 = (X1[edgeSource]==1 && edgeSource != source);
				bool isFromX2 = (X2[edgeSource]==1 && edgeSource != target);
				if(isFromX1 && isFromX2)
				{
					edgeSource->setNote("Covered", Note::newBool(true));
				}
			//}
		}
		//вернуть w в g и обратную - в r
		//AddEdge(g, deletedEdge);
		//AddEdge(r, reverseEdge);
		// очистим метки X1 и X2
		ClearMarks(X1,X2);
	//}
}


// копирует полностью lGraph в rGraph, rGraph создается новый
void CopyGraph(const ControlFlowGraph& lGraph, ControlFlowGraph::StatementGraph& rGraph)
{
	const ControlFlowGraph::StatementVector& v = lGraph.getStatementVector();
	for(ControlFlowGraph::StatementVector::const_iterator it = v.begin(); it!=v.end(); ++it)
	{
		const ControlFlowGraph::StatementList& l = lGraph[*it];
		rGraph[*it] = new ControlFlowGraph::StatementList(l.begin(), l.end());
	}
}

//строит в rGraph граф lGraph с обращенными дугами
void BuildReverseGraph(const ControlFlowGraph::StatementGraph& lGraph, ControlFlowGraph::StatementGraph& rGraph)
{
	ControlFlowGraph::StatementGraph::const_iterator it = lGraph.begin();
	for(; it!=lGraph.end(); ++it)
	{
		const StatementBase* pStatement = it->first;
		const ControlFlowGraph::StatementList* pList = it->second;
		rGraph[it->first];
		// пройдемся по всему списку *pList и добавим в rGraph дуги из узлов списка в *pStatement
		ControlFlowGraph::StatementList::const_iterator jt = pList->begin();
		for(; jt!=pList->end(); ++jt)
		{
			if(rGraph[*jt] == NULL) { 
				rGraph[*jt] = new ControlFlowGraph::StatementList(); 
			}
			rGraph[*jt]->push_back(pStatement);
		}
	}
}

// возвращает число дуг графа
int GraphEdgesCount(const ControlFlowGraph::StatementGraph& g)
{
	int result = 0;
	ControlFlowGraph::StatementGraph::const_iterator it = g.begin();
	for(; it!=g.end(); ++it)
	{
		if((it->second!=NULL) && (!it->second->empty())) {
			ControlFlowGraph::StatementList::const_iterator jt = it->second->begin();
			for(; jt!=it->second->end(); ++jt)
			{
				result++;
			}
		}
	}
	return result;
}

//инициализация пометок
//каждой дуге графа g присваивается метка 0
void CreateMarks(const ControlFlowGraph::StatementGraph& g, EdgesMarks& marks, EdgesMarks& wasSeen, VertexMarks& X1, VertexMarks& X2)
{
	ControlFlowGraph::StatementGraph::const_iterator it = g.begin();
	for(; it!=g.end(); ++it)
	{
		StatementBase* pStatement = const_cast<StatementBase*>(it->first);
		const ControlFlowGraph::StatementList* pList = it->second;
		X1[pStatement] = 0;
		X2[pStatement] = 0;
		ControlFlowGraph::StatementList::const_iterator jt = pList->begin();
		for(; jt!=pList->end(); ++jt)
		{
			Edge e(pStatement, *jt);
			marks[e] = 0;
			wasSeen[e] = 0;
		}
	}
}

void ClearMarks(VertexMarks& X1, VertexMarks& X2)
{
	VertexMarks::iterator it = X1.begin();
	VertexMarks::iterator jt = X2.begin();
	for(; it!=X1.end(); ++it, ++jt)
	{
		it->second = 0;
		jt->second = 0;
	}
}

//удаляет ребро e из графа g, если оно в нем есть
void DeleteEdge(ControlFlowGraph::StatementGraph& g, const Edge& e)
{
	const StatementBase* pStatement = e.first;
	ControlFlowGraph::StatementList* pList = g[pStatement];
	pList->remove(e.second);
}

//вставляет ребро e в граф g
void AddEdge(ControlFlowGraph::StatementGraph& g, const Edge& e)
{
	g[e.first]->push_back(e.second);
}

//поиск в глубину в графе g начиная с вершины source с расстановкой пометок в X
//(рекурсивная)
void DepthFirstReach(ControlFlowGraph::StatementGraph& g, const StatementBase* source, VertexMarks& X)
{
	if(X[const_cast<StatementBase*>(source)]!=0) return;
	X[const_cast<StatementBase*>(source)] = 1;
	if(source!=NULL) {
		const ControlFlowGraph::StatementList* pList = g[source];
		if(pList!=NULL) {
			ControlFlowGraph::StatementList::const_iterator it = pList->begin();
			for(; it!=pList->end(); ++it)
			{
				DepthFirstReach(g, *it, X);
			}
		}
	}
}

int DegPlus(ControlFlowGraph::StatementGraph& aGraph, const StatementBase* pStatement)
{
	int result = 0;
	ControlFlowGraph::StatementGraph::iterator it = aGraph.begin();
	for(; it!=aGraph.end(); ++it)
	{
		if(it->second != NULL && (! it->second->empty())) {
			ControlFlowGraph::StatementList::iterator jt = it->second->begin();
			for(; jt!=it->second->end(); ++jt)
			{
				if(*jt == pStatement) result ++;
			}
		}
	}
	return result;
}

int DegMinus(ControlFlowGraph::StatementGraph& aGraph, const StatementBase* pStatement)
{
	int result = 0;
	ControlFlowGraph::StatementGraph::iterator it = aGraph.find(pStatement);
	if(it!=aGraph.end()) {
		ControlFlowGraph::StatementList::iterator jt = it->second->begin();
		for(; jt!=it->second->end(); ++jt)
		{
			result++;
		}
	
	}
	return result;
}

////class vertexRemover {
////private:
////	const Reprise::StatementBase* m_pRemoved;
////public:
////	vertexRemover(const Reprise::StatementBase* pStatement): m_pRemoved(pStatement) {}
////	bool operator()(ControlFlowGraph::StatementGraph::value_type& value)
////	{
////		return (value.first == m_pRemoved);
////	}
////};

void DeleteRootBlock(ControlFlowGraph::StatementGraph& aGraph, const StatementBase* pStatement)
{
	// удалим все вершины, соответствующие pStatement и дуги, им инцидентные
	// сначала удалим список смежности для pStatement
	//std::remove_if(aGraph.begin(), aGraph.end(), vertexRemover(pStatement));
	ControlFlowGraph::StatementGraph::iterator pRoot = aGraph.find(pStatement);
	aGraph.erase(pRoot);
	ControlFlowGraph::StatementGraph::iterator it = aGraph.begin();
	for(; it!=aGraph.end(); ++it)
	{
		// удалим все вхождения pStatement в список смежности вершины
		if(it->second != NULL && (! it->second->empty()))
		{
			ControlFlowGraph::StatementList* pNewList = new ControlFlowGraph::StatementList;
			std::back_insert_iterator<ControlFlowGraph::StatementList> dest = std::back_inserter(*pNewList);
			std::remove_copy(it->second->begin(), it->second->end(), dest, pStatement);
			delete it->second;
			it->second = pNewList;
		}
		
	}
}
