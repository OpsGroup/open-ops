#include "OPS_Core/Core.h"

#include "OPS_Core/ServiceLocator.h"

#include "CoreSettingsService.h"

#include <locale>

namespace OPS
{
	namespace Core
	{
		Core::Core()
		{
			initialize();
		}

		Core::~Core()
		{
			terminate();
		}

		void Core::initialize()
		{
			std::locale::global(std::locale(""));

			ServiceLocator::initialize();

			// install core services
			ServiceLocator::instance().addService<ICoreSettingsService>(
				new CoreSettingsService());
		}

		void Core::terminate()
		{
			// uninstall core services
			ServiceLocator::instance().deleteService<ICoreSettingsService>();

			ServiceLocator::terminate();
		}
	}
}
