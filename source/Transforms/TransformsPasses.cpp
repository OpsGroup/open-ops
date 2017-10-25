//  Standard includes
#include <iostream>

//  OPS includes
#include "Transforms/TransformsPasses.h"
#include "Reprise/Reprise.h"
#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
#include "Transforms/Loops/LoopFragmentation/LoopFragmentation.h"
#include "FrontTransforms/C2RPass.h"
#include "Shared/NodesCollector.h"
#include "Transforms/DataDistribution/Shared/DataDistributionForSharedMemory.h"

//  Local includes

//  Namespaces using
using namespace OPS;
using namespace OPS::Reprise;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{

//  Constants and enums

//  Classes

//  Functions declaration

//  Variables

//  Classes implementation

//  Global classes implementation
LoopDistributionPass::LoopDistributionPass()
{
}

bool LoopDistributionPass::run()
{
    Reprise::ProgramUnit& unit = workContext().program();
	for(int i=0; i<unit.getUnitCount(); ++i)
	{
		Declarations::SubrIterator itSubr = unit.getUnit(i).getGlobals().getFirstSubr();
		for(; itSubr.isValid(); itSubr++)
		{
			list<ForStatement*> queue; // очередь "внешних" циклов for
			// TODO: сделать очередь циклов for с учетом вложенности с помощью визитора
			BlockStatement::Iterator itStatement = itSubr->getBodyBlock().getFirst();
			for(; itStatement.isValid(); ++itStatement)
			{
				if (itStatement->is_a<ForStatement>())
					queue.push_back(itStatement->cast_ptr<ForStatement>());
			}
			list<ForStatement*>::iterator itFor = queue.begin();
			for(; itFor != queue.end(); ++itFor)
				OPS::Transforms::Loops::LoopDistribution(*itFor);
		}
	}
	
    workContext().addService<LoopDistributionDone>(new LoopDistributionDone);
    return true;
}

LoopFragmentationPass::LoopFragmentationPass()
{
}

bool LoopFragmentationPass::run()
{
    Reprise::ProgramUnit& unit = workContext().program();
    for(int i=0; i<unit.getUnitCount(); ++i)
    {
        Declarations::SubrIterator itSubr = unit.getUnit(i).getGlobals().getFirstSubr();
        for(; itSubr.isValid(); itSubr++)
        {
            if (itSubr->hasImplementation() == false)
                continue;

            OPS::Transforms::Loops::BlockNestPragmaWalker walker;
            walker.visit(itSubr->getBodyBlock());
        }
    }

    return true;
}

AnalysisUsage LoopFragmentationPass::getAnalysisUsage() const
{
	return AnalysisUsage().addRequired<NoCanto>().addRequired<RepriseCanonical>();
}

DataDistributionForSharedMemoryPass::DataDistributionForSharedMemoryPass()
{
}

bool DataDistributionForSharedMemoryPass::run()
{
    Reprise::ProgramUnit& unit = workContext().program();
    for(int i=0; i<unit.getUnitCount(); ++i)
    {
        OPS::Transforms::DataDistribution::DataDistributionForSharedMemory transform;
        transform.makeTransform(&unit);
    }

    return true;
}

//  Functions implementation

//  Exit namespace
}
}
