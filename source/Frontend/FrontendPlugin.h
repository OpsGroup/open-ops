#ifndef _FRONTEND_PLUGIN_H_INCLUDED_
#define _FRONTEND_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace Frontend
	{
		class FrontendPlugin
			: public Core::IPlugin
		{
		public:

			FrontendPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _FRONTEND_PLUGIN_H_INCLUDED_
