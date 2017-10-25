/*
	Montego/Shared/Loops.h - Loop description classes

*/

//  Multiple include guard start
#ifndef OPS_ANALYSIS_MONTEGO_SHARED_LOOPS_H__
#define OPS_ANALYSIS_MONTEGO_SHARED_LOOPS_H__

//  Standard includes

//  OPS includes
#include "Reprise/Reprise.h"
#include "Shared/ParametricLinearExpressions.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Montego
{

//  Constants and enums

//  Global classes

// Будем рассматривать циклы следущих видов:
// i = const1; i < const2; i = i + const3 - цикл с константными границами
// i = n + const1; i < m + const2; i = i + const3 - цикл с границами, линейно зависящими только от внешних переменных
// i = j + n + const1; i < k +  m + const2; i = i + const3 - цикл с границами, линейно зависящими от индексов внешних циклов и внешних переменных
// i = expr1; i < expr2; i = i + expr3 - цикл со счетчиком общего вида

class GenericLoop;


/// Класс, описывающий гнездо циклов
class LoopNest : public std::vector<GenericLoop*>
{
public:
	/// Возвращает счетчики циклов
	std::vector<Reprise::VariableDeclaration*> getCounters() const;
};

/// Описание цикла общего вида:
///   for(counter = initExpr; counter < finalExpr; counter = counter + stepExpr)
/// Где: counter - переменная, счетчик цикла
///      initExpr, finalExpr, stepExpr - произвольные выражения
///   for(i = 10 + 1; i <= 20; i = i + 1)
/// finalExpr будет равно не 20, а (20 + 1)
// TODO: еще нужно доказать, что счетчик цикла не меняется внутри цикла, а меняется только в заголовке
class GenericLoop : public NonCopyableMix
{
public:

	/// Возвращает ссылку на оператор цикла во внутреннем представлении
	Reprise::ForStatement& getForStmt() const;

	/// Возвращает ссылку на определение переменной-счетчика цикла
	Reprise::VariableDeclaration& getCounter() const;

	/// Возвращает ссылку на инициализирующее выражение
	Reprise::ExpressionBase& getInitExpr() const;

	/// Возвращает ссылку на конечное выражение для счетчика цикла
	Reprise::ExpressionBase& getFinalExpr() const;

	Reprise::ExpressionBase& getStepExpr() const;

	/// Возвращает true, если данный цикл вложен в другой
	bool hasParentLoop() const;

	/// Возвращает вышестоящий цикл
	GenericLoop& getParentLoop() const;

	/// Возвращает количество циклов в гнезде = количество циклов в которые вложен данный + 1
	int getNestSize() const;

	/// Возвращает все циклы гнезда, включая данный
	LoopNest getNest();

	virtual ~GenericLoop() {}

protected:
	GenericLoop(Reprise::ForStatement& forStmt,
				Reprise::VariableDeclaration& counter,
				Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
				Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
				Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
				GenericLoop* parentLoop = 0
				);

	friend class LoopFactory;

private:
	Reprise::ForStatement* m_forStmt;
	Reprise::VariableDeclaration* m_counter;
	Reprise::ReprisePtr<Reprise::ExpressionBase> m_initExpr, m_finalExpr, m_stepExpr;
	GenericLoop* m_parentLoop;
};

/// Граница цикла, зависящая от счетчиков внешних циклов и внешних переменных
/// В отлиции от других подобных классов, верктор коэффициентов не содержит свободный член.
class ParametricBound
{
public:
	/// Вектор коэффициентов перед счетчиками циклов
	typedef std::vector<int> CoefficientVector;

	/// Инициализировать границу вектором коэффициентов и свободным выражением
	ParametricBound(const CoefficientVector& coeffs, const Shared::CanonicalLinearExpression& freeExpr);

	/// Инициализировать границу нулевым вектором коэффициентов и свободным выражением
	/// \param loopsCount - количество внешних циклов
	ParametricBound(int loopsCount, const Shared::CanonicalLinearExpression& freeExpr);

	/// Инициализировать границу нулевым вектором коэффициентов и свободным слагаемым-константой.
	explicit ParametricBound(int loopsCount, Shared::CanonicalLinearExpression::Coefficient freeSummand = 0);

	/// Возвращает вектор коэффициентов перед счетчиками внешних циклов
	CoefficientVector& getCoefficients();
	const CoefficientVector& getCoefficients() const;

	/// Возвращает свободное выражение, содержащее внешние переменные и свободный член
	Shared::CanonicalLinearExpression& getFreeExpr();
	const Shared::CanonicalLinearExpression& getFreeExpr() const;

private:
	CoefficientVector m_coeffs;
	Shared::CanonicalLinearExpression m_freeExpr;
};

class ParametricBoundsLoop : public GenericLoop
{
public:
	const ParametricBound& getLeftParametricBound() const;
	const ParametricBound& getRightParametricBound() const;

protected:
	ParametricBoundsLoop(Reprise::ForStatement& forStmt,
						 Reprise::VariableDeclaration& counter,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
						 const ParametricBound& leftBound,
						 const ParametricBound& rightBound,
						 int step,
						 GenericLoop* parentLoop = 0);
	friend class LoopFactory;

private:
	ParametricBound m_leftParametricBound;
	ParametricBound m_rightPametricBound;
	int m_step;
};

class BasicBoundsLoop : public ParametricBoundsLoop
{
public:
	const Shared::CanonicalLinearExpression& getLeftBound() const;
	const Shared::CanonicalLinearExpression& getRightBound() const;

protected:
	BasicBoundsLoop(Reprise::ForStatement& forStmt,
					Reprise::VariableDeclaration& counter,
					Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
					Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
					Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
					int loopsCount,
					const Shared::CanonicalLinearExpression& leftBound,
					const Shared::CanonicalLinearExpression& rightBound,
					int step,
					GenericLoop* parentLoop = 0);

	friend class LoopFactory;
};

class ConstBoundsLoop : public BasicBoundsLoop
{
public:
	bool isCanonized() const;

	int getLeftBound() const;
	int getRightBound() const;

protected:
	ConstBoundsLoop(Reprise::ForStatement& forStmt,
					Reprise::VariableDeclaration& counter,
					Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
					Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
					Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
					int loopsCount,
					int lowerBound,
					int upperBound,
					int step,
					GenericLoop* parentLoop = 0
					);

	friend class LoopFactory;
};

//  Global functions

/// Создает описание цикла
GenericLoop* createLoop(Reprise::ForStatement& forStmt, GenericLoop* parentLoop = 0);

//  Exit namespace
}
}

//  Multiple include guard end
#endif		//	OPS_ANALYSIS_MONTEGO_SHARED_LOOPS_H__
