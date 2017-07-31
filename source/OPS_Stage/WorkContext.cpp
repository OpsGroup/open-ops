//  Standard includes

//  OPS includes
#include "OPS_Stage/WorkContext.h"

//  Local includes

//  Namespaces using

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

WorkContext::WorkContext()
{
}

WorkContext::~WorkContext()
{
    clearServices();
}

void WorkContext::reset()
{
    m_program.reset(new OPS::Reprise::ProgramUnit);
    m_satellitePrograms.clear();
    clearServices();
}

//  Global classes implementation

//  Functions implementation

//  Exit namespace
}
}
