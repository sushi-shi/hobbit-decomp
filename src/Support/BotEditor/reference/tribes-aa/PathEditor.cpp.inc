#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "PathEditor.hpp"
#include <xCore/Entropy/e_Draw.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include "Objects/Bot/BotObject.hpp"
#include "Objects/Player/DefaultLoadouts.hpp"
#include "Objects/Player/PlayerObject.hpp"
#include "Building/BuildingOBJ.hpp"
#include "resource.h" 
#include "GameMgr/GameMgr.hpp"
#include <Support/ObjectMgr/Collider.hpp>
#include <stdio.h>
#include "Objects/Bot/Graph.hpp"
#include "objects/Bot/BotLog.hpp"
#include "Objects/Projectiles/Flag.hpp"
#include "Objects/Projectiles/Nexus.hpp"
#define LOADINIT 0
#define TEMP_FILE "temp.gph"
#define VERSION_NUMBER 2
#define MAX_TIME_SLICE 3.0
#define EDGE_COST_MODIFIERS_ON 1
//==============================================================================
//  VARIABLES
//==============================================================================
xbool g_bCostAnalysisOn = FALSE;
xbool g_DrawEdgeLabels = FALSE;
xbool            bShowBounds = FALSE;
xbool   g_NodeAdjusted = FALSE;
extern graph            g_Graph;
extern xbool   g_RunningPathEditor;
extern xbool bUpdated;
extern object::id g_BotObjectID;
extern bot_object s_pBotObject;

grid_cell* LargestCell = NULL;

//==============================================================================
//  FUNCTIONS
//==============================================================================

xbool PointInsideBuilding( const vector3& Point )
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
    }
    ObjMgr.EndTypeLoop();

    // Building bbox check.
    if (!bInsideBuilding)
        return FALSE;

    // Do a ray check from the point to the top of the bbox.
    ASSERT(pBuilding);

    vector3 Top(Point);
    Top.Y = pBuilding->GetBBox().Max.Y + 1.0f;

    collider Ray;
    Ray.RaySetup(NULL, Point, Top, -1, TRUE);
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray);

    return Ray.HasCollided();
}

//==============================================================================

void path_editor::InitClass( void )
{
    m_hSelectedNode     = -1;
    m_hSelectedEdge     = -1;
    m_hSelectedSpawn    = -1;

    m_EdgeList.DeleteAll( TRUE );
    m_NodeList.DeleteAll( TRUE );

    x_memset( &m_ActiveEdge, 0, sizeof( edge ) );
    x_memset( &m_pBots, NULL, sizeof( m_pBots ) );
    x_memset( &iMinVis, 0, sizeof(iMinVis));

    m_EdgeType = EDGE_TYPE_GROWN;
    m_bLoose   = TRUE;
    m_bChanged = FALSE;
    
    m_bBotInit = FALSE;

    // Cost Analysis Variable
    if (m_GraphToPathHandle)
    {
        delete [] m_GraphToPathHandle;
        m_GraphToPathHandle = NULL;
    }
    m_pFirstMinNode     = NULL;

    m_nMarks = 0;
}

//=========================================================================

path_editor::path_editor ( void )
{
    m_Grid.Cell = NULL;
    m_GraphToPathHandle = NULL;
    InitClass();
}

//=========================================================================

path_editor::~path_editor( void )
{
    InitClass();
}

//=========================================================================
static 
f32 SquareDistanceLine( const vector3& Point, 
                        const vector3& Start,
                        const vector3& End,
                        f32*           pT = NULL )
{
    vector3 Diff = Point - Start;
    vector3 Dir  = End   - Start;
    f32     fT   = Diff.Dot( Dir );

    if( fT <= 0.0 )
    {
        fT = 0.0;
    }
    else
    {
        f32 fSqrLen = Dir.Dot( Dir );

        if ( fT >= fSqrLen )
        {
            fT = 1.0;
            Diff -= Dir;
        }
        else
        {
            fT /= fSqrLen;
            Diff -= fT * Dir;
        }
    }

    if ( pT )
        *pT = fT;

    return Diff.Dot( Diff );
}


//=========================================================================

f32 path_editor::FindClosestNode( const vector3& Pos, xhandle& hHandle )
{
    s32 Count   = m_NodeList.GetCount();
    f32 MinDist = 999999999.9f;
    hHandle     = -1;

    for( s32 i=0; i<Count; i++ )
    {
        node&   Node = m_NodeList[i];
        vector3 V    = Node.Pos - Pos;
        f32     d    = V.Dot( V );

        if( d < MinDist )
        {
            MinDist = d;
            hHandle   = m_NodeList.GetHandleFromIndex( i );
        }            
    }

    return MinDist;
}

//=========================================================================

f32 path_editor::FindClosestEdge( const vector3& Pos, xhandle& hHandle )
{
    s32 Count   = m_EdgeList.GetCount();
    f32 MinDist = 999999999.9f;
    hHandle     = -1;

    for( s32 i=0; i<Count; i++ )
    {
        edge&   Edge    = m_EdgeList[i];
        node&   NodeA   = m_NodeList( Edge.hNodeA );
        node&   NodeB   = m_NodeList( Edge.hNodeB );
        f32     d       = SquareDistanceLine( Pos, NodeA.Pos, NodeB.Pos );

        if( d < MinDist )
        {
            MinDist = d;
            hHandle   = m_EdgeList.GetHandleFromIndex( i );
        }            
    }

    return MinDist;
}

//=========================================================================

void path_editor::AddEdgeToNode( node& Node, xhandle hEdge )
{
    // Check if we already have it
    for( s32 i=0; i<Node.nEdges; i++ )
    {
        if( hEdge == Node.hEdgeList[i] ) return;
    }

    // Add up the ref count for the edge
    m_EdgeList(hEdge).RefCount++;

    ASSERT( Node.nEdges <= MAX_EDGES_NODE );
    Node.hEdgeList[ Node.nEdges ] = hEdge;
    Node.nEdges++;
    m_bChanged = TRUE;
}

//=========================================================================

void path_editor::DeleteEdge( xhandle hEdge )
{
    s32 Count   = m_NodeList.GetCount();

    //
    // Free the edge from all the nodes
    //
    for( s32 i=0; i<Count; i++ )
    {
        node&   Node   = m_NodeList[i];

        for( s32 e=0; e<Node.nEdges; e++ )
        {
            if( Node.hEdgeList[e] == hEdge )
            {
                // Collapse the edge list
                Node.nEdges--;
                x_memmove( &Node.hEdgeList[e], &Node.hEdgeList[e+1], sizeof(s32)*(Node.nEdges-e) );
            }
        }
    }

    //
    // Delete the edge from the main list
    //
    m_EdgeList.DeleteNodeByHandle( hEdge );
    m_bChanged = TRUE;
}

//=========================================================================

void path_editor::DeleteActiveEdge( void )
{
    if( m_hSelectedEdge != -1 )
    {
        DeleteEdge( m_hSelectedEdge );
        m_hSelectedEdge = -1;
        m_bChanged = TRUE;
    }
}

//=========================================================================

void path_editor::SelectNode( const vector3& Pos )
{
    FindClosestNode( Pos, m_hSelectedNode );
}

//=========================================================================

void path_editor::RemoveEdgeFromNode( node& Node, xhandle hEdge )
{
    for( s32 i=0; i<Node.nEdges; i++ )
    {
        if( Node.hEdgeList[i] == hEdge )
        {
            //
            // Update the edge info.
            //
            edge& Edge = m_EdgeList( hEdge );
            Edge.RefCount--;

            if( Edge.RefCount <= 0 )
            {
                m_EdgeList.DeleteNodeByHandle( hEdge );
            }

            //
            // Update the list of edges
            //
            Node.nEdges--;
            x_memmove( &Node.hEdgeList[i], &Node.hEdgeList[i+1], sizeof(s32)*(Node.nEdges-i) );
        
            m_bChanged = TRUE;

            // Assume that there is not duplicates
            return;
        }
    }    
}

//=========================================================================

void path_editor::DeleteNode( xhandle hNode )
{
    node& Node = m_NodeList( hNode );

    while( Node.nEdges > 0 )
    {
        xhandle     hEdge  = Node.hEdgeList[0];
        edge&       Edge   = m_EdgeList( hEdge );

        // Remove the edge references
        RemoveEdgeFromNode( m_NodeList(Edge.hNodeA), hEdge );
        RemoveEdgeFromNode( m_NodeList(Edge.hNodeB), hEdge );
    }

    // Delete the actual node
    m_NodeList.DeleteNodeByHandle( hNode );
    m_bChanged = TRUE;
}

//=========================================================================

void path_editor::RemoveSelectedNode( void )
{
    if( m_hSelectedNode != -1 ) 
    {
        if( m_hSelectedSpawn == m_hSelectedNode )
            m_hSelectedSpawn = -1;

        DeleteNode( m_hSelectedNode );
        m_hSelectedNode = -1;
        m_bChanged = TRUE;
    }
}

//=========================================================================

xhandle path_editor::AddEdge( edge& NewEdge )
{
    ASSERT( NewEdge.hNodeA != NewEdge.hNodeB );

    //
    // Make sure that the node is order corectly
    //
    if( NewEdge.hNodeA > NewEdge.hNodeB )
    {
        xhandle      i = NewEdge.hNodeA;
        NewEdge.hNodeA = NewEdge.hNodeB;
        NewEdge.hNodeB = i;
    }

    // Take our node
    xhandle hEdge;
    node&   NodeA  = m_NodeList( NewEdge.hNodeA );
    node&   NodeB  = m_NodeList( NewEdge.hNodeB );
    s32     nEdges = m_EdgeList.GetCount();
    
    //
    // Check whether we already have that edge
    //
    for( s32 i=0; i<nEdges; i++ )
    {
        edge& Edge = m_EdgeList[i];

        if( Edge.hNodeA == NewEdge.hNodeA && 
            Edge.hNodeB == NewEdge.hNodeB )
            break;
    }

    //
    // Set the paramters into the new edge
    //
    NewEdge.bLoose  = m_bLoose;
    NewEdge.Type    = m_EdgeType;


    //
    // Add the edge if we didn't found a match
    //
    if( i == nEdges )
    {
        // Add the actual edge in the list
        m_EdgeList.AddNode( hEdge )  = NewEdge;        
    }
    else
    {
        hEdge = m_EdgeList.GetHandleFromIndex( i );
    }

    //
    // Add the actual edge to the Node
    //
    AddEdgeToNode( NodeA, hEdge );
    AddEdgeToNode( NodeB, hEdge );

    m_bChanged = TRUE;
    return hEdge;
}


//=========================================================================

void path_editor::AddNode( const vector3& Pos )
{
    vector3 GroundPos = Pos;

    if (m_EdgeType != EDGE_TYPE_AIR )
    {
        // Fire ray straight down
        vector3 Dir(0.0f, -5000.0f, 0.0f);

        collider  Ray;
        Ray.RaySetup( NULL, Pos, Pos+Dir );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );

        if (!Ray.HasCollided())
        {
            vector3 BackUp(0.0f, 10.0f, 0.0f);
            do
            {
                GroundPos += BackUp;
                Ray.RaySetup( NULL, GroundPos, GroundPos+Dir );
                ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
                if (GroundPos.Y > 5000.0f)
                {
                    ASSERT(!"Can't Find Ground Level!");
                }
            }
            while (!Ray.HasCollided());
        }   

        GroundPos = GroundPos + Ray.GetCollisionT()*Dir;
    }

    xhandle hHandle; 
    s32     bNewNode = ( FindClosestNode( GroundPos, hHandle ) > 4 );
    node&   Node     = ( bNewNode ) ? m_NodeList.AddNode( hHandle ) : m_NodeList( hHandle );

    //
    // If it is a new node clear it up
    //
    if( bNewNode )
    {
        x_memset( &Node, 0, sizeof(Node) );
    }

    //
    // Add the edge in the pool
    //
    if( m_hSelectedNode != -1 && m_hSelectedNode != hHandle )
    {
        // Compleat the edge
        m_ActiveEdge.hNodeA = m_hSelectedNode;
        m_ActiveEdge.hNodeB = hHandle;

        // Add the new edge
        AddEdge( m_ActiveEdge );
    }

    //
    // Add more params to the new node
    //
    if( bNewNode )
    {
        Node.Pos      = GroundPos;
    }

    //
    // Set the active Handle node
    //
    m_hSelectedNode = hHandle;
    m_bChanged = TRUE;

    if (m_EdgeType != EDGE_TYPE_AIR )
    {
        // Raise the node up to a clear level.
        DropNode();
    }
    else
    {
        if (ScanNodeForCollision( Node ))
            LowerNode();
    }
}

