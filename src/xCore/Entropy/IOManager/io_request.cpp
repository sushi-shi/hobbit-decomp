#include <rva.h>

#include <xCore/Entropy/IOManager/io_request.hpp>

RVA(0x00280660, 0x33)
io_request::io_request() : m_Semaphore(1, 0) {
    m_pPrev = 0;
    m_pNext = 0;
    m_pOpenFile = 0;
    m_pBuffer = 0;
    m_pCallback = 0;
    m_Status = NOT_QUEUED;
    m_Offset = 0;
    m_Length = 0;
    m_Sequence = 0;
    m_UserData = 0;
}

RVA(0x002806a0, 0x8)
io_request::~io_request() {}

RVA(0x002806b0, 0x4d)
void io_request::SetRequest(
    io_open_file* file,
    void* buffer,
    int offset,
    int length,
    priority requestPriority,
    int useSemaphore,
    unsigned int destination,
    unsigned int userData,
    callback_fn* callback
) {
    m_pOpenFile = file;
    m_pBuffer = buffer;
    m_Offset = offset;
    m_Length = length;
    m_ChunkOffset = 0;
    m_ChunkLength = 0;
    m_Priority = requestPriority;
    m_Status = NOT_QUEUED;
    m_UseSema = useSemaphore;
    m_Destination = destination;
    m_UserData = userData;
    m_pCallback = callback;
}
