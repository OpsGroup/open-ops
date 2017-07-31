#ifndef OPS_STAGE_UTILS_H__
#define OPS_STAGE_UTILS_H__

#include <string>
#include <vector>
#include <map>

namespace OPS
{
namespace Stage
{

std::wstring getStageRoot(void);

void setStageRoot(const wchar_t* rootPath);


std::wstring getStageConfigsRoot(void);

std::wstring getStageSamplesRoot(void);

}
}

#endif 						//	OPS_STAGE_UTILS_H__
