#include <rva.h>
#include <windows.h>
#include <xCore/Entropy/e_Virtual.hpp>
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


RVA(0x275e20, 0x6d)
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


RVA(0x275e90, 0x10c)
static s32 io_read_sync( io_device_file* pFile, void* Buffer, s32 Offset, s32 Length )
{
    io_request      Request;
    io_open_file    OpenFile;

    OpenFile.pDeviceFile       = pFile;
    OpenFile.Offset            = 0;
    OpenFile.Length            = pFile->Length;
    OpenFile.Position          = 0;
    OpenFile.Mode              = 0; // TODO: Implement file mode.
    OpenFile.pNext             = NULL;

    // Set the request
    Request.SetRequest( &OpenFile, Buffer, Offset, Length, io_request::MEDIUM_PRIORITY, FALSE, 0, 0, 0 );   

    // Queue it up
    g_IoMgr.QueueRequest( &Request );

    // Wait for read to finish.
    while( Request.GetStatus() < io_request::COMPLETED )
    {
#if defined(TARGET_XBOX) || defined(TARGET_PC)
        Sleep( 1 );
#endif // TARGET_XBOX
    }

    // Success?
    if( Request.GetStatus() == io_request::COMPLETED )
        return Length;
    else
        return 0;
}

#define USE_VM_ALLOC
RVA(0x275910, 0x509)
xbool io_fs::MountFileSystem( const char* pPathName, s32 SearchPriority )
{
    io_device_file* pFile;
    dfs_header*     pHeader = NULL;
    xbool           bSuccess = FALSE;
    char            pCleanFilename[X_MAX_PATH];

    CONTEXT( "io_fs::MountFileSystem" );
    x_mem_owner __owner__( "io_fs::MountFileSystem" );

    // Snag that bad boy!
    m_Mutex.Enter();

    // Clean the filename.
    ASSERT( pPathName );
    io_clean_path( pCleanFilename, pPathName );

    // Open the filesystem header file.
    pFile = g_IoMgr.OpenDeviceFile( xfs("%s.DFS", pCleanFilename), IO_DEVICE_DVD );
    
    // Success?
    if( pFile )
    {
        // Allocate space for the header file.
#ifdef USE_VM_ALLOC
        pHeader = (dfs_header*)vm_Alloc( pFile->Length );
#else
        pHeader = (dfs_header*)x_malloc( pFile->Length );
#endif
        // Read the file system header
        if( io_read_sync( pFile, pHeader, 0, pFile->Length ) )
        {
            pHeader = dfs_InitHeaderFromRawPtr(pHeader);
            if( pHeader )
            {
                bSuccess = TRUE;
#ifdef bhapgood
                {
                    static s32 I=0;
                    //dfs_DumpFileListing( pHeader, xfs("T:\\DFS_DUMP%03d.txt",I));
                    I++;
                }
#endif
            }
        }

        // Did we fail?
        if( !bSuccess )
        {
            // Free ram and nuke the pointer.
#ifdef USE_VM_ALLOC
            vm_Free( pHeader );
#else
            x_free( pHeader );
#endif
            pHeader = NULL;
        }

        // Close file and nuke it.
        g_IoMgr.CloseDeviceFile( pFile );
        pFile = NULL;
    }

    // Header file loaded successfully?
    if( bSuccess )
    {
        s32 FileIndex;
        s32 j;

        // Get the index.
        FileIndex = m_DFS.GetCount();

        // Add one to the list.
        m_DFS.Append();

        // Set the DFS up.
        m_DFS[FileIndex].PathName       = pPathName;
        m_DFS[FileIndex].SearchPriority = SearchPriority;
        m_DFS[FileIndex].pHeader        = pHeader;

        // Make room for sub-files.
        m_DFS[FileIndex].DeviceFiles.SetCapacity( pHeader->nSubFiles );

        // Lets open up the sub-files now.
        for( j=0 ; (j<pHeader->nSubFiles) && bSuccess ; j++ )
        {
            // Open the sub-file.
            pFile = g_IoMgr.OpenDeviceFile( xfs("%s.%03d", pCleanFilename, j), IO_DEVICE_DVD );

            // Only if it was found!
            if( pFile )
            {
                // Mark it as a DFS file.
                pFile->pHeader      = pHeader;

                // Put it in the list.
                m_DFS[FileIndex].DeviceFiles.Append() = pFile;
            }
            else
            {
                // Oops!
                bSuccess = FALSE;
            }
        }

        // Failure to open one?
        if( !bSuccess )
        {
            // For each opened sub-file...
            for( j=0 ; j<m_DFS[FileIndex].DeviceFiles.GetCount() ; j++ )
            {
                // Close the bad boy...
                g_IoMgr.CloseDeviceFile( m_DFS[FileIndex].DeviceFiles[j] );
            }

            // Clean it up (the following m_DFS.Delete does NOT call a destructor!)
            m_DFS[FileIndex].DeviceFiles.Clear();

            // Free the header file now.
#ifdef USE_VM_ALLOC
            vm_Free( m_DFS[FileIndex].pHeader );
#else
            x_free( m_DFS[FileIndex].pHeader );
#endif
            // Now nuke it from the list.
            m_DFS.Delete( FileIndex );
        }
    }

    // Bump filesystem count if successful.
    if( bSuccess )
    {
        s_MountedCount++;

        // Let FileSystem search in latest mounted .dfs
        m_CurrentDFS = -1;

        //DumpFileSystem( m_DFS.GetCount() - 1 );
    }

    // Release the mutex!
    m_Mutex.Exit();

    
    // Tell the world.
    return bSuccess;
}

