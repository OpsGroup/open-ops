#ifndef _I_PLUGIN_H_INCLUDED_
#define _I_PLUGIN_H_INCLUDED_

namespace OPS
{
	namespace Core
	{
		class IPlugin
		{
		public:

			virtual void initialize() = 0;
			virtual void terminate() = 0;

		protected:

			inline ~IPlugin() {}
		};
	}
}

#endif // _I_PLUGIN_H_INCLUDED_
