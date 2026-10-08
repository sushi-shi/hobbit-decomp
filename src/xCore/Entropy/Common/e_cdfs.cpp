// Imported sibling-engine source; no PC address claim. See docs/imports/entropy-common-input.md.
//==============================================================================
//
//  e_cdfs.cpp
//
//==============================================================================
//
//  CD File System Implementation
//
//==============================================================================

//==============================================================================
//  CONTROL SWITCHES
//==============================================================================

#define ASSERT_ON_WRITE     (0)

#if defined(TARGET_PS2_DEV) && defined(X_DEBUG)
#define ENABLE_PASSTHROUGH  (1)
#else
#define ENABLE_PASSTHROUGH  (0)
#endif

// Set to 1 for logging
#define CDFS_LOG            (0)

//==============================================================================
//  DEFINES
//==============================================================================

#if defined(TARGET_PS2_DEV) && defined(X_DEBUG)
#define MINIMUM_CACHE_SIZE  ( 32*1024)
#define MAXIMUM_CACHE_SIZE  (256*1024)
#else
#define MINIMUM_CACHE_SIZE  ( 32*1024)
#define MAXIMUM_CACHE_SIZE  (256*1024)
#endif

//==============================================================================
//  INCLUDES
//==============================================================================

#include <stdio.h>

#include <xCore/x_files/x_files.hpp>
#include <xCore/x_files/x_array.hpp>
#include <xCore/x_files/x_string.hpp>

#include <xCore/Entropy/e_cdfs.hpp>

#ifdef TARGET_PS2
#include <sifdev.h>
#endif

//==============================================================================
//  STRUCTURES
//==============================================================================

#define CDFS_VERSION    1                           // *** INCREMENT THIS EACH TIME STRUCT CHANGES ***
#define CDFS_MAGIC      0x43444653                  // Magic Number 'CDFS'

struct cdfsHeader
{
    u32         Magic;                              // Magic string CDFS
    s32         Version;                            // Version Number
    s32         SectorSize;                         // Sector size in bytes
    s32         RecommendedCacheSize;               // Recommended cache size
    s32         FirstSectorOffset;                  // Offset to first data sector in xcdfs file
    s32         TotalSectors;                       // Total sectors in filesystem
    s32         FileTableLength;                    // Length of FileTable in bytes
    s32         FileTableEntries;                   // Length of FileTable in entries
    s32         StringTableLength;                  // Length of StringTable in bytes
    s32         StringTableEntries;                 // Length of StringTable in entries
};

struct cdfsEntry
{
    s32         FileNameOffset;                     // Offset into StringTable
    s32         DirNameOffset;                      // Offset into StringTable
    s32         StartSector;                        // Start sector for file data
    s32         Length;                             // Length of file in bytes
};

struct cdfsOpenFile
{
    cdfsEntry*  pFileTableEntry;                    // Pointer to File Table entry
    s32         Position;                           // Byte position in file
};

//==============================================================================
//  VARIABLES
//==============================================================================

static open_fn*                 old_Open            = NULL;     // old filesystem functions
static close_fn*                old_Close           = NULL;
static read_fn*                 old_Read            = NULL;
static write_fn*                old_Write           = NULL;
static seek_fn*                 old_Seek            = NULL;
static tell_fn*                 old_Tell            = NULL;
static flush_fn*                old_Flush           = NULL;
static eof_fn*                  old_EOF             = NULL;
static length_fn*               old_Length          = NULL;

static xbool                    s_Initialized       = FALSE;    // TRUE after cdfs_Init call   

static cdfsHeader               s_Header;                       // FileSystem header

static cdfsEntry*               s_pFileTable        = NULL;     // Pointer to allocated FileTable
static char*                    s_pStringTable      = NULL;     // Pointer to allocated StringTable
static s32                      s_LastFileIndex     = 0;        // Index of last file opened

static s64                      s_CacheFirstByte    = 0;        // First Byte in cache
static s32                      s_CachedBytes       = 0;        // Number of Bytes in cache
static s32                      s_CacheSize;                    // Cache size in bytes
static u8*                      s_pCacheMemory      = NULL;     // Pointer to cache memory
static u8*                      s_pCacheData        = NULL;     // Pointer to cache data (same as above, but aligned)

