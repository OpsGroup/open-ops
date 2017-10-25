#ifndef _CONSIST_CHECK_SERVICE_H_INCLUDED_
#define _CONSIST_CHECK_SERVICE_H_INCLUDED_

#include "Analysis/ConsistCheck/IConsistCheckService.h"

namespace OPS
{

namespace Analysis
{

class ConsistCheckService
	: public IConsistCheckService
{
public:
	ConsistCheckService();
	~ConsistCheckService();

	virtual bool matchTo(const Reprise::RepriseBase& repriseObject,
		const ConditionList& conditions) const;
};

}

}

#endif
