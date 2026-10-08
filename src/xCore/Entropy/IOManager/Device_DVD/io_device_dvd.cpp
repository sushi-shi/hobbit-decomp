#include <rva.h>

// Earlier Hobbit PC DVD revision reconstructed from surviving Area51 bodies.
// Evidence and retained incompatible revision: docs/imports/io-dvd-pc-recovery.json.
#include <xCore/Entropy/IOManager/Device_DVD/io_device_dvd.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>
#include <xCore/Entropy/IOManager/Device_DVD/io_device_dvd.hpp>
#include <xCore/x_files/x_log.hpp>

//#define LOG_PHYSICAL_READ  "io_device_dvd::PhysicalRead(read)"
//#define LOG_PHYSICAL_SEEK  "io_device_dvd::PhysicalRead(seek)"
//#define LOG_PHYSICAL_WRITE "io_device_dvd::PhysicalWrite"

//==============================================================================

DATA(0x00408f30)
io_device_dvd g_IODeviceDVD;

//==============================================================================

RVA(0x00280e80, 0x30)
io_device_dvd::io_device_dvd( void )
{
    m_pLastFile  = NULL;
    m_LastOffset = -1;
    m_LastLength = 0;
    m_nSeeks     = 0;           
}

//==============================================================================

RVA(0x00280eb0, 0xb)
io_device_dvd::~io_device_dvd( void )
{
}

//==============================================================================
RVA(0x00280ec0, 0x3)
void io_device_dvd::LogPhysRead( io_device_file* pFile, s32 Length, s32 Offset )
{
    (void)pFile;
    (void)Length;
    (void)Offset;

#ifdef LOG_PHYSICAL_SEEK
    // Same file?
    if( m_pLastFile != pFile )
    {
        LOG_MESSAGE( LOG_PHYSICAL_SEEK, "SEEK! Different File: %s", pFile->Filename );
        m_nSeeks++;
    }
    // Need to seek?
    else if( Offset != (m_LastOffset+m_LastLength) )
    {
        LOG_MESSAGE( LOG_PHYSICAL_SEEK, "SEEK! Prev Offset: %d, Len: %d, Curr Offset: %d, Len: %d, Delta %d", m_LastOffset, m_LastLength, Offset, Length, Offset-m_LastOffset );
        m_nSeeks++;
    }

    // Update 'em
    m_pLastFile  = pFile;
    m_LastOffset = Offset;
    m_LastLength = Length;
#endif // LOG_PHYSICAL_SEEK

#ifdef LOG_PHYSICAL_READ
    LOG_MESSAGE( LOG_PHYSICAL_READ, "READ! File: %s, Offset: %d, Length: %d", pFile->Filename, Offset, Length );
#endif // LOG_PHYSICAL_READ
}

//==============================================================================


RVA_DYNINIT(0x00280e40, 0xa, g_IODeviceDVD)
RVA_DYNINIT(0x00280e50, 0xa, g_IODeviceDVD)
RVA_DYNINIT(0x00280e60, 0xc, g_IODeviceDVD)
RVA_DYNINIT(0x00280e70, 0xa, g_IODeviceDVD)
RVA_COMPGEN(0x00280ed0, 0x1e, ??_Gio_device_dvd@@UAEPAXI@Z)
