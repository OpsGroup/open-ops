#ifndef _FRONT_TRASFORMATIONS_PLUGIN_H_INCLUDED_
#define _FRONT_TRASFORMATIONS_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace FrontTransformations
	{
		class FrontTransformationsPlugin
			: public Core::IPlugin
		{
		public:

			FrontTransformationsPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _FRONT_TRASFORMATIONS_PLUGIN_H_INCLUDED_
