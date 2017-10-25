#ifndef _ALIAS_ANALYSIS_H_INCLUDED_
#define _ALIAS_ANALYSIS_H_INCLUDED_

#include "Analysis/Renewal/AliasAnalysis/IAliasAnalysisService.h"
#include "IOccurrenceAnalyzerHolder.h"

#include <Shared/ContextNotifier/ContextObserverBase.h>

#include <memory>

namespace OPS
{

namespace Montego
{

class AliasInterface;
class OccurrenceContainer;

}

namespace Renewal
{

class AliasAnalysisService
	: public Shared::ContextObserverBase
	, public IAliasAnalysisService
	, public Internals::IOccurrenceAnalyzerHolder
{
public:
	AliasAnalysisService();

	~AliasAnalysisService();

public:
	// Internals::IOccurrenceAnalyzerHolder

	virtual Montego::OccurrenceContainer& reqestOccurrenceAnalyzer(
		const Reprise::RepriseBase& context);

protected:
    virtual void syncronizeWithContext(const Reprise::RepriseBase& context);

private:
	Montego::AliasInterface& getAliasAnalyzer(const Reprise::RepriseBase& context) const;
	Montego::OccurrenceContainer& getOccurrenceAnalyzer(
		const Reprise::RepriseBase& context) const;
};

}

}

#endif
