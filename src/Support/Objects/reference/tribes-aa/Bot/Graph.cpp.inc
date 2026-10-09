#include "Graph.hpp"
#include "Support/GameMgr/GameMgr.hpp"
#include "Objects/Terrain/Terrain.hpp"
#include "Support\Building\BuildingOBJ.hpp"
#include "BotLog.hpp"


static const f32 BLOCKED_EDGE_TIMEOUT = 5.0f * 60.0f; // seconds
static const s32 VERSION = 2;
#define SHOW_PATH_FIND 0
#if SHOW_PATH_FIND
    s32 g_EdgeCost[2000];
#endif
///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////

graph g_Graph;
static vector3 s_Center;

//==============================================================================

void graph::Init( void )
{
    m_pEdgeList = NULL;
    m_pNodeList = NULL;
    m_pNodeEdges = NULL;
    m_pNodeInside = NULL;

    m_nEdges =
    m_nNodes =
    m_nNodeEdges = 0;

    m_BlockedEdgeTimer.Reset();
    m_BlockedEdgeTimer.Start();
    m_nNodeEdges = 0;

    m_Version = VERSION;

#if SHOW_EDGE_COSTS
    ResetEdgeCosts();
#endif

#if USING_PREPRECESSED_VPS
    m_nVantagePoints = 0;
#endif
}

//==============================================================================

graph::graph( void )
{
    Init();
}

//==============================================================================

graph::~graph( void )
{
    Unload();
}

//==============================================================================

void graph::Unload( void )
{
    if (m_pEdgeList)
    {
        delete [] m_pEdgeList;
        m_pEdgeList = NULL;
    }
    if (m_pNodeList)
    {
        delete [] m_pNodeList;
        m_pNodeList = NULL;
    }
    if (m_pNodeEdges)
    {
        delete [] m_pNodeEdges;
        m_pNodeEdges = NULL;
    }
    if (m_pNodeInside)
    {
        delete [] m_pNodeInside;
        m_pNodeInside = NULL;
    }

    Init();
    
}

//==============================================================================

void graph::Load( const char* pFileName
    , xbool BuildGraphIfNoneAvailable /* = TRUE */ )
{
    X_FILE* Fp;

    m_nNodes = 0;
    m_nEdges = 0;
    m_nNodeEdges = 0;
    Fp = x_fopen( pFileName, "rb" );
    s32 i;
    if( Fp == NULL ) 
    {
        if ( BuildGraphIfNoneAvailable )
        {
            x_DebugMsg( "***%s Not Found, Auto-Generating graph.***\n", pFileName );

            CreatePlaceHolderTerrainGraph();
        
            // save the graph
            Save( pFileName );
        }
    }
    else
    {
        x_fread( this,  sizeof(*this),  1, Fp );

        m_pEdgeList = new edge[m_nEdges];
        ASSERT(m_pEdgeList);

        x_fread( m_pEdgeList,  sizeof(edge),  m_nEdges, Fp );


        m_pNodeList = new node[m_nNodes];
        ASSERT(m_pNodeList);

        x_fread( m_pNodeList,  sizeof(node),  m_nNodes, Fp );

        m_pNodeEdges = new s16[m_nNodeEdges];
        ASSERT(m_pNodeEdges);

        x_fread( m_pNodeEdges,  sizeof(s16),  m_nNodeEdges, Fp );

        m_pNodeInside = new xbool[m_nNodes];
        ASSERT(m_pNodeInside);

        x_fread( m_pNodeInside,  sizeof(xbool),  m_nNodes, Fp );

        x_fread( m_Grid.GridEdgesList, sizeof(s32), m_Grid.nGridEdges, Fp );

        for (i = 0; i < 32; i++)
        {
            x_fread( m_Grid.Cell[i], sizeof(grid_cell), 32,    Fp );
        };
x_fclose( Fp );
    }

    if ( m_Version != VERSION )
    {
        x_printf( "WARNING: Graph file wrong version. Current nav graph may be invalid.",
            m_Version, VERSION );
        m_Version = VERSION;
        if (m_nNodes < 0 || m_nNodes > 5000)
            m_nNodes = 0;
    }
        
    m_nBlockedEdges = 0;
    m_BlockedEdgeTimer.Reset();
    m_BlockedEdgeTimer.Start();

    // Find the "center" of the graph
    s_Center.Zero();
    // BW 9/8/01 - ASSERT was being hit because m_nNodes was zero for some reason. Bad file?
    if (m_nNodes)
    {
        s32 i;
        for ( i = 0; i < m_nNodes; ++i )
        {
            s_Center += m_pNodeList[i].Pos;
            x_memset(m_pNodeList[i].PlayerInfo, 0, MAX_BOTS*sizeof(graph::node::bot_path_info));
        }

        s_Center /= (f32)m_nNodes;
    }
#ifdef TARGET_PC
    // Check all edge values are valid.
    for (i = 0; i < m_nEdges; i++)
    {
        ASSERT(m_pEdgeList[i].CostFromA2B < 5000);
        ASSERT(m_pEdgeList[i].CostFromB2A < 5000);
        ASSERT(m_pEdgeList[i].NodeA       < 5000);
        ASSERT(m_pEdgeList[i].NodeB       < 5000);
    }
#endif
}

