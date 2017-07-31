#pragma once

#include <iostream>

#include "OPS_Core/Mixins.h"

#include "OPS_Stage/GenericWriter.h"



namespace OPS
{
namespace Stage
{


class StdStreamWriter : public GenericWriter<StdStreamWriter>, virtual public OPS::NonCopyableMix
{
public:
	StdStreamWriter(std::ostream& stream);

	void write(const char* string);

	void write(const wchar_t* string);

	void writeLine(const char* string);

	void writeLine(const wchar_t* string);

private:
	std::ostream& m_stream;
};

}
}
