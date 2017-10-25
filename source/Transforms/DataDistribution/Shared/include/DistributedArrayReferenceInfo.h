#ifndef DISTRIBUTED_ARRAY_REFERENCE_INFO_H
#define DISTRIBUTED_ARRAY_REFERENCE_INFO_H

#include <memory>
#include <vector>

#include "OPS_Core/MemoryHelper.h"

#include "Reprise/Utils.h"
#include "Reprise/Declarations.h"
#include "Reprise/Types.h"

#include "ArrayDistributionInfo.h"


namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using OPS::Reprise::BasicCallExpression;

struct DistributedArrayReferenceInfo
{
    OPS::Reprise::BasicCallExpression* source;
    std::shared_ptr<ArrayDistributionInfo> distributionInfo;

    DistributedArrayReferenceInfo();
    DistributedArrayReferenceInfo(OPS::Reprise::BasicCallExpression* source,
                                  std::shared_ptr<ArrayDistributionInfo> distributionInfo);

    ExpressionBase* calculateIndex(int dim);

    bool operator!=(const DistributedArrayReferenceInfo& i2) const;
    bool operator==(const DistributedArrayReferenceInfo& i2) const;
    bool operator<(const DistributedArrayReferenceInfo& i2)  const;
};


std::list<DistributedArrayReferenceInfo> collectAllBlockArrayReferences(std::list<std::shared_ptr<ArrayDistributionInfo> >& infoList,
                                                            SubroutineDeclaration* func);

}
}
}

#endif
