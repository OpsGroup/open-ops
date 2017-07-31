#include "OPS_Stage/Utils.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Kernel.h"

using namespace std;

namespace OPS
{
namespace Stage
{

//  Constants and enums
static const wchar_t* ROOT_PATH_RELATIVE = L"../";
static const wchar_t* CONFIGS_ROOT_PATH_RELATIVE = L"conf";
static const wchar_t* SAMPLES_ROOT_PATH_RELATIVE = L"samples";

//  Classes

//  Functions declaration

//  Variables
static std::wstring g_stageRootPath;

//  Classes implementation

//  Global classes implementation

//  Functions implementation
std::wstring getStageRoot(void)
{
	if (g_stageRootPath.empty())
	{
		return IO::combinePath(getStartupPath(), ROOT_PATH_RELATIVE);
	}
	else
	{
		return g_stageRootPath;
	}
}

void setStageRoot(const wchar_t* rootPath)
{
	g_stageRootPath = rootPath;
}

wstring getStageConfigsRoot(void)
{
	return IO::combinePath(getStageRoot(), CONFIGS_ROOT_PATH_RELATIVE);
}

wstring getStageSamplesRoot(void)
{
	return IO::combinePath(getStageRoot(), SAMPLES_ROOT_PATH_RELATIVE);
}


//  Exit namespace
}
}
