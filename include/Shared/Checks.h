#ifndef _CHECK_H_INCLUDED_
#define _CHECK_H_INCLUDED_

#include "Shared/Checks/CompositionCheck/CompositionCheckObjects.h"
#include "Shared/Checks/CompositionCheck/CompositionCheckWalker.h"

namespace OPS
{
namespace Shared
{
namespace Checks
{

// ---------- Composition Check
//
// Шаблонная функция compositionCheck позволяет ответить на следующий вопрос:
// "Правда ли, что в указанном фрагменте программы содержатся только объекты из указанного набора?"
//
// Класс CompositionCheckObjects позволяет задать список допустимых объектов для проверяемого фрагмента программы.
// На данный момент поддерживаемые объекты - это операторы внутреннего представления (потомки StatementBase,
// обработку которых допускает DeepWalker) и метки.
    
template<class RepriseStatementContainer>
bool makeCompositionCheck(RepriseStatementContainer& container, CompositionCheckObjects acceptableObjects)
{
	CompositionCheckWalker compositionCheckWalker(acceptableObjects);
	container.accept(compositionCheckWalker);
	return compositionCheckWalker.compositionCheckResult();
}

}	// OPS
}	// Shared
}	// Checks

#endif	// _CHECK_H_INCLUDED_
