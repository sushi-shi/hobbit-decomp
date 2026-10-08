#include <rva.h>

#include <xCore/x_files/x_time.hpp>

#include <windows.h>

// Names are inferred; PC stores a 32-bit monotonic millisecond value and
// separately records an eight-byte initial sample. See docs/x-time.md.
DATA(0x003cd000) static unsigned long s_LastTime;
DATA(0x003cd008) static xtick s_BaseTime;

RVA(0x0024cb30, 0x23)
void x_TimeInit()
{
    timeBeginPeriod(1);
    s_LastTime = timeGetTime();
    s_BaseTime = s_LastTime;
}

RVA(0x0024cb60, 0x1)
void x_TimeKill()
{
}

RVA(0x0024cb70, 0x1b)
xtick x_GetTime()
{
    unsigned long ticks = timeGetTime();
    if (ticks < s_LastTime)
        return s_LastTime;
    s_LastTime = ticks;
    return ticks;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cb90, 0x8)
xtick x_GetTicksPerMs()
{
    return 1;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cba0, 0x8)
xtick x_GetTicksPerSecond()
{
    return 1000;
}

RVA(0x0024cbb0, 0x5)
float x_TicksToMs(xtick ticks)
{
    return static_cast<float>(ticks);
}

RVA(0x0024cbc0, 0xb)
double x_TicksToSec(xtick ticks)
{
    return static_cast<double>(ticks) / 1000.0;
}

RVA(0x0024cbd0, 0x10)
double x_GetTimeSec()
{
    return x_TicksToSec(x_GetTime());
}

RVA(0x0024cbe0, 0x16)
xtimer::xtimer()
{
    m_Running = 0;
    m_StartTime = 0;
    m_TotalTime = 0;
    m_NSamples = 0;
}

RVA(0x0024cc00, 0x24)
void xtimer::Start()
{
    if (!m_Running)
    {
        m_StartTime = x_GetTime();
        m_Running = 1;
        ++m_NSamples;
    }
}

RVA(0x0024cc30, 0x14)
void xtimer::Reset()
{
    m_Running = 0;
    m_StartTime = 0;
    m_TotalTime = 0;
    m_NSamples = 0;
}

RVA(0x0024cc50, 0x33)
xtick xtimer::Stop()
{
    if (m_Running)
    {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = 0;
    }
    return m_TotalTime;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cc90, 0x30)
float xtimer::StopMs()
{
    if (m_Running)
    {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = 0;
    }
    return static_cast<float>(m_TotalTime);
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024ccc0, 0x36)
float xtimer::StopSec()
{
    if (m_Running)
    {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = 0;
    }
    return static_cast<float>(m_TotalTime) / 1000.0;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cd00, 0x30)
xtick xtimer::Read() const
{
    if (m_Running)
        return m_TotalTime + (x_GetTime() - m_StartTime);
    return m_TotalTime;
}

RVA(0x0024cd30, 0x51)
float xtimer::ReadMs() const
{
    xtick ticks;
    if (m_Running)
        ticks = m_TotalTime + (x_GetTime() - m_StartTime);
    else
        ticks = m_TotalTime;
    return static_cast<float>(ticks);
}

RVA(0x0024cd90, 0x5d)
float xtimer::ReadSec() const
{
    xtick ticks;
    if (m_Running)
        ticks = m_TotalTime + (x_GetTime() - m_StartTime);
    else
        ticks = m_TotalTime;
    return static_cast<float>(ticks) / 1000.0;
}

RVA(0x0024cdf0, 0x4a)
xtick xtimer::Trip()
{
    xtick ticks = 0;
    if (m_Running)
    {
        xtick now = x_GetTime();
        ticks = m_TotalTime + (now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = now;
        ++m_NSamples;
    }
    return ticks;
}

RVA(0x0024ce40, 0x58)
float xtimer::TripMs()
{
    xtick ticks = 0;
    if (m_Running)
    {
        xtick now = x_GetTime();
        ticks = m_TotalTime + (now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = now;
        ++m_NSamples;
    }
    return static_cast<float>(ticks);
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cea0, 0x5e)
float xtimer::TripSec()
{
    xtick ticks = 0;
    if (m_Running)
    {
        xtick now = x_GetTime();
        ticks = m_TotalTime + (now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = now;
        ++m_NSamples;
    }
    return static_cast<float>(ticks) / 1000.0;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cf00, 0x4)
int xtimer::GetNSamples() const
{
    return m_NSamples;
}

// @dead-code
// Zero-ref: no E8/E9 target or absolute VA to any byte of this PC body.
RVA(0x0024cf10, 0x1e)
float xtimer::GetAverageMs() const
{
    if (m_NSamples <= 0)
        return 0;
    return ReadMs() / m_NSamples;
}
