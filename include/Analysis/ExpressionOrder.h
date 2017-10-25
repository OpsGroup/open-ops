#ifndef EXPRESSIONORDER_H
#define EXPRESSIONORDER_H

#include "Reprise/Expressions.h"

namespace OPS
{
namespace Analysis
{

// Возвращает > 0 если выражение first выполняется раньше second
//            = 0 если не определено
//            < 0 если выражение first выполняется позже second
// Выражения должны принадлежать одному оператору, иначе будет выброшено исключение
int getExpressionOrder(Reprise::ExpressionBase& first, Reprise::ExpressionBase& second);

}
}

#endif // EXPRESSIONORDER_H