#ifdef TARGET_PS2
static s32                      s_fh                = -1;       // File Handle to cdfs file
#else
static FILE*                    s_fh                = NULL;     // File Handle to cdfs file
#endif

static s64                      s_cdfsPosition      = 0;        // Position within cdfs file

static xarray<cdfsOpenFile*>    s_OpenFiles;                    // Array of Open Files

static xstring*                 s_pLog              = NULL;     // Pointer to log

//==============================================================================
//  HELPER FUNCTIONS
//==============================================================================

#define TOFWSLASH( c ) (((c)=='\\')?'/':(c))

static s32 cdfs_strcmp( const char* pStr1, const char* pStr2 )
{
    s32 Result = 0;

    ASSERT( pStr1 );
    ASSERT( pStr2 );

    while( !(Result = TOFWSLASH(((s32)*pStr1)) - TOFWSLASH(((s32)*pStr2))) && *pStr1 )
        ++pStr1, ++pStr2;

    return( Result );
}

//==============================================================================

#if ENABLE_PASSTHROUGH

static xbool cdfs_IsOpenFile( X_FILE* pFile )
{
    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    return s_OpenFiles.Find( pOpenFile ) != -1;
}

#endif

//==============================================================================
//  PROTOTYPES
//==============================================================================

void    cdfs_DumpFileTable  ( void );

static X_FILE* cdfs_Open           ( const char* pPathName, const char* pMode );
static void    cdfs_Close          ( X_FILE* pFile );
static s32     cdfs_Read           ( X_FILE* pFile, byte* pBuffer, s32 Bytes );
static s32     cdfs_Write          ( X_FILE* pFile, const byte* pBuffer, s32 Bytes );
static s32     cdfs_Seek           ( X_FILE* pFile, s32 Offset, s32 Origin );
static s32     cdfs_Tell           ( X_FILE* pFile );
static s32     cdfs_Flush          ( X_FILE* pFile );
static xbool   cdfs_EOF            ( X_FILE* pFile );
static s32     cdfs_Length         ( X_FILE* pFile );

#if ENABLE_PASSTHROUGH
static X_FILE* pass_Open           ( const char* pPathName, const char* pMode );
static void    pass_Close          ( X_FILE* pFile );
static s32     pass_Read           ( X_FILE* pFile, byte* pBuffer, s32 Bytes );
static s32     pass_Write          ( X_FILE* pFile, const byte* pBuffer, s32 Bytes );
static s32     pass_Seek           ( X_FILE* pFile, s32 Offset, s32 Origin );
static s32     pass_Tell           ( X_FILE* pFile );
static s32     pass_Flush          ( X_FILE* pFile );
static xbool   pass_EOF            ( X_FILE* pFile );
static s32     pass_Length         ( X_FILE* pFile );
#endif

#ifdef TARGET_PS2
static s32 MySceRead(s32 fh,void *pData,s32 Length)
{
    s32 BytesRead;
    s32 Retries;
    s32 Position;

    BytesRead = 0;        
    Retries = 512;

    //
    // Get our current position just in case we
    // need to reseek to retry the read request.
    //
    Position = sceLseek(fh,0,SCE_SEEK_CUR);

    while (1)
    {
        BytesRead = sceRead(fh,pData,Length);
        if ( (BytesRead == Length) || (Retries==0) )
            break;
        sceLseek(fh,Position,SCE_SEEK_SET);
        Retries--;
		if (Retries==0)
		{
			x_DebugMsg("cdfs: A read error occurred at index 0x%08x\n",Position);
#ifdef TARGET_PS2_DVD
			Retries = 512;
#endif
		}
    }
    return BytesRead;
}
#endif
//==============================================================================
//  IMPLEMENTATION
//==============================================================================

