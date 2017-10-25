#ifndef OPS_SHARED_DATASHARED_H_INCLUDED__
#define OPS_SHARED_DATASHARED_H_INCLUDED__

#include "Reprise/Declarations.h"
#include "Reprise/Types.h"
#include "Reprise/Expressions.h"
#include "Reprise/ProgramFragment.h"
#include <vector>
#include <list>

namespace OPS
{
namespace Shared
{
	
/// возвращает размер массива
void getArrayLimits(OPS::Reprise::TypeBase* typeBase, std::vector<int>& limits);

/** Возвращает размер типа.
Для массивов - число элементов массива
Для скаляров - 1
*/
int getTypeSize(OPS::Reprise::TypeBase& typeBase);

/**
	Возвращает размерность переменной
*/
int getTypeDimension(OPS::Reprise::TypeBase& typeBase);

/// возвращает тип элемента массива
OPS::Reprise::TypeBase* getArrayElementBasicType(OPS::Reprise::TypeBase* pTypeBase);


/// Функция, которая собирает информацию обо всех взодящих в выражение переменных 
std::list<Reprise::VariableDeclaration*> getAllVariableDeclarations(Reprise::ExpressionBase* ExpressionToInvestigate);

/** Returns:
	input                              output
	a // ReferenceExpr           ----> a // VariableDeclaration
	A[i][j] // Array access      ----> A // VariableDeclaration
*/
OPS::Reprise::VariableDeclaration* getArrayVariableDeclaration(OPS::Reprise::ExpressionBase& expression);

/// Returns variables which are declared inside input statement
std::set<Reprise::VariableDeclaration*> getDeclaredVariables(Reprise::StatementBase* pStatement);
std::set<Reprise::VariableDeclaration*> getDeclaredVariables(const Reprise::ProgramFragment& fragment);

/**
	Возвращает тип массива для векторизации исходного
	length - число элементов массива, которые помещаются в вектор
*/
Reprise::ReprisePtr<Reprise::ArrayType> getVectorizedArray(const Reprise::ArrayType* arrayToVectorise, int length);

namespace Literals
{
/**
	Функция, которая пытается вычислить результат сложения двух литералов
*/
Reprise::ExpressionBase* addWithEvaluating(const Reprise::LiteralExpression* firstArg, const Reprise::LiteralExpression* secondArg, bool& overflow);

/**
	Функция, которая пытается вычислить результат разности двух литералов
*/
Reprise::ExpressionBase* subtractWithEvaluating(const Reprise::LiteralExpression* firstArg, const Reprise::LiteralExpression* secondArg, bool& overflow);

/**
	Функция, которая пытается вычислить результат умножения двух литералов
*/
Reprise::ExpressionBase* multiplyWithEvaluating(const Reprise::LiteralExpression* firstArg, const Reprise::LiteralExpression* secondArg, bool& overflow);

/**
	Функция, которая пытается вычислить результат умножения двух выражений (возможно литералов)
*/
Reprise::ExpressionBase* multiplyWithLiteralSimplification(const Reprise::ExpressionBase* firstArg, const Reprise::ExpressionBase* secondArg, bool& overflow);

/**
	Функция, которая пытается вычислить результат деления двух литералов
*/
Reprise::ExpressionBase* divideWithEvaluating(const Reprise::LiteralExpression* firstArg, const Reprise::LiteralExpression* secondArg, bool& overflow);

/**
	Функция, которая пытается вычислить противоположное число к данному
*/
Reprise::ExpressionBase* getOpposite(const Reprise::LiteralExpression* arg);

bool couldBeImplicitlyConverted(OPS::Reprise::TypeBase* MainType, OPS::Reprise::TypeBase* TypeToConvert);

/** Функция, которая проверяет не равно ли выражение ExpressionToInvestigate значению Value, без приведения подобных.
    Удобно для проверки ExpressionBase на равенство 0 или 1.
*/
bool isEqualToIntValue(const Reprise::ExpressionBase* ExpressionToInvestigate, int Value);

enum SimpleLiterals
{
    SL_NOT_SIMPLE,
    SL_NEGATIVE_ONE,
    SL_ZERO,
    SL_POSITIVE_ONE
};

SimpleLiterals specifySimplicityOfExpression(const OPS::Reprise::ExpressionBase* expression);
}

// Returns generators in statement(exclude loops' heads)
//std::list<OccurenceInfo> getGeneratorsInfo(OPS::Reprise::StatementBase& statement, bool isCStyle);

// возращает TranslationUnit для RepriseBase
OPS::Reprise::TranslationUnit* getTranslationUnit(OPS::Reprise::RepriseBase*);

// Возвращает число [] при ссылке на переменную. Например:
// referenceExpression == r в выражении (r + ...) --> 0   
// referenceExpression == r в выражении (r[i] + ...) --> 1
// referenceExpression == r в выражении (r[i][j] + ...) --> 2
// Идет вверх по дереву Reprise и смотрит тип операции родителя
int getArrayAccessDepth(OPS::Reprise::ReferenceExpression& referenceExpression);
}
}

#endif //OPS_SHARED_DATASHARED_H_INCLUDED__
