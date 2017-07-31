#include "Reprise/Utils.h"

//	Enter namespace
namespace OPS
{
namespace Reprise
{

RepriseManager* RepriseManager::s_instance = 0;

unsigned g_unique_number = 0;

std::string generateUniqueIndentifier(const std::string& partName)
{
	return Strings::format("__uni%u%hs", g_unique_number++, partName.c_str());
}


//	Exit namespace
}
}