xbool cdfs_Init( const char* pFilesystemFile )
{
    xbool       Success = TRUE;

    // Can only initialize once
    ASSERT( !s_Initialized );

    // Open filesystem file
#ifdef TARGET_PS2
    s_fh = sceOpen( (char*)pFilesystemFile, SCE_RDONLY );
    if( s_fh >= 0 )
#else
    s_fh = fopen( pFilesystemFile, "rb" );
    if( s_fh )
#endif
    {
        // Read Header
#ifdef TARGET_PS2
        s32 BytesRead = MySceRead( s_fh, &s_Header, sizeof(s_Header) );
#else
        s32 BytesRead = fread( &s_Header, 1, sizeof(s_Header), s_fh );
#endif
        if( BytesRead == sizeof(s_Header) )
        {
            // Verify it is a 'cdfs' header by its magic number
            if( s_Header.Magic == (CDFS_MAGIC) )
            {
                // Allocate & Read FileTable
                s_pFileTable = (cdfsEntry*)x_malloc( s_Header.FileTableLength );
                ASSERTS( s_pFileTable, "Failed to allocate FileTable" );
                if( s_pFileTable )
                {
                    // Read File Table
#ifdef TARGET_PS2
                    s32 BytesRead = MySceRead( s_fh, s_pFileTable, s_Header.FileTableLength );
#else
                    s32 BytesRead = fread( s_pFileTable, 1, s_Header.FileTableLength, s_fh );
#endif
                    ASSERTS( BytesRead == s_Header.FileTableLength, "End of file reading FileTable" );
                    (void)BytesRead;
                }
                else
                {
                    // Failed File Table read
                    ASSERTS( 0, "Failed to read FileTable" );
                    Success = FALSE;
                }

                // Allocate & Read StringTable
                s_pStringTable = (char*)x_malloc( s_Header.StringTableLength );
                ASSERTS( s_pStringTable, "Failed to allocate StringTable" );
                if( s_pStringTable )
                {
                    // Read File Table
#ifdef TARGET_PS2
                    s32 BytesRead = MySceRead( s_fh, s_pStringTable, s_Header.StringTableLength );
#else
                    s32 BytesRead = fread( s_pStringTable, 1, s_Header.StringTableLength, s_fh );
#endif
                    ASSERTS( BytesRead == s_Header.StringTableLength, "End of file reading StringTable" );
                    (void)BytesRead;
                }
                else
                {
                    // Failed String Table read
                    ASSERTS( 0, "Failed to read StringTable" );
                    Success = FALSE;
                }

                // Allocate a Cache buffer
                s_CacheSize = s_Header.RecommendedCacheSize;
                if( s_CacheSize < MINIMUM_CACHE_SIZE ) s_CacheSize = MINIMUM_CACHE_SIZE;
                if( s_CacheSize > MAXIMUM_CACHE_SIZE ) s_CacheSize = MAXIMUM_CACHE_SIZE;
                s_pCacheMemory = (u8*)x_malloc( s_CacheSize + 256 );
                s_pCacheData   = (u8*)(((s32)s_pCacheMemory+255) & -256);
                ASSERTS( s_pCacheData, "Failed to allocate Cache buffer" );
                if( s_pCacheData == NULL )
                {
                    // Failed Cache buffer allocate
                    ASSERTS( 0, "Failed to allocate cdfs cache" );
                    Success = FALSE;
                }

                // Initialized OpenFiles array
                s_OpenFiles.SetCapacity( 8 );
            }
            else
            {
                // Failed Header Magic number
                ASSERTS( 0, "Invalid cdfs file" );
                Success = FALSE;
            }
        }

        // Record current position within file
#ifdef TARGET_PS2
        s_cdfsPosition = sceLseek( s_fh, 0, SCE_SEEK_CUR );
#else
        s_cdfsPosition = ftell( s_fh );
#endif
    }
    else
    {
        // Filesystem not found
#ifdef TARGET_PS2_DVD
        ASSERTS( 0, "Filesystem 'FILES.DAT' not found\n" );
#endif

        // Failed
        Success = FALSE;
    }

#if CDFS_LOG
    // Create log string
    s_pLog = new xstring;
#endif

    // Set Initialized
    s_Initialized = Success;

    // Set IO hooks if we succeeded, else Cleanup
    if( Success )
    {
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

        // Set new IOHooks
#if ENABLE_PASSTHROUGH
        x_SetFileIOHooks(  pass_Open,
                           pass_Close,
                           pass_Read,
                           pass_Write,
                           pass_Seek,
                           pass_Tell,
                           pass_Flush,
                           pass_EOF,
                           pass_Length );
#else
        x_SetFileIOHooks(  cdfs_Open,
                           cdfs_Close,
                           cdfs_Read,
                           cdfs_Write,
                           cdfs_Seek,
                           cdfs_Tell,
                           cdfs_Flush,
                           cdfs_EOF,
                           cdfs_Length );
#endif
    }
    else
    {
        cdfs_Kill( );
    }

    return (Success);
}

