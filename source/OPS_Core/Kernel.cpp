#include <cerrno>
#include "OPS_Core/Kernel.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"

#if OPS_PLATFORM_IS_WIN32
#include <Windows.h>
#endif

#if OPS_PLATFORM_IS_UNIX
#include <sys/time.h>
#include <unistd.h>
#endif

#include <iostream>

namespace OPS
{

static std::wstring g_exePath;

#if OPS_PLATFORM_IS_WIN32
static void setCmdLineArgs(const wchar_t* args);

const std::wstring& getStartupPath()
{
	if (g_exePath.empty())
		setCmdLineArgs(::GetCommandLineW());
	return g_exePath;
}
#elif OPS_PLATFORM_IS_UNIX

const std::wstring& getStartupPath()
{
	if (g_exePath.empty())
	{
        char buff[1024];
        ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
        if (len != -1) {
          buff[len] = '\0';
          g_exePath = Strings::widen(buff);
          g_exePath = g_exePath.substr(0, g_exePath.find_last_of(L'/'));
        } else {
         /* handle error condition */
        }
	}

	return g_exePath;
}
#endif

void log(const std::wstring& message)
{
	log(Strings::narrow(message));
}

void log(const std::string& message)
{
	std::cout << message;
}

void log_console(const std::wstring& message)
{
	log_console(Strings::narrow(message));
}

void log_console(const std::string& message)
{
	std::cout << message;
}

#if OPS_PLATFORM_IS_WIN32
static void setCmdLineArgs(const wchar_t* args)
{
	std::wstring lineParams(args);
	if (lineParams.empty())
		throw ArgumentError("Could not get application path, command line parameters are empty.");

	std::wstring argv0;
	if (lineParams[0] == L'"')
	{
		size_t quotePos = lineParams.find(L'"', 1);
		if (quotePos != lineParams.npos)
			argv0 = lineParams.substr(1, quotePos - 1);
		else
			throw ArgumentError("Could not get application path, command line parameters quotes mismatch.");
	}
	else
	{
		size_t spacePos = lineParams.find(L' ');
		if (spacePos != lineParams.npos)
			argv0 = lineParams.substr(0, spacePos);
		else
			argv0 = lineParams;
	}

	size_t slashPos = argv0.rfind(L'\\');
	if (slashPos != argv0.npos)
	{
        g_exePath = argv0.substr(0, slashPos + 1);
	}
	else
	{
		wchar_t	szPath[_MAX_PATH];
		if (::GetCurrentDirectoryW(_MAX_PATH, szPath) )
		{
			g_exePath = std::wstring(szPath) + L"\\";
		}
		else
			throw RuntimeError("Directory name too long.");
	}
}
#endif


dword getTickCount(void)
{
#if OPS_PLATFORM_IS_WIN32
	return ::GetTickCount();
#elif OPS_PLATFORM_IS_UNIX
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (tv.tv_sec*1000+tv.tv_usec/1000);
#else
#error Needs porting (unsupported platform)
#endif
}

int getLastOsError(void) throw()
{
#if OPS_PLATFORM_IS_WIN32
    const DWORD error = ::GetLastError();
    return (error & 0xFFFF0000) != (0x80000000 | (FACILITY_WIN32 << 16)) ? error : error & 0x0000FFFF;
#elif OPS_PLATFORM_IS_UNIX
	return errno;
#else
#error Needs porting (unsupported platform)
#endif
}

}
