/*
    OPS_Stage/Config.cpp - OPS_Stage module, config implementation

*/

//  Standard includes
//  OPS includes
#include "OPS_Core/Serialization.h"

//  Local includes
#include "OPS_Stage/Config.h"

//  Namespaces using
using namespace std;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{

//  Constants and enums

//  Classes

//  Functions declaration
static void processClang(EnvironmentConfig& config, OPS::Serialization::Item& item);

//  Variables

//  Classes implementation

//  Global classes implementation
EnvironmentConfig::EnvironmentConfig(void)
{
}

EnvironmentConfig::EnvironmentConfig(const wchar_t* filePath)
{
	load(filePath);
}

void EnvironmentConfig::load(const wchar_t* filePath)
{
    using namespace OPS::Serialization;

    Schema envSchema(L"Environment", Schema::MODE_REQUIRED_UNIQUE);
    envSchema.addSchema(L"Clang", Schema::MODE_REQUIRED_UNIQUE)
        .addSchema(L"Headers", Schema::MODE_REQUIRED_UNIQUE)
        .addSchema(L"Item", Schema::MODE_NORMAL).addParameter(L"value", Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE);
    envSchema.addSchema(L"F2003", Schema::MODE_REQUIRED_UNIQUE)
        .addSchema(L"Headers", Schema::MODE_REQUIRED_UNIQUE)
        .addSchema(L"Item", Schema::MODE_NORMAL).addParameter(L"value", Parameter::PARAMETER_TYPE_STRING, ParameterInfo::MODE_REQUIRED_UNIQUE);

    OPS::Serialization::XMLSerializer environmentXml(envSchema);
    std::unique_ptr<OPS::Serialization::Item> env(environmentXml.read(filePath));
    processClang(*this, env->getUniqueItem(L"Clang"));
}

//  Functions implementation
void processClang(EnvironmentConfig& config, OPS::Serialization::Item& item)
{
    using namespace OPS::Serialization;

    Item& headers = item.getUniqueItem(L"Headers");

    for (Item::items_type::const_iterator includePath = headers.begin(); includePath != headers.end(); ++includePath)
    {
        config.ClangIncludePathList.push_back(OPS::Strings::narrow(includePath->getParameter(L"value").getString()));
    }
}

//  Exit namespace
}
}
