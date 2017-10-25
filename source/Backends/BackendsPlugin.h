#ifndef _BACKENDS_PLUGIN_H_INCLUDED_
#define _BACKENDS_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace Backends
	{
		class BackendsPlugin
			: public Core::IPlugin
		{
		public:

			BackendsPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _BACKENDS_PLUGIN_H_INCLUDED_
