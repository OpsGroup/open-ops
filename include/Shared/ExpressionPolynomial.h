#ifndef OPS_SHARED_EXPRESSIONPOLYNOMIAL_H_INCLUDED__
#define OPS_SHARED_EXPRESSIONPOLYNOMIAL_H_INCLUDED__

#include "Shared/ExpressionOperators.h"

#include <vector>

namespace OPS
{
namespace Shared
{

class Polynomial;
class PriorityMonomial;

/// Абстрактный класс необходимый для построения деревьев одночленов с учетом приоритетов
class MonomialOperand: public Reprise::IntrusivePointerBase, public TypeConvertibleMix //public Reprise::ExpressionBase
{
public:
	/// Возвращает дерево выражений Reprise 
	virtual Reprise::ExpressionBase* convert2RepriseExpression() const = 0;

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	virtual void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind) = 0;

	/// Создание копии
	virtual MonomialOperand* clone(void) const = 0;

	/// Вывод в виде текста
	std::string dumpState(void)
	{
		return Reprise::ReprisePtr<Reprise::ExpressionBase>(convert2RepriseExpression())->dumpState();
	}

	/// Сравнивает на равенство два выражения с учетом коммутативных законов и пр
	virtual bool isEqual(const MonomialOperand* SecondArg) const = 0;
};

/**
	Абстрактный класс, описывающий неделимую для алгебраических операций единицу.
	Его потомки описывают скаляры, массивы, структура, ... 
*/
class SymbolicDescription: public MonomialOperand, public OPS::BaseVisitable<>
{ // класс нужен чтобы отделить переменные (SymbolicDescription) от констант (MonomialConstant)
public:
	/// Создает описание "символьной переменной" 
	static SymbolicDescription* createSymbolicDescription(Reprise::ExpressionBase& Prototype);

	/// Создание копии
	virtual SymbolicDescription* clone(void) const = 0;

	/// Возвращает первичное объявление "символьной переменной"
	virtual Reprise::DeclarationBase* getDeclaration() const = 0;
};

/**
	Класс, описывающий скалярные переменные, в алгебраическом смысле. 
*/
class SymbolicScalar: public SymbolicDescription
{
public:
	// Конструктор
	explicit SymbolicScalar(Reprise::VariableDeclaration* Prototype):
		m_DeclarationName(Prototype) {};

	/// Сравнение на равенство
	bool isEqual(const MonomialOperand* SecondArg) const;

	/// Возвращает дерево выражений Reprise для данного представления переменной.
	Reprise::ExpressionBase* convert2RepriseExpression() const;

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind) {};
	
	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SymbolicScalar)

	/// Возвращает первичное объявление "символьной переменной"
	Reprise::DeclarationBase* getDeclaration() const { return m_DeclarationName; } 
private:
	Reprise::VariableDeclaration* m_DeclarationName;
};

/**
	Класс, описывающий обращение к элементу массива, в алгебраическом смысле. 
*/
class SymbolicArray: public SymbolicDescription
{
public:
	// Конструктор
	explicit SymbolicArray(SymbolicDescription* Head, PriorityMonomial* SubExpression):
		m_Head(Head), m_SubExpression(SubExpression) {};

	/// Сравнение на равенство
	bool isEqual(const MonomialOperand* SecondArg) const;

	/// Возвращает дерево выражений Reprise для данного представления переменной.
	Reprise::ExpressionBase* convert2RepriseExpression() const;

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind) {};
	
	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SymbolicArray)

	/// Возвращает первичное объявление "символьной переменной"
	Reprise::DeclarationBase* getDeclaration() const { return m_Head->getDeclaration(); }
private:
	Reprise::ReprisePtr<SymbolicDescription> m_Head;
	Reprise::ReprisePtr<PriorityMonomial> m_SubExpression; // главное отличие от обычного Expression
							// - подвыражения можно сравнивать с учетом коммутативности и ассоциативности,
							// поэтому подвыражение представлено PriorityMonomial
};

/**
	Класс, описывающий обращение к полю структуры, в алгебраическом смысле. 
*/
class SymbolicStruct: public SymbolicDescription
{
public:
	// Конструктор
	explicit SymbolicStruct(SymbolicDescription* Head, Reprise::StructMemberDescriptor* SubExpression):
		m_Head(Head), m_SubExpression(SubExpression) {};

	/// Сравнение на равенство
	bool isEqual(const MonomialOperand* SecondArg) const;

	/// Возвращает дерево выражений Reprise для данного представления переменной.
	Reprise::ExpressionBase* convert2RepriseExpression() const;

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind) {};
	
	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SymbolicStruct)

	/// Возвращает первичное объявление "символьной переменной"
	Reprise::DeclarationBase* getDeclaration() const { return m_Head->getDeclaration(); }
private:
	Reprise::ReprisePtr<SymbolicDescription> m_Head;
	Reprise::StructMemberDescriptor* m_SubExpression; // главное отличие от обычного Expression
										// - подвыражения можно сравнивать с учетом коммутативности и ассоциативности
};

