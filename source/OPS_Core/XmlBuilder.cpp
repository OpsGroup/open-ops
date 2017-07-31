// XmlBuilder.cpp: implementation of the OPSSettings class.
//
//////////////////////////////////////////////////////////////////////

#include "OPS_Core/XmlBuilder.h"
#include "OPS_Core/Helpers.h"
#include "OPS_Core/Strings.h"
#include <cstring>
#include <iostream>

namespace OPS
{

using namespace std;

static std::string encodeAttribute(const std::string& value);


XmlBuilder::XmlBuilder(ostream &os) : m_os(os), m_attributesPending(false), m_level(0)
{
}

XmlBuilder::~XmlBuilder()
{
}

void XmlBuilder::writeStartDocument(void)
{
    m_os << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << endl;
}

void XmlBuilder::writeEndDocument(void)
{
}

void XmlBuilder::writeStartElement(const std::string& qualifiedName)
{
	if (!m_elements.empty())
	{
		writePendingAttributes(false);
	}
	m_elements.push(qualifiedName);
    m_os << indent() << Strings::format("<%hs ", qualifiedName.c_str());
	m_attributesPending = true;
	m_level += 1;
}

void XmlBuilder::writeEndElement(void)
{
	if (m_elements.empty())
		throw OPS::RuntimeError("Unexpected writeEndElement()");

	m_level -= 1;
	if (m_attributesPending)
	{
		writePendingAttributes(true);
	}
	else
	{
        m_os << indent() << "</" + m_elements.top() + ">" << endl;
	}
	m_elements.pop();
}

void XmlBuilder::writeAttribute(const std::string& qualifiedName, const std::string& value)
{
	m_attributes.push_back(std::make_pair(qualifiedName, value));
}

void XmlBuilder::writeAttribute(const std::string& qualifiedName, const unsigned value)
{
	writeAttribute(qualifiedName, Strings::format("%u", value));
}

void XmlBuilder::writeAttribute(const std::string& qualifiedName, const int value)
{
	writeAttribute(qualifiedName, Strings::format("%i", value));
}

void XmlBuilder::writeAttribute(const std::string& qualifiedName, const double value)
{
	writeAttribute(qualifiedName, Strings::format("%f", value));
}

void XmlBuilder::writeAttribute(const std::string& qualifiedName, const bool value)
{
	writeAttribute(qualifiedName, value ? std::string("true") : std::string("false"));
}

//void XmlBuilder::writeCDATA(const std::string& text)
//{
//}

std::string XmlBuilder::indent()
{
	std::string result;
	for (int i = 0; i < m_level; ++i)
	{
		result += "  ";
	}
	return result;
}

void XmlBuilder::writePendingAttributes(const bool endElement)
{
	if (m_attributesPending)
	{
		for (TListOfAttributes::const_iterator it = m_attributes.begin(); it != m_attributes.end(); ++it)
		{
            m_os << it->first << "=\"" << encodeAttribute(it->second) << "\" ";
		}
		m_attributes.clear();
		if (endElement)
		{
            m_os << "/>" << endl;
		}
		else
		{
            m_os << ">" << endl;
		}
		m_attributesPending = false;
	}
}

static std::string encodeAttribute(const std::string& value)
{
    std::string result = value;
    size_t offset = 0;
    while (offset < result.size())
    {
        const char* replacement;
        switch (result[offset])
        {
        case '<':
            replacement = "&lt;";
            break;

        case '>':
            replacement = "&gt;";
            break;

        case '&':
            replacement = "&amp;";
            break;

        case '"':
            replacement = "&quot;";
            break;

        case '\'':
            replacement = "&apos;";
            break;

        default:
            ++offset;
            continue;
        }
		const size_t size = std::strlen(replacement);
        result.replace(offset, 1, replacement, size);
        offset += size;
    }
    return result;
}


}

