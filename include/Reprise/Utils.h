#ifndef OPS_IR_REPRISE_UTILS_H_INCLUDED__
#define OPS_IR_REPRISE_UTILS_H_INCLUDED__

#include <vector>
#include <string>

#include "OPS_Core/Helpers.h"
#include "OPS_Core/Console.h"
#include "Reprise/Exceptions.h"

namespace OPS
{
namespace Reprise
{

class IntrusivePointerBase;

class WeakCounter
{
	static const unsigned EXPIRE_FLAG = 0x10000000U;
	static const unsigned EXPIRE_MASK = ~EXPIRE_FLAG;
public:
	WeakCounter() : m_weakCount(0)
	{
	}

	void addCount()
	{
		if (m_weakCount == 0)
		{
			m_weakCount = new unsigned();
			*m_weakCount = 1;
		}
		else
		{
			OPS_ASSERT(!expired());
			*m_weakCount = ((*m_weakCount & EXPIRE_MASK) + 1) | (*m_weakCount & EXPIRE_FLAG);
		}
	}

	unsigned getCount() const
	{
		if (m_weakCount == 0)
		{
			return 0;
		}
		return *m_weakCount & EXPIRE_MASK;
	}

	void reset()
	{
		decreaseCount();
        m_weakCount = 0;
	}

	void setExpired()
	{
		if (m_weakCount != 0)
		{
			*m_weakCount |= EXPIRE_FLAG;
		}
	}

	bool expired() const
	{
		if (m_weakCount != 0)
		{
			return (*m_weakCount & EXPIRE_FLAG) == EXPIRE_FLAG;
		}
		return true;
	}

	void decreaseCount()
	{
		if (m_weakCount == 0)
			return;
		if ((*m_weakCount & EXPIRE_MASK) > 1)
		{
			*m_weakCount = ((*m_weakCount & EXPIRE_MASK) - 1) | (*m_weakCount & EXPIRE_FLAG);
		}
		else 
		{
			delete m_weakCount;
			m_weakCount = 0;
		}
	}

private:
	unsigned* m_weakCount;
};

class IntrusivePointerBase
{
public:
	inline IntrusivePointerBase(void) : m_refCount(0)
	{
	}

	inline IntrusivePointerBase(const IntrusivePointerBase&) : m_refCount(0)
	{
	}

	inline void private_addRef(void)
	{
		m_weakCounter.addCount();
		++m_refCount;
	}
	
	inline bool private_removeRef(void)
	{
		m_weakCounter.decreaseCount();
		if (--m_refCount == 0)
		{
			m_weakCounter.setExpired();
			return true;
		}
		return false;
	}

	inline void private_release(void)
	{
		m_weakCounter.decreaseCount();
		--m_refCount;
	}

	inline unsigned private_getRefCount(void) const
	{
		return m_refCount;
	}

	inline WeakCounter& private_addWeakCounter(void)
	{
		m_weakCounter.addCount();
		return m_weakCounter;
	}

private:
	unsigned m_refCount;
	WeakCounter m_weakCounter;
};


///	Reprise runtime manager
class RepriseManager
{
public:
	inline static RepriseManager& instance(void)
	{
		if (s_instance == 0)
			s_instance = new RepriseManager();
		return *s_instance;
	}

	inline void addError(const std::string& message)
	{
		OPS::getOutputConsole("Reprise").log(OPS::Console::LEVEL_ERROR, message);
	}

private:
	inline RepriseManager(void)
	{
	}

	static RepriseManager* s_instance;
};

///	Reprise smart pointer
template<typename Data>
class ReprisePtr
{
	typedef ReprisePtr<Data> CopyArg;
	typedef Data* DataPtr;
	typedef Data& DataRef;
	typedef const Data* ConstDataPtr;
	typedef const Data& ConstDataRef;

public:
	ReprisePtr(void) : m_data(0)
	{
	}

	explicit ReprisePtr(const DataPtr dataPtr) : m_data(0)
	{
		if (dataPtr == 0)
			throw OPS::Reprise::RepriseError("Unexpected creation of null Reprise object.");
		m_data = dataPtr;
		m_data->private_addRef();
	}

	ReprisePtr(const CopyArg& reprisePtr) : m_data(0)
	{
		if (reprisePtr.m_data != 0)
		{
			m_data = reprisePtr.m_data;
			m_data->private_addRef();
		}
		//else
		//	throw OPS::Reprise::RepriseError("Unexpected creation of null Reprise object.");
	}

	template<typename RelData>
	ReprisePtr(const ReprisePtr<RelData>& reprisePtr) : m_data(0)
	{
		if (reprisePtr.get() != 0)
		{
			DataPtr ptr = dynamic_cast<DataPtr>(reprisePtr.get());
			if (ptr != 0)
			{
				m_data = ptr;
				m_data->private_addRef();
			}
			else
				throw OPS::Reprise::RepriseError("Unexpected creation of Reprise object.");
		}
	}

