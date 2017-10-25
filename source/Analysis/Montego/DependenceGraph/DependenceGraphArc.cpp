#include "Analysis/Montego/DependenceGraph/DependenceGraphArc.h"

using namespace OPS::Reprise;
using std::tr1::shared_ptr;

namespace OPS
{
namespace Montego
{

DependenceGraphVertex DependenceGraphAbstractArc::getStartVertex() const
{
    return m_startVertex;
}

DependenceGraphVertex DependenceGraphAbstractArc::getEndVertex() const
{
    return m_endVertex;
}

DependenceGraphAbstractArc::DependenceType
    DependenceGraphAbstractArc::getDependenceType() const
{
    return m_dependenceType;
}



DependenceGraphTrueArc::DependenceGraphTrueArc(const OccurrencePtr& startOccurence,
    const OccurrencePtr& endOccurence, DependenceType dependenceType,
    const Reprise::StatementBase& sourceStatement)
    : DependenceGraphAbstractArc()
{
    OPS_ASSERT(startOccurence.get() != 0);
    OPS_ASSERT(endOccurence.get() != 0);

    m_startVertex = DependenceGraphVertex(startOccurence);
    m_endVertex = DependenceGraphVertex(endOccurence);
    m_dependenceType = dependenceType;

    OPS_ASSERT(startOccurence->getParentStatement() != 0);
    OPS_ASSERT(endOccurence->getParentStatement() != 0);

    const std::list<DependenceLevel> startLevels = getAllDependenceLevelsBetween(
        sourceStatement, *startOccurence->getParentStatement());
    const std::list<DependenceLevel> endLevels = getAllDependenceLevelsBetween(
        sourceStatement, *endOccurence->getParentStatement());

    if (startLevels.size() < endLevels.size())
    {
        m_dependenceLevels = startLevels;
    }
    else
    {
        m_dependenceLevels = endLevels;
    }
}

const std::list<DependenceLevel>& DependenceGraphTrueArc::getDependenceLevels() const
{
    return m_dependenceLevels;
}

void DependenceGraphTrueArc::removeDependenceLevel(
    const DependenceLevel& dependenceLevel)
{
    m_dependenceLevels.remove(dependenceLevel);
}



DependenceGraphMappedArc::DependenceGraphMappedArc(
    const shared_ptr<DependenceGraphTrueArc>& trueArc,
    const BlockStatement& blockForMapping)
    : DependenceGraphAbstractArc()
    , m_trueArc(trueArc)
    , m_blockForMapping(blockForMapping)
{
    OPS_ASSERT(m_trueArc.get() != 0);

    m_startVertex = mapVertex(trueArc->getStartVertex());
    m_endVertex = mapVertex(trueArc->getEndVertex());
    m_dependenceType = trueArc->getDependenceType();
}

DependenceGraphMappedArc::StatementPlace DependenceGraphMappedArc::checkStatementPlace(
    const StatementBase& statement) const
{
    OPS_ASSERT(statement.hasParentBlock());

    if (&statement.getParentBlock() == &m_blockForMapping)
    {
        return SP_SAME_BLOCK;
    }

    BlockStatement& block = const_cast<BlockStatement&>(statement.getParentBlock());
    while (block.hasParentBlock())
    {
        block = block.getParentBlock();

        if (&block == &m_blockForMapping)
        {
            return SP_INTERNAL_BLOCK;
        }
    }

    return SP_EXTERNAL_BLOCK;
}

StatementBase* DependenceGraphMappedArc::getParentStatementInBlock(
    const StatementBase& statement) const
{
    RepriseBase* parent = statement.getParent();
    while (parent != 0)
    {
        if (parent->is_a<StatementBase>())
        {
            StatementBase& parentStatement = parent->cast_to<StatementBase>();

            if (parentStatement.hasParentBlock())
            {
                BlockStatement& block = parentStatement.getParentBlock();

                if (&block == &m_blockForMapping)
                {
                    return &parentStatement;
                }
            }
        }

        parent = parent->getParent();
    }

    OPS_ASSERT(!"Parent statement not found");

    return 0;
}

DependenceGraphVertex DependenceGraphMappedArc::mapVertex(
    const DependenceGraphVertex& vertex) const
{
    StatementBase* sourceStatement = vertex.getParentStatement();

    OPS_ASSERT(sourceStatement != 0);

    const StatementPlace statementPlace = checkStatementPlace(*sourceStatement);

    switch (statementPlace)
    {
    case SP_SAME_BLOCK:
    case SP_EXTERNAL_BLOCK:
        return vertex;
    case SP_INTERNAL_BLOCK:
        return DependenceGraphVertex(getParentStatementInBlock(*sourceStatement));
    default:
        OPS_ASSERT(!"Unexpected statement place type");
    }

    return DependenceGraphVertex();
}

shared_ptr<DependenceGraphTrueArc> DependenceGraphMappedArc::getTrueArc() const
{
    OPS_ASSERT(m_trueArc.get() != 0);

    return m_trueArc;
}

}
}
