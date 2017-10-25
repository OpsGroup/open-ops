#ifndef OPS_CALCGRAPH_
#define OPS_CALCGRAPH_

// STL includes
#include <cmath>
#include <list>
#include <deque>
#include <set>

// OPS includes
#include "Analysis/CalculationGraph/CalculationGraphBase.h"
#include "ExpressionGraph.h"

/// namespace для графа вычислений
namespace CalculationGraphSpace
{

/// Класс "Граф Вычислений" (только для одномерных циклов)
class CalculationGraph: public ExpressionsGraph<CalculationGraphNode, CalculationGraphDirEdge>, public OPS::NonAssignableMix
{
public:
	// Режим построения графа
	enum EN_GraphCreatingMode
	{
		GCM_RECONFIGURABLE_PIPELINE = 0,	// граф строится для архитектуры с перестраиваемым конвейером
		GCM_ALL_DEPENDENCIES,				// граф строится для всех зависимостей
		GCM_MULTYPIPELINE				// граф строится для всех зависимостей
	};

	/// Конструктор
    explicit CalculationGraph(EN_GraphCreatingMode GraphType, EN_NodeType DefaultNodeType = NT_PIPELINED);

	/// Конструктор копии
	CalculationGraph(const CalculationGraph& Original);

	/// Деструктор
    ~CalculationGraph() { clear(); }
	
	/**
		\brief Строит граф вычислений.
		Эта функция позволяет создать граф вычислений, но не выполняет алгоритмов расчета различных параметров конвейеризации.
		\param Loop - цикл, по которому будет строиться граф вычислений;
		\param PathToConfigurationFile - путь к файлу с настройками ОРС.
		\return 
		\retval true, если построение графа прошло успешно.
		\retval false, если нарушено хоть одно требование связанное с конвейеризацией цикла \see Loop.
	*/
	bool createGraph(OPS::Reprise::ForStatement* Loop, const std::string PathToConfigurationFile);

	/// Находит вершину графа по ее номеру
	CalculationGraphNode* findNodeByNodeNumber(int i);

	/// Находит дугу графа по номерам вершин
	CalculationGraphDirEdge* findDirEdgeByNodesNumbers(int Begin, int End);

	/// Возвращает итератор на первую вершину
	NodeIterator begin() { return m_Nodes.begin(); }

	/// Возвращает итератор за последней вершиной
	NodeIterator end() { return m_Nodes.end(); }

	/**
		\brief Запускает алгоритмы рассчета параметров конвейеризации
		Эта функция рассчитывает iii, буферы задержек, определяет обратные дуги и строит расписание выполнения конвейерных вычислений.
	*/
	void executeAllAlgorithms();

	/// Выдает списки содержащие результаты работы алгоритмов (НОВАЯ)
	void getLists(std::vector<std::string> &rVertexNamesList, std::vector<int> &rVertexDelaysList, std::vector<std::string> &rDirEdgesList, std::vector<int> &rCriticalPath, std::vector<int> &rStartTacts, int &iii, int& rStartDelay);

	/// Выдает списки содержащие результаты работы алгоритмов (СТАРАЯ)
	void getLists(std::vector<std::string> &rVertexNamesList, std::vector<int> &rVertexDelaysList, std::vector<std::string> &rDirEdgesList, std::vector<int> &rCriticalPath, std::vector<int> &rStartTacts, int &iii);

	/// Определяет расстояние зависиомсти для двух вершин по их номерам
	int getDependenceDistanceByNodesNumbers(int Begin, int End);

	/// Получить задержку пересылки по дуге содиняющей две вершины
	unsigned getSendingDelay(int BeginVertexNumber, int EndVertexNumber);

	/// Определяет по номеру CalculationGraphNode, и возвращает задержку в этой вершине
	int getNodeOperationDelayByNodeNumber(int VertexNumber);

	/// Определяет по Data тип операции, а следовательно и задержку
	int getNodeOperationDelayByExpressionBase(OPS::Reprise::ExpressionBase* Data) const;

	/// Выдает список достижимостей графа вычислений с исключенными обратными дугами 
	const std::vector<std::deque<int> > getReachabilityList(int &n);

	/// Добавляет дугу в граф вычислений
	CalculationGraphDirEdge* addDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistance = 0, CalculationGraphDirEdge::EN_SignalType SignalType = CalculationGraphDirEdge::ST_DATAFLOW);

	/// Загружает информацию о задержках операций
	bool loadOperationsDelays(std::string PathToConfigurationFile);

	/// Выгружает списки, получаемые функцией getLists, в файл
	void dump2File(std::string name);
private:
	///Очищает всю память занятую вторичными вычислениями
	void clearSecondaryCalculations();

