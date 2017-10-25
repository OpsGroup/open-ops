#ifndef _DEPGRAPH_
#define _DEPGRAPH_

#include <iostream>
#include <list>
#include "id.h"

#include "Status.h"
#include "LoopStruct.h"

namespace DepGraph
{


/// Различные типы зависимости
enum EN_DepType
{
	/// Истинная (потоковая) зависимость. От генератора к использованию.
	FLOWDEP = 1,
	/// Антизависимость. От использования к генератору.
	ANTIDEP = 2,
	/// Выходная зависимость. От генератора к генератору.
	OUTPUTDEP = 4,
	/// Входная зависимость. Между использованиями.
	INPUTDEP = 8
};

/// Носитель зависимости
enum EN_DepSupp
{
	/// Нет зависимости (соответственно нет носителей).
	NODEP = -1,
	/// Циклически независимая (Loop Independent).
	LIDEP = -2,
	/// Все возможные носители
	ANYSUPP = -3
};

/// Набор флагов, которые используются для описания вхождения (структура OccurDesc)
enum EN_VarEntryStatus
{
	/// Нелинейное выражение
	NONLINEAR = 1,
	/// Внешние переменные в свободном члене выражения
	FREE_IS_PARAMETER = 2,
    /// Внешние переменные при счетчике цикла
	INDEX_COEF_IS_PARAMETER = 4,
	EXAMINED = 8,
	IS_GENERATOR = 16,
	UNKNOWN_BOUNDS = 32
};

/// Набор флагов, которые используются для описания параметров дуги 
/// графа информационных зависимостей (структура LampArrow)
enum EN_LampArrowStatus
{
	/// Нелинейное выражение относительно счетчиков циклов в индексном выражении.
	LAS_CR_EXTERNAL_NONLINEAR_EXPR_IN_SUBSCRIPT = 1,
	
	/// Причина создания дуги - внешняя переменная в индексном выражении одного из вхождений.
	/// !ВНИМАНИЕ! В текущей реализации этот флаг не установливается, если установлен флаг ..NONLINEAR_EXPR_IN_SUBSCRIPT
	LAS_CR_EXTERNAL_VAR_IN_SUBSCRIPT = 2,
	/// Причина создания дуги - нет описания некоторых циклов (в данный момент это означает, что выражение границы цикла либо нелинейно, либо содержит внешние переменные и т.д.).
    LAS_CR_SOME_COMMON_LOOPS_WASNT_PARSERED_CAREFULLY = 4
};


/// Набор флагов, которые определяют параметры построения графа информационных зависимостей
enum EN_LamportGraphBuildParams
{
	LMP_DEFAULT = 0,
	/// Собирать информацию о входных зависимостях
	LMP_CONSIDER_INPUTDEP = 1
};

/// Параметры построения информации о вхождениях указанного фрагмента программы (ф-ция BuildVO(...))
enum EN_OccurListBuildParams 
{
	OLBP_DEFAULT = 0,

	/// При построении будет осуществлен поиск операторов цикла, содержащих указанный фрагмент программы 
	/// Если этот флаг не установлен, то будут учитываться только те циклы, 
	/// которые принадлежат указанному фрагменту.
	OLBP_SEARCH_OUTERLOOPS = 1,
};

/// Параметры печати чего-либо
enum EN_PrintModes
{
	/// При печати использовать русский язык; излагать в популярной форме.
	PM_POPULAR_RUSSIAN = 1,
	/// При печати использовать английский язык; излагать в удобной для разработчика форме
	PM_DEVELOPER_ENGLISH = 2,
	PM_DEVELOPER_RUSSIAN = 3,
	PM_POPULAR_ENGLISH
};

/// Параметры метода уточнения решетчатого графа на основе различных решетчатых графов
enum EN_RefineMethod
{
	/// Не выполнять никаких улучшений
	RM_NO_REFINEMENT = 0,
	
	/// Попытаться улучшить что-либо (напрмер, граф Лампорта) используя элементарные решетчатые графы
	RM_ELEM_LATTICE = 1,
	
	/// Попытаться улучшить что-либо используя минимальные снизу решетчатые графы. 
	/// Не все из таких графов описывают зависимость по значению (см. мой диссер).
	RM_BELOW_LATTICE = 2,
	
	/// Попытаться улучшить что-либо используя только те решетчатые графы, которые описывают 
	/// зависимость по значению (см. мой диссер). Если улучшать граф Лампорта, то только 
	/// после этого, граф Лампорта будет действительно описывать зависимость по значению.
	RM_VALUE_LATTICE = 3 
};



/**
	Структура, описывающая один(!) цикл.
**/

struct LoopDesc
{
    LoopDesc();

    LoopDesc(int lower, int upper, int step);


    /// Имя переменной счетчика цикла
	OPS::Reprise::VariableDeclaration* counterIter;				

	/// Указатель на сам оператор цикла
	OPS::Reprise::ForStatement* stmtFor;

	/// l & u могут быть аппроксимированы. 
	///   ЗАМЕЧАНИЕ: если соотв. граница охватывающего цикла имеет BoundUnk=1, 
	///   данная аппроксимация не соответствует действительности
	/// Нижняя граница цикла
	int l;
	/// Верхняя граница цикла
	int u;
	/// Шаг цикла
	int s;

	LoopBounds loopBounds;

	/// Если флаг равен true, то макс. значение соотв. границы мы не знаем. 
	/// Для таких границ проверяем только НОД тест (и то в зависимости от индексных выражений)
	bool lBoundUnk;
	bool rBoundUnk;

