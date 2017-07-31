/*
    OPS_Stage_test/main.cpp - OPS_Stage_test module, main implementation

*/

//  Standard includes
#include <iostream>

//  OPS includes
#include "Frontend/Frontend.h"
#include "OPS_Core/EntryPoint.h"
#include "OPS_Stage/OPS_Stage.h"
#include "FrontTransforms/C2RPass.h"
#include "FrontTransforms/ResolverPass.h"
#include "ClangParser/ClangGenerator.h"
#include "OPS_Core/Kernel.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/File.h"
//  Local includes

//  Namespaces using
using namespace std;
using namespace OPS::Stage;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Core
{
//  Constants and enums

//  Classes

//  Functions declaration

//  Variables

//  Classes implementation

//  Global classes implementation
//		Application class implementation
Application::Application(void)
{
}

void Application::process(void)
{
    setStageRoot(IO::combinePath(getStartupPath(), L"../../hlc/bin/").c_str());

	EnvironmentConfig cfg;
	cfg.load(IO::combinePath(getStageConfigsRoot(), L"environment.xml").c_str());

    PassManager passManager;

	FilesCollector collector("*.c");
	FilesCollector::List files;
    collector.getFilesRecursive(Strings::narrow(IO::combinePath(getStageSamplesRoot(), L"NPB/DC/")), files);

    ClangGenerator* clangGen = new ClangGenerator;
    clangGen->settings().m_userIncludePaths = cfg.ClangIncludePathList;
    clangGen->settings().m_useStandardIncludes = false;
    clangGen->settings().m_useBuiltinIncludes = false;
    clangGen->settings().m_userDefines["__int64"] = "long long";
    clangGen->settings().m_userUndefines.push_back("UNIX");
    clangGen->settings().addIncludePath(Strings::narrow(IO::combinePath(getStageRoot(), L"lib/clang/1.1/include")));
    clangGen->settings().addIncludePath(Strings::narrow(IO::combinePath(getStageRoot(), L"lib/msvc/include")));

	cout << "Processing files:\n";
	for (FilesCollector::List::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		std::string currentFile = Strings::narrow(IO::posixPathToOsPath(*it));
		cout << currentFile << "\n";
        clangGen->addFile(currentFile);
	}

    passManager.setGenerator(clangGen);
    passManager.addPass(new C2RPass);
    passManager.addPass(new RepriseCanonizationPass);
    passManager.addPass(new ResolverPass);

    passManager.run();

	std::cout << "Application run loop." << std::endl;
	std::cout << "Throwing." << std::endl;
	throw StateError("Error!");
}

void Application::end_process(void)
{
	std::cout << "Application end." << std::endl;
}

//  Static methods
bool Application::is_console_only(void)
{
	return true;
}

const char* Application::get_name(void)
{
	return "OPS_Stage_testmod";
}

const wchar_t* Application::get_title(void)
{
	return L"OPS Stage testmod Application";
}

const char* Application::get_default_language(void)
{
	return "en";
}

bool Application::handle_error(const wchar_t* text, const wchar_t* header)
{
	OPS_UNUSED(header)
	std::cout << "Error occured: " << Strings::narrow(text) << "\n";
	return true;
}

//  Functions implementation

//  Exit namespace
}
}
