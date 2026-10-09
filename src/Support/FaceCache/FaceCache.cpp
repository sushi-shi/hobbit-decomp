//=========================================================================
//
// FACECACHE.CPP
//
//=========================================================================

#include <Support/FaceCache/FaceCache.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>

//=========================================================================

#define MAX_FCACHE_FACES        512
#define MAX_FCACHE_HASH_BITS    10
#define MAX_FCACHE_HASH_ENTRIES (1<<MAX_FCACHE_HASH_BITS)

struct fcache_hash
{
    s32 First;
    s32 NFaces;
};

static xbool        s_Inited = FALSE;
static fcache_face  s_Face[ MAX_FCACHE_FACES ];
static fcache_hash  s_Hash[ MAX_FCACHE_HASH_ENTRIES ];
static s32          s_FirstAge;
static s32          s_LastAge;
static s32          s_FirstFree;

//=========================================================================

void fcache_Flush( void )
{
    s32 i;

    // Clear hash entries
    for( i=0; i<MAX_FCACHE_HASH_ENTRIES; i++ )
    {
        s_Hash[i].First = -1;
        s_Hash[i].NFaces = 0;
    }

    // String all faces into free list
    for( s32 i=0; i<MAX_FCACHE_FACES; i++ )
    {
        s_Face[i].NVerts   =  0;
        s_Face[i].AgeNext  = -1;
        s_Face[i].AgePrev  = -1;
        s_Face[i].HashNext = i+1;
        s_Face[i].HashPrev = i-1;
    }
    s_Face[MAX_FCACHE_FACES-1].HashNext = -1;
    s_FirstFree = 0;
    s_FirstAge = -1;
    s_LastAge  = -1;
}

//=========================================================================

void fcache_Init( void )
{
    ASSERT( s_Inited == FALSE );
    s_Inited = TRUE;
    fcache_Flush();
}

//=========================================================================

void fcache_Kill( void )
{
}

//=========================================================================

static
s32 ComputeHash( fcache_key Key )
{
    s32 i=0;
    s32 H=0;

    while( i < 32 )
    {
        H ^= (((u32)Key) >> i) & ((1<MAX_FCACHE_HASH_BITS)-1);
        i+=MAX_FCACHE_HASH_BITS;
    }

    ASSERT( (H>0) && (H<MAX_FCACHE_HASH_ENTRIES) );
    return H;
}

//=========================================================================

static
void FreeFace( void )
{
    // Free face on last age

    // Remove from age list

    // Remove from hash list

    // Add to free list
}

//=========================================================================

static
s32 AllocFace( fcache_key Key )
{
    // Remove from free list

    // Add to hash list

    // Add to age list
}

//=========================================================================

static
void BuildFace( fcache_face& Face, fcache_key Key )
{
}

//=========================================================================

static
s32 FindFace( fcache_key Key )
{
    // Check hash table for face
}

//=========================================================================