    /// Если флаг равен true, то значение соответствующей границы нелинейно зависит от внешних параметров
    bool lBoundNotLinear;
    bool rBoundNotLinear;

};

/// строит описание гнезда циклов данного вхождения
/// рассматриваются, только циклы, содержащиеся внутри code
/// Если code = 0, возвращает все циклы
std::vector<DepGraph::LoopDesc> buildLoopDesc(OPS::Reprise::ExpressionBase* expr,
											  OPS::Reprise::RepriseBase* code = 0);


/**
	Структура, описывающая вхождение и являющаяся элементом контейнера в классах, 
	содержащих информацию о вхождениях. 
*/
struct OccurDesc : public Status
{
	/// Количество циклов в опорном гнезде для данного вхождения.
	int loopNumb;

	/// Размерность вхождения переменной
	int dim;

	/// Коэффиценты при счетчиках охватывающих циклов.
	/// index data (data[x][loopNumb] == свободный член
	int** data;

	/// Коэффициенты при внешних параметрах и ссылки на эти параметры (таких линейных выражений должно быть ровно dim штук)
	std::vector<OPS::Shared::CanonicalLinearExpression> m_externalParamCoefs;

	/// Ссылка на имя переменной во внутреннем представлении
	const OPS::Reprise::VariableDeclaration* m_varDecl;					

	///	d1 эквивалентно data, НО аппроксимировано до одномерного массива; ub верхняя граница размерностей переменной
	std::vector<int> ub, lb;
	int* d1;
	/// То же самое, что и m_externalParamCoefs, но аппроксимированно до 1мерного массива
	OPS::Shared::CanonicalLinearExpression m_externalParamCoefsMapped;

	/// Указатель на данное вхождение во внутреннем представлении
	OPS::Reprise::ReferenceExpression* pOccur;

	/// массив описаний охватывающих данное вхождение циклов
	LoopDesc* loops;

    /// Признак линейности индексных выражений/!!! 
    /**
    если m_isIndexesLinear=false (т.е. нелинейность), то dim<>0, а d1==data==NULL !!!
    более точно: оно используется в алгоритме построения Лампорт графа и 
    m_isIndexesLinear==false означает, что зависимость определить не удастся и 
    нужно предположить, что она есть
    **/
    bool m_isIndexesLinear;

    bool m_hasExtVarsOrNotLinear;

	/// Опорный многогранник для вхождения. Термин В.В. Воеводина. 
	/** 
		(В будущем нужно будет перейти к GenArea - когда будут рассматриваться условные операторы).
		Заполняется в функции SetData.(видимо временно).
	**/
    //если указатель == 0, то гнездо циклов данного вхождения не принадлежит линейному классу
    OPS::LatticeGraph::Polyhedron* suppPolyhedron;
    
	OccurDesc():loopNumb(0),dim(0),data(NULL),m_varDecl(NULL),d1(NULL),pOccur(NULL),loops(NULL){}
	OccurDesc(int _occurNumb,const OPS::Reprise::VariableDeclaration* _varDecl, OPS::Reprise::StatementBase* _pStmt, OPS::Reprise::ReferenceExpression* _pOccur,int ln,int _dim,bool _pris);
	OccurDesc(const OccurDesc& indElem);
	OccurDesc& operator=(const OccurDesc& indElem);
	
	~OccurDesc();
	
	/// Получить номер вхождения
	int GetNumber() const { return occurNumb; }
	/// Возвращает true, если данное вхождение содержится в теле цикла stmtFor.
	bool IsIncludedByForBody(const OPS::Reprise::ForStatement* stmtFor) const;

	/// Возвращает true в случае линейных индексов
	bool IsIndexesLinear() const { return m_isIndexesLinear; }
    /// Возвращает true в случае линейных индексов, не зависящих от внешних переменных
    bool hasExtVarsOrNotLinear() const { return m_hasExtVarsOrNotLinear; }
	/// Проверить, вхождение массива или скалярной переменной?
	bool IsArray() const { return dim != 0; }
	/// Получить оператор, которому принадлежит вхождение
	OPS::Reprise::StatementBase* GetStatement() const { return pStmt; }
	/// Инициализирует массив d1 смапленными коэффициентами
	void MapToOneDim();

	/// Получить глубину вложенности цикла stmtFor в опорном гнезде циклов для данного вхождения.
	/// Глубина нумеруется, начиная с 0. 
	/// Функция возвращает глубину, либо -1, если подаваемый цикл не находится в опорном гнезде.
	int GetLoopDeep(const OPS::Reprise::ForStatement* stmtFor) const;

	void ClearData();

private:
	void Clear();
	void Copy(const OccurDesc& other);
	
	/// Указатель на оператор во внутреннем представлении, которому принадлежит данное вхождение
	OPS::Reprise::StatementBase* pStmt;

	/// Номер вхождения (Occurence's Number). 
	/** 
		При построении вспомогательно информации о вхождениях, все вхождения 
		нумеруются, начиная с 0, слева направо, и сверху вниз.
	**/
	int occurNumb;
};

class IndOccurContainer;

typedef std::list<OccurDesc*> OccurList_t;
typedef OccurList_t::iterator OccurIter;

/**
	Класс, хранящий информацию о вхождениях в более удобном для построения графов виде.
*/
class OccurList : public OPS::NonCopyableMix
{
	OccurList_t m_occurrences;
	typedef std::map<int, OccurDesc*>	OccursIndex;
	OccursIndex		m_globalIndex;

public:
	~OccurList() { Clear(); }

	OccurDesc* AddNewElem(
		int occurNumb, 
		OPS::Reprise::StatementBase* pStmt, 
		LoopDesc* indLoops,
		Id::OccurrenceInfo& idOccur);

	/// Получить следующую, начиная с searchFrom, непросмотренную переменную var. Меняет searchFrom
	OccurIter GetNextUnexaminedVarNew(OccurIter searchFrom, const OPS::Reprise::DeclarationBase* varDecl);

