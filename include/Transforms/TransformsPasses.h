#ifndef TRANSFORMS_PASSES_H
#define TRANSFORMS_PASSES_H

//  Standard includes

//  OPS includes
#include "OPS_Stage/Passes.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{

//  Constants and enums

//  Global classes

class LoopDistributionDone {}; // LoopDistribution не выдает никаких результатов наружу

class LoopDistributionPass : public PassBase
{
public:
    LoopDistributionPass();
    bool run();
};

class LoopFragmentationPass : public PassBase
{
public:
	LoopFragmentationPass();
	bool run();
	AnalysisUsage getAnalysisUsage() const;
};

class DataDistributionForSharedMemoryPass : public PassBase
{
public:
    DataDistributionForSharedMemoryPass();
    bool run();
};


//  Global functions

//  Exit namespace

}
}

#endif // TRANSFORMS_PASSES_H
