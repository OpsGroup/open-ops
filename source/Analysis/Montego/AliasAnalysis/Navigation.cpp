#include "Navigation.h"
#include <algorithm>
#include <iostream>
#include "Shared/StatementsShared.h"
#include "Reprise/Reprise.h"
#include <set>
#include <list>
#include <map>

using namespace OPS;
using namespace OPS::Reprise;
using namespace std;

namespace OPS
{
namespace Montego
{

//возвращает не определение функции, если удалось его найти
SubroutineDeclaration* getDefinedSubroutine(SubroutineDeclaration& sdecl0)
{
    SubroutineDeclaration* sdecl = &sdecl0;
    if (sdecl->hasDefinition()) sdecl = &sdecl->getDefinition();
    if (sdecl->hasImplementation()) return sdecl;
    return 0;
}

SubroutineDeclaration* getDefinedSubroutine(SubroutineCallExpression& e)
{
    if (e.hasExplicitSubroutineDeclaration())
        return getDefinedSubroutine(e.getExplicitSubroutineDeclaration());
    return 0;
}

class FindSubroutineCalls : public Service::DeepWalker
{
public:
    explicit FindSubroutineCalls(ProgramUnit& pu):m_currentSubroutine(0),m_stmtCounter(0) { pu.accept(*this); }

    void visit(SubroutineDeclaration &d)
    {
        if (getDefinedSubroutine(d) != &d) return;
        m_stmtCounter = 0;
        m_currentSubroutine = &d;
        m_allSubroutines.insert(&d);
        DeepWalker::visit(d);
        m_subroutineSize[&d] = m_stmtCounter;
    }

    void visit(ExpressionStatement& s) { DeepWalker::visit(s);  m_stmtCounter++; }
    void visit(IfStatement& s) { DeepWalker::visit(s);  m_stmtCounter++; }
    void visit(ForStatement& s) { DeepWalker::visit(s);  m_stmtCounter++; }
    void visit(WhileStatement& s) { DeepWalker::visit(s);  m_stmtCounter++; }
    void visit(SubroutineReferenceExpression& e)
    {
        if ( ! (e.getParent()->is_a<SubroutineCallExpression>() && (&e.getParent()->getChild(0) == &e)) )
            if (SubroutineDeclaration* sd = getDefinedSubroutine(e.getReference()))
                m_calledByPointerFuncs.insert(sd);
    }

