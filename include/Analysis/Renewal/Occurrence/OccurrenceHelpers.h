#ifndef _OCCURRENCE_HELPERS_H_INCLUDED_
#define _OCCURRENCE_HELPERS_H_INCLUDED_

#include "Analysis/Renewal/Occurrence/Occurrences.h"

namespace OPS
{

namespace Montego
{

class Occurrence;

}

namespace Renewal
{

OccurrencePtr convertMontegoToRenewal(const Montego::Occurrence& montegoOccurrence);

}

}

#endif
