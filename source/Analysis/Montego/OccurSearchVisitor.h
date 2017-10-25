#pragma once
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Reprise.h"

//OccurSearchVisitor находит список всех ReferenceExpression в данном участке кода
//заходит внутрь встреченных функций!
//если встречает вызов указателя на функцию, то для определения функции 
//пользуется информацией анализатора альясов


namespace OPS
{
namespace Montego
{
class AliasInterface;
class OccurrenceContainer;

class OccurSearchVisitor : public OPS::Reprise::Service::DeepWalker
{
public:
    //Если ai=0 flagEnterSubrouitineCalls=true и в коде есть вызов функции через указатель, то выскочит исключение
    OccurSearchVisitor(OccurrenceContainer& occurContainer, bool flagEnterSubrouitineCalls, AliasInterface* ai = 0);

    void visit(OPS::Reprise::ReferenceExpression& e);

    void visit(OPS::Reprise::SubroutineCallExpression& e);

    std::set<OPS::Reprise::ReferenceExpression*> getRefSet();
    std::set<OPS::Reprise::SubroutineCallExpression*> getSCallSet();

private:
    std::set<OPS::Reprise::ReferenceExpression*> m_refSet;
    std::set<OPS::Reprise::SubroutineCallExpression*> m_scallSet;
    OccurrenceContainer* m_occurContainer;
    AliasInterface* m_aliasInterface;
    bool m_flagEnterSubrouitineCalls;
};

}//end of namespace
}//end of namespace
