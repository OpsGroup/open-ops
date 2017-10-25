#ifndef OPS_DEAD_SUBROUTINE_ELIMINATION_H_INCLUDED__
#define OPS_DEAD_SUBROUTINE_ELIMINATION_H_INCLUDED__

#include <Reprise/Declarations.h>

namespace OPS
{
namespace Transforms
{
namespace Subroutines
{

/// Удаляет все функции, которые прямо или косвенно не вызываются из главной функции mainSubroutine
/// Возвращает количество удаленных функций
int removeDeadSubroutines(OPS::Reprise::SubroutineDeclaration& mainSubroutine);

/// Удаляет все неиспользуемые функции. Используемыми считаются только те, которые переданы в списке usedSubroutines
/// Возвращает количество удаленных функций
int removeDeadSubroutines(const std::set<OPS::Reprise::SubroutineDeclaration*>& usedSubroutines);

}
}
}

#endif // OPS_DEAD_SUBROUTINE_ELIMINATION_H_INCLUDED__