//==============================================================================

void graph::Save( const char* pFileName )
{
    s32 i;

    // Check all edge values are valid.
    for (i = 0; i < m_nEdges; i++)
    {
        ASSERT(m_pEdgeList[i].CostFromA2B < 5000);
        ASSERT(m_pEdgeList[i].CostFromB2A < 5000);
        ASSERT(m_pEdgeList[i].NodeA       < 5000);
        ASSERT(m_pEdgeList[i].NodeB       < 5000);
    }

    X_FILE* Fp;

    Fp = x_fopen( pFileName, "wb" );
#ifdef TARGET_PC
    if( !Fp )
    {
        MessageBox(NULL, "Save FAILED!  (File write-protected?)", "Error", MB_OK);
        return;
    }
#endif
    x_fwrite( this,          sizeof(*this),  1,            Fp );
    x_fwrite( m_pEdgeList,   sizeof(edge),   m_nEdges,     Fp );
    x_fwrite( m_pNodeList,   sizeof(node),   m_nNodes,     Fp );
    x_fwrite( m_pNodeEdges,  sizeof(s16),    m_nNodeEdges, Fp );
    x_fwrite( m_pNodeInside, sizeof(xbool),  m_nNodes,     Fp );
#if USING_GRID
    x_fwrite( m_Grid.GridEdgesList, sizeof(s32), m_Grid.nGridEdges, Fp );

    for (i = 0; i < 32; i++)
    {
        x_fwrite(m_Grid.Cell[i], sizeof(grid_cell), 32,    Fp );
    };
#endif
    x_fclose( Fp );
}

//=========================================================================
extern vector3* g_DebugTargetLoc;
#define LABEL_EDGES 0
#ifdef TARGET_PS2
s32 GRAPH_RENDER_PERCENT = 5;
#else
s32 GRAPH_RENDER_PERCENT = 50;
#endif
extern s32 ClosestEdge;
void graph::Render( xbool bUseZBUffer )
{
    s32 i;
    //
    // Render the nodes
    //
    static xbool RenderInside = TRUE;

    vector3 BBoxMin, BBoxMax;

    for( i=0; i<m_nNodes; i++ )
    {
        if ( x_irand( 1, 100 ) < GRAPH_RENDER_PERCENT )
        {
            const node& Node = m_pNodeList[i];
            BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
            BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);

            if (m_pNodeInside[i] && RenderInside)
                draw_BBox( bbox( BBoxMin, BBoxMax), XCOLOR_YELLOW );
            else
                draw_BBox( bbox( BBoxMin, BBoxMax) );
        }
    }
#if USING_PREPROCESSED_VPS
    for (i = 0; i < m_nVantagePoints; i++)
    {
        xcolor Color(XCOLOR_PURPLE);
        vantage_point* VP = m_pVantagePoint[i];
        const node& Node = m_pNodeList[VP->NodeID];
        BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
        BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);
        vector3 BBoxCenter = Node.Pos + vector3(0.0f, 1.3f, 0.0f);
        draw_BBox( bbox( BBoxMin, BBoxMax), Color );
        draw_Begin( DRAW_LINES );
        draw_Color ( Color );
        draw_Vertex( BBoxCenter );
        draw_Vertex( VP->TargetWorldPos );
        draw_End();
    }
