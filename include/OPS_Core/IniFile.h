// IniFile.h: interface for the IniFile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef OPS_COMMON_CORE_INIFILE_H_
#define OPS_COMMON_CORE_INIFILE_H_

#include "OPS_Core/Mixins.h"
#include "OPS_Core/Exceptions.h"

#include <string>
#include <map>

namespace OPS
{

/// Класс IniFile предназначен для работы с ini файлами.
/**
*	Файлы, с которыми работает класс OPSSettings напоминают ini-файлы ОС Windows.
*	
*	Каждый файл, таким образом, состоит из нескольких секций (идентификаторы в квадратных
*	скобках) и ряда параметров. Параметр и его значение разделяются знаком '='.
*
*	Пример (examp1.cfg).
*
*	\code
*	[Section 1]
*
*	iParam2 = 12
*	fParam2 = 2.1
*	sString3 = StringValue
*	\endcode
*
*	Имеются следующие особенности. Различают три типа параметров. integer, float и string.
*	Тип параметра определяется первой буквой его названия. В примере выше параметр 'iParam1'
*	имеет тип int, а параметр 'fParam2' - тип float. 

	
*
*/
class IniFile : public OPS::NonCopyableMix
{
public:

	/// Типы параметров, сохраняемых в конфигурационном файле
	enum ParamType
	{
		PT_UNKNOWN = 0,

		PT_INT = 1,
		PT_FIRST = PT_INT,
		PT_FLOAT,
		PT_STRING,

		PT_LAST
	};

	IniFile();
	~IniFile();

	/// Загрузить параметры из файла
	void load(const std::wstring& fileName);

	/// Получить значение параметра int
	/**
	*	Возвращает значение параметра как целое число (в случае ошибки возбуждается исключение
	*	ValueExcept)
	*
	*	\param	strSection	-	Название секции, в которой определен параметр.
	*	\param	strParam	-	Название параметра
	*	\return Значение параметра
	*/
	int getInt(const std::string& strSection, const std::string& strParam) const;

	/// Получить значение параметра как double
	/**
	*	Возвращает значение параметра как вещественное число (в случае ошибки возбуждается исключение
	*	ValueExcept)
	*
	*	\param	strSection	-	Название секции, в которой определен параметр.
	*	\param	strParam	-	Название параметра
	*	\return Значение параметра
	*/
	double getDouble(const std::string& strSection, const std::string& strParam) const;

	/// Получить значение параметра как строку
	/**
	*	Возвращает значение строкового параметра. В случае ошибки возбуждается исключение
	*	ValueExcept.
	*
	*	\param	strSection	-	Название секции, в которой определен параметр.
	*	\param	strParam	-	Название параметра
	*	\return Значение параметра
	*/
	std::string getString(const std::string& strSection, const std::string& strParam) const;

	/// Получить тип параметра
	/**
	*	Возвращает тип параметра.
	*
	*	\param	strSection	-	Название секции, в которой определен параметр.
	*	\param	strParam	-	Название параметра
	*	\return Тип параметра в соответствии с enum ParamTypes
	*/
	ParamType getType(const std::string& strSection, const std::string& strParam) const;

	/// Проверить существование секции
	/**
	*	Позволяет определить, имеется ли указанная секция в конфигурационном файле.
	*	Секция считается существующей, если в ней определен хотя бы один параметр.
	*
	*	\param	strSection	-	Название секции.
	*	\return true - секция присутствует.
	*/
	bool isSectionPresent(const std::string& strSection) const;

	/// Проверить существование параметра
	/**
	*	Позволяет определить, имеется ли указанный параметр в конфигурационном файле.
	*
	*	\param	strSection	-	Название секции.
	*	\param	strParam	-	Название параметра.
	*	\return true - секция присутствует.
	*/
	bool isParameterPresent(const std::string& strSection, const std::string& strParam) const;

	/// Возвращает содержимое загруженного файла в виде строки
	std::string dumpState() const;

	/// Общий класс исключений для конфигурационного файла
	struct Error : public OPS::RuntimeError
	{
		inline Error(const std::string& errorMsgArg, const std::wstring& filenameArg) 
			: OPS::RuntimeError(errorMsgArg), filename(filenameArg)	
		{
		}

		~Error() throw() {}

		std::wstring filename;		///<	Имя файла
	};

	/// Исключение при получении значения параметра
	struct ValueError : public Error
	{
		inline ValueError(const std::string& errorMsgArg, const std::wstring& filenameArg, 
			const std::string& sectionArg, const std::string& parameterArg) 
				: Error(errorMsgArg, filenameArg), section(sectionArg), parameter(parameterArg)
		{
		}

		~ValueError() throw() {}

		std::string section;		///< Секция
		std::string parameter;		///< Параметр
	};

	/// Исключение при разборе файла
	struct ParseError : public Error
	{
		inline ParseError(const std::string& errorMsgArg, const std::wstring& filenameArg, const int lineArg) :
			Error(errorMsgArg, filenameArg), line(lineArg)
		{
		}

		int	line;		///<	Номер строки, на которой произошла ошибка
	};

private:
	struct TValue
	{
		ParamType type;
		union MixedValue
		{
			int intValue;
			double floatValue;
		} mixedValue;
		std::string stringValue;
	};
	typedef std::map<std::pair<std::string, std::string>, TValue> TListOfValues;

	TListOfValues	m_values;
	std::wstring	m_filename;
};


//	End namespace OPS
}

#endif		// OPS_COMMON_CORE_INIFILE_H_
