#ifndef OPS_IR_REPRISE_COLLECTIONS_H_INCLUDED__
#define OPS_IR_REPRISE_COLLECTIONS_H_INCLUDED__

#include "OPS_Core/Compiler.h"
#include "OPS_Core/Exceptions.h"

#if OPS_COMPILER_GCC_VER > 0
#include <stdint.h>
#endif

namespace OPS
{
namespace Reprise
{

template<typename NodeType>
class IntrusiveList;

template<typename NodeType>
class IntrusiveIteratorBase;

template<typename NodeType>
class IntrusiveNodeBase
{
private:
	friend class IntrusiveList<NodeType>;
	friend class IntrusiveIteratorBase<NodeType>;
	NodeType* m_next; 
	NodeType* m_prev; 

protected:
	inline IntrusiveNodeBase(void) : m_next(0), m_prev(0)
	{
	}
};

template<typename NodeType>
class ConstIntrusiveIterator;

template<typename NodeType>
class IntrusiveIterator;

template<typename NodeType>
class IntrusiveIteratorBase
{
public:
	typedef NodeType ListNodeType;
	typedef IntrusiveList<ListNodeType> ListType;

	bool isValid(void) const
	{
		return m_state != 0 && m_state != -1;
	}

	void goNext(void)
	{
		if (m_state == 0)
			throw StateError("Unexpected goNext() after the end of the list.");
		if (m_state == -1)
			m_current = m_list->m_first;
		else
			m_current = m_current->m_next;
	}

	void goPrevious(void)
	{
		if (m_state == -1)
			throw StateError("Unexpected goPrevious() before the start of the list.");
		if (m_state == 0)
		{
			if (m_list->isEmpty())
				throw StateError("Unexpected goPrevious() in empty list.");
			else
				m_current = m_list->m_last;
		}
		else
		{
			m_current = m_current->m_prev;
			if (m_current == 0)
				m_state = -1;
		}
	}

	void goFirst(void)
	{
		m_current = m_list->m_first;
	}

	void goLast(void)
	{
		m_current = m_list->m_last;
	}

	IntrusiveIteratorBase<NodeType>& operator=(const IntrusiveIteratorBase<NodeType>& iter)
	{
		m_list = iter.m_list;
		m_current = iter.m_current;
		return *this;
	}

	bool operator==(const IntrusiveIteratorBase<NodeType>& iter) const
	{
		return m_list == iter.m_list && m_current == iter.m_current;
	}

	bool operator!=(const IntrusiveIteratorBase<NodeType>& iter) const
	{
		return !(*this == iter);
	}

protected:
	IntrusiveIteratorBase() : m_list(0), m_current(0)
	{
	}

	IntrusiveIteratorBase(ListType& parentList, NodeType* initNode) : 
		m_list(&parentList), m_current(initNode)
	{
	}

	IntrusiveIteratorBase(const ListType& parentList, const NodeType* initNode) : 
		m_list(const_cast<ListType*>(&parentList)), m_current(const_cast<NodeType*>(initNode))
	{
	}

	IntrusiveIteratorBase(const IntrusiveIteratorBase& other) : 
		m_list(other.m_list), m_current(other.m_current)
	{
	}

	ListType* m_list;
	union
	{
		mutable NodeType* m_current;
		mutable intptr_t m_state;
	};

	friend class IntrusiveList<NodeType>;
};

template<typename NodeType>
class IntrusiveIterator : public IntrusiveIteratorBase<NodeType>
{
	friend class IntrusiveList<NodeType>;
public:
	typedef typename IntrusiveIteratorBase<NodeType>::ListNodeType ListNodeType;
	typedef IntrusiveIterator<ListNodeType> Iterator;
	typedef IntrusiveList<ListNodeType> ListType;

	IntrusiveIterator(const Iterator& iter) : 
		IntrusiveIteratorBase<NodeType>(iter)
	{
	}

	NodeType& operator*()
	{
		if (!IntrusiveIteratorBase<NodeType>::isValid())
			throw StateError("Unexpected dereferencing of null iterator.");
		return *IntrusiveIteratorBase<NodeType>::m_current;
	}

	NodeType* operator->()
	{
		if (!IntrusiveIteratorBase<NodeType>::isValid())
			throw StateError("Unexpected dereferencing of null iterator.");
		return IntrusiveIteratorBase<NodeType>::m_current;
	}

	//	Prefix
	Iterator& operator++()
	{
		IntrusiveIteratorBase<NodeType>::goNext();
		return *this;
	}

	//	Postfix
	Iterator operator++(int)
	{
		Iterator temp(*this);
		IntrusiveIteratorBase<NodeType>::goNext();
		return temp;
	}

	//	Prefix
	Iterator& operator--()
	{
		IntrusiveIteratorBase<NodeType>::goPrevious();
		return *this;
	}

