#include "DynamicMemoryAllotmentSearchVisitor.h"

namespace OPS
{
namespace Montego
{

const char *(dinMemAllotmentFuncNames[]) =
    {"calloc", "malloc", "ALLOCATE", "aligned_alloc", "_aligned_malloc",
        "valloc", "memalign", "pvalloc", "posix_memalign", "polybench_alloc_data"};

DynamicMemoryAllotmentSearchVisitor::DynamicMemoryAllotmentSearchVisitor()
{

}

//выполняет accept
DynamicMemoryAllotmentSearchVisitor::DynamicMemoryAllotmentSearchVisitor(OPS::Reprise::SubroutineDeclaration& e)
{
    e.accept(*this);
}

void DynamicMemoryAllotmentSearchVisitor::visit(OPS::Reprise::SubroutineCallExpression& e)
{
    if (e.hasExplicitSubroutineDeclaration())
    {
        OPS::Reprise::SubroutineDeclaration& sdecl = e.getExplicitSubroutineDeclaration();
        if (isMemAllocFuncName(sdecl.getName()))
        {
            m_dinMemAllotmentList.push_back(&e);
        }
    }
    OPS::Reprise::Service::DeepWalker::visit(e);//он рекурсивно вызывает методы потомка!!!
}

std::list<OPS::Reprise::SubroutineCallExpression*> DynamicMemoryAllotmentSearchVisitor::getDinMemAllotmentList()
{
    return m_dinMemAllotmentList;
}

// есть ли данная строка в массиве m_funcNames
bool DynamicMemoryAllotmentSearchVisitor::isMemAllocFuncName(std::string s)
{
    bool res = false;
	int n = sizeof(dinMemAllotmentFuncNames)/sizeof(dinMemAllotmentFuncNames[0]);
    for (int i = 0; i < n; ++i)
    {
        if (s == dinMemAllotmentFuncNames[i])
        {
            res = true;
            break;
        }
    }
    return res;
}

}//end of namespace
} // end namespace
