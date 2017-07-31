#ifndef C2RPASS_H
#define C2RPASS_H

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

class NoCanto {};

/// Pass converting Canto nodes to Reprise nodes
class C2RPass : public PassBase
{
public:
    C2RPass();
    bool run();
};

class RepriseCanonical {};

/// Pass converts variable initializations and multiple assignments
class RepriseCanonizationPass : public PassBase
{
public:
    RepriseCanonizationPass();
    bool run();
};

//  Global functions

//  Exit namespace

}
}

#endif // C2RPASS_H
