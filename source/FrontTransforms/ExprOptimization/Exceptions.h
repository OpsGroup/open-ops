#ifndef EXPRESSION_OPTIMIZATION_EXCEPTIONS_H
#define EXPRESSION_OPTIMIZATION_EXCEPTIONS_H

#include "OPS_Core/Exceptions.h"

namespace OPS
{
namespace ExpressionSimplifier
{

OPS_DEFINE_EXCEPTION_CLASS(ArgumentError, OPS::RuntimeError)

}
}

#endif 
