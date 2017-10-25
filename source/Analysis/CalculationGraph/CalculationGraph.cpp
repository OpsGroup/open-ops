#ifdef _MSC_VER
#pragma warning(disable : 880)	// Запрещаем сообщение о отсутствии int в определении 
//#pragma warning(disable : 1011)	// Запрещаем сообщение о осутствует return в non-void функции
#pragma warning(disable : 4786)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4503)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4355)	// Запрещаем сообщение о this в конструкторе
#endif


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "Analysis/CalculationGraph/CalculationGraph.h"
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

// *******************************
// *** class CalculationGraph ***
// *******************************

CalculationGraph::CalculationGraph(EN_GraphCreatingMode GraphType, EN_NodeType DefaultNodeType)
    :m_Type(GraphType)
    , m_VertexMatrix(0)
    , m_DirEdgeTypesMatrix(0)
    , m_iii(0)
	,m_PlatformSettings(0)
    , m_StartTacts(0)
{
    m_strGraphName = g_strGraphName;
    m_DefaultNodeType = DefaultNodeType;
}

CalculationGraph::CalculationGraph(const CalculationGraph& Original)
{
    m_strGraphName = g_strGraphName;
    m_DefaultNodeType = Original.m_DefaultNodeType;

	// Копируем простые поля
	m_iii = Original.m_iii; 
	m_Type = Original.m_Type; 
	m_PlatformSettings = Original.m_PlatformSettings; 

	// Копируем поля-списки
	if (Original.m_Nodes.size())
	{
		// Копируем вершины без списков дуг
        for(CalculationGraphNode* iter: Original.m_Nodes)
		{
			CalculationGraphNode* curNode;
			switch (iter->getDataType())
			{
			case CalculationGraphNode::DT_EXPRESSION:
				{
					curNode = new CalculationGraphNode(iter->m_Data.front()->cast_ptr<ExpressionBase>(), iter->m_OperationDelay, iter->m_NodeType);
			
					list<RepriseBase*>::iterator dataIter = iter->m_Data.begin(); 
					for (dataIter++; dataIter != iter->m_Data.end(); ++dataIter) // первый элемент списка уже вставлен с помощью конструктора
						curNode->m_Data.push_back(dynamic_cast<ExpressionBase*>(*dataIter));
					break;
				}
			case CalculationGraphNode::DT_STATEMENT:
				{
					curNode = new CalculationGraphNode(dynamic_cast<IfStatement*>(iter->m_Data.front()), iter->m_OperationDelay);
					break;
				}
			default:
				throw new OPS::ArgumentError("Unknown CalculationGraph node data type!");

			}
			curNode->m_NodeNumber = iter->m_NodeNumber;
			addNode(curNode);
		}

        // Копируем дуги и добавляем их в списки дуг соответсвующих вершин
		for(CalculationGraphNode* iter: Original.m_Nodes)
		{
			list<CalculationGraphDirEdge*>::iterator itDirEdge = iter->m_InDirEdges.begin();
			for(; itDirEdge != iter->m_InDirEdges.end(); ++itDirEdge )
			{
				CalculationGraphNode* head = findNodeByNodeNumber((*itDirEdge )->m_Begin->m_NodeNumber);
				CalculationGraphNode* tail = findNodeByNodeNumber(iter->m_NodeNumber);
				CalculationGraphDirEdge* curDirEdge = addDirEdge(head, tail, (*itDirEdge)->m_DependenceDistance, (*itDirEdge)->m_SignalType);
				curDirEdge->m_DirEdgeType = (*itDirEdge )->m_DirEdgeType;
				curDirEdge->m_BufferLatency = (*itDirEdge )->m_BufferLatency;
				curDirEdge->m_SendingDelay = (*itDirEdge)->m_SendingDelay;
                
                CalculationGraphDirEdge* initEdge = (*itDirEdge)->m_InitializationDirEdge;
                if (initEdge != 0)
				    curDirEdge->m_InitializationDirEdge = findDirEdgeByNodesNumbers(initEdge->m_Begin->m_NodeNumber, initEdge->m_End->m_NodeNumber);
			}
		}

		if (Original.m_VertexMatrix != 0)
		{
			// Выделение памяти. Вершина номер 0 это добавочная вершина
            m_VertexMatrix = new int*[m_Nodes.size()+1];
			for(unsigned i=0; i<=m_Nodes.size(); i++)
			{
                m_VertexMatrix[i] = new int[m_Nodes.size()+1];
			}
			// Копирование
			for(unsigned i=0; i<=m_Nodes.size(); i++)
				for(unsigned j=0; j<=m_Nodes.size(); j++)
					m_VertexMatrix[i][j] = Original.m_VertexMatrix[i][j];
		}

		if (Original.m_DirEdgeTypesMatrix != nullptr)
		{
			// Выделение памяти. Вершина номер 0 это добавочная вершина
            m_DirEdgeTypesMatrix = new int*[m_Nodes.size()+1];
			for(unsigned i=0; i<=m_Nodes.size(); i++)
			{
                m_DirEdgeTypesMatrix[i] = new int[m_Nodes.size()+1];
			}
			// Копирование
			for(size_t i=0; i<=m_Nodes.size(); i++)
				for(unsigned j=0; j<=m_Nodes.size(); j++)
					m_DirEdgeTypesMatrix[i][j] = Original.m_DirEdgeTypesMatrix[i][j];
		}

		if (Original.m_StartTacts != 0)
		{
			// Выделяем память под массив начальных тактов
            m_StartTacts = new int[m_Nodes.size()+1];
			for(size_t i=0; i<=m_Nodes.size(); i++)
				m_StartTacts[i] = Original.m_StartTacts[i];
		}
	}
	
}

// Функция создает граф по циклу for. 
// Результирующий граф пригоден для вывода, но не содержит рассчетной информации,
// которая определяется по готовому графу 
bool CalculationGraph::createGraph(ForStatement* Loop, string PathToConfigurationFile)
{
    bool bRes = loadOperationsDelays(PathToConfigurationFile);
	if (!bRes)
        return false;
    
    CalculationGraphBuilder builder(m_Type, m_PlatformSettings, m_DefaultNodeType); 
    bRes = builder.createGraph(Loop);
    if (!bRes) 
        return false;

    m_Nodes = std::move(builder.m_Nodes);
    return bRes;
}

// Присоединяет уже созданный граф вычислений к графу вычислений this
void CalculationGraph::takeUpAnotherGraph(CalculationGraph* AnotherGraph)
{
	// Если ранее проводились расчеты, то теперь их надо будет провести заново 
	m_iii = 0;
	clearSecondaryCalculations();

    for (CalculationGraphNode* itCurNode: AnotherGraph->m_Nodes)
        m_Nodes.push_back(itCurNode);

	AnotherGraph->m_Nodes.clear(); // для того чтобы деструктор графа AnotherGraph не удалил скопированные вершины
	delete AnotherGraph;
	AnotherGraph = 0;
}

bool CalculationGraph::loadOperationsDelays(string PathToConfigurationFile)
{
	//try
	//{
		m_PlatformSettings = loadPlatformSettings(PathToConfigurationFile);
	/*}
	catch (OPSSettings::Except *e) 
	{
		OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
		pConsole->log(OPS::Console::LEVEL_ERROR, e->m_strErrorMsg + " ("+PathToConfigurationFile+")");
		return false;
	}*/
	return true; 
}

void CalculationGraph::dump2File(string name)
{
	vector<string> VertexNamesList, DirEdgesList;
	vector<int> VertexDelaysList, CriticalPath, StartTacts;
	int iii, StartDelay;

	getLists(VertexNamesList, VertexDelaysList, DirEdgesList, CriticalPath, StartTacts, iii, StartDelay);
	ofstream fout;
	fout.open(name.c_str());

	for(size_t i=0; i<VertexNamesList.size(); ++i)
		if (i < VertexNamesList.size()-1)
			fout << VertexNamesList[i] << ",";
		else
			fout << VertexNamesList[i] << endl;

	for(size_t i=0; i<VertexDelaysList.size(); ++i)
		if (i < VertexDelaysList.size()-1)
			fout << VertexDelaysList[i] << " ";
		else
			fout << VertexDelaysList[i] << endl;

	fout << DirEdgesList.size() << endl;
	for(size_t i=0; i<DirEdgesList.size(); ++i)
		fout << DirEdgesList[i] << endl;

	for(size_t i=0; i<CriticalPath.size(); ++i)
		if (i < CriticalPath.size()-1)
			fout << CriticalPath[i] << " ";
		else
			fout << CriticalPath[i] << endl;

	for(size_t i=0; i<StartTacts.size(); ++i)
		if (i < StartTacts.size()-1)
			fout << StartTacts[i] << " ";
		else
			fout << StartTacts.back() << endl;
	fout << iii << endl;
	fout.close();
}

