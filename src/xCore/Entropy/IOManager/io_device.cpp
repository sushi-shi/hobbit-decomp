#include <rva.h>

#include <xCore/Entropy/IOManager/io_device.hpp>

#include <xCore/Entropy/IOManager/io_filesystem.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/x_files/x_log.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

RVA(0x00280700, 0x1f)
void ProcessEndOfRequest(io_device* device, int status) {
    io_request* request = device->m_CurrentRequest;
    request->m_HardwareStatus = status;
    g_IoMgr.m_DispatcherMQ.Send(request, 0);
}

RVA(0x00280720, 0x162)
void io_device::ServiceDeviceCurrentRequest() {
    io_request* request = m_CurrentRequest;
    m_CurrentRequest = 0;
    if (request) {
        io_device_file* file = request->m_pOpenFile->pDeviceFile;
        int done = 0;
        switch (request->m_HardwareStatus) {
            case io_request::COMPLETED: {
                void* dest;
                void* src;
                int count;
                if (request->m_ChunkOffset < 0) {
                    src = static_cast<unsigned char*>(file->pBuffer) - request->m_ChunkOffset;
                    dest = request->m_pBuffer;
                    request->m_ChunkOffset = request->m_ChunkLength + request->m_ChunkOffset;
                    count = request->m_ChunkLength + request->m_ChunkOffset;
                    if (count > request->m_Length) {
                        count = request->m_Length;
                    }
                } else {
                    src = file->pBuffer;
                    dest = static_cast<unsigned char*>(request->m_pBuffer) + request->m_ChunkOffset;
                    if (request->m_ChunkOffset + request->m_ChunkLength > request->m_Length) {
                        count = request->m_Length - request->m_ChunkOffset;
                    } else {
                        count = request->m_ChunkLength;
                    }
                    request->m_ChunkOffset += count;
                }
                if ((dest != src) && (request->m_Destination == 0)) {
                    x_memcpy(dest, src, count);
                }
                if (request->m_ChunkOffset >= request->m_Length) {
                    done = 1;
                    request->m_Status = io_request::COMPLETED;
                }
                break;
            }
            case io_request::FAILED:
                done = 1;
                request->m_Status = io_request::FAILED;
                break;
        }
        if (done) {
            io_request* prev = request->m_pPrev;
            io_request* next = request->m_pNext;
            prev->m_pNext = next;
            next->m_pPrev = prev;
            request->m_pPrev = 0;
            request->m_pNext = 0;
            m_RequestCount--;
            request->m_CompleteTick = x_GetTime();
            xtick dispatchTicks = request->m_DispatchTick - request->m_QueueTick;
            xtick readTicks = request->m_CompleteTick - request->m_DispatchTick;
            float dispatchMS = x_TicksToMs(dispatchTicks);
            float readMS = x_TicksToMs(readTicks);
            float totalMS = dispatchMS + readMS;
            log_NULL(
                "ServiceDeviceCurrentRequest",
                "Request Complete! Total: %07.3fms, Dispatch: %07.3fms, Read: %07.3fms",
                totalMS,
                dispatchMS,
                readMS
            );
            file->ReferenceCount--;
            if (request->m_pCallback) {
                (*request->m_pCallback)(request);
            }
            if (request->m_UseSema) {
                request->m_Semaphore.Release(1);
            }
        }
    }
}

RVA(0x00280890, 0xdc)
void io_device::ServiceDeviceQueue() {
    io_request* request = m_RequestQueue.m_pNext;
    if (request != &m_RequestQueue) {
        io_device_file* file = request->m_pOpenFile->pDeviceFile;
        if (request->m_Status == io_request::PENDING) {
            request->m_DispatchTick = x_GetTime();
            request->m_Status = io_request::IN_PROGRESS;
            request->m_ChunkOffset = 0;
        }
        request->m_ChunkLength = m_CacheSize;
        int adjust;
        if (m_OffsetAlign) {
            adjust = (request->m_Offset + request->m_ChunkOffset) & (m_OffsetAlign - 1);
        } else {
            adjust = 0;
        }
        if (adjust) {
            request->m_ChunkOffset -= adjust;
        }
        if ((request->m_Offset + request->m_ChunkOffset + request->m_ChunkLength) > file->Length) {
            request->m_ChunkLength = file->Length - (request->m_ChunkOffset + request->m_Offset);
        }
        m_CurrentRequest = request;
        int length = request->m_ChunkLength;
        if (m_LengthAlign) {
            int mask = m_LengthAlign - 1;
            length = (request->m_ChunkLength + mask) & ~mask;
        }
        if (request->m_Destination == 0) {
            PhysicalRead(
                file,
                file->pBuffer,
                length,
                request->m_Offset + request->m_ChunkOffset,
                request->m_Destination
            );
        } else {
            PhysicalRead(
                file,
                static_cast<unsigned char*>(request->m_pBuffer) + request->m_ChunkOffset,
                length,
                request->m_Offset + request->m_ChunkOffset,
                request->m_Destination
            );
        }
    }
}

RVA(0x00280970, 0x5f)
io_device::io_device() : m_Semaphore(1) {
    m_IsSupported = 0;
}

RVA(0x002809d0, 0x59)
io_device::~io_device() {}

