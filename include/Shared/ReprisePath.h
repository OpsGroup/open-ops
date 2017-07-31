#pragma once

#include <list>
#include <string>
#include "Reprise/Common.h"

namespace OPS
{
namespace Shared
{
	typedef std::list<int> ReprisePath;

	OPS_DEFINE_EXCEPTION_CLASS(InvalidReprisePath, OPS::RuntimeError)

	ReprisePath makePath(const OPS::Reprise::RepriseBase& obj, const OPS::Reprise::RepriseBase* root = 0);
	OPS::Reprise::RepriseBase* findByPath(OPS::Reprise::RepriseBase& root, ReprisePath path);

	template<typename _Ty>
		_Ty* findByPathEx(OPS::Reprise::RepriseBase& root, const ReprisePath& path)
	{
		OPS::Reprise::RepriseBase* node = findByPath(root, path);
		if (node != 0)
			return node->cast_ptr<_Ty>();
		else
			return 0;
	}

	ReprisePath makePathFromString(const std::string& path);
	std::string makeStringFromPath(const ReprisePath& path);

	std::ostream& operator<<(std::ostream& os, const ReprisePath& path);
}
}