void CalculationGraph::clearSecondaryCalculations()
{
	// Очищаем критический путь
	m_CriticalPath.clear();

	// Очищаем множество обратных дуг
	m_Uback.clear();

	// Очищаем матрицу смежностей
	if (m_VertexMatrix != 0)
	{
		for(size_t i=0; i<=m_Nodes.size(); i++)
            delete[] m_VertexMatrix[i];
			
			delete[] m_VertexMatrix;
	}

	// Очищаем матрицу типизации дуг
	if (m_DirEdgeTypesMatrix != nullptr)
	{
		for(size_t i=0; i<=m_Nodes.size(); i++)
            delete[] m_DirEdgeTypesMatrix[i];
			
			delete[] m_DirEdgeTypesMatrix;
	}

	// Очищаем начальные такты
	if (m_StartTacts != 0)
	{
        delete[] m_StartTacts;
	}

}

// Функция, очищающая всю информации о графе вычислений
void CalculationGraph::clear() 
{
	// Очистим всю память занятую вторичными вычислениями
	clearSecondaryCalculations();

	// Очищаем список вершин
    for (CalculationGraphNode* node: m_Nodes)
        delete node;
	m_Nodes.clear();

    delete m_PlatformSettings;
}

// Добавляет дугу в граф
CalculationGraphDirEdge* CalculationGraph::addDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistance, CalculationGraphDirEdge::EN_SignalType SignalType)
{
	return Begin->addOutDirEdge(End, DepDistance, SignalType);
}

// Возвращает: указатель	- если найдена вершина
//             0			- если не найдена вершина
CalculationGraphNode* CalculationGraph::findNodeByNodeNumber(int i)
{
	if ((1<=i) && (i<=(int)m_Nodes.size()))
		return m_Nodes[i-1];
	else
		return 0;
}

CalculationGraphDirEdge* CalculationGraph::findDirEdgeByNodesNumbers(int Begin, int End)
{
	// Необходимо искать с конца, т.к. иначе может быть найдена вершина не имеющая 
	// соответствующей дуги, а приклееная к ней вершина будет содержать нужную дугу.
	CalculationGraphNode* pEndOfTheDirEdge = findNodeByNodeNumber(End);

	if (pEndOfTheDirEdge != nullptr)
        for (CalculationGraphDirEdge* itCurrentDirEdge: pEndOfTheDirEdge->m_InDirEdges)
        	if (itCurrentDirEdge->m_Begin->m_NodeNumber == Begin)
				return itCurrentDirEdge;

	return nullptr;
}

int CalculationGraph::getDependenceDistanceByNodesNumbers(int Begin, int End)
{
	CalculationGraphDirEdge* p = findDirEdgeByNodesNumbers(Begin,End);
	return p->m_DependenceDistance;
}

void CalculationGraph::saveToFiles()
{
	// Подготовка к выводу
	int iNumberOfDirEdges = 0;
	std::list<string> Nodes;
	std::list<string> DirEdges;

	for(size_t i=1; i<=m_Nodes.size() ; i++)
	{
		CalculationGraphNode* pCurrentNode = findNodeByNodeNumber(i);
		if (pCurrentNode != nullptr)
		{
			Nodes.push_back(pCurrentNode->getStringRepresentation());

			// Все дуги записываем в список, кроме уже записанных.
			// Уже записанные дуги - это дуги ведущие из вершин с 
			// меньшими номерами в вершины с большими номерами
            for (CalculationGraphDirEdge* itCurrentDirEdge: pCurrentNode->m_InDirEdges)
            {
				iNumberOfDirEdges++;
				DirEdges.push_back(itCurrentDirEdge->getStringRepresentation());
			}
		}
	}
}



// Функция выполняет вторичные алгоритмы, инициализирующие основные объекты необходимые 
// для работы основных алгоритмов (executeAllAlgorithms)
void CalculationGraph::executeInitAlgorithms()
{
	// Построим матрицу смежности
	fillVertexMatrix();

	// Создаем вспомогательную вершину
	generateSourceVertex();

	// Построим критический путь
	buildCriticalPath();

	// Построим остовное дерево для графа без дуг инициализации с добавленой вершиной,
	// ведущей ко всем источникам
	buildSpanningTree();

	// Распределим дуги по типам, в соответствии с остовным деревом 
	typifyDirEdges();

}

void CalculationGraph::fillVertexMatrix()
{
	size_t i,j;

	if (m_VertexMatrix == 0) 
	{
		// Вершина номер 0 это добавочная вершина
        m_VertexMatrix = new int*[m_Nodes.size()+1];
		for(i=0; i<=m_Nodes.size(); i++)
            m_VertexMatrix[i] = new int[m_Nodes.size()+1];
	}

	for(i=0; i<=m_Nodes.size(); i++)
		for(j=0; j<=m_Nodes.size(); j++)
			m_VertexMatrix[i][j] = 0;

	// Осуществляем перебор всех вершин графа и устанавливаем исходящие дуги,
	// т.о. ни одна дуга пропущена не будет, несмотря на склееные вершины.
    for (CalculationGraphNode* itCurrentNode: m_Nodes)
        for (CalculationGraphDirEdge* itCurrentDirEdge : itCurrentNode->m_OutDirEdges)
		{
			// Получаем конец дуги
			CalculationGraphNode* pEndVertex = itCurrentDirEdge->getEndUnsafe();

			if (itCurrentDirEdge->getDirEdgeType() != CalculationGraphDirEdge::DET_INITIALIZE)
			{
				// Вносим дугу в матрицу смежностей
				m_VertexMatrix[itCurrentNode->m_NodeNumber][pEndVertex->m_NodeNumber] = 1;
			}
		}
}

// Создает дополнительную вершину, используемую в большинстве алгоритмов.
// Из этой вершины ведут дуги во все источники исходного графа
void CalculationGraph::generateSourceVertex()
{
	int OutDegree;

	//Поиск источников
	for(size_t j=1; j<=m_Nodes.size(); j++)
	{
		OutDegree = 0;
		for(size_t i=1; i<=m_Nodes.size(); i++)
			OutDegree = OutDegree + m_VertexMatrix[i][j];

		// Источник? Включая петлю // TODO: может ли быть более длинный цикл? Возможны недостижимые вершины
		if (OutDegree == 0 || (m_VertexMatrix[j][j] == 1 && OutDegree == 1))
		{
			// Теперь нет!
			m_VertexMatrix[0][j] = 1;
		}
	}
}

// Функция строит критический путь, самый длинный путь проходящий через каждую вершину только один раз
void CalculationGraph::buildCriticalPath()
{
	list<int> CurPath; // содержит текущий путь
	int StartSearchPoint = 1; // содержит "разумное" стартовое значение, с которого надо 
							// просматривать вершины, чтобы найти следующую в пути
	int iCurVertex = 0; // текущая вершина пути
	int goDeeper = 1; // признак завершения шагов в глубину, 
				// а значит необходимости сделать шаг наверх
	int j;

	while ((iCurVertex != 0) || (goDeeper != 0))
	{
		
		goDeeper = 0;
		j = StartSearchPoint;
		while((j<=(int)m_Nodes.size()) && (goDeeper == 0))
		{	
			if ((m_VertexMatrix[iCurVertex][j] == 1) 
                && (find(CurPath.begin(), CurPath.end(), j)==CurPath.end())
				&& (iCurVertex != j)) // страхуемся от петель
			{
				// Помечаем вершину как уже отнесенную к пути
				CurPath.push_back(iCurVertex);
				iCurVertex = j; // следующая текущая вершина
				goDeeper = 1; // нашли и выходим из этого цикла
				StartSearchPoint = 1;
			}
			j++;
		}
		
		if (goDeeper == 0)
		{
			if (iCurVertex == 0)
			{	// Конец поиска новых путей. Все пути были просмотренны.
				
				return;
			}
			else
			{	// Еще не все пути просмотренны
				goDeeper = 1;
				
				// Завершаем путь
				CurPath.push_back(iCurVertex);
				// всегда пропускаем первую вершину, у нее номер 0 (вспомогательная)
				CurPath.pop_front();
				
				// Сравниваем пути по длине
				if (CurPath.size() > m_CriticalPath.size())
				{
					// Копирование более длинного пути в критический путь
					m_CriticalPath.clear(); // очистка старого
					
                    for (int itNumberOfCurrentVertex: CurPath)
					{
						CalculationGraphNode* pNode = findNodeByNodeNumber(itNumberOfCurrentVertex);
						if (pNode != nullptr)
							m_CriticalPath.push_back(pNode);
					}
				}

				// всегда возвращаем первую вершину, у нее номер 0 (вспомогательная)
				CurPath.push_front(0);
				
				// Шаг наверх из листа
				StartSearchPoint = CurPath.back() +1; // следующая разумная стартовая вершина
				CurPath.pop_back();
				iCurVertex = CurPath.back(); // следующая текущая вершина
				CurPath.pop_back();
			}
		}
	}
}

