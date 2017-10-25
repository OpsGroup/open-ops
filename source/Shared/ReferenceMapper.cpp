#include "Shared/ReferenceMapper.h"
#include "Shared/ReprisePath.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Shared
{

void mapReferences(const Reprise::RepriseBase& left,
				   Reprise::RepriseBase& right,
				   RepriseReferenceMap& refMap)
{
	RepriseReferenceMap::iterator it = refMap.begin(), itEnd = refMap.end();

	for(;it != itEnd; ++it)
	{
		if (it->first != 0)
		{
			try
			{
				ReprisePath path = makePath(*it->first, &left);
				it->second = findByPath(right, path);
			}
			catch(InvalidReprisePath&)
			{
				// соответствующий узел не найден
				it->second = 0;
			}
		}
		else
		{
			it->second = 0;
		}
	}
}

RepriseBase* mapReference(const Reprise::RepriseBase& left,
						  Reprise::RepriseBase& right,
						  const Reprise::RepriseBase& sourceObject)
{
	RepriseReferenceMap refMap;
	refMap[&sourceObject] = 0;
	mapReferences(left, right, refMap);

	return refMap[&sourceObject];
}

}
}
