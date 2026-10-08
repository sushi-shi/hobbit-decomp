#include <rva.h>

#include <xCore/Entropy/e_ScratchMem.hpp>
#include <xCore/x_files/x_memory.hpp>

#define ALIGN_16(x) (((x) + 15) & ~15)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Preserve the explicit zero initializer: VC6 places this after the other BSS
// contributions. The complete section, including array alignment, is proved.
DATA(0x003f2420) static int s_Initialized = 0;
DATA(0x003f23b0) static int s_Active;
DATA(0x003f23c4) static unsigned char* s_pStorage[2];
DATA(0x003f23bc) static int s_CurrentSize[2];
DATA(0x003f23d0) static int s_RequestedSize;
DATA(0x003f23b8) static int s_MaxUsed;
// Name inferred from the same three reset sites in both sibling sources;
// the PC address and storage are proved independently of that name.
DATA(0x003f23b4) static int s_NLogEntries;
DATA(0x003f23d8) unsigned char* s_pScratchMemBufferTop;
DATA(0x003f23cc) unsigned char* s_pScratchMemStackTop;
DATA(0x003f23e0) unsigned char* s_pScratchMemMarker[16];
DATA(0x003f23d4) int s_ScratchMemNextMarker;

static void smem_ActivateSection();

RVA(0x00271230, 0x6d)
void smem_Init(int NBytes)
{
    s_RequestedSize = ALIGN_16(NBytes);
    for (int i = 0; i < 2; i++)
    {
        s_pStorage[i] = (unsigned char*)x_malloc_fn(
            s_RequestedSize,
            "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_ScratchMem.cpp", 142);
        s_CurrentSize[i] = s_RequestedSize;
    }
    s_Active = 0;
    smem_ActivateSection();
    s_Initialized = 1;
    s_MaxUsed = 0;
    s_NLogEntries = 0;
}

RVA(0x002712a0, 0x2b)
static void smem_ActivateSection()
{
    s_pScratchMemBufferTop = s_pStorage[s_Active];
    s_pScratchMemStackTop = s_pScratchMemBufferTop + s_CurrentSize[s_Active];
    s_ScratchMemNextMarker = 0;
}

RVA(0x002712d0, 0x42)
void smem_Kill()
{
    for (int i = 0; i < 2; i++)
    {
        x_free_fn(s_pStorage[i],
            "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_ScratchMem.cpp", 164);
        s_pStorage[i] = 0;
        s_CurrentSize[i] = 0;
    }
    s_NLogEntries = 0;
    s_Initialized = 0;
}

RVA(0x00271320, 0xa5)
void smem_Toggle()
{
    s_MaxUsed = MAX(s_pScratchMemBufferTop - s_pStorage[s_Active], s_MaxUsed);
    int Other = (s_Active + 1) % 2;
    if (s_CurrentSize[Other] != s_RequestedSize)
    {
        s_MaxUsed = 0;
        x_free_fn(s_pStorage[Other],
            "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_ScratchMem.cpp", 225);
        s_pStorage[Other] = (unsigned char*)x_malloc_fn(
            s_RequestedSize,
            "C:\\projects\\meridian\\xCore\\entropy\\Common\\e_ScratchMem.cpp", 226);
        s_CurrentSize[Other] = s_RequestedSize;
    }
    s_Active = Other;
    smem_ActivateSection();
    s_NLogEntries = 0;
}

RVA(0x002713d0, 0x10)
void smem_ChangeSize(int NBytes)
{
    s_RequestedSize = ALIGN_16(NBytes);
}

RVA(0x002713e0, 0x6)
int smem_GetMaxUsed()
{
    return s_MaxUsed;
}

// Additional release stack operations from Tribes-AA 4aab7137; no PC claims.
#include <xCore/x_files/x_files.hpp>
#define SMEM_ALIGN ALIGN_16
#define MAX_MARKERS 16
#define s_pBufferTop s_pScratchMemBufferTop
#define s_pStackTop s_pScratchMemStackTop
#define s_pMarker s_pScratchMemMarker
#define s_NextMarker s_ScratchMemNextMarker

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

void smem_rfunc_StackPushMarker( void )
{
    ASSERT( s_Initialized );
    ASSERT( s_NextMarker < MAX_MARKERS );   // Too many markers!

    s_pMarker[s_NextMarker] = s_pStackTop;
    s_NextMarker++;
}

void smem_rfunc_StackPopToMarker( void )
{
    ASSERT( s_Initialized );
    ASSERT( s_NextMarker > 0 );     // Too many pops!

    s_NextMarker--;
    s_pStackTop = s_pMarker[s_NextMarker];
}

// Original Area51 431f72b9 requested-buffer-size query; unlabelled PC body.
int smem_GetBufferSize(void)
{
    return s_RequestedSize;
}
