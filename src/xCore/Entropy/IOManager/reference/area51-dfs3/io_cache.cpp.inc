// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#include <xCore/x_files/x_files.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/Entropy/IOManager/io_cache.hpp>

//==============================================================================

io_cache::io_cache( void ) : m_Semaphore(1,1)
{
    m_Ticks        = 0;
    m_FirstByte    = (s64)(((u64)-1) >> 1);   
    m_LastThreadID = 0;   
    m_BytesCached  = 0;   
    m_CacheSize    = 0;
    m_IsCacheValid = FALSE;
    m_pCacheMemory = NULL;
    m_pCacheData   = NULL;
}

//==============================================================================

io_cache::~io_cache( void )
{
}

//==============================================================================

void io_cache::Init( void )
{
    MEMORY_OWNER( "io_cache::Init()" );
    m_CacheSize    = g_IoMgr.m_Devices[ IO_DEVICE_DVD ]->m_CacheSize;
    m_pCacheMemory = (void*)x_malloc( m_CacheSize + 256 );
    m_pCacheData   = (u8*)(((s32)m_pCacheMemory+255) & -256);
    ASSERTS( m_pCacheMemory, "Failed to allocate Cache buffer" );
}

//==============================================================================

void io_cache::Kill( void )
{
    x_free( m_pCacheMemory );
    m_pCacheMemory=NULL;
}
