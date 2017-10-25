#ifndef _LOOP_SPLIT_H_INCLUDED_
#define _LOOP_SPLIT_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
    {
    namespace Loops
        {
            std::pair<Reprise::ForStatement*, Reprise::ForStatement*>
                makeLoopSplitting(Reprise::ForStatement* forStatement, Reprise::ExpressionBase* minSplittingExpression,
                                  Reprise::ExpressionBase* maxSplittingExpression);
            std::pair<Reprise::ForStatement*, Reprise::ForStatement*>
                makeLoopSplittingSafe(Reprise::ForStatement* forStatement, Reprise::ExpressionBase* splittingExpression);

        }	// Loops
    }	// Transforms
}	// OPS

#endif	// _LOOP_SPLIT_H_INCLUDED_
