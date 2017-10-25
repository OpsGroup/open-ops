#ifndef _DEPENDENCE_LEVEL_H_INCLUDED_
#define _DEPENDENCE_LEVEL_H_INCLUDED_

#include <Reprise/Reprise.h>

#include <list>

namespace OPS
{
namespace Montego
{

class DependenceLevel
{
public:
	explicit DependenceLevel(const Reprise::ForStatement& forStatement);
	explicit DependenceLevel(const Reprise::WhileStatement& whileStatement);

public:
	bool operator ==(const DependenceLevel& otherVertex) const;

private:
	DependenceLevel();

private:
	const Reprise::ForStatement* m_forStatement;
	const Reprise::WhileStatement* m_whileStatement;
};



// childStatement must be a child of sourceStatement!!!
std::list<DependenceLevel> getAllDependenceLevelsBetween(
	const Reprise::StatementBase& sourceStatement,
	const Reprise::StatementBase& childStatement);

}
}

#endif // _DEPENDENCE_LEVEL_H_INCLUDED_