// Функция строит остовное дерево методом поиска в глубину на графе с вспомогательной вершиной.
// Корнем всегда является единственный источник - вспомогательная вершина.
void CalculationGraph::buildSpanningTree()
{
	const int n = m_Nodes.size(); // количество вершин в графе без фиктивной вершины
	int iCurVertex; // текущая вершина обхода в глубину
	list<int> sources; // список источников
	list<int> PathToPrev; // путь от корня до предыдущей вершины по обходу в глубину
	vector<bool> VertexesOfTree; // хеш-список вершин уже отнесенных к дереву
	VertexesOfTree.assign(n+1, false);
	
	size_t i,j;

	// Выделяем память под матрицу типов дуг
    m_DirEdgeTypesMatrix = new int * [m_Nodes.size()+1];
	for(i=0; i<=m_Nodes.size(); i++)
        m_DirEdgeTypesMatrix[i] = new int[m_Nodes.size()+1];

	// Заполняем матрицу -1(нет дуги) 
    for(i=0; i<=m_Nodes.size(); i++)
		for(j=0; j<=m_Nodes.size(); j++)
			m_DirEdgeTypesMatrix[i][j] = -1;
	
    // Проставляем 0, если дуга в графе есть
    for (CalculationGraphNode* itNode: m_Nodes)
	{
		if (itNode->m_InDirEdges.empty())
		{
			m_DirEdgeTypesMatrix[0][itNode->m_NodeNumber] = CalculationGraphDirEdge::DET_NEUTRAL;
			sources.push_back(itNode->m_NodeNumber);
		}
		for (CalculationGraphDirEdge* itDirEdge: itNode->m_OutDirEdges)
			m_DirEdgeTypesMatrix[itDirEdge->getBegin()->getNodeNumber()][itDirEdge->getEnd()->getNodeNumber()] = CalculationGraphDirEdge::DET_NEUTRAL;
	}

	// Стартовая вершина корень остовного дерева получает номер n
	PathToPrev.push_back(0);
	VertexesOfTree[0] = true;

	// Первым к дереву относим критический путь
    for (CalculationGraphNode* itCurrent: m_CriticalPath)
	{
		j = itCurrent->m_NodeNumber;
		// Помечаем дугу как дугу дерева
		m_DirEdgeTypesMatrix[PathToPrev.back()][itCurrent->m_NodeNumber] = CalculationGraphDirEdge::DET_TREE;

		// Добавляем вершину в путь PathToPrev
		PathToPrev.push_back(itCurrent->m_NodeNumber);

		// Помечаем вершину как уже отнесенную к дереву
		VertexesOfTree[itCurrent->m_NodeNumber] = true;
	}

	// Достраиваем остовное дерево методом поиска в глубину
	iCurVertex= PathToPrev.back();
	PathToPrev.pop_back();

	int treeVertexesCount = 1 + m_CriticalPath.size();
	bool dfsContinue = (iCurVertex > 0);
	while(dfsContinue)
	{
		// Шаги в глубину
		list<CalculationGraphDirEdge*>::iterator itDirEdge = m_Nodes[iCurVertex-1]->m_OutDirEdges.begin();
		for(; itDirEdge != m_Nodes[iCurVertex-1]->m_OutDirEdges.end(); ++itDirEdge)
		{
			j = (*itDirEdge)->getEnd()->getNodeNumber();
			if (!VertexesOfTree[j])
			{	// Нашли нейтральную дугу не ведущую в вершину уже отнесенную к остовному дереву

				// Теперь это дуга дерева
				m_DirEdgeTypesMatrix[iCurVertex][j] = CalculationGraphDirEdge::DET_TREE;

				// Теперь это уже вершина отнесенная к остовному дереву
				VertexesOfTree[j] = true;
				treeVertexesCount++;

				// Собственно, шаг в глубину
				if (!m_Nodes[j-1]->m_OutDirEdges.empty())
				{
					PathToPrev.push_back(iCurVertex);
					iCurVertex = j;
					itDirEdge = m_Nodes[iCurVertex-1]->m_OutDirEdges.begin();
				}
			}
		}

		// В глубину шаг сделать нельзя, значит надо сделать
		// Шаг к поверхности
		if (!PathToPrev.empty()) // это условие тождественно условию выхода из внешнего while-а
		{
			// Шаг наверх
			iCurVertex = PathToPrev.back(); // следующая текущая вершина
			PathToPrev.pop_back();

			if (iCurVertex == 0)
			{
				while((!sources.empty()) && (VertexesOfTree[sources.front()])) // как минимум 1 источник был в критическом пути, а значит он отмечен
					sources.pop_front();
				if (!sources.empty())
				{
					iCurVertex = sources.front();
					sources.pop_front();

					// Теперь это дуга дерева
					m_DirEdgeTypesMatrix[0][iCurVertex] = CalculationGraphDirEdge::DET_TREE;
					// Теперь это уже вершина отнесенная к остовному дереву
					VertexesOfTree[iCurVertex] = true;
					PathToPrev.push_back(0);
				}
			}
			dfsContinue = !sources.empty();
		}
		else
			dfsContinue = false;
	}
}

// Функция поиска начала древесной дуги по ее концу
int CalculationGraph::findBeginOfTreeDirEdgeByEnd(int End)
{
	for(size_t i = 0; i<=m_Nodes.size(); i++)
		if (m_DirEdgeTypesMatrix[i][End] == CalculationGraphDirEdge::DET_TREE)
			return i;
	return -1;
}

void CalculationGraph::typifyDirEdges()
{
	// Определяем типы дуг для графа на числовой матрице m_DirEdgesTypesMatrix
	typifyDirEdgesInMatrix();

	// Устанавливаем типы дуг графа после того как они уже установлены в матрице m_DirEdgesTypesMatrix
	typifyDirEdgesInGraphAfterMatrix();
}

void CalculationGraph::typifyDirEdgesInGraphAfterMatrix()
{
    for (CalculationGraphNode* itCurrentNode: m_Nodes) 
        for (CalculationGraphDirEdge* itCurrentDirEdge: itCurrentNode->m_OutDirEdges)
			if (itCurrentDirEdge->getDirEdgeType() == CalculationGraphDirEdge::DET_NEUTRAL)
			{
				CalculationGraphNode* pEndOfTheDirEdge = itCurrentDirEdge->getEndUnsafe();
				const int type = m_DirEdgeTypesMatrix[itCurrentNode->m_NodeNumber][pEndOfTheDirEdge->m_NodeNumber];
				itCurrentDirEdge->m_DirEdgeType = (CalculationGraphDirEdge::EN_DirEdgeType)type;
				
				if (type == CalculationGraphDirEdge::DET_BACK)
				{	// Обратная дуга
					m_Uback.push_back(itCurrentDirEdge);
				}
			}
}

