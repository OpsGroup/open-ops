#ifndef _CORE_SETTINGS_SERVICE_H_INCLUDED_
#define _CORE_SETTINGS_SERVICE_H_INCLUDED_

#include "OPS_Core/Interfaces/ICoreSettingsService.h"

#include <map>
#include <string>

namespace OPS
{

namespace Core
{

class CoreSettingsService
	: public ICoreSettingsService
{
public:
	CoreSettingsService();
	~CoreSettingsService();

	virtual int getAsInt(const std::string& propertyName) const;
	virtual std::string getAsString(const std::string& propertyName) const;

	virtual void setInt(const std::string& propertyName, int value);
	virtual void setString(const std::string& propertyName, const std::string& value);

private:
	enum Type
	{
		typeUndefined = 0,
		typeInt,
		typeString
	};

	struct VariantValue
	{
		VariantValue()
			: type(typeUndefined)
		{
		}

		explicit VariantValue(int value)
			: type(typeInt)
			, intValue(value)
		{
		}

		explicit VariantValue(const std::string& value)
			: type(typeString)
			, stringValue(value)
		{
		}

		Type type;
		union
		{
			int intValue;
		};
		std::string stringValue;
	};

	typedef std::map<std::string, VariantValue> VariantMap;

private:
	mutable VariantMap m_variantMap;
};

}

}

#endif // _CORE_SETTINGS_SERVICE_H_INCLUDED_
