#pragma once

#include <list>

#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/LatticeGraph/PIP.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"

//#define LATTICE_TABLES_DEBUG

using namespace OPS;
using namespace Reprise;
using namespace DepGraph;


namespace OPS
{
namespace LatticeGraph
{

struct ExtendedQuast;


//все линейные выражения, по которым строится решетчатый граф, приведенные к одинаковому количеству коэффициентов
struct LinearExprDataForLatticeGraph
{
    //ВНИМАНИЕ: во всем, что касается m_srсEntry - m_srcEntry.loopNumb+m_externalParamNum+1 коэффициентов
    //а для m_depEntry - m_depEntry.loopNumb+m_externalParamNum+1
    //общие циклы относятся к m_srсEntry
    
    LinearExprDataForLatticeGraph();
    ~LinearExprDataForLatticeGraph();
    LinearExprDataForLatticeGraph(const LinearExprDataForLatticeGraph& other);
    LinearExprDataForLatticeGraph& operator=(const LinearExprDataForLatticeGraph& other);

    //информация о гнездах циклов вхождений
    std::vector<LoopDesc> m_loopDescSrcEntry;
    std::vector<LoopDesc> m_loopDescDepEntry;

    //массивы границ циклов, окружающих вхождения. Как обычно нумеруем, начиная с верхнего
    //у всех элементов массивов ОДИНАКОВОЕ число коэффициентов (просто некоторые равны нулю)
    //для srcEntry кол-во коэффициентов: m_loopNumbSrcEntry+m_externalParamNum+1
    //для depEntry кол-во коэффициентов: m_loopNumbDepEntryOnly+m_commonLoopsNumb+m_externalParamNum+1
    SimpleLinearExpression* m_loopLowerBoundForSrcEntry; 
    SimpleLinearExpression* m_loopUpperBoundForSrcEntry; 
    SimpleLinearExpression* m_loopLowerBoundForDepEntry; //циклы лично только DepEntry. Их m_loopNumbDepEntryOnly=m_depEntry->loopNumb-m_commonLoopsNumb штук
    SimpleLinearExpression* m_loopUpperBoundForDepEntry; //циклы лично только DepEntry. Их m_loopNumbDepEntryOnly=m_depEntry->loopNumb-m_commonLoopsNumb штук
    
    ////минимальные значения счетчиков циклов (они зависят только от внешних параметров, 
    ////а в границах циклов могу присутствовать и счетчики внешних циклов!)
    ////Они нужны для замен переменных, которые сохраняют порядок итераций в гнезде
    //SimpleLinearExpression* m_loopCounterMinForSrcEntry;
    //SimpleLinearExpression* m_loopCounterMaxForSrcEntry;
    //SimpleLinearExpression* m_loopCounterMinForDepEntry;//циклы лично только DepEntry
    //SimpleLinearExpression* m_loopCounterMaxForDepEntry;//циклы лично только DepEntry
    
    int m_loopNumbDepEntryOnly;
    int m_loopNumbSrcEntry;
    int m_commonLoopsNumb;
    int m_externalParamNum;
    /// Коэффиценты при счетчиках охватывающих циклов и внешних параметрах решетчатого графа
    std::vector<SimpleLinearExpression> m_coefsSrcEntry, m_coefsDepEntry;
    int m_coefsNumSrcDepEntry;//размерность массива (количество элементов в m_coefsSrcEntry и m_coefsDepEntry)
    //матрицы линейных преобразований, если эти линейные выражения были трансформированы
    int** m_transformMatrixSrcEntry; 
    int m_dimiSrcEntryTransformMatrix,m_dimjSrcEntryTransformMatrix;
    int** m_transformMatrixDepEntry; 
    int m_dimiDepEntryTransformMatrix,m_dimjDepEntryTransformMatrix;
    //матрицы обратных линейных преобразований, если эти линейные выражения были трансформированы
    int** m_invTransformMatrixSrcEntry; 
    int** m_invTransformMatrixDepEntry; 
    void buildTransformMatrixDepEntry();//заполняет матрицу m_transformMatrixDepEntry 
    void buildTransformMatrixSrcEntry();//заполняет матрицу m_transformMatrixSrcEntry 
    void findMinAndMaxCounterValues();//заполняет m_loopCounterMinForSrcEntry и другие такие же
    void clear();

    //Чтобы выполнить преобразование, нужно: 
    //1. Скопировать структуру методом copyAllExceptTransform (матрицы преобразований не копируются!!!!!!)
    //2. Заполнить матрицы преобразований
    //3. Выполнить преобразование методом applyTransform
    void applyTransform();//делает замену почти всего, согласно матрицам преобразований
    //копирует все поля из структуры other кроме матриц преобразований
	void copyAllExceptTransform(const LinearExprDataForLatticeGraph& other);
    //копирует все поля из структуры other
    void copyAllFrom(LinearExprDataForLatticeGraph& other);
    //подставляет значения внешних параметров
    //пока подставляет только в границы циклов, если нужно еще - связывайтесь с Гудой
    void substituteParams(const std::vector<int>& externalParamValuesVector);
};

class ElemLatticeGraph;//будет опредлен в этом файле ниже

//список служебных структур типа LinearExprDataForLatticeGraph. 
//Сделан так, чтобы можно было выполнить преобразования только в нужной последовательности
struct LinData: public LinearExprDataForLatticeGraph  {
    //делает полиэдральное представление графа, т.е. заполняет структуру m_linData
    void makeLinData(ElemLatticeGraph& graph);
    void newmakeLinData(ElemLatticeGraph& graph);
};

struct LinDataWithZeroLowerBounds : public LinearExprDataForLatticeGraph  {
    //делает преобразованияе циклов НЕ к нулевым нижним границам, а к нулевым минимальным значениям счетчиков
    void makeZeroLowerBounds(LinData &linData);
};

struct LinDataForTrueDep : public LinearExprDataForLatticeGraph   {    
    //МЫ В КОНЦЕ build БУДЕМ ПОЛЬЗОВАТЬСЯ ФУНКЦИЕЙ ApplySimplex, которая находит лексикографический МИНИМУМ,
    //для РГ истинной зависимости нужен максимум, поэтому нужно делать замену:
    //i_k:=верхняя граница к-того цикла - i_k для счетчиков циклов srcEntry (со штрихами)
    void makeTransformForTrueDep(LinDataWithZeroLowerBounds& linDataWithZeroLowerBounds);
};


//TODO: надо сделать конструктором Context!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Polyhedron* GetIterationSpace(const OccurDesc& entry, int deepLimit=-1);//возвр. NULL, если действие окончено удачно
// В результате выполнения данной функции будет построен контекст, содержащий описание опорной области (по-сути - итерац прос-во)
// для вхождение "entry". Считается, что в описания границ циклов не входят внешние переменные.	

} // end namespace LatticeGraph
} // end namespace LatticeGraph


