#ifndef OPS_IR_REPRISE_SERVICE_MARKER_H_INCLUDED__
#define OPS_IR_REPRISE_SERVICE_MARKER_H_INCLUDED__

#include <map>
#include <set>

#include "Reprise/Statements.h"


namespace OPS
{
namespace Reprise
{
namespace Service
{

/**
	Simple Markers Storage.
	Store markers as set. Each node may have bool marks, defined by TMarks::MarksKind enum.
*/
template<typename TMarks>
class SimpleMarkersStorage
{
public:
	typedef typename TMarks::MarksKind MarksKind;
	typedef std::set<MarksKind> TMarksSet;

/**
	Mark checker.
	\param	kind - mark kind to check.
	\return present flag
*/
	bool hasMark(const MarksKind& kind) const
	{
		return m_marks.find(kind) != m_marks.end();
	}

/**
	Mark adder.
	\param	kind - mark kind to add
	\return true if added, otherwise - mark already present
*/
	bool addMark(const MarksKind& kind)
	{
		return m_marks.insert(kind).second;
	}

/**
	Markers getter
	\return const set of marks assigned to node
*/
	const TMarksSet& getMarks(void) const
	{
		return m_marks;
	}

private:

	TMarksSet m_marks;
};

/**
	Marker class.
	Marker class store marks for Reprise.
*/
template<typename TMarkersStorage>
class Marker
{
public:
	typedef typename TMarkersStorage::MarksKind MarksKind;

/**
	Clear all marks.
*/
	void clear(void)
	{
		m_marks.clear();
	}

/**
	Mark checker.
	\param	node - Reprise node to check mark of
	\param	kind - kind of mark to check
	\return	present flag
*/
	bool hasMark(const RepriseBase& node, const MarksKind& kind) const
	{
        typename TMarkedNodes::const_iterator marked = m_marks.find(&node);
		if (marked != m_marks.end())
			return marked->second.hasMark(kind);
		else
			return false;
	}
	
/**
	Checks that node has at least one mark
	\param	node - Reprise node to check marks of
	\return at least one mark is present
*/
	bool hasMarks(const RepriseBase& node) const
	{
		return m_marks.find(&node) != m_marks.end();
	}

/**
	Adds mark to specified node.
	\param	node - Reprise node to add mark to
	\param	kind - mark kind to add to node
	\return	true if added, otherwise - mark already present
*/
	bool addMark(const RepriseBase& node, const MarksKind& kind)
	{
		return m_marks[&node].addMark(kind);
	}

/**
	Marks getter.
	\param	node - Reprise node to get marks storage for
	\return	marks storage for node
*/
	TMarkersStorage& getMarks(const RepriseBase& node)
	{
		return m_marks[&node];
	}

/**
	[subject to change] Find marked statement straight in block
	\param	block - block of statements to find of
	\param	kind - mark kind to find
	\return 0 - if no marked statement found, otherwise - marked statement
*/
	StatementBase* findMarkedStatementInBlock(BlockStatement& block, const MarksKind& kind) const
	{
		if (block.isEmpty())
			return 0;
		for (BlockStatement::Iterator iter = block.getFirst(); iter.isValid(); iter.goNext())
		{
			if (hasMark(*iter, kind))
				return &*iter;
		}
		return 0;
	}

private:
	typedef std::map<const RepriseBase*, TMarkersStorage> TMarkedNodes;
	TMarkedNodes m_marks;
};


}
}
}

#endif
