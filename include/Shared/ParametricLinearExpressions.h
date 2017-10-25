#ifndef OPS_SHARED_PARAMETRICLINEAREXPRESSIONS_H_INCLUDED__
#define OPS_SHARED_PARAMETRICLINEAREXPRESSIONS_H_INCLUDED__

#include "Shared/ExpressionPolynomial.h"
#include "Reprise/Expressions.h"
#include <map>
#include <vector>
#include <list>

namespace OPS
{
namespace Shared
{

/// Вспомогательный класс, позволяющий анализировать линейные выражения вида: i1+2*i3-i4+2
class CanonicalLinearExpression
{
public:
	typedef long Coefficient;
	typedef std::map<OPS::Reprise::VariableDeclaration*, Coefficient> SummandsMap;

	CanonicalLinearExpression();
	explicit CanonicalLinearExpression(Coefficient Summand);
	explicit CanonicalLinearExpression(OPS::Reprise::VariableDeclaration* Summand, Coefficient coeff = 1);
    CanonicalLinearExpression(const SummandsMap& summands, Coefficient freeSummand = 0):m_summands(summands),m_freeSummand(freeSummand){}
    ~CanonicalLinearExpression(){}

	/// Обнуляет выражение
	void clear();

	/// Прибавляет к текущему каноническому линейному выражению число
	void add(Coefficient Summand);

	/// Прибавляет к текущему каноническому линейному выражению переменную
	void add(OPS::Reprise::VariableDeclaration* Summand, Coefficient coeff = 1);

	/// Прибавляет к текущему каноническому линейному выражению второе каноническое линейное выражение
	void add(const CanonicalLinearExpression& Summand);

	/// Умножает текущее каноническое линейное выражение на число
	void multiply(Coefficient Multiplier);

	/// Умножает текущее каноническое линейное выражение на переменную
	bool multiply(OPS::Reprise::VariableDeclaration* Multiplier);

	/// Умножает текущее каноническое линейное выражение на второе каноническое линейное выражение
	bool multiply(const CanonicalLinearExpression& Multiplier);

	/// Выдает хеш-список переменных и коэффициэнтов при них
	const SummandsMap& getMap() const { return m_summands; }

	/// Выдает коэффициент перед переменной
	Coefficient getCoefficient(OPS::Reprise::VariableDeclaration* Summand) const;

	/// Выдает свободный член в каноническом представлении линейного выражения
	Coefficient getFreeSummand() const { return m_freeSummand; }

	/// Выдает список VariableDeclaration's переменных, коэффициенты при которых отличны от нуля
	std::list<OPS::Reprise::VariableDeclaration*> getVariables() const;

	/// Возвращает противоположное по знаку выражение
	CanonicalLinearExpression getOpposite() const;

	/// Возвращает true, если все коэффициенты при переменных равны 0, т.е. выражение является константным
	bool isConstant() const;

private:
	SummandsMap m_summands;
	Coefficient m_freeSummand;
};

/// Класс описывающий линейные выражения с параметрами
// Массивы считаются такими же параметрами как и скалярные переменные
class ParametricLinearExpression
{
public:
	typedef Reprise::ExpressionBase CoefficientRef;
	typedef Reprise::ReprisePtr<CoefficientRef> Coefficient;
	typedef std::vector<Reprise::VariableDeclaration*> VariablesDeclarationsVector;
	typedef std::map<SymbolicDescription*, Coefficient> SummandsMap;
	typedef std::list<Reprise::ReprisePtr<SymbolicDescription> > OccurenceDescriptionList;
	typedef std::map<const Reprise::DeclarationBase*, OccurenceDescriptionList> ExpressionVariablesMap;