	/// Получить следующую, начиная с searchFrom, любую непросмотренную переменную. Меняет searchFrom
	OccurIter GetNextUnexaminedElemNew(OccurIter searchFrom);
	void Clear();
	void SetAllUnexamined();
	OccurIter Begin() { return m_occurrences.begin(); }
	OccurIter End() { return m_occurrences.end(); }
	OccurIter Erase(const OccurIter& it);
	OccurIter Erase(const OccurIter& first, const OccurIter& last);
	
	// returns 'NULL' if there's no Var Entry with global number==index
	OccurDesc* operator[](int index);
	int GetSize() const { return (int)m_occurrences.size(); }

	/// Ужасно неоптимальная функция поиска описания вхождения (OccurDesc) по указателю на него во внутреннем представлении.
	/// Очищать память по возвр. указателю НЕЛЬЗЯ, равно как и изменять содержимое этой памяти.
	/// Если найти описание подаваемого вхождения не удалось, то возвращается NULL.
	OccurDesc* FindOccurDesc(const OPS::Reprise::ReferenceExpression* occur);
};

/**
	Класс, хранящий информацию о вхождениях переменных (и генераторах и использованиях) в более удобном для построения графов виде.
	Имеется возможность доступа к вхождению по его номеру (индексированние).
*/
class IndOccurContainer : public OPS::NonCopyableMix
{
	OccurList indGenList;
	OccurList indUtilList;

	typedef std::vector<OccurDesc*>	OccursIndex;
	// Массив, для быстрого доступа к вхождениям по их глобальному номеру.
	OccursIndex globalIndex;

	void ParseExpression(OPS::Reprise::ExpressionBase* expr, Id::id& index, EN_OccurListBuildParams buildParams, int* numbE);

public:

	/// Построить информацию о вхождениях переменных (левых и правых)
	void Build(Id::id index, EN_OccurListBuildParams buildParams = OLBP_SEARCH_OUTERLOOPS);

	/// Получить итератор на следующую, начиная с searchFrom, непросмотренную переменную-генератор varName. Функция изеняет searchFrom.
	OccurIter GetNextUnexaminedVarGenNew(OccurIter searchFrom, const OPS::Reprise::DeclarationBase* varDecl);

	/// Получить итератор на следующую, начиная с searchFrom, любую непросмотренную переменную-генератор. Функция изменяет searchFrom.
	OccurIter GetNextUnexaminedElemGenNew(OccurIter searchFrom);

	/// Получить итератор на следующую, начиная с searchFrom, непросмотренную переменную-использование varName. Функция изеняет searchFrom.
	OccurIter GetNextUnexaminedVarUtilNew(OccurIter searchFrom, const OPS::Reprise::DeclarationBase* varDecl);

	/// Получить итератор на следующую, начиная с searchFrom, любую непросмотренную переменную-использование. Функция изменяет searchFrom.
	OccurIter GetNextUnexaminedElemUtilNew(OccurIter searchFrom);
	
	int GetSize() const;
	
	/// Очистить списки вхождений.
	void Clear();

	/// Пометить все генераторы как непросмотренные.
	void SetAllGenUnexamined() {indGenList.SetAllUnexamined();}

	/// Пометить все использованя как непросмотренные.
	void SetAllUtilUnexamined() {indUtilList.SetAllUnexamined();}

	/// Пометить все использованя как непросмотренные.
	void SetAllUnexamined() {indGenList.SetAllUnexamined(); indUtilList.SetAllUnexamined();}

	/// Получить список генераторов.
	OccurList& GetIndGenList() {return indGenList;}
	
	/// Получить список использований.
	OccurList& GetIndUtilList() {return indUtilList;}
	
	/// Получить указатель на описание вхождения по его номеру. Если список не индексирован, то возращается NULL. Очищать память по возвр. указателю НЕЛЬЗЯ, равно как и изменять содержимое этой памяти. Стандартно, функция BuildVO выпоняет индексирование после построения списка.
	OccurDesc* operator[](int index);// returns 'NULL' iff there's no Var Entry with global number==index

	/// Ужасно неоптимальная функция поиска описания вхождения (OccurDesc) по указателю на него во внутреннем представлении.
	/// Очищать память по возвр. указателю НЕЛЬЗЯ, равно как и изменять содержимое этой памяти.
	/// Если найти описание подаваемого вхождения не удалось, то возвращается NULL.
	OccurDesc* FindOccurDesc(const OPS::Reprise::ReferenceExpression* occur);

	/// Возвращает минимально и максимально возможные номера для вхождений, принадлежащих блоку.
	/// Вхождение с минимальным номером - первое вхождение в блоке (в порядке нумерации), с максимальным - последнее.
	/// Но это все - в случае удачи. 
	/// Возвращает значение true - в случае удачи, и false - в случае неудачи.
	/// ПРЕДПОЛАГАЕТСЯ, что в программе есть только операторы присваивания.
	bool GetFirstAndLastBlockOccurNumb(const OPS::Reprise::BlockStatement& block,int& firstOccurNumb,int& lastOccurNumb);