//==============================================================================

#undef USE_VM_ALLOC
// Complete source-backed methods, retained donor bodies plus bounded PC API revision.
RVA(0x275fa0, 0x208)
xbool io_fs::UnmountFileSystem( const char* pPathName )
{
    xbool bSuccess = FALSE;
    s32   i;
    char  pCleanFilename[X_MAX_PATH];

    CONTEXT( "io_fs::UnmountFileSystem" );

    // Snag that bad boy!
    m_Mutex.Enter();

    // Clean the filename.
    ASSERT( pPathName );
    io_clean_path( pCleanFilename, pPathName );

    // Search thru all the disk file systems.
    for( i=0 ; i<m_DFS.GetCount() ; i++ )
    {
        // Match?
        if( x_stricmp( m_DFS[i].PathName, pPathName ) == 0 )
            break;
    }
    
    // Find it?
    if( i<m_DFS.GetCount() )
    {
        // This the current file?
        if( i == m_CurrentDFS )
        {
            m_CurrentDFS      = -1;
            m_CurrentDFSIndex = 0;
        }

        // For each opened sub-file...
        for( s32 j=0 ; j<m_DFS[i].DeviceFiles.GetCount() ; j++ )
        {
            io_device_file* pFile = m_DFS[i].DeviceFiles[j];
            ASSERT( pFile );

            // Close the bad boy...
            g_IoMgr.CloseDeviceFile( pFile );

            // Look for a file reference (should not have one).
            for( s32 k=0 ; k<MAX_FILES ; k++ )
            {
                ASSERT( m_Files[ k ].pDeviceFile != pFile );
            }
        }

        // Clean it up (the following m_DFS.Delete does NOT call a destructor!)
        m_DFS[i].DeviceFiles.Clear();

        // Free the header file now.
        vm_Free( m_DFS[i].pHeader );
        // Now nuke it from the list.
        m_DFS.Delete( i );

        // One less file mounted...
        s_MountedCount--;
    }

    // Release it!
    m_Mutex.Exit();

    // Tell the world.
    return bSuccess;
}


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
        goto ReturnNull;
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
ReturnNull:
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

DATA(0x3f871c)
s32 g_IOFSReadBytesRequested = 0;
DATA(0x3f8720)
s32 g_IOFSReadBytesRead = 0;

