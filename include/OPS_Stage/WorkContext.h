#pragma once
//  Standard includes

//  OPS includes
#include "Reprise/Units.h"
#include "OPS_Core/ServiceLocator.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{

//  Constants and enums

//  Global classes

class WorkContext : public OPS::Core::ServiceLocatorBase
{
public:
    WorkContext();
    ~WorkContext();

    Reprise::ProgramUnit& program() { return *m_program; }
    const Reprise::ProgramUnit& program() const { return *m_program; }

    void reset();

    // Satellite programs: opencl, ...
    Reprise::RepriseList<Reprise::ProgramUnit> m_satellitePrograms;

private:
    // Main program
    Reprise::ReprisePtr<Reprise::ProgramUnit> m_program;
};

//  Global functions

//  Exit namespace
}
}
