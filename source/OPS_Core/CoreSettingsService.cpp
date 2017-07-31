#include "CoreSettingsService.h"

#include "OPS_Core/Helpers.h"

namespace OPS
{

namespace Core
{

CoreSettingsService::CoreSettingsService()
{
}

CoreSettingsService::~CoreSettingsService()
{
}

int CoreSettingsService::getAsInt(const std::string& propertyName) const
{
	const VariantValue value = m_variantMap[propertyName];

	OPS_ASSERT(value.type == typeInt);

	return value.intValue;
}

std::string CoreSettingsService::getAsString(const std::string& propertyName) const
{
	const VariantValue value = m_variantMap[propertyName];

	OPS_ASSERT(value.type == typeString);

	return value.stringValue;
}

void CoreSettingsService::setInt(const std::string& propertyName, int value)
{
	m_variantMap[propertyName] = VariantValue(value);
}

void CoreSettingsService::setString(const std::string& propertyName,
	const std::string& value)
{
	m_variantMap[propertyName] = VariantValue(value);
}

}

}
