#ifndef _TRANSFORMATIONS_PLUGIN_H_INCLUDED_
#define _TRANSFORMATIONS_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace Transforms
	{
		class TransformationsPlugin
			: public Core::IPlugin
		{
		public:

			TransformationsPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _TRANSFORMATIONS_PLUGIN_H_INCLUDED_
