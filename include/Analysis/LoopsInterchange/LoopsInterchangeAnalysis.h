#ifndef LOOPS_INTERCHANGE_ANALYSIS_H_INCLUDED
#define LOOPS_INTERCHANGE_ANALYSIS_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

using OPS::Reprise::ForStatement;

namespace OPS
{
namespace Analysis
{
namespace LoopsInterchange
{

//на вход подается внешний цикл тесного гнезда
//функция строит депграф
bool isInterchangable(ForStatement& forStatement);

}	// LoopsInterchange
}	// Analysis
}	// OPS

#endif