    ParametricLinearExpression():  m_evaluatable(true) { m_freeSummand.reset(Reprise::StrictLiteralExpression::createInt32(0)); }
	ParametricLinearExpression(const ParametricLinearExpression& arg);
	explicit ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables);
	explicit ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables, const Reprise::ReferenceExpression* oneSummand);
	explicit ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables, const SymbolicDescription* oneSummand);
	explicit ParametricLinearExpression(VariablesDeclarationsVector someBaseVariables, const Reprise::LiteralExpression* oneSummand);

    //конструирует ParametricLinearExpression по заданному ExpressionBase
    static ParametricLinearExpression* createByAllVariables(Reprise::ExpressionBase* Node);
    //конструирует ParametricLinearExpression по заданному ExpressionBase, используя в качестве базы линеаризации только счетчики циклов
    static ParametricLinearExpression* createByIndexes(Reprise::ExpressionBase* Node);
    //конструирует ParametricLinearExpression по заданному ExpressionBase и базе линеаризации
	static ParametricLinearExpression* createByListOfVariables(Reprise::ExpressionBase* Node, const VariablesDeclarationsVector& vectorOfBaseVariables);
    //упрощает, если возможно, выражения, которые представляются линейными
	static Reprise::ReprisePtr<Reprise::ExpressionBase> simplify(Reprise::ExpressionBase* Target);

	void clear();
	~ParametricLinearExpression();

	// Методы получения коэффициентов при переменных участвующих в разложении
	long_long_t getCoefficientAsInteger(const SymbolicDescription* BaseVariable) const;
	long_long_t getCoefficientAsInteger(const Reprise::VariableDeclaration* BaseVariable) const;
	double getCoefficientAsDouble(const SymbolicDescription* BaseVariable) const;
	double getCoefficientAsDouble(const Reprise::VariableDeclaration* BaseVariable) const;
	Coefficient getCoefficient(Reprise::VariableDeclaration* BaseVariable) { return this->operator [](BaseVariable); } //*(new Reprise::ReprisePtr<CoefficientRef>())
    //получает коэффициенты при всех переменных, не являющихся счетчиками циклов
	CanonicalLinearExpression getExternalParamCoefficients(const VariablesDeclarationsVector &loopCounters) const;
	
    // Методы получения свободных коэффициентов 
	long_long_t getFreeCoefficientAsInteger() const;
	double getFreeCoefficientAsDouble() const;
	Coefficient getFreeCoefficient() const { return m_freeSummand; }

    ParametricLinearExpression* operator +(const ParametricLinearExpression& secondArg) const;
    ParametricLinearExpression* operator -(const ParametricLinearExpression& secondArg) const;
    ParametricLinearExpression& operator =(const ParametricLinearExpression& rhs);

	// Возвращает противоположное линейное выражение (умноженное на -1)
    ParametricLinearExpression* getOpposite() const;

	// Добавление константы к линейному выражению
	ParametricLinearExpression& add(const Reprise::LiteralExpression& summand);

	// Добавление переменной к линейному выражению
	ParametricLinearExpression& add(const Reprise::ReferenceExpression& summand);

	// Умножение линейного выражения на константу 
	ParametricLinearExpression& multiply(const Reprise::BasicLiteralExpression* multiplier);

	// Умножение линейного выражения на переменную
	ParametricLinearExpression& multiply(const Reprise::ReferenceExpression* multiplier);

	/// Примечание: умножение возможно только в случае, если не пермножаются переменные из базы линеаризации 
	bool multiply(const ParametricLinearExpression* multiplier);

	/**
		Деление возможно только в случае, если переменные из базы линеаризации не деляться
		Примечание: делить мы можем только, если делитель не зависит от базы линеаризации делимого
	*/
	bool divide(const ParametricLinearExpression* divider);

	/**
		\brief Проверяет зависит ли выражение, хотя бы от одной из переменных базы линеаризации
		\return
			\retval true - коэффициенты при всех переменных базы линеаризации равны 0.
			\retval false - хотя бы один из коэффициентов не равнен 0.
	*/
	bool isIndependent() const;

	/**
		\brief Проверяет можно ли вычислить все коэффициенты линейного выражения
		\return
			\retval true - все коэффициенты не зависят параметров и являются числовыми.
			\retval false - хотя бы один из коэффициентов зависит от внешнего параметра.
	*/
	bool isEvaluatable() const { return m_evaluatable; }

	/**
		\brief Проверяет равно ли выражение 0
		\return
			\retval true - выражение равно 0.
			\retval false - выражение не равно 0.
	*/
	bool isZero() const;

	//Выдает список VariableDeclaration's переменных, коэффициенты при которых отличны от нуля
	//std::list<Reprise::VariableDeclaration*> getVariables() const;
	VariablesDeclarationsVector getVariables() const;

	/// Выдает хеш-список переменных и коэффициэнтов при них
	std::map<Reprise::VariableDeclaration*, long_long_t> getMap();

	// Получает коэффициент для этого BaseVariable, с учетом возможности равенства его другому SymbolicDescription
	Coefficient operator[](SymbolicDescription* BaseVariable) const; 
	Coefficient operator[](unsigned BaseVariableNumber) const;

	/// Выдает первый встретившийся коэффициент при BaseVariable. Для массива их может быть несколько!
	Coefficient operator[](Reprise::VariableDeclaration* BaseVariable) const;

	/// Проверяет содержит ли база линеаризации this базу линеаризации Second как подмножество
	bool includesLinearizationBaseOf(const ParametricLinearExpression& Second) const;

	/// Вывод в строку
	std::string dumpState() const;

	/// Выдает список используемых вхождений
	OccurenceDescriptionList getOccurencesDescriptionsList(Reprise::DeclarationBase* DeclarationSample) const;

    /// Выдает выражение сконструированное по линейному выражению
    Reprise::ExpressionBase* convert2RepriseExpression() const;

private:
	/** \brief Не безопасная функция умножения на ExpressionBase* 
		Предупреждение: если выражение содержит хотя бы одну переменную из базы линеаризации, то это не будет учтено!
		\param multiplier - множитель или делитель, в зависимости от \see OperationKind
		\param OperationKind - тип операции (умножение или деление), т.к. ф-ция private то подаваться будут только корректные типы
	*/
	void multiplyUnsafe(const Reprise::ExpressionBase& multiplier, Reprise::BasicCallExpression::BuiltinCallKind OperationKind = Reprise::BasicCallExpression::BCK_MULTIPLY);

	// Получает коэффициент для этого ExactlyVariable, и без учета возможности равенства его другому SymbolicDescription
	Coefficient getCoefficientExactly(SymbolicDescription* ExactlyVariable) const;

	// Ищет в m_occurenceDescriptions такое описание переменной, которое равно Prototype
	SymbolicDescription* findEqualExpressionVariable(const SymbolicDescription* Prototype) const;

	// Добавляет элементы в ExpressionVariablesMap и SummandsMap по
	void setSummandsInfo(const SymbolicDescription* Var, const Reprise::ExpressionBase* Coef);

	// Копирует поля из Original
	void copyFrom(const ParametricLinearExpression* Original);

	// Вспомогательные функции
	static long_long_t getLiteralAsInteger(const Reprise::LiteralExpression* Literal); // TODO: const и вынести их отсюда
	static double getLiteralAsDouble(const Reprise::LiteralExpression* Literal); // TODO: const и вынести их отсюда


	SummandsMap m_summands;
	ExpressionVariablesMap m_occurenceDescriptions;
	Reprise::ReprisePtr<CoefficientRef> m_freeSummand;
	bool m_evaluatable;
};

}
}

#endif

// TODO 1. Сделать в ParametricLinearExpression сравнение баз линеаризации
// TODO 2. Сделать операции над ParametricLinearExpression так, чтобы они объединяли базы линеаризации (с предупреждением)

