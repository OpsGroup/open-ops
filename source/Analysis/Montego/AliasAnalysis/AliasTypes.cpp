#include "Analysis/Montego/AliasAnalysis/AliasTypes.h"
#include "Reprise/Expressions.h"
#include "FunctionContext.h"

namespace OPS
{
namespace Montego
{

AliasCheckerException::AliasCheckerException()
    :AliasAnalysisException(std::string())
    ,m_isError(false)
{
}

void AliasCheckerException::addNotVisitedExpr(const NotVisitedExpr& expr)
{
    appendMessage("Alias analyser didn't visit " + expr.first->dumpState() +
                  " inside the " + expr.second->getName() + " subroutine.\n");

    if (m_list.empty())
    {
        appendMessage("Make the following steps: \n"
           "\t1. Check program correctness: there should be no references to uninitialized pointers.\n"
           "\t2. There should be no dead code with that pointers.\n"
           "\t3. If your program is correct - send message to the developer\n");
    }
    m_list.push_back(expr);
    m_isError = true;
}

void AliasCheckerException::addNotVisitedSubr(const OPS::Reprise::SubroutineDeclaration* sd)
{
    m_isError = true;
    appendMessage("Alias analyser didn't visit subroutine " + sd->getName() + "\n");
}


bool AliasCheckerException::isError()
{
    return m_isError;
}

SAMCForOneContext::SAMCForOneContext(MemoryCellContainer& cont)
    :m_returnValue(cont)
    ,m_containerCells(cont)
    ,m_dependentCells(cont)
{
}

OccurrenceSAMC::OccurrenceSAMC(MemoryCellContainer& cont)
    :m_memoryCellContainer(&cont)
{
}

OccurrenceSAMC::OccurrenceSAMC()
{
}

OccurrenceSAMC::iterator OccurrenceSAMC::begin()
{
    return m_occurrenceSAMC.begin();
}

OccurrenceSAMC::iterator OccurrenceSAMC::end()
{
    return m_occurrenceSAMC.end();
}

void OccurrenceSAMC::push_back(PairForOccurrenceSAMC p)
{
    m_occurrenceSAMC.push_back(p);
}

size_t OccurrenceSAMC::size()
{
    return m_occurrenceSAMC.size();
}

PairForOccurrenceSAMC& OccurrenceSAMC::front()
{
    return m_occurrenceSAMC.front();
}

SAMCForOneContext OccurrenceSAMC::getUnion()
{
    SAMCForOneContext res(*m_memoryCellContainer);
    for(iterator it = m_occurrenceSAMC.begin(); it != m_occurrenceSAMC.end(); ++it)
    {
        SAMCForOneContext& s = it->second;
        res.m_containerCells.unionWith(s.m_containerCells);
        res.m_returnValue.unionWith(s.m_returnValue);
        res.m_dependentCells.unionWith(s.m_dependentCells);
    }
    return res;
}

std::string OccurrenceSAMC::toString()
{
    std::string res;
    if (m_occurrenceSAMC.size() == 0)
        res += "no contexts";
    else
    {
        for (iterator it2 = m_occurrenceSAMC.begin(); it2 != m_occurrenceSAMC.end(); ++it2)
        {
            if (it2 != m_occurrenceSAMC.begin()) res += "\n";
            if (it2->first)
                res += it2->first->toString() + "  SAMCForOneContext: ";
            else
                res += "GlobalContext  SAMCForOneContext: ";
            res += "returnValue = " + it2->second.m_returnValue.toString() + ";  ";
            res += "containerCells = " + it2->second.m_containerCells.toString() + ";  ";
            res += "dependentCells = " + it2->second.m_dependentCells.toString() + ";  ";
        }
    }
    return res;
}


SuboffsetsContent::SuboffsetsContent()
{

}

SuboffsetsContentIterator SuboffsetsContent::begin()
{
    return m_content.begin();
}

SuboffsetsContentIterator SuboffsetsContent::end()
{
    return m_content.end();
}

void SuboffsetsContent::clear()
{
    return m_content.clear();
}

void SuboffsetsContent::push_back(PairForSuboffsetsContent& p)
{
    m_content.push_back(p);
}

void SuboffsetsContent::unionWith(SuboffsetsContent& other)
{
    for(SuboffsetsContentIterator it = other.begin(); it != other.end(); ++it)
    {
        MemoryCellOffset o(0, it->first);
        //ищем такое же смещение внутри this
        bool found = false;
        SuboffsetsContentIterator it2;
        for (it2 = begin(); it2 != end(); ++it2)
        {
            MemoryCellOffset o2(0, it2->first);
            if (MemoryCellOffset::getIntersectType(&o, &o2) == 3) {found = true; break;}
        }
        if (found)
            it2->second.unionWith(it->second);
        else
            m_content.push_back(*it);
    }
}

std::string SuboffsetsContent::toString()
{
    std::string res = "SuboffsetsContent:\n";
    for(SuboffsetsContentIterator it = m_content.begin(); it != m_content.end(); ++it)
    {
        std::list<MemoryCellOffset::ElementaryOffset> off = it->first;
        std::list<MemoryCellOffset::ElementaryOffset>::iterator it2 = off.begin();
        SetAbstractMemoryCell& content = it->second;
        for (; it2 != off.end(); ++it2) res += it2->toString();
        res += " contains " + content.toString() + "\n";
    }
    return res;
}


AliasInformationContainer::AliasInformationContainer(MemoryCellContainer& memCont)
    :m_memoryCellContainer(&memCont)
{

}

OccurrenceSAMC& AliasInformationContainer::operator[](BasicOccurrence* o)
{
    std::map<BasicOccurrence*,OccurrenceSAMC>::iterator it = m_cont.find(o);
    if (it != m_cont.end()) return it->second;
    else
    {
        //создаем запись
        OccurrenceSAMC osamc(*m_memoryCellContainer);
        m_cont.insert(std::make_pair(o,osamc));
        return m_cont.find(o)->second;
    }
}

AliasInformationContainer::AliasInformationContainer()
{
}

std::string AliasInformationContainer::toString(bool onlyNonEmpty)
{
    std::string res;
    std::map<BasicOccurrence*, OccurrenceSAMC>::iterator it;
    for (it = m_cont.begin(); it != m_cont.end(); ++it)
    {
        std::string samcString = it->first->toString();
        if (onlyNonEmpty == false || !samcString.empty())
        {
             res += "Occurrence: " + it->first->toString() + "   SAMC: ";
             res += samcString;
             res += "\n";
        }
    }
    return res;
}

}
}//end of namespace
