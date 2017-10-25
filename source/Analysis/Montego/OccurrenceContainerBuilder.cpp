#include "OccurrenceContainerBuilder.h"
#include "Reprise/Reprise.h"
#include "Analysis/Montego/Occurrence.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include<fstream>

namespace OPS
{
namespace Montego
{

OccurrenceContainerBuilder::OccurrenceContainerBuilder()
{}

OccurrenceContainerBuilder::~OccurrenceContainerBuilder()
{}

OccurrenceContainerBuilder::OccurByReferenceMap OccurrenceContainerBuilder::getOccurByReferenceMap()
{
    return m_occurByReference;
}

//добавляет элементарные вхождения из program в occurMap
//визитор обходит программу как DeepWalker т.е. не заходит внутрь вызываемых функций!
void OccurrenceContainerBuilder::addAllBasicOccurrencesIn(Reprise::RepriseBase& program, OccurByReferenceMap& occurMap, bool replaceOccurrences)
{
    m_occurByReference.clear();
    program.accept(*this);
    OccurrenceContainerBuilder::OccurByReferenceMap::iterator it = m_occurByReference.begin();
    for ( ; it != m_occurByReference.end(); ++it )
    {
        if (replaceOccurrences)
            occurMap[it->first] = it->second;
        else//заменять нельзя
        {
            if (occurMap.find(it->first) == occurMap.end())
                occurMap[it->first] = it->second; //если не нашли, то добавляем
        }
    }
}

//Для куска кода строит map<вызов функции, ее возможные тела>
//и еще строит множество всех участвующих тел (чтобы потом исключить повторные построения множеств 
//вхождений для одних и тех же тел)
class SCallMapBuilder : public OPS::Reprise::Service::DeepWalker
{
public:
    SCallMapBuilder(AliasInterface& ai):m_ai(&ai){}
    ~SCallMapBuilder(){}

    std::set<Reprise::BlockStatement*> m_allBodies;
    std::map<Reprise::SubroutineCallExpression*,std::set<Reprise::BlockStatement*> > m_bodiesBySCalls;
    AliasInterface* m_ai;

    void visit(OPS::Reprise::SubroutineCallExpression& e)
    {
        std::list<Reprise::SubroutineDeclaration*> decls = m_ai->getAllPossibleSubroutinesByPointer(e);
        std::list<Reprise::SubroutineDeclaration*>::iterator it;
        if (decls.size() == 0) 
            //обращаемся к элементу, чтобы создать пустое множество
            m_bodiesBySCalls[&e];
        for (it = decls.begin(); it != decls.end(); ++it)
        {
            if ((*it)->hasDefinition())   
            {
                Reprise::SubroutineDeclaration* sdecl = &(*it)->getDefinition();
                Reprise::BlockStatement* body = &sdecl->getBodyBlock();
                m_allBodies.insert(body);
                m_bodiesBySCalls[&e].insert(body);
            }
            else
                //обращаемся к элементу, чтобы создать пустое множество, если множества еще нет
                m_bodiesBySCalls[&e];
        }
        DeepWalker::visit(e);
    }
};


//добавляет составные вхождения из program в compOccurMap
//визитор обходит программу как DeepWalker т.е. не заходит внутрь вызываемых функций!
//Так как функции могут вызываться по указателю, то для построения составных вхождений 
//требуется информация об альясах
//и построенные элементарные вхождения в контейнере
void OccurrenceContainerBuilder::addAllComplexOccurrencesIn(OPS::Reprise::RepriseBase& program, OccurBySCallMap& compOccurMap, OccurrenceContainer& ocont, AliasInterface& ai)
{
    SCallMapBuilder sCallMapBuilder(ai);
    program.accept(sCallMapBuilder);
    std::map<Reprise::SubroutineCallExpression*, std::set<Reprise::BlockStatement*> >::iterator it;
    std::map<Reprise::BlockStatement*, std::vector<BasicOccurrencePtr> > occursForBodies;
    std::set<Reprise::BlockStatement*> allBodies = sCallMapBuilder.m_allBodies;
    std::set<Reprise::BlockStatement*>::iterator itb;

    //находим множества вхождений для всех тел (чтобы потом это использовать и повторно не находить)
    for (itb = sCallMapBuilder.m_allBodies.begin(); itb != sCallMapBuilder.m_allBodies.end(); ++itb)
    {
        occursForBodies[*itb] = ocont.getDeepAllBasicOccurrencesIn(*itb,&ai);
    }
    
    //теперь заполняем составные вхождения
    for (it = sCallMapBuilder.m_bodiesBySCalls.begin(); it != sCallMapBuilder.m_bodiesBySCalls.end(); ++it)
    {
        //собираем все вхождения внутри тел вызываемой функции
        std::set<Reprise::BlockStatement*> bodies = it->second;
        std::vector<BasicOccurrencePtr> occurs;
        for (itb = bodies.begin(); itb != bodies.end(); ++itb)
        {
            occurs.insert(occurs.end(), occursForBodies[*itb].begin(), occursForBodies[*itb].end());
        }
        ComplexOccurrencePtr co(new ComplexOccurrence(*(it->first), occurs));
        compOccurMap.insert( std::pair<OPS::Reprise::SubroutineCallExpression*, ComplexOccurrencePtr>(it->first,co) );
    }
}

void OccurrenceContainerBuilder::visit(OPS::Reprise::ReferenceExpression& e)
{
    BasicOccurrencePtr occur(new BasicOccurrence(e));
#if OPS_BUILD_DEBUG
        if (m_occurByReference.find(&e) == m_occurByReference.end())
            m_occurByReference[&e] = occur;
        else //такое вхождение уже сущестсвует. 
            //Вышеуказанная операция: m_occurByReference[&e] = occur; ошибочной не будет.
            //Но повторное внесение вхождений затирает информацию у старых (коэффициенты и инф. о циклах)
            //поэтому скорей всего у того, кто повторно вызывает visit контейнера вхождений
            //где-то в программе ошибка.
            //Если нужно перестроить информацию о вхождениях вызывайте функцию addOccurrencesIn
            //или создавайте новый контейнер (удалив старый).
            throw OPS::RuntimeError("OccurrenceContainerBuilder::visit(ReferenceExpression&): Вхождения перезаписываются в контейнер! У вас где-то ошибка (забыли отчистить?)!");
#else
        m_occurByReference[&e] = occur;
#endif
}

void OccurrenceContainerBuilder::visit(OPS::Reprise::ForStatement& forstmt)
{
    OPS::Reprise::Service::DeepWalker::visit(forstmt);
}

}//end of namespace
}//end of namespace