	/// Проиндексировать все хранящиеся описания вхождений в списке. Только после этого можно будет пользоваться []. Если изменено содержание контейнера, то нужно проиндексировать его. Функция BuildVO вызывает функцию CreateIndex у заполненного контейнера.
	void CreateIndex();
};

/**
	Структура, описывающая дугу графа Лампорта.
*/
struct LampArrow : public Status
				 , public OPS::NonCopyableMix
{
private:
	typedef std::list<int>	SuppList;
	/// Cписок носителей; носитель - номер оператора цикла по глубине вложенности начиная с 0 
	/// для самого внешнего.  Некоторые элементы списка могут принимать 
	/// значения LIDEP,ANYSUPP. ПРИЧЕМ, если в этом списке существует элемент,
	/// равный ANYSUPP, то этот элемент будет стоять на первом месте в списке и 
	/// он будет единственным элементом ! Эта особенность используется в функции TestSupp, IsLoopIndependent
	/// этот список может быть и пустой: если не существует гнезда циклов, содержащего оба вхождения
	SuppList	suppList;
public:

	///	тип зависимости.
	EN_DepType	type;

	/// глобальный номер вхождений (источника и зависимого).
	int srcOccurNumb, depOccurNumb;

	/// количество циклов, которые одновременно охватываю (содержат) два зависимых вхождения: srcOccurNumb и depOccurNumb. Носитель всегда строго меньше этого числа.
	int commonLoopNumb;

	const OccurDesc* srcOccurDesc, *depOccurDesc;

	/// указатели на операторы во внутреннем представлении: содержащий вхождение порожд. зависимость (Src) и содержащий зависимое вхождение (Dep).
	OPS::Reprise::StatementBase* pSrcStmt, *pDepStmt;

	/// указатели на соотв. вхождения во внутреннем представлении: порожд. зависимость (pSrcOccur) и зависимое (pDepOccur).
	OPS::Reprise::ReferenceExpression* pSrcOccur, *pDepOccur;

	LampArrow(
		const OccurDesc& srcOccur,
		EN_DepType deptype,
		int _commonLoopNumb,
		const OccurDesc& depOccur);

	/// Функция проверки носителя зависимости данной дуги. 
	/// Возвращает true, если носитель зависимости (точнее, один из носителей 
	///    описываемой зависимости) равен _supp (полагается равным _supp).
	bool TestSupp(int _supp) const;

	bool TestSupp(const OPS::Reprise::ForStatement& supp) const;

	/// Возвращает true, если зависимость - циклически независимая. 
	/// Если носитель зависимости - ANYSUPP, то эта функция также вернет true 
	/// (т.к. любой носитель подразумевает и циклически независимую зависимость).
	bool IsLoopIndependent() const;

	/// Возвращает true, если зависимость - циклически независимая и других 
	/// носителей у данной зависимости НЕТ.
	bool IsLoopIndependentOnly() const;

	std::string GetTag(EN_PrintModes printMode = PM_POPULAR_RUSSIAN);

	SuppList GetSuppList() { return suppList; }

private:
	void PushSupp(int supp){suppList.push_back(supp);}

	friend class LamportGraph;
	friend std::ostream& operator<<(std::ostream&, const LampArrow&);
};

typedef std::list<LampArrow*> LampArrowList;
typedef LampArrowList::iterator LampArrowIterator;
typedef LampArrowList::const_iterator ConstLampArrowIterator;

/**
	Класс, описывающий граф Лампорта. Граф Лампорта описывается списком дуг.
*/
class LamportGraph : public OPS::NonCopyableMix
{
	IndOccurContainer m_occurContainer;
	LampArrowList conList;

	LampArrow* AddRow(const OccurDesc& srcOccur,EN_DepType deptype,int commonLoopNumb,const OccurDesc& depOccur);

	void RefineByDepGraphEx();


public:
	LamportGraph(){}
	~LamportGraph(){Clear();}

	/// Очистить граф.
	void Clear();

	/// Функции построения графа Лампорта. Возможные параметры построения определяются соответствующим enum-ом EN_LamportGraphBuildParams (см. начало файла DepGraph.h).
	void Build(OPS::Reprise::BlockStatement& block, EN_LamportGraphBuildParams buildParams=LMP_DEFAULT, bool searchOuterLoops = true);

	void Build(const Id::id& index, EN_LamportGraphBuildParams buildParams=LMP_DEFAULT, bool searchOuterLoops = true);

	/// Модифицировать построенный граф (уточнить).
	/// Способ улучшения определяется значениями параметра refineMethod:
	/// RM_NO_REFINEMENT - ничего не делается,
	/// RM_ELEM_LATTICE - улучшение с помощью элементарных решетчатых графов,
	/// RM_BELOW_LATTICE - улучшение с помощью минимальных снизу решетчатых графов, (не реализовано)
	/// RM_VALUE_LATTICE - улучшение с помощью решетчатых графов, описывающих зависимость по значению.  (не реализовано)
	/// Последний метод для линейных программ без внешних переменных и условных операторов (пока так) позволяет перейти в графе от "инф. зависимости" к "инф. зависимости по значению". 
	/// В результате из графа могут быть удалены некоторые дуги зависимостей или некоторые носители (см. мою диссер.)
	/// возвращает 0, в случае успеха.

	/// пока работает не совсем так, как хотелось бы: не учитывается, что может быть, например, два генератора и одно использование одной переменной.
	int Refine(EN_RefineMethod refineMethod);

	/// Получить список дуг идущих из генераторов в данное вхождение
	/**
		\param	pDepOccur	- указатель на вхождение, в которое входят дуги.
		\param  arrowList - заполняемый в ходе работы функции список дуг, где начало каждой дуги это генератор, из которого идет дуга во вх pDepOccur
	*/
	void GetAllArrowsFromGeneratorsToThisOccur(const OPS::Reprise::ReferenceExpression* pDepOccur,std::list<LampArrowIterator>& arrowList);

	/// Получить список дуг идущих из использований в данное вхождение
	/**
		\param	pDepOccur	- указатель на вхождение, в которое входят дуги.
		\param  arrowList - заполняемый в ходе работы функции список дуг, где начало каждой дуги это использование, из которого идет дуга во вх pDepOccur
	*/
	void GetAllArrowsFromUtilitiesToThisOccur(const OPS::Reprise::ReferenceExpression* pDepOccur,std::list<LampArrowIterator>& arrowList);