//==============================================================================

void cdfs_Kill( void )
{
    if( s_Initialized )
    {
        // Close Filesystem File
#ifdef TARGET_PS2
        if( s_fh != -1 )
            sceClose( s_fh );
#else
        if( s_fh )
            fclose( s_fh );
#endif

        // Free File Table
        if( s_pFileTable )
            x_free( s_pFileTable );

        // Free String Table
        if( s_pStringTable )
            x_free( s_pStringTable );

        // Free Cache
        // NOTE: s_pCacheData is the actual ptr to cache memory but it is aligned
        // to 256 bytes. s_pCacheMemory is the actual memory block start
        if( s_pCacheMemory )
            x_free( s_pCacheMemory );

        // Clear Data
        s_fh           = NULL;
        s_pFileTable   = NULL;
        s_pStringTable = NULL;
        s_pCacheData   = NULL;
        s_pCacheMemory = NULL;

        x_SetFileIOHooks(  old_Open,
                           old_Close,
                           old_Read,
                           old_Write,
                           old_Seek,
                           old_Tell,
                           old_Flush,
                           old_EOF,
                           old_Length );
        // Clear Initialized Flag
        s_Initialized = FALSE;
    }
}

//==============================================================================
//  DEBUG FUNCTIONS
//==============================================================================

void cdfs_DumpFileTable( void )
{
    s32 i;

    ASSERT( s_Initialized );

    for( i=0 ; i<s_Header.FileTableEntries ; i++ )
    {
        cdfsEntry* pEntry = &s_pFileTable[i];
        x_printf( "%10d bytes @ %6d - %s\\%s\n", pEntry->Length, pEntry->StartSector, &s_pStringTable[pEntry->DirNameOffset], &s_pStringTable[pEntry->FileNameOffset] );
    }
}

//==============================================================================
//  FILE I/O FUNCTIONS FOR CDFS
//==============================================================================

