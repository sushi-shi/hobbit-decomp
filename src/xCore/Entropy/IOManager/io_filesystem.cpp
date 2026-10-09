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
DATA(0x3f86f4)
static close_fn*    old_Close           = NULL;
DATA(0x3f86f8)
static read_fn*     old_Read            = NULL;
DATA(0x3f86fc)
static write_fn*    old_Write           = NULL;
DATA(0x3f8700)
static seek_fn*     old_Seek            = NULL;
DATA(0x3f8704)
static tell_fn*     old_Tell            = NULL;
DATA(0x3f8708)
static flush_fn*    old_Flush           = NULL;
DATA(0x3f870c)
static eof_fn*      old_EOF             = NULL;
DATA(0x3f8710)
static length_fn*   old_Length          = NULL;

DATA(0x3f8714)
static xbool s_Initialized = FALSE;
DATA(0x3f8718)
static s32 s_MountedCount = 0;

// Actual naturally emitted generic instantiation and implicit entry constructor.
RVA_COMPGEN(0x276c30, 0x164, ?SetCapacity@?$xarray@Uio_dfs_data@io_fs@@@@QAEXH@Z)
RVA_COMPGEN(0x276e90, 0x23, ??0io_dfs_data@io_fs@@QAE@XZ)

// Natural implicit entry destructor; no authored destructor body.
RVA_COMPGEN(0x00276ec0, 0x56, ??1io_dfs_data@io_fs@@QAE@XZ)

DATA(0x003f72c0)
io_fs g_IOFSMgr;

RVA(0x2756e0, 0x15)
static X_FILE* io_open( const char* pFileName, const char* pMode )
{
    return (X_FILE*)g_IOFSMgr.Open( pFileName, pMode );
}

//==============================================================================

RVA(0x275490, 0x30)
void io_close( X_FILE* pFile )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Close );
        old_Close( ((io_open_file*)pFile)->PassThrough );
        g_IOFSMgr.ReleaseFile( (io_open_file*)pFile );
    }
    else
    {
        g_IOFSMgr.Close( (io_open_file*)pFile );
    }
}

//==============================================================================

RVA(0x275700, 0x2b)
static s32 io_read( X_FILE* pFile, byte* pBuffer, s32 Bytes )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Read );
        return old_Read( ((io_open_file*)pFile)->PassThrough, pBuffer, Bytes );
    }
    else
    {
        return g_IOFSMgr.Read( (io_open_file*)pFile, pBuffer, Bytes );
    }
}

//==============================================================================

RVA(0x275730, 0x2b)
static s32 io_write( X_FILE* pFile, const byte* pBuffer, s32 Bytes )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Write );
        return old_Write( ((io_open_file*)pFile)->PassThrough, pBuffer, Bytes );
    }
    else
    {
        return g_IOFSMgr.Write( (io_open_file*)pFile, pBuffer, Bytes );
    }
}

//==============================================================================

RVA(0x275760, 0x48)
static s32 io_seek( X_FILE* pFile, s32 Offset, s32 Origin )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Seek );
        return old_Seek( ((io_open_file*)pFile)->PassThrough, Offset, Origin );
    }
    else
    {
        // (s32) cast is temporary warning fix
        s32 Position = (s32)((io_open_file*)pFile)->Position;
        s32 Length   = ((io_open_file*)pFile)->Length;
        s32 Result   = 0;

        switch( Origin )
        {
            case X_SEEK_SET: 
                Position = Offset;   
                break;

            case X_SEEK_CUR: 
                Position += Offset; 
                break;

            case X_SEEK_END: 
                Position = Length + Offset;
                break;

            default:
                ASSERT( 0 );
                Result = -1;
                break;
        }

        if( Position < 0 )
        {
            Result = -1;
        }
        else
        {
            ((io_open_file*)pFile)->Position = Position;
        }

        return Result;
    }
}

//==============================================================================

