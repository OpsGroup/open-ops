#include "Analysis/Frames/ProfilerBridge.h"

#include "Analysis/Frames/FrameDataSpecification.h"
#include "Analysis/ComplexOccurrenceAnalysis/GrouppedOccurrences.h"
#include "Shared/ExpressionHelpers.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			using namespace OPS::Reprise;
            using namespace OPS::Profiling;

            ProfilerBridge::ProfilerBridge() :
                m_analizeOnlyDataSize(true)
            {
            }

            ProfilerBridge::ProfilerBridge(std::tr1::shared_ptr<Profiler> spProfiler) :
                m_spProfiler(spProfiler), m_analizeOnlyDataSize(false)
			{
            }

            long_long_t ProfilerBridge::GetSizeCommand(StatementBase& statement, bool isOnlyForHeaders)
            {
                // For now all headers are little
                // todo: support headers
                if(isOnlyForHeaders)
                {
                    return 0;
                }

                if(m_analizeOnlyDataSize)
                {
                    return 0;
                }
                return m_spProfiler->GetSizeCommand(&statement, false);
            }

            long_long_t ProfilerBridge::GetSizeData(StatementBase& statement, bool isOnlyForHeaders)
            {
                // For now all headers are little
                // todo: support headers
                if(isOnlyForHeaders)
                {
                    return 0;
                }

                DataSpecificationAnalysis dsa;

                OccurrencesByDeclarations readOccurrences = findAllTopLevelOccurrences(statement, OPS::Analysis::GBT_READ);
                dsa.specifyDataSubsets(readOccurrences, statement);

                OccurrencesByDeclarations writeOccurrences = findAllTopLevelOccurrences(statement, OPS::Analysis::GBT_WRITE);
                dsa.specifyDataSubsets(writeOccurrences, statement);

                return dsa.calculateDataAmount();
            }

            long_long_t ProfilerBridge::GetSizeData(ForStatement& forStatement, const std::map<const ForStatement*, int>& nestIterationsCount)
            {
                OPS::Shared::ExpressionHelpers::IntegerHelper ih(BasicType::BT_INT32);
                std::map<const Reprise::ForStatement*, DataSpecificationAnalysis::Range> lengths;
                for(std::map<const ForStatement*, int>::const_iterator it = nestIterationsCount.begin(); it != nestIterationsCount.end(); ++it)
                {
                    ReprisePtr<ExpressionBase> rpFrom(&(ih(0)));
                    ReprisePtr<ExpressionBase> rpTo(&(ih(it->second)));
                    DataSpecificationAnalysis::Range range(1);
                    std::pair<Reprise::ReprisePtr<Reprise::ExpressionBase>, Reprise::ReprisePtr<Reprise::ExpressionBase> > pair(rpFrom, rpTo);
                    range[0] = pair;
                    lengths[it->first] = range;
                }

                DataSpecificationAnalysis dsa;

                OccurrencesByDeclarations readOccurrences = findAllTopLevelOccurrences(forStatement, OPS::Analysis::GBT_READ);
                dsa.specifyDataSubsets(readOccurrences, forStatement, false, &lengths);

                OccurrencesByDeclarations writeOccurrences = findAllTopLevelOccurrences(forStatement, OPS::Analysis::GBT_WRITE);
                dsa.specifyDataSubsets(writeOccurrences, forStatement, false, &lengths);

                return dsa.calculateDataAmount();
            }
		}
	}
}
