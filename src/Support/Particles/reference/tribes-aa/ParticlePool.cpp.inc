#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Entropy/e_VRAM.hpp>
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include "ParticlePool.hpp"

//=============================================================================

#define MAX_PARTICLES    5000

//=============================================================================
// Node Definition and helper macros
//
// This structure overlays with the pfx_particle structure to save ram since
// the particle is never active when in the free list it's data members can
// safely be overwritten with the link information
//=============================================================================

struct pfx_node
{
    s32     Count;                          // Number of particles
    s32     Prev;                           // Previous particle index
    s32     Next;                           // Next particle index
};

#define NODE_COUNT( Index ) (((pfx_node*)&m_pParticles[Index])->Count)
#define NODE_PREV(  Index ) (((pfx_node*)&m_pParticles[Index])->Prev )
#define NODE_NEXT(  Index ) (((pfx_node*)&m_pParticles[Index])->Next )

//=============================================================================
//  Particle pool
//=============================================================================

pfx_pool::pfx_pool( )
{
    m_pParticles    = NULL;
    m_Created       = FALSE;
}

//=============================================================================

pfx_pool::~pfx_pool( )
{
    if( m_pParticles )
        delete[] m_pParticles;
    m_pParticles = NULL;
    m_Created    = FALSE;
}

//=============================================================================

xbool pfx_pool::Init( s32 NumParticles )
{
    ASSERT( !m_Created );
    ASSERT( NumParticles > 0 );
    ASSERT( NumParticles <= MAX_PARTICLES );

    // Keep record of how many particles we have
    m_NumParticles = NumParticles;
    m_NumFree      = NumParticles;

    // Create array, 1 extra for dummy list head
    m_pParticles = new pfx_particle[NumParticles+1];
    ASSERT( m_pParticles );
    x_memset( m_pParticles, 0, sizeof(pfx_particle)*(NumParticles+1) );

    // Initialize free links
    NODE_COUNT(0) = 0;
    NODE_PREV (0) = 0;
    NODE_NEXT (0) = 1;
    NODE_COUNT(1) = m_NumParticles;
    NODE_PREV (1) = 0;
    NODE_NEXT (1) = 0;

    // Set created flag
    m_Created = TRUE;

    // Return Success Code
    return m_Created;
}

//=============================================================================

void pfx_pool::Kill( void )
{
    // Pool is no longer available
    delete [] m_pParticles;
    m_pParticles = NULL;
    m_Created    = FALSE;
}

//=============================================================================

s32 pfx_pool::GetNumAllocatedParticles( void )
{
    return m_NumParticles - m_NumFree;
}

//=============================================================================

s32 pfx_pool::GetNumFreeParticles( void )
{
    return m_NumFree;
}

//=============================================================================

s32 pfx_pool::GetFragmentation( void )
{
    pfx_node*   pNode = (pfx_node*)m_pParticles;
    s32 Index = pNode->Next;
    s32 Count = 0;

    while( Index != 0 )
    {
        pNode = (pfx_node*)&m_pParticles[Index];
        Index = pNode->Next;
        Count++;
    }

    return Count;
}

//=============================================================================

