#ifndef _OCCURRENCES_H_INCLUDED_
#define _OCCURRENCES_H_INCLUDED_

#include <OPS_Core/MemoryHelper.h>
#include <OPS_Core/Mixins.h>

#include <Reprise/Expressions.h>
#include <Reprise/Utils.h>

namespace OPS
{

namespace Renewal
{

class AbstractOccurrence
	: public TypeConvertibleMix
{
public:
	enum Type
	{
		T_GENERATOR = 0,
		T_USSAGE
	};

public:
	virtual ~AbstractOccurrence();

public:
	Type getType() const;

	bool equals(const AbstractOccurrence& other) const;

	virtual const Reprise::ExpressionBase& getOccurrenceNode() const = 0;
	virtual const Reprise::ExpressionBase& getMemoryAccessNode() const = 0;

protected:
	explicit AbstractOccurrence(Type type);

private:
	Type m_type;
};

typedef std::tr1::shared_ptr<AbstractOccurrence> OccurrencePtr;

//////////////////////////////////////////////////////////////////////////////////////////

class DataAccessOccurrence
	: public AbstractOccurrence
{
public:
	DataAccessOccurrence(Type type, const Reprise::ExpressionBase& occurrenceNode,
		const Reprise::ExpressionBase& memoryAccessNode);

	virtual ~DataAccessOccurrence();

public:
	virtual const Reprise::ExpressionBase& getOccurrenceNode() const;
	virtual const Reprise::ExpressionBase& getMemoryAccessNode() const;

private:
	Reprise::RepriseWeakPtr<Reprise::ExpressionBase> m_occurrenceNode;
	Reprise::RepriseWeakPtr<Reprise::ExpressionBase> m_memoryAccessNode;
};

//////////////////////////////////////////////////////////////////////////////////////////

class FunctionCallOccurrence
	: public AbstractOccurrence
{
public:
	FunctionCallOccurrence(Type type,
		const Reprise::SubroutineCallExpression& subroutineCallNode);

	virtual ~FunctionCallOccurrence();

public:
	virtual const Reprise::SubroutineCallExpression& getOccurrenceNode() const;
	virtual const Reprise::SubroutineCallExpression& getMemoryAccessNode() const;

private:
	Reprise::RepriseWeakPtr<Reprise::SubroutineCallExpression> m_subroutineCallNode;
};

}

}

#endif
