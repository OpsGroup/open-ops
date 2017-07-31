#include "OPS_Core/ServiceLocator.h"

namespace OPS
{
	namespace Core
	{
		ServiceLocator* ServiceLocator::m_instance = 0;

		void ServiceLocator::initialize()
		{
			OPS_ASSERT(m_instance == 0);

			m_instance = new ServiceLocator();
		}

		void ServiceLocator::terminate()
		{
			OPS_ASSERT(m_instance != 0);

			delete m_instance;
			m_instance = 0;
		}

		ServiceLocator& ServiceLocator::instance()
		{
			OPS_ASSERT(m_instance != 0);

			return *m_instance;
		}

        ServiceLocatorBase::~ServiceLocatorBase()
		{
			OPS_ASSERT(m_serviceStorages.size() == 0);
		}

        void ServiceLocatorBase::clearServices()
        {
            ServiceStorageMap::iterator itStorage = m_serviceStorages.begin();
            for(; itStorage != m_serviceStorages.end(); ++itStorage)
                delete itStorage->second;
            m_serviceStorages.clear();
        }
	}
}
