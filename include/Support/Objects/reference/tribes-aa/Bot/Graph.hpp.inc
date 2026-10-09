
#ifndef GRAPH_HPP
#define GRAPH_HPP

#define USING_GRID 1
#define USING_PREPROCESSED_VPS 0

///////////////////////////////////////////////////////////////////////////
// INCLUDES 
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_files.hpp>
#include "FibHeapAC.hpp"
#include <Support/ObjectMgr/Object.hpp>

#define MAX_BLOCKED_EDGES 100
#define MAX_BOTS 32
#define MAX_GRID_EDGES      3400
#define SHOW_CLOSEST_EDGE   0


///////////////////////////////////////////////////////////////////////////
// GRAPH CLASS
///////////////////////////////////////////////////////////////////////////

class graph
{
//=========================================================================

public:

    // Flags
    enum            
    {   
        EDGE_FLAGS_TEAM_0_BLOCK     = (1),   // MUST be first bit
        EDGE_FLAGS_TEAM_1_BLOCK     = (1<<1),// MUST be second bit
        EDGE_FLAGS_LOOSE            = (1<<2),
        EDGE_FLAGS_INDOOR           = (1<<3),
        EDGE_FLAGS_FORCE_FIELD      = (1<<4),
    };

//------------------------------------------------------------------------------

    enum
    {
        EDGE_TYPE_GROWN,
        EDGE_TYPE_AEREAL,
        EDGE_TYPE_SKI,
    };

//------------------------------------------------------------------------------

    struct edge
    {
        s16             NodeA;
        s16             NodeB;
        u8              Type;         
        u8              Flags;         
        u16             CostFromA2B;
        u16             CostFromB2A;
    };

//------------------------------------------------------------------------------

    struct node
    {
        struct bot_path_info
        {
            u32         Data;

            u8          GetVisited      ( void );
            u16         GetCostSoFar    ( void );
            u16         GetPrev         ( void );

            void        SetVisited      ( u8 Val );
            void        SetCostSoFar    ( u16 Val );
            void        SetPrev         ( u16 iNode );
        };
        xbool operator== ( const node& rhs ) const
        {
            return ((Pos - rhs.Pos).LengthSquared() < 0.00001f );
        };
        
        vector3         Pos;
        s16             EdgeIndex;
        s16             nEdges;
        bot_path_info   PlayerInfo[MAX_BOTS];

    };

//------------------------------------------------------------------------------

    struct blocked_edge
    {
        s32 EdgeID;
        f32 TimeBlocked; // timer on m_BlockedEdgeTimer when edge was blocked
    };

//------------------------------------------------------------------------------

    struct vantage_point
    {
        s32 NodeID;
        object::type TargetType;
        vector3 TargetWorldPos;
        xbool IsMortarPoint; // otherwise, is a linear point
    };

//------------------------------------------------------------------------------

    // GRID INFO
#if USING_GRID
    struct grid_cell
    {
        // Reference to start of edges list in master list, GridEdgesList
        u16     EdgeOffset;
        u16     nEdges;
    };

    struct grid
    {
        vector3 Min;
        vector3 Max;

        f32     CellWidth;
        f32     CellDepth;

        // List of all cell edges
        s32     GridEdgesList[MAX_GRID_EDGES];
        s32     nGridEdges;

        // The grid of cells
        grid_cell Cell[32][32];
    };
#endif
    
//=========================================================================
private:
    xtimer              m_BlockedEdgeTimer;
    blocked_edge        m_BlockedEdges[MAX_BLOCKED_EDGES];
    s16                 m_nBlockedEdges;

public:
                        graph           ( void );
    static void         DestroyInstance ( void );
        
                       ~graph           ( void );
    void                Init            ( void );

    void                Load            ( const char* pFileName
        , xbool BuildGraphIfNoneAvailable = TRUE );
    void                Unload          ( void );
    void                Save            ( const char* pFileName );
    void                Render          ( xbool bUseZBUffer );
    xbool               GetEdgeBetween  ( edge& Edge
                                            , const node& NodeA
                                            , const node& NodeB ) const;
    const node&         GetEdgeStart    ( const xarray<u16>& EdgeIdxList
        , s32 WhichEdge ) const;
    const node&         GetEdgeEnd    ( const xarray<u16>& EdgeIdxList
        , s32 WhichEdge ) const;

    void                BlockEdge           ( s32 EdgeId, u8 Flags );
    void                UpdateBlockedEdges  ( void );

    s32                 GetNumBlockedEdges ( void ) { return m_nBlockedEdges;}

    xbool               DebugConnected       ( void ) const;
    void                DoVisitedFloodFill( s32 NodeId, xbool* pVisited ) const;
    void                DebugGetChildren    ( s32 NodeId, xarray<s32>& Children ) const;
    xbool               NodeIsOnGround      ( s32 NodeId ) const;
    const vector3&      GetCenter           ( void ) const;
    s32                 GetNGroundPointsInRadius ( s32 N
                            , const vector3& Pos
                            , f32 Radius
                            , vector3* pPoints ) const;

#if SHOW_EDGE_COSTS
    void                ShowEdgeCosts   ( void );
    void                ResetEdgeCosts  ( void );
#endif

private:
    void                UnblockOldestEdge   ( void );
    void                UnblockEdge         ( s32 BlockedIdx );
    xbool               NodesCanSeeEachOther( const node& NodeA
                            , const node& NodeB ) const;
    static xbool        PointInsideBuilding ( const vector3& Point );
    s32                 GetNClosestNodes    ( s32 N
                            , const vector3& Pos
                            , s32* pNodeList ) const;
                              
    void                CreatePlaceHolderTerrainGraph( void );

//=========================================================================

public:
    s32                 m_Version;
    edge*               m_pEdgeList;
    node*               m_pNodeList;
    s16*                m_pNodeEdges;
    xbool*              m_pNodeInside;

    s32                 m_nEdges;
    s32                 m_nNodes;
    s32                 m_nNodeEdges;

#if USING_PREPROCESSED_VPS
    vantage_point*      m_pVantagePoint;
    s32                 m_nVantagePoints;
#endif

#if USING_GRID
    grid                m_Grid;
#endif

public:
    xbool               IsIdenticalToFile(const char* pFilename );

#if USING_GRID
    s32                 GetColumn       ( f32 XPos ) const;
    s32                 GetRow          ( f32 ZPos) const;

    void                GetCellBBox     ( s32 X, s32 Z, bbox& Box ) const;
    void                GetCellBBox     ( const vector3& Pos, bbox& Box ) const;
#endif

private:
    xbool               EdgesEqual( const edge& A,  const edge& B ) const;
    xbool               NodesEqual( const node& A,  const node& B ) const;
};

///////////////////////////////////////////////////////////////////////////
// END 
///////////////////////////////////////////////////////////////////////////
#endif
