/*Разрезание циклов*/
/*Вопросы: Олег 8-903-462-33-24*/
/*olegsteinb@gmail.com*/

#ifndef _RECURR
#define _RECURR

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{
	bool MakeRLT(OPS::Reprise::ForStatement* pFor, int number_of_proc = 2, const double Tf = 5, const double Ts = 7);
}	// Loops
}	// Transforms
}	// OPS

#endif // _RECURR
