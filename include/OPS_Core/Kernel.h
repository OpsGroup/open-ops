#ifndef OPS_COMMON_CORE_KERNEL_H_
#define OPS_COMMON_CORE_KERNEL_H_

#include <cstddef>
#include <string>
#include "OPS_Core/Compiler.h"
#include "OPS_Core/Types.h"

namespace OPS
{

/**
	Application startup path getter

	\return Application startup path
*/
const std::wstring& getStartupPath(void);

void log(const std::wstring& message);

void log(const std::string& message);

void log_console(const std::wstring& message);

void log_console(const std::string& message);

dword getTickCount(void);

int getLastOsError(void) throw();

}

#endif                      // OPS_COMMON_CORE_KERNEL_H_
