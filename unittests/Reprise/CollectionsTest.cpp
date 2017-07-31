#include "Reprise/Reprise.h"

#include <vector>
#include <string>
#include <memory>
#include "GTestIncludeWrapper.h"

using namespace std;
using namespace OPS;
using namespace OPS::Reprise;

class Dummy : public IntrusiveNodeBase<Dummy>, public IntrusivePointerBase
{
public:
	enum EventType
	{
		ET_CREATE,
		ET_DELETE,
		ET_RESET
	};

	struct TState
	{
		int Id;
		int GlobalId;
	};

	typedef pair<EventType, TState> TLogEntry;
	typedef vector<TLogEntry> TLogList;

	Dummy()
	{
		m_state.Id = ++s_state.Id;
		m_state.GlobalId = ++s_state.GlobalId;
		addLog(ET_CREATE);
	}
	
	~Dummy()
	{
		addLog(ET_DELETE);
	}

	int getId() const
	{
		return m_state.Id;
	}

	static void reset()
	{
		addGlobalLog(ET_RESET);
		s_state.Id = 0;
		s_log.clear();
	}

	static TLogList& log()
	{
		return s_log;
	}

private:

	void addLog(const EventType evType)
	{
		s_log.push_back(make_pair(evType, m_state));
		s_fullLog.push_back(make_pair(evType, m_state));
	}

	static void addGlobalLog(const EventType evType)
	{
		s_fullLog.push_back(make_pair(evType, s_state));
	}

	TState m_state;
	static TState s_state;

	static TLogList s_fullLog;
	static TLogList s_log;
};

Dummy::TLogList Dummy::s_fullLog;
Dummy::TLogList Dummy::s_log;
Dummy::TState Dummy::s_state = {0, 0};


typedef IntrusiveList<Dummy> DummyListType;


TEST(RepriseCollections, IntrusiveListDoubleAdd)
{
	std::unique_ptr<TranslationUnit> unit(new TranslationUnit(TranslationUnit::SL_C));
	std::unique_ptr<EnumType> enumType(new EnumType);
	std::unique_ptr<TypeDeclaration> enumDecl(new TypeDeclaration(enumType.get(), "FOO"));

	enumType.release();

	unit->getGlobals().addType(enumDecl.get());
	EXPECT_THROW(unit->getGlobals().addType(enumDecl.get()), OPS::ArgumentError);

	enumDecl.release();
	EXPECT_NO_THROW(unit.reset(0));
}

TEST(RepriseCollections, emptyListTest)
{
	//	Empty List
	DummyListType list1;
	ASSERT_TRUE(list1.isEmpty());
	ASSERT_EQ(0, list1.getCount());
	EXPECT_NO_THROW(list1.clear());
}

TEST(RepriseCollections, oneElementTest)
{
	DummyListType list1;
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addBefore(list1.getFirst(), new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addBefore(list1.getLast(), new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addAfter(list1.getFirst(), new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addAfter(list1.getLast(), new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
	//	add First/Last
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addFirst(new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it = list1.addLast(new Dummy);
		ASSERT_EQ(1, it->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(1, list1.getCount());
		ASSERT_TRUE(list1.getFirst() == list1.getLast());
		ASSERT_FALSE(list1.getFirst() != list1.getLast());
		ASSERT_EQ(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(2, Dummy::log().size());
	}
}

TEST(RepriseCollections, twoElementsTest)
{
	DummyListType list1;
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addFirst(new Dummy);
		DummyListType::Iterator it2 = list1.addBefore(it1, new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addFirst(new Dummy);
		DummyListType::Iterator it2 = list1.addAfter(it1, new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addLast(new Dummy);
		DummyListType::Iterator it2 = list1.addBefore(it1, new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addLast(new Dummy);
		DummyListType::Iterator it2 = list1.addAfter(it1, new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addLast(new Dummy);
		DummyListType::Iterator it2 = list1.addFirst(new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		DummyListType::Iterator it1 = list1.addFirst(new Dummy);
		DummyListType::Iterator it2 = list1.addLast(new Dummy);
		ASSERT_EQ(2, it2->getId());
		ASSERT_FALSE(list1.isEmpty());
		ASSERT_EQ(2, list1.getCount());
		ASSERT_TRUE(list1.getFirst() != list1.getLast());
		ASSERT_FALSE(list1.getFirst() == list1.getLast());
		ASSERT_NE(list1.getFirst()->getId(), list1.getLast()->getId());
		EXPECT_NO_THROW(list1.clear());
		ASSERT_EQ(4, Dummy::log().size());
	}
}

TEST(RepriseCollections, removeTest)
{
	DummyListType list1;
	{
		Dummy::reset();
		Dummy* d1 = new Dummy;
		DummyListType::Iterator it1 = list1.addFirst(d1);
		DummyListType::Iterator it2 = list1.addBefore(it1, new Dummy);
		list1.remove(d1);
		list1.remove(it2);
		ASSERT_EQ(4, Dummy::log().size());
	}
	{
		Dummy::reset();
		Dummy* d1 = new Dummy;
		DummyListType::Iterator it1 = list1.addFirst(d1);
		DummyListType::Iterator it2 = list1.addBefore(it1, new Dummy);
		list1.removeFirst();
		list1.removeLast();
		ASSERT_EQ(4, Dummy::log().size());
	}

    DummyListType list2;
    {
        Dummy::reset();
        ReprisePtr<Dummy> d1(new Dummy), d2(new Dummy), d3(new Dummy), d4(new Dummy);
        list1.addLast(d1.get());
        list1.addLast(d2.get());
        list1.addLast(d3.get());
        list1.addLast(d4.get());

        list1.remove(d2.get());
        list1.removeFirst();
        list1.removeLast();

        EXPECT_NO_THROW(list2.addLast(d1.get()));
        EXPECT_NO_THROW(list2.addLast(d2.get()));
        EXPECT_NO_THROW(list2.addLast(d4.get()));
    }
}
