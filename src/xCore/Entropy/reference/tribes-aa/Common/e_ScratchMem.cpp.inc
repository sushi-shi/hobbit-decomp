//==============================================================================
//  
//  e_ScratchMem.cpp
//
//==============================================================================

//==============================================================================
//  
//  
//              +---BufferTop                             StackTop---+
//              |                                                    |
//              |                            StackMarker[]--+--+--+  |
//              |                                           |  |  |  |
//              V                                       ... v  v  v  V
//  Storage---> +----------------------------------------------------+
//              |                                                    |
//              |    Buffer -->                         <-- Stack    |
//              |                                                    |
//              +----------------------------------------------------+
//  
//==============================================================================

//#define USE_LOG

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/e_ScratchMem.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_stdio.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

#define MAX_MARKERS     16

#define SMEM_ALIGN      ALIGN_16            // Set to ALIGN_8, ALIGN_16, etc.
#define SMEM_ALIGNMENT  SMEM_ALIGN(1)       // Do not change this.

//==============================================================================
//  STORAGE
//==============================================================================

static  xbool   s_Initialized = FALSE;

static  s32     s_ActiveID = 0 ;    
                
static  s32     s_Active;
static  byte*   s_pStorage[2];
static  s32     s_CurrentSize[2];
static  s32     s_RequestedSize;
                
static  byte*   s_pMarker[ MAX_MARKERS ];
static  s32     s_NextMarker;
                
static  byte*   s_pBufferTop;
static  byte*   s_pStackTop;

static  byte    s_Signature[ SMEM_ALIGNMENT ];
static  s32     s_MaxUsed;

enum calltype
{
    BUFFER_ALLOC = 0,
    STACK_ALLOC,
    STACK_PUSH_MARKER,
    STACK_POP_MARKER,
};

//==============================================================================
//==============================================================================
//==============================================================================
#ifdef USE_LOG // IF LOGGING
//==============================================================================
//==============================================================================
//==============================================================================

struct smem_log
{
    calltype Type;
    char     FileName[32];
    s32      FileLine;
    s32      Amount;
};

#define MAX_LOG_ENTRIES 1024

static smem_log s_Log[ MAX_LOG_ENTRIES ];
static s32      s_NLogEntries;

//==============================================================================

static
void smem_AddLogEntry( calltype Type, const char* pFileName, s32 Line, s32 Amount=0 )
{
    ASSERT( s_NLogEntries < MAX_LOG_ENTRIES );

    s32 Len = x_strlen( pFileName ) + 1;
    if( Len > 32 )  pFileName += (Len-32);
    x_strncpy( s_Log[s_NLogEntries].FileName, pFileName, 32 );

    s_Log[s_NLogEntries].Type = Type;
    s_Log[s_NLogEntries].FileLine = Line;
    s_Log[s_NLogEntries].Amount = Amount;

    s_NLogEntries++;
    (void)Type;
    (void)pFileName;
    (void)Line;
    (void)Amount;
}

//==============================================================================

