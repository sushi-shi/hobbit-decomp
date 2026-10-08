#ifndef HOBBIT_IO_MGR_HPP
#define HOBBIT_IO_MGR_HPP

#include <xCore/x_files/x_mqueue.hpp>

// Original shared device enumeration; the proved PC manager has one device.
enum { IO_DEVICE_DVD = 0, NUM_IO_DEVICES };

class io_device;
class io_request;
struct io_device_file;
// PC initialization establishes a thread pointer, one device, and the
// 96-byte dispatcher queue; the complete global occupies 104 bytes.
class io_mgr {
    friend void ProcessEndOfRequest(io_device* device, int status);
    friend class io_cache;
    friend void io_dispatcher();

public:
    io_mgr();
    ~io_mgr();
    int Init();
    int Kill();
    int QueueRequest(io_request* request);
    io_device_file* OpenDeviceFile(const char* filename, int deviceIndex);
    void CloseDeviceFile(io_device_file* file);
    int GetDeviceQueueStatus(int device) const;
    void SetDevicePathPrefix(const char* prefix, int deviceIndex);

private:
    xthread* m_pThread;
    io_device* m_Devices[1];
    xmesgq m_DispatcherMQ;
};
extern io_mgr g_IoMgr;
#endif
