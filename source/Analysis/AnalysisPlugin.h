#ifndef _ANALYSIS_PLUGIN_H_INCLUDED_
#define _ANALYSIS_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace Analysis
	{
		class AnalysisPlugin
			: public Core::IPlugin
		{
		public:

			AnalysisPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _ANALYSIS_PLUGIN_H_INCLUDED_