	/// Полностью очищает данные-члены графа
	void clear();

	/// Присоединяет уже созданный граф вычислений к графу вычислений this
	void takeUpAnotherGraph(CalculationGraph* AnotherGraph);

	
//	CalculationGraphNode* findNodeByExpressionBase(ExpressionBase* a);


////////////////////////////////////////////////////////////
//! Набор вспомогательный функций необходимых для работы ///
//! основных алгоритмов вычисления параметров конвейера  ///
////////////////////////////////////////////////////////////

	/// Выполняет все инициализирующие алгоритмы
	void executeInitAlgorithms();

	/// Заполняет матрицу смежностей
	void fillVertexMatrix();

	/// Создает вспомогательную вершину источник (с номером 0)
	void generateSourceVertex();

	/// Строит критический путь
	void buildCriticalPath();

	/// Строит остовное дерево методом поиска в глубину
	void buildSpanningTree();

	/// Запускает обе функции типизации дуг в матрице смежностей и на графе
	void typifyDirEdges();

	/// Определяет типы для всех дуг в матрице смежностей
	void typifyDirEdgesInMatrix();

	/// Определяет типы всех дуг на графе по уже полученнным типам дуг в матрице смежностей
	void typifyDirEdgesInGraphAfterMatrix();

	
///////////////////////////////////////////
//! Набор алгоритмов вычисляющих буферы ///
///////////////////////////////////////////

	/// Разбивает граф зависимостей на подграфы. Вызывается из \see calculateBuffers()
	void separationOfGraph(std::list<Int_Set> &SubGraphs);
	void calculateScheduleOfSubGraphs(std::list<Int_Set>& SubGraphs);
	void uniteCalculatedSubGraphs(std::list<Int_Set> &SubGraphs);
	void calculateBuffersOnBackDirEdges();
	void calculateStartTactsOfRWVertexes();
	void normalizeSchedule();

	// Выполняет все алгоритмы (основная)
	
	/// Алгоритм вычисления интервала инициализации итераций
	void calculate_iii();
    void calculateBackEdge_iii(CalculationGraphDirEdge* itCurrentBackDirEdge, int** labels);

	/// Алгоритм вычисления буферов
	void calculateBuffers();

	/// Сложить все расстояния зависимостей по циклу Cycle
	double sumCycleDependenceDistancies(std::list<CalculationGraphDirEdge*> Cycle);

	/// Сложить все задержки операций по циклу Cycle
	double sumCycleOperationsDelays(Int_Set Cycle);
		
	/// Функция поиска начала древесной дуги по ее концу
	int findBeginOfTreeDirEdgeByEnd(int End);

	/// Нормализация расписания подграфа
	void normalizeSubGraphSchedule(Int_Set SubGraph);

	/// Устаревшая
	void saveToFiles();

	/// Служебная (для отладки)
	void printMatrixes();

	// Данные-члены
	EN_GraphCreatingMode m_Type;
	std::list<CalculationGraphNode*> m_CriticalPath; // набор вершин входящих в критический путь
	std::list<CalculationGraphDirEdge*> m_Uback; // множество обратных дуг
	
//	unsigned m_NumberOfNodes; // количество вершин графа
	int** m_VertexMatrix; // матрица смежности
	int** m_DirEdgeTypesMatrix; // матрица, в которой указаны номера типов дуг графа 
							// по классификации EN_DirEdgeType
	int m_iii; // интервал инициализации итераций (= 0, когда еще не рассчитан)
    const OPS::PlatformSettingsSpace::PlatformSettings* m_PlatformSettings; // считываемые из конфигурационного файла параметры платформы, включая задержки по данным
	int* m_StartTacts; // стартовые такты для всех вершин

};

/**
	\brief Вычисляет глубину вложенности циклов, начиная с ForLoop
	\param ForLoop - внешний цикл для всего гнезда циклов (проверка того, что он самый внешний не делается).
*/
int getForNestDimensionCount(OPS::Reprise::ForStatement* ForLoop);


/**
	\brief Операция объединения множеств вершин
	Заменяет второе множество на объединение с первого и второго множества
	\param SourceSet - первое множество вершин графа вычислений;
	\param UniteResultSet - второе множество вершин графа вычислений, вместо которого будет записан результат объединения по завершении алгоритма.
*/
void uniteSets(Node_Set SourceSet, Node_Set &UniteResultSet);


//bool isPipelinableOneDimLoop(OPS::Reprise::ForStatement* Loop);
} // end namespace CalculationGraphSpace

#endif //_CALCGRAPH_