/**
	Класс, описывающий унарные операции. 
*/
class SymbolicUnaryOperator: public SymbolicDescription
{
public:
	// Конструктор
	explicit SymbolicUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind Kind, SymbolicDescription* SubExpression);

	/// Сравнение на равенство
	bool isEqual(const MonomialOperand* SecondArg) const;

	/// Возвращает дерево выражений Reprise для данного представления переменной.
	Reprise::ExpressionBase* convert2RepriseExpression() const;

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind) {};
	
	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SymbolicUnaryOperator)

	/// Возвращает первичное объявление "символьной переменной"
	Reprise::DeclarationBase* getDeclaration() const { return m_SubExpression->getDeclaration(); }
private:
	bool isCorrect() const;

	Reprise::BasicCallExpression::BuiltinCallKind m_Kind;
	Reprise::ReprisePtr<SymbolicDescription> m_SubExpression;
};

class PriorityMonomial: public MonomialOperand
{
public:
	
	/// Конструктор
	PriorityMonomial(): m_Coefficient(0) { }
	explicit PriorityMonomial(const MonomialOperand& Variable);

	static PriorityMonomial* createPriorityMonomial(const Reprise::ExpressionBase& OriginalExpression);

	/// Применить операцию к текущему приоритетному одночлену. Возможно наличие других аргументов.
	bool applyOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind, const PriorityMonomial* SecondOperand = 0, const PriorityMonomial* ThirdOperand = 0);

	/// Получить приоритет приоритетного одночлена.
	EN_Operator_Precedence_Level getPrecedencyLevel() const;

	/// Определяет одинаковые ли операции в приоритетном одночлене
	bool areOperatorsTheSame() const;

	/// Выполняет упрощения
	//void makeReduction();

	/// Возвращает дерево выражений Reprise для данного приоритетного одночлена.
	virtual Reprise::ExpressionBase* convert2RepriseExpression() const;

	/// Применяет унарную операцию
	virtual void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind);

	/// Сравнивает на равенство два выражения с учетом коммутативных законов и пр
	virtual bool isEqual(const MonomialOperand* SecondArg) const;

	OPS_DEFINE_CLONABLE_INTERFACE(PriorityMonomial);
private:
	/// Обнуление одночлена
	void clear() 
	{ 
		m_Operators.clear(); 
		m_Operands.clear(); 
		if (m_Coefficient.get() != 0)
			m_Coefficient.release(); 
	}

	/// Преобразование дистрибутивности справа. Законность применения дистрибутивности НЕ проверяется!
	void distributeRight(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind, const PriorityMonomial* LeftOperand);

	/// Преобразование дистрибутивности слева. Законность применения дистрибутивности НЕ проверяется!
	void distributeLeft(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind, const PriorityMonomial* RightOperand);

	/// Преобразование двойного отрицания. Законность применения НЕ проверяется!
	void applyDoubleNegationLaw();

	/// Преобразование по закону ДеМоргана. Законность применения проверяется!
	void applyDeMorgansLaw(Reprise::BasicCallExpression::BuiltinCallKind NegationOperator);

	std::vector<Reprise::BasicCallExpression::BuiltinCallKind> m_Operators;
	std::vector<Reprise::ReprisePtr<MonomialOperand> > m_Operands;
	Reprise::ReprisePtr<Reprise::StrictLiteralExpression> m_Coefficient;
};

class MonomialConstant: public MonomialOperand
{
public:
	explicit MonomialConstant(Reprise::StrictLiteralExpression& Prototype): m_Constant(&Prototype) {}

	/// Возвращает дерево выражений Reprise для данного приоритетного одночлена.
	virtual Reprise::ExpressionBase* convert2RepriseExpression() const 
	{ 
		return getStrictLiteralClone(); 
	}

	/// Возвращает копию литерала
	Reprise::StrictLiteralExpression* getStrictLiteralClone() const 
	{ 
		return m_Constant->clone(); 
	}

	/// Применяет одноместные (унарные в математическом смысле) операции к операндам
	virtual void applyUnaryOperator(Reprise::BasicCallExpression::BuiltinCallKind OperatorKind);

	/// Сравнивает на равенство два выражения с учетом коммутативных законов и пр
	virtual bool isEqual(const MonomialOperand* SecondArg) const;
	
	OPS_DEFINE_CLONABLE_INTERFACE(MonomialConstant)
private:
	Reprise::ReprisePtr<Reprise::StrictLiteralExpression> m_Constant;
};

} // end namespace Shared
} // end namespace OPS

#endif

// TODO: 0! Какого типа описание у структур, массивов, функций? Нужен ли нам DeclarationBase или достаточно VariableDeclaration?
// TODO: 1. Учет наличия преобразований типов в преобразуемом выражении
// TODO: 2. Ввести в рассмотрение константы как операнды
// TODO: 3. Учет преобразования выражений с плавающей точкой к целочисленному значению.
// TODO: 4. Ввести Вычислительное упрощение и Алгебраическое упрощение.
// TODO: 5. Вычислительное упрощение: Исключение преобразований выражений, содержащих операции с плавающей точкой, исключающей ассоциативность
// TODO: 6. Алгебраическое упрощение: Операции div и mod не дистрибутивна. А вещественное деление дистрибутивно только справа и только для примерно равных чисел.
// TODO: 7. Учет наличия вызовов функций в преобразуемом выражении
