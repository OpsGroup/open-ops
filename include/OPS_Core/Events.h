//  Multiple include guard start
#ifndef OPS_COMMON_CORE_EVENTS_H_
#define OPS_COMMON_CORE_EVENTS_H_

//  Standard includes
#include <memory>
#include <vector>

//  Local includes
#include "OPS_Core/Environment.h"
#include "OPS_Core/Helpers.h"
//#include "OPS_Core/TypeConcepts.h"

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Events
{

//  Constants and enums

//  Global classes
//		Base event arguments
struct BaseEventArgs : public OPS::TypeConvertibleMix
{
};

//		Base event invoke adapter
class BaseEventInvokeAdapter : virtual public OPS::ClonableMix<BaseEventInvokeAdapter>
{
public:
//	Constructors/destructor
	virtual ~BaseEventInvokeAdapter(void)
	{
	}

//	Methods
    virtual void invoke(BaseEventArgs& args) const = 0;
	virtual void invoke(const BaseEventArgs& args) const = 0;
};

//		Event invoke adapter
template <class TargetType, class EventArgsType>
class EventInvokeAdapter : public BaseEventInvokeAdapter
{
public:
//	Classes
    typedef void (TargetType::*method_type)(EventArgsType& args);

//	Constructors/destructor
	inline EventInvokeAdapter(TargetType* const target, const method_type method) :
        m_target(target), m_method(method)
    {
    }

//	Methods
	virtual void invoke(BaseEventArgs& args) const
    {
		(m_target->*m_method)(args.template cast_to<EventArgsType>());
    }

    virtual void invoke(const BaseEventArgs& args) const
    {
//#if OPS_COMPILER_MSC_VER >= 1310
//#pragma warning(disable:4127)
//#endif
//		if (!TypeConcepts::is_const<EventArgsType>::state)
//			throw std::bad_cast();
//#if OPS_COMPILER_MSC_VER >= 1310
//#pragma warning(default:4127)
//#endif
		(m_target->*m_method)(const_cast<BaseEventArgs&>(args).template cast_to<EventArgsType>());
    }

//  Features
    OPS_DEFINE_CLONABLE_INTERFACE(EventInvokeAdapter)

private:
//	Members
    method_type m_method;
    TargetType* m_target;

//	Friendship
	friend class MultiEventHandler;
};

//		Event handler
class EventHandler
{
public:
//	Constructors/destructor
	inline EventHandler(void) :
		m_muted(false)
	{
	}

	inline EventHandler(const EventHandler& handler) :
		m_invoke_adapter(handler.m_invoke_adapter.get() == 0 ? 0 : handler.m_invoke_adapter->clone()),
		m_muted(false)
	{
	}

//	Methods
	inline EventHandler& operator=(const EventHandler& handler)
	{
		if (this != &handler)
		{
			if (handler.m_invoke_adapter.get() != 0)
				m_invoke_adapter.reset(handler.m_invoke_adapter->clone());
			else
				m_invoke_adapter.reset();
			m_muted = false;
		}
		return *this;
	}

	template <class TargetType, class EventArgsType>
    inline void set(TargetType* const target, void (TargetType::*method)(EventArgsType& args))
    {
        m_invoke_adapter.reset(new EventInvokeAdapter<TargetType, EventArgsType>(target, method));
    }

	inline void reset(void)
    {
        m_invoke_adapter.reset();
    }

	inline bool isAssigned(void) const
	{
		return m_invoke_adapter.get() != 0;
	}

    inline void invoke(BaseEventArgs& args) const
    {
		if (!m_muted && m_invoke_adapter.get() != 0)
			m_invoke_adapter->invoke(args);
    }

    inline void invoke(const BaseEventArgs& args) const
    {
		if (!m_muted && m_invoke_adapter.get() != 0)
	        m_invoke_adapter->invoke(args);
    }

	inline bool isMuted(void) const
	{
		return m_muted;
	}

	inline void mute(const bool muted)
	{
		m_muted = muted;
	}

private:
//	Members
    std::unique_ptr<BaseEventInvokeAdapter> m_invoke_adapter;
	bool m_muted;
};

//		Multi-event handler
class MultiEventHandler
{
public:
//	Constructors/destructor
	inline MultiEventHandler(void) :
		m_muted(false)
	{
	}

	inline MultiEventHandler(const MultiEventHandler& handler) :
		m_muted(false)
	{
		for (TAdaptersList::const_iterator it = handler.m_adapter_list.begin(); it != handler.m_adapter_list.end(); ++it)
		{
			m_adapter_list.push_back((*it)->clone());
		}
	}

	inline ~MultiEventHandler(void)
	{
		for (TAdaptersList::iterator it = m_adapter_list.begin(); it != m_adapter_list.end(); ++it)
		{
			delete *it;
		}
		m_adapter_list.clear();
	}

//	Methods
	inline MultiEventHandler& operator=(const MultiEventHandler& handler)
	{
		if (this != &handler)
		{
			m_adapter_list.clear();
			for (TAdaptersList::const_iterator it = handler.m_adapter_list.begin(); it != handler.m_adapter_list.end(); ++it)
			{
				m_adapter_list.push_back((*it)->clone());
			}
			m_muted = false;
		}
		return *this;
	}

	template <class TargetType, class EventArgsType>
    inline bool has(TargetType* const target, void (TargetType::*method)(EventArgsType& args))
    {
		typedef EventInvokeAdapter<TargetType, EventArgsType> CurrentAdapterType;

		for (TAdaptersList::const_iterator it = m_adapter_list.begin(); it != m_adapter_list.end(); ++it)
		{
			CurrentAdapterType* adapter = dynamic_cast<CurrentAdapterType*>(*it);
			OPS_ASSERT(adapter != 0)
			if (adapter->m_target == target && adapter->m_method == method)
			{
				return true;
			}
		}
		return false;
    }

	template <class TargetType, class EventArgsType>
    inline void add(TargetType* const target, void (TargetType::*method)(EventArgsType& args))
    {
		if (has(target, method))
		{
			throw OPS::RuntimeError("Event handler already added.");
		}
		m_adapter_list.push_back(new EventInvokeAdapter<TargetType, EventArgsType>(target, method));
    }

	template <class TargetType, class EventArgsType>
	inline void remove(TargetType* const target, void (TargetType::*method)(EventArgsType& args))
    {
		typedef EventInvokeAdapter<TargetType, EventArgsType> CurrentAdapterType;
		for (TAdaptersList::iterator it = m_adapter_list.begin(); it != m_adapter_list.end(); ++it)
		{
			CurrentAdapterType* adapter = dynamic_cast<CurrentAdapterType*>(*it);
			OPS_ASSERT(adapter != 0)
			if (adapter->m_target == target && adapter->m_method == method)
			{
				delete *it;
				m_adapter_list.erase(it);
				return;
			}
		}
		throw OPS::RuntimeError("Event handler not present.");
    }

    inline void invoke(BaseEventArgs& args) const
    {
		if (!m_muted)
		{
			for (TAdaptersList::const_iterator it = m_adapter_list.begin(); it != m_adapter_list.end(); ++it)
			{
				(*it)->invoke(args);
			}
		}
    }

    inline void invoke(const BaseEventArgs& args) const
    {
		if (!m_muted)
		{
			for (TAdaptersList::const_iterator it = m_adapter_list.begin(); it != m_adapter_list.end(); ++it)
			{
				(*it)->invoke(args);
			}
		}
    }

	inline bool isMuted(void) const
	{
		return m_muted;
	}

	inline void mute(const bool muted)
	{
		m_muted = muted;
	}

private:
//	Classes
    typedef std::vector<BaseEventInvokeAdapter*> TAdaptersList;

//	Members
    TAdaptersList m_adapter_list;
	bool m_muted;
};

//		Event handler muter
template <class EventHandler>
class EventHandlerMuter : public OPS::NonCopyableMix
{
public:
//	Constructors/destructor
	inline EventHandlerMuter(EventHandler& handler) :
		m_handler(handler), m_muted(handler.isMuted())
	{
		m_handler.mute(true);
	}

	inline ~EventHandlerMuter(void)
	{
		m_handler.mute(m_muted);
	}

private:
//	Members
	EventHandler& m_handler;
	bool m_muted;
};

//  Global functions

//  Exit namespace
}
}

//  Multiple include guard end
#endif	// OPS_COMMON_CORE_EVENTS_H_
