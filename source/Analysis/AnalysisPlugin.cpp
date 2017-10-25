#include "AnalysisPlugin.h"

#include "ConsistCheck/ConsistCheckService.h"
#include "Renewal/AliasAnalysis/AliasAnalysisService.h"
#include "Renewal/OccurrenceAnalysis/OccurrenceAnalysisService.h"
#include "Analysis/Montego/AliasAnalysis/CanonicalForm.h"
#include "Analysis/ParallelLoops.h"

#include <OPS_Core/Plugins/PluginRegistration.h>
#include <OPS_Core/ServiceLocator.h>

EXPORT_STATIC_PLUGIN2(AnalysisPlugin, OPS::Analysis::AnalysisPlugin)

using namespace OPS::Core;
using namespace OPS::Renewal;

namespace OPS
{
	namespace Analysis
	{
		AnalysisPlugin::AnalysisPlugin()
		{
		}

		void AnalysisPlugin::initialize()
		{
			ServiceLocator::instance().addService<IAliasAnalysisService>(
				new AliasAnalysisService());

			ServiceLocator::instance().addService<IOccurrenceAnalysisService>(
				new OccurrenceAnalysisService());

			ServiceLocator::instance().addService<IConsistCheckService>(
				new ConsistCheckService());

            using namespace OPS::Stage;

            RegisterAnalysisPass<AliasCanonicalForm, CanonicalFormPass>
                    canonicalForm("Canonical Form");

            RegisterAnalysisPass<OPS::Analysis::ParallelLoops::LoopToMarksMap,FindParallelLoopsPass>
                    findParallelLoops("Find Parallel Loops");
		}

		void AnalysisPlugin::terminate()
		{
			ServiceLocator::instance().deleteService<IConsistCheckService>();

			ServiceLocator::instance().deleteService<IOccurrenceAnalysisService>();

			ServiceLocator::instance().deleteService<IAliasAnalysisService>();
		}
	}
}
