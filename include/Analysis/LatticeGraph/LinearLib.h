#ifndef _LINEAR_OBJECTS_LIB_
#define _LINEAR_OBJECTS_LIB_

#include "Reprise/Expressions.h"
#include "Analysis/DepGraph/Status.h"
#include "Shared/ParametricLinearExpressions.h"
#include <memory>
#include <list>
#include <cstring>

namespace OPS {namespace Montego {class BasicOccurrence;}}

namespace OPS
{
namespace LatticeGraph
{
//enum LWTContextStatus{UNFEASIBLE=1};
//enum GenAreaStatus{GE_UNFEASIBLE=1};

/// Набор флагов, которые используются в классах Context и GenArea для индикации доказанного состояния допустимости: допустимый (содержит целые точки), недопустимый, неопределенный
enum EN_FeasibilityState
{
	/// Доказана допустимость
	FEASIBILITY_PROVEN = 1,
	/// Доказана НЕдопустимость
	UNFEASIBILITY_PROVEN = 2,
    /// Допустимость неизвестна
	UNKNOWN_FEAS = 3,
};

/**
	Структура, описывающая линейное выражение с целочисленными коэффициентами.
*/
struct SimpleLinearExpression
{
	/// Массив коэффициентов перед переменными; m_coefs[0] - св. член.
	int* m_coefs;

	/// Кол-во элементов массива m_coefs.
	int m_dim;
    SimpleLinearExpression();
	explicit SimpleLinearExpression(int dim);
	SimpleLinearExpression(const int* _data, int _dim);
	SimpleLinearExpression(const SimpleLinearExpression& linearExpr);
	SimpleLinearExpression& operator=(const SimpleLinearExpression& linearExpr);
	~SimpleLinearExpression();
    void allocateMem(int dim);

	void Clear();

	void Print(std::ostream& os, const char** paramNames, int paramNamesDim);
	void Print(std::ostream& os, const std::string* paramNames, int paramNamesDim);
    std::string toString(const std::vector<std::string>& paramNames);
    std::string toString();
	void MakeZero();
	int& operator[](int index);
    int& at(int index);
    long evaluate(std::vector<int>& paramValues);//вычисляет линейное выражение
    long evaluate(const std::vector<int>& paramValues);
    void multiply(int p);//умножает линейное выражение на p
    void substract(SimpleLinearExpression& subtrahend);
    void add(SimpleLinearExpression& summand);

    //Применяет трансформацию к линейному выражению
    //в трансформационной матрице по строчкам записаны выражения старых переменных через новые
    //Последний элемент каждой строки трансформационной матрицы - св. член. 
    //если строчек меньше, чем переменных, то заменяет только те, которые заменить нужно согласно матрице (при этом 
    //коэффициенты при не заменяемых параметрах могут поменяться, если они участвуют в замене, т.е. если dimj-1>dimi)
    //свободный член замене не подлежит (это не значи что он не меняется вследствии изменения других параметров)
    //т.е. если dimi>=m_dim, то строки матрицы трансформации начиная с m_dim-1 игнорируются 
    //ВНИМАНИЕ!!! ЕСЛИ dimi<m_dim-1, то СЧИТАЕТСЯ, ЧТО ОСТАВШИЕСЯ ПАРАМЕТРЫ В m_coefs (с dimi по m_dim)
    //И НОВЫЕ ПАРАМЕТРЫ - ЭТО СОВЕРШЕННО РАЗНЫЕ ПАРАМЕТРЫ 
    //ЕСЛИ У ВАС ПО СМЫСЛУ ЗАДАЧИ ОНИ ОДИНАКОВЫЕ, ТО, БУДБТЕ ДОБРЫ, ДОБАВЬТЕ ЕЩЕ СТРОЧЕК В matrix 
    //C ЕДИНИЦАМИ НА ГЛАВНОЙ ДИАГОНАЛИ
	void transform(int** matrix,int dimi,int dimj);

    void insertNZerosBeforeNewParamCoefs(int N, int varNum);//varNum - кол-во обычных переменных (не новых параметров)

    //varNum - кол-во обычных переменных (не новых параметров)
    //удаляем коэффициенты с varNum-N+1 до varNum
    void deleteNZerosBeforeNewParamCoefs(int N, int varNum);
};


/**
	Структура, описывающая линейное неравенство с целочисленными коэффициентами.
	Неравенство имеет вид: линейное_выражение>=0.
*/
struct Inequality: public SimpleLinearExpression
{
    Inequality();
	Inequality(int dim);
	Inequality(const int* data,int dim);
	Inequality(const SimpleLinearExpression& in);
    Inequality& operator=(const Inequality& in);

	/// Переставить местами слагаемые в неравенстве.
	/// Если какого-то слагаемого в неравенстве нет, то считаем, что оно нулевое
	void SwapItems(int i, int j);

    //меняет данное неравенство на противоположное
    void makeOposite();