	inline ~ReprisePtr(void)
	{
		if (m_data != 0)
		{
			if (m_data->private_removeRef())
				delete m_data;
			m_data = 0;
		}
	}

	void reset(const DataPtr& dataPtr)
	{
		if (m_data != 0)
			if (m_data->private_removeRef())
				delete m_data;
		m_data = dataPtr;
		if (m_data != 0)
			m_data->private_addRef();
	}

	void release(DataPtr& dataPtr)
	{
		if (m_data == 0)
			throw OPS::Reprise::RepriseError("Unexpected release of Reprise object.");
		m_data->private_release();
		dataPtr = m_data;
		m_data = 0;
	}

	DataPtr release(void)
	{
		DataPtr temp;
		release(temp);
		return temp;
	}

	inline DataPtr get(void) const
	{
		return m_data;
	}

	inline DataPtr operator->() 
	{
		if (m_data == 0)
			throw OPS::Reprise::RepriseError("Unexpected dereference of Reprise object.");
		return m_data;
	}

	inline ConstDataPtr operator->() const
	{
		if (m_data == 0)
			throw OPS::Reprise::RepriseError("Unexpected dereference of Reprise object.");
		return m_data;
	}

	inline DataRef operator*() 
	{
		if (m_data == 0)
			throw OPS::Reprise::RepriseError("Unexpected dereference of Reprise object.");
		return *m_data;
	}

	inline ConstDataRef operator*() const
	{
		if (m_data == 0)
			throw OPS::Reprise::RepriseError("Unexpected dereference of Reprise object.");
		return *m_data;
	}

	inline ReprisePtr<Data> operator=(const ReprisePtr<Data>& trackPtr)
	{
		ReprisePtr<Data> temp(trackPtr);
		temp.swap(*this);
		return *this;
	}

	template<typename RelData>
	inline ReprisePtr<Data> operator=(const ReprisePtr<RelData>& trackPtr)
	{
		ReprisePtr<Data> temp(trackPtr);
		temp.swap(*this);
		return *this;
	}

	void swap(ReprisePtr& reprisePtr)
	{
		std::swap(m_data, reprisePtr.m_data);
	}

	inline bool operator==(const ReprisePtr<Data>& trackPtr) const
	{
		return m_data == trackPtr.m_data;
	}

	inline bool operator!=(const ReprisePtr<Data>& trackPtr) const
	{
		return !operator==(trackPtr);
	}

	template<typename RelData>
	inline bool operator==(const ReprisePtr<RelData>& trackPtr) const
	{
		return m_data == trackPtr.get();
	}

	template<typename RelData>
	inline bool operator!=(const ReprisePtr<RelData>& trackPtr) const
	{
		return !operator==(trackPtr);
	}

	inline bool operator<(const ReprisePtr<Data>& rhs) const
	{ 
		return m_data < rhs.m_data; 
	}

protected:
	DataPtr m_data;
};

template<typename Data>
class RepriseWeakPtr
{
public:
	RepriseWeakPtr() : m_data(0)
	{
	}

	RepriseWeakPtr(Data* const data) : m_data(0)
	{
		if (data == 0)
		{
			throw OPS::Reprise::RepriseError("Unexpected creation of null Reprise weak object.");
		}
		m_data = data;
		m_counter = data->private_addWeakCounter();
	}

    template<class RelData> 
	RepriseWeakPtr(RelData* const data) : m_data(0)
	{
		if (data == 0)
		{
			throw OPS::Reprise::RepriseError("Unexpected creation of null Reprise weak object.");
		}
		m_data = data;
		m_counter = data->private_addWeakCounter();
	}

    template<class RelData> 
	RepriseWeakPtr(const ReprisePtr<RelData>& r) : m_data(0)
	{
		if (r.get() != 0)
		{
			m_data = r.get();
			m_counter = r->private_addWeakCounter();
		}
	}

	RepriseWeakPtr(const RepriseWeakPtr& r) : m_data(0)
	{
		if (!r.expired())
		{
			m_data = r.m_data;
			m_counter = r.m_counter;
			m_counter.addCount();
		}
	}

	template<class RelData> 
	RepriseWeakPtr(const RepriseWeakPtr<RelData>& r) : m_data(0)
	{
		if (!r.expired())
		{
			m_data = r.m_data;
			m_counter = r.m_counter;
			m_counter.addCount();
		}
	}

	~RepriseWeakPtr()
	{
		m_counter.decreaseCount();
	}

	void reset(Data* const data)
	{
		m_counter.reset();
		if (data == 0)
		{
			m_data = 0;
		}
		else
		{
			m_data = data;
			m_counter = data->private_addWeakCounter();
		}
	}

