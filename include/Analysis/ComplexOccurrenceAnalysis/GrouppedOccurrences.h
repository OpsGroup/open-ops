#ifndef _GROUPPED_OCCURRENCES_H_INCLUDED_
#define _GROUPPED_OCCURRENCES_H_INCLUDED_

#include <list>
#include <map>

namespace OPS
{

namespace Reprise
{

class ExpressionBase;
class StatementBase;
class VariableDeclaration;

}

namespace Analysis
{

enum GrouppedByType
{
	GBT_READ = 0,
	GBT_WRITE
};

typedef std::list<const Reprise::ExpressionBase*> TopLevelOccurrenceList;
typedef std::map<const Reprise::VariableDeclaration*, TopLevelOccurrenceList>
	OccurrencesByDeclarations;

OccurrencesByDeclarations findAllTopLevelOccurrences(const Reprise::StatementBase& statement,
	GrouppedByType grouppedBy);

}

}

#endif
