#ifndef _OPS_TRANSFORMS_SWITCH_TO_IF
#define _OPS_TRANSFORMS_SWITCH_TO_IF

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Scalar
{

/**
* Преобразование оператора switch в эквивалентную последовательность if
*
* \param pSwitch - оператор switch. После преобразования вместо него в 
*                  содержащем его блоке будет цепочка операторов if, 
*                  исходный оператор будет удален
*
* \return void
*********************************************************************/

void makeIfFromSwitch(OPS::Reprise::SwitchStatement* pSwitch);

/**
* Преобразование оператора switch языка С в эквивалентную последовательность if
*
* \param pSwitch - оператор switch. После преобразования вместо него в 
*                  содержащем его блоке будет последовательность условных
*                  переходов на метки внутри блока, бывшего телом оператора switch
*
* \return void
*********************************************************************/

void makeIfFromSwitch(OPS::Reprise::PlainSwitchStatement* pSwitch);


}

}

}

#endif //_OPS_TRANSFORMS_SWITCH_TO_IF
