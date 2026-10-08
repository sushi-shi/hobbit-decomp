#include <rva.h>

// The PC call uses the noreturn API contract. VC6 retains an unreachable
// POP ESI afterward but emits no RET. The old SDK lacks this attribute,
// which VC6 requires on the first declaration.
extern "C" __declspec(dllimport) __declspec(noreturn) void __stdcall ExitThread(unsigned long);
#include <windows.h>
#include <xCore/x_files/x_threads.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_mutex.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <new>
#include <xCore/x_files/x_mqueue.hpp>

// Pinned VC6 <new> supplies placement delete for constructor failure cleanup.
RVA_COMPGEN(0x00025890, 0x1, ??3@YAXPAX0@Z)

// Tribes: AA's PC branch preserves these globals and this exact locking
// sequence. Area 51's later sys_thread implementation is different.
DATA(0x003e3570) int IntsDisabled;
DATA(0x003e86b0) CRITICAL_SECTION s_Critical;
DATA(0x003e86cc) int s_crit_initialized = 0;

// Natural 64-byte PC layout, established by InitMain, Init, x_InitThreads,
// and the placement storage immediately before the main-thread stack.
struct thread_vars
{
    xthreadlist m_RunList;
    int m_ThreadTicks;
    xmutex m_Lock;
    int m_TopThreadId;
    int m_InterruptCount;
    xthread* m_pAppMain;
};
DATA(0x003e35fc) static thread_vars* s_pThreadVars;

// PC debugger protocol: four 32-bit fields passed through RaiseException.
// SetThreadName is descriptive; the PC body proves the protocol and ABI.
struct thread_name_info
{
    DWORD type;
    const char* name;
    DWORD id;
    DWORD flags;
};

DATA(0x003e3668) xthread* pMainThread;

DATA(0x003e3670) static char s_ThreadVars[sizeof(thread_vars)];
DATA(0x003e3578) static char s_MainThread[sizeof(xthread)];
DATA(0x003e36b0) static char s_MainStack[20*1024];
DATA(0x003e86c8) static int s_Initialized;

// Xbox names tmq in x_threads.obj; PC startup constructs this 96-byte queue.
DATA(0x003e3608) xmesgq tmq(1);
RVA_DYNINIT(0x00253510, 0xa, tmq)
RVA_DYNINIT(0x00253520, 0xd, tmq)
RVA_DYNINIT(0x00253530, 0xc, tmq)
RVA_DYNINIT(0x00253540, 0xa, tmq)

void SetThreadName(DWORD id, const char* name);
void CreateThreadVars();
void CreateMainThread();

RVA(0x00253550, 0x36)
void x_InitThreads()
{
    CreateThreadVars();
    s_pThreadVars->m_ThreadTicks = 1000;
    s_pThreadVars->m_TopThreadId = 1;
    s_pThreadVars->m_InterruptCount = 0;
    s_Initialized = 1;
    CreateMainThread();
}

RVA(0x00253590, 0x59)
void x_KillThreads()
{
    x_BeginAtomic();
    xthread* thread;
    xthread* current;
    current = x_GetCurrentThread();
    while (1)
    {
        thread = s_pThreadVars->m_RunList.UnlinkFirst();
        if (!thread)
            break;
        if (thread == current)
            continue;
        thread->Link();
        delete thread;
    }
    s_pThreadVars = 0;
    s_Initialized = 0;
    x_EndAtomic();
}

RVA(0x002535f0, 0x3)
xthread::xthread()
{
}

// Source-port lifecycle API adapted from Area51 x_threads.cpp to the existing
// Hobbit PC state/handle fields. No PC spans are admitted for these additions.
// The later sibling's distinct TERMINATING state has no established PC value;
// Hobbit's termination request and terminal state are already explicit here.
int xthread::IsActive()
{
    return !m_NeedToTerminate && m_Status != TERMINATED;
}

void xthread::Kill()
{
    int commitSuicide = x_GetCurrentThread() == this;
    x_BeginAtomic();
    Unlink();
    m_Status = TERMINATED;
    x_EndAtomic();
    if (commitSuicide)
        ExitThread(0);
}

void xthread::SetPriority(int priority)
{
    m_Priority = priority;
    // Hobbit's Init passes this priority directly to the Win32 operation.
    SetThreadPriority(m_SysHandle, priority);
}

RVA(0x00253600, 0xf3)
void xthread::InitMain(void* stack, int size)
{
    x_BeginAtomic();
    pMainThread = this;
    m_pStack = stack;
    m_ThreadId = s_pThreadVars->m_TopThreadId;
    s_pThreadVars->m_TopThreadId++;
    m_Globals.NextOffset = 0;
    m_Globals.StringBuffer = static_cast<char*>(m_pStack);
    m_Globals.BufferSize = size > 1024 ? size - 1024 : size / 2;
    x_strcpy(m_Name, "AppMain");
    m_Priority = 0;
    m_BasePriority = 0;
    m_pNext = 0;
    m_pPrev = 0;
    m_pOwningQueue = 0;
    m_NeedToTerminate = 0;
    s_pThreadVars->m_ThreadTicks = 10000;
    m_Initialized = 1;
    x_memset(m_pStack, m_ThreadId, size);
    m_SysSemaphore = CreateSemaphore(0, 0, 128, 0);
    m_SysThreadId = GetCurrentThreadId();
    m_SysHandle = GetCurrentThread();
    Link();
    SetThreadPriority(m_SysHandle, 0);
    SetThreadName(m_SysThreadId, m_Name);
    SetThreadAffinityMask(m_SysHandle, 1);
    x_EndAtomic();
}

