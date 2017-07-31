#include "OPS_Stage/StdStreamWriter.h"

namespace OPS
{
namespace Stage
{

StdStreamWriter::StdStreamWriter(std::ostream& stream) : m_stream(stream)
{
}

void StdStreamWriter::write(const char* string)
{
	m_stream << string;
}

void StdStreamWriter::write(const wchar_t* string)
{
	m_stream << string;
}

void StdStreamWriter::writeLine(const char* string)
{
	m_stream << string << "\n";
}

void StdStreamWriter::writeLine(const wchar_t* string)
{
	m_stream << string << L"\n";
}

}
}
