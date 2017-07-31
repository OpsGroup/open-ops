#include "Reprise/ProgramFragment.h"

namespace OPS
{
namespace Reprise
{

bool ProgramFragment::isFragment(const StatementBase& firstStmt, const StatementBase& lastStmt)
{
    if (firstStmt.getParent() == 0)
        return false;
	if (!firstStmt.getParent()->is_a<BlockStatement>())
	{
		return false;
	}
	const BlockStatement& firstBlock = firstStmt.getParentBlock();
    if (lastStmt.getParent() == 0)
        return false;
	if (lastStmt.getParent() != &firstBlock)
	{
		return false;
	}
	for (BlockStatement::ConstIterator firstIter = firstBlock.convertToIterator(&firstStmt); firstIter.isValid(); ++firstIter)
	{
		if (&*firstIter == &lastStmt)
		{
			return true; 
		}
	}
	return false;
}

ProgramFragment ProgramFragment::fromBlock(BlockStatement& block)
{
	ProgramFragment fragment;
	fragment.setFromBlock(block);
	return fragment;
}

ProgramFragment::ProgramFragment(void)
{
}

ProgramFragment::ProgramFragment(const ProgramFragment& other)
{
	if (!other.isEmpty())
	{
		m_first = other.m_first;
		m_last = other.m_last;
	}
}

ProgramFragment::ProgramFragment(StatementBase& stmt)
{
    set(stmt);
}

ProgramFragment::ProgramFragment(StatementBase& firstStmt, StatementBase& lastStmt)
{
	set(firstStmt, lastStmt);
}

bool ProgramFragment::isEmpty() const
{
	if ((m_first.get() == 0 && m_last.get() != 0) ||
		(m_first.get() != 0 && m_last.get() == 0))
	{
		throw RepriseError("Unexpected class state. One of firstStmt and lastStmt is NULL.");
	}
	return (m_first.get() == 0 && m_last.get() == 0);
}

bool ProgramFragment::isSingleStatement() const
{
    return m_first.get() != 0 && m_last.get() != 0 && m_last.get() == m_first.get();
}

void ProgramFragment::setEmpty(void)
{
	m_first.reset(0);
	m_last.reset(0);
}

void ProgramFragment::setFromBlock(BlockStatement& block)
{
	if (block.isEmpty())
	{
		setEmpty();
	}
	else
	{
		m_first.reset(&*block.getFirst());
		m_last.reset(&*block.getLast());
	}
}

void ProgramFragment::set(StatementBase& stmt)
{
    if (!stmt.is_a<BlockStatement>())
        set(stmt, stmt);
    else
        setFromBlock(stmt.cast_to<BlockStatement>());
}

void ProgramFragment::set(StatementBase& firstStmt, StatementBase& lastStmt)
{
	if (isFragment(firstStmt, lastStmt))
	{
		m_first.reset(&firstStmt);
		m_last.reset(&lastStmt);
	}
	else
	{
		throw RepriseError("Unexpected statements passed. firstStmt could not reach lastStmt.");
	}
}

const BlockStatement& ProgramFragment::getStatementsBlock(void) const
{
	return const_cast<ProgramFragment&>(*this).getStatementsBlock();
}

BlockStatement& ProgramFragment::getStatementsBlock(void)
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	BlockStatement& firstBlock = m_first->getParentBlock();
	if (&m_last->getParentBlock() != &firstBlock)
	{
		throw RepriseError("Unexpected class state. firstStmt and lastStmt have different parents.");
	}
	return firstBlock;
}

StatementBase& ProgramFragment::getFirst(void)
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return *m_first;
}

StatementBase& ProgramFragment::getLast(void)
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return *m_last;
}

const StatementBase& ProgramFragment::getFirst(void) const
{
    if (isEmpty())
    {
        throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
    }
    return *m_first;
}

const StatementBase& ProgramFragment::getLast(void) const
{
    if (isEmpty())
    {
        throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
    }
    return *m_last;
}

BlockStatement::Iterator ProgramFragment::getFirstIterator(void)
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return getStatementsBlock().convertToIterator(m_first.get());
}

BlockStatement::Iterator ProgramFragment::getLastIterator(void)
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return getStatementsBlock().convertToIterator(m_last.get());
}

BlockStatement::Iterator ProgramFragment::getAfterLastIterator(void)
{
    if (isEmpty())
    {
        throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
    }
    BlockStatement::Iterator lastIter = getStatementsBlock().convertToIterator(m_last.get());
    lastIter++;
    return lastIter;
}

BlockStatement::ConstIterator ProgramFragment::getFirstIterator(void) const
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return getStatementsBlock().convertToIterator(m_first.get());
}

BlockStatement::ConstIterator ProgramFragment::getLastIterator(void) const
{
	if (isEmpty())
	{
		throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
	}
	return getStatementsBlock().convertToIterator(m_last.get());
}

BlockStatement::ConstIterator ProgramFragment::getAfterLastIterator(void) const
{
    if (isEmpty())
    {
        throw RepriseError("Unexpected getting of statements block. Fragment is empty.");
    }
    BlockStatement::ConstIterator lastIter = getStatementsBlock().convertToIterator(m_last.get());
    lastIter++;
    return lastIter;
}

ProgramFragment& ProgramFragment::operator=(const ProgramFragment& other)
{
	if (other.isEmpty())
	{
		setEmpty();
	}
	else
	{
		m_first = other.m_first;
		m_last = other.m_last;
	}
	return *this;
}

bool ProgramFragment::operator == (const ProgramFragment& other)
{
	return this->m_first.get() == other.m_first.get() &&
		   this->m_last.get() == other.m_last.get();
}


}
}