//=========================================================================
extern vector3* g_DebugTargetLoc;
void path_editor::Render( xbool bUseZBUffer )
{
    const view&     View        = *eng_GetView(0);
    const vector3   Pos         = View.GetPosition();
    eng_Begin();

    s32 X = g_Graph.GetColumn(Pos.X);
    s32 Z = g_Graph.GetRow(Pos.Z);
    x_printfxy(5, 35, "Cell (%d,%d)", X, Z);
    x_printfxy(5, 36, "Cell Edges  = %d", g_Graph.m_Grid.Cell[X][Z].nEdges);
    bbox Box;
    g_Graph.GetCellBBox(X,Z, Box);
    draw_BBox(Box, XCOLOR_WHITE);

    x_printfxy(30, 35, "Grid Cell Width = %f", m_Grid.CellWidth);
    x_printfxy(30, 36, "Grid Cell Depth = %f", m_Grid.CellDepth);
    x_printfxy(30, 37, "Cell Max Edges  = %d", m_nMaxEdgesPerCell);
    x_printfxy(30, 38, "Grid Cells Wide = %d", m_Grid.nCellsWide);
    x_printfxy(30, 39, "Grid Cells Deep = %d", m_Grid.nCellsDeep);
    x_printfxy(30, 40, "Grid-Edge list size: %d", m_Grid.nGridEdges);

    // Draw marked points.
    s32 i;
    for (i = 0; i < m_nMarks; i++ )
    {
        draw_Label(m_Mark[i].Pos, xcolor(255,255,0), "%d", m_Mark[i].Intensity);
        draw_Marker( m_Mark[i].Pos, XCOLOR_RED );
    }

    if (bShowBounds)
    {
        // Render the map borders.
        bbox WorldBounds = GameMgr.GetBounds();
        draw_BBox(WorldBounds, XCOLOR_WHITE);
        vector3 vMin = WorldBounds.Min;
        vector3 vMax = WorldBounds.Max;

        vector3 inc(10.0f, 0, 0);

        f32 fi;
        draw_Begin( DRAW_LINES );
        draw_Color( XCOLOR_WHITE );
        for (fi = vMin.X; fi < vMax.X; fi+= 10)
        {
            draw_Vertex( vector3(fi, vMin.Y, vMin.Z));
            draw_Vertex( vector3(fi, vMax.Y, vMin.Z ));
        }
        for (fi = vMin.X; fi < vMax.X; fi+= 10)
        {
            draw_Vertex( vector3(fi, vMin.Y, vMax.Z));
            draw_Vertex( vector3(fi, vMax.Y, vMax.Z ));
        }
        for (fi = vMin.Z; fi < vMax.Z; fi+= 10)
        {
            draw_Vertex( vector3(vMin.X, vMin.Y, fi));
            draw_Vertex( vector3(vMin.X, vMax.Y, fi));
        }
        for (fi = vMin.Z; fi < vMax.Z; fi+= 10)
        {
            draw_Vertex( vector3(vMax.X, vMin.Y, fi));
            draw_Vertex( vector3(vMax.X, vMax.Y, fi ));
        }
        
        draw_End();
    }
    //
    // Render the nodes
    //
    {
        const vector3 BBoxSize( 0.5f,0.5f,0.5f );
        vector3 BBoxMin, BBoxMax;

        s32 BadNodes = 0;
        s32 Count = m_NodeList.GetCount();
        for(i=0; i<Count; i++ )
        {
            const node& Node = m_NodeList[i];
            BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
            BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);
            draw_BBox( bbox( BBoxMin, BBoxMax) );

            if (g_DrawEdgeLabels)
                draw_Label(Node.Pos + BBoxSize*2, XCOLOR_YELLOW, "%d", i);

            if (Node.Type == NDTYPE_QUESTIONABLE)
            {
                draw_Label(Node.Pos, xcolor(255,128,0), "???");
                draw_Marker( Node.Pos, XCOLOR_YELLOW );
                BadNodes++;
            }

            if (Node.Type == NDTYPE_UNCONNECTED)
            {
                draw_Label(Node.Pos, xcolor(255,128,0), "###");
                draw_Marker( Node.Pos, xcolor(255,128,0));
            }

            if (Node.nEdges  <= 1)
            {
                draw_Point(Node.Pos, XCOLOR_RED);
            }

        }
        x_printfxy(5, 15, "Questionable nodes: %d", BadNodes);

        //
        // Render the "spawn node"
        //
        if( m_hSelectedSpawn >= 0 )
        {
            const node& Node = m_NodeList(m_hSelectedSpawn);
            BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
            BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);
            draw_BBox( bbox( BBoxMin, BBoxMax), xcolor(0,255,0,255) );
        }

        //
        // Render the "active node"
        //
        if( m_hSelectedNode >= 0 )
        {
            const node& Node = m_NodeList(m_hSelectedNode);
            BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
            BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);
            draw_BBox( bbox( BBoxMin, BBoxMax), xcolor(255,0,0,255) );
        }
    }

    if (LargestCell)
    {
        draw_BBox(LargestCell->CellBox, XCOLOR_YELLOW);
    }

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
        s32 Count = m_EdgeList.GetCount();
        for( i=0; i<Count; i++ )
        {
            const edge& Edge     = m_EdgeList[i];
            const node& NodeFrom = m_NodeList(Edge.hNodeA);
            const node& NodeTo   = m_NodeList(Edge.hNodeB);
            xcolor      Color;
            
            switch( Edge.Type )
            {
            case EDGE_TYPE_GROWN:   Color.Set(0,255,0,255); break; 
            case EDGE_TYPE_AIR:     Color.Set(0,0,255,255); break; 
            case EDGE_TYPE_SKI:     Color.Set(255,255,0,255); break; 
            }

            draw_Color( Color );
            if( Edge.bLoose == TRUE )
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
            if (g_DrawEdgeLabels)
            draw_Label(NodeFrom.Pos + (NodeTo.Pos - NodeFrom.Pos)/2,
                (i==m_iFirstMinNode?XCOLOR_RED:XCOLOR_WHITE), "%d", i);
            
#if DEBUG_COST_ANALYSIS
        if (g_bCostAnalysisOn)
        {
            x_printfxy(0, 15 + i, "Edge Index %d: Cost A-B: %2.2f, nA2B = %d     Cost B-A: %2.2f, nB2A = %d",
            i,
            m_EdgeList[i].CostFromA2B, m_EdgeList[i].nA2B,
            m_EdgeList[i].CostFromB2A, m_EdgeList[i].nB2A);
        }
#endif
        }
        draw_End();

        if( m_hSelectedEdge >= 0 )
        {
            const edge& Edge     = m_EdgeList(m_hSelectedEdge);
            const node& NodeFrom = m_NodeList(Edge.hNodeA);
            const node& NodeTo   = m_NodeList(Edge.hNodeB);

            draw_Line( NodeFrom.Pos, NodeTo.Pos, xcolor(255,0,0,255) );
        }
    
    }

    // Render Bot numbers for cost analysis
    if (m_pBots[0])
    for (i = 0; i < NUM_BOTS; i++)
    {
        draw_Label(((bot_object*)m_pBots[i])->GetPosition(),
            XCOLOR_WHITE, "%d", i);
    }
#if SHOW_EDGE_COSTS
    g_Graph.ShowEdgeCosts();
#endif
    eng_End();

    //x_printfxy( 0, 6, "BotEditMode = %d ", m_NodeList.GetCount() );

    x_printfxy( 0, 7, "nNodes = %d ", m_NodeList.GetCount() );
    x_printfxy( 0, 8, "nEdges = %d ", m_EdgeList.GetCount() );

    if (g_DebugTargetLoc)
    {
        x_printfxy( 0, 10, "Target = (%2.2f, %2.2f)", g_DebugTargetLoc->X, g_DebugTargetLoc->Z);
    }
}

//=========================================================================

void path_editor::SelectEdge( const vector3& Pos )
{
    FindClosestEdge( Pos, m_hSelectedEdge );
}

//=========================================================================

void path_editor::UnSelectActiveNode( void )
{
    m_hSelectedNode = -1;
    m_hSelectedEdge = -1;
}

//=========================================================================

void path_editor::MoveSelectedNode( const vector3& Pos )
{
    if( m_hSelectedNode != -1 )
    {
        node& Node = m_NodeList( m_hSelectedNode );
        Node.Pos = Pos;

        for( s32 i=0; i<Node.nEdges; i++ )
        {
            if( m_EdgeList( Node.hEdgeList[i]).Type != EDGE_TYPE_AIR )
            {
                DropNode(); 
                break;
            }
        }
        if (ScanNodeForCollision(Node))
        {
            LowerNode();
        }
        m_bChanged = TRUE;
    }
}

//=========================================================================

void path_editor::InsertNodeInActiveEdge( const vector3& Pos )
{
    if( m_hSelectedEdge == -1 ) return;

    //
    // Add New Node
    //
    m_hSelectedNode = -1;
    AddNode( Pos );

    //
    // Add New Edges
    //
    xhandle     hEdge;
    edge        Edge    = m_EdgeList( m_hSelectedEdge );
    edge        NewEdge;

    // Add edge 1
    NewEdge        = Edge;
    NewEdge.hNodeA = m_hSelectedNode;
    hEdge          = AddEdge( NewEdge );

    // Add edge to node 1
    AddEdgeToNode( m_NodeList( m_hSelectedNode ), hEdge );

    // Add edge 2
    NewEdge        = Edge;
    NewEdge.hNodeB = m_hSelectedNode;
    hEdge          = AddEdge( NewEdge );

    // Add edge to node 2
    AddEdgeToNode( m_NodeList( m_hSelectedNode ), hEdge );

    //
    // Delete old edge
    //
    DeleteActiveEdge();
    m_bChanged = TRUE;
}

//=========================================================================

void path_editor::Save( const char* pFileName )
{
    matrix4 M;
    M.Identity();
    Save( pFileName, M );
}

//=========================================================================

xbool path_editor::Load( const char* pFileName )
{
#if LOADINIT
    LoadTest();
    return;
#endif

    InitClass();

    matrix4 M;
    M.Identity();

    m_bChanged = TRUE;

    return Load( pFileName, M );
}

//=========================================================================

xbool path_editor::LoadTemplate( const char* pFileName, const matrix4& L2W )
{
    return Load( pFileName, L2W );
}

//=========================================================================

xbool path_editor::SaveTemplate( const char* pFileName, const matrix4& W2L )
{
    if (Save( pFileName, W2L ))
    {
        // Clear out the graph.
        InitClass();
        return TRUE;
    }
    else
        return FALSE;
}

//=========================================================================

xbool path_editor::Save( const char* pFileName, const matrix4& W2L )
{
    X_FILE* Fp;
    s32     i;

    Fp = x_fopen( pFileName, "wt" );

    if( !Fp )
    {
        MessageBox(NULL, "Save FAILED!  (File write-protected?)", "Error", MB_OK);
        return FALSE;
    }

    //
    // Save the version number
    //
    x_fprintf( Fp, "[ PathEditor ]\n" );
    x_fprintf( Fp, "{ Version Selected BotStart }\n" );
    x_fprintf( Fp, "%d ", VERSION_NUMBER );
    x_fprintf( Fp, "%d ", m_hSelectedNode==-1?-1:m_NodeList.GetIndexFromHandle(m_hSelectedNode) );
    x_fprintf( Fp, "%d ", m_hSelectedSpawn==-1?-1:m_NodeList.GetIndexFromHandle(m_hSelectedSpawn) );
    x_fprintf( Fp, "\n");

    //
    // Save the node cores
    //
    x_fprintf( Fp, "\n[ NodeCore : %d ]\n", m_NodeList.GetCount() );
    x_fprintf( Fp, "{ ID Position Type nEdges }\n" );

    xstring File(pFileName);
    xbool SavingToCurrentMap = (File.Find(tgl.MissionName) != -1);

    s32 NodeRef = 0;
    for( i=0; i<m_NodeList.GetCount(); i++ )
    {
//        if (SavingToCurrentMap)
//        this->RaiseNodeByBBox(m_NodeList[i]);

        const node&   Node = m_NodeList[i];

        vector3 V;

        NodeRef += Node.nEdges;

        x_fprintf( Fp, "%d ", i );
        V = W2L * Node.Pos;
        x_fprintf( Fp, "%f %f %f ", V.X, V.Y, V.Z );
        x_fprintf( Fp, "%d ", Node.Type );
        x_fprintf( Fp, "%d ", Node.nEdges );
        x_fprintf( Fp, "\n");
    }

    //
    // Save node references
    //
    x_fprintf( Fp, "\n[ NodeEdgeRef : %d ]\n", NodeRef );
    x_fprintf( Fp, "{ NodeID EdgeID }\n" );

    for( i=0; i<m_NodeList.GetCount(); i++ )
    {
        const node& Node = m_NodeList[i];

        for( s32 k=0; k<Node.nEdges; k++ )
        {
            x_fprintf( Fp, "%d %d", i, m_EdgeList.GetIndexFromHandle( Node.hEdgeList[k] ));
            x_fprintf( Fp, "\n");
        }
    }

    //
    // Save Edges 
    //
    x_fprintf( Fp, "\n[ Edges : %d ]\n", m_EdgeList.GetCount() );
    x_fprintf( Fp, "{ EdgeID Type bLoose RefCount NodeA NodeB CostFromA2B nA2B CostFromB2A nB2A }\n" );

    for( i=0; i<m_EdgeList.GetCount(); i++ )
    {
        edge& Edge = m_EdgeList[i];

        x_fprintf( Fp, "%d ", i );
        x_fprintf( Fp, "%d ", Edge.Type  );
        x_fprintf( Fp, "%d ", Edge.bLoose   );
        x_fprintf( Fp, "%d ", Edge.RefCount );
        x_fprintf( Fp, "%d ", m_NodeList.GetIndexFromHandle( Edge.hNodeA ) );
        x_fprintf( Fp, "%d ", m_NodeList.GetIndexFromHandle( Edge.hNodeB ) );
        x_fprintf( Fp, "%f ", Edge.CostFromA2B );
        x_fprintf( Fp, "%d ", Edge.nA2B   );
        x_fprintf( Fp, "%f ", Edge.CostFromB2A );
        x_fprintf( Fp, "%d ", Edge.nB2A );
        x_fprintf( Fp, "\n" );
    }

    x_fclose( Fp );   
    return TRUE;
}

//=========================================================================

struct field
{
    char Name[256];
};
static
xbool ReadStructure( FILE* Fp, field* pField, s32& nFields )
{
    char Buff[256];

    // Read the begingin of the structure
    fscanf( Fp, "%s", Buff );
    if( x_strcmp( Buff, "{" ) != 0 ) return FALSE;

    // Read the structure
    nFields = 0;
    while(1)
    {
        fscanf( Fp, "%s", pField[nFields].Name );

        if( x_strcmp( pField[nFields].Name, "}" ) == 0 ) break;

        nFields++;
        if( nFields == 32 ) return FALSE;
    }

    return TRUE;
}

