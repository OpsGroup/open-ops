#ifndef _CSE_H_INCLUDED_
#define _CSE_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

using OPS::Reprise::BlockStatement;
using OPS::Reprise::StatementBase;
using OPS::Reprise::ExpressionStatement;
using OPS::Reprise::ReferenceExpression;
using OPS::Reprise::BasicCallExpression;
using OPS::Reprise::ExpressionBase;
using OPS::Reprise::TypeBase;
using OPS::Reprise::VariableDeclaration;

/** Описывает вычисляемое выражение внутри базового блока */
struct AEB
{
	int position;
	BasicCallExpression* expr;
    Reprise::ReprisePtr<TypeBase> vartype;
	ReferenceExpression* tmpVar;
	BasicCallExpression* wholeExpr;
};
/** Список вычисляемых подвыражений в блоке */
typedef std::list<AEB> AEBList;

/**
 * Производит локальный вынос общих подвыражений в блоке. Эту функцию нужно использовать для вызова преобразования.
 * @param blockStatement Блок, в котором нужно произвести преобразование.
 * @return Количество новых переменных, созданных в блоке.
 */
int makeCommonSubexpressionElimination(BlockStatement& blockStatement);

// Служебные функции
/**
 * Проверяет, является ли указанная операция выражением, вычисляющим бинарное выражение.
 * Если проверка прошла успешно, поля выходного параметра result будут заполнены соответствующими значениями.
 * @param stmt Проверяемая операция.
 * @param result Выходной параметр.
 * @param position Номер операции в блоке. Нужен для заполнения информации о выражении.
 * @return Ответ на вопрос, поставленный в названии функции.
 */
bool isAssignBinary(StatementBase& stmt, AEB* result, int position);

/**
 * @brief Проверяет список вычисляемых выражений на наличие в нем переданного выражения.
 * Если указаное выражение нашлось, то создает в блоке новую переменную, в которую записывается результат вычисления общего подвыражения.
 * Если выражение не найдено, то добавлет его в список вычисляемых выражений.
 * @param AEBs Список вычисляемых выражений.
 * @param expr Проверяемое выражение.
 * @return Возвращает true, если в ходе проверки в блок была добавлена новая операция вычисления общего подвыражения.
 */
bool checkAEBList(AEBList* AEBs, AEB* expr, BlockStatement& block);
/**
 * @brief Проверяет, изменяет ли данная операция значение какой-либо переменной в вычисляемых подвыражениях.
 * Если операция изменяет переменную в подвыражении, то удаляет все такие подвыражения из списка.
 * @param AEBs Список подвыражений.
 * @param stmt Проверяемая операция.
 */
void checkVarsChanging(AEBList* AEBs, StatementBase& stmt);


/**
 * Вставляет операцию в блок по номеру операции.
 */
void blockAddBeforePosition(BlockStatement& block, int position, StatementBase* stmt);
/**
 * Перенумеровывет операции в списке вычисляемых операций. Используется при добавлении временной переменной в блок.
 */
void renumberAEBs(AEBList* AEBs, AEBList::iterator* from);
/**
 * Проверяет, присутствует ли переменная в выражении.
 */
bool isVarInExpression(ReferenceExpression* var, ExpressionBase* expr);

} // Scalar
} // OPS
} // Transforms

#endif	// _CSE_H_INCLUDED_
