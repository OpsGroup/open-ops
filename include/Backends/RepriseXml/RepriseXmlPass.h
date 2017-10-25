#ifndef REPRISEXMLPASS_H
#define REPRISEXMLPASS_H

//  Standard includes

//  OPS includes

//  Local includes
#include "OPS_Stage/Passes.h"

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{

//  Constants and enums

//  Global classes
class RepriseXmlSerializer : public SimpleSerializerBase
{
public:
    explicit RepriseXmlSerializer(const std::string& fileName = std::string());
    std::string getName() const;

protected:
    void serialize(Reprise::TranslationUnit &unit, std::ostream &os);
    std::string generateFileName(Reprise::TranslationUnit &unit);
};

//  Global functions

//  Exit namespace
}
}

#endif // REPRISEXMLPASS_H
