#if 0 
#include "OPS_Core/OPS_Core.h"
#include "Reprise/Reprise.h"

#include <iostream>

OPS_DEFINE_EXCEPTION_CLASS(StatementIteratorError, OPS::StateError)

template<typename Data>
class IntrusiveList;


template<typename Data>
class ConstIntrusiveIterator
{
protected:
	typedef Data* DataPtr;
	typedef const Data* ConstDataPtr;
	typedef Data& DataRef;
	typedef const Data& ConstDataRef;
	typedef IntrusiveList<Data> ListType;
	typedef const ListType* ConstListTypePtr;
	typedef ListType& ListTypeRef;
	typedef const IntrusiveList<Data>& ConstListTypeRef;
	typedef ConstIntrusiveIterator<Data> ConstIterator;
public:

	inline ConstIntrusiveIterator(const ConstIterator& iter) : m_list(iter.m_list), m_current(iter.m_current)
	{
	}

	inline bool isValid(void) const
	{
		return m_state != 0 && m_state != -1;
	}

	inline void goNext(void) const
	{
		if (m_state == 0)
			throw StatementIteratorError("Unexpected goNext() after the end of the list.");
		if (m_state == -1)
			m_current = m_list->m_first;
		else
			m_current = m_current->m_next;
	}

	inline void goPrevious(void) const
	{
		if (m_state == -1)
			throw StatementIteratorError("Unexpected goNext() after the end of the list.");
		if (m_state == 0)
		{
			if (m_list->isEmpty())
				throw StatementIteratorError("Unexpected goNext() in empty list.");
			else
				m_current = m_list->m_last;
		}
		else
		{
			m_current = m_current->m_next;
			if (m_current == 0)
				m_state = -1;
		}
	}

	inline void goFirst(void)
	{
		m_current = m_list->m_first;
	}

	inline void goLast(void)
	{
		m_current = m_list->m_last;
	}

	inline ConstDataRef operator*() const
	{
		if (!isValid())
			throw StatementIteratorError("Unexpected dereferencing of null iterator.");
		return *m_current;
	}

	inline ConstDataPtr operator->() const
	{
		if (!isValid())
			throw StatementIteratorError("Unexpected dereferencing of null iterator.");
		return m_current;
	}

	inline ConstIterator& operator=(const ConstIterator& iter)
	{
		m_list = iter.m_list;
		m_current = iter.m_current;
		return *this;
	}

	inline bool operator==(const ConstIterator& iter) const
	{
		return m_list == iter.m_list && m_current == iter.m_current;
	}

	inline bool operator!=(const ConstIterator& iter) const
	{
		return !(*this == iter);
	}

	//	Prefix
	inline ConstIterator& operator++()
	{
		goNext();
		return const_cast<ConstIterator&>(*this);
	}

	//	Postfix
	inline ConstIterator operator++(int)
	{
		Iterator temp(*this);
		goNext();
		return temp;
	}


protected:
	inline ConstIntrusiveIterator(ConstListTypeRef parentList, DataPtr initNode) : 
		m_list(&parentList), m_current(initNode)
	{
	}

//private:
	ConstListTypePtr m_list;
	union
	{
		mutable DataPtr m_current;
		mutable intptr_t m_state;
	};

	friend class IntrusiveList<Data>;
};

template<typename Data>
class IntrusiveIterator : public ConstIntrusiveIterator<Data>
{
	typedef IntrusiveIterator<Data> Iterator;
	typedef Data* DataPtr;
	typedef Data& DataRef;
public:

	inline IntrusiveIterator(const Iterator& iter) : 
		ConstIntrusiveIterator<Data>(iter)
	{
	}

	inline DataRef operator*()
	{
		if (!isValid())
			throw StatementIteratorError("Unexpected dereferencing of null iterator.");
		return *m_current;
	}

	inline DataPtr operator->()
	{
		if (!ConstIntrusiveIterator<Data>::isValid())
			throw StatementIteratorError("Unexpected dereferencing of null iterator.");
		return ConstIntrusiveIterator<Data>::m_current;
	}

	//	Prefix
	inline Iterator& operator++()
	{
		goNext();
		return const_cast<Iterator&>(*this);
	}

	//	Postfix
	inline Iterator operator++(int)
	{
		Iterator temp(*this);
		ConstIntrusiveIterator<Data>::goNext();
		return temp;
	}

protected:
	inline IntrusiveIterator(ConstIntrusiveIterator<Data>::ListTypeRef parentList, ConstIntrusiveIterator<Data>::DataPtr initNode) : 
		ConstIntrusiveIterator<Data>(parentList, initNode)
	{
	}


	friend class IntrusiveList<Data>;
};

template<typename Data>
class IntrusiveList
{
	typedef Data* DataPtr;
	typedef Data& DataRef;

public:
	typedef IntrusiveIterator<Data> Iterator;
	typedef ConstIntrusiveIterator<Data> ConstIterator;

	inline IntrusiveList(void) : m_first(0), m_last(0), m_count(0)
	{
	}

	inline ConstIterator getFirst(void) const
	{
		return ConstIterator(*this, m_first);
	}

	inline Iterator getFirst(void)
	{
		return Iterator(*this, m_first);
	}

	inline ConstIterator getLast(void) const
	{
		return ConstIterator(*this, m_last);
	}
	
	inline Iterator getLast(void)
	{
		return Iterator(*this, m_last);
	}

