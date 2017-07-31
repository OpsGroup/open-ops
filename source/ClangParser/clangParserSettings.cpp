#include "ClangParser/clangParserSettings.h"
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include "OPS_Core/Environment.h"
#include "OPS_Core/ExternalCompiler.h"

namespace clang
{
	ClangParserSettings::ClangParserSettings(StringToStringMap* parsedMacrosMap)
		: m_useStandardIncludes(false)
		, m_useBuiltinIncludes(false)
		, m_boolKeywords(true)
		, m_compatibilityMode(CM_NONE)
		, m_parsedMacrosMap(parsedMacrosMap)
	{
	}

	void ClangParserSettings::addIncludePath(const std::string& additionalIncludePath)
	{
		m_userIncludePaths.push_back(additionalIncludePath);
	}

    void ClangParserSettings::initWithStdSettingsForCurrentPlatform()
    {
		ClangParserSettings s;
#if OPS_PLATFORM_IS_WIN32
		s = ClangParserSettings::msvcSettings();
#else
		s = ClangParserSettings::gccSettings();
#endif
		*this = s;
        std::list<std::string> stdIncludes = OPS::ExternalCompiler::getGCCorMSVSIncludePaths();
        std::list<std::string>::iterator it = stdIncludes.begin();
        for( ; it != stdIncludes.end(); ++it) addIncludePath(*it);
    }

	void ClangParserSettings::defineMacro(const std::string& name, const std::string& value)
	{
		m_userDefines[name] = value;
	}

	void ClangParserSettings::undefineMacro(const std::string& name)
	{
		m_userUndefines.push_back(name);
	}

	const ClangParserSettings& ClangParserSettings::defaultSettings()
	{
		static ClangParserSettings defaultSettings;
		return defaultSettings;
	}

	ClangParserSettings ClangParserSettings::msvcSettings()
	{
		ClangParserSettings settings;

		//settings.defineMacro("_CHAR_UNSIGNED");
		settings.defineMacro("_INTEGRAL_MAX_BITS", "64");
		settings.defineMacro("_MSC_VER", "1"); // 1500 = vs2008
		settings.defineMacro("_WIN32", "1");

		settings.m_boolKeywords = false;
		settings.m_compatibilityMode = CM_MICROSOFT;

		return settings;
	}

	ClangParserSettings ClangParserSettings::gccSettings()
	{
		ClangParserSettings settings;
		//settings.defineMacro("__STRICT_ANSI__", "1");
		settings.m_compatibilityMode = CM_GCC;
		return settings;
	}
}
