#include "PluginRegistry.h"

#include "OPS_Core/Helpers.h"
#include "OPS_Core/Plugins/IPlugin.h"

namespace OPS
{
	namespace Core
	{
		PluginRegistry& PluginRegistry::instance()
		{
			static PluginRegistry s_instance;

			return s_instance;
		}

		void PluginRegistry::registerPluginInstanceFunction(const std::string& pluginName,
			PluginInstanceFunction pluginInstanceFunction)
		{
			OPS_ASSERT(!isPluginRegistered(pluginName));

			m_pluginInstanceFunctions[pluginName] = pluginInstanceFunction;
		}

		IPlugin& PluginRegistry::pluginByName(const std::string& pluginName) const
		{
			OPS_ASSERT(isPluginRegistered(pluginName));

			return m_pluginInstanceFunctions[pluginName]();
		}

		bool PluginRegistry::isPluginRegistered(const std::string& pluginName) const
		{
			return m_pluginInstanceFunctions.find(pluginName) !=
				m_pluginInstanceFunctions.end();
		}
	}
}

void registerStaticPluginInstanceFunction(const std::string& pluginName,
	OPS::Core::PluginRegistry::PluginInstanceFunction pluginInstanceFunction)
{
	OPS::Core::PluginRegistry::instance().registerPluginInstanceFunction(pluginName,
		pluginInstanceFunction);
}
