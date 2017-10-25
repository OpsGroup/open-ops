#pragma once

#include <map>
#include "Reprise/Common.h"

namespace OPS
{
namespace Shared
{
	typedef std::map<const Reprise::RepriseBase*, Reprise::RepriseBase*>	RepriseReferenceMap;

	/** Функция позволяет для двух одинаковых деревьев найти
	   соответствующие друг другу объекты в них.
	   \param left, right - одинаковые деревья
	   \param refMap - map: ключи - для которых нужно найти соответсвие,
							значения - соответсвия
	**/
	void mapReferences(const Reprise::RepriseBase& left,
					   Reprise::RepriseBase& right,
					   RepriseReferenceMap& refMap);

	/** Функция делает то же самое, что и mapReferences(), но для
	  одного объекта. Эта просто обертка над mapReferences() - не используйте
	  её если у вас несколько объектов.
	**/
	Reprise::RepriseBase* mapReference(const Reprise::RepriseBase& left,
									   Reprise::RepriseBase& right,
									   const Reprise::RepriseBase& sourceObject);
}
}
