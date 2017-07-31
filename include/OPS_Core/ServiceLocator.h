#ifndef _SERVICE_LOCATOR_H_INCLUDED_
#define _SERVICE_LOCATOR_H_INCLUDED_

#include "OPS_Core/Helpers.h"

#include <map>
#include <set>
#include <string>
#include <typeinfo>

namespace OPS
{
	namespace Core
	{
        class ServiceLocatorBase
		{
		public:

            typedef std::string ServiceId;

            template<class IService>
            static ServiceId serviceId()
            {
                return typeid(IService).name();
            }

			template<class IService, class ServiceImpl>
			inline void addService(ServiceImpl* serviceImpl)
			{
				OPS_ASSERT(serviceImpl != 0);
				OPS_ASSERT(!serviceExists<IService>());

				m_serviceStorages[serviceId<IService>()] =
					new ServiceImplementationWrapper<IService, ServiceImpl>(serviceImpl);
			}

			template<class IService>
			inline void deleteService()
			{
				OPS_ASSERT(serviceExists<IService>());

				delete m_serviceStorages[serviceId<IService>()];
				m_serviceStorages.erase(serviceId<IService>());
			}

            inline bool serviceExists(ServiceId serviceId) const
            {
                typedef ServiceStorageMap::const_iterator ServiceStorageMapConstIter;

                ServiceStorageMapConstIter serviceStorageIt =
                    m_serviceStorages.find(serviceId);

                if (serviceStorageIt != m_serviceStorages.end())
                {
                    OPS_ASSERT(m_serviceStorages.count(serviceId) == 1);
                    OPS_ASSERT(serviceStorageIt->second != 0);

                    return true;
                }

                return false;
            }

			template<class IService>
			inline bool serviceExists() const
			{
                return serviceExists(serviceId<IService>());
			}

			template<class IService>
			inline IService& getService() const
			{				
				OPS_ASSERT(serviceExists<IService>());

				return static_cast<ServiceInterfaceWrapper<IService>*>(
					m_serviceStorages[serviceId<IService>()])->getService();
			}

		private:

			class AbstractServiceStorage
			{
			public:

				virtual ~AbstractServiceStorage() {}
			};

            typedef std::map<ServiceId, AbstractServiceStorage*> ServiceStorageMap;

			template<class IService>
			class ServiceInterfaceWrapper
				: public AbstractServiceStorage
			{
			public:

				virtual IService& getService() const = 0;
			};

			template<class IService, class ServiceImpl>
			class ServiceImplementationWrapper
				: public ServiceInterfaceWrapper<IService>
			{
			public:

				inline explicit ServiceImplementationWrapper(ServiceImpl* serviceImpl)
					: m_serviceImpl(serviceImpl)
				{
					OPS_ASSERT(m_serviceImpl != 0);
				}

				virtual ~ServiceImplementationWrapper()
				{
					OPS_ASSERT(m_serviceImpl != 0);

					delete m_serviceImpl;
					m_serviceImpl = 0;
				}

				virtual IService& getService() const
				{
					OPS_ASSERT(m_serviceImpl != 0);

					return *m_serviceImpl;
				}

			private:

				ServiceImpl* m_serviceImpl;
			};

        protected:
            inline ServiceLocatorBase() {}
            ~ServiceLocatorBase();

            void clearServices();

		private:


            ServiceLocatorBase(const ServiceLocatorBase& );
            ServiceLocatorBase& operator =(const ServiceLocatorBase& );

		private:

			mutable ServiceStorageMap m_serviceStorages;

		};

        class ServiceUsage
        {
        public:
            typedef std::set<ServiceLocatorBase::ServiceId> ServiceList;

            template<class IService>
                inline ServiceUsage& addRequired()
            {
                 m_requiredServices.insert(ServiceLocatorBase::serviceId<IService>());
                 return *this;
            }

            const ServiceList& getRequired() const { return m_requiredServices; }

        private:
            ServiceList m_requiredServices;
        };

        class ServiceLocator : public ServiceLocatorBase
        {
        public:

            static void initialize();
            static void terminate();

            static ServiceLocator& instance();
        private:

            static ServiceLocator* m_instance;
        };
	}
}

#endif // _SERVICE_LOCATOR_H_INCLUDED_