//=========================================================================
static
s32 ReadHeader( FILE* Fp, char* pHeader )
{
    char    pBuff[256];
    s32     c       = 0;
    char*   pNum    = NULL;

    while( feof(Fp) == FALSE )
    {
        pBuff[c] = fgetc( Fp );
        if( c )
        {
            c++;
            if( c == 255 ) 
            {
                break;
            }
            if( pBuff[c-1] == ']' ) 
            {
                break;
            }
            if( pBuff[c-1] == ':' ) 
            {
                pNum = &pBuff[c];
            }
        }
        else
        {
            if( pBuff[c] == '[' ) c++;
        }
    }

    pBuff[c] = 0;

    if( x_strstr( pBuff, pHeader ) == NULL ) return -1;

    return pNum == NULL ? 0 : atoi( pNum );
}

//=========================================================================

xbool path_editor::Load( const char* pFileName, const matrix4& L2W )
{
    FILE*       Fp;
    s32         iBuff;
    field       Field[32];
    s32         nFields;
    s32         i;
    xhandle*    NodeIndexToHash = NULL;
    xhandle*    EdgeIndexToHash = NULL;
    s32         NodeOffset      = m_NodeList.GetCount();
    s32         nNodes;
    s32         nEdges;

    Fp = fopen( pFileName, "rt" );
    if( Fp == NULL ) return FALSE;

    //
    // Load the version number
    //
    
    if( ReadHeader( Fp, "PathEditor" ) == -1 ) return FALSE;

    // Read the structure
    if( ReadStructure( Fp, Field, nFields ) == FALSE ) return FALSE;

    {
        for( s32 k=0; k<nFields; k++ )
        {
            if( 0 )
            {
            }
            else if( x_strcmp( Field[k].Name, "Version" ) == 0 )
            {
                fscanf( Fp, "%d", &iBuff );
                if( iBuff != VERSION_NUMBER ) return FALSE;
            }
            else if( x_strcmp( Field[k].Name, "Selected" ) == 0 )
            {
                fscanf( Fp, "%d", &m_hSelectedNode );
            }
            else if( x_strcmp( Field[k].Name, "BotStart" ) == 0 )
            {
                fscanf( Fp, "%d", &m_hSelectedSpawn );
            }
            else
            {
                ASSERT( 0 );
            }
        }
    }

    //
    // Load the node cores
    //
    nNodes = ReadHeader( Fp, "NodeCore" ); 
    if( nNodes < 0 ) return FALSE;

    // Read the structure
    if( ReadStructure( Fp, Field, nFields ) == FALSE ) return FALSE;

    // Allocate the index to hash table
    NodeIndexToHash = new xhandle[ nNodes ];
    ASSERT( NodeIndexToHash );

    // Read the data
    for( i=0; i<nNodes; i++ )
    {
        node& Node = m_NodeList.AddNode( NodeIndexToHash[i] );
        x_memset( &Node, 0, sizeof(Node) );

        for( s32 k=0; k<nFields; k++ )
        {
            if( 0 )
            {
            }
            else if( x_strcmp( Field[k].Name, "ID" ) == 0 )
            {
                s32 D;
                fscanf( Fp, "%d", &D );
                ASSERT( D == i );
            }
            else if( x_strcmp( Field[k].Name, "Position" ) == 0 )
            {
                vector3 V;
                fscanf( Fp, "%f%f%f", &V.X, &V.Y, &V.Z );

                Node.Pos = L2W * V;
            }
            else if( x_strcmp( Field[k].Name, "Type" ) == 0 )
            {
                fscanf( Fp, "%d", &Node.Type );
            }
            else if( x_strcmp( Field[k].Name, "nEdges" ) == 0 )
            {
                fscanf( Fp, "%d", &Node.nEdges );
                Node.nEdges = 0;    // Must be set to null
            }
            else
            {
                ASSERT( 0 );
            }
        }        
    }

    //
    // Load the Edge References
    //
    iBuff = ReadHeader( Fp, "NodeEdgeRef" ); 
    if( iBuff < 0 ) return FALSE;

    // Read the structure
    if( ReadStructure( Fp, Field, nFields ) == FALSE ) return FALSE;

    for( i=0; i<iBuff; i++ )
    {
        s32 ID = -1;

        for( s32 k=0; k<nFields; k++ )
        {
            if( 0 )
            {
            }
            else if( x_strcmp( Field[k].Name, "NodeID" ) == 0 )
            {
                fscanf( Fp, "%d", &ID );
            }
            else if( x_strcmp( Field[k].Name, "EdgeID" ) == 0 )
            {
                node& Node = m_NodeList[ID + NodeOffset ];
                fscanf( Fp, "%d", &Node.hEdgeList[ Node.nEdges++ ] );
            }
            else
            {
                ASSERT( 0 );
            }
        }        
    }

    //
    // Load the Edge References
    //
    nEdges = ReadHeader( Fp, "Edges" ); 
    if( nEdges < 0 ) return FALSE;

    // Read the structure
    if( ReadStructure( Fp, Field, nFields ) == FALSE ) return FALSE;

    // Allocate the index to hash table
    EdgeIndexToHash = new xhandle[ nEdges ];
    ASSERT( EdgeIndexToHash );

    for( i=0; i<nEdges; i++ )
    {
        edge& Edge = m_EdgeList.AddNode( EdgeIndexToHash[i] );
        x_memset( &Edge, 0, sizeof(Edge) );

        for( s32 k=0; k<nFields; k++ )
        {
            if( 0 )
            {
            }
            else if( x_strcmp( Field[k].Name, "EdgeID" ) == 0 )
            {
                s32 ID;
                fscanf( Fp, "%d", &ID );
                ASSERT( ID == i );
            }
            else if( (x_strcmp( Field[k].Name, "bAerial" ) == 0) || 
                     (x_strcmp( Field[k].Name, "Type"    ) == 0) )
            {
                fscanf( Fp, "%d", &Edge.Type );
            }
            else if( x_strcmp( Field[k].Name, "bLoose" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.bLoose );
            }
            else if( x_strcmp( Field[k].Name, "RefCount" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.RefCount );
            }
            else if( x_strcmp( Field[k].Name, "NodeA" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.hNodeA );
                Edge.hNodeA = NodeIndexToHash[ Edge.hNodeA.Handle ];
                if (Edge.hNodeA < 0 || Edge.hNodeA > nEdges * 2)
                    Edge.hNodeA = 0;
            }
            else if( x_strcmp( Field[k].Name, "NodeB" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.hNodeB );
                Edge.hNodeB = NodeIndexToHash[ Edge.hNodeB.Handle ];
                if (Edge.hNodeB < 0 || Edge.hNodeB > nEdges * 2)
                    Edge.hNodeB = 0;
            }
            else if( x_strcmp( Field[k].Name, "CostFromA2B" ) == 0 )
            {
                fscanf( Fp, "%f", &Edge.CostFromA2B );
            }
            else if( x_strcmp( Field[k].Name, "CostFromB2A" ) == 0 )
            {
                fscanf( Fp, "%f", &Edge.CostFromB2A );
            }
            else if( x_strcmp( Field[k].Name, "nA2B" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.nA2B );
            }
            else if( x_strcmp( Field[k].Name, "nB2A" ) == 0 )
            {
                fscanf( Fp, "%d", &Edge.nB2A );
            }
            else
            {
                ASSERT( 0 );
            }
        }        
    }

    fclose( Fp );

    m_bChanged = FALSE;
    
    xstring File(pFileName);
    xbool SavingToCurrentMap = (File.Find(tgl.MissionName) != -1);

    //
    // Convert all the virtual handles to actual handles
    //
    for( i=0;i<nNodes;i++)
    {
        node& Node = m_NodeList[ NodeOffset + i ];
        for( s32 k=0;k<Node.nEdges; k++ )
        {
            Node.hEdgeList[k] = EdgeIndexToHash[ Node.hEdgeList[k].Handle ];
        }
//        if (SavingToCurrentMap)
//            RaiseNodeByBBox(m_NodeList[i]); // Requires Map to be loaded.
    }

    m_hSelectedNode.Handle  = ( m_hSelectedNode.Handle  == -1) ? -1 : NodeIndexToHash[ m_hSelectedNode.Handle  ].Handle;
    m_hSelectedSpawn.Handle = ( m_hSelectedSpawn.Handle == -1) ? -1 : NodeIndexToHash[ m_hSelectedSpawn.Handle ].Handle;

    delete []EdgeIndexToHash;
    delete []NodeIndexToHash;

    return TRUE;
}

//=========================================================================

void path_editor::SelectSpawnNode( const vector3& Pos )
{
    FindClosestNode( Pos, m_hSelectedSpawn );
}

//=========================================================================


void path_editor::ComputePathToActiveNode( xarray<u16>& NodeArray )
{
    //
    // Validate the inputs
    //
    if( m_hSelectedSpawn == -1 ) return;
    if( m_hSelectedNode  == -1 ) return;

    NodeArray.Clear();

    g_Graph.Init();

    //
    // Go
    //
    xhandle         hActNode = m_hSelectedSpawn;
    vector3         DestPos  = m_NodeList( m_hSelectedNode ).Pos;
    xarray<xhandle> VisitedNodes;
    xarray<node>    Array;
    s32             TotalEdgeNodes = 0;

    // Add the very first node
    TotalEdgeNodes += m_NodeList( hActNode ).nEdges;
    Array.Append( m_NodeList( hActNode ) );
    VisitedNodes.Append( hActNode );

    while( 1 )
    {
        xhandle     hBest;
        node&       Node    = m_NodeList( hActNode );
        f32         MinDist = 99999999.0f;
                    hBest   = -1;
    
        for( s32 i=0; i<Node.nEdges; i++ )
        {
            edge&   Edge  = m_EdgeList( Node.hEdgeList[i] );
            xhandle hNext = ( Edge.hNodeA == hActNode ) ? Edge.hNodeB : Edge.hNodeA;
            vector3 Dir   = DestPos - m_NodeList( hNext ).Pos;
            f32     d     = Dir.Dot( Dir );

            if( d < MinDist )
            {
                // Make sure we don't back track
                s32 Count = VisitedNodes.GetCount();
                for( s32 j=0; j<Count; j++ )
                {
                    if( VisitedNodes[j] == hNext ) break;
                }

                // We have a new best one
                if( j == Count )
                {
                    MinDist = d;
                    hBest   = hNext;
                }
            }
        }
    
        // We are done if we have rich to the end or there are not better choices
        if( hBest == m_hSelectedNode || hBest == -1 ) 
            break;

        // Add the new node into the list
        TotalEdgeNodes += m_NodeList( hBest ).nEdges;
        Array.Append( m_NodeList( hBest ) );

        // Add the new visited node into the list
        VisitedNodes.Append( hBest );

        // Make the new active node be the best one 
        hActNode = hBest;
    }

    // Add the very last node 
    TotalEdgeNodes += m_NodeList( m_hSelectedNode ).nEdges;
    Array.Append( m_NodeList( m_hSelectedNode ) );

    //
    // Okay now that we have the array populated with nodes we must construct the 
    // run time structures for it.
    //
    s32         i;
    s32         iNodeEdgeList = 0;
    s32         nEdges        = 0 ;
    xhandle*    pEdge         = new xhandle[ m_EdgeList.GetCount() ];
    ASSERT( pEdge );

    g_Graph.m_nNodes    = Array.GetCount();
    g_Graph.m_pNodeList = new graph::node[g_Graph.m_nNodes];
    ASSERT( g_Graph.m_pNodeList );

    g_Graph.m_nNodeEdges = TotalEdgeNodes;
    g_Graph.m_pNodeEdges = new s16[g_Graph.m_nNodeEdges];
    ASSERT( g_Graph.m_pNodeEdges );

    //
    // Fill in the graph node and the graph nodeedges
    //
    for( i=0; i<Array.GetCount(); i++)
    {
        node&           Node    = Array[i];
        graph::node&    FNode   = g_Graph.m_pNodeList[i];

        // Add the index of our path
        NodeArray.Append( i );

        // Back up the start of the node personal edge list
        FNode.EdgeIndex = iNodeEdgeList;

        for( s32 j=0; j<Node.nEdges; j++ )
        {
            //
            // Find or add the edge
            //
            for( s32 k=0; k<nEdges; k++ )
            {
                if( pEdge[k] == Node.hEdgeList[j] ) break;
            }

            if( k == nEdges )
            {
                pEdge[k] = Node.hEdgeList[j];
                ASSERT( k<= m_EdgeList.GetCount() );
                nEdges++;
            }

            //
            // Put the edge in the nodeedge list for the node
            //
            g_Graph.m_pNodeEdges[ iNodeEdgeList++ ] = k;
            ASSERT( iNodeEdgeList <= g_Graph.m_nNodeEdges );
        }

        // Fill the rest of the node info
        FNode.nEdges = iNodeEdgeList - FNode.EdgeIndex;
        FNode.Pos    = Node.Pos;
    }
    
    //
    // Fill in the graph edges
    //
    g_Graph.m_nEdges    = nEdges;
    g_Graph.m_pEdgeList = new graph::edge[g_Graph.m_nEdges];
    ASSERT( g_Graph.m_pEdgeList );

    for( i=0; i<nEdges; i++)
    {
        edge&   Edge = m_EdgeList( pEdge[i] );

        g_Graph.m_pEdgeList[i].NodeA  = m_NodeList.GetIndexFromHandle( Edge.hNodeA );
        g_Graph.m_pEdgeList[i].NodeB  = m_NodeList.GetIndexFromHandle( Edge.hNodeB );

        g_Graph.m_pEdgeList[i].Type   = Edge.Type;

        g_Graph.m_pEdgeList[i].Flags  = 0;
        g_Graph.m_pEdgeList[i].Flags |= Edge.bLoose  ? graph::EDGE_FLAGS_LOOSE  : 0;


    }

    //
    // Free the temp memory
    //
    delete []pEdge;
}

