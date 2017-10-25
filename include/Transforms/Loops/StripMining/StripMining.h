#ifndef _STRIP_MINING_H_INCLUDED_
#define _STRIP_MINING_H_INCLUDED_

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using OPS::Reprise::ReprisePtr;

using OPS::Reprise::ForStatement;

using OPS::Reprise::ExpressionBase;

using OPS::Reprise::BlockStatement;


bool canApplyStripMiningTo(ForStatement& forStatement, ReprisePtr<ExpressionBase>);

BlockStatement& makeStripMining(ForStatement& forStatement, ReprisePtr<ExpressionBase> h);

}	// Loops
}	// Transforms
}	// OPS

#endif	// _STRIP_MINING_H_INCLUDED_
