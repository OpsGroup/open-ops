#include "Analysis/LatticeGraph/LatticeGraph.h"
#include "Analysis/LatticeGraph/LoopIndex.h"
#include "Analysis/DepGraph/DepGraph.h"

//#define LATTICE_TABLES_DEBUG

using namespace OPS::Reprise;
using namespace DepGraph;

namespace OPS
{
namespace LatticeGraph
{
struct ArrowFunc;

/// Решение Воеводина: набор функций, описывающих дуги решетчатого графа. 
class VoevodinSolution : public std::list<ArrowFunc*>,public OPS::NonCopyableMix {
public:
    std::string toString();
    void clear();
    void SimplifyAreas();
    void Print(const char* fileName,EN_PrintModes printMode);
private:
};

/*
Функция, описывающая дугу решетчатого графа в решении Воеводина
*/
struct ArrowFunc
{
    /// Двумерный массив - система, описывающая функцию дуги графа. Последний элемент каждой строки является св. членом
    int** data;

    /// Кол-во строк массива data
    int dimi;

    /// Кол-во столбцов массива data
    int dimj;

    /// Номер альтернативного многогранника, связанного с данной функцей
    int altPolyNumb;

    /// Область определения функции.
    GenArea areas;

    /// Список, описывающий уравнения для добавленных параметров (в случае, когда линейными функциями граф не описывается)
    NewParamVector* newParamList;

    /// Конструкторы
    ArrowFunc():data(NULL),dimi(0),dimj(0),newParamList(NULL){}
    ArrowFunc(int varNum,int paramVarNum,int** _data=NULL);
    ArrowFunc(const ArrowFunc& arrowFunc);

    /// Деструктор
    ~ArrowFunc(){Clear();}

    ArrowFunc& operator=(const ArrowFunc& arrowFunc);

    /// Очистить граф.
    void Clear() {}

    /// По концу дуги (sink) возвращает начало дуги (source). Предполагается, что sink принадлежит О.О.Ф. этой функции.
    void GetSource(LoopIndex& source,const LoopIndex& sink);
};

///// По строке из функции дуг, именам переменных и Namespace, в котором эти переменные видны, строится выражение-дерево
///// Память освобождает тот, кто пользуется
///// line - номер строки, начиная с 0.
//ExpressionBase* GetExprNodeFromRowFuncLine(ArrowFunc& arrowFunc,int line,Declarations& ns,std::string* varNames);
//
///// Функция возвращает носитель зависимости по значению используя номер альтернативного многогранника для функции (в формуле отсутствует "+1" т.к. номера носителей в реализации начинаются с 0 (а в работе - с 1)
//inline int GetValueBasedSupp(ArrowFunc& af,int commonLoopNumb){return af.altPolyNumb==0?LIDEP:(commonLoopNumb-af.altPolyNumb);}
//
///// Построить обратную фукцию дуг к заданной (если удастся)
///// Возвращает 0, в случае удачи.
//int GetInvertedArrowFunc(const ArrowFunc& srcArrowFunc, ArrowFunc& invertedArrowFunc);

/*
Функция, описывающая дугу простого решетчатого графа. Это ArrowFunc, снабженный информацией о вхождениях
*/
//struct LatticeDataElem //: public OPS::NonCopyableMix
//{
//    ///функция, описывающая дуги
//    ArrowFunc arrowFunc;		
//
//    ///далее идет описание вхождения, которое порождает начала дуг решетчатого графа
//    ///	!! В случае изменения внутр. представления следующая информация (или ее часть) может потерять актуальность
//
//    ///указатель на оператор, которому оно принадлежит 
//    StatementBase* pStmt;		
//
//    ///указатель на данное вхождение во внутреннем представлении
//    const ReferenceExpression* pOccur;	
//
//    /// описание вхождения
//    OccurDesc* occurDesc;
//
//    /// номер вхождения во фрагменте программы.
//    int occurNumb;
//
//    ///Количество общих циклов для рассматриваемых вхождений
//    int commonLoopsNumb;
//
//    ///имена переменных и параметров. Тоже, что и в классе ElemLatticeGraph
//    std::string* varNames;	
//
//    //LatticeDataElem():pStmt(NULL),pOccur(NULL),occurDesc(NULL),varNames(NULL){}
//
//    LatticeDataElem(ArrowFunc& _arrowFunc,StatementBase* _pStmt,const ReferenceExpression* _pOccur,OccurDesc* _occurDesc,int _occurNumb,int _commonLoopsNumb,std::string* _varNames);
//
//    LatticeDataElem(const LatticeDataElem& latticeDataElem);
//
//    ~LatticeDataElem();
//
//private:
//    LatticeDataElem& operator=(const LatticeDataElem& latticeDataElem){}//ДОПИСАТЬ !
//};
//
//typedef std::list<LatticeDataElem*>::iterator LatticeDataElemIterator;
//typedef std::list<LatticeDataElem*>::const_iterator ConstLatticeDataElemIterator;


}//end of namespace
}//end of namespace