// Функция типизирует все дуги в соответствии с классификацией EN_DirEdgeType
void CalculationGraph::typifyDirEdgesInMatrix()
{
	size_t i = 1, j = 1;
    bool found = true;
	int iAncestorOfTheBegin, iAncestorOfTheEnd; 
	list<int> lAncestorsOfTheBegin, lAncestorsOfTheEnd;

	while(found)
	{
		// Ищем следующую (в лексикографическом смысле) дугу после дуги (i,j) являющуюся нейтральной
		found = false; 
		for(; (i<=m_Nodes.size()) && (found == false); i++) // Все дуги выходящие из корня 
		{
			if (j > m_Nodes.size())
				j = 1;

			for(; j<=m_Nodes.size(); j++) // являются дугами дерева
				if (m_DirEdgeTypesMatrix[i][j] == CalculationGraphDirEdge::DET_NEUTRAL) 
                {
					found = true;
                    ++j;
                    break;
                }
		}
		
		if (found)
		{	// Нашли
			iAncestorOfTheBegin = --i; // Возвращаем последнее приращение i
			iAncestorOfTheEnd = --j; // Возвращаем последнее приращение j
			
			// Петля
			if (iAncestorOfTheBegin == iAncestorOfTheEnd)
			{	// Петля относится к обратным дугам
				m_DirEdgeTypesMatrix[iAncestorOfTheBegin][iAncestorOfTheEnd] = CalculationGraphDirEdge::DET_BACK;
			}
			
			lAncestorsOfTheBegin.clear();
			lAncestorsOfTheEnd.clear();

			// Определяем тип дуги.
			// На каждом шаге:
			// 1) Добавляем к спискам предков обеих концов дуги по 1 предщественнику в соответствии с построенным ранее деревом.
			// 2) Проверяем не появился ли общий предок у конца и начала рассматриваемой дуги. И если нашелся общий предок то определяем по нему тип дуги.
			// Зацикливание не возможно, т.к. вспомогательная вершина является общим предком для всех.
			while(m_DirEdgeTypesMatrix[i][j] == CalculationGraphDirEdge::DET_NEUTRAL)
			{
				lAncestorsOfTheBegin.push_front(iAncestorOfTheBegin);
				lAncestorsOfTheEnd.push_front(iAncestorOfTheEnd);
				
				if (iAncestorOfTheBegin != -1)
					iAncestorOfTheBegin = findBeginOfTreeDirEdgeByEnd(iAncestorOfTheBegin);
				
				if (iAncestorOfTheEnd != -1)
					iAncestorOfTheEnd = findBeginOfTreeDirEdgeByEnd(iAncestorOfTheEnd);
				
				if (find(lAncestorsOfTheBegin.begin(), lAncestorsOfTheBegin.end(), iAncestorOfTheEnd) != lAncestorsOfTheBegin.end())
				{	// Найден
					if (lAncestorsOfTheBegin.back() == iAncestorOfTheEnd)
					{	// Дуга прямая, т.к. ее начало является предком конца
						m_DirEdgeTypesMatrix[i][j] = CalculationGraphDirEdge::DET_STRAIGHT;
					}
					else
					{	// Дуга поперечная, т.к. ее начало имеет ближайшего общего предка концом, 
						// не совпадающего ни с тем ни с другим
						m_DirEdgeTypesMatrix[i][j] = CalculationGraphDirEdge::DET_ACROSS;
					}
				}
				
				if (find(lAncestorsOfTheEnd.begin(), lAncestorsOfTheEnd.end(), iAncestorOfTheBegin) != lAncestorsOfTheEnd.end())
				{	// Найден
					if (lAncestorsOfTheEnd.back() == iAncestorOfTheBegin)
					{	// Дуга обратная, т.к. ее конец является предком начала
						m_DirEdgeTypesMatrix[i][j] = CalculationGraphDirEdge::DET_BACK;
					}
					else
					{	// Дуга поперечная, т.к. ее начало имеет ближайшего общего предка с концом, 
						// не совпадающего ни с тем ни с другим
						m_DirEdgeTypesMatrix[i][j] = CalculationGraphDirEdge::DET_ACROSS;
					}
				}
			}
		}	// if (found == 1)
	}
}

// Функция выполняет все(!!!) алгоритмы. После необходимой 
// инициализации (executeInitAlgorithms) выполняются основные алгоритмы.
void CalculationGraph::executeAllAlgorithms()
{
	// Настроим все вспомогательные объекты для работы с алгоритмами
	executeInitAlgorithms();

	// Служебная
	//printMatrixes();

	// Рассчитаем iii
	calculate_iii();

	// Расчитываем буферы
	calculateBuffers();

}

double CalculationGraph::sumCycleDependenceDistancies(list<CalculationGraphDirEdge*> Cycle)
{
	int sum = 0;
    for (CalculationGraphDirEdge*itCurrentDirEdge: Cycle)
	    sum = sum + itCurrentDirEdge->m_DependenceDistance;

	return sum;
}

double CalculationGraph::sumCycleOperationsDelays(Int_Set Cycle)
{
	int sum = 0;
    for (int itCurrent: Cycle)
		sum = sum + findNodeByNodeNumber(itCurrent)->m_OperationDelay;

	return sum;
}

// типы дуг для меток на матрице смежности графа
enum ARC_LABEL_TYPE 
{
    ALT_UNMARKED = 0, // дуга еще не рассмотрена
    ALT_MARKED = 1, // дуга пройдена
    ALT_ABSENT = -1 // дуга отсутствует (для матрицы смежности)
};

void CalculationGraph::calculateBackEdge_iii(CalculationGraphDirEdge* itCurrentBackDirEdge, int** labels)
{
    list<CalculationGraphDirEdge*> Path; // путь, который достраивается до 
										// простейшего цикла на графе
	set<int> VertexesOfThePath; // вершины входящие в путь Path
		
    // Начало и конец дуги
	int iBeginOfTheDirEdge = itCurrentBackDirEdge->m_Begin->m_NodeNumber;
	int iEndOfTheDirEdge = itCurrentBackDirEdge->m_End->m_NodeNumber;
		
	// Инициализируем iCurrentVertex, AllDirEdgesLabeled, Path и VertexesOfThePath
	int iCurrentVertex = iEndOfTheDirEdge;
    bool bAllDirEdgesLabeled = false; // признак того что все дуги исходящие из вершины iCurrentVertex уже помечены
	Path.push_back(itCurrentBackDirEdge);
	VertexesOfThePath.insert(iEndOfTheDirEdge);

	// Петля?
	if (iBeginOfTheDirEdge == iEndOfTheDirEdge)
	{
		// Проверка петли на максимум отношения суммы задержек операций
		// к сумме расстояний зависимостей.
		int ratio = (int) ceil(sumCycleOperationsDelays(VertexesOfThePath)/sumCycleDependenceDistancies(Path));
		m_iii = std::max(m_iii, ratio);


		// С петлей не будет других простейших циклов, 
		// поэтому минуем перебор всех простейших циклов
		bAllDirEdgesLabeled = true;
	}

	// Перебираем все простейшие циклы для дуги itCurrentDirEdge
	while ((iCurrentVertex != iEndOfTheDirEdge) || (!bAllDirEdgesLabeled))
	{
		// Проверяем метки исходящих дуг из вершины iCurrentVertex
        int* curVertexLabels = labels[iCurrentVertex]+1; // без фиктивной 0ой вершины
        int* iter = std::find(curVertexLabels, curVertexLabels + m_Nodes.size(), ALT_UNMARKED); // ищем непомеченную дугу
        size_t j = iter - curVertexLabels + 1; // номер непомеченной дуги
        bAllDirEdgesLabeled = !(j < m_Nodes.size() + 1);

		// Все ли исходящие дуги помечены?
		if (!bAllDirEdgesLabeled)
		{	// Не все помечены

			int next = j;
			CalculationGraphDirEdge* pCurrentDirEdge = findDirEdgeByNodesNumbers(iCurrentVertex, next);
			if (next == iBeginOfTheDirEdge)
			{
				// iEndOfTheDirEdge - никогда не может оказаться в Path, иначе Path 
				// становится не простейшим путем. Значит, нет смысла проверять
				// находится ли next в VertexesOfThePath. Делаем эту проверку только в else.

				// Шаг для правильного заполнения структур данных, связанных с циклом
				Path.push_back(pCurrentDirEdge);
				VertexesOfThePath.insert(next);

				// Проверка цикла на максимум отношения суммы задержек операций
				// к сумме расстояний зависимостей.
				int ratio = (int) ceil(sumCycleOperationsDelays(VertexesOfThePath)/sumCycleDependenceDistancies(Path));
				m_iii = std::max(m_iii, ratio);
					
				// Шаг обратно
				Path.pop_back();
				VertexesOfThePath.erase(next);
				labels[iCurrentVertex][next] = ALT_MARKED;
			}
			else
			{	// next - не начало обратной дуги *itCurrentBackDirEdge
				if (VertexesOfThePath.count(next) == 1)
				{	// Дуга ведет в вершину пути Path
					labels[iCurrentVertex][next] = ALT_MARKED;
				}
				else
				{	// Дуга ведет в вершину, которой нет в текущем пути Path
					// Шаг в глубину
					Path.push_back(pCurrentDirEdge);
					VertexesOfThePath.insert(next);
					iCurrentVertex = next;
				}
			}
		}
		else
		{	// Все исходящие дуги помечены
				
			// Снимем метки, т.к. будем делать шаг к поверхности
			for(size_t j=1; j<=m_Nodes.size(); j++)
			{	// снимаем метки
				if (labels[iCurrentVertex][j] == ALT_MARKED)
					labels[iCurrentVertex][j] = ALT_UNMARKED;
			}

			// Шаг к поверхности
			if (iCurrentVertex != iEndOfTheDirEdge) 
			{
				VertexesOfThePath.erase(iCurrentVertex); // удаляем вершину из пути
				CalculationGraphDirEdge* pCurrentDirEdge = Path.back(); // укорачиваем путь
				Path.pop_back();			// укорачиваем путь
				iCurrentVertex = pCurrentDirEdge->m_Begin->m_NodeNumber; // новая текущая вершина
				labels[pCurrentDirEdge->m_Begin->m_NodeNumber][pCurrentDirEdge->m_End->m_NodeNumber] = ALT_MARKED; // помечаем пройденную дугу
				bAllDirEdgesLabeled = false; // чтобы не выскочить из цикла 
			}
		}
	} // end of while по простейшим циклам
}

