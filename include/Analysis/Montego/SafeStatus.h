#pragma once

namespace OPS
{
namespace Montego
{

class SafeStatus
{
public:

    typedef unsigned int status_t;

    explicit SafeStatus(status_t s = 0);

    SafeStatus(const SafeStatus& s);

    SafeStatus& operator=(const SafeStatus& s);

    bool isStatus(status_t s) const;

    status_t getStatusData() const;

    //добавляет к имеющемуся еще флаги в s
    void addStatus(status_t s);

    //заменяет имеющийся на статус s
    void replaceAllStatusBy(status_t s);

    //отчищает все статусы
    void clearAllStatus(status_t s);

    //удаляет из имеющегося указанные статусы
    void unsetStatus(status_t s);

private:

    status_t m_status;
};

}//end of namespace
}//end of namespace
