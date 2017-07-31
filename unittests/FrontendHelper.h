#ifndef FRONTENDHELPER_H
#define FRONTENDHELPER_H

#include <Frontend/Frontend.h>

#include <Reprise/ParserResult.h>
#include <Reprise/Units.h>
#include <Reprise/Utils.h>

#include <OPS_Core/IO.h>
#include <OPS_Core/Kernel.h>

#include <string>

extern std::wstring sourceRoot;

#define COMPILE_FILE(relativeFileName)\
    OPS::Frontend::Frontend frontend; \
    { \
		const std::string input_filepath = OPS::IO::osPathToPosixPath(OPS::IO::combinePath(sourceRoot, L##relativeFileName)); \
		const OPS::Reprise::CompileResult& result = frontend.compileSingleFile(input_filepath); \
        if (result.errorCount() > 0) {std::cout << result.errorText(); std::cout.flush();}\
		ASSERT_EQ(0, result.errorCount()); \
    }

#define COMPILE_FILE_WITH_STD_INCLUDES(relativeFileName)\
    OPS::Frontend::Frontend frontend; \
    { \
        const std::string input_filepath = OPS::IO::osPathToPosixPath(OPS::IO::combinePath(sourceRoot, OPS::IO::posixPathToOsPath(relativeFileName))); \
        frontend.clangSettings().initWithStdSettingsForCurrentPlatform(); \
        const OPS::Reprise::CompileResult& result = frontend.compileSingleFile(input_filepath); \
        if (result.errorCount() > 0) {std::cout << result.errorText(); std::cout.flush();}\
        ASSERT_EQ(0, result.errorCount()); \
    }

typedef OPS::Reprise::ReprisePtr<OPS::Reprise::ProgramUnit> ProgramUnitPtr;

inline ProgramUnitPtr compileFile(const std::wstring& relativeFileName,
	OPS::Reprise::CompileResult* compileResult = NULL)
{
	const std::string input_filepath = OPS::IO::osPathToPosixPath(
		OPS::IO::combinePath(sourceRoot, relativeFileName));

	OPS::Frontend::Frontend frontend;
	OPS::Reprise::CompileResult result = frontend.compileSingleFile(input_filepath);

	if (compileResult != NULL)
	{
		*compileResult = result;
	}

	return frontend.detachProgramUnit();
}

#endif // FRONTENDHELPER_H
