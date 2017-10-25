#include "Analysis/Montego/SafeStatus.h"

namespace OPS
{
namespace Montego
{

SafeStatus::SafeStatus(status_t s):m_status(s)
{
}

SafeStatus::SafeStatus(const SafeStatus& s):m_status(s.getStatusData())
{
}

SafeStatus& SafeStatus::operator=(const SafeStatus& s)
{
    m_status=s.getStatusData();
    return *this;
}

bool SafeStatus::isStatus(status_t s) const 
{ 
    return (m_status & s) == s;
}

SafeStatus::status_t SafeStatus::getStatusData() const 
{ 
    return m_status; 
}

void SafeStatus::addStatus(status_t s) 
{ 
    m_status |= s; 
}

void SafeStatus::replaceAllStatusBy(status_t s)
{ 
    m_status = s; 
}

void SafeStatus::clearAllStatus(status_t s) 
{ 
    m_status = 0; 
}

void SafeStatus::unsetStatus(status_t s) 
{ 
    m_status = m_status & (~s);
}

}//end of namespace
}//end of namespace
