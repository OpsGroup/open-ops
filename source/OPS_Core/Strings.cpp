//	OPS includes
#include "OPS_Core/Compiler.h"
#include "OPS_Core/Strings.h"

//	Standard includes
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
//#include <cwctype>
#include <locale>
#include <string>
#include <valarray>
#include <cmath>

namespace OPS
{
namespace Strings
{

//      Wide string buffer size for character conversions
static const size_t STRING_BUFFER_SIZE = 32;

//      UCS-4 to UCS-2 string converter
static std::basic_string<ucs2char_t> ucs4_to_ucs2(const std::basic_string<ucs4char_t>& source);
//      UCS-2 to UCS-4 string converter
static std::basic_string<ucs4char_t> ucs2_to_ucs4(const std::basic_string<ucs2char_t>& source);

std::string format(const char* const szFmt, ...)
{
	std::string message;
#if OPS_COMPILER_MSC_VER != 0
	va_list argList;
	va_start(argList, szFmt);
	const int strLength = _vscprintf(szFmt, argList);
	std::valarray<std::string::value_type> buffer(strLength + 1);
    if (vsprintf_s(&buffer[0], strLength + 1, szFmt, argList) == strLength)
		message = &buffer[0];
   	va_end(argList);
#else
    size_t buffer_size = std::strlen(szFmt) * 4;
    std::valarray<char> buffer(buffer_size + 1);
    buffer[0] = '\0';
    try
	{
        for (;;)
        {
            va_list argList;
            va_start(argList, szFmt);
            buffer[0] = '\0';
            const int count = vsnprintf(&buffer[0], buffer_size, szFmt, argList);
            va_end(argList);
            if (count >= 0 && static_cast<size_t>(count) < buffer_size - 1)
                break;
            buffer_size *= 2;
            buffer.resize(buffer_size + 1);
        }
	}
    catch (...)
	{
	}
    buffer[buffer_size] = '\0';
    message = &buffer[0];

#endif
	return message;
}

std::wstring format(const wchar_t* const format_string, ...)
{
	size_t buffer_size = std::wcslen(format_string) * 2;
	std::valarray<wchar_t> buffer(buffer_size + 1);
	try
	{
		for (;;)
		{
			buffer[0] = L'\0';
                        std::va_list va;
                        va_start(va, format_string);
			if (std::vswprintf(&buffer[0], buffer_size, format_string, va) >= 0)
				break;
                        va_end(va);
			buffer_size *= 2;
			buffer.resize(buffer_size + 1);
		}
	}
	catch (...)
	{
	}
	buffer[buffer_size] = L'\0';
	const std::wstring result(&buffer[0]);
	return result;
}

bool fetch(const char* const value, bool& result)
{
    if (std::strcmp(value, "true") == 0)
        result = true;
    else
        if (std::strcmp(value, "false") == 0)
            result = false;
        else
            return false;
    return true;
}

bool fetch(const char* value, int& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - '0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case '-':
                if (sign)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case '+':
                if (sign)
                    return false;
                ++value;
                sign = true;
                continue;

            }
            return false;
        }
        const int last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const char* value, unsigned& result)
{
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - '0';
        if (digit < 0 || digit > 9)
            return false;
        const unsigned last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    return success;
}

bool fetch(const char* value, long_long_t& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - '0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case '-':
                if (sign)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case '+':
                if (sign)
                    return false;
                ++value;
                sign = true;
                continue;

            }
            return false;
        }
        const long_long_t last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const char* value, unsigned_long_long_t& result)
{
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - '0';
        if (digit < 0 || digit > 9)
            return false;
        const unsigned_long_long_t last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    return success;
}

