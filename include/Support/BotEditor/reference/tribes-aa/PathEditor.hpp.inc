
#ifndef PATH_EDITOR_HPP
#define PATH_EDITOR_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES 
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_files.hpp>
#include "dharray.hpp"
#include "..\Support\Objects\bot\graph.hpp"

///////////////////////////////////////////////////////////////////////////
// BOT-EDITOR FUNCTIONS
///////////////////////////////////////////////////////////////////////////
class bot_object;

struct grid_cell
{
    bbox    CellBox;
    u16     nEdges;

    // Reference to start of edges list in master list, GridEdgesList
    u16     EdgeOffset;
};

enum {
    CELLS_WIDE = 32,
    CELLS_DEEP = 32,
};

struct grid
{
    vector3 Min;
    vector3 Max;

    f32     CellWidth;
    f32     CellDepth;

    s32     nCellsWide;
    s32     nCellsDeep;

    // List of all cell edges
    xarray<s32> GridEdgesList;
    s32     nGridEdges;

    // The grid of cells
    grid_cell **Cell;
};

class path_editor
{
//=========================================================================
public:
    enum walk_type
    {
        WKTYPE_NEIGHBOR,
        WKTYPE_RAND_NODE,
        WKTYPE_LOWEST_NODE,
        WKTYPE_RAND_TYPE,

        WKTYPE_EDGE,
        WKTYPE_PATH,
    };

    enum node_type
    {
        NDTYPE_NULL,
        NDTYPE_QUESTIONABLE,
        NDTYPE_UNCONNECTED,
        NDTYPE_ADJUSTED,
    };

    enum edge_type
    {
        EDGE_TYPE_GROWN,
        EDGE_TYPE_AIR,
        EDGE_TYPE_SKI,
    };

    enum
    {
        MAX_EDGES_NODE = 32,
        NUM_BOTS       = 16,
    };

    struct edge
    {
        edge_type   Type;       
        xbool       bLoose;
        
        s32         RefCount;
        s32         nA2B;
        s32         nB2A;
        xhandle     hNodeA;
        xhandle     hNodeB;
        f32         CostFromA2B;
        f32         CostFromB2A;
    };

    struct node
    {
        vector3     Pos;
        node_type   Type;

        s32         nEdges;
        xhandle     hEdgeList[MAX_EDGES_NODE];        
    };

    enum editor_mode
    {
        EDMODE_NULL,
        EDMODE_ADDNODE,
        EDMODE_ADDSELECT,
        EDMODE_ADDMOVE,
    };

    struct mark
    {
        vector3 Pos;
        s32     Intensity;
    };

//=========================================================================
public:
    
                path_editor             ( void );
               ~path_editor             ( void );

    void        Save                    ( const char* pFileName );
    xbool       Load                    ( const char* pFileName );
    void        Export                  ( const char* pFileName );
    void        Import                  ( const char* pFileName );
    void        Render                  ( xbool bUseZBUffer );

    void        AddNode                 ( const vector3& Pos );
    void        SelectEdge              ( const vector3& Pos );
    void        SelectNode              ( const vector3& Pos );
    void        SelectSpawnNode         ( const vector3& Pos );
    void        DeleteActiveEdge        ( void );
    void        RemoveSelectedNode      ( void );
    void        UnSelectActiveNode      ( void );
    void        MoveSelectedNode        ( const vector3& Pos );
    void        InsertNodeInActiveEdge  ( const vector3& Pos );
    void        ComputePathToActiveNode ( xarray<u16>& Array );
    void        DropNode                ( node* pNode = NULL );
    void        DropNode2               ( node* pNode = NULL );
    xbool       LowerNode               ( node* pNode = NULL );

    void        UnselectedActiveEdge    ( void );
    void        ChangeActiveEdge        ( void );
    void        ChangeEdgeType          ( edge_type Type );
    void        SetAerial               ( void );
    void        SetGround               ( void );
    void        SetSki                  ( void );
    void        SetLoose                ( void );
    void        SetTight                ( void );

