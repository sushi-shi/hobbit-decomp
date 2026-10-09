#include <rva.h>
#include <xCore/Entropy/IOManager/io_device.hpp>
#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/x_files/x_files.hpp>
#include <xCore/x_files/x_log.hpp>
#include <xCore/Entropy/IOManager/io_filesystem.hpp>

// Genuine earlier PC class/storage owner. Remaining filesystem method bodies
// await revision recovery; full prior and later originals remain retained.
#if defined(TARGET_PC) && !defined(HOBBIT_IO_LATER_FILESYSTEM)

DATA(0x3f86f0)
static open_fn*     old_Open            = NULL;     // old filesystem functions
static close_fn*    old_Close           = NULL;
static read_fn*     old_Read            = NULL;
static write_fn*    old_Write           = NULL;
static seek_fn*     old_Seek            = NULL;
static tell_fn*     old_Tell            = NULL;
static flush_fn*    old_Flush           = NULL;
static eof_fn*      old_EOF             = NULL;
static length_fn*   old_Length          = NULL;

DATA(0x3f8714)
static xbool s_Initialized = FALSE;
DATA(0x3f8718)
static s32 s_MountedCount = 0;

// Natural implicit entry destructor; no authored destructor body.
RVA_COMPGEN(0x00276ec0, 0x56, ??1io_dfs_data@io_fs@@QAE@XZ)

DATA(0x003f72c0)
io_fs g_IOFSMgr;

RVA(0x2754c0, 0x95)
io_fs::io_fs( void )
{
    m_CurrentDFS      = -1;
    m_CurrentDFSIndex = 0;
    m_Retries         = 10;
    m_LogFlags        = 0;
}

//==============================================================================

RVA(0x275560, 0x8e)
io_fs::~io_fs( void )
{
}

//==============================================================================


static void io_clean_path( char* pClean, const char* pFilename )
{
    char* pOrig = pClean;

    // Skip over leading "."
    if( *pFilename == '.' )
    {
        pFilename++;
    }

    // Skip any leading \ or /
    while( *pFilename == '\\' || *pFilename == '/' )
        pFilename++;

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

    *pClean = 0;

    x_strtoupper( pOrig );
}


// Complete source-backed methods, retained donor bodies plus bounded PC API revision.
RVA(0x2761b0, 0x36)
io_open_file* io_fs::AcquireFile( void )
{
    io_open_file* pResult;

    // Snag that bad boy!
    m_Mutex.Enter();

    // Error check.
#ifndef bhapgood
    ASSERT( m_FreeFiles );
#endif

    // Take one out of the free list.
    pResult = m_FreeFiles;
    
    // Any free files left?
    if( m_FreeFiles )
    {
        // Walk the list...
        m_FreeFiles = m_FreeFiles->pNext;
    }

    // Release it!
    m_Mutex.Exit();

    // Tell the world.
    return( pResult );
}

RVA(0x2761f0, 0x52)
void io_fs::ReleaseFile( io_open_file* pFile )
{
    // Error check.
    ASSERT( pFile >= &m_Files[ 0 ]         );
    ASSERT( pFile  < &m_Files[ MAX_FILES ] );

    // Snag that bad boy!
    m_Mutex.Enter();

    // Put it in the free list.
    if( (pFile >= &m_Files[ 0 ]) && (pFile < &m_Files[ MAX_FILES ]) )
    {
        // Nuke it.
        x_memset( pFile, 0, sizeof( io_open_file ) );

        // Put it in the free list...
        pFile->pNext = m_FreeFiles;
        m_FreeFiles  = pFile;
    }

    // Release it!
    m_Mutex.Exit();
}

RVA(0x276250, 0x2b)
void io_fs::SetRetries( s32 Retries )
{
    ASSERT( Retries > 0 );

    // Snag that bad boy!
    m_Mutex.Enter();

    // Set the retries.
    m_Retries = Retries;

    // Release it!
    m_Mutex.Exit();
}

