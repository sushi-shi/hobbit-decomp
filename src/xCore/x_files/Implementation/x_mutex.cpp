#include <rva.h>

#include <xCore/x_files/x_mutex.hpp>

DATA(0x003e8784) int s_AcquiresDone = 0;
DATA(0x003e8788) int s_ReleasesDone = 0;
DATA(0x003e878c) int s_EntersDone = 0;
DATA(0x003e8790) int s_ExitsDone = 0;

RVA(0x00255850, 0x27)
xmutex::xmutex( void )
       :m_Semaphore( 1, 1 )
{
    m_Initialized = 1;
    m_EnterCount  = 0;
    m_pOwner      = 0;

}

RVA(0x00255880, 0x8)
xmutex::~xmutex( void )
{

}

RVA(0x00255890, 0x34)
int xmutex::Enter( int Flags )
{
    int status;

    if( m_pOwner!=x_GetCurrentThread() )
    {
        status = m_Semaphore.Acquire( Flags );
        if( !status )
            return 0;
        m_pOwner = x_GetCurrentThread();
    }
    m_EnterCount++;
    return 1;
}

RVA(0x002558d0, 0x20)
int xmutex::Exit( int Flags )
{

    m_EnterCount--;
    if( m_EnterCount==0 )
    {
        m_pOwner=0;
        m_Semaphore.Release( Flags );
    }
    return 1;
}

RVA(0x002558f0, 0x5d)
xsema::xsema(int count,int initial)
{
    m_Initialized       = 1;
    m_Count             = count;
    m_Available         = initial;

}

RVA(0x00255950, 0x4d)
xsema::~xsema(void)
{

}

RVA(0x002559a0, 0xb9)
int xsema::Acquire(int Flags)
{
    xthread* pThread;

    x_BeginAtomic();
    s_EntersDone++;
	if (Flags & 1)
	{
        while (m_Available == 0)
        {
			xthread *pThread;
			pThread = x_GetCurrentThread();
			pThread->Unlink();
			pThread->Link(m_WaitingAcquire);
			x_EndAtomic();
            pThread->Suspend(Flags,xthread::BLOCKED_ON_SEMAPHORE_ACQUIRE);
			x_BeginAtomic();
		}

    }
    else
    {
		if (m_Available == 0)
        {
			x_EndAtomic();
			return 0;
		}
    }

    m_Available--;
    pThread = m_WaitingRelease.UnlinkFirst();
    if (pThread)
    {
        pThread->Link();
        s_ReleasesDone++;
        x_EndAtomic();
        pThread->Resume(Flags);
    }
    else
    {
        x_EndAtomic();
    }
	return 1;

}

RVA(0x00255a60, 0xbe)
int xsema::Release(int Flags)
{
    xthread *pThread;

    x_BeginAtomic();
    s_ExitsDone++;

	if (Flags & 1)
	{
        while (m_Available == m_Count)
        {
			xthread *pThread;
			pThread = x_GetCurrentThread();
			pThread->Unlink();
			pThread->Link(m_WaitingRelease);
			x_EndAtomic();
            pThread->Suspend(Flags,xthread::BLOCKED_ON_SEMAPHORE_RELEASE);
			x_BeginAtomic();
		}
    }
    else
    {
        if (m_Available == m_Count)
		{
			x_EndAtomic();
			return 0;
		}
    }

    m_Available++;

    pThread = m_WaitingAcquire.UnlinkFirst();

    if (pThread)
    {
        s_AcquiresDone++;
        pThread->Link();
        x_EndAtomic();
        pThread->Resume(Flags);
    }
    else
    {
        x_EndAtomic();
    }
	return 1;
}

