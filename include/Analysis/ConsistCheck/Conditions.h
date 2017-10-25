#ifndef _CONDITIONS_H_INCLUDED_
#define _CONDITIONS_H_INCLUDED_

#include <Reprise/Common.h>
#include <Reprise/Types.h>

#include <OPS_Core/MemoryHelper.h>

#include <list>

namespace OPS
{

namespace Reprise
{

class ReferenceExpression;

}

namespace Analysis
{

class ConditionBase
{
public:
	virtual ~ConditionBase() {}

	virtual bool isAllowed(const Reprise::RepriseBase& repriseObject) const = 0;
};

typedef std::tr1::shared_ptr<ConditionBase> ConditionPtr;

typedef std::list<ConditionPtr> ConditionList;

void addCondition(ConditionList& conditions, ConditionBase* condition);

/////////////////////////////////////////////////////////////////////////////////////////

template<class RepriseClass>
class RepriseCondition
	: public ConditionBase
{
public:
	virtual bool isAllowed(const Reprise::RepriseBase& repriseObject) const;
};

template<class RepriseClass>
bool RepriseCondition<RepriseClass>::isAllowed(
	const Reprise::RepriseBase &repriseObject) const
{
	return repriseObject.is_a<RepriseClass>();
}

/////////////////////////////////////////////////////////////////////////////////////////

template<class StatementClass>
class StatementCondition
	: public RepriseCondition<StatementClass>
{
public:
	explicit StatementCondition(bool isLableAllowed = false);

	virtual bool isAllowed(const Reprise::RepriseBase& repriseObject) const;

private:
	bool m_isLableAllowed;
};

template<class StatementClass>
StatementCondition<StatementClass>::StatementCondition(bool isLableAllowed)
	: m_isLableAllowed(isLableAllowed)
{
}

template<class StatementClass>
bool StatementCondition<StatementClass>::isAllowed(
	const Reprise::RepriseBase& repriseObject) const
{
	return RepriseCondition<StatementClass>::isAllowed(repriseObject) &&
		(m_isLableAllowed || !repriseObject.cast_to<StatementClass>().hasLabel());
}

/////////////////////////////////////////////////////////////////////////////////////////

class BasicTypeCondition
	: public RepriseCondition<Reprise::BasicType>
{
public:
	explicit BasicTypeCondition(Reprise::BasicType::BasicTypes type);

	virtual bool isAllowed(const Reprise::RepriseBase& repriseObject) const;

private:
	Reprise::BasicType::BasicTypes m_type;
};

/////////////////////////////////////////////////////////////////////////////////////////

class VariableBasicTypeCondition
	: public RepriseCondition<Reprise::ReferenceExpression>
{
public:
	explicit VariableBasicTypeCondition(Reprise::BasicType::BasicTypes type);

	virtual bool isAllowed(const Reprise::RepriseBase& repriseObject) const;

private:
	Reprise::BasicType::BasicTypes m_type;
};

void addAllIntVariableConditions(ConditionList& conditions);

}

}

#endif
