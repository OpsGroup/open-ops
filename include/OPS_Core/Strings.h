#ifndef OPS_COMMON_CORE_STRINGS_H_
#define OPS_COMMON_CORE_STRINGS_H_

//	OPS includes
#include "Types.h"

//	Standard includes
#include <string>
#include <list>


namespace OPS
{
namespace Strings
{
/**
	String formatter (ASCII version)

	\param format_string - format string
	%[flags] [width] [.precision] [{h | l | ll | I | I32 | I64}]type

	Please check documentation for printf() for format specifiers.

	Also strong recommended to use OPS_CRT_FORMAT_LONG_LONG_PREFIX
	when deal with long_long_t and unsigned_long_long_t.

	\return result string
*/
std::string format(const char* const szFmt, ...);

/**
	String formatter (WIDE version)

	\param format_string - format string
	%[flags] [width] [.precision] [{h | l | ll | I | I32 | I64}]type

	Please check documentation for printf() for format specifiers.

	Also strong recommended to use OPS_CRT_FORMAT_LONG_LONG_LPREFIX
	when deal with long_long_t and unsigned_long_long_t.

	\return result string
*/
std::wstring format(const wchar_t* format_string, ...);

/**
	Boolean value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* const value, bool& result);

/**
	Integer value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* value, int& result);

/**
	Unsigned value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* value, unsigned& result);

/**
	Long long value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* value, long_long_t& result);

/**
	Unsigned long long value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* value, unsigned_long_long_t& result);

/**
	Double value fetcher (ASCII version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const char* value, double& result);

/**
	Boolean value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* const value, bool& result);

/**
	Integer value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* value, int& result);

/**
	Unsigned value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* value, unsigned& result);

/**
	Long long value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* value, long_long_t& result);

/**
	Unsigned long long value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* value, unsigned_long_long_t& result);

/**
	Double value fetcher (WIDE version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const wchar_t* value, double& result);


/**
	Boolean value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, bool& result);

/**
	Integer value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, int& result);

/**
	Unsigned value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, unsigned& result);

/**
	Long long value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, long_long_t& result);

/**
	Unsigned long long value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, unsigned_long_long_t& result);

/**
	Double value fetcher (ASCII string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::string& value, double& result);

/**
	Boolean value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, bool& result);

/**
	Integer value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, int& result);

/**
	Unsigned value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, unsigned& result);

/**
	Long long value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, long_long_t& result);

/**
	Unsigned long long value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, unsigned_long_long_t& result);

/**
	Double value fetcher (WIDE string version)

	\param value
	\param result - result value
	\return - true if success
*/
bool fetch(const std::wstring& value, double& result);



/**
	Trims tabs and spaces from left & right
*/
std::string trim(const std::string& str);

/**
	Trims tabs and spaces from left & right
*/
std::wstring trim(const std::wstring& source);

/**
	Convert ASCII string to WIDE string
*/
std::wstring widen(const std::string& source);

/**
	Convert WIDE string to ASCII string
*/
std::string narrow(const std::wstring& source);

/**
	Lowercase std::string converter
*/
std::string tolower(const std::string& source);

/**
	Lowercase std::wstring converter
*/
std::wstring tolower(const std::wstring& source);

/**
	Uppercase std::string converter
*/
std::string toupper(const std::string& source);

/**
	Uppercase std::wstring converter
*/
std::wstring toupper(const std::wstring& source);

/**
	std::string ignore-case comparer
*/
int compare_ignore_case(const std::string& string1, const std::string string2);

/**
	std::wstring ignore-case comparer
*/
int compare_ignore_case(const std::wstring& string1, const std::wstring string2);


std::basic_string<ucs2char_t> to_ucs2(const std::wstring& source);
std::basic_string<ucs4char_t> to_ucs4(const std::wstring& source);

std::wstring from_ucs2(const std::basic_string<ucs2char_t>& source);
std::wstring from_ucs4(const std::basic_string<ucs4char_t>& source);

std::string narrow_utf8(const std::wstring& source);
std::wstring widen_utf8(const std::string& source);

std::string narrow_mbcs(const std::wstring& source);

std::wstring widen_mbcs(const std::string& source);

std::list<std::string> split(const std::string& source, const std::string& delimiter);

}
}

#endif                      // OPS_COMMON_CORE_STRINGS_H_
