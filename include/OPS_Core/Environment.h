#ifndef OPS_COMMON_CORE_ENVIRONMENT_H_
#define OPS_COMMON_CORE_ENVIRONMENT_H_

#include "Compiler.h"
#include "Platform.h"

//  Global defines and macros
//      Build details
//          Debug state
#if OPS_COMPILER_MSC_VER >= 1310
#if defined(_DEBUG) && defined(NDEBUG)
#error Conflicting debug state definitions
#endif
#if defined(_DEBUG)
#define OPS_BUILD_DEBUG 1
#endif
#if defined(NDEBUG)
#define OPS_BUILD_DEBUG 0
#endif
#elif OPS_COMPILER_GCC_VER >= 0300
#if defined(_DEBUG) && defined(NDEBUG)
#error Conflicting debug state definitions
#endif
#if defined(_DEBUG)
#define OPS_BUILD_DEBUG 1
#endif
#if defined(NDEBUG)
#define OPS_BUILD_DEBUG 0
#endif
#endif
#if !defined(OPS_BUILD_DEBUG)
#error Debug state not defined (unsupported compiler)
#endif


//          wchar_t UCS format
#if OPS_PLATFORM_IS_WIN32
#define OPS_ABI_WCHAR_T_UCS_MODE 2
#elif OPS_PLATFORM_IS_UNIX
#define OPS_ABI_WCHAR_T_UCS_MODE 4
#endif
#if !defined(OPS_ABI_WCHAR_T_UCS_MODE)
#error wchar_t UCS format not detected (unsupported platform)
#endif


#endif	// OPS_COMMON_CORE_ENVIRONMENT_H_
