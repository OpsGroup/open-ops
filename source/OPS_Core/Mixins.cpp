#include "OPS_Core/Mixins.h"
#include "OPS_Core/Exceptions.h"
#include "OPS_Core/Strings.h"

namespace OPS
{

void TypeConvertibleMix::castToErrorHandler(const char* const typeName) const
{
	throw OPS::TypeCastError(Strings::format("Cannot cast from <%s*>@%p to <%s>.", 
		typeid(*this).name(), this, typeName));
}

}