pfx_particle* pfx_pool::AllocateParticles( s32& MaxParticles )
{
    s32             BiggestIndex;
    s32             BiggestCount;
    s32             Index;
    pfx_particle*   pParticles;
    pfx_node*       pNode;
    s32             NumAllocated;

    ASSERT( m_Created );

    // Setup for search
    Index        = ((pfx_node*)m_pParticles)->Next;
    pNode        = (pfx_node*)&m_pParticles[Index];
    BiggestIndex = 0;
    BiggestCount = 0;

    // Search freelinks for a block that can fit the requirements
    while( Index != 0 )
    {
        // Check if a match found
        if( pNode->Count >= MaxParticles )
        {
            BiggestIndex = Index;
            BiggestCount = pNode->Count;
            break;
        }
        else if( pNode->Count > BiggestCount )
        {
            BiggestIndex = Index;
            BiggestCount = pNode->Count;
        }

        Index = pNode->Next;
        pNode = (pfx_node*)&m_pParticles[Index];
    }

    // Did we find a block
    if( BiggestIndex != 0 )
    {
        // Determine how many will be allocated
        NumAllocated = MIN( MaxParticles, BiggestCount );

        // Allocating all of the block?
        if( NumAllocated == BiggestCount )
        {
            // Remove this block from list
            NODE_NEXT( NODE_PREV( BiggestIndex ) ) = NODE_NEXT( BiggestIndex );
            NODE_PREV( NODE_NEXT( BiggestIndex ) ) = NODE_PREV( BiggestIndex );
        }
        else
        {
            // Insert a new block in this blocks place
            NODE_COUNT( BiggestIndex + NumAllocated ) = BiggestCount - NumAllocated;
            NODE_PREV ( BiggestIndex + NumAllocated ) = NODE_PREV( BiggestIndex );
            NODE_NEXT ( BiggestIndex + NumAllocated ) = NODE_NEXT( BiggestIndex );
            NODE_NEXT ( NODE_PREV( BiggestIndex )   ) = BiggestIndex + NumAllocated;
            NODE_PREV ( NODE_NEXT( BiggestIndex )   ) = BiggestIndex + NumAllocated;
        }

        // Set return pointer & Number allocated
        pParticles    = &m_pParticles[BiggestIndex];
        MaxParticles  = NumAllocated;
        m_NumFree    -= NumAllocated;
    }
    else
    {
        // Failed to allocate particles
        pParticles   = NULL;
        MaxParticles = 0;
    }

    // Return pointer to allocated block
    return pParticles;
}

//-----------------------------------------------------------------------------

void pfx_pool::FreeParticles( pfx_particle* pParticles, s32 Count )
{
    s32             Index;
    s32             FreeIndex;
    s32             PrevIndex;

    ASSERT( m_Created );
    ASSERT( (m_NumFree+Count) <= m_NumParticles );

    // Calculate FreeIndex from pointer
    FreeIndex = ((s32)pParticles - (s32)m_pParticles) / sizeof(pfx_particle);

    // Search for Node we need to insert before
    Index = NODE_NEXT( 0 );
    while( Index != 0 )
    {
        if( FreeIndex < Index )
            break;
        Index = NODE_NEXT( Index );
    }

    PrevIndex = NODE_PREV( Index );

    // Check if we can merge with before and after blocks
    if( ( (PrevIndex + NODE_COUNT( PrevIndex )) == FreeIndex ) &&
        ( Index == (FreeIndex + Count) ) )
    {
        NODE_COUNT( PrevIndex          ) = Count + NODE_COUNT( PrevIndex ) + NODE_COUNT( Index );
        NODE_NEXT ( PrevIndex          ) = NODE_NEXT( Index );
        NODE_PREV ( NODE_NEXT( Index ) ) = PrevIndex ;
    }
    // Check if we can merge with before block
    else if( (PrevIndex + NODE_COUNT( PrevIndex )) == FreeIndex )
    {
        NODE_COUNT( PrevIndex ) += Count;
    }
    // Check if we can merge with after block
    else if( Index == (FreeIndex + Count) )
    {
        NODE_PREV ( FreeIndex          ) = NODE_PREV( Index );
        NODE_NEXT ( FreeIndex          ) = NODE_NEXT( Index );
        NODE_COUNT( FreeIndex          ) = Count + NODE_COUNT( Index );
        NODE_PREV ( NODE_NEXT( Index ) ) = FreeIndex;
        NODE_NEXT ( NODE_PREV( Index ) ) = FreeIndex;
    }
    // Simple Insert before the pScan Node
    else
    {
        NODE_COUNT( FreeIndex          ) = Count;
        NODE_NEXT ( FreeIndex          ) = Index;
        NODE_PREV ( FreeIndex          ) = NODE_PREV( Index );
        NODE_NEXT ( NODE_PREV( Index ) ) = FreeIndex;
        NODE_PREV ( Index              ) = FreeIndex;
    }

    // Increase count of free particles
    m_NumFree += Count;
}

//=============================================================================
