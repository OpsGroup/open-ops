/*слияние циклов*/
/*Вопросы: Олег 8-903-462-33-24*/
/*olegsteinb@gmail.com*/

#ifndef _FUSION
#define _FUSION

#ifdef _MSC_VER
#pragma warning(disable : 4008)	// Запрещаем сообщение об обрубании имен в debug-версии
#endif

#include "Reprise/Reprise.h" 

namespace OPS
{
namespace Transforms
{
namespace Loops
{
/*Ф-ия слияния двух следующих друг за другом циклов*/
/*Пытается слить два цикла
 (начальные параметры отвечают за использование вспомогательных
  преобразований "растягивание скаляров" и "введение временных массивов")*/
bool LoopFusion(OPS::Reprise::ForStatement* loop1, OPS::Reprise::ForStatement* loop2, bool VarToArray = false, bool TempArrays = false);
}	// Loops
}	// Transforms
}	// OPS
#endif  //_FUSION