    void visit(SubroutineCallExpression & e)
    {
        OPS_ASSERT(m_currentSubroutine!=0);
        DeepWalker::visit(e);
        SubroutineDeclaration* sdecl = getDefinedSubroutine(e);
        if (sdecl != 0)
        {
            m_subroutineCalls[m_currentSubroutine].insert(sdecl);
            m_invertedSubroutineCalls[sdecl].insert(m_currentSubroutine);
        }
        else
            if (!e.hasExplicitSubroutineDeclaration()) m_funcsWithPointerCall.insert(m_currentSubroutine);
    }
    SubroutineDeclaration* m_currentSubroutine;
    int m_stmtCounter;
    std::map<Reprise::SubroutineDeclaration*, std::set<Reprise::SubroutineDeclaration*> > m_subroutineCalls;
    std::map<Reprise::SubroutineDeclaration*, std::set<Reprise::SubroutineDeclaration*> > m_invertedSubroutineCalls;
    std::map<Reprise::SubroutineDeclaration*, int> m_subroutineSize;
    std::set<Reprise::SubroutineDeclaration*> m_funcsWithPointerCall;
    std::set<Reprise::SubroutineDeclaration*> m_calledByPointerFuncs;
    std::set<Reprise::SubroutineDeclaration*> m_allSubroutines;
};

//строит необходимые для анализатора процедур графы
Navigation::Navigation(Reprise::ProgramUnit &pu, AliasAnalysisOptions opt):m_options(opt)
{
    FindSubroutineCalls fsc(pu);
    m_subroutineCalls = fsc.m_subroutineCalls;
    m_invertedSubroutineCalls = fsc.m_invertedSubroutineCalls;
    m_subroutineSize = fsc.m_subroutineSize;
    m_funcsWithPointerCall = fsc.m_funcsWithPointerCall;
    m_calledByPointerFuncs = fsc.m_calledByPointerFuncs;
    m_notVisitedSubroutines = fsc.m_allSubroutines;
    if (m_options.debug)
    {
        std::cout << "Navigator found: " << m_subroutineCalls.size() << " subroutine calls,\n"
                  << fsc.m_allSubroutines.size() << " subroutines with body,\n"
                  << fsc.m_calledByPointerFuncs.size() << " called by pointer subroutines,\n"
                  << m_funcsWithPointerCall.size() << " subroutines with pointer call\n";
    }
}

set<SubroutineDeclaration*> intersect(set<SubroutineDeclaration*>& sa, set<SubroutineDeclaration*>& sb)
{
    //a - max
    set<SubroutineDeclaration*> *a = &sa, *b = &sb, res;
    if (sa.size() < sb.size()) {a = &sb; b = &sa;}
    set<SubroutineDeclaration*>::iterator it = b->begin();
    for ( ; it != b->end(); ++it)
        if (a->find(*it) != a->end()) res.insert(*it);
    return res;
}

//Проводит анализ множеств достижимости в непосещенной части графа вызовов
//и возвращает следующую процедуру, с которой нужно начать обход
//если вернет 0, значит все обошли
SubroutineDeclaration* Navigation::getNextSubroutine()
{
    if (m_options.debug)
        std::cout << "Navigation::getNextSubroutine() started. There are " << m_notVisitedSubroutines.size() << " not visited subroutines\n";
    if (m_notVisitedSubroutines.size() == 0) return 0;
    //находим множества достижимости каждой вершины графа вызовов (количества считать нельзя - рекурсия приведет к бесконечному росту)
    map<SubroutineDeclaration*, set<SubroutineDeclaration*> > atSets;
    //инициализируем непосредственно связанными вершинами
    set<SubroutineDeclaration*>::iterator it = m_notVisitedSubroutines.begin();
    for ( ; it != m_notVisitedSubroutines.end(); ++it)
        atSets[*it] = intersect(m_subroutineCalls[*it], m_notVisitedSubroutines);
    bool flagChanged = true;
    while (flagChanged)
    {
        flagChanged = false;
        for (it = m_notVisitedSubroutines.begin(); it != m_notVisitedSubroutines.end(); ++it)
        {
            set<SubroutineDeclaration*> nextSubr = intersect(m_subroutineCalls[*it], m_notVisitedSubroutines);
            int oldSize = atSets[*it].size();
            set<SubroutineDeclaration*>::iterator it2 = nextSubr.begin();
            for ( ; it2 != nextSubr.end(); ++it2)
            {
                set<SubroutineDeclaration*> nextSubr2 = atSets[*it2];
                atSets[*it].insert(nextSubr2.begin(), nextSubr2.end());
            }
            int newSize = atSets[*it].size();
            if (newSize > oldSize) flagChanged = true;
        }
    }
    //=================================================
    //находим вершину с максимальным множеством достижимости, желательно не вызываемую по указателю
    int maxSize = -1;
    SubroutineDeclaration* maxSubr = 0;
    bool foundFuncNotCallByPointer = false;
    for (it = m_notVisitedSubroutines.begin(); it != m_notVisitedSubroutines.end(); ++it)
    {
        if ((int)atSets[*it].size() >= maxSize)
        {
            if ((int)atSets[*it].size() == maxSize)
            {
                if (!foundFuncNotCallByPointer && (m_calledByPointerFuncs.find(*it)==m_calledByPointerFuncs.end()))
                {
                        foundFuncNotCallByPointer = true;
                        maxSize = atSets[*it].size();
                        maxSubr = *it;
                }
            }
            else
            {
                maxSize = atSets[*it].size();
                maxSubr = *it;
                if (!foundFuncNotCallByPointer && (m_calledByPointerFuncs.find(*it)==m_calledByPointerFuncs.end()))
                        foundFuncNotCallByPointer = true;
            }
        }
    }
    if (m_options.debug)
        std::cout << "Navigation::getNextSubroutine() found subroutine with max reach: " << maxSubr->getName() << "\n";
    return maxSubr;
}

// анализатор процедур сообщает навигатору, что он посетил данную процедуру
void Navigation::putNextSubroutine(Reprise::SubroutineDeclaration* sd)
{
    m_notVisitedSubroutines.erase(sd);
}

Navigation::~Navigation()
{
}

}//namespace Montego
}//namespace OPS
