#ifndef HOBBIT_IO_CACHE_HPP
#define HOBBIT_IO_CACHE_HPP

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
