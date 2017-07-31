#ifndef OPS_COMMON_CORE_TYPES_H_
#define OPS_COMMON_CORE_TYPES_H_

#include <cstddef>


#include "Compiler.h"
#include "Environment.h"
//#include "Exceptions.h"


#if (OPS_COMPILER_MSC_MAJORVER >= 13)
//      Machine-sized types
typedef unsigned __int8 byte;
typedef unsigned __int16 word;
typedef unsigned __int32 dword;
typedef unsigned __int64 qword;
typedef __int8 sbyte;
typedef __int16 sword;
typedef __int32 sdword;
typedef __int64 sqword;
//      Additional types
typedef unsigned __int64 unsigned_long_long_t;
typedef __int64 long_long_t;
typedef ptrdiff_t signed_size_t;
#elif OPS_COMPILER_GCC_MAJORVER >= 4
//      Machine-sized types
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;
typedef signed char sbyte;
typedef short sword;
typedef int sdword;
typedef long long sqword;
//      Additional types
typedef unsigned long long unsigned_long_long_t;
typedef long long long_long_t;
typedef ptrdiff_t signed_size_t;
#else
#error Needs porting or explicit fallback specification (unsupported compiler)
//      Machine-sized types
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned long long qword;
typedef signed char sbyte;
typedef short sword;
typedef long sdword;
typedef long long sqword;
//      Additional types
typedef unsigned long long unsigned_long_long_t;
typedef long long long_long_t;
typedef ptrdiff_t signed_size_t;
#endif

//      Character types
#if OPS_ABI_WCHAR_T_UCS_MODE == 2
typedef wchar_t ucs2char_t;
typedef dword ucs4char_t;
#elif OPS_ABI_WCHAR_T_UCS_MODE == 4
typedef word ucs2char_t;
typedef wchar_t ucs4char_t;
#else
#error Unsupported wchar_t UCS mode
#endif


/*
namespace OPS
{
namespace Casts
{
	template<typename SignedType, typename UnsignedType>
	SignedType castUnsignedToSigned(const UnsignedType& unsignedValue)
	{
		const SignedType& signedValue = static_cast<SignedType>(unsignedValue);
		if (signedValue < 0)
			throw UnsignedToSignedOverflowError(Strings::format("Unexpected conversion from unsigned value (%u) to signed produces overflow.",
				unsignedValue));
		return signedValue;
	}
}
}
*/

#endif                      // OPS_COMMON_CORE_TYPES_H_
