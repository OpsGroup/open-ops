#include "Analysis/LatticeGraph/ParamPoint.h"

#include "OPS_Core/msc_leakcheck.h"

namespace OPS
{
namespace LatticeGraph 
{

ParamPoint::ParamPoint(int n):std::vector<SimpleLinearExpression*>(n)
{
}

}//end of namespace
}//end of namespace
