// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#ifndef IO_DEVICE_DVD_HPP
#define IO_DEVICE_DVD_HPP

#include <xCore/Entropy/IOManager/io_device.hpp>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>

class io_device_dvd : public io_device
{

private:

io_device_file*         m_pLastFile;        // Last file read from (logging).
s32                     m_LastOffset;       // Offset of last read (logging).
s32                     m_LastLength;       // Length of last read (logging).
s32                     m_nSeeks;           // Number of seeks (logging).    

public:

virtual                ~io_device_dvd               ( void );
                        io_device_dvd               ( void );
virtual void            Init                        ( void );
virtual void            Kill                        ( void );

private:

        void            LogPhysRead                 ( io_device_file* pFile, s32 Length, s32 Offset );
        void            LogPhysWrite                ( io_device_file* pFile, s32 Length, s32 Offset );
virtual device_data*    GetDeviceData               ( void );
virtual void            CleanFilename               ( char* pClean, const char* pFilename );
virtual xbool           PhysicalOpen                ( const char* pFilename, io_device_file* pFile, io_device::open_flags OpenFlags );
virtual xbool           PhysicalRead                ( io_device_file* pFile, void* pBuffer, s32 Length, s32 Offset, s32 AddressSpace );
virtual xbool           PhysicalWrite               ( io_device_file* pFile, void* pBuffer, s32 Length, s32 Offset, s32 AddressSpace );
virtual void            PhysicalClose               ( io_device_file* pFile );

};

extern io_device_dvd g_IODeviceDVD;

#endif //IO_DEVICE_DVD_HPP