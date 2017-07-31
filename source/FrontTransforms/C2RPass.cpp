//  Standard includes

//  OPS includes
#include "FrontTransforms/C2RPass.h"
#include "FrontTransforms/CantoToReprise.h"

//  Local includes

//  Namespaces using
using namespace OPS::Frontend;

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
C2RPass::C2RPass()
{
}

bool C2RPass::run()
{
    Reprise::ProgramUnit& unit = workContext().program();
    C2R::convertTypes(unit);
    C2R::convertExpressions(unit, true, true, true);
    C2R::convertLiterals(unit);
	C2R::convertVariablesInit(workContext().program());
	C2R::convertBreakContinue(workContext().program());
    workContext().addService<NoCanto>(new NoCanto);
    return true;
}

RepriseCanonizationPass::RepriseCanonizationPass()
{
}

bool RepriseCanonizationPass::run()
{

    workContext().addService<RepriseCanonical>(new RepriseCanonical);
    return true;
}

//  Functions implementation

//  Exit namespace
}
}