bool fetch(const char* value, double& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    double fraction = 0.0;
    result = 0.0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - '0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case '-':
                if (sign || fraction != 0.0)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case '+':
                if (sign || fraction != 0.0)
                    return false;
                ++value;
                sign = true;
                continue;

            case '.':
            case ',':
                if (fraction > 0.0)
                    return false;
                fraction = 0.1;
                ++value;
                continue;

            case 'e':
            case 'E':
                {
                    ++value;
                    int exponent;
                    if (!fetch(value, exponent))
                        return false;
                    result *= std::pow(10.0, exponent);
                    if (success && negate)
                        result = -result;
                    return success;
                }
                break;
            }
            return false;
        }
        if (fraction == 0.0)
            result = result * 10 + digit;
        else
        {
            result += digit * fraction;
            fraction *= 0.1;
        }
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const wchar_t* const value, bool& result)
{
    if (std::wcscmp(value, L"true") == 0)
        result = true;
    else
        if (std::wcscmp(value, L"false") == 0)
            result = false;
        else
            return false;
    return true;
}

bool fetch(const wchar_t* value, int& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - L'0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case L'-':
                if (sign)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case L'+':
                if (sign)
                    return false;
                ++value;
                sign = true;
                continue;

            }
            return false;
        }
        const int last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const wchar_t* value, unsigned& result)
{
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - L'0';
        if (digit < 0 || digit > 9)
            return false;
        const unsigned last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    return success;
}

bool fetch(const wchar_t* value, long_long_t& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - L'0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case L'-':
                if (sign)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case L'+':
                if (sign)
                    return false;
                ++value;
                sign = true;
                continue;

            }
            return false;
        }
        const long_long_t last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const wchar_t* value, unsigned_long_long_t& result)
{
    bool success = false;
    result = 0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - L'0';
        if (digit < 0 || digit > 9)
            return false;
        const unsigned_long_long_t last_result = result;
        result = result * 10 + digit;
        if (result < last_result)
            return false;
        ++value;
        success = true;
    }
    return success;
}

bool fetch(const wchar_t* value, double& result)
{
    bool negate = false;
    bool sign = false;
    bool success = false;
    double fraction = 0.0;
    result = 0.0;
    for (;;)
    {
        if (*value == '\0')
            break;
        const int digit = *value - L'0';
        if (digit < 0 || digit > 9)
        {
            switch (*value)
            {
            case L'-':
                if (sign || fraction != 0.0)
                    return false;
                negate = true;
                ++value;
                sign = true;
                continue;

            case L'+':
                if (sign || fraction != 0.0)
                    return false;
                ++value;
                sign = true;
                continue;

            case L'.':
            case L',':
                if (fraction > 0.0)
                    return false;
                fraction = 0.1;
                ++value;
                continue;

            case L'e':
            case L'E':
                {
                    ++value;
                    int exponent;
                    if (!fetch(value, exponent))
                        return false;
                    result *= std::pow(10.0, exponent);
                    if (success && negate)
                        result = -result;
                    return success;
                }
                break;
            }
            return false;
        }
        if (fraction == 0.0)
            result = result * 10 + digit;
        else
        {
            result += digit * fraction;
            fraction *= 0.1;
        }
        ++value;
        success = true;
    }
    if (success && negate)
        result = -result;
    return success;
}

bool fetch(const std::string& value, bool& result)
{
	return fetch(value.c_str(), result);
}

bool fetch(const std::string& value, int& result)
{
	return fetch(value.c_str(), result);
}

bool fetch(const std::string& value, unsigned& result)
{
	return fetch(value.c_str(), result);
}

bool fetch(const std::string& value, long_long_t& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::string& value, unsigned_long_long_t& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::string& value, double& result)
{
	return fetch(value.c_str(), result);
}

bool fetch(const std::wstring& value, bool& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::wstring& value, int& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::wstring& value, unsigned& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::wstring& value, long_long_t& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::wstring& value, unsigned_long_long_t& result)
{
	return fetch(value.c_str(), result);
}
bool fetch(const std::wstring& value, double& result)
{
	return fetch(value.c_str(), result);
}


