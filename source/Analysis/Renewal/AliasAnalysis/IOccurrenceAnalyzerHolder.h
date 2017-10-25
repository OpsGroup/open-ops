#ifndef _I_OCCURRENCE_ANALYZER_HOLDER_H_INCLUDED_
#define _I_OCCURRENCE_ANALYZER_HOLDER_H_INCLUDED_

namespace OPS
{

namespace Montego
{

class OccurrenceContainer;

}

namespace Reprise
{

class RepriseBase;

}

namespace Renewal
{

namespace Internals
{

class IOccurrenceAnalyzerHolder
{
public:
	virtual Montego::OccurrenceContainer& reqestOccurrenceAnalyzer(
		const Reprise::RepriseBase& context) = 0;

protected:
	inline ~IOccurrenceAnalyzerHolder() {}
};

}

}

}

#endif