// Функция перебирает все простейшие циклы и расчитывает интервал инициализации итераций
void CalculationGraph::calculate_iii()
{
	// Обратные дуги нумеруем от 0 до n-1
	int** labels; // -1 - нет дуги, 0 - не помеченная дуга, 1 - помеченная дуга

	// Создаем и обнуляем метки
    labels = new int* [m_Nodes.size()+1];
	for(size_t i=0; i<=m_Nodes.size(); i++)
		labels[i] = new int[m_Nodes.size()+1];

	for(size_t i=1; i<=m_Nodes.size(); i++)
	{
		// Нам не интересны дуги исходящие из вспомогательной вершины, поэтому мы считаем что их нет
		labels[i][0] = labels[0][i] = ALT_ABSENT;
		for(size_t j=1; j<=m_Nodes.size(); j++)
			labels[i][j] = (m_VertexMatrix[i][j]!=(int)CalculationGraphDirEdge::DET_NEUTRAL) && (m_VertexMatrix[i][j]!=(int)CalculationGraphDirEdge::DET_INITIALIZE) ? ALT_UNMARKED : ALT_ABSENT;
	}

	// Изначально предполагаем минимальный интервал инициализации итераций
	m_iii = 1;

	// Если есть не конвейерные вычислители, то надо это учесть при расчете iii
	for(const CalculationGraphNode* iter: m_Nodes)
		if (iter->getType() == NT_NOTPIPELINED) 
			m_iii = std::max(m_iii, iter->getOperationDelay());

	// Для каждой дуги будем строить все простейшие циклы
    for (CalculationGraphDirEdge* itCurrentBackDirEdge: m_Uback)
        calculateBackEdge_iii(itCurrentBackDirEdge, labels);

    for(size_t i=0; i<=m_Nodes.size(); i++)
        delete[] labels[i];
    delete[] labels;
}

void CalculationGraph::separationOfGraph(list<Int_Set> &SubGraphs)
{
	// Множество источников
	Int_Set Sources;

	// Отнесена ли уже вершина к какому-нибудь подграфу (вершины чтения записи сразу =true) 
    vector<bool> labels(m_Nodes.size()+1);
				
	labels[0] = true;

	// Выход из цикла разбиения на подграфы
	bool escape = true;
	// Единственный источник в подграфе
	int iSubGraphSource = 0;
	// Текущий подграф
	Int_Set CurSubGraph;
	// Условие завершения формирования подграфа
	bool bSubGraphConstructed;

	bool bSink,bSource;

	// Помечаем вершины чтения и записи
	for(size_t i=1; i<=m_Nodes.size(); i++)
	{
		bSink = true;
		bSource = true;

		for(size_t j=1; j<=m_Nodes.size(); j++)
		{
			if (m_VertexMatrix[i][j] == 1)
				bSink = false;

			if (m_VertexMatrix[j][i] == 1)
				bSource = false;
		}

		// Помечаем вершины чтения и записи
		labels[i] = (bSink || bSource);

		// Собираем источники
		if (bSource)
			Sources.insert(i);
	}
	// Проверка есть ли еще не отнесенные к подграфам вершины
	for(size_t i=1; i<=m_Nodes.size(); i++)
		escape = escape && labels[i];

	size_t i,j;
	while (!escape)
	{
		// Найдем единственный источник подграфа
		
		// TODO: пример для старого алгоритма a=b+c
		// сколько подграфов?какие?  

		for(j=1; j<=m_Nodes.size(); j++)
		{
			if (!labels[j])
			{
				bSource = true;
				for(i=1; i<=m_Nodes.size(); i++)
				{
					if ((m_VertexMatrix[i][j] == 1) && (m_DirEdgeTypesMatrix[i][j] != CalculationGraphDirEdge::DET_BACK) && (Sources.count(i) == 0) && (!labels[i]))
					{	// Дуга i->j не обратная, ведет не из источника и i,j не помечены 
						bSource = false;
					}
				}
				
				if (bSource)
				{
					iSubGraphSource = j;
					break;
				}
			}
		}

		// Очищаем подграф
		CurSubGraph.clear();

		// Вносим источник в подграф
		CurSubGraph.insert(iSubGraphSource);
		labels[iSubGraphSource] = true;

		// Создаем этот подграф
		bSubGraphConstructed = false;
		while (!bSubGraphConstructed)
		{
			bSubGraphConstructed = true;

            for (int i: CurSubGraph) // i - CurrentVertexNumber
				for(j=1; j<=m_Nodes.size(); j++)
				{
					if ((m_VertexMatrix[i][j] == 1) && (m_DirEdgeTypesMatrix[i][j] != CalculationGraphDirEdge::DET_BACK) && (CurSubGraph.count(j) == 0))
					{	// Граф еще не достроен
						bSubGraphConstructed = false;
						CurSubGraph.insert(j);
						labels[j] = true;
					}
				}
		}

		// Добавляем выделенный подграф к списку подграфов
		SubGraphs.push_back(CurSubGraph);

		// Проверка есть ли еще не отнесенные к подграфам вершины
		escape = true;
		for(i=1; i<=m_Nodes.size(); i++)
			escape = escape && labels[i];
	}
}

void CalculationGraph::calculateScheduleOfSubGraphs(list<Int_Set>& SubGraphs)
{
	int s1, s2;
	size_t i, j(0);

	// Матрица смежностей текущего подграфа
    int **Cur_SubGraph_VertexMatrix = new int* [m_Nodes.size()+1];
	for(i=0; i<=m_Nodes.size(); i++)
        Cur_SubGraph_VertexMatrix[i] = new int[m_Nodes.size()+1];
	
	// Метки означают, что вершина уже пройдена в порядке правильной нумерации (false - нет)
    bool *passed = new bool[m_Nodes.size()+1];

	bool bSource;

	// Начала дуг для текущей вершины в правильной нумерации
	list<int> DirEdgesBegins;
	list<int>::iterator itCurBegin;

	// Итератор текущей вершины в множестве вершин графа
	Int_Set::iterator itCurVertex;

	// Множество вершин текущего подграфа
	Int_Set CurSubGraph;

	CalculationGraphDirEdge* pDirEdge;

    for (Int_Set& CurSubGraph: SubGraphs)
	{
		if (CurSubGraph.empty())
			continue;

		// Заполняем матрицу для текущего подграфа
		for(i=1; i<=m_Nodes.size(); i++)
		{
			// Начальные значения для меток
			passed[i] = !(CurSubGraph.count(i) == 1);

			// Заполнение матрицы
			for(j=1; j<=m_Nodes.size(); j++)
			{
				if ((m_VertexMatrix[i][j] == 1) 
                    && (m_DirEdgeTypesMatrix[i][j] != CalculationGraphDirEdge::DET_BACK) 
                    && (CurSubGraph.count(i) == 1) 
                    && (CurSubGraph.count(j) == 1))
					Cur_SubGraph_VertexMatrix[i][j] = 1;
				else
					Cur_SubGraph_VertexMatrix[i][j] = 0;
			}
		}

		// Ищем источник
		itCurVertex = CurSubGraph.begin();
		bSource = false;
		while (!bSource)
		{
			bSource = true;
			j = *itCurVertex;
			for(i=1; i<=m_Nodes.size(); i++)
				if (Cur_SubGraph_VertexMatrix[i][j] == 1)
					bSource = false;
			itCurVertex++;
		}

		// Источник
		m_StartTacts[j] = 1;
		passed[j] = true;

		// Цикл с длиной равной количество вершин подграфа -1 (источник уже включили в расписание)
		for(unsigned k=1; k<CurSubGraph.size(); k++)
		{
			// Ищем вершину очередную в порядке правильной нумерации
			itCurVertex = CurSubGraph.begin();
			
			bSource = false;
			while (!bSource)
			{
				j = (*itCurVertex);
				if (!passed[j])
				{
					DirEdgesBegins.clear();
					bSource = true;
					for(i=1; i<=m_Nodes.size(); i++)
					{
						if (Cur_SubGraph_VertexMatrix[i][j] == 1)
						{
							DirEdgesBegins.push_back(i);
							if (!passed[i])
							{	// вершина еще не очередная в порядке правильной нумерации
								bSource = false;
							}
						}
					}
				}
				itCurVertex++;
			}
			// По завершении цикла j - это текущая вершина в правильной нумерации

			passed[j] = true;

			if (DirEdgesBegins.size() == 1)
			{
				i = DirEdgesBegins.front();

				// Теперь это дуга i->j
				m_StartTacts[j] = m_StartTacts[i]+getNodeOperationDelayByNodeNumber(i) + getSendingDelay(i, j) - getDependenceDistanceByNodesNumbers(i,j)*m_iii;
			}
			else
			{	// равно 2

				i = DirEdgesBegins.front();
				s1 = m_StartTacts[i]+getNodeOperationDelayByNodeNumber(i) + getSendingDelay(i, j) - getDependenceDistanceByNodesNumbers(i,j)*m_iii;
				i = DirEdgesBegins.back();
				s2 = m_StartTacts[i]+getNodeOperationDelayByNodeNumber(i) + getSendingDelay(i, j) - getDependenceDistanceByNodesNumbers(i,j)*m_iii;
				if (s1==s2)
				{
					m_StartTacts[j] = s1;
				}
				else
				{
					if (s1>s2)
					{
						m_StartTacts[j] = s1;
						// Дуга с меньшим значением s
						pDirEdge = findDirEdgeByNodesNumbers(DirEdgesBegins.back(),j);
						pDirEdge->m_BufferLatency = s1-s2;
					}
					else
					{
						m_StartTacts[j] = s2;
						// Дуга с меньшим значением s
						pDirEdge = findDirEdgeByNodesNumbers(DirEdgesBegins.front(),j);
						pDirEdge->m_BufferLatency = s2-s1;
					}
				}
			}
		} // for(k= ... по подграфу
	}

    delete[] passed;

	for(size_t i=0; i<=m_Nodes.size(); i++)
	{
        delete[] Cur_SubGraph_VertexMatrix[i];
	}
	delete[] Cur_SubGraph_VertexMatrix;
}