	inline bool isEmpty(void) const
	{
		return m_count == 0;
	}

	inline unsigned getCount(void) const
	{
		return m_count;
	}

	inline Iterator addAfter(Iterator node, DataPtr data)
	{
		if (node.m_list != this)
			throw StatementIteratorError("Unexpected Iterator for list.");
		if (m_count == 0)
		{
			if (node.isValid())
				throw StatementIteratorError("Unexpected Iterator state for list.");
			m_first = m_last = data;
			m_count = 1;
		}
		else
		{
			if (node.m_current == m_last)
			{
				data->m_next = 0;
				data->m_previous = m_last;
				m_last->m_next = data;
				m_last = data;
			}
			else
			{
				data->m_next = node.m_current->m_next;
				data->m_previous = node.m_current;
				node.m_current->m_next = data;
				data->m_next->m_previous = data;
			}
			m_count += 1;
		}
		return Iterator(*this, data);
	}

	inline Iterator addBefore(Iterator node, DataPtr data)
	{
		if (node.m_list != this)
			throw StatementIteratorError("Unexpected Iterator for list.");
		if (m_count == 0)
		{
			if (node.isValid())
				throw StatementIteratorError("Unexpected Iterator state for list.");
			m_first = m_last = data;
			m_count = 1;
		}
		else
		{
			if (node.m_current == m_first)
			{
				data->m_next = m_first;
				data->m_previous = 0;
				m_first->m_previous = data;
				m_first = data;
			}
			else
			{
				data->m_next = node.m_current;
				data->m_previous = node.m_current->m_previous;
				node.m_current->m_previous = data;
				data->m_previous->m_next = data;
			}
			m_count += 1;
		}
		return Iterator(*this, data);
	}

	inline Iterator addFirst(DataPtr data)
	{
		data->m_next = m_first;
		data->m_previous = 0;
		if (m_count == 0)
		{
			m_first = m_last = data;
			m_count = 1;
		}
		else
		{
			m_first->m_previous = data;
			m_first = data;
			m_count += 1;
		}
		return Iterator(*this, data);
	}
	
	inline Iterator addLast(DataPtr data)
	{
		data->m_next = 0;
		data->m_previous = m_last;
		if (m_count == 0)
		{
			m_first = m_last = data;
			m_count = 1;
		}
		else
		{
			m_last->m_next = data;
			m_last = data;
			m_count += 1;
		}
		return Iterator(*this, data);
	}

	inline void clear(void)
	{
		Iterator current = getFirst();
		for (;;)
		{
			Iterator temp = current++;
			DataPtr tempPtr = temp.m_current;
			remove(temp);
			delete tempPtr;
			if (!current.isValid())
				break;
		}
	}
	
	inline bool contains(DataPtr data)
	{
	}

	inline Iterator find(DataPtr data)
	{
	}
	
	inline void remove(Iterator& node)
	{
		if (node.m_list != this)
			throw StatementIteratorError("Unexpected Iterator for list.");
		if (!node.isValid())
			throw StatementIteratorError("Unexpected Iterator state for list.");
		if (m_count == 0)
		{
			throw StatementIteratorError("Noting to remove from list.");
		}
		else
		{
			if (m_count == 1)
			{
				if (node.m_current != m_last)
					throw StatementIteratorError("Unexpected Iterator for list.");
				node.m_current->m_next = node.m_current->m_previous = 0;
				node.m_state = 0;
				m_first = m_last = 0;
				m_count = 0;
			}
			else
			{
				if (node.m_current->m_next != 0)
					node.m_current->m_next->m_previous = node.m_current->m_previous;
				else
					m_last = node.m_current->m_previous;
				if (node.m_current->m_previous != 0)
					node.m_current->m_previous->m_next = node.m_current->m_next;
				else
					m_first = node.m_current->m_next;
				node.m_current->m_next = node.m_current->m_previous = 0;
				node.m_state = 0;
				m_count -= 1;
			}
		}
	}
	
	inline bool remove(DataPtr data)
	{
		Iterator current = getFirst();
		for (;;)
		{
			Iterator temp = current++;
			if (temp.m_current == data)
			{
				remove(temp);
				return true;
			}
			if (!current.isValid())
				break;
		}
		return false;
	}
	
	inline void removeFirst(void)
	{
	}
	
	inline void removeLast(void)
	{
	}

	
private:
	DataPtr m_first;
	DataPtr m_last;
	unsigned m_count;
	friend class ConstIntrusiveIterator<Data>;
};


void print_list(const IntrusiveList<OPS::Reprise::StatementBase>& list_to_print)
{
	IntrusiveList<OPS::Reprise::StatementBase>::ConstIterator iter = list_to_print.getFirst();
	for (; iter.isValid(); ++iter)
	{
		std::cout << iter->dumpState();
	}

}
/*
int main()
{
	using namespace std;
	typedef std::list<int> TList3;
	TList3 list3;
	TList3::const_iterator iter3 = list3.begin();

	typedef IntrusiveList<OPS::Reprise::StatementBase> TList;
	TList statements;

	statements.addAfter(statements.addFirst(new OPS::Reprise::EmptyStatement("s1")), new OPS::Reprise::EmptyStatement("s2"));
	print_list(statements);
	TList::Iterator iter = statements.getFirst();
	iter->getChild(0);
	statements.clear();
	
	return 0;
}
*/

#endif
