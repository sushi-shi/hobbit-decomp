#ifndef HOBBIT_IO_DEVICE_HPP
#define HOBBIT_IO_DEVICE_HPP

#define IO_DEVICE_FILENAME_LIMIT 256

#include <xCore/Entropy/IOManager/io_request.hpp>
#include <xCore/x_files/x_mqueue.hpp>

class io_device;
struct io_device_file {
    void* pBuffer;
    int IsOpen;
    void* pHeader;
    void* Handle;
    int Length;
    int BufferValid;
    volatile int ReferenceCount;
    io_device* pDevice;
    void* pHardwareData;
    char Filename[256];
    io_device_file* pNext;
};
// PC accesses establish this natural 616-byte layout. RTTI and the actual
// eight-slot vtable establish the virtual interface.
class io_device {
    friend void ProcessEndOfRequest(io_device* device, int status);
    friend class io_cache;
    friend class io_mgr;
    friend void io_dispatcher();

public:
    struct device_data {
        char Name[64];
        int IsSupported;
        int IsReadable;
        int IsWriteable;
        int CacheSize;
        int BufferAlign;
        int OffsetAlign;
        int LengthAlign;
        int NumFiles;
        int HardwareDataSize;
        void* pCache;
        void* pFilesBuffer;
    };
    // Surviving original callback helpers; PC callback accesses field612.
    void EnterCallback() { m_CallbackLevel++; }
    void LeaveCallback() { m_CallbackLevel--; }
    io_device();
    virtual ~io_device();
    virtual void Init();
    virtual void Kill();

protected:
    char m_Name[64];
    char m_Prefix[256];
    int m_DeviceIndex;
    int m_IsSupported;
    int m_IsReadable;
    int m_IsWriteable;
    int m_CacheSize;
    int m_BufferAlign;
    int m_OffsetAlign;
    int m_LengthAlign;
    int m_NumFiles;
    int m_HardwareDataSize;
    void* m_pCache;
    void* m_pFilesBuffer;
    io_device_file* m_pFreeFiles;
    xmesgq m_Semaphore;
    int m_RequestCount;
    int m_Sequence;
    io_request m_RequestQueue;
    io_request* m_CurrentRequest;
    volatile int m_CallbackLevel;

    io_device_file* OpenFile(const char* filename);
    void CloseFile(io_device_file* file);
    int GetFileDevice(io_device_file* file) const;
    int GetDeviceQueueStatus() const;
    void ServiceDeviceQueue();
    void ServiceDeviceCurrentRequest();
    void SetPathPrefix(const char* prefix);
    virtual device_data* GetDeviceData();
    virtual void CleanFilename(char* clean, char* filename);
    virtual int PhysicalOpen(const char* filename, io_device_file* file);
    virtual int
    PhysicalRead(io_device_file* file, void* buffer, int length, int offset, int addressSpace);
    virtual void PhysicalClose(io_device_file* file);
};
#endif
