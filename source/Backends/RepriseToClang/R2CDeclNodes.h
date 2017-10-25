/// R2CDeclNodes.h
///   Declaration for a map RepriseBase -> cland Decl.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created:  9.08.2013

#ifndef R2CDECLNODES_H__
#define R2CDECLNODES_H__

#include <map>

namespace clang
{
	class Decl;
}

namespace OPS
{
	namespace Reprise
	{
		class RepriseBase;
	}

	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				typedef
					std::map <const OPS::Reprise::RepriseBase*, clang::Decl*>
					R2CDeclNodes;

			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // R2CDECLNODES_H__

// End of File
