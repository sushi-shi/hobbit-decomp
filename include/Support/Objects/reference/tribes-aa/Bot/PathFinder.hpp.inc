//==============================================================================
//
//  PathFinder.hpp
//
//==============================================================================

#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "Path.hpp"
#include <Support/Terrain/Terr.hpp>
#include "CellRef.hpp"
#include "FibHeap.hpp"

#define TIMING_PATH_FINDER 1
#define DEBUG_DRAW_EXTRACTED_NODES 0
#define DEBUG_DRAW_BLOCKED_CELLS 0

//==============================================================================
//  TYPES
//==============================================================================

class path_finder
{
// Types

    typedef byte edge_dir;

// Data
public:
    
    static const s16 VERTEX_SPACING;

// Functions
public:
            path_finder         ( void );
            ~path_finder        ( void );
    void    Initialize          ( terr& Terrain );
    xbool   InitializeFindPath  ( s16 VertexDensity
        , const vector3& From
        , const vector3& To );
    xbool   Update              ( void );
    void    DrawExtractedNodes  ( void ) const;
    
    const   bot_path* const GetPath( void ) { return m_pPath; };
    f32     GetElapsedTime      ( void );

#if DEBUG_DRAW_BLOCKED_CELLS
    xarray<cell_ref> m_BlockedCells;
#endif

private:
    f32     CostNorm            ( const cell_ref& Src
                                    , const cell_ref& Dst ) const;
    void    GetNearestVertex    ( cell_ref& Cell
                                    , const vector3& Point
                                    , s32 Density ) const;
    f32     EdgeCost            ( cell_ref &crDst
                                    , const cell_ref& crSrc
                                    , edge_dir ed
                                    , edge_dir PrevDir );
    void    WalkEdge            ( cell_ref& Dst
                                    , const cell_ref& Src
                                    , edge_dir Dir ) const;
    xbool   IsCellRefValid      ( cell_ref Cell, s32 Density) const;
    s32     IndexOf             ( const cell_ref& Cell ) const  { return( Cell.Z * m_pPath->GetNumVertsHoriz() + Cell.X ); };
    void    CellRefOf           ( cell_ref& Cell, s32 Index ) const;
    xbool   CellIsBlocked       ( const cell_ref& Cell, s32 Density ) const;
    xbool   EdgeIsBlocked       ( const cell_ref& Src
                                    , const cell_ref& Dst
                                    , s32 Density ) const;
    f32     GetTotalHeightClimbed( const cell_ref& Cell ) const;
            
    terr*   m_pTerrain;

    //----------------------------------------------------------------------
    // Path Find State info
    //----------------------------------------------------------------------
    vector3         m_From;
    vector3         m_To;
    cell_ref        m_crBeg;
    cell_ref        m_crEnd;
    PFibHeapNode*   m_Nodes;
    edge_dir*       m_Ed;
    PFibHeapNode    m_pU;
    PFibHeapNode    m_pV;
    PFibHeapNode    m_pReuse;
    FibHeap*        m_pPQ;
    cell_ref        m_crU;
    cell_ref        m_crV;
    f32             m_Cost;
    s32             m_iV;
    PFibHeapNode    m_notvisited;
    PFibHeapNode    m_retired;
    //----------------------------------------------------------------------

    bot_path*       m_pPath;
    xbool           m_FindingPath;
    xtimer          m_Timer;
    f32             m_TimeLastUpdateEndedMs;
    f32             m_FindPathStartTimeMs;
    cell_attribute* m_CellAttributes;

#if DEBUG_DRAW_EXTRACTED_NODES
    xarray<cell_ref> m_ExtractedNodes;
#endif

#if TIMING_PATH_FINDER
    s32     m_DebugNumExtractedNodes;
    xtimer  m_DebugTimer;
    s32     m_DebugNumUpdates;
#endif
};

//==============================================================================
#endif // PATHFINDER_HPP
//==============================================================================
