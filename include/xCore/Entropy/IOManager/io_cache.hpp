#ifndef HOBBIT_IO_CACHE_HPP
#define HOBBIT_IO_CACHE_HPP

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_mutex.hpp>
#include <xCore/x_files/x_time.hpp>

struct io_open_file;
// PC cache arrays use a 336-byte stride, including the 256-byte filename.
class io_cache {
public:
    io_cache();
    ~io_cache();
    void Init();
    void Kill();


    // Complete genuine donor inline APIs; existing PC fields, no layout changes.
inline xsema*           GetSemaphore                ( void )                { return &m_Semaphore; }
inline io_open_file*    GetFile                     ( void )                { return m_pFile; }
inline void             SetFile                     ( io_open_file* pFile ) { m_pFile = pFile; }
inline s64              GetTicks                    ( void )                { return m_Ticks; }
inline void             SetTicks                    ( s64 Ticks )           { m_Ticks = Ticks; }
inline s64              GetFirstByte                ( void )                { return m_FirstByte; }
inline void             SetFirstByte                ( s64 FirstByte )       { m_FirstByte = FirstByte; }
inline s32              GetThreadID                 ( void )                { return m_LastThreadID; }
inline void             SetThreadID                 ( s32 ThreadID )        { m_LastThreadID = ThreadID; }
inline s32              GetBytesCached              ( void )                { return m_IsCacheValid ? m_BytesCached : 0; }
inline s32              GetCacheSize                ( void )                { return m_CacheSize; }
inline xbool            IsCacheValid                ( void )                { return m_IsCacheValid; }
inline void             Invalidate                  ( void )                { m_BytesCached = 0; m_FirstByte = (s64)(((u64)(-1)) >> 1); m_IsCacheValid = FALSE; }
inline void             Validate                    ( s32 Bytes )           { m_BytesCached = Bytes; m_IsCacheValid = TRUE; }
inline u8*              GetBuffer                   ( void )                { return m_pCacheData; }
inline s32              GetSize                     ( void )                { return m_CacheSize; }

private:
    xsema m_Semaphore;
    xtick m_Ticks;
    __int64 m_FirstByte;
    io_open_file* m_pFile;
    char Filename[256];
    int m_LastThreadID;
    int m_BytesCached;
    int m_CacheSize;
    int m_IsCacheValid;
    void* m_pCacheMemory;
    unsigned char* m_pCacheData;
};
#endif