#endif

    //
    // Render the edges
    //
    {
        if( bUseZBUffer )
        {
            draw_Begin( DRAW_LINES );
        }
        else
        {
            draw_Begin( DRAW_LINES, DRAW_NO_ZBUFFER );
        }

        for( i=0; i<m_nEdges; i++ )
        {
            if ( x_irand( 1, 100 ) < GRAPH_RENDER_PERCENT )
            {
                const edge& Edge     = m_pEdgeList[i];
                const node& NodeFrom = m_pNodeList[Edge.NodeA];
                const node& NodeTo   = m_pNodeList[Edge.NodeB];
                xcolor      Color;
            
                switch( Edge.Type )
                {
                case graph::EDGE_TYPE_GROWN:   Color.Set(0,255,0,255); break; 
                case graph::EDGE_TYPE_AEREAL:     Color.Set(0,0,255,255); break; 
                case graph::EDGE_TYPE_SKI:     Color.Set(255,255,0,255); break; 
                }
#if LABEL_EDGES
    draw_Label((NodeFrom.Pos + NodeTo.Pos)/2, Color, "%d", i);
#endif
                if (Edge.Flags & EDGE_FLAGS_INDOOR)
                    Color.Set(XCOLOR_YELLOW);

                // Draw Blocked Edges Gray
                if (Edge.Flags & 0x11)
                {
                    if ((Edge.Flags & 0x11) == 0x11)
                    {
                        Color.Set(64,64,64);
                    }
                    else if ((Edge.Flags & 0x11) == 0x01)
                    {
                        Color.Set(XCOLOR_AQUA);
                    }
                    else if ((Edge.Flags & 0x11) == 0x10)
                    {
                        Color.Set(255,128, 0);
                    }
                }

                draw_Color( Color );
                if( Edge.Flags & graph::EDGE_FLAGS_LOOSE )
                {
                    vector3 V = NodeTo.Pos - NodeFrom.Pos;
                    vector3 L = V*1/10.0f;
                    for( s32 k=0;k<10;k++ )
                    {
                        if(k&1) continue;
                        vector3 F,T;
                        F = NodeFrom.Pos + (V*(k/10.0f));
                        T = F + L;
                        draw_Vertex( F );
                        draw_Vertex( T );
                    }
                }
                else
                {
                    draw_Vertex( NodeFrom.Pos );
                    draw_Vertex( NodeTo.Pos );
                }
            }
        }

        // Draw closest edge
#if SHOW_CLOSEST_EDGE
        draw_Color(XCOLOR_RED);
        draw_Vertex(m_pNodeList[m_pEdgeList[ClosestEdge].NodeA].Pos);
        draw_Vertex(m_pNodeList[m_pEdgeList[ClosestEdge].NodeB].Pos);
#endif
        draw_End();
    }
}

//==============================================================================

xbool graph::GetEdgeBetween  ( edge& Edge
    , const node& NodeA
    , const node& NodeB ) const
{
    s32 i;
    
    for ( i = 0; i < m_nEdges; ++i )
    {
        if ( ((m_pNodeList[m_pEdgeList[i].NodeA] == NodeA)
            && (m_pNodeList[m_pEdgeList[i].NodeB] == NodeB))
          || ((m_pNodeList[m_pEdgeList[i].NodeB] == NodeA)
              && (m_pNodeList[m_pEdgeList[i].NodeA] == NodeB)) )
        {
            Edge = m_pEdgeList[i];
            return TRUE;
        }
    }

    return FALSE;
}

//==============================================================================

const graph::node& graph::GetEdgeStart ( const xarray<u16>& EdgeIdxList
    , s32 WhichEdge ) const
{
    ASSERT( EdgeIdxList.GetCount() > 1 ); // we need two edges to determine this
    
    // if this is the first edge in the list, then we have to compare with
    // the next edge
    const edge& CurEdge = m_pEdgeList[EdgeIdxList[WhichEdge]];
    
    if ( WhichEdge == 0 )
    {
        // compare with WhichEdge+1
        const edge& NextEdge = m_pEdgeList[EdgeIdxList[WhichEdge+1]];
        
        if ( (CurEdge.NodeA == NextEdge.NodeA)
            || (CurEdge.NodeA == NextEdge.NodeB) )
        {
            // NodeA is the match, so return NodeB
            return m_pNodeList[CurEdge.NodeB];
        }
        else
        {
            // NodeB is the match
            return m_pNodeList[CurEdge.NodeA];
        }
    }
    else
    {
        // compare with WhichEdge-1
        const edge& PrevEdge = m_pEdgeList[EdgeIdxList[WhichEdge-1]];
        
        if ( (CurEdge.NodeA == PrevEdge.NodeA)
            || (CurEdge.NodeA == PrevEdge.NodeB) )
        {
            // NodeA is the Match
            return m_pNodeList[CurEdge.NodeA];
        }
        else
        {
            return m_pNodeList[CurEdge.NodeB];
        }
    }
}

//==============================================================================

