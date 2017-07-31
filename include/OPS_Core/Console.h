#ifndef OPS_COMMON_CORE_CONSOLE_H_
#define OPS_COMMON_CORE_CONSOLE_H_

// Standard includes
#include <string>

namespace OPS
{
/**
	Манипулятор для вывода на консоль
*/
class Console
{
public:
///	Уровни сообщений
	enum MessageLevel
	{
		LEVEL_DEBUG = 1,
		LEVEL_NOTE,
		LEVEL_WARNING,
		LEVEL_ERROR
	};
	
///	Вывести строку в лог.
/**
	\param level - уровень сообщений
	\param message - сообщение
	\return none
*/
	void log(MessageLevel level, const std::string& message);
	bool getNeedLogging();
	void setNeedLogging(bool needLogging);
protected:
	Console(const std::string& modName, bool needLogging = true);
	std::string m_modName;
	bool m_needLogging;
	friend Console& getOutputConsole(const std::string& modName, bool needLogging = true);
};

/// Получить манипулятор для вывода на консоль
Console& getOutputConsole(const std::string& modName, bool needLogging);

/// Привязка консоли, в которую выводятся строки
class IConsoleBind
{
public:
	
	virtual void writeLine(const std::string& modName, Console::MessageLevel level, const std::string& message) = 0;

protected:
	static std::string prepareLine(const std::string& modName, Console::MessageLevel level, const std::string& message);
};

/// Установить привязку консоли
void setConsoleBinding(IConsoleBind& pConsole);

}

#endif                      // OPS_COMMON_CORE_CONSOLE_H_
