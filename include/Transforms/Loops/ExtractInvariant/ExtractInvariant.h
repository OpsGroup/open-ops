#ifndef EXTRACTINVARIANT_H
#define EXTRACTINVARIANT_H


#include <Reprise/Reprise.h>


namespace OPS
{
namespace Analysis
{
void extractInvariant(Reprise::BasicCallExpression* expressionForReplace, Reprise::ForStatement* target);
}
}
#endif // EXTRACTINVARIANT_H