const graph::node& graph::GetEdgeEnd    ( const xarray<u16>& EdgeIdxList
    , s32 WhichEdge ) const
{
    ASSERT( EdgeIdxList.GetCount() > 1 ); // we need two edges to determine this
    
    // if this is the last edge in the list, then we have to compare with
    // the previous edge
    const edge& CurEdge = m_pEdgeList[EdgeIdxList[WhichEdge]];

    if ( WhichEdge == EdgeIdxList.GetCount() - 1 )
    {
        // compare with WhichEdge-1
        const edge& PrevEdge = m_pEdgeList[EdgeIdxList[WhichEdge-1]];

        if ( (CurEdge.NodeA == PrevEdge.NodeA)
            || (CurEdge.NodeA == PrevEdge.NodeB) )
        {
            // NodeA is the match return NodeB
            return m_pNodeList[CurEdge.NodeB];
        }
        else
        {
            // NodeB is the match, return NodeA
            return m_pNodeList[CurEdge.NodeA];
        }
    }
    else
    {
        // compare with WhichEdge+1
        const edge& NextEdge = m_pEdgeList[EdgeIdxList[WhichEdge+1]];

        if ( (CurEdge.NodeA == NextEdge.NodeA)
            || (CurEdge.NodeA == NextEdge.NodeB) )
        {
            // NodeA is the Match
            return m_pNodeList[CurEdge.NodeA];
        }
        else
        {
            // NodeB is the match
            return m_pNodeList[CurEdge.NodeB];
        }
    }
}

//==============================================================================
void graph::BlockEdge( s32 EdgeID, u8 Flags )
{
    ASSERT( EdgeID >= 0 );
    ASSERT( EdgeID < m_nEdges );
    edge& Edge = m_pEdgeList[EdgeID];
    Edge.Flags |= Flags;

    //--------------------------------------------------
    // See if the edge is already blocked, if so, just
    // update it's time.
    //--------------------------------------------------
    s32 i;

    blocked_edge* pBlockedEdge = NULL;
    
    for ( i = 0; i < m_nBlockedEdges; ++i )
    {
        if ( m_BlockedEdges[i].EdgeID == EdgeID )
        {
            pBlockedEdge = &m_BlockedEdges[i];
            break;
        }
    }

    if ( pBlockedEdge == NULL )
    {
        // new blocked edge, append to the end...
        if ( m_nBlockedEdges == MAX_BLOCKED_EDGES )
        {
            UnblockOldestEdge();
        }

        ASSERT( m_nBlockedEdges < MAX_BLOCKED_EDGES );
        pBlockedEdge = &m_BlockedEdges[m_nBlockedEdges];
        ++m_nBlockedEdges;
        pBlockedEdge->EdgeID = EdgeID;
    }

    pBlockedEdge->TimeBlocked = m_BlockedEdgeTimer.ReadSec();
}

//==============================================================================
// UpdateBlockedEdges()
//
// Unblocks edges that have been blocked for BLOCKED_EDGE_TIMEOUT seconds
//==============================================================================
void graph::UpdateBlockedEdges( void )
{
    // TODO: this will unblock an edge for both teams, regardless of who
    // blocked it. This should maybe be fixed, although this isn't terrible...
    
    const f32 TimeSec = m_BlockedEdgeTimer.ReadSec();

    s32 i;

    for ( i = 0; i < m_nBlockedEdges; ++i )
    {
        if ( (TimeSec - m_BlockedEdges[i].TimeBlocked) > BLOCKED_EDGE_TIMEOUT )
        {
            // This block is too old, unblock it
            m_pEdgeList[m_BlockedEdges[0].EdgeID].Flags
                &= !EDGE_FLAGS_TEAM_0_BLOCK;
            m_pEdgeList[m_BlockedEdges[0].EdgeID].Flags
                &= !EDGE_FLAGS_TEAM_1_BLOCK;
            UnblockEdge( i );
        }
    }
}

//==============================================================================
// UnblockOldestBlockedEdge()
//==============================================================================
void graph::UnblockOldestEdge( void )
{
    s32 i;
    f32 OldestTime = 0.0f;
    s32 OldestIndex = -1;

    for ( i = 0; i < m_nBlockedEdges; ++i )
    {
        if ( m_BlockedEdges[i].TimeBlocked > OldestTime )
        {
            OldestTime = m_BlockedEdges[i].TimeBlocked;
            OldestIndex = i;
        }
    }

    if ( OldestIndex >= 0 )
    {
        // found the oldest, so unblock it
        UnblockEdge( OldestIndex );
    }
}