/*
std::string ltrim(const std::string& str)
{
	std::string tmp(str);
	tmp.erase(0, tmp.find_first_not_of(" \t"));
	return tmp;
}

std::string rtrim(const std::string& str)
{
	std::string tmp(str);
	tmp.erase(tmp.find_last_not_of(" \t") + 1);
	return tmp;
}
*/

std::string trim(const std::string& source)
{
	const char* const begin = source.c_str();
	const char* const end = begin + source.size();
	const std::ctype<char>& scan_facet = std::use_facet<std::ctype<char> >(std::locale());
	const char* const start = scan_facet.scan_not(std::ctype_base::space, begin, end);
	if (start == end)
		return "";
	const char* finish = start;
	for (;;)
	{
		finish = scan_facet.scan_is(std::ctype_base::space, finish + 1, end);
		if (finish == end)
			break;
		const char* last = scan_facet.scan_not(std::ctype_base::space, finish + 1, end);
		if (last == end)
			break;
		finish = last;
	}
	return std::string(start, finish);
}

std::wstring trim(const std::wstring& source)
{
	const wchar_t* const begin = source.c_str();
	const wchar_t* const end = begin + source.size();
	const std::ctype<wchar_t>& scan_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	const wchar_t* const start = scan_facet.scan_not(std::ctype_base::space, begin, end);
	if (start == end)
		return L"";
	const wchar_t* finish = start;
	for (;;)
	{
		finish = scan_facet.scan_is(std::ctype_base::space, finish + 1, end);
		if (finish == end)
			break;
		const wchar_t* last = scan_facet.scan_not(std::ctype_base::space, finish + 1, end);
		if (last == end)
			break;
		finish = last;
	}
	return std::wstring(start, finish);
}

