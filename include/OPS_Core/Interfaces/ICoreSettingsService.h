#ifndef _I_CORE_SETTINGS_SERVICE_H_INCLUDED_
#define _I_CORE_SETTINGS_SERVICE_H_INCLUDED_

#include "OPS_Core/ServiceLocator.h"

#include <string>

namespace OPS
{

namespace Core
{

class ICoreSettingsService
{
public:
	virtual int getAsInt(const std::string& propertyName) const = 0;
	virtual std::string getAsString(const std::string& propertyName) const = 0;

	virtual void setInt(const std::string& propertyName, int value) = 0;
	virtual void setString(const std::string& propertyName, const std::string& value) = 0;

protected:
	virtual inline ~ICoreSettingsService() {}
};

inline ICoreSettingsService& coreSettings()
{
	return ServiceLocator::instance().getService<ICoreSettingsService>();
}

}

}

#endif // _I_CORE_SETTINGS_SERVICE_H_INCLUDED_
