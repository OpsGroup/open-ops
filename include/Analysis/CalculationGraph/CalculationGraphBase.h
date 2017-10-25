#ifndef OPS_CALCGRAPH_BASE_
#define OPS_CALCGRAPH_BASE_

// STL includes
#include <cmath>
#include <list>
#include <deque>
#include <set>

// OPS includes
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "ExpressionGraph.h"

//TODO: позаменять int на unsigned по возможности (буферы, номера вершин)

namespace OPS
{
namespace Montego { class DependenceGraph; }

namespace PlatformSettingsSpace
{
	struct PlatformSettings
	{
		// Delays
		int m_IntSumDelay, m_IntMultiplicationDelay, m_IntDivisionDelay;
		//int m_DoubleSumDelay, m_DoubleMultiplicationDelay, m_DoubleDivisionDelay;
		int m_CompareDelay;
		int m_BooleanDelay;
		int m_ReadingWriting;

	};

    const PlatformSettings* loadPlatformSettings(std::string PathToConfigurationFile);
}

}

/// namespace для графа вычислений
namespace CalculationGraphSpace
{
extern const char* g_strGraphName;

// Forward definition
class CalculationGraphNode;

typedef std::set<int> Int_Set;
typedef std::set<CalculationGraphNode*> Node_Set;


/// Класс дуг в графе вычислений
class CalculationGraphDirEdge 
{
public:
    friend class CalculationGraph;
	friend class CalculationGraphBuilder;
	friend class CalculationGraphNode;

	// Тип дуги графа вычислений, определяемый алгоритмом построения остовного 
	// дерева + дуги инициализации
	enum EN_DirEdgeType
	{
		DET_NEUTRAL = 0,// нейтральная, еще без типа
		DET_TREE,		// древесная
		DET_STRAIGHT,	// прямая
		DET_ACROSS,		// поперечная
		DET_BACK,		// обратная
		DET_INITIALIZE	// дуга инициализации, определяется на этапе проверки out-in зависимостей
	};

	enum EN_SignalType
	{
		ST_DATAFLOW = 0,     // дуга служит для передачи данных
		ST_CONTROLFLOW_00,   // дуга служит для управления по сигналу 00
		ST_CONTROLFLOW_01,   // дуга служит для управления по сигналу 01
		ST_CONTROLFLOW_10,   // дуга служит для управления по сигналу 10
	    ST_CONTROL_DELAY     // дуга для указания явных задержек между операциями
    };

	/// Конструктор 
	CalculationGraphDirEdge(CalculationGraphNode* Begin, CalculationGraphNode* End, int DepDistence = 0, EN_SignalType SignalType = ST_DATAFLOW, int SendingDelay = 0);

	/// Получить задержку на пересылку, которая определена для этой дуги
	void setSendingDelay(unsigned SendingDelay) { m_SendingDelay = SendingDelay; } 

	/// Получить задержку на пересылку, которая определена для этой дуги
	unsigned getSendingDelay() { return m_SendingDelay; } 

	/// Получить начало дуги (безопасный вариант)
	CalculationGraphNode* getBegin() { return m_Begin; }

	/// Получить конец дуги (безопасный вариант)
	CalculationGraphNode* getEnd() { return m_End; }

	/// Получить строковое представление дуги
	std::string getStringRepresentation();

	/// Получить тип дуги
	EN_DirEdgeType getDirEdgeType() { return m_DirEdgeType; } 

	/// Получиь задержку в буффере, рассчитанную алгоритмом (TODO: возможно стоит хранить буферы в графе в виде map)
	unsigned getBuffer() { return m_BufferLatency; }

	/// Получиь размер буффера (кол-во эл-тов, которые одновременно могут храниться в буффере)
	unsigned getBufferSize() { return m_BufferSize; }

	/// Получить расстояние зависимости
	int getDependenceDistance() { return m_DependenceDistance; }

	/// Получить тип передаваемого по дуге сигнала
	EN_SignalType getSignalType() { return m_SignalType; }

	/**
		\brief Возвращает для обратной дуги имеющую с ней общий вход дугу инициализации
		\return 
		\retval NULL, если текущая дуга не обратная
		\retval указатель на дугу инициализации, если текущая дуга обратная и получена в результате out-in склейки
	*/ 
	CalculationGraphDirEdge* getInitializationDirEdge() { return m_InitializationDirEdge; } 

