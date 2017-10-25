#pragma once
#include <vector>
#include "Analysis/LatticeGraph/LinearLib.h"

namespace OPS
{
namespace LatticeGraph 
{

/// Целая точка n-мерного пространства, аффинно зависящая от каких-то параметров
struct ParamPoint : public std::vector<SimpleLinearExpression*>
{
public: 
    
    ParamPoint(int n);


};
}//end of namespace
}//end of namespace

