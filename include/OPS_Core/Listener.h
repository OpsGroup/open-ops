#ifndef OPS_COMMON_CORE_LISTENER_H_
#define OPS_COMMON_CORE_LISTENER_H_

#include <set>

/*
	Acyclic visitor pattern.
	Implementation taken from Andrei Alexandrescu "Modern C++ Design" book (Chapter 10)
*/

#define OPS_DEFINE_LISTENABLE()	\
	virtual void preNotify(BaseListener::ChangeType ct)	\
	{\
		BaseListener::propogatePreNotify(*this, ct);\
	}\
	virtual void postNotify(BaseListener::ChangeType ct)	\
	{\
		BaseListener::propogatePostNotify(*this, ct);\
	}


namespace OPS
{

template <class T>
	class Listener;

//	Listener
class BaseListener
{
public:
	enum ChangeType
	{
		CT_UNUSED = 0,
		CT_CREATE,
		CT_MODIFY,
		CT_DELETE
	};

	virtual ~BaseListener()
	{
	}

	static void add(BaseListener& listener);
	static void remove(BaseListener& listener);

	template<class T>
	static void propogatePreNotify(T& t, ChangeType ct)
	{
		for (ListenerSet::iterator it = g_listeners.begin(); it != g_listeners.end(); ++it)
		{
			if (Listener<T>* p = dynamic_cast<Listener<T>*>(*it))
			{
				p->preChange(t, ct);
			}
		}
	}

	template<class T>
	static void propogatePostNotify(T& t, ChangeType ct)
	{
		for (ListenerSet::iterator it = g_listeners.begin(); it != g_listeners.end(); ++it)
		{
			if (Listener<T>* p = dynamic_cast<Listener<T>*>(*it))
			{
				p->postChange(t, ct);
			}
		}
	}

private:
	typedef std::set<BaseListener*> ListenerSet;

	static ListenerSet g_listeners;
};

template <class T>
class Listener
{
public:
	virtual ~Listener() { }
	inline virtual void preChange(T&, BaseListener::ChangeType) {}
	inline virtual void postChange(T&, BaseListener::ChangeType) {}
};

//	Listenable part
class BaseListenable
{
public:
	virtual ~BaseListenable()
	{
	}

protected:                
	virtual void preNotify(BaseListener::ChangeType)
	{
	}

	virtual void postNotify(BaseListener::ChangeType)
	{
	}
};

}

#endif                      // OPS_COMMON_CORE_VISITOR_H_