RVA(0x276280, 0x1d)
void io_fs::InvalidateCaches( void )
{
    io_cache* pCache;
    s32       i;

    // Find this threads cache.
    for( i=0, pCache=m_Caches ; i<NUM_CACHES ; i++, pCache++ )
    {
        // Invalidate the cache.
        pCache->Invalidate();
    }
}

RVA(0x2762a0, 0xef)
io_cache* io_fs::AcquireCache( io_open_file* pOpenFile )
{
    s32       ThreadID = x_GetThreadID();
    io_cache* pResult  = NULL;
    io_cache* pCache;
    s32       i;

    // Find this threads cache.
    for( i=0, pCache=m_Caches ; (i<NUM_CACHES && pResult==NULL) ; i++, pCache++ )
    {
        // Threads match?
        if( pCache->GetThreadID() == ThreadID )
            pResult = pCache;
    }

    // Not in the list?
    if( !pResult )
    {
        s64 SmallTicks = (s64)(((u64)(-1)) >> 1);

        // Find LRU cache then...
        for( i=0, pCache=m_Caches ; (i<NUM_CACHES && pResult==NULL) ; i++, pCache++ )
        {
            if( pCache->GetTicks() < SmallTicks )
            {
                // New smaller ticks!
                SmallTicks = pCache->GetTicks();

                // This is the one so far...
                pResult = pCache;
            }
        }
    }

    // Aquire the the cache.
    ASSERT( pResult );
    pResult->GetSemaphore()->Acquire();

    // Set ticks.
    pResult->SetTicks( x_GetTime() );

    // If different threads OR files, then invalidate the cache.
    if( (pResult->GetThreadID() != ThreadID) || x_strcmp( pResult->Filename, pOpenFile->pDeviceFile->Filename ) ) 
        pResult->Invalidate();

    // Set the last thread ID.
    pResult->SetThreadID( ThreadID );

    // Set the filename
    x_strcpy( pResult->Filename, pOpenFile->pDeviceFile->Filename );

    // Tell the world...
    return( pResult );
}

RVA(0x276390, 0xe)
void io_fs::ReleaseCache( io_cache* pCache )
{
    ASSERT( pCache );
    pCache->GetSemaphore()->Release();
}

RVA(0x2763a0, 0x117)
xbool io_fs::CompareFile( const char* pPathName, io_device_file* &DeviceFile, u32 &Offset, u32 &Length, s32 SubFile, s32 Index )
{
    dfs_header* pHeader = m_DFS[SubFile].pHeader;
    dfs_file*   pEntry  = pHeader->pFiles + Index;
    const char* p1      = pPathName;
    char*       p2;

    // Compare the path...
    p2 = pHeader->pStrings + (u32)pEntry->PathNameOffset;
    while( *p2 )
    {
        if( *p1++ != *p2++ )
            return FALSE;
    }

    // Compare the filename (part 1)...
    p2 = pHeader->pStrings + (u32)pEntry->FileNameOffset1;
    while( *p2 )
    {
        if( *p1++ != *p2++ )
            return FALSE;
    }

    // Compare the filename (part 2)...
    p2 = pHeader->pStrings + (u32)pEntry->FileNameOffset2;
    while( *p2 )
    {
        if( *p1++ != *p2++ )
            return FALSE;
    }

    // Compare the extension...
    p2 = pHeader->pStrings + (u32)pEntry->ExtNameOffset;
    while( *p2 )
    {
        if( *p1++ != *p2++ )
            return FALSE;
    }

    // Make sure terminating 0 matches...
    if( *p1 != *p2 )
        return FALSE;

    // Set the offset and length.
    Offset = pEntry->DataOffset;
    Length = pEntry->Length;

    // Now figure out which device file.
    s32             nSubFiles = pHeader->nSubFiles;
    dfs_subfile*    pSubFile  = pHeader->pSubFileTable;
    s32             i         = 0;
    while( i<nSubFiles )
    {
        // This the right file?
        if( Offset < (pSubFile[i].Offset ) )
        {
            // Earlier PC disk-backed path; complete later RAM path retained.
            DeviceFile = m_DFS[SubFile].DeviceFiles[ i ];
            if( i>0 )
                Offset -= pSubFile[i-1].Offset;

            // Woot!
            return TRUE;
        }
        else
        {
            // Next!!!
            i++;
        }
    }

    // Should never get here...
    ASSERT( 0 );
    return FALSE;
}

