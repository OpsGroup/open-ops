#pragma once
/*
здесь описан клас DinMemAllotmentSearchVisitor - визитор для поиска функций
динамического выделения памяти в программе
*/

#include <list>
#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
namespace Montego
{

class DynamicMemoryAllotmentSearchVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    DynamicMemoryAllotmentSearchVisitor();

    //выполняет accept
    DynamicMemoryAllotmentSearchVisitor(OPS::Reprise::SubroutineDeclaration& e);

    void visit(OPS::Reprise::SubroutineCallExpression& e);

    std::list<OPS::Reprise::SubroutineCallExpression*> getDinMemAllotmentList();

    // есть ли данная строка в массиве m_funcNames
    static bool isMemAllocFuncName(std::string s);

private:
    std::list<OPS::Reprise::SubroutineCallExpression*> m_dinMemAllotmentList;
};


}//end of namespace
}//end of namespace