	/// Получить список вхождений, из которых идут дуги в данное вхождение
	/**
		\param	pDepOccur	- указатель на вхождение, в которое входят дуги.
		\param  occurList - заполняемый в ходе работы функции список описаний вхождений, причем из каждого идет дуга во вх pDepOccur
		\param  srcOccurType - фильтр отбираемых вхождений (из которых идут дуги): 
					0 - отбираются только генераторы, 
					1 - отбираются только использования, 
					2 - отбираются и генераторы и использования.
	*/
	void GetAllOccursInArrowsToThisOccur(const OPS::Reprise::ReferenceExpression* pDepOccur,std::list<const OccurDesc*>& occurList, int srcOccurType);

	IndOccurContainer& getOccurContainer() { return m_occurContainer; }
	const IndOccurContainer& getOccurContainer() const { return m_occurContainer; }

	/// Функция возвращает итератор на начало списка зависимостей. Для тех гурманов, кому не достаточно стандартных средств.
	LampArrowIterator Begin(){return conList.begin();}
	ConstLampArrowIterator Begin() const {return conList.begin();}

	/// Функция возвращает итератор на начало списка зависимостей.
	LampArrowIterator End(){return conList.end();}
	ConstLampArrowIterator End() const {return conList.end();}

	/// Удалить дугу
	LampArrowIterator DeleteArrow(LampArrowIterator arrowIter);

	///	Функция возвращает итератор на первую в списке дуг структуру, описывающую зависимость
	/// оператора pDepStmt от вх pSrcStmt (задаются указатели на операторы во внутр. представлении).
	LampArrowIterator GetDepIter(const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt)
	{return GetDepIterEx(conList.begin(),pSrcStmt,pDepStmt);} 
	
	///	Функция возвращает итератор на первую в списке дуг структуру, описывающую зависимость оператора pDepStmt от вх pSrcStmt 
	/// (задаются указатели на операторы во внутр. представлении). НО, Поиск ведется от итератора searchFrom включительно.
	LampArrowIterator GetDepIterEx(LampArrowIterator searchFrom,const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt);
	

	///	Функция возвращает итератор на первую в списке дуг структуру, описывающую зависимость вхождения pDepOccur от вх pSrcOccur
	/// (задаются указатели на вхождения во внутр. представлении).
	LampArrowIterator GetDepIter(const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur)
	{return GetDepIterEx(conList.begin(),pSrcOccur,pDepOccur);} 

	///	Функция возвращает итератор на первую в списке дуг структуру, описывающую зависимость вхождения pDepOccur от вх pSrcOccur
	///(задаются указатели на вхождения во внутр. представлении). НО, Поиск ведется от итератора searchFrom включительно.
	LampArrowIterator GetDepIterEx(LampArrowIterator searchFrom,const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur);


	/// Тестирование существования зависимости (любого типа и носителя). Или по-другому - тестирование существования дуги в графе.

	///	Функция возвращает не 0, если вхождение с номером depOccurNumb зависит от вхождения с номером occurNumb.
	/**
		\param	occurNumb, depOccurNumb	- глобальные в искомом фрагменте программы номера вхождений.
	*/
	bool TestDep(int occurNumb,int depOccurNumb);

	///	Функция возвращает не 0, если оператор pDepStmt зависит от оператора pSrcStmt.
	/**
		\param	pSrcStmt, pDepStmt	- указатели на операторы во внутреннем представлении.
	*/
	bool TestDep(const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt);

	///	Функция возвращает не 0, если вхождение pDepOccur зависит от вхождения pSrcOccur.
	/**
		\param	pSrcOccur, pDepOccur	- глобальные в искомом фрагменте программы номера вхождений.
	*/
	bool TestDep(const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur);




	/// Тестирование существования зависимости заданного типа.

	///	Функция возвращает не 0, если вхождение с номером depOccurNumb зависит от вхождения с номером occurNumb, ПРИЧЕМ зависимость имеет тип depType.
	/**
		\param	occurNumb, depOccurNumb	- глобальные в искомом фрагменте программы номера вхождений.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
	*/
	bool TestDep(EN_DepType depType,int occurNumb,int depOccurNumb);

	///	Функция возвращает не 0, если оператор pDepStmt зависит от оператора pSrcStmt, ПРИЧЕМ зависимость имеет тип depType.
	/**
		\param	pSrcStmt, pDepStmt	- указатели на операторы во внутреннем представлении.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
	*/
	bool TestDep(EN_DepType depType,const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt);

	///	Функция возвращает не 0, если оператор pDepStmt зависит от операторов во фромгменте [pSrcStmt1, pSrcStmt2), 
	/// ПРИЧЕМ зависимость имеет тип depType.
	/**
		\param	pSrcStmt, pDepStmt	- указатели на операторы во внутреннем представлении.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
	*/
	bool TestDep(EN_DepType depType, const OPS::Reprise::StatementBase* pSrcStmt1, 
		const OPS::Reprise::StatementBase* pSrcStmt2, const OPS::Reprise::StatementBase* pDepStmt);

	///	Функция возвращает не 0, если вхождение pDepOccur зависит от вхождения pSrcOccur, ПРИЧЕМ зависимость имеет тип depType.
	/**
		\param	pSrcOccur, pDepOccur	- глобальные в искомом фрагменте программы номера вхождений.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
	*/
	bool TestDep(EN_DepType depType,const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur);




	/// Тестирование существования зависимости заданного носителя (или циклической независимости).