    void        GetSelectedPos          ( vector3& SelectedPos ) const;
    void        GetSelectedSpawnPos     ( vector3& SelectedSpawnPos ) const;
    void        GetPlayerPos            ( vector3& PlayerPos ) const;

    xbool       LoadTemplate            ( const char* pFileName, const matrix4& L2W );
    xbool       SaveTemplate            ( const char* pFileName, const matrix4& W2L );

    void        CostAnalysis            ( xhandle hNodeA, xhandle hNodeB );
    void        CostAnalysis            ( walk_type Type = WKTYPE_RAND_TYPE );

    void        UpdateCost              ( s32 nBot );
    void        PickNewPath             ( s32 nBot);
    void        ViewData                ( s32 hWnd );
    void        RenewDestination        ( void );

    void        RunStressTest           ( void );
    void        BlowEdges               ( s32 BBoxSize );

    void        RecomputeGrid           ( s32 Width = CELLS_WIDE, s32 Depth = CELLS_DEEP);

    // Store the current path_editor graph into the g_Graph object.
    void        Refresh_g_GRAPH     ( void );

    xbool       m_bChanged;
    void        AddMarker               ( const vector3& Pos, s32 Checks );

//=========================================================================
protected:

    void        InitClass               ( void );
    xbool       InitTerr                ( void );
    void        InitializeBots          ( void );
    void        EditModeAddNode         ( void );
    void        EditModeSelectNode      ( void );
    f32         FindClosestNode         ( const vector3& Pos, xhandle& hHandle );
    xhandle     AddEdge                 ( edge& NewEdge );
    f32         FindClosestEdge         ( const vector3& Pos, xhandle& hHandle );
    void        AddEdgeToNode           ( node& Node, xhandle hEdge );
    void        DeleteEdge              ( xhandle hEdge );
    void        DeleteNode              ( xhandle hNode );
    void        RemoveEdgeFromNode      ( node& Node, xhandle hEdge );

    void        ResetVisited            ();
    s32         DetermineCost           ( xhandle hNodeA, xhandle hNodeB );

    void        LoadTest                ();

    void        RaiseNodeByBBox         ( node& Node );
    xbool        ScanNodeForCollision    ( node& Node );

    xbool        Save                    ( const char* pFileName, const matrix4& W2L );
    xbool       Load                    ( const char* pFileName, const matrix4& L2W );

    void        AdjustNodesToCrossPoint ( node& NodeA, node& NodeB,
                                            graph::node& GNodeA, graph::node& GNodeB,
                                            const vector3& Pos);
    void        TestAssetsForEdges      ( void );



//=========================================================================
public:
    grid            m_Grid;
    s32             m_nCells;
    s32             m_nMaxEdgesPerCell;
protected:

    dharray<edge>   m_EdgeList;
public:
    dharray<node>   m_NodeList;
protected:
    editor_mode     m_Mode;                 // Current editor mode
    
    xhandle         m_hSelectedNode;        // Activate/Selected node
    xhandle         m_hSelectedSpawn;       // Selected Spawn Point 
    xhandle         m_hSelectedEdge;        // Selected Edge

    edge_type       m_EdgeType;             // Add aerial or ground nodes
    xbool           m_bLoose;               // Add Loose or Tight edges.

    edge            m_ActiveEdge;           // Active edge info.
    
    void*           m_pBots[NUM_BOTS];      // Array of bot pointers.
    xbool           m_bBotInit;          
    
    xhandle         *m_GraphToPathHandle;
    s32             iMinVis[NUM_BOTS];
    edge*           m_pFirstMinNode;
    s32             m_iFirstMinNode;
    walk_type       m_CurrWalkType;

    mark            m_Mark[50];
    s32             m_nMarks;


};

///////////////////////////////////////////////////////////////////////////
// EMD 
///////////////////////////////////////////////////////////////////////////
#endif