	void Print(std::ostream& os, const char** paramNames, int paramNamesDim, int mode);
	void Print(std::ostream& os, const std::string* paramNames, int paramNamesDim, int mode);
    ~Inequality();
};


class NewParamVector;
struct ComplexCondition;

/**
	Структура, описывающая выпуклую область. Эта область определяется системой линейных неравенств.
*/
struct Polyhedron
{
public:

	typedef std::list<Inequality*> InequalityList;

    /// Список линейных неравенств.
	InequalityList		m_ins;

    //Список и порядок следования внешних параметров
    std::vector<OPS::Reprise::VariableDeclaration*> m_externalParamsVector;

    Polyhedron(){feasibilityState = FEASIBILITY_PROVEN;}

    Polyhedron(std::vector<Inequality>& ineqVector)
    {for (int i=0; i<(int)ineqVector.size(); i++) m_ins.push_back(new Inequality(ineqVector[i]));
    feasibilityState = FEASIBILITY_PROVEN;}

    Polyhedron(const Polyhedron& ct);

    ~Polyhedron(){Clear();}

    //составляет опорный многогранник для вхождения. Порядок следования коэффициентов неравенств 
    //определяется externalParamsVector (там должны быть все внешние параметры)
    //если externalParamsVector=0, то он составляется автоматически
    Polyhedron(OPS::Montego::BasicOccurrence& o, OPS::Reprise::RepriseBase* code, 
        int deepLimit = -1, std::vector<OPS::Reprise::VariableDeclaration*>* externalParamsVector = 0);

	void Clear();
	void AddInequality(const int* data,int dim);
	void AddInequality(const Inequality& inq);

	/// Добавляет неравенство в контекст. После добавления считается, что объект *inq стал принадлежать данному контексту! (Контекст освобождает его память!)
	void AddInequality(Inequality*& inq);
    void AddInequalities(Polyhedron& p);//добавляет все неравенства из p к текущим
	/// Добавляет неравенство вида ...<0. Оно автоматически переводится в вид ...>=0.
	void AddOppositeInequality(const int* data,int dim);
	void AddOppositeInequality(const Inequality& inq){AddOppositeInequality(inq.m_coefs,inq.m_dim);}

	/// Добавляет неравенство -inq.data>=0
	void AddInverseInequality(const Inequality& inq);
	void AddInverseInequality(const int* data,int dim);

	/// Получить докузанное состояние допустимости.
	EN_FeasibilityState GetFeasibilityState() const {return feasibilityState;}

	/// Упростить контекст. Функция пытается упростить описание контекста; медленный и затратный алгоритм
	void Simplify();

    //возвращает 1, если контекст (область) содержит хотябы одну целочисленную точку
    //при подсчете считается, что первые nonnegativeVarNum переменные в каждом неравенстве контекста >=0, а остальные - произвольные
	int IsFeasible(int nonnegativeVarNum=0) const;

    //Применяется только к допустимому (непустому!) контексту! 
    //Функция возвращает 1, если данный контекст допустим, ПРИ УСЛОВИИ, 
    //что в каждом неравенстве на позиции veryLargeParamCol стоит переменная "очень большого" значения...
	int IsFeasibleEx(int veryLargeParamCol);

	void Print(std::ostream& os, const char** paramNames, int paramNamesDim, int mode/*=0*/);
	void Print(std::ostream& os, const std::string* paramNames, int paramNamesDim, int mode);
    std::string toString();

	void IntersectWith(const Polyhedron& con);// После работы этой функции данный контекст будет пересечением его же и контекста con
									 // одновременно осуществляется проверка на допустимость контекста
	Polyhedron& operator=(const Polyhedron& con);
	int GetSize() const {return (int)m_ins.size();}
	int SatisfiedWith(const int* data,int dim,NewParamVector* paramList);//возвращает НЕ 0, если вектор, описываемый data и dim принадлежит контексту.
	//paramList - список выражений для добавленных параметров. ТРЕБОВАНИЕ: кол-во переменных в каждом неравенстве должно быть не больше размерности входного вектора плюс кол-ва элементов списка paramList
	//																	&& размерность входного вектора больше размерности любого неравенства в контексте

	void Transform(int** matrix,int dimi,int dimj);//Применить трансф. матрицу ко всем лин. выражениям контекста
	//Допустимость контекста до и после преобразования не проверяется
	//массив matrix описывает матрицу также, как массив data в классе ArrowFunc . Последний элемент каждой строки - св. член

	// Построить контекст, который получается из данного контекста подстановкой значений первых dim параметров 
    // в линейные неравенства.
	/**
		Возвращенный контекст может быть пустым.
		Возвращенную память после использования нужно очистить (delete ...)
	*/
	Polyhedron* substituteParams(const int* data,int dim);

    //Строит контекст, который получается из данного контекста подстановкой ВСЕХ внешних параметров из m_externalParamsVector
    Polyhedron* substituteParams(std::map<OPS::Reprise::VariableDeclaration*,int>& externalParamValuesMap);


