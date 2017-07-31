#pragma once

#include "OPS_Core/Exceptions.h"

namespace OPS
{
namespace Stage
{

template<typename ConcreteWriter>
class GenericWriter
{
public:
	GenericWriter() : m_indent(0), m_useTabs(true)
	{
	}

	void indent()
	{
		m_indent += 1;
	}
	void outdent()
	{
		if (m_indent < 0)
			throw RuntimeError("Improper indenting used");
		m_indent -= 1;
	}

	void write(const char* string)
	{
		static_cast<ConcreteWriter*>(this)->write(string);
	}
	
	void write(const wchar_t* string)
	{
		static_cast<ConcreteWriter*>(this)->write(string);
	}

	void writeLine(const char* string)
	{
		static_cast<ConcreteWriter*>(this)->writeLine(string);
	}

	void writeLine(const wchar_t* string)
	{
		static_cast<ConcreteWriter*>(this)->writeLine(string);
	}

protected:
	int m_indent;
	bool m_useTabs;
};

}
}