std::wstring widen(const std::string& source)
{
	const size_t length = source.size();
	std::wstring result;
	result.reserve(length);
	const char* const begin = source.c_str();
	const std::ctype<wchar_t>& conversion_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		wchar_t target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400 && OPS_COMPILER_MSC_VER < 1600
		conversion_facet._Widen_s(begin + offset, begin + offset + buffer_size, target, buffer_size);
#else
		conversion_facet.widen(begin + offset, begin + offset + buffer_size, target);
#endif
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

std::string narrow(const std::wstring& source)
{
	const size_t length = source.size();
	std::string result;
	result.reserve(length);
	const wchar_t* const begin = source.c_str();
	const std::ctype<wchar_t>& conversion_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		char target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400 && OPS_COMPILER_MSC_VER < 1600
		conversion_facet._Narrow_s(begin + offset, begin + offset + buffer_size, '?', target, buffer_size);
#else
		conversion_facet.narrow(begin + offset, begin + offset + buffer_size, '?', target);
#endif
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

std::string tolower(const std::string& source)
{
	const size_t length = source.size();
	std::string result;
	result.reserve(length);
	const std::ctype<char>& conversion_facet = std::use_facet<std::ctype<char> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		char target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		source._Copy_s(target, buffer_size, buffer_size, offset);
#else
		source.copy(target, buffer_size, offset);
#endif
		conversion_facet.tolower(target, target + buffer_size);
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

std::wstring tolower(const std::wstring& source)
{
	const size_t length = source.size();
	std::wstring result;
	result.reserve(length);
	const std::ctype<wchar_t>& conversion_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		wchar_t target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		source._Copy_s(target, buffer_size, buffer_size, offset);
#else
		source.copy(target, buffer_size, offset);
#endif
		conversion_facet.tolower(target, target + buffer_size);
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

std::string toupper(const std::string& source)
{
	const size_t length = source.size();
	std::string result;
	result.reserve(length);
	const std::ctype<char>& conversion_facet = std::use_facet<std::ctype<char> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		char target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		source._Copy_s(target, buffer_size, buffer_size, offset);
#else
		source.copy(target, buffer_size, offset);
#endif
		conversion_facet.toupper(target, target + buffer_size);
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

std::wstring toupper(const std::wstring& source)
{
	const size_t length = source.size();
	std::wstring result;
	result.reserve(length);
	const std::ctype<wchar_t>& conversion_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		wchar_t target[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		source._Copy_s(target, buffer_size, buffer_size, offset);
#else
		source.copy(target, buffer_size, offset);
#endif
		conversion_facet.toupper(target, target + buffer_size);
		result.append(target, target + buffer_size);
		offset += buffer_size;
	}
	return result;
}

int compare_ignore_case(const std::string& string1, const std::string string2)
{
	const size_t length = std::min(string1.size(), string2.size());
	const std::ctype<char>& conversion_facet = std::use_facet<std::ctype<char> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		char target1[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		string1._Copy_s(target1, buffer_size, buffer_size, offset);
#else
		string1.copy(target1, buffer_size, offset);
#endif
		conversion_facet.tolower(target1, target1 + buffer_size);
		char target2[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		string2._Copy_s(target2, buffer_size, buffer_size, offset);
#else
		string2.copy(target2, buffer_size, offset);
#endif
		conversion_facet.tolower(target2, target2 + buffer_size);
		const int result = std::memcmp(target1, target2, buffer_size);
		if (result != 0)
			return result;
		offset += buffer_size;
	}
	return string1.size() < string2.size() ? -1 : string1.size() > string2.size() ? 1 : 0;
}

int compare_ignore_case(const std::wstring& string1, const std::wstring string2)
{
	const size_t length = std::min(string1.size(), string2.size());
	const std::ctype<wchar_t>& conversion_facet = std::use_facet<std::ctype<wchar_t> >(std::locale());
	size_t offset = 0;
	while (offset < length)
	{
		const size_t difference = length - offset;
		const size_t buffer_size = difference > STRING_BUFFER_SIZE ? STRING_BUFFER_SIZE : difference;
		wchar_t target1[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		string1._Copy_s(target1, buffer_size, buffer_size, offset);
#else
		string1.copy(target1, buffer_size, offset);
#endif
		conversion_facet.tolower(target1, target1 + buffer_size);
		wchar_t target2[STRING_BUFFER_SIZE];
#if OPS_COMPILER_MSC_VER >= 1400
		string2._Copy_s(target2, buffer_size, buffer_size, offset);
#else
		string2.copy(target2, buffer_size, offset);
#endif
		conversion_facet.tolower(target2, target2 + buffer_size);
		const int result = std::wmemcmp(target1, target2, buffer_size);
		if (result != 0)
			return result;
		offset += buffer_size;
	}
	return string1.size() < string2.size() ? -1 : string1.size() > string2.size() ? 1 : 0;
}

std::basic_string<ucs2char_t> to_ucs2(const std::wstring& source)
{
#if OPS_ABI_WCHAR_T_UCS_MODE == 2
	return source;
#elif OPS_ABI_WCHAR_T_UCS_MODE == 4
	return ucs4_to_ucs2(source);
#else
#error Unsupported wchar_t UCS mode
#endif
}

std::basic_string<ucs4char_t> to_ucs4(const std::wstring& source)
{
#if OPS_ABI_WCHAR_T_UCS_MODE == 2
	return ucs2_to_ucs4(source);
#elif OPS_ABI_WCHAR_T_UCS_MODE == 4
	return source;
#else
#error Unsupported wchar_t UCS mode
#endif
}

std::wstring from_ucs2(const std::basic_string<ucs2char_t>& source)
{
#if OPS_ABI_WCHAR_T_UCS_MODE == 2
	return source;
#elif OPS_ABI_WCHAR_T_UCS_MODE == 4
	return ucs2_to_ucs4(source);
#else
#error Unsupported wchar_t UCS mode
#endif
}

std::wstring from_ucs4(const std::basic_string<ucs4char_t>& source)
{
#if OPS_ABI_WCHAR_T_UCS_MODE == 2
	return ucs4_to_ucs2(source);
#elif OPS_ABI_WCHAR_T_UCS_MODE == 4
	return source;
#else
#error Unsupported wchar_t UCS mode
#endif
}

std::string narrow_utf8(const std::wstring& source)
{
	std::string result;
	std::basic_string<ucs4char_t> ucs4_source = to_ucs4(source);
	result.reserve(ucs4_source.size());
	for (std::basic_string<ucs4char_t>::const_iterator position = ucs4_source.begin();
		position != ucs4_source.end();
		++position)
	{
		byte buffer[5] = "?\0\0\0";
		ucs4char_t code = *position;
		int count = 0;
		if (code < 0x00000080)
			count = 1;
		else
			if (code < 0x0000800)
				count = 2;
			else 
				if (code < 0x00010000)
					count = 3;
				else 
					if (code < 0x00110000)
						count = 4;
		switch (count)
		{
		case 4:
			buffer[3] = 0x80 | (static_cast<byte>(code) & 0x3F);
			code >>= 6;
			code |= 0x10000;

		case 3: 
			buffer[2] = 0x80 | (static_cast<byte>(code) & 0x3F);
			code >>= 6;
			code |= 0x800;

		case 2:
			buffer[1] = 0x80 | (static_cast<byte>(code) & 0x3F);
			code >>= 6; 
			code |= 0xC0;

		case 1:
			buffer[0] = static_cast<byte>(code);
		}
		result += reinterpret_cast<char*>(buffer);
	}
	return result;
}

std::wstring widen_utf8(const std::string& source)
{
	std::basic_string<ucs4char_t> result;
	result.reserve(source.size());
	std::string::const_iterator position = source.begin();
	while (position != source.end())
	{
		ucs4char_t result_code = 0x0000FFFD;
		const byte code = *position++;
		if (code < 0x80)
			result_code = code;
		else
			if (code >= 0xC2)
			{
				if (code < 0xE0)
				{
					if (position != source.end())
					{
						const byte code_1 = *position++;
						if ((code_1 ^ 0x80) < 0x40)
							result_code = (static_cast<ucs4char_t>(code & 0x1F) << 6) | 
							static_cast<ucs4char_t>(code_1 ^ 0x80);
					}
				}
				else
					if (code < 0xF0)
					{
						if (position != source.end())
						{
							const byte code_1 = *position++;
							if (position != source.end())
							{
								const byte code_2 = *position++;
								if ((code_1 ^ 0x80) < 0x40 && (code_2 ^ 0x80) < 0x40 && 
									(code >= 0xE1 || code_1 >= 0xA0))
									result_code = (static_cast<ucs4char_t>(code & 0x0F) << 12) | 
									(static_cast<ucs4char_t>(code_1 ^ 0x80) << 6) | 
									static_cast<ucs4char_t>(code_2 ^ 0x80);
							}
						}
					}
					else
						if (code < 0xF8)
						{
							if (position != source.end())
							{
								const byte code_1 = *position++;
								if (position != source.end())
								{
									const byte code_2 = *position++;
									if (position != source.end())
									{
										const byte code_3 = *position++;
										if ((code_1 ^ 0x80) < 0x40 && (code_2 ^ 0x80) < 0x40 &&
											(code_3 ^ 0x80) < 0x40 &&
											(code >= 0xF1 || code_1 >= 0x90) &&
											(code < 0xF4 || (code == 0xF4 && code_1 < 0x90)))
											result_code = (static_cast<ucs4char_t>(code & 0x07) << 18) |
											(static_cast<ucs4char_t>(code_1 ^ 0x80) << 12) |
											(static_cast<ucs4char_t>(code_2 ^ 0x80) << 6) |
											static_cast<ucs4char_t>(code_3 ^ 0x80);
									}
								}
							}
						}
			}
			result.push_back(result_code);
	}
	return from_ucs4(result);
}

std::string narrow_mbcs(const std::wstring& source)
{
#if OPS_COMPILER_MSC_VER >= 1400
	size_t length;
	wcstombs_s(&length, 0, 0, source.c_str(), 0);
#else
	const size_t length = std::wcstombs(0, source.c_str(), 0);
#endif
	if (length == static_cast<size_t>(-1))
#if OPS_PLATFORM_IS_WIN32
		return narrow(source);
#elif OPS_PLATFORM_IS_UNIX
		return narrow_utf8(source);
#else
#error Needs porting or explicit fallback specification (unsupported platform)
		return narrow_utf8(source);
#endif
	std::valarray<char> buffer(length + 1);
#if OPS_COMPILER_MSC_VER >= 1400
	{
		size_t unused_length;
		wcstombs_s(&unused_length, &buffer[0], length, source.c_str(), length);
	}
#else
	std::wcstombs(&buffer[0], source.c_str(), length);
#endif
	buffer[length] = '\0';
	return &buffer[0];
}

std::wstring widen_mbcs(const std::string& source)
{
#if OPS_COMPILER_MSC_VER >= 1400
	size_t length;
	mbstowcs_s(&length, 0, 0, source.c_str(), 0);
#else
	const size_t length = std::mbstowcs(0, source.c_str(), 0);
#endif
	if (length == static_cast<size_t>(-1))
#if OPS_PLATFORM_IS_WIN32
		return widen(source);
#elif OPS_PLATFORM_IS_UNIX
		return widen_utf8(source);
#else
#error Needs porting or explicit fallback specification (unsupported platform)
		return widen_utf8(source);
#endif
	std::valarray<wchar_t> buffer(length + 1);
#if OPS_COMPILER_MSC_VER >= 1400
	{
		size_t unused_length;
		mbstowcs_s(&unused_length, &buffer[0], length, source.c_str(), length);
	}
#else
	std::mbstowcs(&buffer[0], source.c_str(), length);
#endif
	buffer[length] = L'\0';
	return &buffer[0];
}

//  Functions implementation
static std::basic_string<ucs2char_t> ucs4_to_ucs2(const std::basic_string<ucs4char_t>& source)
{
	std::basic_string<ucs2char_t> result;
	result.reserve(source.size());
	for (std::basic_string<ucs4char_t>::const_iterator position = source.begin();
		position != source.end();
		++position)
	{
		ucs2char_t result_code = 0xFFFD;
		ucs4char_t code = *position;
		if (code < 0x10000)
			result_code = static_cast<ucs2char_t>(code);
		else
			if (code < 0x110000)
			{
				code -= 0x10000;
				result.push_back(0xD800 + static_cast<ucs2char_t>(code >> 10));
				result_code = 0xDC00 + static_cast<ucs2char_t>(code & 0x000003FF);
			}
			result.push_back(result_code);
	}
	return result;
}

static std::basic_string<ucs4char_t> ucs2_to_ucs4(const std::basic_string<ucs2char_t>& source)
{
	std::basic_string<ucs4char_t> result;
	result.reserve(source.size());
	std::basic_string<ucs2char_t>::const_iterator position = source.begin();
	while (position != source.end())
	{
		ucs4char_t result_code = 0x0000FFFD;
		ucs2char_t code = *position++;
		if (code < 0x0000D800 || code >= 0x0000E000)
			result_code = code;
		else
			if (code < 0x0000DC00)
			{
				if (position != source.end())
				{
					const wchar_t code_1 = *position++;
					if (code_1 >= 0xDC00 && code_1 < 0xE000)
						result_code = 0x00010000 + ((code - 0x0000D800) << 10) + (code_1 - 0xDC00);
				}
			}
			result.push_back(result_code);
	}
	return result;
}

std::list<std::string> split(const std::string& source, const std::string& delimiter)
{
	std::list<std::string> result;
	std::string s = source;
	size_t pos = 0;

	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		result.push_back(s.substr(0, pos));
		s.erase(0, pos + delimiter.size());
	}
	result.push_back(s);
	return result;
}

}
}
