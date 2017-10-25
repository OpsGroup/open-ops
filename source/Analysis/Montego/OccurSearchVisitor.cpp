#include "OccurSearchVisitor.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"

namespace OPS
{
namespace Montego
{

OccurSearchVisitor::OccurSearchVisitor(OccurrenceContainer& occurContainer, bool flagEnterSubrouitineCalls, AliasInterface* ai)
: m_occurContainer(&occurContainer), m_flagEnterSubrouitineCalls(flagEnterSubrouitineCalls)
{
    m_aliasInterface = ai;
}

void OccurSearchVisitor::visit(OPS::Reprise::ReferenceExpression& e)
{
    m_refSet.insert(&e);
}

void OccurSearchVisitor::visit(OPS::Reprise::SubroutineCallExpression& e)
{
    //обходим имя процедуры и ее параметры
    DeepWalker::visit(e);

    if (m_scallSet.find(&e) == m_scallSet.end())
        m_scallSet.insert(&e);
    else return;//если уже заходили, то второй раз не заходим

    if (m_flagEnterSubrouitineCalls)
    {
		BasicOccurrence* occur = 0;
        //список возможных вызываемых процедур
        std::list<OPS::Reprise::SubroutineDeclaration*> sdeclarations;//их может быть много, если мы точно не узнаем
		if ( e.hasExplicitSubroutineDeclaration() )
        {
			sdeclarations.push_back(&e.getExplicitSubroutineDeclaration());
        }
        else
        {
            if ( m_aliasInterface == 0 )
                throw OPS::RuntimeError("OccurSearchVisitor::visit(SubroutineCallExpression): Не могу определить какая функция вызывается по заданному указателю, т.к. не проведен анализ альясов!");
            sdeclarations = m_aliasInterface->getAllPossibleSubroutinesByPointer(e);
            OPS_ASSERT(sdeclarations.size() != 0);
            if (sdeclarations.size() == 0) 
                throw OPS::RuntimeError("Can't find suitable subroutine declaration for occurrence: " + occur->toString());
        }
        //заходим внутрь каждой
        std::list<OPS::Reprise::SubroutineDeclaration*>::iterator it;
        for (it = sdeclarations.begin(); it != sdeclarations.end(); ++it)
        {
            if ((*it)->hasDefinition())
            {
                Reprise::SubroutineDeclaration* sdecl = &(*it)->getDefinition();
                sdecl->getBodyBlock().accept(*this);
            }
        }
    }
}

std::set<OPS::Reprise::ReferenceExpression*> OccurSearchVisitor::getRefSet()
{
    return m_refSet;
}

std::set<OPS::Reprise::SubroutineCallExpression*> OccurSearchVisitor::getSCallSet()
{
    return m_scallSet;
}


}//end of namespace
}//end of namespace

