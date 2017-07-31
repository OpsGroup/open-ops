#include "OPS_Core/Listener.h"
#include "OPS_Core/Helpers.h"

namespace OPS
{
	BaseListener::ListenerSet BaseListener::g_listeners;

	void BaseListener::add(BaseListener& listener)
	{
		g_listeners.insert(&listener);
	}
	
	void BaseListener::remove(BaseListener& listener)
	{
		g_listeners.erase(&listener);
	}

	
	//template<class T>
	//void BaseListener::propogatePostNotify(T&, ChangeType)
	//{
	//}

}