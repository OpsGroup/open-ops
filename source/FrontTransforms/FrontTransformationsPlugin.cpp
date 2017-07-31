#include "FrontTransformationsPlugin.h"
#include "FrontTransforms/ResolverPass.h"
#include "FrontTransforms/C2RPass.h"

#include <OPS_Core/Plugins/PluginRegistration.h>

EXPORT_STATIC_PLUGIN2(FrontTransformationsPlugin,
	OPS::FrontTransformations::FrontTransformationsPlugin)

namespace OPS
{
	namespace FrontTransformations
	{
		FrontTransformationsPlugin::FrontTransformationsPlugin()
		{
		}

		void FrontTransformationsPlugin::initialize()
		{
            using namespace OPS::Stage;

            RegisterAnalysisPass<NoCanto,C2RPass>
                    c2rPass("Canto To Reprise");

            RegisterAnalysisPass<RepriseCanonical,RepriseCanonizationPass>
                    repriseCanonization("Reprise Canonization");

            RegisterAnalysisPass<NamesResolved, ResolverPass>
                    resolver("Name Resolver");

		}

		void FrontTransformationsPlugin::terminate()
		{
		}
	}
}
