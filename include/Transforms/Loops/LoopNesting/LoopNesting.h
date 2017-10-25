#ifndef _LOOP_NESTING_H_INCLUDED_
#define _LOOP_NESTING_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::ReprisePtr;

using OPS::Reprise::ForStatement;

using OPS::Reprise::BlockStatement;

using OPS::Reprise::ExpressionBase;


bool canApplyLoopNestingTo(ForStatement& forStatement);

BlockStatement& makeLoopNesting(ForStatement& forStatement, const ExpressionBase& h, bool flag = false, bool native=true, bool tryOptimizeBorders = false, bool generateTail = true);
BlockStatement& makeLoopNesting(ForStatement& forStatement, const int h, bool flag = false, bool native=true, bool tryOptimizeBorders = false, bool generateTail = true);
BlockStatement& makeLoopNesting(ForStatement& forStatement,
                                const ExpressionBase& h,
                                ForStatement*& resForStatement1,
                                ForStatement*& resForStatement2,
                                ForStatement*& resForStatement3,
                                bool outerBound = true,
                                bool tryOptimizeBorders = false,
                                bool generateTail = true);
BlockStatement& makeLoopNestingWithNativeCounter(ForStatement& forStatement,
                                                 const ExpressionBase& h,
                                                 ForStatement*& outerForStatement,
                                                 ForStatement*& innerForStatement,
                                                 ForStatement*& restForStatement,
                                                 bool outerBound=false,
                                                 bool tryOptimizeBorders=false,
                                                 bool generateTail = true);


}	// Loops
}	// Transforms
}	// OPS

#endif	// _LOOP_NESTING_H_INCLUDED_