//==============================================================================

u16 EdgeCost(const vector3& PosA, const vector3& PosB, path_editor::edge_type Type, xbool Loose)
{
    f32 Cost = (vector3(PosA - PosB).Length());

    // Calculate slope
    f32 dY = PosB.Y - PosA.Y;
    f32 dXZ = vector2(vector2(PosB.X, PosB.Z) - vector2(PosA.X, PosA.Z)).Length();

    f32 Slope;
    xbool SlopeDefined = FALSE;
    if (dXZ)
    {
        Slope = dY/dXZ;
        SlopeDefined = TRUE;
    }
    f32 Modifier = 1.0;
#if (EDGE_COST_MODIFIERS_ON)
    switch (Type)
    {
        case path_editor::EDGE_TYPE_GROWN:
            Modifier = 1.0f;
        break;

        case path_editor::EDGE_TYPE_AIR:
            Modifier = 0.8f;
        break;

        case path_editor::EDGE_TYPE_SKI:
            if (SlopeDefined )
            {
                if( Slope > 0)
                    // Going up hill.  No better.
                    Modifier = 1.0f;
                else
                    // Whee - ski
                    Modifier = 0.6f;
            }
            else
            {
                if (dY > 0)
                    // Crap, Pure vertical upward.
                    Modifier = 1.1f;
                else if (dY < 0)
                    Modifier = 0.6f;
                else
                {
                    ASSERT( !" ERROR Zero-length Edge! ");
                    return 0;
                }
            }
        break;
    }
    if (Loose)
    {
        Modifier -= 0.1f;
    }
    else
        Modifier += 0.1f;
    
#endif
    Cost *= Modifier;

    return (u16)Cost;
}

//==============================================================================

void path_editor::Export( const char* pFileName )
{
    // Refresh the g_Graph object (minus the grid).
    Refresh_g_GRAPH();

#if USING_GRID
    s32         i,j;

    // Evaluate the grid values & cell edge list (stored in the path editor).
    RecomputeGrid();
    
    // Store the grid into g_Graph.
    for (i = 0; i < m_Grid.nCellsWide; i++)
    {
        for (j = 0; j < m_Grid.nCellsDeep; j++)
        {
            g_Graph.m_Grid.Cell[i][j].EdgeOffset = m_Grid.Cell[i][j].EdgeOffset;
            g_Graph.m_Grid.Cell[i][j].nEdges     = m_Grid.Cell[i][j].nEdges;
        }
    }
    g_Graph.m_Grid.CellDepth    = m_Grid.CellDepth;
    g_Graph.m_Grid.CellWidth    = m_Grid.CellWidth;
    g_Graph.m_Grid.Min          = m_Grid.Min;
    g_Graph.m_Grid.Max          = m_Grid.Max;
    g_Graph.m_Grid.nGridEdges   = m_Grid.nGridEdges;
    x_memset(g_Graph.m_Grid.GridEdgesList, -1, sizeof(g_Graph.m_Grid.GridEdgesList));
    s32 Count = m_Grid.nGridEdges;

    // Write g_Graph's grid's CellEdges list.
    for (i = 0; i < Count; i++)
    {
        g_Graph.m_Grid.GridEdgesList[i] = m_Grid.GridEdgesList[i];
    }

    ASSERT(g_Graph.m_Grid.nGridEdges == m_Grid.GridEdgesList.GetCount());

    // Verify each asset has a node within its radius.
    TestAssetsForEdges();

#endif

    //
    // Save it to disk
    //
    g_Graph.Save( pFileName );

    if (x_strcmp(pFileName, TEMP_FILE) != 0)
    {
        g_Graph.Save( TEMP_FILE);
    }
    m_bChanged = FALSE;

}

//==============================================================================

void path_editor::Import( const char* pFileName )
{
    // Clear out the contents of the current path-editor graph in memory.
    InitClass();

    X_FILE* Fp;
    s32     i;

    Fp = x_fopen( pFileName, "wt" );
    ASSERT( Fp );

    //
    // Save the version number
    //
    x_fprintf( Fp, "[ PathEditor ]\n" );
    x_fprintf( Fp, "{ Version Selected BotStart }\n" );
    x_fprintf( Fp, "%d ", VERSION_NUMBER );
    x_fprintf( Fp, "%d ", -1);
    x_fprintf( Fp, "%d ", -1);
    x_fprintf( Fp, "\n");

    //
    // Save the node cores
    //
    x_fprintf( Fp, "\n[ NodeCore : %d ]\n", g_Graph.m_nNodes);
    x_fprintf( Fp, "{ ID Position Type nEdges }\n" );

    for( i=0; i<g_Graph.m_nNodes; i++ )
    {
        const graph::node&   Node = g_Graph.m_pNodeList[i];
        vector3 V;

        x_fprintf( Fp, "%d ", i );
        V = Node.Pos;
        x_fprintf( Fp, "%f %f %f ", V.X, V.Y, V.Z );
        x_fprintf( Fp, "%d ", NDTYPE_NULL );
        x_fprintf( Fp, "%d ", Node.nEdges );
        x_fprintf( Fp, "\n");
    }

    //
    // Save node references
    //
    x_fprintf( Fp, "\n[ NodeEdgeRef : %d ]\n", g_Graph.m_nNodeEdges );
    x_fprintf( Fp, "{ NodeID EdgeID }\n" );

    for( i=0; i<g_Graph.m_nNodes; i++ )
    {
        const graph::node& Node = g_Graph.m_pNodeList[i];

        for( s32 k=Node.EdgeIndex; k<Node.EdgeIndex+Node.nEdges; k++ )
        {
            x_fprintf( Fp, "%d %d", i, g_Graph.m_pNodeEdges[k]);
            x_fprintf( Fp, "\n");
        }
    }

    //
    // Save Edges 
    //
    x_fprintf( Fp, "\n[ Edges : %d ]\n", g_Graph.m_nEdges);
    x_fprintf( Fp, "{ EdgeID Type bLoose RefCount NodeA NodeB CostFromA2B nA2B CostFromB2A nB2A }\n" );

    for( i=0; i<g_Graph.m_nEdges; i++ )
    {
        graph::edge& Edge = g_Graph.m_pEdgeList[i];

        x_fprintf( Fp, "%d ", i );
        x_fprintf( Fp, "%d ", Edge.Type  );
        x_fprintf( Fp, "%d ", Edge.Flags & graph::EDGE_FLAGS_LOOSE?1:0);
        x_fprintf( Fp, "%d ", 2 );
        x_fprintf( Fp, "%d ", Edge.NodeA);
        x_fprintf( Fp, "%d ", Edge.NodeB);
        x_fprintf( Fp, "%f ", Edge.CostFromA2B );
        x_fprintf( Fp, "%d ", 0   );
        x_fprintf( Fp, "%f ", Edge.CostFromB2A );
        x_fprintf( Fp, "%d ", 0 );
        x_fprintf( Fp, "\n" );
    }

    x_fclose( Fp );   

    this->Load(    pFileName);        
}

//==============================================================================

void path_editor::DropNode( node* pNode )
{
    node *Node;
    if (pNode)
        Node = pNode;
    else if( m_hSelectedNode != -1 )
    {
        Node = &m_NodeList(m_hSelectedNode);
    }
    else
        return;

    // Fire ray straight down
    vector3 Dir(0.0f, -1000.0f, 0.0f);

    vector3 Pos = Node->Pos;
    vector3 Pos2;
    collider  Ray;
    Ray.RaySetup( NULL, Pos, Pos+Dir );
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
    if (!Ray.HasCollided())
    {
        vector3 BackUp(0.0f, 10.0f, 0.0f);
        do
        {
            Pos += BackUp;
            Ray.RaySetup( NULL, Pos, Pos+Dir );
            ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
            if (Pos.Y > 1000.0f)
            {
                ASSERT(!"Can't Find Ground Level!");
            }
        }
        while (!Ray.HasCollided());
    }   
    Pos2 = Pos + Ray.GetCollisionT()*Dir;

    Node->Pos.Y = Pos2.Y;
    m_bChanged = TRUE;

    DropNode2(Node);
}

//==============================================================================

xbool path_editor::LowerNode( node* pNode )
{
    node *Node;
    if (pNode)
        Node = pNode;
    else if( m_hSelectedNode != -1 )
    {
        Node = &m_NodeList(m_hSelectedNode);
    }
    else
        return TRUE;

    // First check if this node is already on the ground.
    
    // Raise position of node a bit
    vector3 Pos = Node->Pos + vector3(0, 1.0f, 0);

    // Fire ray straight down
    vector3 Dir(0.0f, -1.5f, 0.0f);

    collider  Ray;
    collider::collision Col;
    Ray.RaySetup( NULL, Pos, Pos+Dir );
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
    if (Ray.HasCollided() )
    {
        // Looks like this edge was on the ground to begin with.
        return TRUE;
    }

    // Lower the bbox up by up to 4 meters, or until it hits something.
    vector3 BBoxMin, BBoxMax;
    // actual size of largest player's bbox (bioderm heavy)
    BBoxMin = vector3(Node->Pos.X - 0.95f, Node->Pos.Y, Node->Pos.Z - 0.95f);
    BBoxMax = vector3(Node->Pos.X + 0.95f, Node->Pos.Y + 2.6f, Node->Pos.Z + 0.95f);
    bbox BBox(BBoxMin, BBoxMax);
    collider Ext, UpExt;
    collider::collision Collision;

    vector3 MoveUp(0.0f, 1.0f, 0.0f);
    vector3 MoveDown(0.0f, -1.0f, 0.0f);

    f32 CurrentY = Node->Pos.Y;
    s32 TotalDown = 1;

    xbool HaventHitAnythingYet = TRUE;
    do
    {
        Ext.ExtSetup(NULL, BBox, MoveDown * (f32)TotalDown); 
        TotalDown++;
        ObjMgr.Collide( object::ATTR_SOLID_STATIC | object::ATTR_PERMANENT, Ext );

        // Extrude Downwards and see if we hit anything or went low enough..
        if ( Ext.HasCollided() || TotalDown >= 3.0f)
        {
            HaventHitAnythingYet = FALSE;
            if (Ext.HasCollided() )
            {
                // Get the point of collision.
                Ext.GetCollision( Collision );
                CurrentY = (Collision.Point.Y + 0.1f);
            }
            else
            {
                CurrentY = Node->Pos.Y - (f32)TotalDown;
            }

            // Assume the position.
            BBoxMin.Y = CurrentY;
            BBoxMax.Y = CurrentY + 2.6f;

            BBox.Set(BBoxMin, BBoxMax);

            // Extrude Upwards.
            UpExt.ExtSetup(NULL, BBox, MoveUp * (TotalDown + 1.0f));
            ObjMgr.Collide( object::ATTR_SOLID_STATIC | object::ATTR_PERMANENT, UpExt );
            if ( UpExt.HasCollided() )
            {
                UpExt.GetCollision(Collision);
                CurrentY = Collision.Point.Y - 2.6f;
            }
        }
    }
    while ( HaventHitAnythingYet );

    // Make sure we got a new Y value that's reasonable.
    if( Node->Pos.Y - 3.0 > CurrentY  || Node->Pos.Y + 4.0 < CurrentY)
    {
        // Log this node as a potential bad Node->
        Node->Type = NDTYPE_QUESTIONABLE;

        xfs XFS( "Map %s: Questionable Node at (%4.2f,%4.2f,%4.2f)\n",
            tgl.MissionName,
            Node->Pos.X, Node->Pos.Y, Node->Pos.Z
            );
        x_DebugMsg((const char*)XFS);
        bot_log::Log((const char*)XFS);
        return FALSE;
    }
    else
    {
        if (x_abs(Node->Pos.Y - (CurrentY + 0.1f)) > 0.01f)
            bUpdated = TRUE;
        Node->Pos.Y = CurrentY - 0.1f;
        if ( ScanNodeForCollision(*Node) )
            return FALSE;
    }
    return TRUE;

}

//==============================================================================

void path_editor::DropNode2(node* pNode)
{
    node* Node;
    if (pNode)
        Node = pNode;
    else if( m_hSelectedNode != -1 )
    {
        Node = &m_NodeList(m_hSelectedNode);
    }
    else
        return;

    Node = &m_NodeList(m_hSelectedNode);
    this->RaiseNodeByBBox(*Node);    
}

//==============================================================================

void path_editor::UnselectedActiveEdge( void )
{
    m_hSelectedEdge = -1;
}

//==============================================================================

