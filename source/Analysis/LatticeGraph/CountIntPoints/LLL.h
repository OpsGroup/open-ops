#pragma once 

#include "Analysis/LatticeGraph/RationalNumber.h"

namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

//a[i] - векторы, n - размерность и количество векторов, delta - параметр LLL-алгоритма
void LLL(RationalNumber* a, int n, double delta);

}//end of namespace
}//end of namespace
}//end of namespace