	//	Postfix
	Iterator operator--(int)
	{
		Iterator temp(*this);
		IntrusiveIteratorBase<NodeType>::goPrevious();
		return temp;
	}
private:
	IntrusiveIterator(ListType& parentList, ListNodeType* initNode)
		: IntrusiveIteratorBase<NodeType>(parentList, initNode)
	{
	}

};

template<typename NodeType>
class ConstIntrusiveIterator : public IntrusiveIteratorBase<NodeType>
{
	friend class IntrusiveList<NodeType>;
public:
	typedef typename IntrusiveIteratorBase<NodeType>::ListNodeType ListNodeType;
	typedef ConstIntrusiveIterator<ListNodeType> ConstIterator;
	typedef IntrusiveIterator<ListNodeType> Iterator;
	typedef IntrusiveList<ListNodeType> ListType;

	ConstIntrusiveIterator(const ConstIterator& iter) : IntrusiveIteratorBase<NodeType>(iter)
	{
	}

	ConstIntrusiveIterator(const Iterator& iter) : IntrusiveIteratorBase<NodeType>(iter)
	{
	}

	inline const NodeType& operator*() const
	{
		if (!IntrusiveIteratorBase<NodeType>::isValid())
			throw StateError("Unexpected dereferencing of null iterator.");
		return *IntrusiveIteratorBase<NodeType>::m_current;
	}

	inline const NodeType* operator->() const
	{
		if (!IntrusiveIteratorBase<NodeType>::isValid())
			throw StateError("Unexpected dereferencing of null iterator.");
		return IntrusiveIteratorBase<NodeType>::m_current;
	}

	inline ConstIterator& operator=(const ConstIterator& iter)
	{
		IntrusiveIteratorBase<NodeType>::operator=(iter);
		return *this;
	}

	//	Prefix
	inline ConstIterator& operator++()
	{
		IntrusiveIteratorBase<NodeType>::goNext();
		return const_cast<ConstIterator&>(*this);
	}

	//	Postfix
	inline ConstIterator operator++(int)
	{
		ConstIterator temp(*this);
		IntrusiveIteratorBase<NodeType>::goNext();
		return temp;
	}

	//	Prefix
	inline ConstIterator& operator--()
	{
		IntrusiveIteratorBase<NodeType>::goPrevious();
		return const_cast<ConstIterator&>(*this);
	}

	//	Postfix
	inline ConstIterator operator--(int)
	{
		ConstIterator temp(*this);
		IntrusiveIteratorBase<NodeType>::goPrevious();
		return temp;
	}
private:
	ConstIntrusiveIterator(const ListType& parentList, const ListNodeType* initNode)
		: IntrusiveIteratorBase<NodeType>(parentList, initNode)
	{
	}

};

template<typename NodeType>
class IntrusiveList
{
	friend class IntrusiveIteratorBase<NodeType>;
public:
	typedef IntrusiveIterator<NodeType> Iterator;
	typedef ConstIntrusiveIterator<NodeType> ConstIterator;

	IntrusiveList(void) : m_first(0), m_last(0), m_count(0)
	{
	}

