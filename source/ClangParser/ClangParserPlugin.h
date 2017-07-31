#ifndef _CLANG_PARSER_PLUGIN_H_INCLUDED_
#define _CLANG_PARSER_PLUGIN_H_INCLUDED_

#include <OPS_Core/Plugins/IPlugin.h>

namespace OPS
{
	namespace ClangParser
	{
		class ClangParserPlugin
			: public Core::IPlugin
		{
		public:

			ClangParserPlugin();

		public: // Core::IPlugin

			virtual void initialize(); // override
			virtual void terminate(); // override
		};
	}
}

#endif // _CLANG_PARSER_PLUGIN_H_INCLUDED_