//==============================================================================
// UnblockEdge()
//==============================================================================
void graph::UnblockEdge( s32 BlockedIdx )
{
    ASSERT( BlockedIdx < m_nBlockedEdges );
    ASSERT( BlockedIdx >= 0 );

    // copy over the edge to unblock with the last edge, and decrement count
    m_BlockedEdges[BlockedIdx] = m_BlockedEdges[m_nBlockedEdges-1];
    --m_nBlockedEdges;
}

//==============================================================================
xbool graph::DebugConnected( void ) const 
{
    xbool* pVisited = new xbool[m_nNodes];
    x_memset( pVisited, 0, m_nNodes*sizeof( xbool ) );

    DoVisitedFloodFill( 0, pVisited );

    // Make sure we've visited all nodes
    s32 i;
    xbool RetVal = TRUE;

    for (i = 0; i < m_nNodes; i ++)
    {
        m_pNodeList[i].PlayerInfo[0].SetVisited(0);
    }
    
    for ( i = 0; i < m_nNodes; ++i ) 
    {
        if ( pVisited[i] == FALSE )
        {
            m_pNodeList[i].PlayerInfo[0].SetVisited(1);
            if ( RetVal )
            {
                // first time through -- log a header
                bot_log::Log( "\nGraph is not connected. Unable to reach nodes at\n" );
                bot_log::Log( "================================================\n" );
                RetVal = FALSE;
            }

            bot_log::Log( "( %8.1f, %8.1f, %8.1f )\n"
                , m_pNodeList[i].Pos.X
                , m_pNodeList[i].Pos.Y
                , m_pNodeList[i].Pos.Z );
            
        }
    }

    if ( !RetVal ) 
    {
                bot_log::Log( "================================================\n" );
        bot_log::Log( "End of unreachable node positions\n\n" );
    }

    return RetVal;
}

//==============================================================================
void graph::DoVisitedFloodFill( s32 NodeId, xbool* pVisited ) const
{
    if ( pVisited[NodeId] == TRUE )
    {
        return;
    }
    else
    {
        pVisited[NodeId] = TRUE;

        xarray<s32> Children;
        DebugGetChildren( NodeId, Children );

        s32 i;

        for ( i = 0; i < Children.GetCount(); ++i )
        {
            DoVisitedFloodFill( Children[i], pVisited );
        }
    }
}

//==============================================================================
void graph::DebugGetChildren( s32 NodeId, xarray<s32>& Children ) const
{
    s32 i;

    for ( i = 0; i < g_Graph.m_nEdges; ++i )
    {
        const graph::edge& CurEdge = g_Graph.m_pEdgeList[i];
        
        ASSERT( CurEdge.NodeA != CurEdge.NodeB ); // pointless edge
        
        if ( CurEdge.NodeA == NodeId )
        {
            Children.Append( CurEdge.NodeB );
        }
        else if ( CurEdge.NodeB == NodeId )
        {
            Children.Append( CurEdge.NodeA );
        }
    }
}


//==============================================================================
//==============================================================================

// debug testing function:
xbool   graph::IsIdenticalToFile(const char* pFileName)
{
    graph *GraphFile = new graph;

    GraphFile->Load(pFileName);

    if (GraphFile->m_nEdges != m_nEdges)
    {
        x_DebugMsg ("Different number of Edges!\n");

        delete GraphFile;
        return FALSE;
    }

    if (GraphFile->m_nNodes != m_nNodes)
    {
        x_DebugMsg ("Different number of Nodes!\n");
        delete GraphFile;
        return FALSE;
    }

    if (GraphFile->m_nNodeEdges != m_nNodeEdges)
    {
        x_DebugMsg ("Different number of NodeEdges!\n");
        delete GraphFile;
        return FALSE;
    }

    s32 i;
    for (i = 0; i < m_nEdges; i++)
    {
        if (!EdgesEqual(m_pEdgeList[i], GraphFile->m_pEdgeList[i]))
        {
            x_DebugMsg ("Edge #%d differs between graph file and memory!\n", i);
            delete GraphFile;
            return FALSE;
        }
    }
    for (i = 0; i < m_nNodes; i++)
    {
        if (!NodesEqual(m_pNodeList[i], GraphFile->m_pNodeList[i]))
        {
            x_DebugMsg ("Node #%d differs between graph file and memory!\n", i);
            delete GraphFile;
            return FALSE;
        }
    }
    for (i = 0; i < m_nNodeEdges; i++)
    {
        if (m_pNodeEdges[i] != GraphFile->m_pNodeEdges[i])
        {
            x_DebugMsg ("Node-Edge #%d differs between graph file and memory!\n", i);
            delete GraphFile;
            return FALSE;
        }
    }

    delete GraphFile;
    return TRUE;
}