	///	Функция возвращает не 0, если вхождение с номером depOccurNumb зависит от вхождения с номером occurNumb,
	/// ПРИЧЕМ зависимость имеет носитель supp.
	/**
		\param	occurNumb, depOccurNumb	- глобальные в искомом фрагменте программы номера вхождений.
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestSupp(int supp,int occurNumb,int depOccurNumb);

	///	Функция возвращает не 0, если оператор pDepStmt зависит от оператора pSrcStmt, ПРИЧЕМ зависимость имеет носитель supp.
	/**
		\param	pSrcStmt, pDepStmt	- указатели на операторы во внутреннем представлении.
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestSupp(int supp,const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt);

	///	Функция возвращает не 0, если вхождение pDepOccur зависит от вхождения pSrcOccur, ПРИЧЕМ зависимость имеет носитель supp.
	/**
		\param	pSrcOccur, pDepOccur	- глобальные в искомом фрагменте программы номера вхождений.
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestSupp(int supp,const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur);

	///	Функция возвращает не 0, если в графе существует зависимость с носителем supp.
	/// ВНИМАНИЕ! Результат может быть не адекватным, если, например, проверяется носитель 1, а граф построен для фрагмента программы,
	/// с более чем одним оператором цикла на глубине вложенности "1".
	/**
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestSupp(int supp);


	/// Тестирование существования зависимости заданного и типа и носителя одновременно.

	///	Функция возвращает не 0, если вхождение с номером depOccurNumb зависит от вхождения с номером occurNumb,
	/// ПРИЧЕМ зависимость имеет тип depType и носитель supp.
	/**
		\param	occurNumb, depOccurNumb	- глобальные в искомом фрагменте программы номера вхождений.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestDepAndSupp(EN_DepType depType,int supp,int occurNumb,int depOccurNumb);

	///	Функция возвращает не 0, если оператор pDepStmt зависит от оператора pSrcStmt, ПРИЧЕМ зависимость имеет тип depType и носитель supp.
	/**
		\param	pSrcStmt, pDepStmt	- указатели на операторы во внутреннем представлении.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	/// возвращает 1, если между операторами существует зависимость типа depType и нос. supp, иначе -- 0 
	bool TestDepAndSupp(EN_DepType depType,int supp,const OPS::Reprise::StatementBase* pSrcStmt,const OPS::Reprise::StatementBase* pDepStmt);

	///	Функция возвращает не 0, если вхождение pDepOccur зависит от вхождения pSrcOccur, ПРИЧЕМ зависимость имеет тип depType и носитель supp.
	/**
		\param	pSrcOccur, pDepOccur	- глобальные в искомом фрагменте программы номера вхождений.
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	// то же, но для вхождений; pSrcOccur, pDepOccur - указатели на вхождения во внутреннем представлении  
	bool TestDepAndSupp(EN_DepType depType,int supp,const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* depEnryRef);


	///	Функция возвращает не 0, если в графе существует зависимость с носителем supp типа depType.
	/// ВНИМАНИЕ! Результат может быть не адекватным, если, например, проверяется носитель 1, а граф построен для фрагмента программы,
	/// с более чем одним оператором цикла на глубине вложенности "1".
	/**
		\param	depType	- тип зависимости. Определяется enum-ом EN_DepType (см. начало файла DepGraph.h).
		\param	supp	- носитель зависимости: номер, либо константа LIDEP (для циклически независимой зависимости).
	*/
	bool TestDepAndSupp(EN_DepType depType,int supp);


	///	Возвращает не 0, если в графе Лампорта во вхождение pDepOccur входит ТОЛЬКО дуга из вхождения pSrcOccur;
	/// других дуг, входящих в pDepOccur нет. (В pDepOccur НЕ входят дуги, начала которых НЕ pSrcOccur.)
	bool HasSingleArrowToDepOccur(const OPS::Reprise::ReferenceExpression* pSrcOccur,const OPS::Reprise::ReferenceExpression* pDepOccur);

	/// Проверяется условия, что граф содержит только циклически независимые дуги.
	/// Функция возвращает не 0, если граф содержит только циклически независимые дуги (других дуг нет).
	bool HasLoopIndependentArrowsOnly();

	int GetSize(){return (int)conList.size();}

	/// Возвращает носитель зависимости или -1 (NODEP) если ее нет. 
	/**
		Поиск начинается с предполагаемого носителя start. 
		Если такого носителя не существует, то start:=start+1. 
		Поиск осуществляется до нахождения первого носителя, 
		большего исходного start, который и возвращается.
	**/
	static int GetDepSupp(const OccurDesc& p, const OccurDesc& q, int start, int commonLoopNumb);

private:

	/// Уточнить указанные дуги с помощью минимальных снизу решетчатых графов
	/// Возвращает 0, в случае успеха. Иначе - код ошибки.
	/**
		\param	pDepOccurDesc	- указатель на вхождение, в которое дуги входят
		\param	arrowList	- список дуг, которые нужно уточнить.
	*/
	int RefineArrowsWithBelowLattice(OccurDesc* pDepOccurDesc, std::list<LampArrowIterator>& arrowList);

	void Build(EN_LamportGraphBuildParams buildParams);
	void BuildFlowAntiDeps(const OccurDesc& first, const OccurDesc& second);
	void BuildOutDep(const OccurDesc& first, const OccurDesc& second);
	void BuildSelfDep(const OccurDesc& desc, EN_DepType depType);
	void BuildInDep(const OccurDesc& first, const OccurDesc& second);
};

class GetTypeVisitor : public OPS::BaseVisitor
	, public OPS::Visitor<  OPS::Reprise::BasicLiteralExpression>
	, public OPS::Visitor<  OPS::Reprise::StrictLiteralExpression>
	, public OPS::Visitor<  OPS::Reprise::ReferenceExpression>
	, public OPS::Visitor<  OPS::Reprise::StructAccessExpression>
	, public OPS::Visitor<  OPS::Reprise::EnumAccessExpression>
	, public OPS::Visitor<  OPS::Reprise::TypeCastExpression>
	, public OPS::Visitor<  OPS::Reprise::BasicCallExpression>
	, public OPS::Visitor<  OPS::Reprise::SubroutineCallExpression>
	, public OPS::Visitor<  OPS::Reprise::EmptyExpression>

