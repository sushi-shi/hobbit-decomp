#ifndef HOBBIT_X_MUTEX_HPP
#define HOBBIT_X_MUTEX_HPP

#include <xCore/x_files/x_threads.hpp>

// PC semaphore constructors and blocking paths establish two eight-byte
// thread lists at offsets 4 and 12, then count/available at 20 and 24.
class xsema
{
public:
    xsema(int count, int initial);
    ~xsema();
    int Acquire(int flags);
    int Release(int flags);
    void Acquire() { Acquire(X_TH_BLOCK); }
    void Release() { Release(X_TH_BLOCK); }

private:
    int m_Initialized;
    xthreadlist m_WaitingAcquire;
    xthreadlist m_WaitingRelease;
    int m_Count;
    int m_Available;
};

// Hobbit has the recursive owner/count form preserved in Area 51's
// x_mutex_private.hpp, not the older Tribes mutex waiting-list layout.
class xmutex
{
public:
    xmutex();
    ~xmutex();
    int Enter(int flags);
    int Exit(int flags);
    void Enter() { Enter(X_TH_BLOCK); }
    void Exit() { Exit(X_TH_BLOCK); }
    void Acquire() { Enter(X_TH_BLOCK); }
    void Release() { Exit(X_TH_BLOCK); }
    int Acquire(int flags) { return Enter(flags); }
    int Release(int flags) { return Exit(flags); }
    int IsLocked() { return m_EnterCount > 0; }

private:
    xthread* m_pOwner;
    int m_EnterCount;
    int m_Initialized;
    xsema m_Semaphore;
};

#endif