	~IntrusiveList(void)
	{
		clear();
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

	inline Iterator convertToIterator(NodeType* node)
	{
		return Iterator(*this, node);
	}

	inline ConstIterator convertToIterator(const NodeType* node) const
	{
		return ConstIterator(*this, node);
	}

	inline Iterator addAfter(Iterator iter, NodeType* node)
	{
		if (iter.m_list != this)
			throw ArgumentError("Unexpected Iterator for list.");
		checkNode(node);
		if (m_count == 0)
		{
			if (iter.isValid())
				throw ArgumentError("Unexpected Iterator state for list.");
			node->private_addRef();
			m_first = m_last = node;
			m_count = 1;
		}
		else
		{
			if (iter.m_state == -1)
			{
				return addFirst(node);
			}
			node->private_addRef();
			if (iter.m_current == m_last)
			{
				node->m_next = 0;
				node->m_prev = m_last;
				m_last->m_next = node;
				m_last = node;
			}
			else
			{
				node->m_next = iter.m_current->m_next;
				node->m_prev = iter.m_current;
				iter.m_current->m_next = node;
				node->m_next->m_prev = node;
			}
			m_count += 1;
		}
		return Iterator(*this, node);
	}

	inline Iterator addBefore(Iterator iter, NodeType* node)
	{
		if (iter.m_list != this)
			throw ArgumentError("Unexpected Iterator for list.");
		checkNode(node);
		if (m_count == 0)
		{
			if (iter.isValid())
				throw ArgumentError("Unexpected Iterator state for list.");
			node->private_addRef();
			m_first = m_last = node;
			m_count = 1;
		}
		else
		{
			if (iter.m_current == 0)
			{
				return addLast(node);
			}
			node->private_addRef();
			if (iter.m_current == m_first)
			{
				node->m_next = m_first;
				node->m_prev = 0;
				m_first->m_prev = node;
				m_first = node;
			}
			else
			{
				node->m_next = iter.m_current;
				node->m_prev = iter.m_current->m_prev;
				iter.m_current->m_prev = node;
				node->m_prev->m_next = node;
			}
			m_count += 1;
		}
		return Iterator(*this, node);
	}

	inline Iterator addFirst(NodeType* node)
	{
		checkNode(node);
		node->private_addRef();
		node->m_next = m_first;
		node->m_prev = 0;
		if (m_count == 0)
		{
			m_first = m_last = node;
			m_count = 1;
		}
		else
		{
			m_first->m_prev = node;
			m_first = node;
			m_count += 1;
		}
		return Iterator(*this, node);
	}
	
	inline Iterator addLast(NodeType* node)
	{
		checkNode(node);
		node->private_addRef();
		node->m_next = 0;
		node->m_prev = m_last;
		if (m_count == 0)
		{
			m_first = m_last = node;
			m_count = 1;
		}
		else
		{
			m_last->m_next = node;
			m_last = node;
			m_count += 1;
		}
		return Iterator(*this, node);
	}

	inline void clear(void)
	{
		if (m_count == 0)
			return;
		Iterator current = getFirst();
		for (;;)
		{
			Iterator temp = current++;
			remove(temp);
			if (!current.isValid())
				break;
		}
		m_first = m_last = 0;
		m_count = 0;
	}
	
	inline bool contains(const NodeType* node) const
	{
		for (ConstIterator current = getFirst(); current.isValid(); current.goNext())
		{
			if (node == current.m_current)
				return true;
		}
		return false;
	}

	inline ConstIterator find(const NodeType* node) const
	{
		for (ConstIterator current = getFirst(); current.isValid(); current.goNext())
		{
			if (node == current.m_current)
				return current;
		}
		return ConstIterator(*this, 0);
	}

	inline Iterator find(NodeType* node)
	{
		for (Iterator current = getFirst(); current.isValid(); current.goNext())
		{
			if (node == current.m_current)
				return current;
		}
		return Iterator(*this, 0);
	}

	inline void remove(Iterator& iter)
	{
		if (iter.m_list != this)
			throw ArgumentError("Unexpected Iterator for list.");
		if (!iter.isValid())
			throw ArgumentError("Unexpected Iterator state for list.");
		if (m_count == 0)
		{
			throw ArgumentError("Noting to remove from list.");
		}
		else
		{
			if (m_count == 1)
			{
				if (iter.m_current != m_last)
					throw ArgumentError("Unexpected Iterator for list.");
				if (iter.m_current->private_removeRef())
                    delete iter.m_current;
				iter.m_state = 0;
				m_first = m_last = 0;
				m_count = 0;
			}
			else
			{
				if (iter.m_current->m_next != 0)
					iter.m_current->m_next->m_prev = iter.m_current->m_prev;
				else
					m_last = iter.m_current->m_prev;
				if (iter.m_current->m_prev != 0)
					iter.m_current->m_prev->m_next = iter.m_current->m_next;
				else
					m_first = iter.m_current->m_next;
				if (iter.m_current->private_removeRef())
                    delete iter.m_current;
                else
                    iter.m_current->m_next = iter.m_current->m_prev = 0;
				iter.m_state = 0;
				m_count -= 1;
			}
		}
	}
	
	inline void remove(NodeType* node)
	{
		Iterator temp = convertToIterator(node);
		remove(temp);
	}
	
	inline void removeFirst(void)
	{
		if (m_count == 0)
		{
			throw ArgumentError("Noting to remove from list.");
		}
		else
		{
			if (m_count == 1)
			{
				if (m_first->private_removeRef())
					delete m_first;
				m_first = m_last = 0;
				m_count = 0;
			}
			else
			{
				m_first = m_first->m_next;
				if (m_first->m_prev->private_removeRef())
                    delete m_first->m_prev;
                else
                    m_first->m_prev->m_next = 0;
				m_first->m_prev = 0;
				m_count -= 1;
			}
		}
	}
	
	inline void removeLast(void)
	{
		if (m_count == 0)
		{
			throw ArgumentError("Noting to remove from list.");
		}
		else
		{
			if (m_count == 1)
			{
				if (m_first->private_removeRef())
					delete m_first;
				m_first = m_last = 0;
				m_count = 0;
			}
			else
			{
				m_last = m_last->m_prev;
				if (m_last->m_next->private_removeRef())
					delete m_last->m_next;
                else
                    m_last->m_next->m_prev = 0;
				m_last->m_next = 0;
				m_count -= 1;
			}
		}
	}
	
private:

	void checkNode(const NodeType* data) const
	{
		if (data->m_next != 0 || data->m_prev != 0 || m_first == data || m_last == data)
			throw ArgumentError("Data item already in some list.");
	}

	NodeType* m_first;
	NodeType* m_last;
	unsigned m_count;
};


}
}
#endif
