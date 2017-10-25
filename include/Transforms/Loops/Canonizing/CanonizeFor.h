#pragma once
#include "Reprise/Reprise.h"
#include "Analysis/Montego/AliasAnalysis/AliasImplementation.h"

namespace OPS
{
namespace Transforms
{

bool forIsCanonized(OPS::Reprise::ForStatement& fstmt, OPS::Montego::AliasImplementation &ai);

//возвражает true, если получилось, false - если нет
bool canonizeFor(OPS::Reprise::ForStatement& fstmt);

}
}
