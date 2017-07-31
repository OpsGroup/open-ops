#ifndef OPS_REPRISE_PROGRAMFRAGMENT_H_INCLUDED__
#define OPS_REPRISE_PROGRAMFRAGMENT_H_INCLUDED__

#include "Reprise/Statements.h"
#include "Reprise/Utils.h"

namespace OPS
{
namespace Reprise
{

/**
	Program fragment.
*/
class ProgramFragment : public OPS::BaseVisitable<>
{
public:

	/// Checks that fragment is valid
	static bool isFragment(const StatementBase& firstStmt, const StatementBase& lastStmt);
	
	/// Create ProgramFragment from BlockStatement
	static ProgramFragment fromBlock(BlockStatement& block);

	/// Create empty fragment
	ProgramFragment(void);
	/// Copy constructor
	ProgramFragment(const ProgramFragment& other);
	/// Create fragment from single statement
	ProgramFragment(StatementBase& stmt);
	/// Create fragment from first statement to last (last included)
	ProgramFragment(StatementBase& firstStmt, StatementBase& lastStmt);

	/// Check that fragment is empty
	bool isEmpty() const;

    /// Check that fragment consists of only one statement
    bool isSingleStatement() const;

	/// Set fragment to empty
	void setEmpty(void);
	/// Set fragment from BlockStatement
	void setFromBlock(BlockStatement& block);
	/// Set fragment from single statement
	void set(StatementBase& stmt);
	/// Set fragment from first statement to last (last included)
	void set(StatementBase& firstStmt, StatementBase& lastStmt);

	/// Get block for statements in fragment (const version)
	const BlockStatement& getStatementsBlock(void) const;
	/// Get block for statements in fragment
	BlockStatement& getStatementsBlock(void);

	/// Get first statement in fragment
	StatementBase& getFirst(void);
	/// Get last statement in fragment
	StatementBase& getLast(void);

    /// Get first statement in fragment
    const StatementBase& getFirst(void) const;
    /// Get last statement in fragment
    const StatementBase& getLast(void) const;

	/// Get iterator to first statement in fragment
	BlockStatement::Iterator getFirstIterator(void);
	/// Get iterator to last statement in fragment
	BlockStatement::Iterator getLastIterator(void);
    /// Get iterator to statement after last
    BlockStatement::Iterator getAfterLastIterator(void);

	/// Get const iterator to first statement in fragment
	BlockStatement::ConstIterator getFirstIterator(void) const;
	/// Get const iterator to last statement in fragment
	BlockStatement::ConstIterator getLastIterator(void) const;
    /// Get iterator to statement after last
    BlockStatement::ConstIterator getAfterLastIterator(void) const;

	/// Assignment operator
	ProgramFragment& operator=(const ProgramFragment& other);

	bool operator==(const ProgramFragment& other);

    OPS_DEFINE_VISITABLE()

private:
	RepriseWeakPtr<StatementBase> m_first;
	RepriseWeakPtr<StatementBase> m_last;
};

}
}

#endif 		//	OPS_REPRISE_PROGRAMFRAGMENT_H_INCLUDED__
