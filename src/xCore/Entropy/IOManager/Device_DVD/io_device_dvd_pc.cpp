#include <rva.h>

// Earlier Hobbit PC DVD revision reconstructed from surviving Area51 bodies.
// Evidence and retained incompatible revision: docs/imports/io-dvd-pc-recovery.json.
//==============================================================================
//
// DVD Device layer for PC
//
//==============================================================================
#include <xCore/x_files/x_target.hpp>
#if !defined(TARGET_PC)
#error This is not for this target platform. Check dependancy rules.
#endif

#include <xCore/Entropy/IOManager/Device_DVD/io_device_dvd.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/Entropy/IOManager/Device_DVD/io_device_dvd.hpp>
#include <stdio.h>

//==============================================================================
//
// CDROM Defines, Buffers, Etc
//
//==============================================================================

#define CDROM_CACHE_SIZE    (32*1024)
#define CDROM_NUM_FILES     (16)                        // TODO: CJG - Increase this to accomodate multiple CDFS's
#define CDROM_INFO_SIZE     (32)
#define CDROM_CACHE         ((void*)s_CdromCache)
#define CDROM_FILES         ((void*)s_CdromFiles)
#define CDROM_BUFFER_ALIGN  (0)
#define CDROM_OFFSET_ALIGN  (0)
#define CDROM_LENGTH_ALIGN  (0)

DATA(0x00409da8)
static char           s_CdromCache[ CDROM_CACHE_SIZE ] GCN_ALIGNMENT(CDROM_BUFFER_ALIGN);
DATA(0x00411da8)
static io_device_file s_CdromFiles[ CDROM_NUM_FILES ];

//==============================================================================
//
// Device definition.
//
//==============================================================================

DATA(0x00354410)
io_device::device_data s_DeviceData =
{
    "PC DVD",           // Name
    TRUE,               // IsSupported
    TRUE,               // IsReadable
    FALSE,              // IsWriteable
    CDROM_CACHE_SIZE,   // CacheSize
    CDROM_BUFFER_ALIGN, // BufferAlign
    CDROM_OFFSET_ALIGN, // OffsetAlign
    CDROM_LENGTH_ALIGN, // LengthAlign
    CDROM_NUM_FILES,    // NumFiles
    CDROM_INFO_SIZE,    // InfoSize
    CDROM_CACHE,        // pCache    
    CDROM_FILES         // pFilesBuffer
};

//==============================================================================

RVA(0x00287da0, 0x34)
static void ReadCallback( s32 Result, void* pFileInfo )
{
    (void)pFileInfo;

    // We are in the callback
    g_IODeviceDVD.EnterCallback();

    // Success?
    if( Result >= 0 )
    {
        // Its all good!
        ProcessEndOfRequest( &g_IODeviceDVD, io_request::COMPLETED );
    }
    else
    {
        // Ack failed!
        ProcessEndOfRequest( &g_IODeviceDVD, io_request::FAILED );
    }

    // Done with callback
    g_IODeviceDVD.LeaveCallback();
}

//==============================================================================

RVA(0x00287c20, 0x66)
void io_device_dvd::CleanFilename( char* pClean, const char* pFilename )
{
    // Gotta fit.
    ASSERT( x_strlen(pFilename) + x_strlen(m_Prefix) < IO_DEVICE_FILENAME_LIMIT );

    x_strcpy( pClean, m_Prefix );
    
    // Move to end of string.
    pClean += x_strlen( pClean );

    // Now clean it.
    while( *pFilename )
    {
        if( (*pFilename == '\\') || (*pFilename == '/') )
        {
            *pClean++ = '\\';
            pFilename++;

            while( *pFilename && ((*pFilename == '\\') || (*pFilename == '/')) )
                pFilename++;
        }
        else
        {
            *pClean++ = *pFilename++;
        }
    }

    // Terminate it.
    *pClean = 0;
}

//==============================================================================
RVA(0x00287c90, 0x5)
void io_device_dvd::Init( void )
{
    // Base class initialization
    io_device::Init();


}

//==============================================================================
RVA(0x00287ca0, 0x5)
void io_device_dvd::Kill( void )
{
    io_device::Kill();
}

//==============================================================================

RVA(0x00287cb0, 0x6)
io_device::device_data* io_device_dvd::GetDeviceData( void )
{
    return &s_DeviceData;
}


//==============================================================================
//==============================================================================
//================================ DVD Functions ===============================
//==============================================================================
//==============================================================================

RVA(0x00287cc0, 0x77)
xbool io_device_dvd::PhysicalOpen( const char* pFilename, io_device_file* pFile )
{
    // Clean the filename.
    char CleanFile[IO_DEVICE_FILENAME_LIMIT];
    CleanFilename( CleanFile, (char *)pFilename );

    pFile->Handle = fopen(CleanFile,"rb");
    // Open the file on the dvd.
    if( pFile->Handle )
    {
        // Get the length of the file.
        fseek( (FILE*)pFile->Handle, 0, SEEK_END );
        pFile->Length = ftell( (FILE*)pFile->Handle );
        fseek( (FILE*)pFile->Handle,0,SEEK_SET );
        // Woot!
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//==============================================================================

RVA(0x00287d40, 0x52)
xbool io_device_dvd::PhysicalRead( io_device_file* pFile, void* pBuffer, s32 Length, s32 Offset, s32 AddressSpace )
{
    s32 ReadLength;
	(void)AddressSpace;

    // Log the read.
    LogPhysRead( pFile, Length, Offset );    

    fseek( (FILE*)pFile->Handle, Offset, SEEK_SET );
    ReadLength = fread(pBuffer,1,Length,(FILE*)pFile->Handle);
    ReadCallback((ReadLength==Length),pFile->pHardwareData);

    // Tell the world.
    return ReadLength == Length;
}

//==============================================================================

RVA(0x00287de0, 0x11)
void io_device_dvd::PhysicalClose ( io_device_file* pFile )
{
    fclose((FILE*)pFile->Handle);
    // Close the file on the DVD.
}