RVA(0x2764c0, 0xa2)
xbool io_fs::SearchDFS( const char* pPathName, io_device_file* &DeviceFile, u32 &Offset, u32 &Length, s32 SubFile, s32 StartIndex )
{
    s32 nFiles  = m_DFS[SubFile].pHeader->nFiles;
    s32 iMinus  = StartIndex;
    s32 iPlus   = StartIndex+1;

    // Search thru 'em all!
    while( (iMinus >= 0) || (iPlus < nFiles) )
    {
        if( iMinus >= 0 )
        {
            // Does this one match?
            if( CompareFile( pPathName, DeviceFile, Offset, Length, SubFile, iMinus ) )
            {
                m_CurrentDFSIndex = iMinus;
                return TRUE;
            }
            else
            {
                // Search lower next time.
                iMinus--;
            }
        }

        if( iPlus < nFiles )
        {
            // Does this one match?
            if( CompareFile( pPathName, DeviceFile, Offset, Length, SubFile, iPlus ) )
            {
                m_CurrentDFSIndex = iMinus;
                return TRUE;
            }
            else
            {
                // Search higher next time.
                iPlus++;
            }
        }
    }

    // Earlier PC resets the cached index only on failure.
    m_CurrentDFSIndex = 0;
    return FALSE;
}

RVA(0x276570, 0xe6)
xbool io_fs::FindFile( const char* pPathName, io_device_file* &DeviceFile, u32 &Offset, u32 &Length )
{
    xbool Result = FALSE;
    char  pCleanFilename[X_MAX_PATH];

    // Snag that bad boy!
    m_Mutex.Enter();

    // Default is failure.
    DeviceFile = NULL;
    Offset     = 0;
    Length     = 0;

    // Clean the filename.
    ASSERT( pPathName );
    io_clean_path( pCleanFilename, pPathName );

    // Current DFS valid?
    if( m_CurrentDFS != -1 )
    {
        // Look in the current disk file system...
        Result = SearchDFS( pCleanFilename, DeviceFile, Offset, Length, m_CurrentDFS, m_CurrentDFSIndex );
    }

    // Don't look further if we found it...
    if( !Result )
    {
        s32 nFileSystems = m_DFS.GetCount();
        // TODO: Prioritize the search.
        for( s32 i=nFileSystems-1 ; (i>=0) && (!Result) ; i-- )
        {
            // Don't look in the one we have already searched.
            if( i != m_CurrentDFS )
            {
                // Is it there?
                Result = SearchDFS( pCleanFilename, DeviceFile, Offset, Length, i, 0 );
                if( Result )
                    m_CurrentDFS = i;
            }
        }
    }

    // Let it go!
    m_Mutex.Exit();

    // Tell the world.
    return Result;
}