RVA(0x2768b0, 0x31c)
s32 io_fs::Read( io_open_file* pOpenFile, byte* pBuffer, s32 Bytes )
{
    // Error check.
    ASSERT( pOpenFile );
    ASSERT( pBuffer );
    ASSERT( Bytes >= 0 );

#ifdef DEBUG_IO
    x_DebugMsg( "FS Read: %08x, Buffer: %08x, Bytes: %d\n", pOpenFile, pBuffer, Bytes );
#endif

    // Actual earlier PC evaluates the existing DFS header flag before counters.
    xbool IsFileSystemFile = (pOpenFile->pDeviceFile->pHeader != NULL);


    // Setup counters
    s32 BytesLeft    = Bytes;
    s32 BytesRead    = 0;
    s32 Position     = pOpenFile->Position;

    g_IOFSReadBytesRequested += Bytes;

        // Is this a read from the file system file?

        // Make sure read does not pass end of file
        BytesLeft = MIN( BytesLeft, pOpenFile->Length-Position );

    #ifdef DEBUG_IO
        x_DebugMsg( "   Acquiring Cache\n" );
    #endif

        // Acquire a cache. 
        io_cache* pCache = AcquireCache( pOpenFile );

    #ifdef DEBUG_IO
        x_DebugMsg( "   Acquired!\n" );
    #endif

        // Loop until all bytes have been read
        while( BytesLeft > 0 )
        {
            s32 PhysicalByte = pOpenFile->Offset + Position;

            // Check if there is data in the cache, if so copy it, else fill cache.
            if( pCache->IsCacheValid() &&
                (PhysicalByte >= pCache->GetFirstByte()) &&
                (PhysicalByte <= (pCache->GetFirstByte() + pCache->GetBytesCached()-1)) )
            {
                CONTEXT("IOFS - ReadCacheHit");

                // Determine how many bytes we get from the cache
                s32 nBytes = MIN( BytesLeft, (s32)(pCache->GetFirstByte() + pCache->GetBytesCached() - PhysicalByte) );

                // Copy Data
                x_memcpy( (void*)pBuffer, &pCache->GetBuffer()[ PhysicalByte - pCache->GetFirstByte() ], nBytes );

                // Update Counters
                BytesRead += nBytes;
                BytesLeft -= nBytes;
                Position  += nBytes;
                pBuffer   += nBytes;
            }
            else
            {
                CONTEXT("IOFS - ReadCacheMiss");

                s32         SectorByte;
                s32         Offset;
                s32         SectorSize   = 2048;
                s32         ReadAttempts = 0;
                xbool       Success      = FALSE;
                io_request  Request;
                io_request* pRequest     = &Request;
                s32         BytesToCache;

                // Calculate how many bytes to read into the cache
                if( IsFileSystemFile )
                {
                    dfs_header* pHeader = (dfs_header*)pOpenFile->pDeviceFile->pHeader;
                    SectorSize = pHeader->SectorSize;
                }

                SectorByte   = PhysicalByte - (PhysicalByte % SectorSize);
                Offset       = SectorByte;
                BytesToCache = MIN( pCache->GetCacheSize(), pOpenFile->pDeviceFile->Length - SectorByte );
                g_IOFSReadBytesRead += BytesToCache;

                // Allow for 10 retries...
                while( !Success )
                {
                    // Only try so many times before bailing...
                    if( ReadAttempts > m_Retries )
                    {
                        while( 1 )
                            x_DelayThread( 1 );
                        ASSERT( 0 );
                        goto Error;
                    }
                    
                    // Cache is now invalid...
                    pCache->Invalidate();

                    // Bump the read attempt.
                    ReadAttempts++;

                    // Set up the read request (use the request semaphore).
                    pRequest->SetRequest( pOpenFile, pCache->GetBuffer(), (s32)Offset, BytesToCache, io_request::MEDIUM_PRIORITY, TRUE, 0, 0, NULL ); 
                    
    #ifdef DEBUG_IO
                    x_DebugMsg( "   Requesting Read\n" );
    #endif
                    // Queue the read request.
                    g_IoMgr.QueueRequest( pRequest );

                    // Wait for read to finish...
                    pRequest->AcquireSemaphore();

    #ifdef DEBUG_IO
                    x_DebugMsg( "   Request complete!\n" );
    #endif

                    // Successful?
                    if( pRequest->GetStatus() == io_request::COMPLETED )
                    {
                        // All good!
                        Success = TRUE;

                        // Set cache bytes read.
                        pCache->SetFirstByte( SectorByte );
                        pCache->Validate( BytesToCache );
                    }
                    else
                    {
                        x_DelayThread( 1 );
                    }
                }
            }
        }

    Error:

        // Release the cache.
        ReleaseCache( pCache );


    // Set new position back into open file structure
    pOpenFile->Position = Position;

    // Return number of bytes read
    return BytesRead;
}

// Complete earlier PC unsupported-write behavior: no cache/request side effects.
// Full later cached-write method remains retained under the real revision selector.
RVA(0x276bd0, 0x5)
s32 io_fs::Write( io_open_file* pOpenFile, const byte* pBuffer, s32 Bytes )
{
    return 0;
}


#else
#include "reference/before-cohort19-iofs-PC/io_filesystem.cpp.inc"
#endif
