#include <rva.h>

#include <xCore/Entropy/IOManager/io_mgr.hpp>

#include <xCore/Entropy/IOManager/io_device.hpp>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/Entropy/IOManager/Device_DVD/io_device_dvd.hpp>
#include <xCore/x_files/x_files.hpp>

DATA(0x003f8790)
static int s_Initialized = 0;

DATA(0x003f8794)
static int s_DispatcherActive = 0;

DATA(0x003f8728)
io_mgr g_IoMgr;

RVA(0x00276f60, 0xc8)
void io_dispatcher() {
    s_DispatcherActive = 1;
    while (1) {
        io_device_file* file = 0;
        io_request* curr = 0;
        io_request* request = 0;
        io_device* device = 0;
        int serviceQueue = 1;
        int serviceRequest = 1;
        request = static_cast<io_request*>(g_IoMgr.m_DispatcherMQ.Recv(1));
        file = request->m_pOpenFile->pDeviceFile;
        device = request->m_pOpenFile->pDeviceFile->pDevice;
        device->m_Semaphore.Recv(1);
        if (request->m_Status == io_request::QUEUED) {
            request->m_Status = io_request::PENDING;
            file->ReferenceCount++;
            serviceRequest = 0;
            request->m_Sequence = device->m_Sequence++;
            curr = device->m_RequestQueue.m_pNext;
            while (request->m_Priority >= curr->m_Priority) {
                curr = curr->m_pNext;
            }
            request->m_pNext = curr;
            request->m_pPrev = curr->m_pPrev;
            curr->m_pPrev->m_pNext = request;
            curr->m_pPrev = request;
            device->m_RequestCount++;
            if (device->m_RequestCount != 1) {
                serviceQueue = 0;
            }
        }
        if (serviceRequest) {
            device->ServiceDeviceCurrentRequest();
        }
        if (serviceQueue) {
            device->ServiceDeviceQueue();
        }
        device->m_Semaphore.Send(0, 1);
    }
}

RVA(0x00277030, 0x11)
io_mgr::io_mgr() : m_DispatcherMQ(16) {}

RVA(0x00277050, 0x8)
io_mgr::~io_mgr() {}

RVA_DYNINIT(0x00276f20, 0xa, g_IoMgr)
RVA_DYNINIT(0x00276f30, 0xa, g_IoMgr)
RVA_DYNINIT(0x00276f40, 0xc, g_IoMgr)
RVA_DYNINIT(0x00276f50, 0xa, g_IoMgr)

RVA(0x00277060, 0xac)
s32 io_mgr::Init( void )
{
    // Error check.
    ASSERT( s_Initialized == FALSE );

    // Initialise some important xbox stuff
    //   (a hook is needed before DeviceCDROMOpen to launch a thread)
    // Set up the dvd
#if defined(ENABLE_NETFS)
    m_Devices[ IO_DEVICE_DVD ] = &g_IODeviceNET;
#else
    m_Devices[ IO_DEVICE_DVD ] = &g_IODeviceDVD;
#endif
    m_Devices[ IO_DEVICE_DVD ]->Init();

    // It's inited.
    s_Initialized = TRUE;

    // Create the io_mgr thread.
    m_pThread = new xthread( io_dispatcher, "io_mgr dispatcher", 8192, 2 );

    // Initialize file system
    g_IOFSMgr.Init();
    g_IOFSMgr.MountFileSystem( "Files", 1 );
#if defined(ENABLE_NETFS) && defined(TARGET_XBOX) && defined(X_LOGGING)
    g_LogControl.Enable = TRUE;
#endif

    // It's all good!
    return TRUE;
}

//==============================================================================

RVA(0x00277110, 0x3e)
s32 io_mgr::Kill( void )
{
    s32 i;

    // Error check.
    ASSERT( s_Initialized );

    // Destroy the io_mgr thread.
    delete m_pThread;

    // Shut down the file system
    g_IOFSMgr.Kill();

    // For each device...
    for( i=0 ; i<NUM_IO_DEVICES ; i++ )
    {
        // Kill each device.
        m_Devices[ i ]->Kill();
    }

    // Clear flag
    s_Initialized = FALSE;


    // ok its all good.
    // Destroy some xbox stuff
    //   (a hook is needed before DeviceCDROMClose to kill a thread)
    return 1;
}

//==============================================================================


RVA(0x00277150, 0x2d)
int io_mgr::QueueRequest(io_request* request) {
    request->m_Status = io_request::QUEUED;
    request->m_QueueTick = x_GetTime();
    g_IoMgr.m_DispatcherMQ.Send(request, 0);
    return 1;
}

RVA(0x00277180, 0x18)
io_device_file* io_mgr::OpenDeviceFile(const char* filename, int deviceIndex) {
    io_device* device = g_IoMgr.m_Devices[deviceIndex];
    return device->OpenFile(filename);
}

RVA(0x002771a0, 0x10)
void io_mgr::CloseDeviceFile(io_device_file* file) {
    file->pDevice->CloseFile(file);
}

RVA(0x002771b0, 0x21)
int io_mgr::GetDeviceQueueStatus(int deviceIndex) const {
    if (!s_Initialized) {
        return 0;
    }
    io_device* device = g_IoMgr.m_Devices[deviceIndex];
    return device->GetDeviceQueueStatus();
}

RVA(0x002771e0, 0x18)
void io_mgr::SetDevicePathPrefix(const char* prefix, int deviceIndex) {
    io_device* device = g_IoMgr.m_Devices[deviceIndex];
    device->SetPathPrefix(prefix);
}
