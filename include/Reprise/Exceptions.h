#ifndef OPS_IR_REPRISE_EXCEPTIONS_H_INCLUDED__
#define OPS_IR_REPRISE_EXCEPTIONS_H_INCLUDED__

#include "OPS_Core/Exceptions.h"

namespace OPS
{
namespace Reprise
{
///	RepriseError exception
OPS_DEFINE_EXCEPTION_CLASS(RepriseError, RuntimeError)

///	UnexpectedChildError exception
OPS_DEFINE_EXCEPTION_CLASS(UnexpectedChildError, RepriseError)

///	NotEmplementedError exception
OPS_DEFINE_EXCEPTION_CLASS(NotEmplementedError, RepriseError)
}
}

#endif                      // OPS_IR_REPRISE_EXCEPTIONS_H_INCLUDED__