xbool graph::EdgesEqual(const graph::edge &A, const graph::edge &B) const
{
    if (A.NodeA != B.NodeA || A.NodeB != B.NodeB || A.Type != B.Type)
        return FALSE;
    else 
        return TRUE;
}

xbool graph::NodesEqual(const graph::node &A, const graph::node &B) const
{
    if (A.nEdges != B.nEdges || A.EdgeIndex != B.EdgeIndex)
        return FALSE;
    else 
        return (A==B);
}

s32 graph::GetNClosestNodes( s32 N, const vector3& Pos, s32* pNodeList ) const
{
    ASSERT( pNodeList );
    ASSERT( N < 20 );

    f32 pDistances[20];

    s32 i;


    s32 j;
    s32 NumNodes = 0;

    for ( i = 0; i < N; ++i )
    {
        pDistances[i] = F32_MAX;
    }
    
    for ( i = 0; i < m_nNodes; ++i )
    {
        for ( j = 0; j < N; ++j )
        {
            const f32 DistSQ = (Pos - m_pNodeList[i].Pos).LengthSquared();
            if ( DistSQ < pDistances[j] && DistSQ > 0.00001f )
            {
                s32 k;

                // insert DistSQ into pDistances and insert i into pNodeList
                for ( k = N-1; k > j; --k )
                {
                    pDistances[k] = pDistances[k-1];
                    pNodeList[k] = pNodeList[k-1];
                }
                pDistances[j] = DistSQ;
                pNodeList[j] = i;
                ++NumNodes;
                NumNodes = MIN( NumNodes, N );
                break;
            }
        }
    }

    return NumNodes;
}

s32 graph::GetNGroundPointsInRadius( s32 N
    , const vector3& Pos
    , f32 Radius
    , vector3* pPoints ) const
{
    ASSERT( pPoints );
    ASSERT( N >= 0 );

    s32 i;
    s32 NumNodes = 0;
    const f32 RadiusSQ = x_sqr( Radius );

    for ( i = 0; (i < m_nNodes) && (NumNodes < N); ++i )
    {
        // JP: According to Alex, we dont need to make sure the point is outside
        // The reason it's commented out is so that in the training missions,
        // the player is spawned INSIDE the underground base and not on the terrain above
    
        //if (!m_pNodeInside[i] 
		if( TRUE
            && NodeIsOnGround( i )
            && (m_pNodeList[i].Pos - Pos).LengthSquared() <= RadiusSQ )
        {
            pPoints[NumNodes] = m_pNodeList[i].Pos;
            ++NumNodes;
        }
    }

    return NumNodes;
}

void graph::CreatePlaceHolderTerrainGraph( void )
{
    // grab the terrain
    ObjMgr.StartTypeLoop( object::TYPE_TERRAIN );
    terrain* Terrain = (terrain*)(ObjMgr.GetNextInType());
    ObjMgr.EndTypeLoop();
    ASSERT( Terrain );
    terr* pTerr = &(Terrain->GetTerr());

    const bbox& Bounds = GameMgr.GetBounds();
    static const s32 NODE_SPACING = 50;
    static const s32 NUM_EDGES_PER_NODE = 8;
    s32 NumX = ((s32)x_floor( Bounds.Max.X - Bounds.Min.X )) / NODE_SPACING;
    s32 NumZ = ((s32)x_floor( Bounds.Max.Z - Bounds.Min.Z )) / NODE_SPACING;

        // allocate space for the graph
    delete [] m_pNodeList;
    delete [] m_pEdgeList;
    delete [] m_pNodeEdges;

    m_pNodeList = new node[NumX * NumZ];
    m_pEdgeList = new edge[NumX * NumZ * NUM_EDGES_PER_NODE];

    // place nodes
    f32 X;
    f32 Z;
    s32 nX;
    s32 nZ;
    m_nNodes = 0;
    for ( X = Bounds.Min.X, nX = 0
              ; nX < NumX
              ; X += NODE_SPACING, ++ nX )
    {
        for ( Z = Bounds.Min.Z, nZ = 0
                  ; nZ < NumZ
                  ; Z += NODE_SPACING, ++nZ )
        {
            const f32 Y = pTerr->GetHeight( Z, X );
            const vector3 NewNodePos( X, Y, Z );
            if ( !PointInsideBuilding( NewNodePos ) )
            {
                // drop a node on the ground here
                m_pNodeList[m_nNodes].Pos = NewNodePos;
                ++m_nNodes;
            }
        }
    }

    // create edges
    s32 i;
    s32 j;
    s32 pClosestNodes[NUM_EDGES_PER_NODE];
    edge Dummy;
    m_nEdges = 0;

    for ( i = 1; i < m_nNodes; ++i )
    {
        VERIFY( GetNClosestNodes( NUM_EDGES_PER_NODE, 
                                  m_pNodeList[i].Pos, 
                                  pClosestNodes ) == NUM_EDGES_PER_NODE );

        for ( j = 0; j < NUM_EDGES_PER_NODE; ++j )
        {
            const node& NodeA = m_pNodeList[i];
            const node& NodeB = m_pNodeList[pClosestNodes[j]];

            if ( !GetEdgeBetween( Dummy, NodeA, NodeB )
                && NodesCanSeeEachOther( NodeA, NodeB ) )
            {
                // Add the edge...
                edge& Edge = m_pEdgeList[m_nEdges];
                Edge.NodeA = i;
                Edge.NodeB = pClosestNodes[j];
                Edge.Type = graph::EDGE_TYPE_GROWN;
                Edge.Flags = graph::EDGE_FLAGS_LOOSE;
                Edge.CostFromA2B
                    = (u16)(( NodeA.Pos - NodeB.Pos ).Length());
                Edge.CostFromB2A = Edge.CostFromA2B;

                ++m_nEdges;
            }
        }
    }

    // build node edge list
    m_pNodeEdges = new s16[2 * m_nEdges];
    m_nNodeEdges = 0;

    for ( i = 0; i < m_nNodes; ++i )
    {
        // find all the edges for this node
        node& Node = m_pNodeList[i];
        Node.EdgeIndex = m_nNodeEdges;

        for ( j = 0; j < m_nEdges; ++j )
        {
            edge& Edge = m_pEdgeList[j];
            if ( Edge.NodeA == i || Edge.NodeB == i )
            {
                m_pNodeEdges[m_nNodeEdges] = j;
                ++m_nNodeEdges;
            }
        }

        Node.nEdges = m_nNodeEdges - Node.EdgeIndex;
    }
        
    ASSERT( m_nNodeEdges == 2 * m_nEdges );
}

