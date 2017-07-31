#include "Tests.h"
#include "Reprise/Reprise.h"

#include <string>
#include <iostream>

class DummyClass : public OPS::Reprise::IntrusiveNodeBase<DummyClass>, public OPS::Reprise::IntrusivePointerBase
{
public:
	inline explicit DummyClass(const std::string id) : m_id(id)	{}

	inline std::string getId(void) const
	{
		return m_id;
	}

	inline void setId(const std::string& id)
	{
		m_id = id;
	}

	inline void print(void) const
	{
		std::cout << m_id << "\n";
	}


private:
	std::string m_id;
};

void testCollections()
{
	typedef OPS::Reprise::IntrusiveList<DummyClass> TList;
	{
		TList list1;
		list1.addFirst(new DummyClass("af"));
		list1.addLast(new DummyClass("al"));

		DummyClass* dc = new DummyClass("dc");
		list1.addLast(dc);

		list1.addBefore(list1.getFirst(), new DummyClass("1"));
		list1.addAfter(list1.getLast(), new DummyClass("2"));

		list1.addBefore(list1.getLast(), new DummyClass("3"));
		list1.addAfter(list1.getFirst(), new DummyClass("4"));

		for (TList::ConstIterator it = list1.getFirst(); it.isValid(); ++it)
		{
			std::cout << it->getId() << " ";
		}

		const DummyClass* cdc = dc;
		TList::ConstIterator it = list1.convertToIterator(cdc);

		/*
		std::cout << (list1.contains(dc) ? "true" : "false");
		std::cout << "\n";
		TList::Iterator dc2 = list1.find(dc);
		if (dc2.isValid())
			dc2->print();
		else
			std::cout << "Not Found!!!";

		list1.removeFirst();
		list1.removeLast();
*/

	}

}