RVA(0x00280a30, 0x1b9)
void io_device::Init() {
    device_data* data = GetDeviceData();
    if (data->IsSupported) {
        io_device_file* file;
        int i;
        x_strncpy(m_Name, data->Name, 64);
        m_Name[63] = 0;
        m_IsSupported = data->IsSupported;
        m_IsReadable = data->IsReadable;
        m_IsWriteable = data->IsWriteable;
        m_CacheSize = data->CacheSize;
        m_BufferAlign = data->BufferAlign;
        m_OffsetAlign = data->OffsetAlign;
        m_LengthAlign = data->LengthAlign;
        m_NumFiles = data->NumFiles;
        m_HardwareDataSize = data->HardwareDataSize;
        m_pCache = data->pCache;
        m_pFilesBuffer = data->pFilesBuffer;
        m_CallbackLevel = 0;
        m_Prefix[0] = 0;
        for (i = 0, file = static_cast<io_device_file*>(m_pFilesBuffer); i < m_NumFiles;
             i++, file++) {
            file->pHardwareData = x_malloc_fn(
                m_HardwareDataSize,
                "C:\\projects\\meridian\\xCore\\entropy\\IOManager\\io_device.cpp",
                427
            );
            file->pNext = file + 1;
        }
        file--;
        file->pNext = 0;
        m_pFreeFiles = static_cast<io_device_file*>(m_pFilesBuffer);
        m_RequestCount = 0;
        m_Sequence = 0;
        m_CurrentRequest = 0;
        m_RequestQueue.m_pNext = &m_RequestQueue;
        m_RequestQueue.m_pPrev = &m_RequestQueue;
        m_RequestQueue.m_Priority = io_request::NUM_PRIORITIES;
        m_Semaphore.Send(0, 1);
    } else {
        x_strncpy(m_Name, "UNSUPPORTED", 64);
        m_DeviceIndex = -1;
        m_IsSupported = 0;
        m_IsReadable = 0;
        m_IsWriteable = 0;
        m_CacheSize = 0;
        m_BufferAlign = 0;
        m_OffsetAlign = 0;
        m_LengthAlign = 0;
        m_NumFiles = 0;
        m_pCache = 0;
        m_pFilesBuffer = 0;
        m_HardwareDataSize = 0;
        m_pFreeFiles = 0;
        m_RequestCount = 0;
        m_Sequence = 0;
        m_RequestQueue.m_pNext = &m_RequestQueue;
        m_RequestQueue.m_pPrev = &m_RequestQueue;
        m_CurrentRequest = 0;
        m_CallbackLevel = 0;
        m_Prefix[0] = 0;
    }
}

RVA(0x00280bf0, 0x58)
void io_device::Kill() {
    if (m_IsSupported) {
        io_device_file* file;
        int i;
        for (i = 0, file = static_cast<io_device_file*>(m_pFilesBuffer); i < m_NumFiles;
             i++, file++) {
            if (file->pHardwareData) {
                x_free_fn(
                    file->pHardwareData,
                    "C:\\projects\\meridian\\xCore\\entropy\\IOManager\\io_device.cpp",
                    495
                );
            }
            file->pHardwareData = 0;
        }
    }
}

RVA(0x00280c50, 0xa7)
io_device_file* io_device::OpenFile(const char* filename) {
    io_device_file* file;
    io_device* device = this;
    device->m_Semaphore.Recv(1);
    file = m_pFreeFiles;
    if (file) {
        file->pDevice = device;
        if (PhysicalOpen(filename, file)) {
            file->pBuffer = device->m_pCache;
            file->BufferValid = 0;
            m_pFreeFiles = m_pFreeFiles->pNext;
            file->ReferenceCount = 0;
            file->IsOpen = 1;
            file->pHeader = 0;
        } else {
            file = 0;
        }
    }
    if (file) {
        x_strncpy(file->Filename, filename, 256);
    }
    device->m_Semaphore.Send(0, 1);
    return file;
}

RVA(0x00280d00, 0x58)
void io_device::CloseFile(io_device_file* file) {
    io_device* device = this;
    device->m_Semaphore.Recv(1);
    PhysicalClose(file);
    file->Handle = 0;
    file->Length = 0;
    file->BufferValid = 0;
    file->IsOpen = 0;
    file->pDevice = 0;
    file->pBuffer = 0;
    file->pNext = m_pFreeFiles;
    m_pFreeFiles = file;
    device->m_Semaphore.Send(0, 1);
}

RVA(0x00280d60, 0x10)
int io_device::GetFileDevice(io_device_file* file) const {
    return file->pDevice->m_DeviceIndex;
}

RVA(0x00280d70, 0x7)
int io_device::GetDeviceQueueStatus() const {
    return m_RequestCount;
}

RVA(0x00280d80, 0x42)
void io_device::SetPathPrefix(const char* prefix) {
    m_Semaphore.Recv(1);
    x_strncpy(m_Prefix, prefix, 256);
    m_Prefix[255] = 0;
    m_Semaphore.Send(0, 1);
}

RVA(0x00280dd0, 0x3)
void io_device::CleanFilename(char* clean, char* filename) {
    static_cast<void>(clean);
    static_cast<void>(filename);
}

RVA(0x00280de0, 0x3)
io_device::device_data* io_device::GetDeviceData() {
    return 0;
}

RVA(0x00280df0, 0x5)
int io_device::PhysicalOpen(const char* filename, io_device_file* file) {
    static_cast<void>(filename);
    static_cast<void>(file);
    return 0;
}

RVA(0x00280e00, 0x5)
int io_device::PhysicalRead(
    io_device_file* file,
    void* buffer,
    int length,
    int offset,
    int addressSpace
) {
    static_cast<void>(file);
    static_cast<void>(buffer);
    static_cast<void>(length);
    static_cast<void>(offset);
    static_cast<void>(addressSpace);
    return 0;
}

RVA(0x00280e10, 0x3)
void io_device::PhysicalClose(io_device_file* file) {
    static_cast<void>(file);
}

RVA_COMPGEN(0x00280e20, 0x1e, ??_Gio_device@@UAEPAXI@Z)