static
void smem_DumpLog( void )
{
    x_DebugMsg("====== SCRATCH MEM LOG DUMP ======\n");

    s32 I = s_NLogEntries-1;
    s32 N = 0;
    while( (I>=0) && (N<10) )
    {
        switch( s_Log[I].Type )
        {
        case BUFFER_ALLOC: x_DebugMsg("%3d] BUFFER_ALLOC %5d LINE:%4d <%s>\n",I,s_Log[I].Amount,s_Log[I].FileLine,s_Log[I].FileName);
            break;
        case STACK_ALLOC:  x_DebugMsg("%3d] STACK_ALLOC  %5d LINE:%4d <%s>\n",I,s_Log[I].Amount,s_Log[I].FileLine,s_Log[I].FileName);
            break;
        case STACK_PUSH_MARKER: x_DebugMsg("%3d] STACK_PUSH_M       LINE:%4d <%s>\n",I,s_Log[I].FileLine,s_Log[I].FileName);
            break;
        case STACK_POP_MARKER: x_DebugMsg("%3d] STACK_POP_M        LINE:%4d <%s>\n",I,s_Log[I].FileLine,s_Log[I].FileName);
            break;
        }
    
        I--;
        N++;
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
#endif // END LOGGING
//==============================================================================
//==============================================================================
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================

//==============================================================================
//  Internal functions
//==============================================================================

static
void smem_ActivateSection( void )
{
    s_pBufferTop = s_pStorage[ s_Active ];
    s_pStackTop  = s_pBufferTop + s_CurrentSize[ s_Active ];
    
    // Place a signature below the buffer region for overrun checking.
    x_memcpy( s_pBufferTop, s_Signature, SMEM_ALIGNMENT );
    s_pBufferTop += SMEM_ALIGNMENT;

    // We also place a signature above the stack region.
    s_pStackTop -= SMEM_ALIGNMENT;
    x_memcpy( s_pStackTop, s_Signature, SMEM_ALIGNMENT );

    // The next available marker is 0.
    s_NextMarker = 0;
}

//==============================================================================
//  Exposed functions
//==============================================================================

void smem_Init( s32 NBytes )
{
    ASSERT( !s_Initialized );
    ASSERT( NBytes >= 0 );

    // Build the signature.
    x_memset( s_Signature, '~', SMEM_ALIGNMENT );
    x_memcpy( s_Signature + (SMEM_ALIGNMENT>>1) - 2, "SMEM", 4 );

    s_RequestedSize = SMEM_ALIGN( NBytes );

    s_pStorage[0]    = (byte*)x_malloc( s_RequestedSize );
    s_pStorage[1]    = (byte*)x_malloc( s_RequestedSize );
    s_CurrentSize[1] = s_RequestedSize;
    s_CurrentSize[0] = s_RequestedSize;

    s_Active = 0;
    smem_ActivateSection();

    s_Initialized = TRUE;
    s_MaxUsed = 0;

    #ifdef USE_LOG
    s_NLogEntries = 0;
    #endif
}

//==============================================================================

void smem_Kill( void )
{
    ASSERT( s_Initialized );
    ASSERT( s_NextMarker == 0 );

    x_free( s_pStorage[0] );
    x_free( s_pStorage[1] );
    s_pStorage[0]    = NULL;
    s_pStorage[1]    = NULL;
    s_CurrentSize[0] = 0;
    s_CurrentSize[1] = 0;

    #ifdef USE_LOG
    s_NLogEntries = 0;
    #endif

    s_Initialized = FALSE;
}

//==============================================================================

void smem_Toggle( void )
{
    ASSERT( s_Initialized );

    //x_printfxy(0,3,"MAXSMEM %1d",s_MaxUsed);

    // Make sure all markers which were pushed, have been popped.
    ASSERT( s_NextMarker == 0 );

    // Check to see if the signature under the buffer was violated.
    //
    // NOTE - There is a "signature" under the buffer area of scratch memory.
    //        This signature is no longer intact.  If you got an ASSERT failure
    //        here, then some code is trashing memory.
    //
    ASSERT( x_memcmp( s_pStorage[s_Active], 
                      s_Signature, SMEM_ALIGNMENT ) == 0 );

    // Make sure the stack wasn't blown.
    //
    // NOTE - If you got an ASSERT failure here, then you probably overran a 
    //        scratch memory stack allocation.
    //
    ASSERT( x_memcmp( s_pStorage[s_Active] + 
                        s_CurrentSize[s_Active] - 
                        SMEM_ALIGNMENT, 
                      s_Signature, SMEM_ALIGNMENT ) == 0 );

    // Remember most used
    s_MaxUsed = MAX( (s_pBufferTop-s_pStorage[ s_Active ]), s_MaxUsed );

    // To make life a little easier, get the index to the non-active section.
    s32 Other = 1 - s_Active;

    // Right now, "s_Active" is active.  The "other" section must no longer be
    // needed, otherwise we wouldn't be here.  If the program has requested a 
    // change in scratch memory size, we can go ahead and resize the "other" 
    // section now.

    if( s_CurrentSize[Other] != s_RequestedSize )
    {
        x_free( s_pStorage[Other] );
        s_pStorage[Other]    = (byte*)x_malloc( s_RequestedSize );
        s_CurrentSize[Other] = s_RequestedSize;
    }

    // Toggle the sections.

    s_Active = Other;
    smem_ActivateSection();

    // Next ID
    s_ActiveID++ ;

    // Clear log
    #ifdef USE_LOG
    s_NLogEntries = 0;
    #endif
}

//==============================================================================

void smem_ChangeSize( s32 NBytes )
{
    ASSERT( s_Initialized );
    ASSERT( NBytes >= 0 );
    s_RequestedSize = SMEM_ALIGN( NBytes );
}

//==============================================================================
//==============================================================================
//==============================================================================
#ifdef X_DEBUG // DEBUG
//==============================================================================
//==============================================================================
//==============================================================================

byte* smem_dfunc_BufferAlloc( s32 NBytes, const char* pFileName, s32 FileLine )
{
    (void)pFileName;
    (void)FileLine;

    ASSERT( s_Initialized );
    ASSERT( NBytes >= 0 );

    #ifdef USE_LOG
    smem_AddLogEntry( BUFFER_ALLOC, pFileName, FileLine, NBytes );
    #endif

    // Make sure the previous buffer allocation did not overrun.
    //
    // NOTE - If you got an ASSERT failure here, then you probably overran a 
    //        scratch memory buffer allocation.
    //
    if( x_memcmp( s_pBufferTop - SMEM_ALIGNMENT, 
                  s_Signature, 
                  SMEM_ALIGNMENT ) != 0 )
    {
        #ifdef USE_LOG
        smem_DumpLog();
        #endif

        ASSERT( FALSE );
    }

    // Make sure we haven't exhausted this section.
    //
    // NOTE - If you got an ASSERT failure here, then you have consumed all 
    //        available scratch memory.  Increase the amount of memory dedicated
    //        to the scratch memory system.
    //
    if( (s_pBufferTop + SMEM_ALIGN(NBytes)) > s_pStackTop )
    {
        #ifdef USE_LOG
        smem_DumpLog();
        #endif

        x_DebugMsg( "smem_BufferAlloc Failed!!!\n" );
        return( NULL );
    }

    // Allocate memory for the request.
    byte* pAllocation = s_pBufferTop;
    s_pBufferTop += SMEM_ALIGN( NBytes );

    // Stick in a new overrun detection signature.
    x_memcpy( s_pBufferTop, s_Signature, SMEM_ALIGNMENT );
    s_pBufferTop += SMEM_ALIGNMENT;

    // Return the allocated memory.
    return( pAllocation );
}

//==============================================================================

byte* smem_dfunc_StackAlloc( s32 NBytes, const char* pFileName, s32 FileLine )
{
    (void)pFileName;
    (void)FileLine;

    ASSERT( s_Initialized );
    ASSERT( NBytes >= 0 );

    #ifdef USE_LOG
    smem_AddLogEntry( STACK_ALLOC, pFileName, FileLine, NBytes );
    #endif

    // Make sure we haven't exhausted this section.
    //
    // NOTE - If you got an ASSERT failure here, then you have consumed all 
    //        available scratch memory.  Increase the amount of memory dedicated
    //        to the scratch memory system.
    //
    if( s_pBufferTop > (s_pStackTop - SMEM_ALIGN(NBytes)) )
    {
        #ifdef USE_LOG
        smem_DumpLog();
        #endif

        x_DebugMsg( "smem_StackAlloc Failed!!!\n" );
        return( NULL );
    }

    // Allocate memory for the request.
    s_pStackTop -= SMEM_ALIGN( NBytes );
    byte* pAllocation = s_pStackTop;

    // Stick in a new overrun detection signature.
    s_pStackTop -= SMEM_ALIGNMENT;
    x_memcpy( s_pStackTop, s_Signature, SMEM_ALIGNMENT );

    // Return the allocated memory.
    return( pAllocation );
}

//==============================================================================

void smem_dfunc_StackPushMarker( const char* pFileName, s32 FileLine )
{
    (void)pFileName;
    (void)FileLine;

    ASSERT( s_Initialized );
    ASSERT( s_NextMarker < MAX_MARKERS );   // Too many markers!

    s_pMarker[s_NextMarker] = s_pStackTop;
    s_NextMarker++;

    #ifdef USE_LOG
    smem_AddLogEntry( STACK_PUSH_MARKER, pFileName, FileLine );
    #endif
}

//==============================================================================

void smem_dfunc_StackPopToMarker( const char* pFileName, s32 FileLine )
{
    (void)pFileName;
    (void)FileLine;

    ASSERT( s_Initialized );
    ASSERT( s_NextMarker > 0 );     // Too many pops!

    s_NextMarker--;
    s_pStackTop = s_pMarker[s_NextMarker];

    #ifdef USE_LOG
    smem_AddLogEntry( STACK_POP_MARKER, pFileName, FileLine );
    #endif

    // Make sure we didn't violate the overrun signature at the previous marker.
    //
    // NOTE - If you got an ASSERT failure here, then you probably overran a 
    //        scratch memory stack allocation.
    //
    if( x_memcmp( s_pMarker[s_NextMarker], 
                   s_Signature, SMEM_ALIGNMENT ) != 0 )
    {
        #ifdef USE_LOG
        smem_DumpLog();
        #endif

        ASSERT( FALSE );
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
#endif // DEBUG
//==============================================================================
//==============================================================================
//==============================================================================

//==============================================================================
//==============================================================================
//==============================================================================
#ifndef X_DEBUG // RELEASE
//==============================================================================
//==============================================================================
//==============================================================================

byte* smem_rfunc_BufferAlloc( s32 NBytes )
{
    ASSERT( s_Initialized );
    ASSERT( NBytes >= 0 );

    // Make sure we haven't exhausted this section.
    //
    // NOTE - If you got an ASSERT failure here, then you have consumed all 
    //        available scratch memory.  Increase the amount of memory dedicated
    //        to the scratch memory system.
    //
    if( (s_pBufferTop + SMEM_ALIGN(NBytes)) > s_pStackTop )
    {
        x_DebugMsg( "smem_BufferAlloc Failed!!!\n" );
        return( NULL );
    }

    // Allocate memory for the request.
    byte* pAllocation = s_pBufferTop;
    s_pBufferTop += SMEM_ALIGN( NBytes );

    // Return the allocated memory.
    return( pAllocation );
}

//==============================================================================

byte* smem_rfunc_StackAlloc( s32 NBytes )
{
    ASSERT( s_Initialized );
    ASSERT( NBytes >= 0 );

    // Make sure we haven't exhausted this section.
    //
    // NOTE - If you got an ASSERT failure here, then you have consumed all 
    //        available scratch memory.  Increase the amount of memory dedicated
    //        to the scratch memory system.
    //
    if( s_pBufferTop > (s_pStackTop - SMEM_ALIGN(NBytes)) )
    {
        x_DebugMsg( "smem_StackAlloc Failed!!!\n" );
        return( NULL );
    }

    // Allocate memory for the request.
    s_pStackTop -= SMEM_ALIGN( NBytes );
    byte* pAllocation = s_pStackTop;

    // Return the allocated memory.
    return( pAllocation );
}

//==============================================================================

void smem_rfunc_StackPushMarker( void )
{
    ASSERT( s_Initialized );
    ASSERT( s_NextMarker < MAX_MARKERS );   // Too many markers!

    s_pMarker[s_NextMarker] = s_pStackTop;
    s_NextMarker++;
}

//==============================================================================

void smem_rfunc_StackPopToMarker( void )
{
    ASSERT( s_Initialized );
    ASSERT( s_NextMarker > 0 );     // Too many pops!

    s_NextMarker--;
    s_pStackTop = s_pMarker[s_NextMarker];
}

//==============================================================================
//==============================================================================
//==============================================================================
#endif // RELEASE
//==============================================================================
//==============================================================================
//==============================================================================

//==============================================================================

s32 smem_GetActiveID( void )
{
    return( s_ActiveID );
}

//==============================================================================

byte* smem_GetActiveStartAddr( void ) 
{
    return( s_pStorage[ s_Active ] + SMEM_ALIGNMENT );
}

//==============================================================================

byte* smem_GetActiveEndAddr( void )
{
    // Just return current active buffer position
    return( s_pBufferTop );
}

//==============================================================================

void smem_Validate( void )
{
    #ifdef X_DEBUG

    // Make sure the previous buffer allocation did not overrun.
    //
    // NOTE - If you got an ASSERT failure here, then you probably overran a 
    //        scratch memory buffer allocation.
    //
    if( x_memcmp( s_pBufferTop - SMEM_ALIGNMENT, 
                  s_Signature, 
                  SMEM_ALIGNMENT ) != 0 )
    {
        #ifdef USE_LOG
        smem_DumpLog();
        #endif

        ASSERT( FALSE );
    }

    #endif
}

//==============================================================================
