#ifndef HOBBIT_X_MQUEUE_HPP
#define HOBBIT_X_MQUEUE_HPP

#include <xCore/x_files/x_threads.hpp>

#define MQ_NOBLOCK X_TH_NOBLOCK
#define MQ_BLOCK X_TH_BLOCK
#define MQ_JAM X_TH_JAM
#define X_MAX_MESSAGE_QUEUES 32
#define MAX_DEFAULT_MESSAGES 16

// PC accesses establish the four signed-short indices and the two thread
// lists. The constructor selects sixteen inline entries before allocating.
class xmesgq
{
public:
    xmesgq(int entries);
    ~xmesgq();
    void Clear();
    int Send(void* message, int flags);
    void* Recv(int flags, xmesgq* otherQueue = 0);
    int IsEmpty();
    int IsFull();
    int ValidEntries();

private:
    int m_Initialized;
    short m_Head;
    short m_Tail;
    short m_ValidEntries;
    short m_MaxEntries;
    int* m_pQueue;
    xthreadlist m_WaitingForRecv;
    xthreadlist m_WaitingForSend;
    int m_QueueBuffer[16];
};

void mq_DebugDump();

#endif
