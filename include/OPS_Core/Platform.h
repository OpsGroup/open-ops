#ifndef OPS_COMMON_CORE_PLATFORM_H_
#define OPS_COMMON_CORE_PLATFORM_H_

#include "Compiler.h"

//	Platform
#if OPS_COMPILER_MSC_VER >= 1310
	#if defined(_WIN32)
		#define OPS_PLATFORM_IS_WIN32 1
		#define OPS_PLATFORM_IS_UNIX 0
	#else
		#error Win32 platform not detected (unsupported platform)
	#endif
#endif

#if OPS_COMPILER_GCC_VER >= 0400
#define OPS_PLATFORM_IS_WIN32 0
#define OPS_PLATFORM_IS_UNIX 1
#endif

//	Platform flavor
//		MacOS X
#if OPS_COMPILER_MSC_VER >= 1310
#define OPS_PLATFORM_FLAVOR_IS_MACOSX 0
#elif OPS_COMPILER_GCC_VER >= 0300
#if defined(__APPLE__) && defined(__MACH__)
#if OPS_PLATFORM_IS_UNIX
#define OPS_PLATFORM_FLAVOR_IS_MACOSX 1
#else
#error Conflicting platform flavor/type (MacOS X must be Unix)
#endif
#else
#define OPS_PLATFORM_FLAVOR_IS_MACOSX 0
#endif
#else
#error Needs porting (unsupported compiler)
#endif



#if OPS_PLATFORM_IS_WIN32
#define OPS_CRT_FORMAT_LONG_LONG_PREFIX "I64"
#define OPS_CRT_FORMAT_LONG_LONG_LPREFIX L"I64"
#elif OPS_PLATFORM_IS_UNIX
#define OPS_CRT_FORMAT_LONG_LONG_PREFIX "ll"
#define OPS_CRT_FORMAT_LONG_LONG_LPREFIX L"ll"
#endif
#if !defined(OPS_CRT_FORMAT_LONG_LONG_PREFIX) || !defined(OPS_CRT_FORMAT_LONG_LONG_LPREFIX)
#error Format string [unsigned_]long_long_t prefix not detected (unsupported platform)
#endif



#endif                      // OPS_COMMON_CORE_PLATFORM_H_
