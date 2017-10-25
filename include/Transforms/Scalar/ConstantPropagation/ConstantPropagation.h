#ifndef _CONSTANT_PROPAGATION_H_INCLUDED_
#define _CONSTANT_PROPAGATION_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

void makeConstantPropagation(OPS::Reprise::BlockStatement& blockStatement);

// Возвращает true если переменная является константой
bool isConstantVariable(OPS::Reprise::VariableDeclaration& variable);

// Функция протягивает константные переменные типа: const int X = 10
void propagateConstantVariable(OPS::Reprise::VariableDeclaration& variable);
void propagateConstantVariables(const std::set<OPS::Reprise::VariableDeclaration*>& variables);

// Протянуть все константные переменные объявленные во фрагменте
void propagateAllConstantVariables(OPS::Reprise::RepriseBase& fragment);

} // Scalar
} // OPS
} // Transforms

#endif	// _CONSTANT_PROPAGATION_H_INCLUDED_
