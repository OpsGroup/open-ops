#pragma once

#include "Reprise/Reprise.h"
#include "Analysis/Montego/DependenceGraph/DependenceGraph.h"

using OPS::Reprise::ForStatement;

namespace OPS
{
namespace Transforms
{
namespace Loops
{

//на вход подается внешний цикл тесного гнезда 
//функция строит депграф
bool canApplyLoopInterchangeTo(ForStatement& forStatement);

//на вход подается внешний цикл тесного гнезда
void makeLoopInterchange(ForStatement& forStatement);

}	// Loops
}	// Transforms
}	// OPS
