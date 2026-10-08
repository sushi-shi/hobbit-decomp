#ifndef HOBBIT_X_THREADS_HPP
#define HOBBIT_X_THREADS_HPP

#include <xCore/x_files/Implementation/x_files_private.hpp>

#include <windows.h>

#define X_MAX_THREADS 16
#define X_TH_NOBLOCK (0 << 0)
#define X_TH_BLOCK (1 << 0)
#define X_TH_JAM (1 << 1)
#define X_TH_INTERRUPT (1 << 7)

void x_BeginAtomic();
void x_EndAtomic();
void x_InitThreads();
void x_KillThreads();
void x_CheckThreads(int forceDump);
void x_DumpThreads();
void x_WatchdogReset();
class xthread;
class xthreadlist
{
public:
    xthreadlist();
    ~xthreadlist();
    void Clear();
    void Link(xthread* thread);
    void Unlink(xthread* thread);
    xthread* Find(int id);
    xthread* UnlinkFirst();
private:
    xthread* m_pHead;
    xthread* m_pTail;
};

// Natural PC layout, independently checked against InitMain (0x253600),
// Init (0x253780), list operations, and SetFormatBufferSize (0x253a90).
class xthread
{
public:
    enum state
    {
        RUNNING = 0,
        TERMINATED = 2,
        LIMBO = 4,
        BLOCKED_ON_MESSAGE_SEND = 5,
        BLOCKED_ON_MESSAGE_RECV = 6,
        BLOCKED_ON_SEMAPHORE_ACQUIRE = 7,
        BLOCKED_ON_SEMAPHORE_RELEASE = 8
    };
    xthread();
    xthread(void (*entry)(void*), const char* name, int size, int priority, void* argument = 0)
    {
        Init(entry, name, size, priority, argument);
    }
    xthread(void (*entry)(), const char* name, int size, int priority)
    {
        Init((void (*)(void*))entry, name, size, priority, 0);
    }
    ~xthread();
    void InitMain(void* stack, int size);
    void Exit(int result);
    void CheckTermination();
    void Delay(int milliseconds);
    void Link();
    void Unlink();
    void Suspend(int flags, state status);
    void Resume(int flags);
    void Link(xthreadlist& list);
    void Unlink(xthreadlist& list);
    void SetFormatBufferSize(int size);
    int GetId() const { return m_ThreadId; }
    int GetSystemId() const { return m_SysThreadId; }
    const char* GetName() const { return m_Name; }
    int IsActive();
    void Kill();
    void SetPriority(int priority);
    int GetPriority() { return m_Priority; }
    x_thread_globals* GetGlobals() { return &m_Globals; }
protected:
    void Init(void (*entry)(void*), const char* name, int size, int priority, void* argument);
private:
    friend class xthreadlist;
    xthread* m_pPrev;
    xthread* m_pNext;
    char m_Name[64];
    int m_SysThreadId;
    HANDLE m_SysHandle;
    HANDLE m_SysSemaphore;
    xthreadlist* m_pOwningQueue;
    int m_Initialized;
    int m_NeedToTerminate;
    int m_ThreadId;
    int m_Priority;
    int m_BasePriority;
    x_thread_globals m_Globals;
    void* m_pStack;
    int m_Status;
};

xthread* x_GetCurrentThread();
void x_DelayThread(int milliseconds);

#endif