xbool graph::NodesCanSeeEachOther( const node& NodeA, const node& NodeB ) const
{
    xbool RetVal = FALSE;
    
    collider            Ray;

    if ( NodeA.Pos == NodeB.Pos )
    {
        RetVal = TRUE;
    }
    else
    {
        Ray.RaySetup    ( NULL, NodeA.Pos, NodeB.Pos );
        ObjMgr.Collide  ( object::ATTR_BUILDING, Ray );

        if ( Ray.HasCollided() == TRUE )
        {
            // see if the collision point is beyond the point we want to see
            collider::collision Collision;
            Ray.GetCollision( Collision );
            const f32 DistSQ = (NodeB.Pos - NodeA.Pos).LengthSquared();
            const f32 CollisionDistSQ
                = (NodeA.Pos - Collision.Point).LengthSquared();
            RetVal =  CollisionDistSQ > DistSQ;
        }
        else
        {
            RetVal = TRUE;
        }
    }
    
    return RetVal;
}

xbool graph::PointInsideBuilding( const vector3& Point ) 
{
    xbool bInsideBuilding = FALSE;
    building_obj* pBuilding = NULL;

    ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
    while( (pBuilding = (building_obj*)ObjMgr.GetNextInType()) )
    {
        if( pBuilding->GetBBox().Intersect( Point ) )
        {
            bInsideBuilding = TRUE;
            break;
        };

        /*
        if( pBuilding->FindZoneFromPoint( Point ) )
        {
        bInsideBuilding = TRUE;
        break;
        };
        */
    }
    ObjMgr.EndTypeLoop();

    return bInsideBuilding;
}

xbool graph::NodeIsOnGround( s32 NodeId ) const 
{
    ASSERT( NodeId < m_nNodes );
    s32 i;
    const node& Node = m_pNodeList[NodeId];

    for ( i = Node.EdgeIndex; i < Node.EdgeIndex + Node.nEdges; ++i )
    {
        if ( m_pEdgeList[m_pNodeEdges[i]].Type == graph::EDGE_TYPE_GROWN )
        {
            return TRUE;
        }
    }

    return FALSE;
}

const vector3& graph::GetCenter( void ) const
{
    return s_Center;
}

//==============================================================================

u8    graph::node::bot_path_info::GetVisited      ( void )
{
    return (Data >> 24);
}

//==============================================================================

u16   graph::node::bot_path_info::GetCostSoFar    ( void )
{
    return ((Data >> 12) & ((1 << 12) - 1));
}

//==============================================================================

