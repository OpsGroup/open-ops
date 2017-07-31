// IniFile.cpp: implementation of the IniFile class.
//
//////////////////////////////////////////////////////////////////////

#include "OPS_Core/IniFile.h"
#include "OPS_Core/IO.h"
#include "OPS_Core/Helpers.h"
#include "OPS_Core/Strings.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstdio>


namespace OPS
{

using namespace std;

const unsigned MAX_LINE_SIZE = 4096;


// Construction / destruction
IniFile::IniFile()
{
}

IniFile::~IniFile()
{
}


void IniFile::load(const wstring& filename)
{
#if OPS_COMPILER_MSC_VER >= 1400
	FILE* f = 0;
	if (fopen_s(&f, IO::osPathToPosixPath(filename).c_str(), "r") != 0)
	{
		if (f != 0)
		{
			fclose(f);
		}
		throw ParseError(Strings::format("File '%ls' is not found.", filename.c_str()), filename, 0);
	}
	if (f == 0)
	{
		throw ParseError(Strings::format("File '%ls' is not found.", filename.c_str()), filename, 0);
	}
#else
	FILE* f = fopen(IO::osPathToPosixPath(filename).c_str(), "r");

	if (f == 0)	
		throw ParseError(Strings::format("File '%ls' is not found.", filename.c_str()), filename, 0);
#endif

	try
	{
		char buffer[MAX_LINE_SIZE];
		string currentSection;

		unsigned line = 0;
		TListOfValues listOfValues;

		while (!feof(f)) 
		{
			if (fgets(buffer, MAX_LINE_SIZE, f) == 0)
				break;
			line += 1;

			std::string current = Strings::trim(buffer);

			if (current.empty())
				continue;

			switch(current[0]) 
			{
				case '\0':
				case ';':
				case '/':
					break;
				case '[':
					{
						if (current[current.size() - 1] != ']')
						{
							throw ParseError(Strings::format("Section name declaration is incorrect at line %u, file '%ls'.", line, filename.c_str()), 
								filename, line);
						}
						currentSection = current.substr(1, current.size() - 2);
					}
					break;
				default:
					{
						size_t divPosition = current.find('=');
						if (divPosition == std::string::npos)
						{
							throw ParseError(Strings::format("Divisor '=' is not found at line %u, file '%ls'.", line, filename.c_str()), filename, line);
						}
						const std::string name = Strings::trim(current.substr(0, divPosition));
						const std::string strValue = Strings::trim(current.substr(divPosition + 1));
						if (name.empty())
						{
							throw ParseError(Strings::format("Name of the parameter is empty at line %u, file '%ls'.", line, filename.c_str()), filename, line);
						}
						if (strValue.empty())
						{
							throw ParseError(Strings::format("Value of the parameter is empty at line %u, file '%ls'.", line, filename.c_str()), filename, line);
						}
						{
							TValue value;
							switch (name[0])
							{
							case 'i':
								{
									value.type = PT_INT;
									if (!Strings::fetch(strValue, value.mixedValue.intValue))
									{
										throw ParseError(
											Strings::format("Unexpected value '%hs', integer expected at line %u, file '%ls'.", 
												strValue.c_str(), line, filename.c_str()), filename, line);
									}
								}
								break;
							case 'f':
								{
									value.type = PT_FLOAT;
									if (!Strings::fetch(strValue, value.mixedValue.floatValue))
									{
										throw ParseError(
											Strings::format("Unexpected value '%hs', float expected at line %u, file '%ls'.", 
												strValue.c_str(), line, filename.c_str()), filename, line);
									}
								}
								break;
							case 's':
								{
									value.type = PT_STRING;
									if (strValue[0] == '\'')
									{
										if (strValue[strValue.size() - 1] != '\'')
										{
											throw ParseError(Strings::format("Unexpected value, ' mismatch at line %u, file '%ls'.",
												line, filename.c_str()), filename, line);
										}
										std::string subValue = strValue.substr(1, strValue.size() - 2);
										size_t next = 0;
										size_t nextSingle = 0;
										for (;;)
										{
											nextSingle = subValue.find("'", next);
											next = subValue.find("''", next);
											if (nextSingle != next)
											{
												throw ParseError(Strings::format("Unexpected value, ' mismatch at line %u, file '%ls'.",
													line, filename.c_str()), filename, line);
											}
											if (next != std::string::npos)
											{
												subValue.erase(next, 1);
												next += 1;
											}
											else
											{
												break;
											}
										}
										value.stringValue = subValue;
									}
									else
									{
										value.stringValue = strValue;
									}
								}
								break;
							default:
								throw ParseError(Strings::format("Unexpected format specified in parameter '%hs' at line %u, file '%ls'.",
									name.c_str(), line, filename.c_str()), filename, line);
							}
							if (!listOfValues.insert(std::make_pair(std::make_pair(currentSection, name), value)).second)
							{
								throw ParseError(Strings::format("Parameter '%hs' in Section '%hs' at line %u already exist, file '%ls'.",
									name.c_str(), currentSection.c_str(), line, filename.c_str()), filename, line);
							}
						}
					}
			}
		}

		fclose(f);
		m_values = listOfValues;
		m_filename = filename;
	}
	catch (...)
	{
		fclose(f);
		throw;
	}
}

int IniFile::getInt(const string& section, const string& parameter) const
{
	const TListOfValues::const_iterator value = m_values.find(std::make_pair(section, parameter));
	if (value == m_values.end())
	{
		throw ValueError(Strings::format("Could not find value named '%hs' in section '%hs' file '%ls'.", 
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	if (value->second.type != PT_INT)
	{
		throw ValueError(Strings::format("Could not get int parameter named '%hs' in section '%hs', file '%ls'. Parameter type is incorrect.",
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	return value->second.mixedValue.intValue;
}

double IniFile::getDouble(const string& section, const string& parameter) const
{
	const TListOfValues::const_iterator value = m_values.find(std::make_pair(section, parameter));
	if (value == m_values.end())
	{
		throw ValueError(Strings::format("Could not find value named '%hs' in section '%hs' file '%ls'.", 
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	if (value->second.type != PT_FLOAT)
	{
		throw ValueError(Strings::format("Could not get float parameter named '%hs' in section '%hs', file '%ls'. Parameter type is incorrect.",
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	return value->second.mixedValue.floatValue;
}

string IniFile::getString(const string& section, const string& parameter) const
{
	const TListOfValues::const_iterator value = m_values.find(std::make_pair(section, parameter));
	if (value == m_values.end())
	{
		throw ValueError(Strings::format("Could not find value named '%hs' in section '%hs' file '%ls'.", 
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	if (value->second.type != PT_STRING)
	{
		throw ValueError(Strings::format("Could not get string parameter named '%hs' in section '%hs', file '%ls'. Parameter type is incorrect.",
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	return value->second.stringValue;
}

IniFile::ParamType IniFile::getType(const string& section, const string& parameter) const
{
	const TListOfValues::const_iterator value = m_values.find(std::make_pair(section, parameter));
	if (value == m_values.end())
	{
		throw ValueError(Strings::format("Could not find value named '%hs' in section '%hs' file '%ls'.", 
			parameter.c_str(), section.c_str(), m_filename.c_str()), m_filename, section, parameter);
	}
	return value->second.type;
}

bool IniFile::isParameterPresent(const string& section, const string& parameter) const
{
	const TListOfValues::const_iterator value = m_values.find(std::make_pair(section, parameter));
	return value != m_values.end();
}


bool IniFile::isSectionPresent(const string& section) const
{
	const TListOfValues::const_iterator it = m_values.lower_bound(std::make_pair(section, ""));
	if (it == m_values.end())
		return false;
	return it->first.first == section;
}

std::string IniFile::dumpState() const
{
	string result;
	result += Strings::format("File: %ls\n", m_filename.c_str());

	if (m_values.empty())
	{
		result += "Empty file.";
	}
	else
	{
		string lastSection;
		for (TListOfValues::const_iterator it = m_values.begin(); it != m_values.end(); ++it)
		{
			string value;
			switch (it->second.type)
			{
			case PT_INT:
				value = Strings::format("%i", it->second.mixedValue.intValue);
				break;
			case PT_FLOAT:
				value = Strings::format("%f", it->second.mixedValue.floatValue);
				break;
			case PT_STRING:
				value = "'" + it->second.stringValue + "'";
				break;
			OPS_DEFAULT_CASE_LABEL
			}
			if (lastSection != it->first.first)
			{
				result += Strings::format("[%hs]\n", it->first.first.c_str());
			}
			lastSection = it->first.first;
			result += Strings::format("%hs = %hs\n", it->first.second.c_str(), value.c_str());
		}
	}
	return result;
}

}

