//==============================================================================
//
//  Path.hpp
//
//==============================================================================
#ifndef PATH_HPP
#define PATH_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "..\Support\Objects\bot\graph.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

class bot_path 
{
public:
    bot_path( void ) {};
    bot_path( s16 Density );
    
    const graph::node& operator []  ( s32 Index ) const;
    graph::node& operator [] ( s32 Index );
    s32 GetCount( void ) const { return m_Path.GetCount(); };
    graph::node& Append( void ) { return m_Path.Append(); };
    void Clear( void ) { m_Path.Clear(); };

    void SetDensity( s32 Density );
    s32 GetDensity( void ) const { return m_Density; };
    s16 GetTerrainVertexDist( void ) const { return TERRAIN_VERTEX_DIST; };
    s32 GetTerrainDistPerSide( void ) const { return TERRAIN_DIST_PER_SIDE; };
    s16 GetVertexSpacing( void ) const { return VERTEX_SPACING; };
    s16 GetNumVertsHoriz( void ) const { return NUM_VERTS_HORIZ; };
    s16 GetNumVertsVert( void ) const { return NUM_VERTS_VERT; };
    s32 GetTotalCells( void ) const { return TOTAL_CELLS; };
    f32 GetAxialDist( void ) const { return AXIAL_DIST; };
    f32 GetDiagDist( void ) const { return DIAG_DIST; };
    f32 GetMinAxialCost( void ) const { return MIN_AXIAL_COST; };
    f32 GetMinDiagCost( void ) const { return MIN_DIAG_COST; };
    f32 GetCost( s32 Index ) const { return COST_LUT[Index]; };


private:
    static const    s16     TERRAIN_VERTEX_DIST;
    static const    s32     TERRAIN_DIST_PER_SIDE;
    s16     VERTEX_SPACING;
    s16     NUM_VERTS_HORIZ;
    s16     NUM_VERTS_VERT;
    s32     TOTAL_CELLS;
    f32     AXIAL_DIST;
    f32     DIAG_DIST;

    f32     MIN_AXIAL_COST;
    f32     MIN_DIAG_COST;
    f32     COST_LUT[2];

    xarray<graph::node> m_Path;
    s16 m_Density;
};

#endif //ndef PATH_HPP
