#include "TransformationsPlugin.h"
#include "Transforms/TransformsPasses.h"

#include <OPS_Core/Plugins/PluginRegistration.h>

EXPORT_STATIC_PLUGIN2(TransformationsPlugin, OPS::Transforms::TransformationsPlugin)

namespace OPS
{
	namespace Transforms
	{
		TransformationsPlugin::TransformationsPlugin()
		{
		}

		void TransformationsPlugin::initialize()
		{
			using namespace OPS::Stage;

            RegisterAnalysisPass<LoopDistributionDone,LoopDistributionPass>
                    loopDistributionPass("Loop distribution");
		}

		void TransformationsPlugin::terminate()
		{
		}
	}
}