	RepriseWeakPtr& operator=(const RepriseWeakPtr& r)
	{
		m_data = 0;
		m_counter.reset();
		if (!r.expired())
		{
			m_data = r.m_data;
			m_counter = r.m_counter;
			m_counter.addCount();
		}
		return *this;
	}

	template<class RelData> 
	RepriseWeakPtr& operator=(const RepriseWeakPtr<RelData>& r)
	{
		m_data = 0;
		m_counter.reset();
		if (!r.expired())
		{
			m_data = r.m_data;
			m_counter = r.m_counter;
			m_counter.addCount();
		}
		return *this;
	}

	template<class RelData>
	RepriseWeakPtr& operator=(const ReprisePtr<RelData>& r)
	{
		m_data = 0;
		m_counter.reset();
		if (r.get() != 0)
		{
			m_data = r.get();
			m_counter = r.get()->private_addWeakCounter();
		}
		return *this;
	}

	inline Data& operator*() 
	{
		if (expired())
			throw OPS::Reprise::RepriseError("Unexpected dereference of expired Reprise object.");
		return *m_data;
	}

	inline const Data& operator*() const
	{
		if (expired())
			throw OPS::Reprise::RepriseError("Unexpected dereference of expired Reprise object.");
		return *m_data;
	}

	inline Data* operator->() 
	{
		if (expired())
			throw OPS::Reprise::RepriseError("Unexpected dereference of expired Reprise object.");
		return m_data;
	}

	inline const Data* operator->() const
	{
		if (expired())
			throw OPS::Reprise::RepriseError("Unexpected dereference of expired Reprise object.");
		return m_data;
	}

	inline Data* get(void) const
	{
		if (expired())
		{
			return 0;
		}
		else
		{
			return m_data;
		}
	}

	unsigned use_count() const
	{
		return m_counter.getCount();
	}

	bool expired() const
	{
		return m_counter.expired();
	}

	ReprisePtr<Data> lock() const
	{
		if (!expired())
		{
			ReprisePtr<Data> result(m_data);
			return result;
		}
		else
		{
			return ReprisePtr<Data>();
		}
	}

	void reset()
	{
		m_data = 0;
		m_counter.reset();
	}

	void swap(RepriseWeakPtr<Data>& b)
	{
	}

private:
	Data* m_data;
	WeakCounter m_counter;
};


template<typename BaseType>
class RepriseList : OPS::NonCopyableMix
{
public:
	typedef std::vector<BaseType*> ListType;
	typedef typename ListType::iterator Iterator;
	typedef typename ListType::const_iterator ConstIterator;
	typedef typename ListType::reverse_iterator ReverseIterator;

	inline RepriseList(void)
	{
	}

	inline ~RepriseList(void)
	{
		clear();
	}

	inline BaseType& add(BaseType* const value)
	{
		value->private_addRef();
		m_list.push_back(value);
		return *m_list.back();
	}

	inline BaseType& insert(int index, BaseType* value)
	{
		value->private_addRef();
		return **m_list.insert(m_list.begin() + index, value);
	}

	inline BaseType* insert(const BaseType* const before, BaseType* value)
	{
		if (before == 0)
		{
			value->private_addRef();
			m_list.push_back(value);
			return m_list.back();
		}
		for (typename ListType::iterator it = m_list.begin(); it != m_list.end(); ++it)
		{
			if (*it == before)
			{
				value->private_addRef();
				return *m_list.insert(it, value);
			}
		}
		return 0;
	}

	inline void replace(Iterator oldValueIter, BaseType* newValue)
	{
		if ((*oldValueIter)->private_removeRef())
		{
			delete *oldValueIter;
		}
		newValue->private_addRef();
		*oldValueIter = newValue;
	}

	inline void clear(void)
	{
		for (ReverseIterator iter = m_list.rbegin(); iter != m_list.rend(); ++iter)
		{
			if ((*iter)->private_removeRef())
			{
				delete *iter;
			}
		}
		m_list.clear();
	}

	inline int size(void) const
	{
		return static_cast<int>(m_list.size());
	}

	inline const BaseType& operator[](int index) const
	{
		return *m_list[index];
	}

	inline BaseType& operator[](int index)
	{
		return *m_list[index];
	}

	inline Iterator begin(void)
	{
		return m_list.begin();
	}

	inline ConstIterator begin(void) const
	{
		return m_list.begin();
	}

	inline Iterator end(void)
	{
		return m_list.end();
	}

	inline ConstIterator end(void) const
	{
		return m_list.end();
	}

private:
	ListType m_list;
};

//	Global functions
template<typename BaseType>
void repriseDelete(BaseType*& object)
{
	if (object != 0)
	{
		delete object;
		object = 0;
	}
}

std::string generateUniqueIndentifier(const std::string& partName = "");

}
}
#endif
