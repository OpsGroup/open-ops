#include "Analysis/ConsistCheck/Conditions.h"

#include "Analysis/ConsistCheck/IConsistCheckService.h"

#include <Reprise/Reprise.h>

using namespace OPS::Reprise;

namespace OPS
{

namespace Analysis
{

void addCondition(ConditionList& conditions, ConditionBase* condition)
{
	conditions.push_back(ConditionPtr(condition));
}

/////////////////////////////////////////////////////////////////////////////////////////

BasicTypeCondition::BasicTypeCondition(BasicType::BasicTypes type)
	: m_type(type)
{
}

bool BasicTypeCondition::isAllowed(const RepriseBase& repriseObject) const
{
	return RepriseCondition<BasicType>::isAllowed(repriseObject) &&
		repriseObject.cast_to<BasicType>().getKind() == m_type;
}

/////////////////////////////////////////////////////////////////////////////////////////

VariableBasicTypeCondition::VariableBasicTypeCondition(BasicType::BasicTypes type)
	: m_type(type)
{
}

bool VariableBasicTypeCondition::isAllowed(const RepriseBase &repriseObject) const
{
	if (!RepriseCondition<ReferenceExpression>::isAllowed(repriseObject))
	{
		return false;
	}

	ConditionList conditions;
	addCondition(conditions, new BasicTypeCondition(m_type));
	addCondition(conditions, new RepriseCondition<ArrayType>());
	addCondition(conditions, new RepriseCondition<VectorType>());

	return getConsistCheckService().matchTo(
		repriseObject.cast_to<ReferenceExpression>().getReference().getType(), conditions);
}

void addAllIntVariableConditions(ConditionList& conditions)
{
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT8));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT16));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT32));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT64));
    addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_INT128));

	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_UINT8));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_UINT16));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_UINT32));
	addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_UINT64));
    addCondition(conditions, new VariableBasicTypeCondition(BasicType::BT_UINT128));
}

}

}
