//==============================================================================
//
//  Path.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "Path.hpp"
#include <xCore/Entropy/Entropy.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================
static const f32 SQRT_2 = 1.4142135623730950488016887242097f;

const   s16     bot_path::TERRAIN_VERTEX_DIST       = 8;
const   s32     bot_path::TERRAIN_DIST_PER_SIDE     = 256 * bot_path::TERRAIN_VERTEX_DIST;

//==============================================================================
//  FUNCTIONS
//==============================================================================
bot_path::bot_path( s16 Density )
{
    SetDensity( Density );
}

const graph::node& bot_path::operator []  ( s32 Index ) const
{
    ASSERT( Index < m_Path.GetCount() );
    return m_Path[Index];
}

graph::node& bot_path::operator [] ( s32 Index )
{
    ASSERT( Index < m_Path.GetCount() );
    return m_Path[Index];
}

void bot_path::SetDensity( s32 Density ) 
{
    m_Density = Density;

    VERTEX_SPACING  = TERRAIN_VERTEX_DIST * m_Density;
    NUM_VERTS_HORIZ = TERRAIN_DIST_PER_SIDE / VERTEX_SPACING;
    NUM_VERTS_VERT  = NUM_VERTS_HORIZ;
    TOTAL_CELLS = NUM_VERTS_HORIZ * NUM_VERTS_VERT;
    AXIAL_DIST = VERTEX_SPACING;
    DIAG_DIST = AXIAL_DIST * SQRT_2;
    MIN_AXIAL_COST = AXIAL_DIST;
    MIN_DIAG_COST = DIAG_DIST;
    COST_LUT[0] = MIN_AXIAL_COST;
    COST_LUT[1] = MIN_DIAG_COST;
}

