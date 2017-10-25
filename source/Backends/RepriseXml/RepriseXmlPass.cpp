//  Standard includes
#include <iostream>
#include <fstream>

//  OPS includes
#include "Backends/RepriseXml/RepriseXmlPass.h"
#include "Backends/RepriseXml/RepriseXml.h"

//  Local includes

//  Namespaces using
using namespace std;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{
//  Constants and enums
static const char* REPRISE_XML_SERIALIZER_NAME = "RepriseXmlSerializer";

//  Classes

//  Functions declaration

//  Variables

//  Classes implementation

//  Global classes implementation
RepriseXmlSerializer::RepriseXmlSerializer(const string &fileName)
    :SimpleSerializerBase(fileName)
{
}

std::string RepriseXmlSerializer::getName() const
{
    return REPRISE_XML_SERIALIZER_NAME;
}

void RepriseXmlSerializer::serialize(Reprise::TranslationUnit &unit, ostream &os)
{
    XmlBuilder xml(os);
    Backends::RepriseXml repXml(xml);
    unit.accept(repXml);
}

std::string RepriseXmlSerializer::generateFileName(Reprise::TranslationUnit &unit)
{
    return SimpleSerializerBase::generateFileName(unit) + ".xml";
}

//  Functions implementation

//  Exit namespace
}
}
