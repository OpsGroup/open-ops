#ifndef RESOLVERPASS_H
#define RESOLVERPASS_H

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
class NamesResolved {};

class ResolverPass : public PassBase
{
public:
    ResolverPass();
    bool run();
};

//  Global functions

//  Exit namespace
}
}
#endif // RESOLVERPASS_H