void CalculationGraph::uniteCalculatedSubGraphs(list<Int_Set> &SubGraphs)
{
	if (SubGraphs.empty())
		return;
	
	// Текущий уже склееный граф (подграф исходного), который в результате склеивания станет исходным графом
	Int_Set CurUnitedGraph = SubGraphs.front();
	// Нормализуем расписание подграфа
	normalizeSubGraphSchedule(CurUnitedGraph);
	SubGraphs.pop_front();

	Int_Set::iterator itCurVertexNumber;

	typedef pair<int,int> Int_Pair;
	Int_Pair iiDirEdge;
	list<Int_Pair> BetweenGraphsDirEdges;
	list<Int_Pair>::iterator itCurDirEdge;

	CalculationGraphDirEdge* pDirEdge;
	int i, min_f, f;

	list<Int_Set>::iterator itCurSubGraph = SubGraphs.begin();
	while (itCurSubGraph != SubGraphs.end())
	{
		BetweenGraphsDirEdges.clear();

		// 1. Ищем множество дуг проходящих между множествами вершин склеиваимых подграфов
		// Есть ли дуги ведущие от (*itCurSubGraph) к CurUnitedGraph?
		itCurVertexNumber = (*itCurSubGraph).begin();
		while (itCurVertexNumber != (*itCurSubGraph).end())
		{
			i = (*itCurVertexNumber);
			for(size_t j=1; j<=m_Nodes.size(); j++)
			{
				if ((m_VertexMatrix[i][j] == 1) && (CurUnitedGraph.count(j)))
				{	// Вносим дугу в список дуг соединяющих множества
		  			BetweenGraphsDirEdges.push_back(Int_Pair(i,j));
				}
			}
			itCurVertexNumber++;
		}

		// Есть ли дуги ведущие от CurUnitedGraph к (*itCurSubGraph)?
		itCurVertexNumber = CurUnitedGraph.begin();
		while (itCurVertexNumber != CurUnitedGraph.end())
		{
			i = (*itCurVertexNumber);
			for(size_t j=1; j<=m_Nodes.size(); j++)
			{
				if ((m_VertexMatrix[i][j] == 1) && ((*itCurSubGraph).count(j)))
				{	// Вносим дугу в список дуг соединяющих множества
					BetweenGraphsDirEdges.push_back(Int_Pair(i,j));
				}
			}
			itCurVertexNumber++;
		}
		
		
		if (BetweenGraphsDirEdges.size() > 0)
		{
			// 2. Анализируем дуги между множествами вершин склеиваимых подграфов

			iiDirEdge = BetweenGraphsDirEdges.front();
			min_f = getSendingDelay(iiDirEdge.first, iiDirEdge.second) + m_StartTacts[iiDirEdge.second] + getDependenceDistanceByNodesNumbers(iiDirEdge.first,iiDirEdge.second)*m_iii-(m_StartTacts[iiDirEdge.first]+getNodeOperationDelayByNodeNumber(iiDirEdge.first));
			itCurDirEdge = BetweenGraphsDirEdges.begin();
			itCurDirEdge++; // первую дугу уже учли в minf
			while (itCurDirEdge != BetweenGraphsDirEdges.end())
			{
				iiDirEdge = (*itCurDirEdge);
				f = getSendingDelay(iiDirEdge.first, iiDirEdge.second) + m_StartTacts[iiDirEdge.second] + getDependenceDistanceByNodesNumbers(iiDirEdge.first,iiDirEdge.second)*m_iii-(m_StartTacts[iiDirEdge.first]+getNodeOperationDelayByNodeNumber(iiDirEdge.first));
				if (min_f > f)
					min_f = f;
				itCurDirEdge++;
			}

			// 3. Склеиваем подграфы

			if (CurUnitedGraph.count(iiDirEdge.first) == 1)
			{	// Дуги ведут от CurUnitedGraph к (*itCurSubGraph)
				itCurVertexNumber = CurUnitedGraph.begin();
				while (itCurVertexNumber != CurUnitedGraph.end())
				{
					m_StartTacts[*itCurVertexNumber] = m_StartTacts[*itCurVertexNumber] - min_f;
					itCurVertexNumber++;
				}

				// Объединяем множества
				itCurVertexNumber = (*itCurSubGraph).begin();
				while (itCurVertexNumber != (*itCurSubGraph).end())
				{
					CurUnitedGraph.insert(*itCurVertexNumber);
					itCurVertexNumber++;
				}
			}
			else
			{	// Дуги ведут от (*itCurSubGraph) к CurUnitedGraph
				itCurVertexNumber = (*itCurSubGraph).begin();
				while (itCurVertexNumber != (*itCurSubGraph).end())
				{
					m_StartTacts[*itCurVertexNumber] = m_StartTacts[*itCurVertexNumber] - min_f;

					// Объединяем множества
					CurUnitedGraph.insert(*itCurVertexNumber);

					itCurVertexNumber++;
				}
			}

			// 4. Расчитываем буферы

			itCurDirEdge = BetweenGraphsDirEdges.begin();
			while (itCurDirEdge != BetweenGraphsDirEdges.end())
			{
				iiDirEdge = (*itCurDirEdge);
				pDirEdge = findDirEdgeByNodesNumbers(iiDirEdge.first, iiDirEdge.second);
				f = getSendingDelay(iiDirEdge.first, iiDirEdge.second) + m_StartTacts[iiDirEdge.second] + getDependenceDistanceByNodesNumbers(iiDirEdge.first,iiDirEdge.second)*m_iii-(m_StartTacts[iiDirEdge.first]+getNodeOperationDelayByNodeNumber(iiDirEdge.first));
				pDirEdge->m_BufferLatency = f - min_f;
				
				itCurDirEdge++;
			}

			// 5. Нормализуем расписание подграфа
			normalizeSubGraphSchedule(CurUnitedGraph);
		}
		else 
		{
			// Объединяем множества
			itCurVertexNumber = (*itCurSubGraph).begin();
			while (itCurVertexNumber != (*itCurSubGraph).end())
			{
				CurUnitedGraph.insert(*itCurVertexNumber);
				itCurVertexNumber++;
			}
		}

		itCurSubGraph++;
	}

	SubGraphs.clear();
}

