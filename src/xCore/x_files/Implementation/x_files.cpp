#include <rva.h>

#include <xCore/x_files/Implementation/x_files_private.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_threads.hpp>
#include <xCore/x_files/x_time.hpp>

DATA(0x003e86f0) static x_thread_globals s_DummyGlobals;
DATA(0x003e8700) static char s_StringBuffer[128];
DATA(0x003e8780) static int s_Initialized;

RVA(0x002554a0, 0x2c)
x_thread_globals* x_GetThreadGlobals()
{
    xthread* thread = x_GetCurrentThread();
    if (!thread)
    {
        s_DummyGlobals.NextOffset = 0;
        s_DummyGlobals.StringBuffer = s_StringBuffer;
        s_DummyGlobals.BufferSize = sizeof(s_StringBuffer);
        return &s_DummyGlobals;
    }
    return thread->GetGlobals();
}

RVA(0x002554d0, 0x6)
int x_GetInitialized()
{
    return s_Initialized;
}

RVA(0x00255490, 0x9)
int x_GetThreadID()
{
    return x_GetCurrentThread()->GetId();
}


RVA(0x00255430, 0x25)
extern "C" void x_Init()
{
    s_Initialized++;
    if (s_Initialized == 1)
    {
        x_IOInit();
        x_MemInit();
        x_TimeInit();
        x_InitThreads();
    }
}

RVA(0x00255460, 0x24)
extern "C" void x_Kill()
{
    if (s_Initialized == 1)
    {
        x_KillThreads();
        x_TimeKill();
        x_MemKill();
        x_IOKill();
    }
    s_Initialized--;
}