	, public OPS::Visitor<  OPS::Reprise::BlockStatement>
	, public OPS::Visitor<  OPS::Reprise::ForStatement>
	, public OPS::Visitor<  OPS::Reprise::WhileStatement>
	, public OPS::Visitor<  OPS::Reprise::IfStatement>
	, public OPS::Visitor<  OPS::Reprise::GotoStatement>
	, public OPS::Visitor<  OPS::Reprise::ReturnStatement>

	, public OPS::Visitor<  OPS::Reprise::ExpressionStatement>
	, public OPS::Visitor<  OPS::Reprise::EmptyStatement>

	, public OPS::Visitor<  OPS::Reprise::BasicTypeBase>
	, public OPS::Visitor<  OPS::Reprise::PtrType>
	, public OPS::Visitor<  OPS::Reprise::ArrayType>
	, public OPS::Visitor<  OPS::Reprise::VectorType>
	, public OPS::Visitor<  OPS::Reprise::StructType>
	, public OPS::Visitor<  OPS::Reprise::EnumType>
	, public OPS::Visitor<  OPS::Reprise::SubroutineType>
	, public OPS::Visitor<  OPS::Reprise::DeclaredType>

	, public OPS::NonCopyableMix
{
public:
	enum NodeKind
	{
		NK_Unknown,
		NK_BasicLiteralExpression,
		NK_StrictLiteralExpression,
		NK_ReferenceExpression,
		NK_StructAccessExpression,
		NK_EnumAccessExpression,
		NK_TypeCastExpression,
		NK_BasicCallExpression,
		NK_SubroutineCallExpression,
		NK_EmptyExpression,
		NK_HirCCallExpression,

		NK_BlockStatement,
		NK_ForStatement,
		NK_WhileStatement,
		NK_IfStatement,
		NK_GotoStatement,
		NK_ReturnStatement,
		NK_ExpressionStatement,
		NK_EmptyStatement,

		NK_BaseBasicType,
		NK_PtrType,
		NK_ArrayType,
		NK_VectorType,
		NK_StructType,
		NK_EnumType,
		NK_SubroutineType,
		NK_DeclaredType
	};

	GetTypeVisitor():m_typeOfNode(NK_Unknown){}

	void visit(  OPS::Reprise::BasicLiteralExpression&)
	{m_typeOfNode = NK_BasicLiteralExpression;}
	void visit(  OPS::Reprise::StrictLiteralExpression&)
	{m_typeOfNode = NK_StrictLiteralExpression;}
	void visit(  OPS::Reprise::ReferenceExpression&)
	{m_typeOfNode = NK_ReferenceExpression;}
	void visit(  OPS::Reprise::StructAccessExpression&)
	{m_typeOfNode = NK_StructAccessExpression;}
	void visit(  OPS::Reprise::EnumAccessExpression&)
	{m_typeOfNode = NK_EnumAccessExpression;}
	void visit(  OPS::Reprise::TypeCastExpression&)
	{m_typeOfNode = NK_TypeCastExpression;}
	void visit(  OPS::Reprise::BasicCallExpression&)
	{m_typeOfNode = NK_BasicCallExpression;}
	void visit(  OPS::Reprise::SubroutineCallExpression&)
	{m_typeOfNode = NK_SubroutineCallExpression;}
	void visit(  OPS::Reprise::EmptyExpression&)
	{m_typeOfNode = NK_EmptyExpression;}

	void visit(  OPS::Reprise::BlockStatement&)
	{m_typeOfNode = NK_BlockStatement;}
	void visit(  OPS::Reprise::ForStatement&)
	{m_typeOfNode = NK_ForStatement;}
	void visit(  OPS::Reprise::WhileStatement&)
	{m_typeOfNode = NK_WhileStatement;}
	void visit(  OPS::Reprise::IfStatement&)
	{m_typeOfNode = NK_IfStatement;}
	void visit(  OPS::Reprise::GotoStatement&)
	{m_typeOfNode = NK_GotoStatement;}
	void visit(  OPS::Reprise::ReturnStatement&)
	{m_typeOfNode = NK_ReturnStatement;}
	void visit(  OPS::Reprise::ExpressionStatement&)
	{m_typeOfNode = NK_ExpressionStatement;}
	void visit(  OPS::Reprise::EmptyStatement&)
	{m_typeOfNode = NK_EmptyStatement;}

	void visit(  OPS::Reprise::BasicTypeBase&)
	{m_typeOfNode = NK_BaseBasicType;}
	void visit(  OPS::Reprise::PtrType&)
	{m_typeOfNode = NK_PtrType;}
	void visit(  OPS::Reprise::ArrayType&)
	{m_typeOfNode = NK_ArrayType;}
	void visit(  OPS::Reprise::VectorType&)
	{m_typeOfNode = NK_VectorType;}
	void visit(  OPS::Reprise::StructType&)
	{m_typeOfNode = NK_StructType;}
	void visit(  OPS::Reprise::EnumType&)
	{m_typeOfNode = NK_EnumType;}
	void visit(  OPS::Reprise::SubroutineType&)
	{m_typeOfNode = NK_SubroutineType;}
	void visit(  OPS::Reprise::DeclaredType&)
	{m_typeOfNode = NK_DeclaredType;}
	
	NodeKind m_typeOfNode;

};


std::ostream& operator<<(std::ostream& os, const LamportGraph& graph);
std::ostream& operator<<(std::ostream& os, const LampArrow& arrow);

inline int POS(int r) { return r < 0 ? 0 :  r; }
inline int NEG(int r) { return r > 0 ? 0 : -r; }

// Возвращается true, если зависимость вх q от p препятствует перестановке цикла k и k+1;
// Гнезда нумеруются от 0, начиная с самого внешнего.
// Предполагается, что эта зависимость имеет носитель k.
bool DepPreventsLoopInterchange(const OccurDesc& p, const OccurDesc& q, int k);

// Таже, что и описанная выше, (только без предположений о носителе k)
// Эта функция осуществяет самый точный анализ !!
bool DepPreventsLoopInterchangeEx(const OccurDesc& srcEntry, const OccurDesc& depEntry, int k);

// Возвращает true, если в графе Лампорта lg не существует зависимости, препятствующей 
// перестановке циклов k и k+1
// Гнезда нумеруются от 0, начиная с самого внешнего.
// ! Следует учесть, что самое внешнее гнездо может не принадлежать тому фрагменту программы, для которого был построен граф.
// граф Лампорта должен быть построен для тела исследуемого гнезда циклов
int LoopsAreInterchangable(LamportGraph& lg,int k);

// Таже, что и описанная выше, но осуществляется самый точный анализ !!
int LoopsAreInterchangableEx(LamportGraph& lg,int k);

//*** ДАННЫЙ БЛОК ФУНКЦИЙ РАБОТАЕТ ТОЛЬКО С КАНОНИЗИРОВАНЫМИ ЦИКЛАМИ ****//
//*
//*
//void BuildVO(BaseNode* IN,IndOccurContainer& indOccurList);// построить информацию о вхождениях переменных (левых и правых)
// построить информацию о вхождениях переменных (левых и правых)
void BuildVO(const Id::id& index, IndOccurContainer& indOccurList, EN_OccurListBuildParams buildParams=OLBP_SEARCH_OUTERLOOPS); // ========= Исправить id ==========

// построить граф Лампорта программы
void BuildLamportGraph(const Id::id& index, LamportGraph& lamp); 																  // ========= Исправить id ==========


LoopDesc* GetIndLoopArray1(std::vector<Id::LoopInfo>& loops,Id::id& a);
//*
//*
//*** ДАННЫЙ БЛОК ФУНКЦИЙ РАБОТАЕТ ТОЛЬКО С КАНОНИЗИРОВАНЫМИ ЦИКЛАМИ ****//

/// Функция возвращает указатель на "клон" вхождения occur. В работе функции учитывается, что вхождение может быть либо массивом, либо скалярной переменной.
/// В случае ошибки, функция возвращает NULL;
OPS::Reprise::ReferenceExpression* CopyOccurrence(const OPS::Reprise::ReferenceExpression& occur);

bool StringIsNumber(const char* buf, size_t n);//Возвращает 1, если в строке buf записано число; n - размер буфера
bool StringIsNumber(const char* buf);//то же, но полагается, что buf - строка (т.е. он заканчивается 0)

/// Возвращает true, если оператор, содержащий вхождение p, предшествует или 
/// совпадает по тексту программы с оператором, содержащим вхождение q.
bool PrecedesNonStrictly(const OccurDesc& p, const OccurDesc& q);

/// Возвращает true, если оператор, содержащий вхождение p, предшествует 
/// по тексту программы оператор, содержащий вхождение q.
bool PrecedesStrictly(const OccurDesc& p, const OccurDesc& q);

///  Возвращает true, если оператор содержит и вхождение p и вхождение q.
inline bool BelongToSameStatement(const OccurDesc& p, const OccurDesc& q)
{ return p.GetStatement() == q.GetStatement(); }

///	Устанавливает набор флаго для добавленной дуги. Статусы добавляются на основании статусов вхождений, которые анализировались при построении ЭТОЙ дуги.
/// Функция не удаляет флаги дуги, установленные ранее (до ее вызова)
void SetLampArrowBasedOnOccurStatus(LampArrow* lampArrow,const OccurDesc& srcOccur,const OccurDesc& depOccur);

/// Возвращает число общих циклов для двух вхождений
int getCommonUpperLoopsAmount(const OccurDesc& first, const OccurDesc& second);

/// Возвращает список вхождений переменных
void getAllVariables(OPS::Reprise::ExpressionBase& node, const std::vector<Id::LoopInfo>& loops, std::vector<Id::OccurrenceInfo>& varsList);

/// Получить список дуг, удовлетворяющих предикату
template<typename Pred>
	LampArrowList getSubGraphByPred(LampArrowIterator begin, LampArrowIterator end, Pred pred)
{
	LampArrowList list;

	LampArrowIterator it = begin;

	for(; it != end; ++it)
	{
		if (pred(**it))
			list.push_back(*it);
	}

	return list;
}

template<typename Pred>
	LampArrowList getSubGraphByPred(LamportGraph& depGraph, Pred pred)
{
	return getSubGraphByPred(depGraph.Begin(), depGraph.End(), pred);
}

/// Возвращает список дуг, порожденных данным циклом
LampArrowList getSubGraphByCarrier(LamportGraph& depGraph, OPS::Reprise::ForStatement& loop);

/// Возвращает список дуг, начало и конец которых находятся внутри тела данного цикла
LampArrowList getSubGraphByLoopBody(LamportGraph& depGraph, OPS::Reprise::ForStatement& loop);


} // end namespace DepGraph

#endif