void CalculationGraph::calculateBuffersOnBackDirEdges()
{
    for (CalculationGraphDirEdge* itCurDirEdge: m_Uback)
	{
		itCurDirEdge->m_BufferLatency = m_StartTacts[itCurDirEdge->getEnd()->getNodeNumber()] + 
                                        itCurDirEdge->getDependenceDistance() * m_iii - 
                                        (m_StartTacts[itCurDirEdge->getBegin()->getNodeNumber()] + 
                                        itCurDirEdge->getBegin()->getOperationDelay() + 
                                        itCurDirEdge->getSendingDelay());
		itCurDirEdge->m_BufferSize = (unsigned) ceil (itCurDirEdge->m_BufferLatency / (double) m_iii);
	}
}

void CalculationGraph::calculateStartTactsOfRWVertexes()
{
	int min_s(0);
	bool bFirst, bSink;
	CalculationGraphDirEdge* pDirEdge;

	// 1. Вершины чтения

	// Ищем источники
	for(size_t j=1; j<=m_Nodes.size(); j++)
	{
		if (m_VertexMatrix[0][j] == 1)
		{	// j - источник
			bFirst = true;
			for(size_t k=1; k<=m_Nodes.size(); k++)
			{
				if ((m_DirEdgeTypesMatrix[j][k] > 0) && bFirst)
				{
					min_s = m_StartTacts[k] - getSendingDelay(j,k) - getNodeOperationDelayByNodeNumber(j);
					bFirst = false;
				}
				
				if ((m_DirEdgeTypesMatrix[j][k] > 0) && (min_s > m_StartTacts[k]))
					min_s = m_StartTacts[k] - getSendingDelay(j,k) - getNodeOperationDelayByNodeNumber(j);
			}

			if (!bFirst)
			{
				// Назначаем старт
				m_StartTacts[j] = min_s;

				// Назначаем буферы
				for(size_t k=1; k<=m_Nodes.size(); k++)
				{
					if (m_VertexMatrix[j][k] == 1)
					{
						pDirEdge = findDirEdgeByNodesNumbers(j,k);
                        pDirEdge->m_BufferLatency = m_StartTacts[k] - m_StartTacts[j] - m_PlatformSettings->m_ReadingWriting - pDirEdge->getSendingDelay(); // TODO: Создать функцию кот это сама делать будет
						pDirEdge->m_BufferSize = (unsigned) ceil(pDirEdge->m_BufferLatency / (double) m_iii);
					}
				}

			}
			else
			{	// Изолированный источник (вообще-то бред) // TODO: выкинуть варнинг
				m_StartTacts[j] = 1;
			}
		}
	} // for(j=1 ...

	// 2. Вершины записи

	// Ищем стоки
	for(size_t i=1; i<=m_Nodes.size(); i++)
	{
		bSink = true;
		for(size_t j=1; j<=m_Nodes.size(); j++)
			bSink = bSink && (m_VertexMatrix[i][j] == 0);
		
		if (bSink)
		{	// i - сток
			for(size_t k=1; k<=m_Nodes.size(); k++)
			{
				if (m_VertexMatrix[k][i] == 1)
				{
					// Такая вершина должна быть одна, т.к. нет out-out склеек, 
					// а следовательно в вершину записи входит только одна дуга.
					m_StartTacts[i] = m_StartTacts[k] + getNodeOperationDelayByNodeNumber(k) + getSendingDelay(k, i);
				}
			}
		}
	} // for(i=1 ...
}

void CalculationGraph::normalizeSchedule()
{
	Int_Set Vertexes;
	for(size_t i=1; i<=m_Nodes.size(); i++)
		Vertexes.insert(i);
	normalizeSubGraphSchedule(Vertexes);
}


void CalculationGraph::calculateBuffers()
{
	// Выделяем память под массив начальных тактов
    m_StartTacts = new int[m_Nodes.size()+1];

	// Разбиваем граф на подграфы, не содержащие вершин чтения и записи, 
	// и содержащие по одному источнику
	list<Int_Set> SubGraphs;
	separationOfGraph(SubGraphs);

	// Рассчитываем стартовые такты для, полученных подграфов
	calculateScheduleOfSubGraphs(SubGraphs);

	// Объединяем(склеиваем) таблицы расписаний, полученные для подграфов
	uniteCalculatedSubGraphs(SubGraphs);

	// Рассчитываем буферы на обратных дугах
	calculateBuffersOnBackDirEdges();

	// Добавляем в таблицу расписания вершины чтения и записи
	calculateStartTactsOfRWVertexes();

	// Нормализуем таблицу расписания, т.е. делаем так чтобы она начиналась с 1 такта
	normalizeSchedule();

}

int CalculationGraph::getNodeOperationDelayByExpressionBase(ExpressionBase* Data) const
{
	if (Data->is_a<BasicCallExpression>())
	{
		switch(Data->cast_to<BasicCallExpression>().getKind())
		{
		case BasicCallExpression::BCK_BINARY_PLUS:
		case BasicCallExpression::BCK_BINARY_MINUS:
			{
				return m_PlatformSettings->m_IntSumDelay;
				break;
			}
			
		case BasicCallExpression::BCK_MULTIPLY:
			{
				return m_PlatformSettings->m_IntMultiplicationDelay;
				break;
			}
		case BasicCallExpression::BCK_DIVISION:
			{
				return m_PlatformSettings->m_IntDivisionDelay;
				break;
			}
		default: return 1; // TODO: более развернутая проверка необходима. Возможно надо будет кидать исключения. 
		}
	}
	else 
		return m_PlatformSettings->m_ReadingWriting; // TODO: более развернутая проверка необходима

	// для порядка:
	//return 1;
}

int CalculationGraph::getNodeOperationDelayByNodeNumber(int VertexNumber)
{
	return findNodeByNodeNumber(VertexNumber)->m_OperationDelay;
}

unsigned CalculationGraph::getSendingDelay(int BeginVertexNumber, int EndVertexNumber)
{
	return findDirEdgeByNodesNumbers(BeginVertexNumber, EndVertexNumber)->getSendingDelay();
}

void CalculationGraph::normalizeSubGraphSchedule(Int_Set SubGraph)
{
	if (SubGraph.size() > 0)
	{
		Int_Set::iterator itCurVertexNumber = SubGraph.begin();
		int min_s = m_StartTacts[*itCurVertexNumber];
		itCurVertexNumber++; // первая вершина уже учтена в min_s
		while (itCurVertexNumber != SubGraph.end())
		{
            min_s = std::min(min_s, m_StartTacts[*itCurVertexNumber]);
			itCurVertexNumber++;
		}

		// нормализуем по найденнному значению min_s
		itCurVertexNumber = SubGraph.begin();
		while (itCurVertexNumber != SubGraph.end())
		{
			m_StartTacts[*itCurVertexNumber] = m_StartTacts[*itCurVertexNumber] - min_s +1;
			itCurVertexNumber++;
		}
	}
}

/// Функция внешнего интерфейса, передающая всю необходимую для визуализации информацию
void CalculationGraph::getLists(vector<string> &rVertexNamesList, vector<int> &rVertexDelaysList, vector<string> &rDirEdgesList, vector<int> &rCriticalPath, vector<int> &rStartTacts, int &iii, int& rStartDelay) 
{
	rStartDelay = 0;
	getLists(rVertexNamesList, rVertexDelaysList, rDirEdgesList, rCriticalPath, rStartTacts, iii);
}

/// Функция внешнего интерфейса, передающая всю необходимую для визуализации информацию
void CalculationGraph::getLists(vector<string> &rVertexNamesList, vector<int> &rVertexDelaysList, vector<string> &rDirEdgesList, vector<int> &rCriticalPath, vector<int> &rStartTacts, int &iii)
{
	// Интервал Инициализации Итераций
	iii = m_iii;

	// Подготовка к выводу
	rVertexNamesList.clear();
	rVertexDelaysList.clear();
	rDirEdgesList.clear();
	rCriticalPath.clear();
	rStartTacts.clear();

	for(size_t i=1; i<=m_Nodes.size() ; i++)
	{
		CalculationGraphNode* pCurrentNode = findNodeByNodeNumber(i);
		if (pCurrentNode != 0) 
		{	// Вершина найдена
			rVertexNamesList.push_back(pCurrentNode->getStringRepresentation());
			rVertexDelaysList.push_back(pCurrentNode->m_OperationDelay);

			// Все дуги записываем в список, кроме уже записанных.
			// Поэтому используем только m_InDirEdges (и вдобавок проблема
			// перебора склееных вершин отпадает).
            for (CalculationGraphDirEdge* itCurrentDirEdge: pCurrentNode->m_InDirEdges)
				rDirEdgesList.push_back(itCurrentDirEdge->getStringRepresentation());
		}
	}

	// Передача критического пути
    for (CalculationGraphNode* itCurrentNode: m_CriticalPath)
	    rCriticalPath.push_back(itCurrentNode->m_NodeNumber);

	// Передача стартовых тактов всех операций
	if (m_StartTacts)
		for(size_t i = 1; i<=m_Nodes.size(); i++)
			rStartTacts.push_back(m_StartTacts[i]);
}