u16   graph::node::bot_path_info::GetPrev         ( void )
{
    return ((Data >> 0) & ((1 << 12) - 1));
}

//==============================================================================

void  graph::node::bot_path_info::SetVisited      ( u8 Val )
{
    // Clear the old data.
    u32 ClearMask = (1<<24) - 1; // 0000 0000 1111 1111 1111 1111 1111 1111
    Data &= ClearMask;           // 0000 0000 data data data data data data

    // Write the new data.
    u32 NewData = Val;           // 0000 0000 0000 0000 0000 0000 xxxx xxxx
    NewData <<= 24;              // xxxx xxxx 0000 0000 0000 0000 0000 0000
    Data |= NewData;            //  xxxx xxxx data data data data data data
}

//==============================================================================

void  graph::node::bot_path_info::SetCostSoFar    ( u16 Val )
{
//    ASSERT(Val < (1<<12));
    if (Val >= (1<<12))
    {   
        // Useful info output in PathManager.cpp
#if defined acheng
        x_printf("Truncating %d to %d**\n", Val, (1<<12) - 1);
        x_DebugMsg("Truncating %d to %d**\n", Val, (1<<12) - 1);
#endif
        Val = (1<<12) - 1;
    }

    // Clear the old data.  // 1111 1111 0000 0000 0000 1111 1111 1111
    u32 ClearMask = ~(((1<<12) - 1) << 12);
    Data &= ClearMask;

    // Write the new data.
    u32 NewData = Val;
    NewData <<= 12;
    Data |= NewData;
}

//==============================================================================

void  graph::node::bot_path_info::SetPrev         ( u16 iNode )
{
    ASSERT(iNode < (1<<12));

    // Clear the old data. 
    u32 ClearMask = ~((1<<12) - 1); // 1111 1111 1111 1111 1111 0000 0000 0000
    Data &= ClearMask;

    // Write the new data.
    u32 NewData = iNode;
    Data |= NewData;
}
#if SHOW_EDGE_COSTS
//==============================================================================
void graph::ShowEdgeCosts ( void )
{
    s32 i;
    const view&     View        = *eng_GetView(0);
    for (i = 0; i < m_nEdges; i++)
    {
        vector3 EdgeCenter = (m_pNodeList[m_pEdgeList[i].NodeA].Pos
            + m_pNodeList[m_pEdgeList[i].NodeB].Pos ) * 0.5;

static s32 CullDist = 200;
static s32 CullDistSq = CullDist * CullDist;
        if ((View.GetPosition() - EdgeCenter).LengthSquared() < CullDistSq
            && g_EdgeCost[i] > 0)
        {
            draw_Label(EdgeCenter, XCOLOR_YELLOW, "%d", g_EdgeCost[i]);
        }
    }

}
//==============================================================================

void graph::ResetEdgeCosts ( void )
{
    x_memset(g_EdgeCost, 0, sizeof(g_EdgeCost));
}

#endif
//==============================================================================

#if USING_GRID
    
s32 graph::GetRow( f32 ZPos) const
{
    // Return the row, between 0 and 31.
    f32 RelPos = ZPos - m_Grid.Min.Z;

    s32 GridPoint = (s32)(RelPos/ m_Grid.CellDepth);

    if (GridPoint < 1)
        return 0;
    else if (GridPoint > 30)
        return 31;
    else 
        return GridPoint;
}

//==============================================================================

s32 graph::GetColumn( f32 XPos) const
{
    // Return the row, between 0 and 31.
    f32 RelPos = XPos - m_Grid.Min.X;

    s32 GridPoint = (s32)(RelPos/ m_Grid.CellWidth);

    if (GridPoint < 1)
        return 0;
    else if (GridPoint > 30)
        return 31;
    else 
        return GridPoint;
}

//==============================================================================

void graph::GetCellBBox(const vector3& Pos, bbox& Box ) const
{
    GetCellBBox(GetColumn(Pos.X), GetRow(Pos.Z), Box);
}

//==============================================================================

void graph::GetCellBBox(s32 X, s32 Z, bbox& Box ) const
{
    static const vector3 CellSize(m_Grid.CellWidth, 
                                  m_Grid.Max.Y - m_Grid.Min.Y, 
                                  m_Grid.CellDepth);
    static const vector3 Padding(0.1f, 0.1f, 0.1f);

    vector3 Min(m_Grid.Min.X + (m_Grid.CellWidth * X),
                m_Grid.Min.Y,
                m_Grid.Min.Z + (m_Grid.CellDepth * Z));

    Box.Set(Min - Padding, Min + CellSize + Padding);
}

//==============================================================================

#endif