void path_editor::ChangeActiveEdge( void )
{
    m_EdgeList( m_hSelectedEdge ).Type    = m_EdgeType;
    m_EdgeList( m_hSelectedEdge ).bLoose  = m_bLoose;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::ChangeEdgeType( edge_type Type )
{
    m_EdgeType = Type;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::SetAerial()
{
    m_EdgeType = EDGE_TYPE_AIR;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::SetGround()
{
    m_EdgeType = EDGE_TYPE_GROWN;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::SetSki( void )
{
    m_EdgeType = EDGE_TYPE_SKI;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::SetLoose()
{
    m_bLoose = TRUE;
    m_bChanged = TRUE;
}

//==============================================================================

void path_editor::SetTight()
{
    m_bLoose = FALSE;
    m_bChanged = TRUE;
}

//==============================================================================

s32 path_editor::DetermineCost(xhandle hNodeA, xhandle hNodeB)
{
    // To do (fix so it's not simply the distance).
    return (s32)vector3(m_NodeList(hNodeA).Pos - m_NodeList(hNodeB).Pos).Length();
}

//==============================================================================

void path_editor::LoadTest()
{
    const f32 LOWX = 950;
    const f32 HIGHX = 1200;
    const f32 LOWZ = 1050;
    const f32 HIGHZ = 1300;
    const f32 INC = 10;
    const f32 Height = 400;
    f32 x,z;
    for (x = LOWX; x <= HIGHX; x += INC)
    {
        for (z = LOWZ; z <= HIGHZ; z+= INC)
        {
            AddNode(vector3(x, Height, z));
        }
        UnSelectActiveNode();
    }
    for (z = LOWZ; z <= HIGHZ; z += INC)
    {
        for (x = LOWX; x <= HIGHX; x+= INC)
        {
            AddNode(vector3(x, Height, z));
        }
        UnSelectActiveNode();
    }
    for (x = LOWX, z = LOWZ; x <= HIGHX; x += INC, z+= INC)
    {
        AddNode(vector3(x, Height, z));
    }
    m_hSelectedNode = 0;
    m_hSelectedSpawn = 0;

}

//==============================================================================

void path_editor::GetSelectedPos( vector3& SelectedPos ) const
{
    if ( m_hSelectedNode.Handle == -1 )
    {
        SelectedPos.Zero();
    }
    else
    {
        const node& Node = m_NodeList( m_hSelectedNode );
        SelectedPos = Node.Pos;
    }
}


//==============================================================================

void path_editor::GetPlayerPos( vector3& PlayerPos ) const
{
    ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
    object* pObject;
    while( (pObject = ObjMgr.GetNextInType()) )
    {
        // make sure this isn't a bot
        if ( pObject->GetType() == object::TYPE_PLAYER )
        {
            break;
        }
        else
        { 
            pObject = NULL;
        }
    }
    ObjMgr.EndTypeLoop();

    if ( pObject )
    {
        PlayerPos = pObject->GetPosition();
    }
    else
    {
        PlayerPos.Zero();
    }
}


//==============================================================================

void path_editor::GetSelectedSpawnPos( vector3& SelectedSpawnPos ) const
{
    if ( m_hSelectedSpawn.Handle == -1 )
    {
        SelectedSpawnPos.Zero();
    }
    else
    {
        const node& Node = m_NodeList( m_hSelectedSpawn );
        SelectedSpawnPos = Node.Pos;
    }
}

//==============================================================================

void path_editor::CostAnalysis(xhandle hNodeA, xhandle hNodeB)
{
    s32 i = 0;

    if (!m_bBotInit)
    {
        InitializeBots();
    }

    for (i = 0; i < NUM_BOTS; i++)
    {
        vector3 Src, Dst;

        GetSelectedSpawnPos( Src );
        GetSelectedPos( Dst );
        ((bot_object *)m_pBots[i])->RunEditorPath( Src, Dst );
    }
}

//==============================================================================

void path_editor::CostAnalysis(walk_type Type)
{
    if (!m_bBotInit)   
        InitializeBots();
    ASSERT (m_bBotInit == TRUE);

    g_bCostAnalysisOn = TRUE;
    m_CurrWalkType = Type;

    if (m_bChanged)
    {
        Path_Manager_ForceLoad();
        Export("temp.gph" );

        s32 i = 0;
        for (i = 0; i < NUM_BOTS; i++)
        {
            ((bot_object *)m_pBots[i])->LoadGraph("temp.gph");
            PickNewPath(i);
        }
    }
}

//==============================================================================
static s32 g_hWnd = NULL;

void path_editor::PickNewPath(s32 nBot)
{
    s32 iStart, iEnd, OldMin = m_iFirstMinNode;
    vector3 vStart, vEnd;

    static s32 LastEnd[NUM_BOTS];
    static xbool FirstTime[16] = 
    {
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, 
    };

    if (FirstTime[nBot])
    {
        do
        {
            iStart   = x_rand()%(m_NodeList.GetCount());
            iEnd   = x_rand()%(m_NodeList.GetCount());
        } while (iStart == iEnd);
        FirstTime[nBot] = FALSE;
        m_pFirstMinNode = &m_EdgeList[0];
    }
    else
    {
        iStart = LastEnd[nBot];
        if (m_CurrWalkType == WKTYPE_LOWEST_NODE)
        {

            s32 Interval  = m_EdgeList.GetCount()/NUM_BOTS;
            s32 End;
        
            if ( nBot == NUM_BOTS-1 ) End = m_EdgeList.GetCount();
            else                      End = nBot * Interval + Interval;
        
            s32 BestEdge = 0;
            s32 BestMin  = 10000000;

            for( s32 i = nBot * Interval ; i < End; i++ )
            {
                if( m_EdgeList[ i ].nA2B < BestMin  ||
                    m_EdgeList[ i ].nB2A < BestMin )
                {
                    BestEdge = i;
                    BestMin  = MIN( m_EdgeList[ BestEdge ].nA2B, m_EdgeList[ BestEdge ].nB2A );

                    if (BestMin < m_pFirstMinNode->nA2B &&
                        BestMin < m_pFirstMinNode->nB2A)
                    {
                        m_pFirstMinNode = &m_EdgeList[BestEdge];
                        m_iFirstMinNode = BestEdge;
                    }
                    if( BestMin <= iMinVis[nBot] )
                        break;
                }
            }
            if (iMinVis[nBot] != BestMin)
            {
    //            x_DebugMsg("**** Bot #%d completed Level %d ****", nBot, iMinVis[nBot]);
                iMinVis[nBot] = BestMin;
            }
            if( iStart == m_NodeList.GetIndexFromHandle(m_EdgeList[ BestEdge ].hNodeA) )
            {
                iEnd = m_NodeList.GetIndexFromHandle(m_EdgeList[ BestEdge ].hNodeB);
            }
            else
            {
                iEnd = m_NodeList.GetIndexFromHandle(m_EdgeList[ BestEdge ].hNodeA);
            }
        }
        else if (m_CurrWalkType == WKTYPE_RAND_NODE)
        {
            do
            {
                iEnd   = x_rand()%(m_NodeList.GetCount());
            } while (iStart == iEnd);
        }

    }
    LastEnd[nBot] = iEnd;

    vStart = m_NodeList[iStart].Pos;
    vEnd = m_NodeList[iEnd].Pos;
    
    ((bot_object *)m_pBots[nBot])->RunEditorPath(vStart, vEnd);

    if (m_iFirstMinNode != OldMin)
    {
        ViewData(g_hWnd);
    }
}

//==============================================================================

void path_editor::ViewData( s32 hWnd )
{
    #ifdef TARGET_PC

    if (!hWnd)
        return;
    g_hWnd = hWnd;
        char Data[200];
    if (m_hSelectedEdge != -1)
    {
        x_sprintf(Data, 
            "Edge Index %d:                      Cost A-B: %2.2f, nA2B = %d     Cost B-A: %2.2f, nB2A = %d",
            m_EdgeList.GetIndexFromHandle(m_hSelectedEdge), 
            m_EdgeList(m_hSelectedEdge).CostFromA2B, m_EdgeList(m_hSelectedEdge).nA2B,
            m_EdgeList(m_hSelectedEdge).CostFromB2A, m_EdgeList(m_hSelectedEdge).nB2A);
        SetWindowText( GetDlgItem( (HWND)hWnd, IDC_DATA ), Data );     
    }
    else
    {
        if (m_pFirstMinNode)
        {
            x_sprintf(Data, 
                "Min Traversals: %d, Edge %d      Cost A-B: %2.2f, nA2B = %d      Cost B-A: %2.2f, nB2A = %d",
                MIN(m_pFirstMinNode->nA2B,m_pFirstMinNode->nB2A) , m_iFirstMinNode,
                m_pFirstMinNode->CostFromA2B, m_pFirstMinNode->nA2B,
                m_pFirstMinNode->CostFromB2A, m_pFirstMinNode->nB2A);
            SetWindowText( GetDlgItem( (HWND)hWnd, IDC_DATA ), Data );     
        }
    }
#endif
}

//==============================================================================

void path_editor::UpdateCost(s32 nBot)
{
#if EDGE_COST_ANALYSIS
    if (!m_bBotInit)   
        InitializeBots();
    ASSERT (m_bBotInit == TRUE);
    ASSERT (nBot < NUM_BOTS);

    // Currently assuming the edge index of the .pth are equivalent to the
    //          edge index of the .gph file.
    u16 Time = 0;
    s32 Edge = -1;
    s32 Start;
    edge* pEdge = NULL;
    ((bot_object *)m_pBots[nBot])->GetCostAnalysisData(Time, Edge, Start);

    // Figure out which Cost we're updating.
    xhandle hStart = m_NodeList.GetHandleFromIndex(Start);

    pEdge = &m_EdgeList( m_GraphToPathHandle[Edge] );

    if (pEdge->hNodeA == hStart)
    {
        pEdge->CostFromA2B = ((pEdge->CostFromA2B * pEdge->nA2B++) + Time) 
                                                        / (pEdge->nA2B + 1);
    x_printf("Edge %d Cost from Node %d to %d UPDATED.\n", Edge, Start, pEdge->hNodeB);
    }
    else
    {
        ASSERT (pEdge->hNodeB == hStart);
        pEdge->CostFromB2A = ((pEdge->CostFromB2A * pEdge->nB2A++) + Time)
                                                        / (pEdge->nB2A + 1);
    x_printf("Edge %d Cost from Node %d to %d UPDATED.\n", Edge, Start, pEdge->hNodeA);
    }


    // Check the bot to see whether or not he needs a new node assignment.
    if (((bot_object *)m_pBots[nBot])->IsIdle())
    {
        PickNewPath(nBot);
    }
#endif
}

//==============================================================================

void path_editor::RenewDestination( void )
{
#if EDGE_COST_ANALYSIS
    if (m_bBotInit)
    // Check the bot to see whether or not he needs a new node assignment.
    for (s32 nBot = 0; nBot < NUM_BOTS; nBot++)
        if (((bot_object *)m_pBots[nBot])->IsIdle())
        {
            PickNewPath(nBot);
        }
#endif
}
//==============================================================================

void path_editor::InitializeBots( void )
{
    bot_object* pBot;

    s32 TotalBots = 0;

    // First collect all existing bots.
    ObjMgr.StartTypeLoop( object::TYPE_BOT );
    while( pBot = (bot_object*)ObjMgr.GetNextInType() )
    {
        ASSERT (TotalBots < NUM_BOTS);
        ASSERT (m_pBots[pBot->GetBotID()] == NULL);

        m_pBots[pBot->GetBotID()] = pBot;
        TotalBots++;
    }
    ObjMgr.EndTypeLoop();

    // If we don't have 16 bots already, let's make 'em.
    for ( ; TotalBots < NUM_BOTS; TotalBots++)
    {
        xwstring Name( xfs( "Autobot #%d", TotalBots+1 ) );

        pBot = (bot_object*)ObjMgr.CreateObject( object::TYPE_BOT );
        pBot->Initialize( vector3(5.0f*TotalBots,100,0),                    // Pos
                                  eng_GetTVRefreshRate(),                   // TV refresh in Hz
                                  player_object::CHARACTER_TYPE_FEMALE,     // Character type 
                                  player_object::SKIN_TYPE_BEAGLE,          // Skin type
                                  player_object::VOICE_TYPE_FEMALE_HEROINE, // Voice type
                                  default_loadouts::STANDARD_PLAYER,        // Default loadout
                                  (const xwchar*)Name,                      // Name
                                  TRUE );                                   // Created from editor

        ObjMgr.AddObject( pBot, obj_mgr::AVAILABLE_SERVER_SLOT );

        ASSERT (m_pBots[pBot->GetBotID()] == NULL);
        m_pBots[pBot->GetBotID()] = pBot;
    }

    m_bBotInit = TRUE;
}

//==============================================================================

void path_editor::RunStressTest( void )
{
    if (!m_bBotInit)
        InitializeBots();
    
    s32 i;
    while (TRUE)
    {
        for (i = 0; i < NUM_BOTS; i++)
        {
            ((bot_object*)m_pBots[i])->RunStressTest();
        }
    }
}

//==============================================================================

xbool InsideBuilding( const vector3& Point )
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
    }
    ObjMgr.EndTypeLoop();

    return bInsideBuilding;
}

//==============================================================================

void path_editor::BlowEdges ( s32 BBoxSize )
{
    m_hSelectedNode = -1;
    m_hSelectedEdge = -1;
    m_hSelectedSpawn= -1;

    s32 BUILDING_KEEP_EDGE_DIST = BBoxSize;
    enum {
        ASSET_KEEP_EDGE_DIST = 5,
        PICKUP_KEEP_EDGE_DIST = 5,
        MAX_EDGES = 3000};
    if (m_EdgeList.GetCount() > MAX_EDGES
        || m_NodeList.GetCount() > MAX_EDGES)
    {
#ifdef TARGET_PC
        MessageBox(NULL, "Unexpected number of nodes/edges > 3000.", "Error", MB_OK);
        return;
#endif
    }
    xbool SavedEdgeHandleList[MAX_EDGES];
    xbool SavedNodeHandleList[MAX_EDGES];

    x_memset(SavedEdgeHandleList, 0, MAX_EDGES*4);
    x_memset(SavedNodeHandleList, 0, MAX_EDGES*4);

    s32 i, ctr = 0;
    building_obj* pBuilding;
    asset*  pAsset;
    pickup* pPickup;
    bbox BBox;
    s32 MaxHandle = 0;

    // Set all nodes within the expanded building bbox to saved.
    ObjMgr.StartTypeLoop( object::TYPE_BUILDING );
    while( pBuilding = (building_obj*)ObjMgr.GetNextInType() )
    {
        BBox = pBuilding->GetBBox();
        // Expand bbox size.
        BBox.Min.X -= BUILDING_KEEP_EDGE_DIST;
        BBox.Min.Y -= BUILDING_KEEP_EDGE_DIST;
        BBox.Min.Z -= BUILDING_KEEP_EDGE_DIST;

        BBox.Max.X += BUILDING_KEEP_EDGE_DIST;
        BBox.Max.Y += 50;
        BBox.Max.Z += BUILDING_KEEP_EDGE_DIST;


        ctr++;
        for (i = 0; i < m_NodeList.GetCount(); i++)
        {
            if (BBox.Intersect(m_NodeList[i].Pos))
            {
                SavedNodeHandleList[m_NodeList.GetHandleFromIndex(i).Handle] = TRUE;
            }
        }
        x_DebugMsg("Scanning Building: %s nodes...\n", pBuilding->GetBuildingName());
    }
    ObjMgr.EndTypeLoop();

    ctr = 0;
    // Set all nodes within the expanded asset bbox to saved.
    ObjMgr.Select(object::ATTR_ASSET);
    while( (pAsset = (asset*)ObjMgr.GetNextSelected()) != NULL )
    {
        BBox = pAsset->GetBBox();

        // Inside building? skip it.
        if (InsideBuilding( BBox.Min ))
            continue;

        // Expand bbox size.
        BBox.Min.X -= ASSET_KEEP_EDGE_DIST;
        BBox.Min.Y -= ASSET_KEEP_EDGE_DIST;
        BBox.Min.Z -= ASSET_KEEP_EDGE_DIST;

        BBox.Max.X += ASSET_KEEP_EDGE_DIST;
        BBox.Max.Y += ASSET_KEEP_EDGE_DIST;
        BBox.Max.Z += ASSET_KEEP_EDGE_DIST;

        ctr++;
        for (i = 0; i < m_NodeList.GetCount(); i++)
        {
            if (BBox.Intersect(m_NodeList[i].Pos))
            {
                SavedNodeHandleList[m_NodeList.GetHandleFromIndex(i).Handle] = TRUE;
            }
        }
        x_DebugMsg("Scanning Asset: %s nodes...\n", pAsset->GetLabel());
    }
    ObjMgr.ClearSelection();

    ctr = 0;
    // Set all nodes within the expanded pickup bbox to saved.
    ObjMgr.StartTypeLoop( object::TYPE_PICKUP );
    while( (pPickup = (pickup*)ObjMgr.GetNextSelected()) != NULL )
    {
        BBox = pPickup->GetBBox();

        // Inside building? skip it.
        if (InsideBuilding( BBox.Min ))
            continue;

        // Expand bbox size.
        BBox.Min.X -= PICKUP_KEEP_EDGE_DIST;
        BBox.Min.Y -= PICKUP_KEEP_EDGE_DIST;
        BBox.Min.Z -= PICKUP_KEEP_EDGE_DIST;

        BBox.Max.X += PICKUP_KEEP_EDGE_DIST;
        BBox.Max.Y += PICKUP_KEEP_EDGE_DIST;
        BBox.Max.Z += PICKUP_KEEP_EDGE_DIST;

        ctr++;
        for (i = 0; i < m_NodeList.GetCount(); i++)
        {
            if (BBox.Intersect(m_NodeList[i].Pos))
            {
                SavedNodeHandleList[m_NodeList.GetHandleFromIndex(i).Handle] = TRUE;
            }
        }
        x_DebugMsg("Scanning Asset: %s nodes...\n", pAsset->GetLabel());
    }
    ObjMgr.EndTypeLoop();

    xhandle Handle;
    // Go through each edge and check if either nodes are connected to a save-node.
    for (i = 0; i < m_EdgeList.GetCount(); i++)
    {
        Handle = m_EdgeList.GetHandleFromIndex(i);
        if(SavedNodeHandleList[m_NodeList.GetIndexFromHandle(m_EdgeList[i].hNodeA)]
            ||
           SavedNodeHandleList[m_NodeList.GetIndexFromHandle(m_EdgeList[i].hNodeB)] )
        {
            // Keep this edge.
            SavedEdgeHandleList[Handle.Handle] = TRUE;
        }
        // Store the max handle.
        if (Handle.Handle > MaxHandle)
            MaxHandle = Handle.Handle;
    }

    // Delete edges
    for (i = 0; i < m_EdgeList.GetCount(); i++)
    {
        if (m_EdgeList[i].Type != EDGE_TYPE_GROWN)
            continue;
        Handle = m_EdgeList.GetHandleFromIndex(i);
        if (!SavedEdgeHandleList[Handle.Handle])
        {
            DeleteEdge(Handle);
            i--;
        }
    }

    // Delete all nodes with no edges.
    for (i = 0; i < m_NodeList.GetCount(); i++)
    {
        if (m_NodeList[i].nEdges == 0)
        {
            DeleteNode(m_NodeList.GetHandleFromIndex(i));
            i--;
        }
    }

}

//==============================================================================

void path_editor::RaiseNodeByBBox ( node& Node )
{
    if (Node.Type == NDTYPE_ADJUSTED)
        return;
    // Clear out the type.
    Node.Type = NDTYPE_NULL;

    // First check if this node is already on the ground.
    
    // Raise position of node a bit
    vector3 Pos = Node.Pos + vector3(0, 1.0f, 0);

    // Fire ray straight down
    vector3 Dir(0.0f, -1.5f, 0.0f);

    collider  Ray;
    collider::collision Col;
    Ray.RaySetup( NULL, Pos, Pos+Dir );
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
    if (!Ray.HasCollided() )
    {
        // Looks like this edge wasn't on the ground to begin with.
        return;
    }

    // Raise the bbox up by up to 4 meters, or until it hits something.
    vector3 BBoxMin, BBoxMax;
    // actual size of largest player's bbox (bioderm heavy)
    BBoxMin = vector3(Node.Pos.X - 0.95f, Node.Pos.Y, Node.Pos.Z - 0.95f);
    BBoxMax = vector3(Node.Pos.X + 0.95f, Node.Pos.Y + 2.6f, Node.Pos.Z + 0.95f);
    bbox BBox(BBoxMin, BBoxMax);
    collider Ext, DownExt;
    collider::collision CollisionUp, CollisionDown;

    vector3 MoveUp(0.0f, 1.0f, 0.0f);
    vector3 MoveDown(0.0f, -1.0f, 0.0f);

    f32 CurrentY = Node.Pos.Y;
    s32 TotalUp = 1;

    xbool HaventHitAnythingYet = TRUE;
    do
    {
        Ext.ExtSetup(NULL, BBox, MoveUp * (f32)TotalUp); 
        TotalUp++;
        ObjMgr.Collide( object::ATTR_SOLID_STATIC | object::ATTR_PERMANENT, Ext );

        // Extrude Upwards and see if we hit anything or went high enough..
        if ( Ext.HasCollided() || TotalUp > 4.0f)
        {
            HaventHitAnythingYet = FALSE;
            if (Ext.HasCollided() )
            {
                // Get the point of collision.
                Ext.GetCollision( CollisionUp );
                CurrentY = (CollisionUp.Point.Y - 2.6f - 0.1f);
            }
            else
            {
                CurrentY = Node.Pos.Y + (f32)TotalUp;
            }

            // Assume the position.
            BBoxMin.Y = CurrentY;
            BBoxMax.Y = CurrentY + 2.6f;

            BBox.Set(BBoxMin, BBoxMax);

            // Extrude Downwards.
            DownExt.ExtSetup(NULL, BBox, MoveDown * (TotalUp + 1.0f));
            ObjMgr.Collide( object::ATTR_SOLID_STATIC | object::ATTR_PERMANENT, DownExt );
            if ( DownExt.GetCollision(CollisionDown) )
            {
                // Get the point of collision.
                if (CollisionDown.Point.Y  < CurrentY - 0.1f)
                {
                    // Record the point.
                    CurrentY = CollisionDown.Point.Y;
                    break;
                }
                else
                {
                    xfs XFS( "STUCK BETWEEN (%4.2f,%4.2f,%4.2f) and (%4.2f,%4.2f,%4.2f), Map %s!\n",
                        Node.Pos.X, CurrentY, Node.Pos.Z, 
                        Node.Pos.X, CurrentY+2.6f, Node.Pos.Z, 
                        tgl.MissionName);
                    x_DebugMsg((const char*)XFS);
                    bot_log::Log((const char*)XFS);
#ifdef TARGET_PC
//                    MessageBox(NULL, (const char*)XFS, "Bad Node Placement (in Wall?)", MB_OK);
#endif
                                        // Log this node as a potential bad node.
                    Node.Type = NDTYPE_QUESTIONABLE;

                    return;
                }
            }
        }
    }
    while ( HaventHitAnythingYet );

    // Make sure we got a new Y value that's reasonable.
    if( Node.Pos.Y - 0.5 > CurrentY  || Node.Pos.Y + 4.0 < CurrentY)
    {
        // Log this node as a potential bad node.
        Node.Type = NDTYPE_QUESTIONABLE;

        xfs XFS( "Map %s: Questionable Node at (%4.2f,%4.2f,%4.2f)\n",
            tgl.MissionName,
            Node.Pos.X, Node.Pos.Y, Node.Pos.Z
            );
        x_DebugMsg((const char*)XFS);
        bot_log::Log((const char*)XFS);
    }
    else
    {
        if (x_abs(Node.Pos.Y - (CurrentY + 0.1f)) > 0.01f)
            bUpdated = TRUE;
        Node.Pos.Y = CurrentY + 0.1f;
        ScanNodeForCollision(Node);
    }

/*
    vector3 Normal;
    static f32 BBoxWidth = 1.0f; //1.9 * 0.5, heavy bio width / 2

    Ray.GetCollision(Col);
    Normal = Col.Plane.Normal;

    radian Theta = x_acos(Normal.Dot(vector3(0,1.0f,0)));
    

    f32 H = ( BBoxWidth / x_tan(R_90 - Theta) );
    // Intersection point
    Node.Pos.Y = Pos.Y + Ray.GetCollisionT()*Dir.Y;

    // Raise it to clear the bbox, plus a bit.
    Node.Pos.Y += H + 0.1f;
    */
}
//==============================================================================

xbool  path_editor::ScanNodeForCollision ( node &Node)
{
    // We want to run a scan on the bounding box for anything inside.
    // Six checks, up, down, left, right, back, forth. scan in every direction.
    vector3 Pos = Node.Pos;

    // up scan box
    bbox UpBBox(vector3(-0.95f, 0.0f, -0.95f), vector3(0.95f, 0.2f, 0.95f));
    static const vector3 Up(0,2.4f, 0);

    // down scan box
    bbox DownBBox(vector3(-0.95f, 2.4f, -0.95f), vector3(0.95f, 2.6f, 0.95f));
    static const vector3 Down(0, -2.4f, 0);

    // Left scan box
    bbox LBBox(vector3(0.75f, 0, -0.95f), vector3(0.95f, 2.6f, 0.95f));
    static const vector3 Left(-1.7f, 0, 0);
    // Right scan box
    bbox RBBox(vector3(-0.95f, 0, -0.95f), vector3(-0.75f, 2.6f, 0.95f));
    static const vector3 Right(1.7f, 0, 0);
    // Back scan box
    bbox BackBBox(vector3(-0.95f, 0, 0.75f), vector3(0.95f, 2.6f, 0.95f));
    static const vector3 Back(0 , 0, -1.7f);
    // Forth scan box
    bbox ForthBBox(vector3(-0.95f, 0, -0.95f), vector3(0.95f, 2.6f, -0.75f));
    static const vector3 Forth(0 , 0, 1.7f);

    UpBBox.Translate(Pos);
    DownBBox.Translate(Pos);
    LBBox.Translate(Pos);
    RBBox.Translate(Pos);
    BackBBox.Translate(Pos);
    ForthBBox.Translate(Pos);

    bbox ScanBox[6] = { UpBBox,
                        DownBBox,
                        LBBox,
                        RBBox,
                        BackBBox,
                        ForthBBox,
    };

    static vector3 Motion[6] = {Up,Down,Left,Right,Back,Forth};

    collider Ext;
    collider::collision Collision;

    s32 i;
    for (i =0; i < 6; i++)
    {
        Ext.ExtSetup(NULL, ScanBox[i], Motion[i]); 
        ObjMgr.Collide( object::ATTR_SOLID_STATIC | object::ATTR_PERMANENT, Ext );

        // Extrude in the direction and see if we hit anything or went high enough..
        if ( Ext.HasCollided() )
        {
            Node.Type = NDTYPE_QUESTIONABLE;
            return TRUE;
        }
    }
    Node.Type = NDTYPE_NULL;
    return FALSE;
}

//==============================================================================


//==============================================================================
// This recomputes the grid based on the graph structure in memory,
//  not the path editor class structure.
//  (in order to be sure we are using the right edge indices)
void path_editor::RecomputeGrid ( s32 nCellsWide, s32 nCellsDeep )
{
    if (g_Graph.m_nNodes <= 0)
        return;
    s32 i;

    // Find the bounds of the graph.
    vector3 Min = g_Graph.m_pNodeList[0].Pos, Max = g_Graph.m_pNodeList[0].Pos;
    for (i = 0; i < g_Graph.m_nNodes; i++)
    {
        if (g_Graph.m_pNodeList[i].Pos.X < Min.X)
            Min.X = g_Graph.m_pNodeList[i].Pos.X;
        if (g_Graph.m_pNodeList[i].Pos.Y < Min.Y)
            Min.Y = g_Graph.m_pNodeList[i].Pos.Y;
        if (g_Graph.m_pNodeList[i].Pos.Z < Min.Z)
            Min.Z = g_Graph.m_pNodeList[i].Pos.Z;

        if (g_Graph.m_pNodeList[i].Pos.X > Max.X)
            Max.X = g_Graph.m_pNodeList[i].Pos.X;
        if (g_Graph.m_pNodeList[i].Pos.Y > Max.Y)
            Max.Y = g_Graph.m_pNodeList[i].Pos.Y;
        if (g_Graph.m_pNodeList[i].Pos.Z > Max.Z)
            Max.Z = g_Graph.m_pNodeList[i].Pos.Z;
    }

    f32 GridWidth = Max.X - Min.X;
    f32 GridDepth = Max.Z - Min.Z;

    f32 CellWidth = GridWidth/nCellsWide;
    f32 CellDepth = GridDepth/nCellsDeep;

    // Clear away the previous grid
    if (m_Grid.Cell)
    {
        for (i = 0; i < m_Grid.nCellsWide; i++)
        {
            delete [] m_Grid.Cell[i];
        }
    }
    m_Grid.GridEdgesList.Clear();
    m_Grid.nGridEdges = 0;

    // Set grid values.
    m_Grid.Min = Min;
    m_Grid.Max = Max;

    m_Grid.nCellsWide = nCellsWide;
    m_Grid.nCellsDeep = nCellsDeep;

    m_Grid.CellWidth = CellWidth;
    m_Grid.CellDepth = CellDepth;

    // allocate memory for our grid.
    m_Grid.Cell = new grid_cell*[nCellsWide];
    for (i = 0; i < nCellsWide; i++)
    {
        m_Grid.Cell[i] = new grid_cell[nCellsDeep];
    }

    bbox CurCell;
    vector3 Size(CellWidth, m_Grid.Max.Y - m_Grid.Min.Y, CellDepth);

    CurCell.Min = m_Grid.Min;
    CurCell.Max = CurCell.Min + Size;

    // a little padding
    CurCell.Min.Y -= 0.1f;
    CurCell.Max.Y += 0.1f;

    vector3 A, B;
    f32 Intersect;
    s32 x, z;
    grid_cell* Cell;

    m_nMaxEdgesPerCell = 0;
    for ( x = 0; x < nCellsWide; x++)
    {
        CurCell.Min.X = m_Grid.Min.X + (x * CellWidth);
        CurCell.Max.X = m_Grid.Min.X + ((x+1) * CellWidth);

        // a little padding
        CurCell.Min.X -= 0.1f;
        CurCell.Max.X += 0.1f;

        if( CurCell.Min.X > m_Grid.Max.X)
        {
            ASSERT(0);
            break;
        }
        
        for (  z = 0; z < nCellsDeep; z++ )
        {
            CurCell.Min.Z = m_Grid.Min.Z + (z * CellDepth);
            CurCell.Max.Z = m_Grid.Min.Z + ((z + 1) * CellDepth);

            // a little padding
            CurCell.Min.Z -= 0.1f;
            CurCell.Max.Z += 0.1f;

            if( CurCell.Min.Z > m_Grid.Max.Z)
            {
                ASSERT(0);
                break; 
            }

            // Cell bbox is now set up.
            Cell = &m_Grid.Cell[x][z];
            Cell->CellBox = CurCell;
            Cell->nEdges = 0;
            Cell->EdgeOffset = m_Grid.nGridEdges;

            // Count up the intersecting edges.
            for (i = 0; i < g_Graph.m_nEdges; i++)
            {
                A = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeA].Pos;
                B = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeB].Pos;
                if (CurCell.Intersect(Intersect, A, B) && Intersect >= 0.0 && Intersect < 1.0f)
                {
                    Cell->nEdges++;
                    m_Grid.GridEdgesList.Append(i);
                    m_Grid.nGridEdges++;
                }
            }
            if (Cell->nEdges > m_nMaxEdgesPerCell)
            {
                m_nMaxEdgesPerCell = Cell->nEdges;
                LargestCell = Cell;
            }
        }
        ASSERT(m_Grid.nGridEdges == m_Grid.GridEdgesList.GetCount());
    }
    ASSERT(m_Grid.nGridEdges == m_Grid.GridEdgesList.GetCount());
}

//==============================================================================
void path_editor::Refresh_g_GRAPH( void )
{
    s32         i = 0;

    // Drop all ground nodes
    {
        const s32 Count = m_NodeList.GetCount();
        s32 i,j;

        for( i = 0; i < Count; i++ )
        {
            node&   Node = m_NodeList[i];
            
            // See if any of the edges that connect to this node are ground
            
            for ( j = 0; j < Node.nEdges; ++j )
            {
                const edge& Edge = m_EdgeList( Node.hEdgeList[j] );
                if ( Edge.Type != EDGE_TYPE_AIR )
                {
                    // yep, drop it and move on
                    vector3 Pos = Node.Pos;
                    m_hSelectedNode = m_NodeList.GetHandleFromIndex( i );
                    m_bChanged = TRUE;
                    DropNode();
                    if ( (Pos - Node.Pos).LengthSquared() > x_sqr( 10.0f ) )
                    {
                        xfs XFS( "Map %s: Drop() moved node far:(%4.2f,%4.2f,%4.2f)\n",
                            tgl.MissionName,
                            Node.Pos.X, Node.Pos.Y, Node.Pos.Z
                            );
                        x_DebugMsg((const char*)XFS);
                        bot_log::Log((const char*)XFS);
                    }
                    break;
                }
            }

            // Confirm that this node has something beneath it
            {
                // Fire ray straight down
                vector3 Dir(0.0f, -1000.0f, 0.0f);

                vector3 Pos = Node.Pos;
                collider  Ray;
                Ray.RaySetup( NULL, Pos, Pos+Dir );
                ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
                if (!Ray.HasCollided())
                {
                    xfs XFS( "Map %s: Drop didn't get node above ground:(%4.2f,%4.2f,%4.2f)\n",
                        tgl.MissionName,
                        Node.Pos.X, Node.Pos.Y, Node.Pos.Z
                        );
                    x_DebugMsg((const char*)XFS);
                    bot_log::Log((const char*)XFS);
                }
            }
        }
    }
    

#define TWEAK_FLAG_EDGES 0
#if TWEAK_FLAG_EDGES
    g_NodeAdjusted = FALSE;
    xbool CentralNodeAdjusted = FALSE;
    // Get flag & nexus objects for fine tuning
    flag* pFlag;
    flag* Flag[2] = {NULL, NULL};
    bbox FlagBBox[2];
    nexus*       Nexus   = NULL;
    vector3 NexusPos;
    bbox NexusBBox;

    ObjMgr.StartTypeLoop(object::TYPE_FLAG);
    while ((pFlag = (flag*)ObjMgr.GetNextInType()) != NULL)
    {
        ASSERT(i < 2);
        Flag[i] = pFlag;
        FlagBBox[i].Min = pFlag->GetPosition() + vector3(-1.0f, -1.0f, -1.0f);
        FlagBBox[i].Max = pFlag->GetPosition() + vector3( 1.0f,  1.0f,  1.0f);
        i++;
    }
    ObjMgr.EndTypeLoop();

    ObjMgr.StartTypeLoop(object::TYPE_NEXUS);
    Nexus = (nexus*)ObjMgr.GetNextInType();
    if (Nexus)
    {
        NexusPos = Nexus->GetPosition();
        collider Ray;
        Ray.RaySetup(NULL, NexusPos + vector3(0.0f, 4.0f, 0.0f),
                           NexusPos);
        ObjMgr.Collide(object::ATTR_PERMANENT, Ray);
    
        collider::collision Collision;
        Ray.GetCollision(Collision);

        NexusPos = Collision.Point + vector3(0.0f, 0.1f,0.0f);

        NexusBBox = Nexus->GetBBox();
    }
    ObjMgr.EndTypeLoop();
#endif
    s32         TotalEdgeNodes = 0;
    s32         iNodeEdgeList  = 0;
    s32         nEdges         = 0;
    xhandle*    pEdge          = new xhandle[ m_EdgeList.GetCount() ];
    ASSERT( pEdge );

    // Cost Analysis array.
    if (m_GraphToPathHandle)
    {
        delete [] m_GraphToPathHandle;
        m_GraphToPathHandle = NULL;
    }


    // Get acount of how many nodeeges we have
    for( i=0; i<m_NodeList.GetCount(); i++ )
    {
        TotalEdgeNodes += m_NodeList[i].nEdges;
    }

    g_Graph.m_nNodeEdges = TotalEdgeNodes;
    g_Graph.m_pNodeEdges = new s16[g_Graph.m_nNodeEdges];
    ASSERT( g_Graph.m_pNodeEdges );

    g_Graph.m_nEdges    = m_EdgeList.GetCount();
    g_Graph.m_pEdgeList = new graph::edge[g_Graph.m_nEdges];
    ASSERT( g_Graph.m_pEdgeList );

    g_Graph.m_nNodes    = m_NodeList.GetCount();
    g_Graph.m_pNodeList = new graph::node[g_Graph.m_nNodes];
    ASSERT( g_Graph.m_pNodeList );

    g_Graph.m_pNodeInside = new xbool[g_Graph.m_nNodes];
    ASSERT( g_Graph.m_pNodeList );

    m_GraphToPathHandle = new xhandle[g_Graph.m_nEdges];
    ASSERT(m_GraphToPathHandle);
    //
    // Fill in the graph node and the graph nodeedges
    //
    for( i=0; i<m_NodeList.GetCount(); i++)
    {
        node&           Node    = m_NodeList[i];
        graph::node&    FNode   = g_Graph.m_pNodeList[i];
#if TWEAK_FLAG_EDGES
        // Check if this node lies on a flag/nexus.
        if (Flag[0])
        {
            if (FlagBBox[0].Intersect(Node.Pos))
            {
                // Move the position to the flag position.
                Node.Pos = Flag[0]->GetPosition() + vector3(0.0f,0.1f,0.0f);
                CentralNodeAdjusted = TRUE;
                Node.Type = NDTYPE_ADJUSTED;
                if (ScanNodeForCollision(Node))
                {
                    bot_log::Log ( "Warning- Node Adjustment moved into bad spot: ");
                    vector3 RayStart = Flag[0]->GetPosition() + vector3(0.0f, 3.0f, 0.0f);
                    collider Ray;
                    Ray.RaySetup(NULL, RayStart, RayStart + vector3(0.0f, -5.0f, 0.0f));
                    ObjMgr.Collide(object::ATTR_PERMANENT, Ray);

                    if (Ray.HasCollided())
                    {
                        collider::collision Collision;
                        Ray.GetCollision(Collision);

                        Node.Pos.Y = Collision.Point.Y + 0.1f;
                    }
                    else
                    {
                        ASSERT(0);
                    }
                    if( ScanNodeForCollision(Node) )
                    {
                        bot_log::Log ( "Unfixed\n" );
                    }
                    else
                    {
                        bot_log::Log ( "Fixed\n" );
                    }
                }
            }
            else if (FlagBBox[1].Intersect(Node.Pos))
            {
                // Move the position to the flag position.
                Node.Pos = Flag[1]->GetPosition() + vector3(0.0f,0.1f,0.0f);
                CentralNodeAdjusted = TRUE;
                Node.Type = NDTYPE_ADJUSTED;
                if (ScanNodeForCollision(Node))
                {
                    bot_log::Log ( "Warning- Node Adjustment moved into bad spot: ");
                    vector3 RayStart = Flag[1]->GetPosition() + vector3(0.0f, 3.0f, 0.0f);
                    collider Ray;
                    Ray.RaySetup(NULL, RayStart, RayStart + vector3(0.0f, -5.0f, 0.0f));
                    ObjMgr.Collide(object::ATTR_PERMANENT, Ray);

                    if (Ray.HasCollided())
                    {
                        collider::collision Collision;
                        Ray.GetCollision(Collision);

                        Node.Pos.Y = Collision.Point.Y + 0.1f;
                    }
                    else
                    {
                        ASSERT(0);
                    }
                    if( ScanNodeForCollision(Node) )
                    {
                        bot_log::Log ( "Unfixed\n" );
                    }
                    else
                    {
                        bot_log::Log ( "Fixed\n" );
                    }
                }
            }
        }
        else if (Nexus)
        {
            if (Nexus->GetBBox().Intersect(Node.Pos))
            {
                Node.Pos = NexusPos;
                CentralNodeAdjusted = TRUE;
                if (ScanNodeForCollision(Node))
                {
                    bot_log::Log ( "Warning- Node Adjustment moved into bad spot: ");
                    vector3 RayStart = Nexus->GetPosition() + vector3(0.0f, 3.0f, 0.0f);
                    collider Ray;
                    Ray.RaySetup(NULL, RayStart, RayStart + vector3(0.0f, -5.0f, 0.0f));
                    ObjMgr.Collide(object::ATTR_PERMANENT, Ray);

                    if (Ray.HasCollided())
                    {
                        collider::collision Collision;
                        Ray.GetCollision(Collision);

                        Node.Pos.Y = Collision.Point.Y + 0.1f;
                    }
                    else
                    {
                        ASSERT(0);
                    }
                    if( ScanNodeForCollision(Node) )
                    {
                        bot_log::Log ( "Unfixed\n" );
                    }
                    else
                    {
                        bot_log::Log ( "Fixed\n" );
                    }
                }
                Node.Type = NDTYPE_ADJUSTED;
            }
        }
#endif
        // Test the node to make sure it is clear.
        if ( ScanNodeForCollision(Node) )
        {
            // Raise position of node a bit
            vector3 Pos = Node.Pos + vector3(0, 1.0f, 0);

            // Fire ray straight down
            vector3 Dir(0.0f, -1.5f, 0.0f);

            collider  Ray;
            collider::collision Col;
            Ray.RaySetup( NULL, Pos, Pos+Dir );
            ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
            if (!Ray.HasCollided() )
            {
                // Attempt to lower the node automatically to a safe level.
                if ( !LowerNode(&Node) )
                    bot_log::Log ( "Problem Node!\n" );
            }
        }

        // Back up the start of the node personal edge list
        FNode.EdgeIndex = iNodeEdgeList;

        for( s32 j=0; j<Node.nEdges; j++ )
        {
            //
            // Find or add the edge
            //
            for( s32 k=0; k<nEdges; k++ )
            {
                if( pEdge[k] == Node.hEdgeList[j] ) break;
            }

            if( k == nEdges )
            {
                pEdge[k] = Node.hEdgeList[j];
                nEdges++;
            }

            //
            // Put the edge in the nodeedge list for the node
            //
            g_Graph.m_pNodeEdges[ iNodeEdgeList++ ] = k;
        }

        // Fill the rest of the node info
        FNode.nEdges = iNodeEdgeList - FNode.EdgeIndex;
        FNode.Pos    = Node.Pos;

        // Fill the NodeInside list
        g_Graph.m_pNodeInside[i]
            = PointInsideBuilding( m_NodeList[i].Pos );

    }

    collider Ray;
    f32 t = -1.0f;
    for( i=0; i<nEdges; i++)
    {
        edge&   Edge = m_EdgeList( pEdge[i] );

        g_Graph.m_pEdgeList[i].NodeA  = m_NodeList.GetIndexFromHandle( Edge.hNodeA );
        g_Graph.m_pEdgeList[i].NodeB  = m_NodeList.GetIndexFromHandle( Edge.hNodeB );

        g_Graph.m_pEdgeList[i].Type   = Edge.Type;

        g_Graph.m_pEdgeList[i].Flags  = 0;
        g_Graph.m_pEdgeList[i].Flags |= Edge.bLoose  ? graph::EDGE_FLAGS_LOOSE  : 0;

#if TWEAK_FLAG_EDGES
        // Test for edge-flag stand collision.
        if (!CentralNodeAdjusted && Flag[0])
        {
            if (FlagBBox[0].Intersect(t, m_NodeList(Edge.hNodeA).Pos, m_NodeList(Edge.hNodeB).Pos)
                &&  ( t >= 0 && t <= 1.0f))
            {
                AdjustNodesToCrossPoint(m_NodeList(Edge.hNodeA), m_NodeList(Edge.hNodeB),
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeA],
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeB],
                                        Flag[0]->GetPosition() + vector3(0.0f, 0.1f, 0.0f));
            }

            if (FlagBBox[1].Intersect(t, m_NodeList(Edge.hNodeA).Pos, m_NodeList(Edge.hNodeB).Pos)
                &&  ( t >= 0 && t <= 1.0f))
            {
                AdjustNodesToCrossPoint(m_NodeList(Edge.hNodeA), m_NodeList(Edge.hNodeB),
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeA],
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeB],
                                        Flag[1]->GetPosition()+ vector3(0.0f, 0.1f, 0.0f));
            }
        }

        if (!CentralNodeAdjusted && Nexus)
        {
            if (NexusBBox.Intersect(t, m_NodeList(Edge.hNodeA).Pos, m_NodeList(Edge.hNodeB).Pos)
                &&  ( t >= 0 && t <= 1.0f))
            {
                AdjustNodesToCrossPoint(m_NodeList(Edge.hNodeA), m_NodeList(Edge.hNodeB),
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeA],
                                        g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeB],
                                        NexusPos);
            }
        }
