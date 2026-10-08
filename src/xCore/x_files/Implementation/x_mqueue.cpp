#include <rva.h>

#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_mqueue.hpp>

RVA(0x002554f0, 0x8a)
xmesgq::xmesgq( int nEntries )
{

    Clear();
    m_MaxEntries        = nEntries;

    if (m_MaxEntries > 16)

    {
        m_pQueue = static_cast<int*>(x_malloc_fn(m_MaxEntries * sizeof(int), "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_mqueue.cpp", 23));
    }
    else
    {
        m_pQueue = m_QueueBuffer;
    }

    m_Initialized = 1;

}

RVA(0x00255580, 0x22)
void xmesgq::Clear()
{
    m_Head = 0;
    m_Tail = 0;
    m_ValidEntries = 0;
    m_WaitingForRecv.Clear();
    m_WaitingForSend.Clear();
}

RVA(0x002555b0, 0x72)
xmesgq::~xmesgq(void)
{
    if (m_MaxEntries > 16)
        x_free_fn(m_pQueue, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_mqueue.cpp", 53);
    m_Initialized = 0;

}

RVA(0x00255630, 0xf4)
int xmesgq::Send(void *message,int flags)
{
	xthread *pThread;

	x_BeginAtomic();

    if ((flags & 1)==0)
    {

		if (IsFull())
		{
			x_EndAtomic();
			return 0;
		}
    }
    else
    {
		while (IsFull())
		{
			pThread = x_GetCurrentThread();
			pThread->Unlink();
			pThread->Link(m_WaitingForRecv);
			x_EndAtomic();
            pThread->Suspend(flags,xthread::BLOCKED_ON_MESSAGE_RECV);
			x_BeginAtomic();

		}
    }

    m_ValidEntries++;

    if (flags & 2)
    {
        m_Head--;
        if (m_Head < 0)
            m_Head = m_MaxEntries-1;
        m_pQueue[m_Head] = reinterpret_cast<int>(message);
    }
    else
    {
        m_pQueue[m_Tail] = reinterpret_cast<int>(message);
        m_Tail++;
        if (m_Tail >= m_MaxEntries)
            m_Tail = 0;
    }

    pThread = m_WaitingForSend.UnlinkFirst();
    if (pThread)
	{
        pThread->Link();
        x_EndAtomic();
        pThread->Resume(flags);
	}
	else
	{
		x_EndAtomic();
	}
    return 1;
}

RVA(0x00255730, 0xd4)
void *xmesgq::Recv(int flags, xmesgq* otherQueue)
{
	xthread *pThread;
    int message;

	x_BeginAtomic();

    if ((flags & 1)==0)
    {

		if (IsEmpty())
		{
			x_EndAtomic();
			return 0;
		}
    }
    else
    {
		while (IsEmpty())
		{
			pThread = x_GetCurrentThread();
			pThread->Unlink();
			pThread->Link(m_WaitingForSend);
			x_EndAtomic();
            pThread->Suspend(flags,xthread::BLOCKED_ON_MESSAGE_SEND);
			x_BeginAtomic();

		}
    }

    m_ValidEntries--;
    message = m_pQueue[m_Head];
    m_Head++;
    if (m_Head >= m_MaxEntries)
        m_Head = 0;

    if (otherQueue) otherQueue->Recv(flags, 0);

    pThread = m_WaitingForRecv.UnlinkFirst();
    if (pThread)
	{
        pThread->Link();
        x_EndAtomic();
        pThread->Resume(flags);
	}
	else
	{
		x_EndAtomic();
	}

    return reinterpret_cast<void*>(message);
}

RVA(0x00255810, 0xa)
int xmesgq::IsEmpty(void)
{

    return (m_ValidEntries==0);
}

RVA(0x00255820, 0xf)
int xmesgq::IsFull(void)
{

    return (m_ValidEntries == m_MaxEntries);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00255830, 0x5)
int xmesgq::ValidEntries(void)
{

    return m_ValidEntries;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00255840, 0x1)
void mq_DebugDump(void)
{
}