// Complete earlier-PC Open behavior; full later method remains in retained reference.
RVA(0x276660, 0x1f2)
io_open_file* io_fs::Open( const char* pPathName, const char* pMode )
{
    io_open_file* pOpenFile = NULL;
    xbool bRead = FALSE;
    xbool bWrite = FALSE;
    xbool bAppend = FALSE;
    m_Mutex.Enter();
    const char* pModeLocal = pMode;
    while( *pModeLocal )
    {
        if( (*pModeLocal == 'r') || (*pModeLocal == 'R') ) bRead = TRUE;
        if( (*pModeLocal == 'w') || (*pModeLocal == 'W') ) bWrite = TRUE;
        if( (*pModeLocal == 'a') || (*pModeLocal == 'A') ) bAppend = TRUE;
        ++pModeLocal;
    }
    if( !s_Initialized )
        return NULL;
    if( bWrite )
    {
        pOpenFile = AcquireFile();
        if( old_Open )
        {
            pOpenFile->PassThrough = old_Open( pPathName, pMode );
            if( pOpenFile->PassThrough )
            {
                pOpenFile->bRead = FALSE;
                pOpenFile->bWrite = FALSE;
                pOpenFile->bAppend = FALSE;
                pOpenFile->pDeviceFile = NULL;
                pOpenFile->Offset = 0;
                pOpenFile->Length = 0;
                pOpenFile->Position = 0;
                pOpenFile->Mode = 0;
                pOpenFile->pNext = NULL;
                m_Mutex.Exit();
                return pOpenFile;
            }
        }
        ReleaseFile( pOpenFile );
        m_Mutex.Exit();
        return NULL;
    }
    if( s_MountedCount )
    {
        u32 Offset;
        u32 Length;
        io_device_file* pDeviceFile;
        if( FindFile( pPathName, pDeviceFile, Offset, Length ) )
        {
            pOpenFile = AcquireFile();
            if( pOpenFile )
            {
                pOpenFile->bRead = bRead;
                pOpenFile->bWrite = FALSE;
                pOpenFile->bAppend = bAppend;
                pOpenFile->PassThrough = NULL;
                pOpenFile->pDeviceFile = pDeviceFile;
                pOpenFile->Offset = Offset;
                pOpenFile->Length = Length;
                pOpenFile->Position = 0;
                pOpenFile->Mode = 0;
                pOpenFile->pNext = NULL;
                x_strncpy( pOpenFile->Filename, pPathName, 256 );
            }
        }
    }
    if( !pOpenFile )
    {
        io_device_file* pDeviceFile = g_IoMgr.OpenDeviceFile( pPathName, 0 );
        if( pDeviceFile )
        {
            pOpenFile = AcquireFile();
            if( pOpenFile )
            {
                pOpenFile->bRead = bRead;
                pOpenFile->bWrite = FALSE;
                pOpenFile->bAppend = bAppend;
                pOpenFile->PassThrough = NULL;
                pOpenFile->pDeviceFile = pDeviceFile;
                pOpenFile->Offset = 0;
                pOpenFile->Length = pDeviceFile->Length;
                pOpenFile->Position = 0;
                pOpenFile->Mode = 0;
                pOpenFile->pNext = NULL;
                x_strncpy( pOpenFile->Filename, pPathName, 256 );
            }
            else
                g_IoMgr.CloseDeviceFile( pDeviceFile );
        }
    }
    m_Mutex.Exit();
    return pOpenFile;
}

RVA(0x276860, 0x4c)
void io_fs::Close( io_open_file* pOpenFile )
{
    ASSERT( pOpenFile );

        ASSERT( pOpenFile->pDeviceFile );

        // Snag it.
        m_Mutex.Enter();

    #ifdef IO_FS_CLOSE
        LOG_MESSAGE( IO_FS_CLOSE, "Filename: %s (0x%08x)", pOpenFile->Filename, pOpenFile );
    #endif // IO_FS_CLOSE

    #ifdef DEBUG_IO
        x_DebugMsg( "FS Close: %08x\n", pOpenFile );
    #endif
        
        if( pOpenFile )
        {
            // Valid device file?
            if( pOpenFile->pDeviceFile )
            {
                // Only close it if its NOT a DFS file.
                if( !pOpenFile->pDeviceFile->pHeader )
                {
                    g_IoMgr.CloseDeviceFile( pOpenFile->pDeviceFile );
                }
            }
            else
            {
                ASSERTS( FALSE, "Close: NULL device file handle" );
            }

            // Give up the file!
            ReleaseFile( pOpenFile );
        }
        else
        {
            ASSERTS( FALSE, "Close: NULL file handle" );
        }

    // Let it go.
    m_Mutex.Exit();
}

#else
#include "reference/before-cohort19-iofs-PC/io_filesystem.cpp.inc"
#endif
