#ifndef _DEPENDENCE_GRAPH_ARC_H_INCLUDED_
#define _DEPENDENCE_GRAPH_ARC_H_INCLUDED_

#include "Analysis/Montego/DependenceGraph/DependenceGraphVertex.h"
#include "Analysis/Montego/DependenceGraph/DependenceLevel.h"
#include "Analysis/Montego/Occurrence.h"

#include <OPS_Core/Mixins.h>
#include <Reprise/Reprise.h>

#include <list>

namespace OPS
{
namespace Montego
{

class DependenceGraphAbstractArc
    : public TypeConvertibleMix
{
public:
    enum DependenceType
    {
        // out - generator
        // in - usage

        // same statement:
        DT_TRIVIAL_DEPENDENCE = 0,  // in-out

        // different statements:
        DT_TRUE_DEPENDENCE,         // out-in
        DT_ANTIDEPENDENCE,          // in-out
        DT_OUTPUT_DEPENDENCE,       // out-out
        DT_ININ_DEPENDENCE,         // in-in
    };

public:
    virtual ~DependenceGraphAbstractArc()
    {
    }

public:
    DependenceGraphVertex getStartVertex() const;
    DependenceGraphVertex getEndVertex() const;
    DependenceType getDependenceType() const;

protected:
    DependenceGraphAbstractArc()
    {
    }

    DependenceGraphAbstractArc(const DependenceGraphAbstractArc& );

protected:
    DependenceGraphVertex m_startVertex;
    DependenceGraphVertex m_endVertex;
    DependenceType m_dependenceType;
};



class DependenceGraphTrueArc
    : public DependenceGraphAbstractArc
{
public:
    DependenceGraphTrueArc(const OccurrencePtr& startOccurence,
        const OccurrencePtr& endOccurence, DependenceType dependenceType,
        const Reprise::StatementBase& sourceStatement);

public:
    const std::list<DependenceLevel>& getDependenceLevels() const;
    void removeDependenceLevel(const DependenceLevel& dependenceLevel);

protected:
    DependenceGraphTrueArc();

private:
    std::list<DependenceLevel> m_dependenceLevels;
};



class DependenceGraphMappedArc
    : public DependenceGraphAbstractArc
{
public:
    DependenceGraphMappedArc(const std::tr1::shared_ptr<DependenceGraphTrueArc>& trueArc,
        const Reprise::BlockStatement& blockForMapping);

public:
    std::tr1::shared_ptr<DependenceGraphTrueArc> getTrueArc() const;

private:
    enum StatementPlace
    {
        SP_SAME_BLOCK = 0,
        SP_INTERNAL_BLOCK,
        SP_EXTERNAL_BLOCK,
    };

    StatementPlace checkStatementPlace(const Reprise::StatementBase& statement) const;
    Reprise::StatementBase* getParentStatementInBlock(
        const Reprise::StatementBase& statement) const;
    DependenceGraphVertex mapVertex(const DependenceGraphVertex& vertex) const;

private:
    std::tr1::shared_ptr<DependenceGraphTrueArc> m_trueArc;
    const Reprise::BlockStatement& m_blockForMapping;
};

}
}

#endif // _DEPENDENCE_GRAPH_ARC_H_INCLUDED_
