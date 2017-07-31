#ifndef OPS_COMMON_CORE_HELPERS_H_
#define OPS_COMMON_CORE_HELPERS_H_

//	Standard includes
#include <algorithm>
#include <valarray>

//	Local includes
#include "OPS_Core/Environment.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"

/**
	Unused argument
	
	\param argumentExpression
*/
#define OPS_UNUSED(argumentExpression) OPS::Helpers::UNUSED_(argumentExpression);

/**
	OPS Assert
	
	\param argumentExpression
*/
#if OPS_BUILD_DEBUG
#define OPS_ASSERT(argumentExpression)\
{\
    ((argumentExpression) || OPS::Helpers::ASSERT_(#argumentExpression, __FUNCTION__, __FILE__, __LINE__));\
    OPS_COMPILER_HINT_ASSUME(argumentExpression)\
}
#else
#define OPS_ASSERT(argumentExpression) OPS_COMPILER_HINT_ASSUME(argumentExpression)
#endif

/**
	OPS Verify
	
	\param argumentExpression
*/
#if OPS_BUILD_DEBUG
#define OPS_VERIFY(argumentExpression)\
{\
    ((argumentExpression) || OPS::Helpers::ASSERT_(#argumentExpression, __FUNCTION__, __FILE__, __LINE__));\
    OPS_COMPILER_HINT_ASSUME(argumentExpression)\
}
#else
#define OPS_VERIFY(argumentExpression) (argumentExpression);
#endif

/**
	OPS default case label

*/
#if OPS_BUILD_DEBUG
#define OPS_DEFAULT_CASE_LABEL \
default:\
    OPS::Helpers::DEFAULT_CASE_LABEL_(__FUNCTION__, __FILE__, __LINE__);\
    break;
#else
#define OPS_DEFAULT_CASE_LABEL \
default:\
    OPS_COMPILER_HINT_ASSUME(false)\
    break;
#endif

#if OPS_BUILD_DEBUG
#define OPS_UNREACHABLE \
	OPS::Helpers::DEFAULT_CASE_LABEL_(__FUNCTION__, __FILE__, __LINE__);
#else
#define OPS_UNREACHABLE ;
#endif


/**
    OS call handler 
*/
#define OPS_OS_CALL(function_name_value, parameters_list, predicate_expression) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw OSCallError(L###function_name_value);\
}

/**
    OS call handler with result handling
*/
#define OPS_OS_CALL_RESULT(result_expression, function_name_value, parameters_list, predicate_expression) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw OSCallError(L###function_name_value);\
}

/**
    OS call handler with failure message
*/
#define OPS_OS_CALL_MESSAGE(function_name_value, parameters_list, predicate_expression, message_value) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw OSCallError(L###function_name_value message_value);\
}

/**
    OS call handler with result handling and failure message
*/
#define OPS_OS_CALL_RESULT_MESSAGE(result_expression, function_name_value, parameters_list, predicate_expression, \
    message_value) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw OSCallError(L###function_name_value message_value);\
}

/**
    OS call handler with failure formatted text
*/
#define OPS_OS_CALL_TEXT(function_name_value, parameters_list, predicate_expression, arguments_list) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw OSCallError(L###function_name_value + Strings::format arguments_list);\
}

/**
    OS call handler with result handling failure formatted text
*/
#define OPS_OS_CALL_RESULT_TEXT(result_expression, function_name_value, parameters_list, predicate_expression, \
    arguments_list) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw OSCallError(L###function_name_value + Strings::format arguments_list);\
}

/**
    System call handler
*/
#define OPS_SYSTEM_CALL(function_name_value, parameters_list, predicate_expression) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw SystemCallError(L###function_name_value);\
}

/**
    System call handler with result handling
*/
#define OPS_SYSTEM_CALL_RESULT(result_expression, function_name_value, parameters_list, predicate_expression) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw SystemCallError(L###function_name_value);\
}

/**
    System call handler with failure message
*/
#define OPS_SYSTEM_CALL_MESSAGE(function_name_value, parameters_list, predicate_expression, message_value) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw SystemCallError(L###function_name_value message_value);\
}

/**
    System call handler with result handling and failure message
*/
#define OPS_SYSTEM_CALL_RESULT_MESSAGE(result_expression, function_name_value, parameters_list, predicate_expression, \
    message_value) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw SystemCallError(L###function_name_value message_value);\
}

/**
    System call handler with failure formatted text
*/
#define OPS_SYSTEM_CALL_TEXT(function_name_value, parameters_list, predicate_expression, arguments_list) \
{\
    if (function_name_value parameters_list predicate_expression)\
        throw SystemCallError(L###function_name_value + Strings::format arguments_list);\
}

/**
    System call handler with result handling failure formatted text
*/
#define OPS_SYSTEM_CALL_RESULT_TEXT(result_expression, function_name_value, parameters_list, predicate_expression, \
    arguments_list) \
{\
    if ((result_expression = function_name_value parameters_list) predicate_expression)\
        throw SystemCallError(L###function_name_value + Strings::format arguments_list);\
}


//	Enter namespace
namespace OPS
{
namespace Helpers
{

template<class T>
inline void safeDelete(T*& object)
{
	if (object != 0)
	{
		delete object;
		object = 0;
	}
}

template <class T>
inline void safeDeleteArray(T*& object)
{
    if (object != 0)
    {
        delete[] object;
        object = 0;
    }
}

/**
    OPS_UNUSED macro helper (internal)

    \params argument
    \return none

    \comment Used by OPS_UNUSED macro, NEVER use manually.
*/
template <class ArgumentType>
inline void UNUSED_(const ArgumentType&)
{
}

/**
    OPS_ASSERT macro helper (internal)
    \param expression
	\param function name
	\param source file
	\param source line
    \return none

    Used by OPS_ASSERT macro, NEVER use manually.
*/
inline bool ASSERT_(const char* expression, const char* function_name, const char* file_name, const unsigned line)
{
    throw AssertionFailed(Strings::format("Assertion failed (%hs) - %hs(%u) : %hs().", 
        expression, 
        file_name, 
        line, 
        function_name));
}

inline void DEFAULT_CASE_LABEL_(const char* const function_name, const char* const file_name, const unsigned line)
{
    ASSERT_("unexpected case", function_name, file_name, line);
}

//	Exit namespace
}
}

#endif                      // OPS_COMMON_CORE_HELPERS_H_
