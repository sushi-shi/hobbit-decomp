#ifndef X_TIME_HPP
#define X_TIME_HPP

typedef __int64 xtick;

void x_TimeInit();
void x_TimeKill();
xtick x_GetTime();
xtick x_GetTicksPerMs();
xtick x_GetTicksPerSecond();
float x_TicksToMs(xtick ticks);
double x_TicksToSec(xtick ticks);
double x_GetTimeSec();

class xtimer
{
protected:
    xtick m_StartTime;
    xtick m_TotalTime;
    int m_Running;
    int m_NSamples;

public:
    xtimer();
    void Start();
    void Reset();
    xtick Stop();
    float StopMs();
    float StopSec();
    xtick Read() const;
    float ReadMs() const;
    float ReadSec() const;
    xtick Trip();
    float TripMs();
    float TripSec();
    int GetNSamples() const;
    float GetAverageMs() const;
};

#endif