static X_FILE* cdfs_Open( const char* pPathName, const char* pMode )
{
    char            Path[X_MAX_PATH];
    char            File[X_MAX_PATH];
    s32             PathLen;
    s32             Index;
    cdfsOpenFile*   pOpenFile = NULL;
    cdfsEntry*      pFile = NULL;

    (void)pMode;

    ASSERT( s_Initialized );

    // Get Path and File seperated
    PathLen = x_strlen( pPathName );
    Index = PathLen - 1;
    while( (Index >= 0) && (pPathName[Index] != '\\') && (pPathName[Index] != '/'))
    {
        Index--;
    }
    if( Index < 0 )
    {
        // No Path found
        Path[0] = 0;
        x_strcpy( File, pPathName );
    }
    if( Index >= 0)
    {
        ASSERTS( Index < (PathLen-1), xfs("cdfs_Open: No filename '%s'", pPathName) );

        // Copy Path
        x_strncpy( Path, pPathName, Index );
        Path[Index] = 0;

        // Copy File
        x_strcpy( File, &pPathName[Index+1] );
    }

    // Uppercase both Path and File
    x_strtoupper( Path );
    x_strtoupper( File );

    // Search FileTable for a match
    s32 Index1 = s_LastFileIndex;
    s32 Index2 = s_LastFileIndex;
    while( (Index1 >= 0) || (Index2 < s_Header.FileTableEntries) )
    {
        cdfsEntry* pEntry1 = &s_pFileTable[Index1];
        cdfsEntry* pEntry2 = &s_pFileTable[Index2];

        // Test for match on path and file
        if( Index2 < s_Header.FileTableEntries )
        {
            if( (cdfs_strcmp( Path, &s_pStringTable[pEntry2->DirNameOffset]  ) == 0) &&
                (cdfs_strcmp( File, &s_pStringTable[pEntry2->FileNameOffset] ) == 0) )
            {
                // Found Match
                pFile           = pEntry2;
                s_LastFileIndex = Index2;
                break;
            }
            else
            {
                Index2++;
            }
        }

        // Test for match on path and file
        if( Index1 >= 0 )
        {
            if( (cdfs_strcmp( Path, &s_pStringTable[pEntry1->DirNameOffset]  ) == 0) &&
                (cdfs_strcmp( File, &s_pStringTable[pEntry1->FileNameOffset] ) == 0) )
            {
                // Found Match
                pFile           = pEntry1;
                s_LastFileIndex = Index1;
                break;
            }
            else
            {
                Index1--;
            }
        }
    }

    // If file found
    if( pFile != NULL )
    {
        // Create a new openfile structure
        pOpenFile = new cdfsOpenFile;
        ASSERTS( pOpenFile, "cdfs_Open: Out of memory" );
        if( pOpenFile )
        {
            pOpenFile->pFileTableEntry = pFile;
            pOpenFile->Position        = 0;

            // Add to array of open files
            s_OpenFiles.Append() = pOpenFile;
        }

    }
	else
	{
		x_DebugMsg("File '%s' not found in cdfs_Open()\n",pPathName);
	}

    // Add to log
#if CDFS_LOG
    s_pLog->AddFormat( "Open      - %08x, \"%s\", \"%s\"\n", pOpenFile, pPathName, pMode );
#endif

    // Return the file handle
    return (X_FILE*)pOpenFile;
}

//==============================================================================

static void cdfs_Close( X_FILE* pFile )
{
    ASSERT( s_Initialized );

    // Add to log
#if CDFS_LOG
    s_pLog->AddFormat( "Close     - %08x\n", pFile );
#endif

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    s32 Index = s_OpenFiles.Find( pOpenFile );
    if( Index != -1 )
    {
        // Delete open file record
        delete pOpenFile;
        s_OpenFiles.Delete( Index );
    }
    else
    {
        // No record found
        ASSERTS( FALSE, "cdfs_Close: Illegal file handle" );
    }
}

//==============================================================================

