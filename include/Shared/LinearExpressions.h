#ifndef OPS_SHARED_LINEAREXPRESSIONS_H_INCLUDED__
#define OPS_SHARED_LINEAREXPRESSIONS_H_INCLUDED__

#include "Shared/ParametricLinearExpressions.h"
#include "Reprise/Statements.h"
#include <map>
#include <stack>
#include <vector>

namespace OPS
{
namespace Shared
{

/// Класс описывающий матрицу коэффициентов для вхождения переменной
class LinearExpressionMatrix
{
public:
	typedef std::vector<ParametricLinearExpression*> Coefficients;

	explicit LinearExpressionMatrix(OPS::Reprise::ExpressionBase* Node);
	~LinearExpressionMatrix();

	int getLoopNestDepth() { return m_loopNestDepth; }
	int getArrayDimensionsCount() { return m_arrayDimensionsCount; }
	Coefficients detachCoefficients();
	ParametricLinearExpression& operator[](unsigned i);

	// Упрощенный доступ к коэффициентам целого типа
	long_long_t getCoefficientAsInteger(unsigned i, unsigned j);

private:
	unsigned m_loopNestDepth; // кол-во вложенных циклов охватывающих вхождение, содержащее это линейное выражение
	int m_arrayDimensionsCount; // кол-во размерностей массива, являющегося вхождением, содержащим это линейное выражение 
	Coefficients m_linearExpressions; // коэффициенты при счетчиках циклов
	ParametricLinearExpression::VariablesDeclarationsVector m_LinearizationBase; // всегда счетчики циклов, которые можно зафиксировать их VariableDeclaration, не скаляры не рассматриваются
	const OPS::Reprise::ExpressionBase* m_OriginalExpression;
	
};

/// Класс описывающий вхождение переменной вида x[exp1][exp2]...[expn].
/// Каждое индексное выражения разбирается как линейное, если это возможно. 
class Occurrence
{
public:
	explicit Occurrence(Reprise::ExpressionBase* Node)
			:m_node(Node)
			,m_indexExpressionsRepresentation(Node)
	{
	}

	// Позволяет получить коэффициент при переменной
	Reprise::ReprisePtr<Reprise::ExpressionBase> getCoefficient(OPS::Reprise::VariableDeclaration* BaseVariable, int DimensionNumber = 0);
	
	// Позволяет получить свободный коэффициент одного из индексных выражений (в измерении) массива
	Reprise::ReprisePtr<Reprise::ExpressionBase> getFreeCoefficient(int DimensionNumber = 0); 
private:
	OPS::Reprise::ExpressionBase* m_node;
	LinearExpressionMatrix m_indexExpressionsRepresentation;
};

//ParametricLinearExpression* buildLinearExpression(const std::vector<Reprise::VariableDeclaration*>& IndexVariables, Reprise::ExpressionBase* Node);
bool isLinear(const std::vector<OPS::Reprise::VariableDeclaration*>& listOfVar, OPS::Reprise::ExpressionBase* Node);

class OccurenceInfo
{
public:
	typedef std::list<OPS::Reprise::ForStatement*> LoopsNest;
	typedef std::vector<int> ArrayLimits;
	typedef std::map<OPS::Reprise::VariableDeclaration*, Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> > LinearisationCoefficients;

public:
	explicit OccurenceInfo(OPS::Reprise::ExpressionBase& expression, bool isCStyle);

	OccurenceInfo(const OccurenceInfo& occurenceInfo);

	OccurenceInfo& operator=(const OccurenceInfo& occurenceInfo);

	OPS::Reprise::ExpressionBase& getExpression();

	OPS::Reprise::VariableDeclaration* getDeclaration();

	ArrayLimits getArrayLimits();

	LinearExpressionMatrix& getExpressionMatrix();

	Occurrence& getOccurence();

	LoopsNest& getLoopsNest();

	LinearisationCoefficients& getLinearisationCoefficients();

	~OccurenceInfo();

private:
	void initData(OPS::Reprise::ExpressionBase& expression);

	void uninitData();

private:
	bool m_isCStyle;
	OPS::Reprise::ExpressionBase*      m_pExpression;
	OPS::Reprise::VariableDeclaration* m_pDeclaration;
	ArrayLimits                        m_limits;
	LinearExpressionMatrix*       m_pExpressionMatrix;
	Occurrence*                   m_pOccurence;
	LoopsNest                     m_loopsNest;
	LinearisationCoefficients     m_linearisationCoefficients;
};

} // end namespace Shared
} // end namespace OPS

#endif