RVA(0x2757b0, 0x18)
static s32 io_tell( X_FILE* pFile )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Tell );
        return old_Tell( ((io_open_file*)pFile)->PassThrough );
    }
    else
    {
        // (s32) cast is temporary warning fix
        return (s32)((io_open_file*)pFile)->Position;
    }
}

//==============================================================================

RVA(0x2757d0, 0x18)
static s32 io_flush( X_FILE* pFile )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Flush );
        return old_Flush( ((io_open_file*)pFile)->PassThrough );
    }
    else
    {
        return 0;
    }
}

//==============================================================================

RVA(0x2757f0, 0x26)
static xbool io_eof( X_FILE* pFile )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_EOF );
        return old_EOF( ((io_open_file*)pFile)->PassThrough );
    }
    else
    {
        return ((io_open_file*)pFile)->Position >= ((io_open_file*)pFile)->Length;    
    }
}

//==============================================================================

RVA(0x275820, 0x19)
static s32 io_length( X_FILE* pFile )
{
    if( ((io_open_file*)pFile)->PassThrough )
    {
        ASSERT( old_Length );
        return old_Length( ((io_open_file*)pFile)->PassThrough );
    }
    else
    {
        return ((io_open_file*)pFile)->Length;
    }
}


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


RVA(0x2755f0, 0xe3)
xbool io_fs::Init( void )
{
    io_open_file* pOpenFile;
    s32           i;

    // Can't do this multiple times...
    ASSERT( !s_Initialized );

    // Its MINE! MINE! MINE!
    m_Mutex.Enter();

    // Initialize the free list.
    m_FreeFiles = m_Files;
    
    // Set up the file handles
    for( i=0, pOpenFile=m_Files ; i<MAX_FILES ; i++, pOpenFile++ )
    {
        // Nuke it.
        x_memset( pOpenFile, 0, sizeof(io_open_file) );

        // Build the list...
        pOpenFile->pNext = pOpenFile+1;
    }

    // Back up and terminate the list
    (--pOpenFile)->pNext = NULL;

    // Initialize the caches...
    for( i=0 ; i<NUM_CACHES ; i++ )
    {
        m_Caches[ i ].Init();
    }

    // Read old IOHooks
    x_GetFileIOHooks(  old_Open,
                       old_Close,
                       old_Read,
                       old_Write,
                       old_Seek,
                       old_Tell,
                       old_Flush,
                       old_EOF,
                       old_Length );

#if defined(TARGET_GCN) || defined(TARGET_PS2) || defined(TARGET_XBOX) || ( defined(TARGET_PC) && !defined(X_EDITOR) )
    // Set new IOHooks
    x_SetFileIOHooks(  io_open,
                       io_close,
                       io_read,
                       io_write,
                       io_seek,
                       io_tell,
                       io_flush,
                       io_eof,
                       io_length );
#endif


    // Set Initial Capacity of m_DFS now to reduce memory fragmentation
    m_DFS.SetCapacity( 16 );

    // All good now!
    s_Initialized = TRUE;

    // Ok, you can have it now!
    m_Mutex.Exit();

    return s_Initialized;
}

//==============================================================================

RVA(0x275840, 0xc2)
void io_fs::Kill( void )
{
    ASSERT( s_Initialized );

    // Unmount all the currently mounted file systems
    while( m_DFS.GetCount() > 0 )
    {
        UnmountFileSystem( m_DFS[0].PathName );
    }

    // Earlier PC releases DFS storage after unmounting the last filesystem.
    m_DFS.Clear();

    // Snag that bad boy!
    m_Mutex.Enter();

    // Restore old IOHooks
    x_SetFileIOHooks(  old_Open,
                       old_Close,
                       old_Read,
                       old_Write,
                       old_Seek,
                       old_Tell,
                       old_Flush,
                       old_EOF,
                       old_Length );
    
    // Nuke the caches.
    for( s32 i=0 ; i<NUM_CACHES ; i++ )
    {
        m_Caches[ i ].Kill();
    }

    // Let it go.
    m_Mutex.Exit();
    
    // Clear initialized
    s_Initialized = FALSE;
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
