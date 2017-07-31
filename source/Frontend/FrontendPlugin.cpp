#include "FrontendPlugin.h"

#include <OPS_Core/Plugins/PluginRegistration.h>

EXPORT_STATIC_PLUGIN2(FrontendPlugin, OPS::Frontend::FrontendPlugin)

namespace OPS
{
	namespace Frontend
	{
		FrontendPlugin::FrontendPlugin()
		{
		}

		void FrontendPlugin::initialize()
		{
		}

		void FrontendPlugin::terminate()
		{
		}
	}
}
