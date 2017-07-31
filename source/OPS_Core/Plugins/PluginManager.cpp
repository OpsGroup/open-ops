#include "OPS_Core/Plugins/PluginManager.h"

#include "PluginRegistry.h"

#include "OPS_Core/Plugins/IPlugin.h"

namespace OPS
{
	namespace Core
	{
		void PluginManager::setPluginLoadOrder(
			const std::list<std::string>& loadOrderedPluginNames)
		{
			m_loadOrderedPluginNames = loadOrderedPluginNames;
		}

		void PluginManager::initializePlugins() const
		{
			typedef std::list<std::string>::const_iterator StringListConstIter;

			for (StringListConstIter it = m_loadOrderedPluginNames.begin();
				it != m_loadOrderedPluginNames.end(); ++it)
			{
				PluginRegistry::instance().pluginByName(*it).initialize();
			}
		}

		void PluginManager::terminatePlugins() const
		{
			typedef std::list<std::string>::const_reverse_iterator StringListConstRevIter;

			for (StringListConstRevIter it = m_loadOrderedPluginNames.rbegin();
				it != m_loadOrderedPluginNames.rend(); ++it)
			{
				PluginRegistry::instance().pluginByName(*it).terminate();
			}
		}
	}
}
