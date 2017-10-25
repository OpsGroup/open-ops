#include "Analysis/Montego/DependenceGraph/DependenceLevel.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace Montego
{

DependenceLevel::DependenceLevel(const ForStatement& forStatement)
	: m_forStatement(&forStatement)
	, m_whileStatement(0)
{
}

DependenceLevel::DependenceLevel(const WhileStatement& whileStatement)
	: m_forStatement(0)
	, m_whileStatement(&whileStatement)
{
}

bool DependenceLevel::operator ==(const DependenceLevel& otherVertex) const
{
	return m_forStatement == otherVertex.m_forStatement &&
		m_whileStatement == otherVertex.m_whileStatement;
}



std::list<DependenceLevel> getAllDependenceLevelsBetween(
	const Reprise::StatementBase& sourceStatement,
	const Reprise::StatementBase& childStatement)
{
	if (&childStatement == &sourceStatement)
	{
		return std::list<DependenceLevel>();
	}

	std::list<DependenceLevel> levels;

	RepriseBase* parent = childStatement.getParent();
	while (parent != 0)
	{
		if (parent->is_a<StatementBase>())
		{
			const StatementBase& statement = parent->cast_to<StatementBase>();

			if (statement.is_a<ForStatement>())
			{
				levels.push_back(DependenceLevel(statement.cast_to<ForStatement>()));
			}
			else if (statement.is_a<WhileStatement>())
			{
				levels.push_back(DependenceLevel(statement.cast_to<WhileStatement>()));
			}

			if (&statement == &sourceStatement)
			{
				return levels;
			}
		}

		parent = parent->getParent();
	}

	OPS_ASSERT(!"childStatement is not a child of sourceStatement");

	return std::list<DependenceLevel>();
}

}
}
