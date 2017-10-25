/*СЂР°Р·СЂРµР·Р°РЅРёРµ С†РёРєР»РѕРІ*/
/*Р’РѕРїСЂРѕСЃС‹: РћР»РµРі 8-903-462-33-24*/
/*olegsteinb@gmail.com*/

#ifndef _DISTRIBUTE
#define _DISTRIBUTE

#ifdef _MSC_VER
#pragma warning(disable : 4008)	// пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ пїЅ debug-пїЅпїЅпїЅпїЅпїЅпїЅ
#endif

#include "Reprise/Reprise.h" 

using namespace std;

namespace OPS
{
namespace Transforms
{
namespace Loops
{
/*пїЅ-пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ pFor* пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ*/
/*пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ
 (пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
  пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ "пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ" пїЅ "пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ")*/
bool LoopDistribution(OPS::Reprise::ForStatement* pFor, bool VarToArray = false, bool TempArrays = false);
bool LoopNestingAndDistribution(OPS::Reprise::ForStatement* pFor, bool Var2Array = false, bool TempArrays = false);
////bool LoopDistribution(OPS::Reprise::ForStatement* pFor, OPS::Reprise::StatementBase* St, bool VarToArray = false, bool TempArrays = false);
//void getNumStatementInBlock(OPS::Reprise::BlockStatement *InBlock, int N, map<OPS::Reprise::StatementBase*, int> &St_and_countSt);
}	// Loops
}	// Transforms
}	// OPS
#endif  //_DISTRIBUTE
