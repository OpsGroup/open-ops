#ifndef _PLUGIN_REGISTRATION_H_INCLUDED_
#define _PLUGIN_REGISTRATION_H_INCLUDED_

#include "OPS_Core/Plugins/IPlugin.h"

#include <string>

typedef OPS::Core::IPlugin& (*PluginInstanceFunction)();

void registerStaticPluginInstanceFunction(const std::string& pluginName,
	PluginInstanceFunction pluginInstanceFunction);

#define EXPORT_STATIC_PLUGIN2(pluginName, PluginClass) \
OPS::Core::IPlugin& pluginInstanceFunction##pluginName() \
{ \
    static PluginClass s_instance; \
    return s_instance; \
}

#define IMPORT_STATIC_PLUGIN(pluginName) \
OPS::Core::IPlugin& pluginInstanceFunction##pluginName(); \
 \
class Static##pluginName##PluginInitializer \
{ \
public: \
 \
	Static##pluginName##PluginInitializer() \
	{ \
		registerStaticPluginInstanceFunction(#pluginName, \
			pluginInstanceFunction##pluginName); \
	} \
}; \
 \
static Static##pluginName##PluginInitializer s_static##pluginName##PluginInitializer;

#endif // _PLUGIN_REGISTRATION_H_INCLUDED_
