/*
	Montego/Shared/Loops.h - Loop description classes implementation

*/

//  Standard includes

//  OPS includes
#include "Analysis/Montego/Loops.h"

//  Local includes

//  Namespaces using
using namespace OPS::Reprise;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Montego
{

//  Constants and enums

//  Classes
class LoopFactory
{
public:
	GenericLoop* createLoop(Reprise::ForStatement& forStmt, GenericLoop* parentLoop);

private:
	GenericLoop* createLoop();

	GenericLoop* createLoop(const ParametricBound::CoefficientVector& leftBoundCoefficients,
							const Shared::CanonicalLinearExpression& leftBoundFreeExpr,
							const ParametricBound::CoefficientVector& rightBoundCoefficients,
							const Shared::CanonicalLinearExpression& rightBoundFreeExpr,
							int step);

	Reprise::ForStatement* m_forStmt;
	Reprise::VariableDeclaration* m_counter;
	Reprise::ReprisePtr<Reprise::ExpressionBase> m_initExpr, m_finalExpr, m_stepExpr;
	GenericLoop* m_parentLoop;
};

//  Functions declaration
bool isZeroVector(const ParametricBound::CoefficientVector& vec);
void getCoefficients(Shared::ParametricLinearExpression& expr,
					 const std::vector<VariableDeclaration*>& loopCounters,
					 ParametricBound::CoefficientVector& coefficients);
void getFreeExpr(Shared::ParametricLinearExpression& expr,
				 const std::vector<VariableDeclaration*>& loopCounters,
				 Shared::CanonicalLinearExpression& freeExpr);

//  Variables

//  Classes implementation

//  Global classes implementation

std::vector<VariableDeclaration*> LoopNest::getCounters() const
{
	std::vector<VariableDeclaration*> counters;
	for(size_t i = 0; i < this->size(); ++i)
		counters.push_back(&at(i)->getCounter());
	return counters;
}

GenericLoop::GenericLoop(Reprise::ForStatement &forStmt,
						 Reprise::VariableDeclaration &counter,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
						 Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
						 GenericLoop *parentLoop)
	:m_forStmt(&forStmt)
	,m_counter(&counter)
	,m_initExpr(initExpr)
	,m_finalExpr(finalExpr)
	,m_stepExpr(stepExpr)
	,m_parentLoop(parentLoop)
{
}

Reprise::ForStatement& GenericLoop::getForStmt() const
{
	return *m_forStmt;
}

Reprise::VariableDeclaration& GenericLoop::getCounter() const
{
	return *m_counter;
}

Reprise::ExpressionBase& GenericLoop::getInitExpr() const
{
	return *m_initExpr.get();
}

Reprise::ExpressionBase& GenericLoop::getFinalExpr() const
{
	return *m_finalExpr.get();
}

Reprise::ExpressionBase& GenericLoop::getStepExpr() const
{
	return *m_stepExpr.get();
}

bool GenericLoop::hasParentLoop() const
{
	return m_parentLoop != 0;
}

GenericLoop& GenericLoop::getParentLoop() const
{
	return *m_parentLoop;
}

int GenericLoop::getNestSize() const
{
	int nestSize = 1;
	GenericLoop* parentLoop = m_parentLoop;
	while(parentLoop)
	{
		nestSize++;
		parentLoop = parentLoop->m_parentLoop;
	}

	return nestSize;
}

LoopNest GenericLoop::getNest()
{
	LoopNest nest;
	nest.resize(getNestSize(), 0);

	nest.back() = this;

	GenericLoop* parentLoop = m_parentLoop;
	for(int i = nest.size()-2; i >= 0; --i)
	{
		nest[i] = parentLoop;
		parentLoop = parentLoop->m_parentLoop;
	}

	return nest;
}

ParametricBound::ParametricBound(const CoefficientVector &coeffs, const Shared::CanonicalLinearExpression &freeExpr)
	:m_coeffs(coeffs)
	,m_freeExpr(freeExpr)
{
}

ParametricBound::ParametricBound(int loopsCount, const Shared::CanonicalLinearExpression &freeExpr)
	:m_coeffs(loopsCount)
	,m_freeExpr(freeExpr)
{
}

ParametricBound::ParametricBound(int loopsCount, Shared::CanonicalLinearExpression::Coefficient freeSummand)
	:m_coeffs(loopsCount)
	,m_freeExpr(freeSummand)
{
}

ParametricBound::CoefficientVector& ParametricBound::getCoefficients()
{
	return m_coeffs;
}

const ParametricBound::CoefficientVector& ParametricBound::getCoefficients() const
{
	return m_coeffs;
}

Shared::CanonicalLinearExpression& ParametricBound::getFreeExpr()
{
	return m_freeExpr;
}

const Shared::CanonicalLinearExpression& ParametricBound::getFreeExpr() const
{
	return m_freeExpr;
}

ParametricBoundsLoop::ParametricBoundsLoop(Reprise::ForStatement &forStmt,
										   Reprise::VariableDeclaration &counter,
										   Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
										   Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
										   Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
										   const ParametricBound &leftBound,
										   const ParametricBound &rightBound,
										   int step,
										   GenericLoop *parentLoop)
	:GenericLoop(forStmt, counter, initExpr, finalExpr, stepExpr, parentLoop)
	,m_leftParametricBound(leftBound)
	,m_rightPametricBound(rightBound)
	,m_step(step)
{
}

const ParametricBound& ParametricBoundsLoop::getLeftParametricBound() const
{
	return m_leftParametricBound;
}

const ParametricBound& ParametricBoundsLoop::getRightParametricBound() const
{
	return m_rightPametricBound;
}

BasicBoundsLoop::BasicBoundsLoop(Reprise::ForStatement &forStmt,
								 Reprise::VariableDeclaration &counter,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
								 int loopsCount,
								 const Shared::CanonicalLinearExpression &leftBound,
								 const Shared::CanonicalLinearExpression &rightBound,
								 int step,
								 GenericLoop *parentLoop)
	:ParametricBoundsLoop(forStmt, counter, initExpr, finalExpr, stepExpr,
		  ParametricBound(loopsCount, leftBound), ParametricBound(loopsCount, rightBound), step, parentLoop)
{
	OPS_ASSERT(loopsCount == getNestSize()-1);
}

const Shared::CanonicalLinearExpression& BasicBoundsLoop::getLeftBound() const
{
	return getLeftParametricBound().getFreeExpr();
}

const Shared::CanonicalLinearExpression& BasicBoundsLoop::getRightBound() const
{
	return getRightParametricBound().getFreeExpr();
}

ConstBoundsLoop::ConstBoundsLoop(Reprise::ForStatement &forStmt,
								 Reprise::VariableDeclaration &counter,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> initExpr,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> finalExpr,
								 Reprise::ReprisePtr<Reprise::ExpressionBase> stepExpr,
								 int loopsCount,
								 int lowerBound,
								 int upperBound,
								 int step,
								 GenericLoop* parentLoop)
	:BasicBoundsLoop(forStmt, counter, initExpr, finalExpr, stepExpr, loopsCount,
		  Shared::CanonicalLinearExpression(lowerBound), Shared::CanonicalLinearExpression(upperBound),
		  step, parentLoop)
{
}

bool ConstBoundsLoop::isCanonized() const
{
	return getLeftBound() == 0;
}

int ConstBoundsLoop::getLeftBound() const
{
	return BasicBoundsLoop::getLeftBound().getFreeSummand();
}

int ConstBoundsLoop::getRightBound() const
{
	return BasicBoundsLoop::getRightBound().getFreeSummand();
}

GenericLoop* LoopFactory::createLoop(ForStatement &forStmt, GenericLoop* parentLoop)
{
	// for (i = ...; ...; ...)
	ExpressionBase& initExpr = forStmt.getInitExpression();
	if (!initExpr.is_a<BasicCallExpression>())
	{
		return 0;
	}

	BasicCallExpression& initAssignExpr = initExpr.cast_to<BasicCallExpression>();
	if (initAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !initAssignExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return 0;
	}
	ReferenceExpression& initCounter = initAssignExpr.getArgument(0).cast_to<ReferenceExpression>();

	// for (...; i < ...; ...)
	ExpressionBase& finalExpr = forStmt.getFinalExpression();
	if (!finalExpr.is_a<BasicCallExpression>())
	{
		return 0;
	}
	BasicCallExpression& finalLessExpr = finalExpr.cast_to<BasicCallExpression>();
	if ((finalLessExpr.getKind() != BasicCallExpression::BCK_LESS)
			|| !finalLessExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return 0;
	}
	ReferenceExpression& finalCounter = finalLessExpr.getArgument(0).cast_to<ReferenceExpression>();

	// for (...; ...; i = i + ...)
	ExpressionBase& stepExpr = forStmt.getStepExpression();
	if (!stepExpr.is_a<BasicCallExpression>())
	{
		return 0;
	}
	BasicCallExpression& stepAssignExpr = stepExpr.cast_to<BasicCallExpression>();
	if (stepAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !stepAssignExpr.getArgument(0).is_a<ReferenceExpression>() || !stepAssignExpr.getArgument(1).is_a<BasicCallExpression>())
	{
		return 0;
	}
	ReferenceExpression& stepLeftCounter = stepAssignExpr.getArgument(0).cast_to<ReferenceExpression>();
	BasicCallExpression& stepPlusExpr = stepAssignExpr.getArgument(1).cast_to<BasicCallExpression>();
	if (stepPlusExpr.getKind() != BasicCallExpression::BCK_BINARY_PLUS || !stepPlusExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return 0;
	}
	ReferenceExpression& stepRightCounter = stepPlusExpr.getArgument(0).cast_to<ReferenceExpression>();

	// compare all counters
	if (!initCounter.isEqual(finalCounter) || !stepLeftCounter.isEqual(stepRightCounter) || !initCounter.isEqual(stepLeftCounter))
	{
		return 0;
	}

	m_forStmt = &forStmt;
	m_counter = &initCounter.getReference();
	m_initExpr.reset(&initAssignExpr.getArgument(1));
	m_finalExpr.reset(&finalLessExpr.getArgument(1));
	m_stepExpr.reset(&stepPlusExpr.getArgument(1));
	m_parentLoop = parentLoop;

	return createLoop();
}



GenericLoop* LoopFactory::createLoop()
{
	int step = 1;

	// TODO: проверить, что счетчик является числом

	// получаем список счетчиков вышерасположенных циклов
	std::vector<VariableDeclaration*> counters = m_parentLoop != 0
			? m_parentLoop->getNest().getCounters()
			: std::vector<VariableDeclaration*>();

	// Разбираем выражения границ по счетчикам
	Shared::ParametricLinearExpression* leftParametricExpr =
			Shared::ParametricLinearExpression::createByListOfVariables(m_initExpr.get(), counters);

	Shared::ParametricLinearExpression* rightParametricExpr =
			Shared::ParametricLinearExpression::createByListOfVariables(m_finalExpr.get(), counters);

	if (leftParametricExpr != 0 && rightParametricExpr != 0)
	{
		ParametricBound::CoefficientVector leftBoundCoefficients, rightBoundCoefficients;
		getCoefficients(*leftParametricExpr, counters, leftBoundCoefficients);
		getCoefficients(*rightParametricExpr, counters, rightBoundCoefficients);

		Shared::CanonicalLinearExpression leftBoundFreeExpr, rightBoundFreeExpr;
		getFreeExpr(*leftParametricExpr, counters, leftBoundFreeExpr);
		getFreeExpr(*rightParametricExpr, counters, rightBoundFreeExpr);

		return createLoop(leftBoundCoefficients, leftBoundFreeExpr, rightBoundCoefficients, rightBoundFreeExpr, step);
	}
	else
	{
		return new GenericLoop(*m_forStmt, *m_counter, m_initExpr, m_finalExpr, m_stepExpr, m_parentLoop);
	}
}

GenericLoop* LoopFactory::createLoop(const ParametricBound::CoefficientVector &leftBoundCoefficients, const Shared::CanonicalLinearExpression &leftBoundFreeExpr, const ParametricBound::CoefficientVector &rightBoundCoefficients, const Shared::CanonicalLinearExpression &rightBoundFreeExpr, int step)
{
	if (isZeroVector(leftBoundCoefficients) && isZeroVector(rightBoundCoefficients))
	{
		if (leftBoundFreeExpr.isConstant() && rightBoundFreeExpr.isConstant())
		{
			return new ConstBoundsLoop(*m_forStmt, *m_counter, m_initExpr, m_finalExpr, m_stepExpr, leftBoundCoefficients.size(),
									   leftBoundFreeExpr.getFreeSummand(), rightBoundFreeExpr.getFreeSummand(), step, m_parentLoop);
		}
		else
		{
			return new BasicBoundsLoop(*m_forStmt, *m_counter, m_initExpr, m_finalExpr, m_stepExpr, leftBoundCoefficients.size(),
									   leftBoundFreeExpr, rightBoundFreeExpr, step, m_parentLoop);
		}
	}
	else
	{
		return new ParametricBoundsLoop(*m_forStmt, *m_counter, m_initExpr, m_finalExpr, m_stepExpr,
										ParametricBound(leftBoundCoefficients, leftBoundFreeExpr),
										ParametricBound(rightBoundCoefficients, rightBoundFreeExpr),
										step, m_parentLoop);
	}
}

//  Functions implementation

bool isZeroVector(const ParametricBound::CoefficientVector& vec)
{
	for(size_t i = 0; i < vec.size(); ++i)
		if (vec[i] != 0)
			return false;
	return true;
}

void getCoefficients(Shared::ParametricLinearExpression& expr,
					 const std::vector<VariableDeclaration*>& loopCounters,
					 ParametricBound::CoefficientVector& coefficients)
{
	coefficients.resize(loopCounters.size());
	for (size_t i = 0; i < loopCounters.size(); ++i)
	{
		coefficients[i] = expr.getCoefficientAsInteger(loopCounters[i]);
	}
}

void getFreeExpr(Shared::ParametricLinearExpression& expr,
				 const std::vector<VariableDeclaration*>& loopCounters,
				 Shared::CanonicalLinearExpression& freeExpr)
{
	freeExpr = expr.getExternalParamCoefficients(loopCounters);
	freeExpr.add(expr.getFreeCoefficientAsInteger());
}

GenericLoop* createLoop(Reprise::ForStatement &forStmt, GenericLoop *parentLoop)
{
	return LoopFactory().createLoop(forStmt, parentLoop);
}

//  Exit namespace
}
}