#endif
        // Assumption:  both nodes are inside a building => the edge is also inside.
        if (g_Graph.m_pNodeInside[g_Graph.m_pEdgeList[i].NodeA]
            && g_Graph.m_pNodeInside[g_Graph.m_pEdgeList[i].NodeB])
            g_Graph.m_pEdgeList[i].Flags |= graph::EDGE_FLAGS_INDOOR;

        // Does this edge cross a force field?
        Ray.RaySetup(NULL, 
            g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeA].Pos,
            g_Graph.m_pNodeList[g_Graph.m_pEdgeList[i].NodeB].Pos,
            -1, TRUE);
        ObjMgr.Collide( object::ATTR_FORCE_FIELD, Ray);
        if (Ray.HasCollided())
        {
            g_Graph.m_pEdgeList[i].Flags |= graph::EDGE_FLAGS_FORCE_FIELD;
        }

        // Precomputed Edge Costs
        g_Graph.m_pEdgeList[i].CostFromA2B = //(u16)Edge.CostFromA2B;
            EdgeCost(m_NodeList(Edge.hNodeA).Pos, m_NodeList(Edge.hNodeB).Pos,
            Edge.Type, Edge.bLoose);
        g_Graph.m_pEdgeList[i].CostFromB2A = //(u16)Edge.CostFromB2A
            EdgeCost(m_NodeList(Edge.hNodeB).Pos, m_NodeList(Edge.hNodeA).Pos,
            Edge.Type, Edge.bLoose);

        m_GraphToPathHandle[i] = pEdge[i];
    }

