#include <rva.h>

#include <xCore/Entropy/IOManager/io_cache.hpp>

#include <xCore/Entropy/IOManager/io_device.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/x_files/x_memory.hpp>

RVA(0x00280590, 0x4a)
io_cache::io_cache() : m_Semaphore(1, 1) {
    m_Ticks = 0;
    m_FirstByte = static_cast<__int64>(static_cast<unsigned __int64>(-1) >> 1);
    m_LastThreadID = 0;
    m_BytesCached = 0;
    m_CacheSize = 0;
    m_IsCacheValid = 0;
    m_pCacheMemory = 0;
    m_pCacheData = 0;
}

RVA(0x002805e0, 0x5)
io_cache::~io_cache() {}

RVA(0x002805f0, 0x41)
void io_cache::Init() {
    m_CacheSize = g_IoMgr.m_Devices[0]->m_CacheSize;
    m_pCacheMemory = x_malloc_fn(
        m_CacheSize + 256,
        "C:\\projects\\meridian\\xCore\\entropy\\IOManager\\io_cache.cpp",
        29
    );
    m_pCacheData =
        reinterpret_cast<unsigned char*>((reinterpret_cast<int>(m_pCacheMemory) + 255) & -256);
}

RVA(0x00280640, 0x17)
void io_cache::Kill() {
    x_free_fn(
        m_pCacheMemory,
        "C:\\projects\\meridian\\xCore\\entropy\\IOManager\\io_cache.cpp",
        38
    );
}
