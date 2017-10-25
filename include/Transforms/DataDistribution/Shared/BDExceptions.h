#ifndef BD_EXCEPTIONS_H
#define BD_EXCEPTIONS_H

#include <string>
#include "OPS_Core/Exceptions.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

OPS_DEFINE_EXCEPTION_CLASS(BDException, OPS::RuntimeError)

}
}
}

#endif
