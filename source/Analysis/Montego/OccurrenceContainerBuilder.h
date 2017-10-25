#pragma once
#include "Reprise/Service/DeepWalker.h"
#include "Analysis/Montego/Occurrence.h"

//здесь описан класс для поиска и построения вхождений в фрагменте программы
//этот класс должен использоваться только в конструкторе OccurrenceContainer

namespace OPS
{
namespace Montego
{

class AliasInterface;

class OccurrenceContainerBuilder : public OPS::Reprise::Service::DeepWalker
{
public:
    //элементарные вхождения
	typedef std::map <const OPS::Reprise::ReferenceExpression*, BasicOccurrencePtr> OccurByReferenceMap;
    //составные вхождения
	typedef std::map <const OPS::Reprise::SubroutineCallExpression*, ComplexOccurrencePtr> OccurBySCallMap;

    OccurrenceContainerBuilder();
    ~OccurrenceContainerBuilder();

    OccurByReferenceMap getOccurByReferenceMap();

    //добавляет элементарные вхождения из program в occurMap
    //визитор обходит программу как DeepWalker т.е. не заходит внутрь вызываемых функций!
    void addAllBasicOccurrencesIn(OPS::Reprise::RepriseBase& program, OccurByReferenceMap& occurMap, bool replaceOccurrences = true);

    //добавляет составные вхождения из program в compOccurMap
    //визитор обходит программу как DeepWalker т.е. не заходит внутрь вызываемых функций!
    //Так как функции могут вызываться по указателю, то для построения составных вхождений 
    //требуется информация об альясах
    //и построенные элементарные вхождения в контейнере
    void addAllComplexOccurrencesIn(OPS::Reprise::RepriseBase& program, OccurBySCallMap& compOccurMap, OccurrenceContainer& ocont, AliasInterface& ai);

    void visit(OPS::Reprise::ReferenceExpression& e);

    void visit(OPS::Reprise::ForStatement& e);

private:
    OccurByReferenceMap m_occurByReference;
};


}//end of namespace
}//end of namespace

