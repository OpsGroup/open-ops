//  Standard includes
#include <cerrno>
#include <cstring>

//	OPS includes
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Kernel.h"
#include "OPS_Core/Strings.h"

#if OPS_PLATFORM_IS_WIN32
//      Setup bloat level
#define NOMINMAX
//      Include needed headers, fixing up as needed
#include <Windows.h>
#endif

namespace OPS
{

//  Functions declaration
//      System call error message appender
static void appendErrorMessage(SystemCallError& error);
//      OS call error message appender
static void appendErrorMessage(OSCallError& error);
//		System call error id maker
static std::string makeSystemCallErrorId(int errorCode);
//		System call error message maker
static std::string makeSystemCallErrorMessage(int errorCode);


//	Exception class implementation
Exception::Exception(const std::string& message) throw()
{
	m_strErrorMsg = message;
}

Exception::Exception(const std::wstring& message) throw()
{
	m_strErrorMsg = Strings::narrow(message);
}

const std::string& Exception::getMessage() const
{
	return m_strErrorMsg;
}

const char* Exception::what(void) const throw()
{
	return m_strErrorMsg.c_str();
}

void Exception::appendMessage(const std::string& message)
{
    m_strErrorMsg += message;
}

void Exception::appendMessage(const std::wstring& message)
{
    m_strErrorMsg += Strings::narrow(message);
}

//	SystemCallError class implementation
SystemCallError::SystemCallError(const std::string& origin) :
    Exception(origin), m_origin(origin), m_errorCode(errno)
{
    appendErrorMessage(*this);
}

SystemCallError::SystemCallError(const std::string& origin, const int errorCode) :
    Exception(origin), m_origin(origin), m_errorCode(errorCode)
{
    appendErrorMessage(*this);
}

//      SystemCallError - methods
std::string SystemCallError::getErrorId(void) const
{
	return makeSystemCallErrorId(m_errorCode);
}

std::string SystemCallError::getErrorMessage(void) const
{
	return makeSystemCallErrorMessage(m_errorCode);
}

//	OSCallError class implementation
OSCallError::OSCallError(const std::string& origin) :
    Exception(origin), m_origin(origin), m_errorCode(getLastOsError())
{
    appendErrorMessage(*this);
}

OSCallError::OSCallError(const std::string& origin, const int errorCode) :
    Exception(origin), m_origin(origin), m_errorCode(errorCode)
{
    appendErrorMessage(*this);
}

//      OSCallError - methods
std::string OSCallError::getErrorId(void) const
{
#if OPS_PLATFORM_IS_WIN32
	return m_errorCode == NO_ERROR ? "" : Strings::format("0x%08X", m_errorCode);
#elif OPS_PLATFORM_IS_UNIX
	return makeSystemCallErrorId(m_errorCode);
#else
#error Needs porting (unsupported platform)
#endif
}

std::string OSCallError::getErrorMessage(void) const
{
#if OPS_PLATFORM_IS_WIN32
    if (m_errorCode == NO_ERROR)
        return "";
    std::string result;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE module = 0;
//  CONSTANT: first WinINet error code
    static const DWORD WININET_ERROR_CODE_FIRST = 12000;
//  CONSTANT: last WinINet error code
    static const DWORD WININET_ERROR_CODE_LAST = 12999;
    if (m_errorCode >= WININET_ERROR_CODE_FIRST && m_errorCode <= WININET_ERROR_CODE_LAST)
    {
        module = ::GetModuleHandleA("WinINet.dll");
        flags |= module != 0 ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    {
        char* buffer = 0;
        if (::FormatMessageA(flags, module, m_errorCode, 0, reinterpret_cast<LPSTR>(&buffer), 0, 0) != 0 && 
            buffer != 0)
        {
            size_t length = std::strlen(buffer);
            if (length != 0)
            {
                length -= 2;
                if (buffer[length] == '\r' && buffer[length + 1] == '\n')
                    buffer[length] = '\0';
                length = std::strlen(buffer);
                if (length != 0)
                {
                    --length;
                    if (buffer[length] == '.')
                        buffer[length] = '\0';
                    result = buffer;
                }
            }
            ::LocalFree(buffer);
			// TODO:(Top) Add proper error handling
            //if (::LocalFree(buffer) != 0)
            //    Kernel::critical_exception(L"LocalFree failed to free ::FormatMessageA() buffer.");
        }
    }
    return result;
#elif OPS_PLATFORM_IS_UNIX
	return makeSystemCallErrorMessage(m_errorCode);
#else
#error Needs porting (unsupported platform)
#endif
}

//  Functions implementation
static void appendErrorMessage(SystemCallError& error)
{
	if (error.getMessage().empty())
		error.appendMessage("System call");
    error.appendMessage(" failed");
    std::string message = error.getErrorId();
    if (!message.empty())
    {
        error.appendMessage(" with error code ");
        error.appendMessage(message);
    }
    message = error.getErrorMessage();
    if (!message.empty())
    {
        error.appendMessage(": ");
        error.appendMessage(message);
    }
    error.appendMessage(".");
}

static void appendErrorMessage(OSCallError& error)
{
	if (error.getMessage().empty())
		error.appendMessage("OS call");
    error.appendMessage(" failed");
    std::string message = error.getErrorId();
    if (!message.empty())
    {
        error.appendMessage(" with error code ");
        error.appendMessage(message);
    }
    message = error.getErrorMessage();
    if (!message.empty())
    {
        error.appendMessage(": ");
        error.appendMessage(message);
    }
    error.appendMessage(".");
}

static std::string makeSystemCallErrorId(const int errorCode)
{
    return errorCode == 0 ? "" : Strings::format("%i", errorCode);
}

static std::string makeSystemCallErrorMessage(const int errorCode)
{
    if (errorCode == 0)
        return "";
#if OPS_COMPILER_MSC_VER >= 1400
#pragma warning(disable:4996)
#endif
	char* const buffer = std::strerror(errorCode);
#if OPS_COMPILER_MSC_VER >= 1400
#pragma warning(default:4996)
#endif
    return buffer;
}

}
