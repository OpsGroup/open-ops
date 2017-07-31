// XmlBuilder.h: interface for the XmlBuilder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef OPS_COMMON_CORE_XMLBUILDER_H_
#define OPS_COMMON_CORE_XMLBUILDER_H_

#include "OPS_Core/Mixins.h"
#include "OPS_Core/Exceptions.h"

#include <string>
#include <list>
#include <stack>

namespace OPS
{

/// 
/**

*/
class XmlBuilder : public OPS::NonCopyableMix
{
public:
    explicit XmlBuilder(std::ostream& os);
	~XmlBuilder();

	void writeStartDocument(void);
	void writeEndDocument(void);
	void writeStartElement(const std::string& qualifiedName);
	void writeEndElement(void);
	void writeAttribute(const std::string& qualifiedName, const std::string& value);
	void writeAttribute(const std::string& qualifiedName, const unsigned value);
	void writeAttribute(const std::string& qualifiedName, const int value);
	void writeAttribute(const std::string& qualifiedName, const double value);
	void writeAttribute(const std::string& qualifiedName, const bool value);
//	void writeCDATA(const std::string& text);

private:
	typedef std::list<std::pair<std::string, std::string> > TListOfAttributes;

	std::string indent();
	void writePendingAttributes(const bool endElement);

    std::ostream& m_os;
	std::stack<std::string> m_elements;
	TListOfAttributes m_attributes;
	bool m_attributesPending;
	int m_level;
};


//	End namespace OPS
}

#endif		// OPS_COMMON_CORE_XMLBUILDER_H_
