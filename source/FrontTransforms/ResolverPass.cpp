//  Standard includes

//  OPS includes
#include "FrontTransforms/ResolverPass.h"
#include "FrontTransforms/Resolver.h"

//  Local includes

//  Namespaces using
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
ResolverPass::ResolverPass()
{
}

bool ResolverPass::run()
{
    OPS::Transforms::Resolver resolver;
    resolver.setProgram(workContext().program());
    resolver.resolve();

    for(int i = 0; i < resolver.getErrorCount(); ++i)
    {
        manager()->addDiagnostics(CompilerResultMessage(CompilerResultMessage::CRK_ERROR,
                                 resolver.getError(i), ""));
    }

    if (resolver.getErrorCount() == 0)
        workContext().addService<NamesResolved>(new NamesResolved);

    return true;
}

//  Functions implementation

//  Exit namespace
}
}
