#ifndef _PLUGIN_MANAGER_H_INCLUDED_
#define _PLUGIN_MANAGER_H_INCLUDED_

#include <list>
#include <string>

namespace OPS
{
	namespace Core
	{
		class PluginManager
		{
		public:

			inline PluginManager() {}

			void setPluginLoadOrder(const std::list<std::string>& loadOrderedPluginNames);

			void initializePlugins() const;
			void terminatePlugins() const;

		private:

			PluginManager(const PluginManager& );
			PluginManager& operator =(const PluginManager& );

		private:

			std::list<std::string> m_loadOrderedPluginNames;
		};
	}
}

#endif // _PLUGIN_MANAGER_H_INCLUDED_
