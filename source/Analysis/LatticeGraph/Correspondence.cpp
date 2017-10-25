#include "Analysis/LatticeGraph/ExtendedQuast.h"
#include "Analysis/LatticeGraph/Correspondence.h"
#include "Analysis/LatticeGraph/ParamPoint.h"

#include "OPS_Core/msc_leakcheck.h"

namespace OPS 
{
namespace LatticeGraph 
{
Correspondence::~Correspondence()
{
    OPS_ASSERT(!"Еще не реализована!");
}

Correspondence* Correspondence::clone()//копирование
{
    Correspondence* result = new Correspondence;
    result->m_directFunctions.resize(m_directFunctions.size());
    for (size_t i=0; i<m_directFunctions.size(); i++)
        result->m_directFunctions[i]=m_directFunctions[i]->clone();
    result->m_inverseFunctions.resize(m_inverseFunctions.size());
    for (size_t i=0; i<m_inverseFunctions.size(); i++)
        result->m_inverseFunctions[i]=m_inverseFunctions[i]->clone();
    return result;
}

//обращение
Correspondence* Correspondence::buildInverseCorrespondence()
{
    Correspondence* result = clone();
    FunctionVector temp = result->m_directFunctions;
    result->m_directFunctions = result->m_inverseFunctions;
    result->m_inverseFunctions = temp;
    return result;
}

//Вычисление образа 1 точки arg. Получающийся образ будет состоять из объединения списка точек и ККАМ
void Correspondence::evaluateDirectImage(ParamPoint arg, std::list<ParamPoint>& resultPoints, ExtendedQuast& resultPolyhedra)
{

}

//Вычисление прообраза 1 точки arg. Получающийся прообраз будет состоять из объединения списка точек и ККАМ
void Correspondence::evaluateInverseImage(ParamPoint arg, std::list<ParamPoint>& resultPoints, ExtendedQuast& resultPolyhedra)
{

}


}//end of namespace
}//end of namespace