// Служебная
void CalculationGraph::printMatrixes()
{
	if (m_VertexMatrix != 0)
	{
		cout << endl << endl;
		for(size_t i=0; i<=m_Nodes.size(); i++)
		{
			for(size_t j=0; j<=m_Nodes.size(); j++)
				cout << m_VertexMatrix[i][j] << " ";
			cout << endl;
		}
	}

	cout << endl;
	
	if (m_DirEdgeTypesMatrix != nullptr)
	{
		for(size_t i=0; i<=m_Nodes.size(); i++)
		{
			for(size_t j=0; j<=m_Nodes.size(); j++)
				cout << m_DirEdgeTypesMatrix[i][j] << " ";
			cout << endl;
		}
	}
}

const vector<deque<int> > CalculationGraph::getReachabilityList(int &n)
{
	// Результат
	vector<deque<int> > ReachabilityList;
	n = m_Nodes.size();

	// Вспомагательный список достижимостей
	map<int, deque<int> > TempReachabilityList;

	// один элемент результата
	deque<int> CurNodeReachabilityList;

	// Перестановка вершин, соответствующая правильной нумерации map<номер в графе, номер по правильной нумерации>
	map<int, int> Rearrangement;

	bool** GraphWithoutBackDirEdges; // вершине с номером i будут соответствовать i-1 строка и i-1 столбец, чтобы не выделять лишнюю память
	bool** GraphCopy; // копия пред графа, для удаления вершин по алгоритму постр прав нумерации
    GraphWithoutBackDirEdges = new bool * [m_Nodes.size()];
    GraphCopy = new bool * [m_Nodes.size()];
	for(size_t i=0; i<m_Nodes.size(); i++)
	{
        GraphWithoutBackDirEdges[i] = new bool[m_Nodes.size()];
        GraphCopy[i] = new bool[m_Nodes.size()];
	}

	// копируем матрицу типов дуг без обратных
	for(size_t i=0; i<m_Nodes.size(); i++)
		for(size_t j=0; j<m_Nodes.size(); j++)
		{
			if ((m_DirEdgeTypesMatrix[i+1][j+1] != CalculationGraphDirEdge::DET_BACK) &&
				(m_DirEdgeTypesMatrix[i+1][j+1] != -1)) // дуга вообще есть, и она не обратная
			{
				GraphWithoutBackDirEdges[i][j] = true;
				GraphCopy[i][j] = true;
			}
			else
			{
				GraphWithoutBackDirEdges[i][j] = false;
				GraphCopy[i][j] = false;
			}
		}

	// Строим правильную нумерацию с конца и при этом заполняем список достижимостей
	for(int k = m_Nodes.size()-1; k>=0;)
	{
		int Prev_k = k;
		// Найдем хотя бы один сток
		for(size_t i=0; i<m_Nodes.size(); i++)
			if (Rearrangement[i] == 0) // вершина уже пронумерована?
			{
				bool bSink = true;
				for(size_t j=0; j<m_Nodes.size(); j++)
				{
					if (GraphCopy[i][j])
					{
						bSink = false;
						break;
					}
				}

				if (bSink)
				{
					// Очистка старых списков
					CurNodeReachabilityList.clear();

					// Вершина i (сток в текущем графе) получает очередной номер и изменяет номер
					Rearrangement[i] = k--;

					// заполняем текущий список достижимостей и эта вершина удаляется из рассмотрения 
					for(size_t j=0; j<m_Nodes.size(); j++)
					{
						// исходящие дуги
						if (GraphWithoutBackDirEdges[i][j])
						{
							CurNodeReachabilityList.push_back(j);
							//merge(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end(), TempReachabilityList[j].begin(), TempReachabilityList[j].end(), CurNodeReachabilityList.begin());
							deque<int>::iterator iter = TempReachabilityList[j].begin();
							while(iter != TempReachabilityList[j].end())
							{
								CurNodeReachabilityList.push_back(*iter);
								iter++;
							}
							// Сортируем смежные вершины
							sort(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end());
						}

						// входящие дуги
						if (GraphWithoutBackDirEdges[j][i])
						{
							// удаляем на копии
							GraphCopy[j][i] = false;
						}
					}

					// заполняем достижимости этой вершины
					
					/*deque<int> AdjacentVertexes;
					CalculationGraphNode* CurVertex = findNodeByNodeNumber(i);
					list<CalculationGraphDirEdge*>::iterator OutDirEdge = CurVertex->m_OutDirEdges.begin();
					while(OutDirEdge != CurVertex->m_OutDirEdges.end())
					{
						int Num = (*OutDirEdge)->getEnd()->m_NodeNumber;
						AdjacentVertexes.push_back(Num);

						merge(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end(), TempReachabilityList[Rearrangement[Num]].begin(), TempReachabilityList[Rearrangement[Num]].end(), CurNodeReachabilityList.begin());
						
						OutDirEdge++;
					}*/

					//// Сортируем смежные вершины
					//sort(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end());
					//// Добавляем смежные вершины
					//merge(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end(), AdjacentVertexes.begin(), AdjacentVertexes.end(), CurNodeReachabilityList.begin());

					// удаляем дубликаты в отсортированном деке
					unique(CurNodeReachabilityList.begin(), CurNodeReachabilityList.end());
						
					// Добавление в общий список 
					TempReachabilityList[i] = CurNodeReachabilityList;

				} // if (bSink)
			} // if (Rearrangement[i] == 0)

		if (Prev_k == k)
		{
			OPS::Console* const pConsole = &OPS::getOutputConsole(g_strGraphName);
			pConsole->log(OPS::Console::LEVEL_ERROR, _TL("Calculations Graph has loops after back-edges have been removed!",""));
			ReachabilityList.clear();
			return ReachabilityList;
		}
	} // for(int k ...

	// Копирование из map в vector
	for(size_t k = 0; k<m_Nodes.size(); k++)
		ReachabilityList.push_back(TempReachabilityList[k]);

	return ReachabilityList;
}

// **************************************************
// *** Global functions in CalculationGraph ***
// **************************************************

void uniteSets(Node_Set SourceSet, Node_Set &UniteResultSet)
{
    for (CalculationGraphNode* itCurrent: SourceSet)
		UniteResultSet.insert(itCurrent);
}

int getForNestDimensionCount(ForStatement* ForLoop)
{
	int iForCounter = 1;
	BlockStatement* pCurBlock;

	list<BlockStatement*> InnerBlocks;
	InnerBlocks.push_back(&ForLoop->getBody());

	while (!InnerBlocks.empty())
	{
		pCurBlock = InnerBlocks.front();
		InnerBlocks.pop_front();

		// Ищем вложенные
		BlockStatement::Iterator itStmtIter = pCurBlock->getFirst();

		// Цикл по всем операторам расположенным в pCurBlock
		for(; itStmtIter.isValid(); itStmtIter++)
		{
			StatementBase* pCurStatement = &(*itStmtIter);

			if (pCurStatement->is_a<ForStatement>())
			{
				iForCounter++;
				InnerBlocks.push_back(&(pCurStatement->cast_to<ForStatement>().getBody()));
			}

			if (pCurStatement->is_a<BlockStatement>())
			{
				InnerBlocks.push_back(&pCurStatement->cast_to<BlockStatement>());
			}

			if (pCurStatement->is_a<IfStatement>())
			{
				IfStatement& IfStmt = pCurStatement->cast_to<IfStatement>();
				InnerBlocks.push_back(&(IfStmt.getThenBody()));
				InnerBlocks.push_back(&(IfStmt.getElseBody()));
			}
		}
	}

	return iForCounter;
}


} // CalculationGraphSpace

namespace OPS
{
namespace PlatformSettingsSpace
{

const PlatformSettings* loadPlatformSettings(std::string PathToConfigurationFile)
{
	// Заглушка! TODO: сделать полноценный класс загрузки настроек
    PlatformSettings* Settings = new PlatformSettings();
	Settings->m_IntSumDelay = 1;
	Settings->m_IntMultiplicationDelay = 1;
	Settings->m_IntDivisionDelay = 1;
	Settings->m_ReadingWriting = 1;
	Settings->m_BooleanDelay = 1;
	Settings->m_CompareDelay = 1;
	return Settings;
}

}
}