    std::pair<std::list<CalculationGraphDirEdge*>::iterator, std::list<CalculationGraphDirEdge*>::iterator> deleteOccurrences();

private:
	/// Перевести дугу в дугу инициализации (выполняется при склейке вершин)
	void convertDirEdgeToInitDirEdge() { m_DirEdgeType = DET_INITIALIZE; }

	/// Получить начало дуги (для внутреннего использования)
	CalculationGraphNode* getBeginUnsafe() { return m_Begin; }

	/// Получить конец дуги (для внутреннего использования)
	CalculationGraphNode* getEndUnsafe() { return m_End; }

	CalculationGraphNode* m_Begin;
	CalculationGraphNode* m_End;
	//LampArrow* ParentLamportDirEdge; // необходимо только для случая EN_GraphCreatingMode::AllDependences //TODO: разобраться!  
	EN_DirEdgeType m_DirEdgeType; // настраивается после создания графа
	EN_SignalType m_SignalType; // определяется сразу при создании
	int m_DependenceDistance; // расстояние зависимости дуги.
	unsigned m_BufferLatency; // задержка в буфере операции
	unsigned m_BufferSize; // размер буфера задержки
	unsigned m_SendingDelay; // задержка при пересылке
	CalculationGraphDirEdge* m_InitializationDirEdge; // для не обратных дуг =NULL, для обратных указывает на дугу, которая стала дугой инициализации после out-in склейки

};

/// Класс вершин в графе вычислений
class CalculationGraphNode 
{
public:
	
	enum EN_DataType
	{
		DT_EXPRESSION,
		DT_STATEMENT
	};

	friend class CalculationGraph;
    friend class CalculationGraphBuilder;
	friend class CalculationGraphDirEdge;
	friend class ExpressionsGraph<CalculationGraphNode, CalculationGraphDirEdge>;

	/// Конструктор
	CalculationGraphNode(OPS::Reprise::ExpressionBase* Data, int OpDelay, EN_NodeType Type = NT_PIPELINED); 

	/// Конструктор
	CalculationGraphNode(OPS::Reprise::IfStatement* Data, int OpDelay); 

	/// Деструктор
	~CalculationGraphNode() { clear(); };

	/// Добавляет входящую дугу
	CalculationGraphDirEdge* addInDirEdge(CalculationGraphNode* VertexFrom, int DepDistance = 0, CalculationGraphDirEdge::EN_SignalType SignalType = CalculationGraphDirEdge::ST_DATAFLOW);

	/// Добавляет исходящую дугу
	CalculationGraphDirEdge* addOutDirEdge(CalculationGraphNode* VertexTo, int DepDistance = 0, CalculationGraphDirEdge::EN_SignalType SignalType = CalculationGraphDirEdge::ST_DATAFLOW);

	/// Изменяет тип вершины
	void setType(EN_NodeType Type) { m_NodeType = Type; }

	/// Выдает тип вершины
	EN_NodeType getType() const { return m_NodeType; }

	/// Возвращает список входящих дуг
	std::list<CalculationGraphDirEdge*>* getInDirEdgeList() { return &m_InDirEdges; }

	/// Возвращает список исходящих дуг
	std::list<CalculationGraphDirEdge*>* getOutDirEdgeList() { return &m_OutDirEdges; }

	/// Возвращает строковое представление вершины
	std::string getStringRepresentation();

	/// Получить номер вершины в текущей нумерации
	int getNodeNumber() const { return m_NodeNumber; } //TODO: возможно имеет смысл возвращать номер из списка вершин
	 
    /// Установить номер вершины
    void setNodeNumber(const int num) { m_NodeNumber = num; }

	/**
		\brief Возвращает список вхождений (представленно \see ExpressionBase) из внутреннего представления, которым сопоставлена эта вершина
		\return 
		\retval пустой список, если вершина построена по оператору
		\retval список вхождений (\see ExpressionBase), если вершина построена по выражению
	*/
	std::list<OPS::Reprise::ExpressionBase*> getIRNodes();

