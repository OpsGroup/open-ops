#include "BackendsPlugin.h"

#include <OPS_Core/Plugins/PluginRegistration.h>

EXPORT_STATIC_PLUGIN2(BackendsPlugin, OPS::Backends::BackendsPlugin)

namespace OPS
{
	namespace Backends
	{
		BackendsPlugin::BackendsPlugin()
		{
		}

		void BackendsPlugin::initialize()
		{
		}

		void BackendsPlugin::terminate()
		{
		}
	}
}
