#pragma once

#include <list>
#include <OPS_Core/Visitor.h>
#include <Reprise/Common.h>

namespace OPS
{
namespace Analysis
{
namespace Statistics
{

class StatisticsAnalyserBase : public BaseVisitor
{
public:
    virtual void printStatistics(std::ostream& os) const = 0;
};

StatisticsAnalyserBase* createNodeCountAnalyser();
StatisticsAnalyserBase* createVariableTypesAnalyser();
StatisticsAnalyserBase* createForLoopHeaderAnalyser();
StatisticsAnalyserBase* createForStructureAnalyser();
StatisticsAnalyserBase* createSubscriptAnalyser();
StatisticsAnalyserBase* createMemorySizeAnalyser();
StatisticsAnalyserBase* createStructAnalyser();

void makeStatistics(OPS::Reprise::RepriseBase& node,
                    const std::list<StatisticsAnalyserBase*>& analysers,
                    std::ostream& os);

}
}
}
