#ifndef OPS_REPRISE_PARSERRESULT_H_INCLUDED__
#define OPS_REPRISE_PARSERRESULT_H_INCLUDED__

#include <list>
#include <string>
#include "OPS_Core/Strings.h"

namespace OPS
{
namespace Reprise
{

struct CompilerResultMessage
{
    enum CompilerResultKind
    {
        CRK_NONE = 0,
        CRK_FIRST = 1,

        CRK_IGNORE = CRK_FIRST,
        CRK_NOTE,
        CRK_WARNING,
        CRK_ERROR,
        CRK_FATAL,

        CRK_LAST,
    };

	typedef std::pair<unsigned int, unsigned int> TLocation;

    /// Тип сообщения компилятора
    CompilerResultKind    m_resultKind;
    /// Текст сообщения
    std::string           m_message;
    /// Текст сообщения
    std::string           m_codeSnippet;
    ///  Файл с ошибочным кодом
    std::string           m_fileName;
    ///  Строка и столбец, где произошла ошибка
    TLocation m_location;
    ///  Уточнение m_location. Например, если ошибка произошла в h-файле, то здесь будет указан файл, из которого подключен ошибочный
    std::string           m_additionalLocationInfo;

	inline CompilerResultMessage() : m_resultKind(CRK_NONE) {}

    inline CompilerResultMessage(const CompilerResultKind resultKind, 
        const std::string& message, 
        const std::string& codeSnippet,
        const std::string& fileName, 
        const TLocation& location,
        const std::string& additionalLocationInfo):
    m_resultKind(resultKind), 
        m_message(message), 
        m_codeSnippet(codeSnippet), 
        m_fileName(fileName), 
        m_location(location),
        m_additionalLocationInfo(additionalLocationInfo)
    {
    }

    CompilerResultMessage(const CompilerResultKind resultKind, const std::string &message, const std::string &fileName)
        :m_resultKind(resultKind)
        ,m_message(message)
        ,m_fileName(fileName)
        ,m_location(TLocation(0,0))
    {
    }

    std::string errorText() const
    {
        std::string kind;
        switch (m_resultKind)
        {
        case CRK_IGNORE:
            kind = "IGNORE";
            break;
        case CRK_NOTE:
            kind = "NOTE";
            break;
        case CRK_WARNING:
            kind = "WARN";
            break;
        case CRK_ERROR:
            kind = "ERROR";
            break;
        case CRK_FATAL:
            kind = "FATAL";
            break;
        default:
            kind = "(unknown)";
        }
        return Strings::format("%hs %hs:%u:%u %hs", kind.c_str(), m_fileName.c_str(), m_location.first, m_location.second, m_message.c_str());
    }
};

struct CompileResult
{
	typedef std::list<CompilerResultMessage> MessageList;

	MessageList m_messages;
	std::string m_fileName;

	inline int errorCount(void) const
	{
		int result = 0;
		for (MessageList::const_iterator it = m_messages.begin(); it != m_messages.end(); ++it)
		{
			if (it->m_resultKind >= CompilerResultMessage::CRK_ERROR)
			{
				result += 1;
			}
		}
		return result;
	}

	inline std::string errorText(void) const
	{
		std::string result;
		for (MessageList::const_iterator it = m_messages.begin(); it != m_messages.end(); ++it)
		{
            result += Strings::format("%hs\n", it->errorText().c_str());
		}
		return result;
	}
};

}
}

#endif