static s32 cdfs_Read( X_FILE* pFile, byte* pBuffer, s32 Bytes )
{
    ASSERT( s_Initialized );

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Read: Illegal file handle" );

    // Setup Counters
    s32 BytesLeft     = Bytes;
    s32 BytesRead     = 0;

    s64 FileFirstByte = ((s64)pOpenFile->pFileTableEntry->StartSector * s_Header.SectorSize);

    s32 Position      = pOpenFile->Position;
    s32 Length        = pOpenFile->pFileTableEntry->Length;

    // Make sure read does not pass end of file
    BytesLeft = MIN( BytesLeft, Length-Position );

    // Add to log
#if CDFS_LOG
    s_pLog->AddFormat( "Read      - %08x, %08x\n", pFile, Bytes );
#endif

    // Loop until all bytes have been read
    while( BytesLeft > 0 )
    {
        s64 PhysicalByte = FileFirstByte + Position;
        s32 nBytes = 0;

        // Check if there is data in the cache, if so copy it, else fill cache
        if( (PhysicalByte >= s_CacheFirstByte) &&
            (PhysicalByte <= (s_CacheFirstByte + s_CachedBytes-1)) )
        {
            // Determine how many bytes we get from the cache
            nBytes = BytesLeft;
            nBytes = MIN( BytesLeft, (s32)(s_CacheFirstByte + s_CachedBytes - PhysicalByte) );

            // Copy Data
            x_memcpy( pBuffer, &s_pCacheData[PhysicalByte - s_CacheFirstByte], nBytes );
        }
        else
        {
            s64 SectorByte   = PhysicalByte - (PhysicalByte % s_Header.SectorSize);
            s32 BytesToCache;

            // Calculate how many bytes to read into the cache
            BytesToCache = MIN( s_CacheSize, (s32)((s64)s_Header.TotalSectors * s_Header.SectorSize - SectorByte) );

            // Seek within cdfs file if necessary
            if( (s_Header.FirstSectorOffset + SectorByte) != s_cdfsPosition )
            {
#ifdef TARGET_PS2
                s32 pos;
                pos = sceLseek( s_fh, s_Header.FirstSectorOffset + (s32)SectorByte, SCE_SEEK_SET );
                ASSERT(pos>=0);
#else
                fseek( s_fh, s_Header.FirstSectorOffset + (s32)SectorByte, SEEK_SET );
#endif
                s_cdfsPosition = s_Header.FirstSectorOffset + SectorByte;
            }

            // Read data into cache
#ifdef TARGET_PS2
            s32 CacheBytesRead = MySceRead( s_fh, s_pCacheData, BytesToCache );
#else
            s32 CacheBytesRead = fread( s_pCacheData, 1, BytesToCache, s_fh );
#endif
            ASSERT( CacheBytesRead == BytesToCache );
            s_cdfsPosition   += CacheBytesRead;
            s_CacheFirstByte  = SectorByte;
            s_CachedBytes     = CacheBytesRead;

            // Add to log
#if CDFS_LOG
            s_pLog->AddFormat( "CacheFill - %08x, %08x\n", s_CacheFirstByte, s_CachedBytes );
#endif
        }

        // Update Counters
        BytesRead += nBytes;
        BytesLeft -= nBytes;
        Position  += nBytes;
        pBuffer   += nBytes;
    }

    // Set new position back into open file structure
    pOpenFile->Position = Position;

    // Return number of bytes read
    return BytesRead;
}

//==============================================================================

static s32 cdfs_Write( X_FILE* pFile, const byte* pBuffer, s32 Bytes )
{
    ASSERT( s_Initialized );

    (void)pBuffer;
    (void)Bytes;

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Write: Illegal file handle" );
    (void)pOpenFile;

#if ASSERT_ON_WRITE
    ASSERTS( FALSE, "cdfs_Write: Unsupported operation on CD" );
#endif

    return Bytes;
}

//==============================================================================

static s32 cdfs_Seek( X_FILE* pFile, s32 Offset, s32 Origin )
{
    ASSERT( s_Initialized );

    // Add to log
#if CDFS_LOG
    s_pLog->AddFormat( "Seek      - %08x, %08x, %d\n", pFile, Offset, Origin );
#endif

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Seek: Illegal file handle" );

    // Set new position
    switch( Origin )
    {
    case X_SEEK_SET:
        pOpenFile->Position = Offset;
        break;
    case X_SEEK_CUR:
        pOpenFile->Position += Offset;
        break;
    case X_SEEK_END:
        pOpenFile->Position = pOpenFile->pFileTableEntry->Length - Offset;
        break;
    }

    // Limit position
    pOpenFile->Position = MAX( 0, pOpenFile->Position );
    pOpenFile->Position = MIN( pOpenFile->Position, pOpenFile->pFileTableEntry->Length );

    // Success
    return 0;
}

//==============================================================================

static s32 cdfs_Tell( X_FILE* pFile )
{
    ASSERT( s_Initialized );

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Tell: Illegal file handle" );

    return pOpenFile->Position;
}

//==============================================================================

static s32 cdfs_Flush( X_FILE* pFile )
{
    ASSERT( s_Initialized );

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Flush: Illegal file handle" );
    (void)pOpenFile;

    ASSERTS( FALSE, "cdfs_Flush: Unsupported operation on CD" );
    return X_EOF;
}

//==============================================================================

static xbool cdfs_EOF( X_FILE* pFile )
{
    ASSERT( s_Initialized );

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_EOF: Illegal file handle" );

    return pOpenFile->Position >= pOpenFile->pFileTableEntry->Length;
}