#if USING_PREPROCESSED_VPS
    // Fill in the vantage point data.
    if (ObjMgr.GetObjectFromID(g_BotObjectID) == NULL)
    {
        // We need a new bot.
        s_pBotObject = (bot_object*)ObjMgr.CreateObject( object::TYPE_BOT );
        s_pBotObject->Initialize( vector3(0,100,0),                             // Pos
                                  player_object::CHARACTER_TYPE_FEMALE,         // Character type
                                  player_object::SKIN_TYPE_BEAGLE,              // Skin type
                                  player_object::VOICE_TYPE_FEMALE_HEROINE,     // Voice type
                                  default_loadouts::STANDARD_BOT_HEAVY,         // Default loadout
                                  "PEPE THE HAPPY BOT",                         // Name
                                  TRUE );                                       // Created from editor or not

        s_pBotObject->SetTeamBits(2);

        ObjMgr.AddObject( s_pBotObject, obj_mgr::AVAILABLE_SERVER_SLOT );
        s_pBotObject->Respawn() ;
        s_pBotObject->Player_SetupState( player_object::PLAYER_STATE_IDLE );

        g_BotObjectID = s_pBotObject->GetObjectID();
    }
    s_pBotObject = ObjMgr.GetObjectFromID(g_BotObjectID);
    ASSERT(s_pBotObject);

    g_Graph.m_nVantagePoints = 0;
    asset* Asset = NULL;
    xbool FoundLinear = FALSE;
    s32   LinearNode;
    xbool FoundMortar = FALSE;
    s32   MortarNode;
    s32 nAssets = 0;

    ObjMgr.Select(ATTR_ASSET);
    while ((Asset = (asset*)ObjMgr.GetNextSelected()) != NULL)
    {
        if (!(Asset->GetAttrBits() & object::ATTR_PERMANENT))
            continue;
        // Count up all the assets first.
        nAssets++;
    }
    ObjMgr.ClearSelection();

    // Set up our array.
    
    ObjMgr.Select(ATTR_ASSET);
    while ((Asset = (asset*)ObjMgr.GetNextSelected()) != NULL)
    {
        if (!(Asset->GetAttrBits() & object::ATTR_PERMANENT))
            continue;

        s_pBotObject->GetDestroyVantageNodes(FoundLinear, LinearNode, FoundMortar, MortarNode, *Asset);
        vector3 Pos = Asset->GetPosition();

        if (FoundLinear)
        {
            g_Graph.
        }
        else if (FoundMortar)
        {

        }
        else
        {
            x_DebugMsg("%s-%s: No Vantage Point!  (%4.2f, %4.2f, %4.2f)\n",
                tgl.MissionName, Asset->GetLabel(), Pos.X, Pos.Y, Pos.Z);
            bot_log::Log("%s-%s: No Vantage Point!  (%4.2f, %4.2f, %4.2f)\n",
                tgl.MissionName, Asset->GetLabel(), Pos.X, Pos.Y, Pos.Z);
        }
    }
    ObjMgr.ClearSelection();
