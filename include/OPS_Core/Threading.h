#ifndef OPS_COMMON_CORE_THREADING_H_
#define OPS_COMMON_CORE_THREADING_H_

#if 0
#include "OPS_Core/Mixins.h"

namespace OPS
{
namespace Threading
{

//      Mutex handle
typedef intptr_t mutex_handle_type;

/*
    Mutually exclusive object

        Do not intermix this with your OS mutexes - this one may be implemented using different means and has fixed
    predictable functionality.

    NOTE: NEVER define Mutex as a local variable
    NOTE: see comments on top of this file
*/
class Mutex : public OPS::NonCopyableMix
{
public:
/**
        Mutually exclusive lock

        Perfect for controlling mutex locks in a safe manner.

        NOTE: NEVER define Mutex::Lock as a static (locally or globally) object
        NOTE: see comments on top of this file
*/
    class Lock : public OPS::NonCopyableMix
    {
    public:
        Lock(Mutex& mutex) throw();

        ~Lock(void) throw();

    private:
    //  Members
    //      Mutex
        Mutex& m_mutex;
    };
/*
        Mutually exclusive try-lock

        Perfect for controlling mutex try-locks in a safe manner.

        NOTE: NEVER define Mutex::TryLock as a static (locally or globally) object
        NOTE: see comments on top of this file
*/
    class TryLock : public OPS::NonCopyableMix
    {
    public:
    //  Constructors/destructor
    /*
            TryLock constructor
            Params: mutex to lock
    */
        TryLock(Mutex& mutex) throw();

    /*
            TryLock destructor
    */
        ~TryLock(void) throw();

        inline bool is_locked(void) const
        {
            return m_locked;
        }

        bool retry(void) throw();

    private:
    //      Mutex
        Mutex& m_mutex;
    //      Locked state
        bool m_locked;
    //      Spin count
        unsigned m_spinCount;
    };
/*
        Mutually exclusive free lock

        Perfect for controlling mutex free locks in a safe manner.

        NOTE: NEVER define Mutex::FreeLock as a static (locally or globally) object
        NOTE: locking/unlocking is NOT reference-counted
        NOTE: see comments on top of this file
*/
    class FreeLock : public OPS::NonCopyableMix
    {
    public:
        FreeLock(Mutex& mutex) throw();

        ~FreeLock(void) throw();

        inline bool is_locked(void) const
        {
            return m_locked;
        }

        bool lock(void) throw();

        bool unlock(void) throw();

    private:
    //      Mutex
        Mutex& m_mutex;
    //      Locked state
        bool m_locked;
    };

    Mutex(void) throw();

    ~Mutex(void) throw();

private:
//      Mutex handle
    mutex_handle_type m_handle;
};

/*
    "Fast" mutually exclusive object (non-recursive)

        Do not intermix this with your OS mutexes - this one may be implemented using different means and has fixed
    predictable functionality. Also note that FastMutex is recommended to be locked for a very short amount of time,
    elseway the thread may consume additional system resources.

    NOTE: NEVER define FastMutex as a local variable
    NOTE: see comments on top of this file
*/
class FastMutex : public OPS::NonCopyableMix
{
public:
//  Classes
/*
        "Fast" mutually exclusive lock (non-recursive)

        Perfect for controlling "fast" mutex locks in a safe manner.

        NOTE: NEVER define FastMutex::Lock as a static (locally or globally) object
        NOTE: see comments on top of this file
*/
    class Lock : public OPS::NonCopyableMix
    {
    public:
    //  Constructors/destructor
    /*
            Lock constructor
            Params: mutex to lock
    */
        Lock(FastMutex& mutex) throw();

    /*
            Lock destructor
    */
        ~Lock(void) throw();

    private:
    //  Members
    //      Mutex
        FastMutex& m_mutex;
    };
/*
        "Fast" mutually exclusive try-lock

        Perfect for controlling "fast" mutex try-locks in a safe manner.

        NOTE: NEVER define FastMutex::TryLock as a static (locally or globally) object
        NOTE: see comments on top of this file
*/
    class TryLock : public OPS::NonCopyableMix
    {
    public:
    //  Constructors/destructor
    /*
            TryLock constructor
            Params: mutex to lock
    */
        TryLock(FastMutex& mutex) throw();

    /*
            TryLock destructor
    */
        ~TryLock(void) throw();

    //  Methods
    /*
            Locked state getter
            Params: none
            Return: locked state
    */
        inline bool is_locked(void) const
        {
            return m_locked;
        }

    /*
            Lock retrier
            Params: none
            Return: previous locked state
    */
        bool retry(void) throw();

    private:
    //  Members
    //      Mutex
        FastMutex& m_mutex;
    //      Locked state
        bool m_locked;
    //      Spin count
        unsigned m_spin_count;
    };
/*
        "Fast" mutually exclusive free lock

        Perfect for controlling "fast" mutex free locks in a safe manner.

        NOTE: NEVER define FastMutex::FreeLock as a static (locally or globally) object
        NOTE: locking/unlocking is NOT reference-counted
        NOTE: see comments on top of this file
*/
    class FreeLock : public OPS::NonCopyableMix
    {
    public:
    //  Constructors/destructor
    /*
            FreeLock constructor
            Params: mutex to lock
    */
        FreeLock(FastMutex& mutex) throw();

    /*
            FreeLock destructor
    */
        ~FreeLock(void) throw();

    //  Methods
    /*
            Locked state getter
            Params: none
            Return: locked state
    */
        inline bool is_locked(void) const
        {
            return m_locked;
        }

    /*
            Locker
            Params: none
            Return: previous locked state
    */
        bool lock(void) throw();

    /*
            Unlocker
            Params: none
            Return: previous locked state
    */
        bool unlock(void) throw();

    private:
    //  Members
    //      Mutex
        FastMutex& m_mutex;
    //      Locked state
        bool m_locked;
    };

//  Constructors/destructor
/*
        FastMutex default constructor
        Params: none
*/
    FastMutex(void) throw();

/*
        FastMutex destructor
*/
    ~FastMutex(void) throw();

private:
//  Members
//      Mutex handle
    mutex_handle_type m_handle;
};



}
}

#endif
#endif                      // OPS_COMMON_CORE_THREADING_H_


