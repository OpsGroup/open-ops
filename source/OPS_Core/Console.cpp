#include "OPS_Core/Console.h"
#include "OPS_Core/MemoryHelper.h"

#include <iostream>
#include <map>
#include <cassert>

namespace OPS
{
	using namespace std;

	class StdoutConsoleBind : public IConsoleBind
	{
	public:
		virtual void writeLine(const std::string& modName, Console::MessageLevel level, const std::string& message);
	};

	void StdoutConsoleBind::writeLine(const std::string& modName, Console::MessageLevel level, const std::string& message)
	{
		cout << IConsoleBind::prepareLine(modName, level, message) << endl;
	}

	StdoutConsoleBind g_stdConsole;

	IConsoleBind* g_consoleBind = &g_stdConsole;

	typedef std::map<const string, std::tr1::shared_ptr<Console> > consoles_list;
	consoles_list g_consoles;

	Console::Console(const std::string& modName, bool needLogging) :
	m_modName(modName)
	{
		m_needLogging=needLogging;
	}

	void Console::log(const MessageLevel level, const std::string& message)
	{
		if (m_needLogging)
		{
			g_consoleBind->writeLine(m_modName, level, message);
		}
	}

	bool Console::getNeedLogging()
	{
		return m_needLogging;
	}

	void Console::setNeedLogging(bool needLogging)
	{
		m_needLogging=needLogging;
	}

	Console& getOutputConsole(const std::string& modName, bool needLogging)
	{
		const consoles_list::iterator console = g_consoles.find(modName);
		if (console == g_consoles.end())
		{
			std::tr1::shared_ptr<Console> con(new Console(modName,needLogging));
			g_consoles.insert(std::make_pair(modName, con));
			return *con;
		}
		else
		{	
			(*console->second).setNeedLogging(needLogging);
			return *console->second;


		}


	}

	void setConsoleBinding(IConsoleBind& pConsole)
	{
		g_consoleBind = &pConsole;
	}

	std::string IConsoleBind::prepareLine(const std::string& modName, Console::MessageLevel level, const std::string& message)
	{
		std::string stdLevel;
		switch(level) 
		{
		case Console::LEVEL_DEBUG:
			stdLevel = "debug";
			break;
		case Console::LEVEL_NOTE:
			stdLevel = "note ";
			break;
		case Console::LEVEL_WARNING:
			stdLevel = "warn ";
			break;
		case Console::LEVEL_ERROR:
			stdLevel = "error";
			break;
		default:
			assert(!"IConsoleBind::prepareLine() default switch reached.");
		}

		return modName + "(" + stdLevel + "): " + message;
	}

}
