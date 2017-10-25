#ifndef CONVERT_LESSEQUAL_TO_LESS__
#define CONVERT_LESSEQUAL_TO_LESS__

#include "Reprise/Reprise.h"


namespace OPS
{
namespace Transforms
{

bool canConvertLessEqualToLess(OPS::Reprise::ForStatement& forStat);

OPS::Reprise::ForStatement&  convertLessEqualToLess(OPS::Reprise::ForStatement& forStat);

}
}
#endif
