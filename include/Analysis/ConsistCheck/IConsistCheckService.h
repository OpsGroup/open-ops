/*=======================================================================================

	Example: Need to check that loop body contains only expressions without lables,
	function cals, variable declarations and other stuff. And expressions use only
	int scalar variables and int arrays. Like this:

		int a[1000], b[1000], c[1000];

		{
			int i;

			for(i = 0; i < 1000; ++i)
			{
				a[i] = b[i] + c[i];
			}
		}

	Howto:

		using namespace Analysis;
		using namespace Reprise;

		ConditionList conditions;
		addCondition(conditions, new StatementCondition<EmptyStatement>());
		addCondition(conditions, new StatementCondition<ExpressionStatement>());
		addCondition(conditions, new RepriseCondition<EmptyExpression>());
		addCondition(conditions, new RepriseCondition<StrictLiteralExpression>());
		addCondition(conditions, new RepriseCondition<BasicCallExpression>());
		addAllIntVariableConditions(conditions);

		const bool result = getConsistCheckService().matchTo(forStatement.getBody(),
			conditions);

	Note: Validation available only on the statment level, variable declaration level and
	lower levels.

=======================================================================================*/

#ifndef _I_CONSIST_CHECK_H_INCLUDED_
#define _I_CONSIST_CHECK_H_INCLUDED_

#include "Analysis/ConsistCheck/Conditions.h"

#include <OPS_Core/ServiceLocator.h>

namespace OPS
{

namespace Reprise
{

class RepriseBase;

}

namespace Analysis
{

class IConsistCheckService
{
public:
	virtual bool matchTo(const Reprise::RepriseBase& repriseObject,
		const ConditionList& conditions) const = 0;

protected:
	virtual ~IConsistCheckService() {}
};

inline IConsistCheckService& getConsistCheckService()
{
	return Core::ServiceLocator::instance().getService<IConsistCheckService>();
}

}

}

#endif