//==============================================================================

static s32 cdfs_Length( X_FILE* pFile )
{
    ASSERT( s_Initialized );

    cdfsOpenFile* pOpenFile = (cdfsOpenFile*)pFile;
    ASSERTS( s_OpenFiles.Find( pOpenFile ) != -1, "cdfs_Length: Illegal file handle" );

    return pOpenFile->pFileTableEntry->Length;
}

//==============================================================================
//==============================================================================
//  Logging functions
//==============================================================================
//==============================================================================

void cdfs_WriteLog( const char* pPathName )
{
    x_SetFileIOHooks(  old_Open,
                       old_Close,
                       old_Read,
                       old_Write,
                       old_Seek,
                       old_Tell,
                       old_Flush,
                       old_EOF,
                       old_Length );

    s_pLog->SaveFile( pPathName );

#if ENABLE_PASSTHROUGH
    x_SetFileIOHooks(  pass_Open,
                       pass_Close,
                       pass_Read,
                       pass_Write,
                       pass_Seek,
                       pass_Tell,
                       pass_Flush,
                       pass_EOF,
                       pass_Length );
#else
    x_SetFileIOHooks(  cdfs_Open,
                       cdfs_Close,
                       cdfs_Read,
                       cdfs_Write,
                       cdfs_Seek,
                       cdfs_Tell,
                       cdfs_Flush,
                       cdfs_EOF,
                       cdfs_Length );
#endif
}

//==============================================================================

void cdfs_ClearLog( void )
{
    s_pLog->Clear();
}

//==============================================================================
//==============================================================================
//  Passthrough versions of filesystem functions, these will attempt to work
//  on the cdfs, but will fallback to the previous filesystem if the file can
//  not be found in the FILES.DAT image
//==============================================================================
//==============================================================================

#if ENABLE_PASSTHROUGH

static X_FILE* pass_Open( const char* pPathName, const char* pMode )
{
    X_FILE* pFile;

    pFile = cdfs_Open( pPathName, pMode );
    if( !pFile )
    {
        ASSERT( old_Open );
        pFile = old_Open( pPathName, pMode );
    }

    // Return the file handle
    return pFile;
}

static void pass_Close( X_FILE* pFile )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        cdfs_Close( pFile );
    }
    else
    {
        ASSERT( old_Close );
        old_Close( pFile );
    }
}

static s32 pass_Read( X_FILE* pFile, byte* pBuffer, s32 Bytes )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Read( pFile, pBuffer, Bytes );
    }
    else
    {
        ASSERT( old_Read );
        return old_Read( pFile, pBuffer, Bytes );
    }
}

static s32 pass_Write( X_FILE* pFile, const byte* pBuffer, s32 Bytes )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Write( pFile, pBuffer, Bytes );
    }
    else
    {
        ASSERT( old_Write );
        return old_Write( pFile, pBuffer, Bytes );
    }
}

static s32 pass_Seek( X_FILE* pFile, s32 Offset, s32 Origin )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Seek( pFile, Offset, Origin );
    }
    else
    {
        ASSERT( old_Seek );
        return old_Seek( pFile, Offset, Origin );
    }
}

static s32 pass_Tell( X_FILE* pFile )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Tell( pFile );
    }
    else
    {
        ASSERT( old_Tell );
        return old_Tell( pFile );
    }
}

static s32 pass_Flush( X_FILE* pFile )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Flush( pFile );
    }
    else
    {
        ASSERT( old_Flush );
        return old_Flush( pFile );
    }
}

static xbool pass_EOF( X_FILE* pFile )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_EOF( pFile );
    }
    else
    {
        ASSERT( old_EOF );
        return old_EOF( pFile );
    }
}

static s32 pass_Length( X_FILE* pFile )
{
    if( cdfs_IsOpenFile( pFile ) )
    {
        return cdfs_Length( pFile );
    }
    else
    {
        ASSERT( old_Length );
        return old_Length( pFile );
    }
}

#endif