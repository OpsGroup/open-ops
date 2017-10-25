#ifndef CHANGE_REFERENCE_ALGORITHMS_H
#define CHANGE_REFERENCE_ALGORITHMS_H



#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <cassert>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"
#include "RenameDistributionArrayVisitor.h"
#include "ChangeIndexesVisitor.h"
#include "shared_helpers.h"
#include "ArrayDistributionInfo.h"
#include "ReferenceTable.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{


void change_references(std::list<DistributedArrayReferenceInfo>& references_without_multi_index, ReferenceTable& dim_table, std::list<DistributedArrayReferenceInfo>& array_distrib_refs_list,
                                             SubroutineDeclaration* current_function);

}
}
}

#endif
