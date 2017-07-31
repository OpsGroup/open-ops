/*
    OPS_Stage/Config.h - OPS_Stage module, config header

*/

//  Multiple include guard start
#ifndef OPS_STAGE_CONFIG_H__
#define OPS_STAGE_CONFIG_H__

//  Standard includes
#include <string>
#include <vector>
#include <map>

//  OPS includes

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{
//  Constants and enums

//  Global classes
///		EnvironmentConfig class
struct EnvironmentConfig
{
    typedef std::list<std::string> StringListType;

	StringListType ClangIncludePathList;
	StringListType F2003IncludePathList;

	std::wstring ConfigFilePath;

	EnvironmentConfig(void);
	explicit EnvironmentConfig(const wchar_t* filePath);

	void load(const wchar_t* filePath);

};


//  Global functions

//  Exit namespace

}
}

//  Multiple include guard end
#endif 						//	OPS_STAGE_CONFIG_H__