RVA(0x00253700, 0x74)
void SetThreadName(DWORD id, const char* name)
{
    thread_name_info info;
    info.type = 0x1000;
    info.name = name;
    info.id = id;
    info.flags = 0;
    __try
    {
        RaiseException(0x406d1388, 0, sizeof(info)/sizeof(DWORD), (const DWORD*)&info);
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}

RVA(0x00253780, 0x15a)
void xthread::Init(void (*entry)(void*), const char* name, int size, int priority, void* argument)
{
    x_BeginAtomic();
    m_pStack = x_malloc_fn(size,
              "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_threads.cpp", 328);
    s_pThreadVars->m_Lock.Enter(1);
    m_ThreadId = s_pThreadVars->m_TopThreadId;
    s_pThreadVars->m_TopThreadId++;
    m_Globals.BufferSize = size > 1024 ? size - 1024 : size / 2;
    m_Globals.StringBuffer = static_cast<char*>(m_pStack);
    m_Globals.NextOffset = 0;
    x_strncpy(m_Name, name, 64);
    m_Priority = priority;
    m_BasePriority = priority;
    m_pNext = 0;
    m_pPrev = 0;
    m_NeedToTerminate = 0;
    m_pOwningQueue = 0;
    s_pThreadVars->m_ThreadTicks = 10000;
    m_Initialized = 1;
    x_memset(m_pStack, m_ThreadId, size);
    m_SysSemaphore = CreateSemaphore(0, 0, 128, 0);
    DWORD id;
    m_SysHandle = CreateThread(0, size, (LPTHREAD_START_ROUTINE)entry, argument, CREATE_SUSPENDED, &id);
    m_SysThreadId = id;
    Link();
    SetThreadPriority(m_SysHandle, priority);
    SetThreadAffinityMask(m_SysHandle, 1);
    s_pThreadVars->m_Lock.Exit(1);
    ResumeThread(m_SysHandle);
    SetThreadName(id, m_Name);
    x_DebugMsg("xthread::xthread - initialized '%s', id 0x%x\n", m_Name, m_SysHandle);
    x_EndAtomic();
}

RVA(0x002538e0, 0xb9)
xthread::~xthread()
{
    x_DebugMsg("xthread::~xthread - killing '%s', id 0x%x\n", m_Name, m_SysHandle);
    m_NeedToTerminate = 1;
    while (m_Status != TERMINATED)
    {
        Resume(0);
        x_DelayThread(1);
    }
    m_Initialized = 0;
    s_pThreadVars->m_Lock.Enter(1);
    CloseHandle(m_SysHandle);
    CloseHandle(m_SysSemaphore);
    if (m_Globals.StringBuffer != m_pStack)
        x_free_fn(m_Globals.StringBuffer,
                  "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_threads.cpp", 502);
    x_free_fn(m_pStack,
              "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_threads.cpp", 505);
    s_pThreadVars->m_Lock.Exit(1);
}

RVA(0x002539a0, 0x29)
void xthread::Suspend(int flags, state status)
{
    CheckTermination();
    m_Status = status;
    WaitForSingleObject(m_SysSemaphore, INFINITE);
    CheckTermination();
}

RVA(0x002539d0, 0x1b)
void xthread::Resume(int flags)
{
    m_Status = RUNNING;
    ReleaseSemaphore(m_SysSemaphore, 1, 0);
}

RVA(0x002539f0, 0x2e)
void xthread::Exit(int result)
{
    x_BeginAtomic();
    m_Status = TERMINATED;
    Unlink(*m_pOwningQueue);
    x_EndAtomic();
    ExitThread(result);
}

// Helper name is descriptive: the PC body checks the termination request
// around blocking calls and exits with -1. It is not named in the Xbox map.
RVA(0x00253a20, 0xf)
void xthread::CheckTermination()
{
    if (m_NeedToTerminate)
        Exit(-1);
}

RVA(0x00253a30, 0x17)
void xthread::Link()
{
    m_Status = RUNNING;
    s_pThreadVars->m_RunList.Link(this);
}

RVA(0x00253a50, 0x1b)
void xthread::Unlink()
{
    s_pThreadVars->m_RunList.Unlink(this);
    m_Status = LIMBO;
}

RVA(0x00253a70, 0xd)
void xthread::Link(xthreadlist& list)
{
    list.Link(this);
}

RVA(0x00253a80, 0xd)
void xthread::Unlink(xthreadlist& list)
{
    list.Unlink(this);
}

RVA(0x00253a90, 0x41)
void xthread::SetFormatBufferSize(int size)
{
    if (m_Globals.StringBuffer != m_pStack)
        x_free_fn(m_Globals.StringBuffer,
                  "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_threads.cpp", 646);
    m_Globals.StringBuffer = static_cast<char*>(x_malloc_fn(size,
                  "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_threads.cpp", 649));
    m_Globals.BufferSize = size;
}

RVA(0x00253ae0, 0xe)
void xthreadlist::Clear()
{
    m_pHead = 0;
    m_pTail = 0;
}

RVA(0x00253af0, 0x8)
xthreadlist::xthreadlist()
{
    Clear();
}

RVA(0x00253b00, 0x1)
xthreadlist::~xthreadlist()
{
}

RVA(0x00253b10, 0x1f)
xthread* xthreadlist::Find(int id)
{
    xthread* thread = m_pHead;
    while (thread)
    {
        if (thread->GetSystemId() == id)
            break;
        thread = thread->m_pNext;
    }
    return thread;
}

RVA(0x00253b30, 0x33)
void xthreadlist::Link(xthread* thread)
{
    thread->m_pOwningQueue = this;
    if (!m_pHead)
    {
        m_pHead = thread;
        m_pTail = thread;
        thread->m_pPrev = 0;
        thread->m_pNext = 0;
    }
    else
    {
        m_pTail->m_pNext = thread;
        thread->m_pNext = 0;
        thread->m_pPrev = m_pTail;
        m_pTail = thread;
    }
}

RVA(0x00253b70, 0x50)
void xthreadlist::Unlink(xthread* thread)
{
    thread->m_pOwningQueue = 0;
    if (thread->m_pPrev)
        thread->m_pPrev->m_pNext = thread->m_pNext;
    else
        m_pHead = thread->m_pNext;
    if (thread->m_pNext)
        thread->m_pNext->m_pPrev = thread->m_pPrev;
    else
        m_pTail = thread->m_pPrev;
    thread->m_pNext = 0;
    thread->m_pPrev = 0;
}

RVA(0x00253bc0, 0x11)
xthread* xthreadlist::UnlinkFirst()
{
    xthread* thread = m_pHead;
    if (thread)
        Unlink(thread);
    return thread;
}

RVA(0x00253be0, 0x12)
void x_DelayThread(int milliseconds)
{
    xthread* thread = x_GetCurrentThread();
    thread->Delay(milliseconds);
}

RVA(0x00253c00, 0x1d)
void xthread::Delay(int milliseconds)
{
    if (milliseconds > 0)
    {
        Sleep(milliseconds);
        CheckTermination();
    }
}

// No inbound reference is present in the pinned PC image.
RVA(0x00253c40, 0xe)
void x_CheckThreads(int forceDump)
{
    if (forceDump)
        x_DumpThreads();
}

// The surviving diagnostic body is empty in both sibling implementations.
RVA(0x00253c60, 0x1)
void x_DumpThreads()
{
}

RVA(0x00253c70, 0x51)
xthread* x_GetCurrentThread()
{
    // The PC reloads this shared cache word after updating it; removing
    // volatile lets VC6 substitute the saved register and changes the body.
    DATA(0x0034d6e0) static volatile int s_LastThreadId = -1;
    DATA(0x003e3600) static xthread* pThread;
    if (!s_pThreadVars)
        return 0;
    int current = GetCurrentThreadId();
    x_BeginAtomic();
    xthread* result = pThread;
    if (s_LastThreadId != current)
    {
        s_LastThreadId = current;
        pThread = s_pThreadVars->m_RunList.Find(s_LastThreadId);
        result = pThread;
    }
    x_EndAtomic();
    return result;
}

// Tribes and Area51 watchdog reset retains the original forty-tick timeout.
RVA(0x00253cd0, 0xd)
void x_WatchdogReset()
{
    s_pThreadVars->m_ThreadTicks = 40;
}

RVA(0x00253cf0, 0x30)
void x_BeginAtomic()
{
    if (!s_crit_initialized)
    {
        InitializeCriticalSection(&s_Critical);
        s_crit_initialized = 1;
    }
    EnterCriticalSection(&s_Critical);
    IntsDisabled++;
}

RVA(0x00253d20, 0x19)
void x_EndAtomic()
{
    IntsDisabled--;
    LeaveCriticalSection(&s_Critical);
}

RVA(0x00253d90, 0x53)
void CreateThreadVars()
{
    new((void*)&s_ThreadVars) thread_vars;
    s_pThreadVars = (thread_vars*)&s_ThreadVars;
}

RVA(0x00253df0, 0x21)
void CreateMainThread()
{
    new((void*)&s_MainThread) xthread;
    ((xthread*)s_MainThread)->InitMain(s_MainStack, sizeof(s_MainStack));
    s_pThreadVars->m_pAppMain = (xthread*)s_MainThread;
}
