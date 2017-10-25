//		STL headers
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <cassert>
#include <list>

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Declarations.h"
#include "Reprise/Types.h"

#include "include/DistributedArrayReferenceInfo.h"
#include "include/shared_helpers.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Transforms;


DistributedArrayReferenceInfo::DistributedArrayReferenceInfo()
{
    this->source = 0;
}
DistributedArrayReferenceInfo::DistributedArrayReferenceInfo(OPS::Reprise::BasicCallExpression* source,
                              std::shared_ptr<ArrayDistributionInfo> distributionInfo)
{
    if (source == 0)
    {
        int a = 10;
    }
    OPS_ASSERT(source != 0);
    this->source = source;
    this->distributionInfo = distributionInfo;
}

bool DistributedArrayReferenceInfo::operator!=(const DistributedArrayReferenceInfo& i2) const
{
    return !(*this == i2);
}
bool DistributedArrayReferenceInfo::operator==(const DistributedArrayReferenceInfo& i2) const
{
    return source->getNCID() == i2.source->getNCID();
}
bool DistributedArrayReferenceInfo::operator<(const DistributedArrayReferenceInfo& i2)  const
{
    return source->getNCID() < i2.source->getNCID();
}

/*
bool ReferenceTable::Row::operator!=(const ReferenceTable::Row& i2) const {
    return !(*this == i2);
}
bool ReferenceTable::Row::operator==(const ReferenceTable::Row& i2) const {
    return dimensionIndex == i2.dimensionIndex
            && ref.source->getNCID() == i2.ref.source->getNCID();
}
bool ReferenceTable::Row::operator<(const ReferenceTable::Row& i2)  const {
    if (dimensionIndex < i2.dimensionIndex)
        return true;
    return ref.source->getNCID() < i2.ref.source->getNCID();
}
*/

class BlockArrayReferencesCollector : public OPS::Reprise::Service::DeepWalker
{
public:
    list<DistributedArrayReferenceInfo> result;
    list<shared_ptr<ArrayDistributionInfo> > infoList;
    void visit(BasicCallExpression& node)
    {
        if (node.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
        {
            ExpressionBase& expr = node.getArgument(0);
            if (expr.is_a<ReferenceExpression>() == true)
            {
                ReferenceExpression& ref = expr.cast_to<ReferenceExpression>();
                shared_ptr<ArrayDistributionInfo> info;
                if (isBlockArrayReference(ref, infoList, info) == true)
                {
                    DistributedArrayReferenceInfo ri(&node, info);
                    result.push_back(ri);
                }
            }
        }
        Service::DeepWalker::visit(node);
    }
};

std::list<DistributedArrayReferenceInfo> collectAllBlockArrayReferences(list<shared_ptr<ArrayDistributionInfo> >& infoList, SubroutineDeclaration* func)
{
    BlockArrayReferencesCollector visitor;
    visitor.infoList = infoList;
    func->accept(visitor);
    return visitor.result;
}

}
}
}
