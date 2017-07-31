#ifndef OPS_COMMON_CORE_COMPILER_H_
#define OPS_COMMON_CORE_COMPILER_H_

//	MS Visual C/C++
#if defined(_MSC_VER)
#define OPS_COMPILER_MSC_VER _MSC_VER
#define OPS_COMPILER_MSC_MAJORVER (_MSC_VER / 100)
#define OPS_COMPILER_MSC_MINORVER (_MSC_VER % 100)
#else
#define OPS_COMPILER_MSC_VER 0
#define OPS_COMPILER_MSC_MAJORVER 0
#define OPS_COMPILER_MSC_MINORVER 0
#endif

//  GNU C/C++
#if defined(__GNUC__)
#define OPS_COMPILER_GCC_VER (__GNUC__ * 100 + __GNUC_MINOR__)
#define OPS_COMPILER_GCC_MAJORVER __GNUC__
#define OPS_COMPILER_GCC_MINORVER __GNUC_MINOR__
#else
#define OPS_COMPILER_GCC_VER 0
#define OPS_COMPILER_GCC_MAJORVER 0
#define OPS_COMPILER_GCC_MINORVER 0
#endif

//	Only MSVC and GNU C/C++ are supported for now
#if OPS_COMPILER_MSC_VER == 0 && OPS_COMPILER_GCC_VER == 0
#error MSVC or GCC compiler not detected (unsupported compiler)
#endif

//      Object attributes
//          No-inline marker
#if OPS_COMPILER_MSC_VER >= 1310
#define OPS_ATTR_NO_INLINE __declspec(noinline)
#elif OPS_COMPILER_GCC_VER >= 0300
#define OPS_ATTR_NO_INLINE __attribute__((noinline))
#else
#error Needs porting or explicit fallback specification (unsupported compiler)
#define OPS_ATTR_NO_INLINE
#endif
//          Force-inline marker
#if OPS_COMPILER_MSC_VER >= 1310
#define OPS_ATTR_FORCE_INLINE __forceinline
#elif OPS_COMPILER_GCC_VER >= 0300
#define OPS_ATTR_FORCE_INLINE __attribute__((always_inline))
#else
#error Needs porting or explicit fallback specification (unsupported compiler)
#define OPS_ATTR_FORCE_INLINE
#endif

//      Compiler hints
//          Assume
#if OPS_COMPILER_MSC_VER >= 1310
#define OPS_COMPILER_HINT_ASSUME(argument_expression) __assume(argument_expression);
#elif OPS_COMPILER_GCC_VER >= 0300
#define OPS_COMPILER_HINT_ASSUME(argument_expression) ;
#else
#error Needs porting or explicit fallback specification (unsupported compiler)
#define OPS_COMPILER_HINT_ASSUME(argument_expression)
#endif

//      	Secure string API
#if OPS_COMPILER_MSC_VER >= 1310
#define OPS_COMPILER_HINT_SECURE_STRINGS 1
#else
#define OPS_COMPILER_HINT_SECURE_STRINGS 0
#endif


#endif                      // OPS_COMMON_CORE_COMPILER_H_
