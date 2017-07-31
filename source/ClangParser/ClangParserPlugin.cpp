#include "ClangParserPlugin.h"

#include <OPS_Core/Plugins/PluginRegistration.h>
#include "ClangParser/PragmaHandler.h"

EXPORT_STATIC_PLUGIN2(ClangParserPlugin, OPS::ClangParser::ClangParserPlugin)

namespace OPS
{
	namespace ClangParser
	{
		ClangParserPlugin::ClangParserPlugin()
		{
		}

		void ClangParserPlugin::initialize()
		{
            OPS::ClangParser::registerPragmaHandler(OPS::ClangParser::createOpsPragmaHandlers);
		}

		void ClangParserPlugin::terminate()
		{
		}
	}
}
