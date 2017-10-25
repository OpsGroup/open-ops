#include "ConsistCheckService.h"

#include "ConsistCheckWalker.h"

#include <Reprise/Common.h>

using namespace OPS::Reprise;

namespace OPS
{

namespace Analysis
{

ConsistCheckService::ConsistCheckService()
{
}

ConsistCheckService::~ConsistCheckService()
{
}

bool ConsistCheckService::matchTo(const RepriseBase &repriseObject,
	const ConditionList &conditions) const
{
	ConsistCheckWalker consistCheckWalker(conditions);

	const_cast<RepriseBase&>(repriseObject).accept(consistCheckWalker);

	return consistCheckWalker.getResult();
}

}

}