	/// Переставить местами слагаемые в контексте.
	/// Если какого-то слагаемого в неравенстве нет, то считаем, что оно нулевое
	void SwapItems(int i, int j);

private:
	Inequality* AddNewInequality(int dim);

	/// Нормализует последнее добавленное неравенство.
	void NormalizeBackInq();

    //Доказанное состояние допустимости.
    mutable EN_FeasibilityState feasibilityState;

};

class GenArea:public DepGraph::Status	// Описывает произвольную область (есть объединение выпуклых)
{
	//Доказанное состояние допустимости.
	mutable EN_FeasibilityState feasibilityState;

public:
	typedef std::list<Polyhedron*> ContextList;

	GenArea(){feasibilityState = FEASIBILITY_PROVEN;}// конструируется GenArea равное всему Rn
	GenArea(const GenArea& ge);// конструируется GenArea равное ge
	GenArea(const Polyhedron& con);//{if(con.GetStatus(UNFEASIBLE)){SetStatus(GE_UNFEASIBLE);return;}data.push_back(new Context(con));}// Полагается, что добавляются только допустимые контексты
	~GenArea(){Clear();}
	GenArea& operator=(const GenArea& ge);
    GenArea(ComplexCondition& cond);//по сложному условию конструируется объединение многогранников

// !!! Допустимость (непустота) всех контекстов (и областей) -- параметров ф-ций проверяется только по флагу UNFEASIBLE
	void UnionWith(Polyhedron& con);// исходная область становится объединением ее же и новой выпуклой области (контекста); по сути, это просто добавление контекста в список
	void UnionWith(GenArea& ge);// исходная область становится объединением ее же и новой НЕвыпуклой области ; по сути, это просто добавление GenArea в список
	void Clear();// после этого вызова GenArea становиться равным всему Rn
	void IntersectWith(const Polyhedron& con);//Пересечь с выпуклой областью con
	void IntersectWith(const GenArea& ge);//Пересечь с произвольной областью ge
	void DifferenceFrom(Polyhedron& con);// Отнять выпуклую область con
	void DifferenceFrom(GenArea& ge);// Отнять невыпуклую область ge

	int GetSize() const // Возвращает кол-во контекстов, составляющих область
	{return (int)data.size();}

	void Print(std::ostream& os,const char** paramNames,int paramNamesDim,int mode=0);
	void Print(std::ostream& os,std::string* paramNames,int paramNamesDim,int mode);

	void Simplify();

	bool IsFeasible() const;// Возвращает не 0 в случае, если область содержит хотя бы одну целочисленную точку. Иначе возвращает 0;
	//{return !GetStatus(GE_UNFEASIBLE);}

	/// Получить докузанное состояние допустимости.(Обычно используется в служебных целях. Для проверки допустимости нужно использовать IsFeasible)
	EN_FeasibilityState GetFeasibilityState() const {return feasibilityState;}

	bool SatisfiedWith(const int* _data,int _dim,NewParamVector* paramList);//тоже, что и у Context-а. (!!!См. ТРЕБОВАНИЯ в Context::SatisfiedWith !!!)
	ContextList::iterator Begin(){return data.begin();}
	ContextList::iterator End(){return data.end();}
	ContextList& GetData(){return data;}

	void SetEmpty()//установить GenArea равным пустому множеству.
	{Clear();/*SetStatus(GE_UNFEASIBLE)*/feasibilityState = UNFEASIBILITY_PROVEN;return;}

	void Transform(int** matrix,int dimi,int dimj);//Применить трансф. матрицу ко всем лин. выражениям контекста
	//Допустимость контекста до и после преобразования не проверяется
	//массив matrix описывает матрицу также, как массив data в классе ArrowFunc . Последний элемент каждой строки - св. член

private:
	ContextList data;// список контекстов; по сути, весь GenArea это объединение контекстов в списке
};

/// Возвращает наибольший общий делитель (НОД, Greatest common divisor, GCD) двух чисел
int calculateGCD(int a,int b);

/// По линейному выражению, именам переменных и Namespace, в котором эти переменные видны, строится выражение-дерево
/// Память освобождает тот, кто пользуется
/// Кол-во элементов массива varNames равно le.m_dim-1. (по физическому смыслу)
OPS::Reprise::ExpressionBase* GetExprNode(const SimpleLinearExpression& le,OPS::Reprise::Declarations& ns,const std::string* varNames);

//получает коэффициенты разделенные на 2 группы - при внешних параметрах и при счетчиках циклов
void getExternalParamAndLoopCounterCoefficients(const OPS::Shared::ParametricLinearExpression& expr,
												const OPS::Shared::ParametricLinearExpression::VariablesDeclarationsVector &loopCounters,
												OPS::LatticeGraph::SimpleLinearExpression& loopCounterCoefficients,
												OPS::Shared::CanonicalLinearExpression& externalParamCoefficients);
} // end namespace
}
#endif