#endif

    //
    // Free the temp memory
    //
    delete []pEdge;
}

//==============================================================================

void path_editor::AddMarker( const vector3 &Pos, s32 Checks )
{
    if (m_nMarks >= 50)
        return;
    s32 i;
    for (i = 0; i < m_nMarks; i++)
    {
        if ((m_Mark[i].Pos - Pos).LengthSquared() < 25.0f)
        {
            return;
        }
    }

    m_Mark[m_nMarks].Pos = Pos;
    m_Mark[m_nMarks].Intensity = Checks;
 
    m_nMarks++;
}

//==============================================================================

void path_editor::AdjustNodesToCrossPoint ( node& NodeA, node& NodeB,
                                            graph::node& GNodeA, graph::node& GNodeB,
                                            const vector3& Point)
{
    if (!g_NodeAdjusted)
    {
        g_NodeAdjusted = TRUE;
        bot_log::Log( "** ADJUSTING NODES IN %s **\n", tgl.MissionName );
    }
    ASSERT(NodeA.Pos == GNodeA.Pos && NodeB.Pos == GNodeB.Pos);
    f32 OldAY = NodeA.Pos.Y;
    f32 OldBY = NodeB.Pos.Y;

    NodeA.Pos.Y = Point.Y;
    NodeB.Pos.Y = Point.Y;

    NodeA.Type = NDTYPE_ADJUSTED;
    NodeB.Type = NDTYPE_ADJUSTED;

    f32 EdgeLength = (NodeB.Pos - NodeA.Pos).Length();
    vector3 Dir = Point - NodeA.Pos;
    Dir.Normalize();
    Dir.Scale(EdgeLength);

    vector3 NewBPos = NodeA.Pos + Dir;
    if( (NewBPos - NodeB.Pos).Length() > 2.0f)
    {
        bot_log::Log ( "Warning- Node Adjustment moved over 2 meters\n");
    }
    NodeB.Pos = NewBPos;

    if( ScanNodeForCollision(NodeB) || ScanNodeForCollision(NodeA) )
    {
        bot_log::Log ( "Warning- Node Adjustment moved into bad spot: ");
        vector3 RayStart = Point + vector3(0.0f, 3.0f, 0.0f);
        collider Ray;
        Ray.RaySetup(NULL, RayStart, RayStart + vector3(0.0f, -5.0f, 0.0f));
        ObjMgr.Collide(object::ATTR_PERMANENT, Ray);

        if (Ray.HasCollided())
        {
            collider::collision Collision;
            Ray.GetCollision(Collision);

            NodeA.Pos.Y = MAX(Collision.Point.Y + 0.1f, OldAY);
            NodeB.Pos.Y = MAX(Collision.Point.Y + 0.1f, OldBY);
        }
        else
        {
            ASSERT(0);
            NodeA.Pos.Y = OldAY;
            NodeB.Pos.Y = OldBY;
        }
        if( ScanNodeForCollision(NodeB) || ScanNodeForCollision(NodeA) )
        {
            bot_log::Log ( "Unfixed\n" );
        }
        else
        {
            bot_log::Log ( "Fixed\n" );
        }
    }

    GNodeA.Pos = NodeA.Pos;
    GNodeB.Pos = NodeB.Pos;
}

//==============================================================================

void path_editor::TestAssetsForEdges( void )
{
#ifdef T2_DESIGNER_BUILD
//#if (DESIGNER_BUILD)
    s32 testbool = 1;
    if (testbool)
        x_DebugMsg("Got here!!!\n");
#endif
    asset* Asset;
    vector3 AssPos;
    bbox BBox;
    vector3 BoxMin, BoxMax;
    f32 Width, Depth;

    graph::edge *Edge;
    graph::node *NodeA, *NodeB;
    s32 i;
    s32 EdgeIdx;
    s32 GridX, GridZ;

    f32 IntersectPoint;
    graph::grid_cell *Cell;

    ObjMgr.Select(object::ATTR_ASSET);
    while ((Asset = (asset*)ObjMgr.GetNextSelected()) != NULL)
    {
        if (!(Asset->GetAttrBits() & object::ATTR_PERMANENT))
            continue;
        xbool FoundEdge = FALSE;

        AssPos = Asset->GetPosition();

        BoxMin = Asset->GetBBox().Min;
        BoxMax = Asset->GetBBox().Max;
        Width = BoxMax.X - BoxMin.X;
        Depth = BoxMax.Z - BoxMin.Z;
        BoxMin.X -= Width;
        BoxMin.Y -= 2.0f;
        BoxMin.Z -= Depth;
        BoxMax.X += Width;
        BoxMax.Z += Depth;

        BBox.Set(BoxMin, BoxMax);

        for (GridX = g_Graph.GetColumn(BBox.Min.X);
             GridX <= g_Graph.GetColumn(BBox.Max.X);
             GridX ++)
        {
            for (GridZ = g_Graph.GetRow(BBox.Min.Z);
                GridZ <= g_Graph.GetRow(BBox.Max.Z);
                GridZ ++)
            {
                 // Test all these edges for an edge close enough.
                Cell = &g_Graph.m_Grid.Cell[GridX][GridZ];

                for (i = Cell->EdgeOffset; 
                    i < Cell->EdgeOffset + Cell->nEdges; i++)
                {
                    EdgeIdx = g_Graph.m_Grid.GridEdgesList[i];
                    Edge = &g_Graph.m_pEdgeList[EdgeIdx];
                    NodeA = &g_Graph.m_pNodeList[Edge->NodeA];
                    NodeB = &g_Graph.m_pNodeList[Edge->NodeB];

                    if (BBox.Intersect(IntersectPoint,NodeA->Pos, NodeB->Pos)
                        && IntersectPoint <= 1.0f && IntersectPoint >= 0)
                    {
                        FoundEdge = TRUE;
                        break;
                    }
                }
                if (FoundEdge)
                    break;
            }
            if (FoundEdge)
             break;
        }
        if (!FoundEdge)
        {
            // Graph warning message
            bot_log::Log("%s, No edge close enough to %s (%4.2f, %4.2f, %4.2f)\n",
                tgl.MissionName, Asset->GetLabel(), AssPos.X, AssPos.Y, AssPos.Z);
        }
    }
    ObjMgr.ClearSelection();
}