	/**
		\brief Возвращает первое в смысле зависимостей по данным вхождение (представленно \see ExpressionBase), из списка всех вхождений, которым сопоставлена эта вершина.
		\return 
		\retval 0, если вершина построена по оператору
		\retval указатель на вхождение (\see ExpressionBase), если вершина построена по выражению
	*/
	const OPS::Reprise::ExpressionBase* getIRNode();

	/// Возвращает тип вершины с точки зрения данных (узла дерева Reprise)
	EN_DataType getDataType() const { return m_DataType; }

	/// Возвращает задержку выполнения операции
	int getOperationDelay() const { return m_OperationDelay; }

private:

	/// Очистить все списки и выделенную память
	void clear();

	std::list<CalculationGraphDirEdge*>::iterator findInDirEdgeIterator(CalculationGraphDirEdge* DirEdge);
	std::list<CalculationGraphDirEdge*>::iterator findOutDirEdgeIterator(CalculationGraphDirEdge* DirEdge);

	/// Порождающие вершину элемент внутреннего представления
	std::list<OPS::Reprise::RepriseBase*> m_Data; // набор вхождений соответствующих этой вершине
	int m_NodeNumber; // порядковый номер вершины
	std::list<CalculationGraphDirEdge*> m_InDirEdges; // набор всех дуг входящих из данной вершины
	std::list<CalculationGraphDirEdge*> m_OutDirEdges; // набор всех дуг исходящих из данной вершины
	int m_OperationDelay; // задержка операции
	EN_DataType m_DataType;
	EN_NodeType m_NodeType;
};

typedef std::vector<CalculationGraphNode*>::iterator NodeIterator;
typedef std::list<CalculationGraphDirEdge*>::iterator DirEdgeIterator;



/**
	\brief	Определяет является ли одномерный цикл конвейеризуемым.
	\param Loop - цикл for.
	\param DepGraph - для цикла Loop.
	\return	true - если цикл конвейеризуем,
			false - если цикл не конвейеризуем
*/
bool isPipelinableOneDimLoop(const OPS::Reprise::ForStatement* Loop, OPS::Montego::DependenceGraph* DepGraph = 0);


/**
	\brief Проверяет не мешает ли переданная истинная зависимость конвейеризации.
			ВНИМАНИЕ! Дуга может быть только от графа зависимостей построенного по этому циклу!
			Дуги зависмости, порожденные вышестоящими циклами, приведут к undefined behavior.
	\param DependenceArc - дуга в графе зазависимостей
 	\return информацию о зависимости в виде целого числа.
			\retval -3 - зависимость не рассматривается в рамках проекта "граф вычислений";
			\retval -2 - зависимость нерегулярна;
			\retval -1 - если эта дуга не влияет на граф вычислений в одноконвейерном случае (не добавляем к графу вычислений);
			\retval 0 - зависимость не циклическая и внутри конвейера;
			\retval >0 - расстояние зависимости, между зависимыми вхождениями внутри конвейера. 
*/
int calculateDependenceDistance(const OPS::Montego::DependenceGraphAbstractArc* DependenceArc);


/**
	\brief Проверяет не мешает ли переданная истинная зависимость конвейеризации.
			ВНИМАНИЕ! Дуга может быть только от графа зависимостей построенного по этому циклу!
			Дуги зависмости, порожденные вышестоящими циклами, приведут к undefined behavior.
	\param DependenceArc - дуга в графе зазависимостей
 	\return информацию о зависимости в виде целого числа.
			\retval -3 - зависимость не рассматривается в рамках проекта "граф вычислений";
			\retval -2 - зависимость нерегулярна;
			\retval -1 - если эта дуга не влияет на граф вычислений в одноконвейерном случае (не добавляем к графу вычислений);
			\retval 0 - зависимость не циклическая и внутри конвейера;
			\retval >0 - расстояние зависимости, между зависимыми вхождениями внутри конвейера. 
*/
int calculateDependenceDistance(OPS::Reprise::ExpressionBase* source, OPS::Reprise::ExpressionBase* depended);


// определяет как происходит обращение к этой переменной (зависимое вхождение) как к скаляру или как к массиву
bool isArrayAccess(const OPS::Reprise::ExpressionBase* expression);

} // end namespace CalculationGraphSpace

#endif //_CALCGRAPH_BASE_
