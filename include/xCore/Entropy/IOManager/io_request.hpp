#ifndef HOBBIT_IO_REQUEST_HPP
#define HOBBIT_IO_REQUEST_HPP

#include <xCore/x_files/x_mutex.hpp>
#include <xCore/x_files/x_time.hpp>

struct io_open_file;
class io_device;
// Hobbit omits the later operation field. Complete constructor/request bodies
// and the 120-byte array stride establish this natural layout.
class io_request {
    friend void ProcessEndOfRequest(io_device* device, int status);
    friend class io_mgr;
    friend void io_dispatcher();
    friend class io_device;
    friend class audio_stream_mgr;

public:
    enum status {
        NOT_QUEUED,
        QUEUED,
        PENDING,
        IN_PROGRESS,
        COMPLETED,
        FAILED
    };
    enum priority {
        HIGH_PRIORITY,
        MEDIUM_PRIORITY,
        LOW_PRIORITY,
        NUM_PRIORITIES
    };
    typedef void callback_fn(io_request* request);
    // Genuine source accessors using the independently proved PC fields.
    status GetStatus() { return m_Status; }
    int GetLength() { return m_Length; }
    io_request();
    ~io_request();
    void SetRequest(
        io_open_file* file,
        void* buffer,
        int offset,
        int length,
        priority requestPriority,
        int useSemaphore,
        unsigned int destination,
        unsigned int userData,
        callback_fn* callback = 0
    );


    // Complete genuine donor inline APIs; existing PC fields, no layout changes.
inline  void                AcquireSemaphore( void )            { m_Semaphore.Acquire(); }

private:
    io_request* m_pPrev;
    io_request* m_pNext;
    io_open_file* m_pOpenFile;
    priority m_Priority;
    void* m_pBuffer;
    int m_Offset;
    int m_Length;
    int m_ChunkOffset;
    int m_ChunkLength;
    callback_fn* m_pCallback;
    volatile status m_Status;
    int m_HardwareStatus;
    int m_Sequence;
    unsigned int m_UserData;
    int m_UseSema;
    xsema m_Semaphore;
    unsigned int m_Destination;
    xtick m_QueueTick;
    xtick m_DispatchTick;
    xtick m_CompleteTick;
};
#endif
