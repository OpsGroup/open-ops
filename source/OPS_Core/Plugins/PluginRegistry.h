#ifndef _PLUGIN_REGISTRY_H_INCLUDED_
#define _PLUGIN_REGISTRY_H_INCLUDED_

#include <map>
#include <string>

namespace OPS
{
	namespace Core
	{
		class IPlugin;

		class PluginRegistry
		{
		public:

			typedef IPlugin& (*PluginInstanceFunction)();

		public:

			void registerPluginInstanceFunction(const std::string& pluginName,
				PluginInstanceFunction pluginInstanceFunction);

			IPlugin& pluginByName(const std::string& pluginName) const;

		public:

			static PluginRegistry& instance();

		private:

			inline PluginRegistry() {}

			inline ~PluginRegistry() {}

			PluginRegistry(const PluginRegistry& );
			PluginRegistry& operator =(const PluginRegistry& );

			bool isPluginRegistered(const std::string& pluginName) const;

		private:

			mutable std::map<std::string, PluginInstanceFunction> m_pluginInstanceFunctions;
		};
	}
}

#endif // _PLUGIN_REGISTRY_H_INCLUDED_
