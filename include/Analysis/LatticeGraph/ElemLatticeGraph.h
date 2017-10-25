#pragma once
#include <list>

#include "Analysis/LatticeGraph/PIP.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"
#include "Analysis/LatticeGraph/LoopIndex.h"
#include "Analysis/LatticeGraph/LinDataForLatticeGraph.h"

using namespace DepGraph; 

namespace OPS {namespace Montego {
    class Occurrence;
    class AliasInterface;
}}

namespace OPS
{
namespace LatticeGraph
{

struct ExtendedQuast;

std::vector<int> combineLoopIndexDataAndExtParamValues(LoopIndex& sink, const std::vector<int>& externalParamValuesVector);

enum EN_LatticeGraphStatus
{
	LG_EMPTY=1,//нет решения -> реш граф не содержит дуг
	LG_NONLINEAR_SOLUTION=2,//решение существует, но оно описывается нелинейной функцией (в общем случае - часть функций имеют не линейный вид (не обязательно все!)
	LG_ERROR_INIT=4,//граф не построен из-за ошибки при инициализации
	LG_BUILT_BY_INVERTING=8,//граф построен инвертированием матрицы индексных выражений
	LG_PARAMETRIC_GRAPH=16,// граф параметризирован (т.е. в описании графа участвуют внешние переменные)
	LG_DONT_TRY_BUILD_BY_INVERTING=64//Запрещает при построении графа попытку построить его простым инвертированием матрицы индексного выражения (необходимо до лучших времен: когда при инвертировании матрицы будет и носитель правильно строиться)
};

/*
	Элементарный (снизу) решетчатый граф.
    Внимание: теперь решетчатый граф может получится зависящим от внешних параметров.
    Проверку на зависимость от внешних параметров осуществить очень просто:
    достаточно сравнить m_externalParamNum с нулем
*/
class ElemLatticeGraph : public DepGraph::Status
					   , public OPS::NonCopyableMix
{
public:
    
    //Конструктор. 
    //Заметим, что в языке Си вхождения могут одновременноя являться генераторами и использованиями.
    //Для двух таких вхождений-генераторов-использований A[F(I)] и B[G(I)] можно построить 4 различных графа:
    //два истинной зависимости и два антизависимости (графы выходной зависимости совпадают с графами истинной),
    //подавая на вход конструктору вхождения в различном порядке.
    //Дуги графа идут следующим образом:
        //для истинной или выходной зависимости: srcEntry(генератор) -> depEntry
        //для антизависимости: depEntry -> srcEntry(генератор)
    //flagIncludeDepAtSameIter - включать ли петли в решетчатый граф
    //arbitraryParamVector - список внешних параметров, которые могут принимать значения произвольного знака
    //Если = NULL, то все внешние параметры считаются произвольного знака
    ElemLatticeGraph(DepGraph::OccurDesc& srcEntry, DepGraph::OccurDesc& depEntry, DepGraph::EN_DepType depType, bool flagIncludeDepAtSameIter
            , std::vector<OPS::Reprise::VariableDeclaration*>* arbitraryParamVector = NULL);

    //Конструктор для новых вхождений Montego.
    //Заметим, что в языке Си вхождения могут одновременноя являться генераторами и использованиями.
    //Для двух таких вхождений-генераторов-использований A[F(I)] и B[G(I)] можно построить 4 различных графа:
    //два истинной зависимости и два антизависимости (графы выходной зависимости совпадают с графами истинной),
    //подавая на вход конструктору вхождения в различном порядке.
    //Дуги графа идут следующим образом:
    //для истинной или выходной зависимости: srcEntry(генератор) -> depEntry
    //для антизависимости: depEntry -> srcEntry(генератор)
    //program - блок, который содержит анализируемое гнездо циклов. Циклы снаружи блока не рассматриваются, а
    //их счетчики считаются внешними параметрами.
    //ai - информация об альясах (нужна для проверки возможности построения РГ)
    //flagIncludeDepAtSameIter - включать ли петли в решетчатый граф
    //arbitraryParamVector - список внешних параметров, которые могут принимать значения произвольного знака
    //Если = NULL, то все внешние параметры считаются произвольного знака
    ElemLatticeGraph(OPS::Reprise::StatementBase& program, OPS::Montego::AliasInterface& ai,
        OPS::Montego::Occurrence& srcEntry, OPS::Montego::Occurrence& depEntry, 
        OPS::Montego::DependenceGraphAbstractArc::DependenceType depType, bool flagIncludeDepAtSameIter,
        std::vector<OPS::Reprise::VariableDeclaration*>* arbitraryParamVector = NULL);

    ~ElemLatticeGraph();

    ExtendedQuast* getFeautrierSolution();
    ExtendedQuast* getFeautrierSolutionInInitialVars();

    std::string toString();

    //возвращает true, если граф построен правильно. (Выполнение может занять много времени - это перебор. Зато - 100% результат!)
	bool selfTest(std::map<OPS::Reprise::VariableDeclaration*,int>& externalParamValuesMap);

	///	Массив имен счетчиков для вхождения-источника
	std::vector<std::string>& getSrcEntryCounterNames();

	///	Массив имен счетчиков для зависимого вхождения
	std::vector<std::string>& getDepEntryOnlyCounterNames();

    ///	Массив имен внешних параметров
    std::vector<std::string>& getExternalParamNames();
    std::vector<OPS::Reprise::VariableDeclaration*>& getExternalParamsVector();

    ///получить количество внешних параметров
    int getExternalParamNum();

	/// Получить указатели на описания вхождений. Для антизависимости src - это зависимое вхождение, dep - это источник зависимости
    DepGraph::OccurDesc* getSrcOccurDesc();
    DepGraph::OccurDesc* getDepOccurDesc();
    OPS::Montego::BasicOccurrence* getSrcOccurrence();
    OPS::Montego::BasicOccurrence* getDepOccurrence();

public:

    /// Получить кол-во общих циклов для зависимых вхождений
    int getCommonLoopsNumb();

    /// Получить начало дуги по концу.
    /// возвращает не 0, если дуга с концом sink существует. source будет содержать описание начала дуги
    int GetSource(LoopIndex& source,const LoopIndex& sink);

    /// Проверить возможность построения графа для данных вхождений.
    /// Возвращает true, если граф постоить возможно, иначе false;
    static bool TestApplicability(OccurDesc* srcEntry, OccurDesc* depEntry);
    bool TestLinearCoefsAndLoops(OPS::Montego::BasicOccurrence* srcEntry, OPS::Montego::BasicOccurrence* depEntry);
    bool TestExternalParamConst();

    ////static bool TestFragmentApplicability(Statement* first, Statement* last);

    static bool TestFragmentApplicability(Id::id& stmtIndex);			
    bool newTestFragmentApplicability(OPS::Reprise::StatementBase& block, OPS::Reprise::StatementBase& occurrenceStmt);			

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!Исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //	//Проверка, что Id указывает только на фрагмент допустимых операторов (true)
    //	static bool TestAcceptableStmts(Id::id& stmtIndex);

    LinData getLinData();

    //возвращает m_isLoopsPresented
    bool isLoopsPresented();

    OPS::Reprise::StatementBase* getProgramFragment();

private:

    //Решение в виде дерева if-ов Фотрье, расширенного сложными условиями. В терминах положительных переменных и параметров 
    ExtendedQuast* m_FeautrierSolution;
    ExtendedQuast* m_FeautrierSolutionInInitialVars;

    ///указатели на описания вхождений, для которых построен граф
    OccurDesc* m_srcEntry,*m_depEntry;
    Montego::BasicOccurrence* m_newsrcEntry,*m_newdepEntry;

    //фрагмент программы, в котором находится рассматриваемое гнездо циклов
    StatementBase* m_program;

    //информация об альясах (для проверки применимости)
    Montego::AliasInterface* m_aliasInterface;

    //тип зависимостей графа
    EN_DepType m_depType;
    Montego::DependenceGraphAbstractArc::DependenceType m_newdepType;

    ///Количество общих циклов для рассматриваемых вхождений
    int m_commonLoopsNumb;

    //Список и порядок следования внешних параметров
    std::vector<OPS::Reprise::VariableDeclaration*> m_externalParamsVector;
    //список тех внешних параметров, которые могут быть отрицательными
    std::vector<OPS::Reprise::VariableDeclaration*> m_arbitraryParamVector;
    ///количество внешних параметров
    int m_externalParamNum; 

    /// Массив имен счетчиков циклов m_srcEntry
    std::vector<std::string> m_srcEntryCounterNames;
    /// Массив имен счетчиков циклов m_depEntry (первые m_commonLoopsNumb имен - те же, что и у m_srcEntry)
    std::vector<std::string> m_depEntryCounterNames;
    /// Массив имен внешних параметров
    std::vector<std::string> m_externalParamNames;

    //могут ли быть петли в решетчатом графе
    bool m_loopsPresented;

    //строить ли петли в решетчатом графе, если они есть
    bool m_flagIncludeDepAtSameIter;

    /// Построить граф
    void build(bool flagAllExternalParamsAreArbitrary);
    void newbuild(bool flagAllExternalParamsAreArbitrary);

    /// Очистить граф
    void clear();

    //строит список внешних параметров m_externalParamsVector
    void makeExternalVariableVector();
    void newmakeExternalVariableVector();

    //Вместо внешних параметров произвольного знака вводим положительные параметры (их получится на 1 больше)
    //    void makeExtParamPositive(std::vector<OPS::Reprise::VariableDeclaration*>& arbitraryParamVector); - НЕ НАДО. НАШ СИМПЛЕКС МЕТОД РАБОТАЕТ И ДЛЯ ОТРИЦАТЕЛЬНЫХ ПАРАМЕТРОВ

    //все линейные выражения, по которым строится решетчатый граф (их удобно хранить в одном месте)
    LinData m_linData;

    LinDataWithZeroLowerBounds m_linDataWithZeroLowerBounds;

    //строим пространства итераций для вхождений
    void makeIterationSpaces(LinearExprDataForLatticeGraph& linData);
    Polyhedron m_iterationSpaceForDepEntry;
    Polyhedron m_iterationSpaceForSrcEntry;
    Polyhedron* m_suppPolyhedronSrcEntry;
    Polyhedron* m_suppPolyhedronDepEntry;

    //содержит полиэдральную модель РГ после преобразования i'=b-i для НЕ всех счетчиков циклов
    //нужно для графа истинной зависимости, т.к. 
    //МЫ В КОНЦЕ build БУДЕМ ПОЛЬЗОВАТЬСЯ ФУНКЦИЕЙ ApplySimplex, которая находит лексикографический МИНИМУМ,
    //для РГ истинной зависимости нужен максимум, поэтому нужно делать замену:
    //i_k:=верхняя граница к-того цикла - i_k для счетчиков циклов srcEntry (со штрихами)
    LinDataForTrueDep m_linDataForTrueDep;

    void buildFeautrierSolutionInInitialVars();

    //TODO: исправить!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
    int RowExists(Polyhedron* fromCon,Polyhedron* toCon);
    //возвр. 1, если в графе существует дуга из области fromCon в область toCon
    //предполагается, что указатели являются "valid" и указывают на НЕПУСТЫЕ (feasible) контексты
    //И граф описывается ЛИНЕЙНЫМИ функциями

};
}//end of namespace
}//end of namespace
