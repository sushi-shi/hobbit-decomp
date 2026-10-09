//==============================================================================
//
//  PathManager.cpp
//
//==============================================================================

//==============================================================================
//  DEFINES
//==============================================================================

#define FILENAME        "C:\\bigtest.gph"
#define MAX_TIME_SLICE  2.0
#define PATH_FIND_DEBUG 1
#define DEBUG_LOG_PATH  0
#define BOT_RADIUS      0.5
#define BOT_HEIGHT      2
#define EDGE_CHECK_PERCENTAGE   0.20

//==============================================================================
//  INCLUDES
//==============================================================================

#include "PathManager.hpp"
#include "BotLog.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "Support\Building\BuildingOBJ.hpp"
#include "AADS\Globals.hpp"

//==============================================================================
//  VARIABLES
//==============================================================================

static xbool            s_SlotFilled[MAX_BOTS] = {
    FALSE,   FALSE,   FALSE,   FALSE,       FALSE,   FALSE,   FALSE,   FALSE,   
    FALSE,   FALSE,   FALSE,   FALSE,       FALSE,   FALSE,   FALSE,   FALSE,   
    FALSE,   FALSE,   FALSE,   FALSE,       FALSE,   FALSE,   FALSE,   FALSE,   
    FALSE,   FALSE,   FALSE,   FALSE,       FALSE,   FALSE,   FALSE,   FALSE,   
};

static xbool            s_LoadGraph=FALSE;
extern graph            g_Graph;

#if SHOW_EDGE_COSTS
extern s32              g_EdgeCost[2000];
#endif
#define MARK_COSTLY_GCET 0
#if MARK_COSTLY_GCET
    #include "BotEditor/PathEditor.hpp"
    extern path_editor      s_PathEditor;
#endif
//extern xbool            g_bCostAnalysisOn;

//==============================================================================

void PathManager_LoadGraph ( const char* pFilename )
{
    (void)pFilename;
    if( s_LoadGraph == FALSE ) 
    {
        s_LoadGraph = TRUE;
        //g_Graph.Load(pFilename);
    }
}

//==============================================================================

xbool PathManager_IsGraphLoaded( void )
{
    return s_LoadGraph;
}

//==============================================================================

static xbool ExtrusionClearBetweenPoints( const vector3& Position1
    , const vector3& Position2 );

static s32 FindFreeSlot( void );

//==============================================================================

void Path_Manager_InitializePathMng( void )
{
    x_memset(s_SlotFilled, 0, sizeof(s_SlotFilled));
    s_LoadGraph = FALSE;
}

//==============================================================================

void Path_Manager_ForceLoad( void )
{
    s_LoadGraph = FALSE;
}

//==============================================================================

void path_manager::Reset( void )
{
    ResetVisited(m_BotID);

    m_GlobalCounter = 0;
    m_Start = 0;
    m_End = 0;
    m_Flags = 0;
    m_PriorityQueue.Reset();

    m_TimeSegment = 0;
    m_TimeComplete = 0;
    m_bAppendWaiting = FALSE;
    m_iStartEdge = -1;
    m_iEndEdge = -1;

    TriedAgain = FALSE;
    m_BlockedZeroEdgePath = FALSE;

    m_MaxAirDist = 100;

    m_TeamFlags = graph::EDGE_FLAGS_TEAM_0_BLOCK|graph::EDGE_FLAGS_TEAM_1_BLOCK;

    m_UsingPartialPath = FALSE;
    m_PartialPathDst.Zero();

}
//==============================================================================

void path_manager::Init(const char* pFilename)
{
    PathManager_LoadGraph(pFilename);
    Reset();
    m_Initialized = TRUE;
}

//==============================================================================

path_manager::path_manager()
{
    m_TeamBits = 0;
    // Find an empty space for the bot if there exists one.
    m_BotID = FindFreeSlot();
    if (m_BotID == -1)
    {
        ASSERTS(0, "Max number of bots exceeded?");
    }

    m_Initialized = FALSE;
}

//==============================================================================

path_manager::~path_manager()
{
}

//==============================================================================

void path_manager::ResetVisited(s32 BotID)
{
    m_GlobalCounter = 1;

    for (s32 i = 0; i < g_Graph.m_nNodes; i ++)
    {
        g_Graph.m_pNodeList[i].PlayerInfo[BotID].SetVisited(0);
    }
}

//==============================================================================
void path_manager::ResetAllVisited()
{
    for (s32 i = 0; i < MAX_BOTS; i++)
    {
        ResetVisited(i);
    }
}

//==============================================================================

u16 path_manager::GetIdxNearestPoint( vector3 Point ) const 
{
    s32 i;
    u16 NearestIdx = 0;
    f32 NearestDistSQ = F32_MAX;
    f32 TempDistSQ;

    for ( i = 0; i < g_Graph.m_nNodes; ++i )
    {
        TempDistSQ = (Point - g_Graph.m_pNodeList[i].Pos).LengthSquared();
        if ( TempDistSQ < NearestDistSQ )
        {
            NearestDistSQ = TempDistSQ;
            NearestIdx = i;
        }
    }

    return NearestIdx;
}

//==============================================================================
u16 path_manager::FindPath(xarray<u16>& Array, xbool AllAtOnce /* = FALSE */ )
{
    ASSERT(m_Start != m_End);
    ASSERT( m_Initialized && s_SlotFilled[m_BotID]);
    graph::node *pStart, *pEnd;

    g_Graph.UpdateBlockedEdges();
    
    // Init everything.
    m_TimeSegment = 0;      // For checking total time

    if (++m_GlobalCounter >= U8_MAX - 1)
    {
        ResetVisited(m_BotID);
    }

#if defined (acheng)
    s32 i;
    for (i = 0; i < g_Graph.m_nNodes; i++)
    {
        ASSERT(m_GlobalCounter > g_Graph.m_pNodeList[i].PlayerInfo[m_BotID].GetVisited());
    }
#endif

    pStart = &g_Graph.m_pNodeList[m_Start];
    pEnd   = &g_Graph.m_pNodeList[m_End];

    // Now just find the path between the existing nodes.
    m_PriorityQueue.Reset();

    // Set the distance traveled to 0.
    pStart->PlayerInfo[m_BotID].SetCostSoFar(0);

    // Insert the first node into the queue.
    m_PriorityQueue.Insert(pStart, DetermineCost(m_Start, m_End));
        
    pStart->PlayerInfo[m_BotID].SetVisited(m_GlobalCounter);

#if defined (acheng)
    #if PATH_FIND_DEBUG
//                x_DebugMsg("-%d)FP() starting from node %d to node %d.\n", m_BotID, m_Start, m_End);
    #endif
#endif
    m_Flags |= BOT_PATH_FLAG_SEARCHING;

    u16 Ret = Update( Array, AllAtOnce );
    return Ret;
}

//==============================================================================
xtimer FindPathTimer;
void path_manager::FindPath( xarray<u16> &Array, const vector3& Start,
                                const vector3& End, f32 MaxAir, u8 Flags, 
                                xbool AllAtOnce /*= FALSE */ )
{
FindPathTimer.Start();
    m_TeamFlags = Flags;

    m_iStartEdge = GetClosestEdge( Start );
    m_iEndEdge   = GetClosestEdge( End );
/*    if (m_iStartEdge != OldGetClosestEdge(Start)
        || m_iEndEdge != OldGetClosestEdge(End))
    {
        x_printf("New ClosestEdge differs from old ClosestEdge");
    }
    */
    vector3 StartEdgeA = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[m_iStartEdge].NodeA].Pos;
    vector3 StartEdgeB = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[m_iStartEdge].NodeB].Pos;

    vector3 EndEdgeA = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[m_iEndEdge].NodeA].Pos;
    vector3 EndEdgeB = g_Graph.m_pNodeList[g_Graph.m_pEdgeList[m_iEndEdge].NodeB].Pos;

    vector3 DistStart_End = Start - End;
    f32 ScalarDistanceSq = DistStart_End.LengthSquared();

    ASSERT(MaxAir > 15 && MaxAir <= 200);
    m_MaxAirDist = MaxAir;

    // Attempt to find a shorter path first.
    if ( !m_BlockedZeroEdgePath)
    {                         // Within 50 m
        if(  ScalarDistanceSq < 2500.0f && ExtrusionClearBetweenPoints(Start, End) )
        {
            vector3 DistStart_Edge,
                    DistEnd_Edge,
                    DistBtwnClosestEdgePoints;

            DistStart_Edge = Start.GetClosestVToLSeg(StartEdgeA, StartEdgeB);
            DistEnd_Edge =   End.GetClosestVToLSeg(EndEdgeA, EndEdgeB);
            DistBtwnClosestEdgePoints = (Start + DistStart_Edge) - (End + DistEnd_Edge);

            if (ScalarDistanceSq < DistStart_Edge.LengthSquared() + DistEnd_Edge.LengthSquared()
                + DistBtwnClosestEdgePoints.LengthSquared())
            {
                // It's shorter to go directly to the target than to go following paths.
                // **************TO DO***************
                // First make sure it's a clear path.
                // **********************************

                Array.Clear();
                m_Flags &= ~BOT_PATH_FLAG_SEARCHING;
    #if defined (acheng)
//                x_DebugMsg("****** Found 0-edge path:: FindPath ******\n");
    #endif
FindPathTimer.Stop();
                return;
            }
        }
    }
    else
    {
#if defined (acheng)
//        x_DebugMsg("****** Zero Edge Path finding re-enabled ******\n");
#endif
        m_BlockedZeroEdgePath = FALSE;
    }

    // Find the closest nodes to the start and the end, 
    //          and mark them as our start end end nodes.
    f32 DistA1, DistB1;
    DistA1 = (Start - StartEdgeA).LengthSquared();
    DistB1 = (Start - StartEdgeB).LengthSquared();
    if (DistA1 < DistB1) 
    {
        m_Start = g_Graph.m_pEdgeList[m_iStartEdge].NodeA;
    }
    else
    {
        m_Start = g_Graph.m_pEdgeList[m_iStartEdge].NodeB;
    }
    
    f32 DistA2, DistB2;
    DistA2 = (End - EndEdgeA).LengthSquared();
    DistB2 = (End - EndEdgeB).LengthSquared();
    if (DistA2 < DistB2) 
    {
        m_End = g_Graph.m_pEdgeList[m_iEndEdge].NodeA;
    }
    else
    {
        m_End = g_Graph.m_pEdgeList[m_iEndEdge].NodeB;
    }
   
    if( m_iStartEdge == m_iEndEdge )
    {
        Array.Append( m_iStartEdge );
        m_Flags &= ~BOT_PATH_FLAG_SEARCHING;
FindPathTimer.Stop();
        return;
    }

    if (m_Start != m_End )
    {
        m_Flags |= BOT_PATH_FLAG_SEARCHING;
        FindPath( Array, AllAtOnce );
    }
    else
    {
        // If both the edges have this node, then the path is simply the two edges.
        Array.Append(m_iStartEdge);
        Array.Append(m_iEndEdge);
        m_Flags &= ~BOT_PATH_FLAG_SEARCHING;
FindPathTimer.Stop();
        return;
    }

    if (!Searching())
    {
        Append(Array);
    }
    else
    {
        m_bAppendWaiting = TRUE;
    }
FindPathTimer.Stop();
}

//==============================================================================

void path_manager::Append( xarray<u16> &Array)
{
    // Tack on the stored start edge and end edge if they are not already
    //  in the array.
    if(!Array || Array[0] != m_iStartEdge )
    {
        // Make sure the start edge connects to the first edge in the array
        if (Array)
        {
            ASSERT( g_Graph.m_pEdgeList[m_iStartEdge].NodeA
                == g_Graph.m_pEdgeList[Array[0]].NodeA
            
                || g_Graph.m_pEdgeList[m_iStartEdge].NodeA
                == g_Graph.m_pEdgeList[Array[0]].NodeB
            
                || g_Graph.m_pEdgeList[m_iStartEdge].NodeB
                == g_Graph.m_pEdgeList[Array[0]].NodeA
            
                || g_Graph.m_pEdgeList[m_iStartEdge].NodeB
                == g_Graph.m_pEdgeList[Array[0]].NodeB );
        }
        
        Array.Insert(0, m_iStartEdge );
    }

    if( Array[Array.GetCount()-1] != m_iEndEdge && !IsUsingPartialPath() )
    {
        ASSERT( g_Graph.m_pEdgeList[m_iEndEdge].NodeA
            == g_Graph.m_pEdgeList[Array[Array.GetCount()-1]].NodeA
            
            || g_Graph.m_pEdgeList[m_iEndEdge].NodeA
            == g_Graph.m_pEdgeList[Array[Array.GetCount()-1]].NodeB
            
            || g_Graph.m_pEdgeList[m_iEndEdge].NodeB
            == g_Graph.m_pEdgeList[Array[Array.GetCount()-1]].NodeA
            
            || g_Graph.m_pEdgeList[m_iEndEdge].NodeB
            == g_Graph.m_pEdgeList[Array[Array.GetCount()-1]].NodeB );
        
        Array.Append( m_iEndEdge );
    }
}


//==============================================================================
    
xbool TestGraphFunc = FALSE;

u16 path_manager::Update( xarray<u16> &Array, xbool AllAtOnce /* = FALSE */ )
{
    ASSERT(m_Start != m_End);
    if (TestGraphFunc)
    {
        if ( g_Graph.IsIdenticalToFile(xfs( "%s/%s.gph", tgl.MissionDir, tgl.MissionName )))
            x_DebugMsg("***      Tested Graph Integrity:  OK        ***\n");
        else
            x_DebugMsg("***      Tested Graph Integrity:  BAD DATA! ***\n");
    }

    ASSERT( m_Initialized );
    
    static xtimer Timer;
    static s32 CurrentFrame = tgl.EventRenderCount;
    if (CurrentFrame != tgl.EventRenderCount)
        Timer.Reset();
    else
    {
        if (Timer.ReadMs() > MAX_TIME_SLICE && !AllAtOnce)
        {
            m_Flags |= BOT_PATH_FLAG_SEARCHING;
            return m_Start;
        }
    }
    graph::node *pStart, *pEnd, *pNeighbor, *pCurrNode;
    graph::edge* pCurrEdge;
    s16     CurrIndex, NeighIndex, EdgeIndex;
    u16 Cost = 0;
    f32 LocalTime = 0;

    Timer.Start();

    pStart = &g_Graph.m_pNodeList[m_Start];
    pEnd   = &g_Graph.m_pNodeList[m_End];

    while ((pCurrNode = (graph::node *)m_PriorityQueue.ExtractMinPtr()) != NULL)
    {
        if (pCurrNode == pEnd)
        {
            // We found our goal.  Fill out the xarray and return.
            Array.Clear();

            // Continue until we've hit the start.
            while (pCurrNode != pStart)
            {
                // Locate the next edge on the path to the start.
                EdgeIndex = pCurrNode->PlayerInfo[m_BotID].GetPrev();

                pCurrEdge = &g_Graph.m_pEdgeList[EdgeIndex];

                // Insert the node index into the start of the list, so
                // that the array comes out in order.
                Array.Insert(0, EdgeIndex);

                // Reassign current node to backtrack to the start.
                if (&g_Graph.m_pNodeList[pCurrEdge->NodeA] == pCurrNode)
                {
                    pCurrNode  = &g_Graph.m_pNodeList[pCurrEdge->NodeB];
                }
                else
                {
                    ASSERT (&g_Graph.m_pNodeList[pCurrEdge->NodeB] == pCurrNode
                        && "Error- Node's breadcrumb edge does not contain node.");
                    pCurrNode  = &g_Graph.m_pNodeList[pCurrEdge->NodeA];
                }
            }
            ASSERT (pCurrNode == pStart);

//                    x_DebugMsg("%d. Path found from Node %d to Node %d, along %d edges, Bot ID %d\n", m_GlobalCounter, 
//                        m_Start, m_End, Array.GetCount(), m_BotID);

            // We've completed our pathfind- turn off the flag and return.
            m_Flags &= ~BOT_PATH_FLAG_SEARCHING;

            if (m_bAppendWaiting)
            {
                Append(Array);
                m_bAppendWaiting = FALSE;
            }
     
            if (TriedAgain)
            {
                TriedAgain = FALSE;
                x_DebugMsg("***     FindPath() RECOVERED from Failure   ***\n");
                x_DebugMsg("***       by Resetting Visited flags!       ***\n");
                x_DebugMsg("***********************************************\n");
            }
#if defined (acheng)
    #if PATH_FIND_DEBUG
//                x_DebugMsg("*%d)FP() found path from node %d to node %d.\n", m_BotID, m_Start, m_End);
    #endif
#endif
            return m_Start;
        }

        s32 PrevEdgeAerial = 0;
        if (pCurrNode != pStart)
        {
            graph::edge PrevEdge = g_Graph.m_pEdgeList[pCurrNode->PlayerInfo[m_BotID].GetPrev()];
            if (PrevEdge.Type == graph::EDGE_TYPE_AEREAL // Prev edge aerial?
                && (PrevEdge.Flags & graph::EDGE_FLAGS_LOOSE)) // Prev edge loose?
            {
                if(&g_Graph.m_pNodeList[PrevEdge.NodeA] == pCurrNode) 
                {
                    PrevEdgeAerial = PrevEdge.CostFromB2A;
                }
                else
                {
                    PrevEdgeAerial = PrevEdge.CostFromA2B;
                }
            }
        }
        // Look at all the edges of this node.
        for (s32 i = pCurrNode->EdgeIndex; i < pCurrNode->EdgeIndex + pCurrNode->nEdges; i++)
        {
            // Find each neighbor.
            EdgeIndex = g_Graph.m_pNodeEdges[i];
            pCurrEdge = &g_Graph.m_pEdgeList[EdgeIndex];
            ASSERT(g_Graph.m_pNodeEdges[i] < g_Graph.m_nEdges && pCurrEdge);

            if (&g_Graph.m_pNodeList[pCurrEdge->NodeA] == pCurrNode)
            {
                CurrIndex  = pCurrEdge->NodeA;
                NeighIndex = pCurrEdge->NodeB;
                pNeighbor = &g_Graph.m_pNodeList[NeighIndex];
            }
            else
            {
                CurrIndex  = pCurrEdge->NodeB;
                NeighIndex = pCurrEdge->NodeA;
                pNeighbor = &g_Graph.m_pNodeList[NeighIndex];

                ASSERT(&g_Graph.m_pNodeList[CurrIndex] == pCurrNode && 
                                          "Node's Edge does not contain Node!");
            }

            // Determine the cost to travel to this neighbor from the start.
            // Assign the new cost to the neighbor.
            Cost = DetermineEdgeCost(EdgeIndex, CurrIndex, PrevEdgeAerial) + 
                           pCurrNode->PlayerInfo[m_BotID].GetCostSoFar();

            if (Cost >= (1<<12))
            {
#if defined (acheng)
                x_printf  ("**Path Manager: %s, Node %d->%d, at %d->%d, CostSoFar = %d!\n",
                    tgl.MissionName, m_Start, m_End, CurrIndex, NeighIndex, Cost);
                x_DebugMsg("**Path Manager: %s, Node %d->%d, at %d->%d, CostSoFar = %d!\n",
                    tgl.MissionName, m_Start, m_End, CurrIndex, NeighIndex, Cost);
#endif
            }

            // Examine the neighbor to see if already visited.
            if (pNeighbor->PlayerInfo[m_BotID].GetVisited() < m_GlobalCounter) 
                                                    // Unvisited until now.
            {
                pNeighbor->PlayerInfo[m_BotID].SetCostSoFar( Cost );

                // Determine the overall cost.
                Cost = pNeighbor->PlayerInfo[m_BotID].GetCostSoFar() + 
                                        DetermineCost(NeighIndex, m_End);
#if SHOW_EDGE_COSTS
                g_EdgeCost[EdgeIndex] = Cost;
#endif
                // Set the breadcrumb (for retrieving the full path).
                // prev = the index of the edge in the m_pEdgeList.

                pNeighbor->PlayerInfo[m_BotID].SetPrev(EdgeIndex);

                // Set our visited flag on.
                pNeighbor->PlayerInfo[m_BotID].SetVisited(m_GlobalCounter);

                // Insert this neighbor into the priority queue.
                if (!m_PriorityQueue.Insert(pNeighbor, Cost))
                {
                    // Need to quit now- Priority Queue is FULL.
                    TerminatePathFind(Array);
                    return m_Start;

                }

                ASSERT(pNeighbor == &g_Graph.m_pNodeList[NeighIndex] );
                ASSERT(pEnd ==      &g_Graph.m_pNodeList[m_End] );

            }

            // In case we've already added this one...
            else
            {
                ASSERTS(pNeighbor->PlayerInfo[m_BotID].GetVisited() == m_GlobalCounter,
                    "Invalid Data: graph::node::player_info::Visited");


                // Compare the current cost to the pre-existing one.
                if (Cost < pNeighbor->PlayerInfo[m_BotID].GetCostSoFar())
                {
//                       ASSERT(!"Visited Node Cost Improved Via Alternate Node");

                    // Though if it is the case, we have to recalculate all costs that
                    // were based on this one.  EEK!
                    pNeighbor->PlayerInfo[m_BotID].SetCostSoFar(Cost);

                    pNeighbor->PlayerInfo[m_BotID].SetPrev(EdgeIndex);
                    
                    // Have to update the cost in the priority queue, though
                    // currently there is no function to do that.
                    //
                    // Also any nodes that are based on this node need to be updated.

                    // Instead of searching the priority queue to find the node to update,
                    // the node is simply being reinserted into the queue.
                    if (!m_PriorityQueue.Insert(pNeighbor, 
                                            pNeighbor->PlayerInfo[m_BotID].GetCostSoFar() + 
                                                        DetermineCost(NeighIndex, m_End)))
                    {
                        // Need to stop now- priority queue is full!
                        TerminatePathFind(Array);
                        return m_Start;
                    }

                }
            }
        }   

        // Check the time since the start of the function.
        LocalTime = Timer.ReadMs();

        // Have we exceeded our allotted time?
        if (LocalTime > MAX_TIME_SLICE && !AllAtOnce)
        {
            m_TimeSegment      += LocalTime;
            Array.Clear();
            Timer.Stop();

            m_Flags |= BOT_PATH_FLAG_SEARCHING;
            return m_Start;
        }

    } // End while(...)
    Array.Clear();
    m_TimeComplete = m_TimeSegment + Timer.ReadMs();

    // No path found.

    x_DebugMsg("***********************************************\n");
    x_DebugMsg("***             FindPath() Failed           ***\n");
    x_DebugMsg("*** Start = %d,   End = %d               ****\n", m_Start, m_End);
    x_DebugMsg("***********************************************\n");

    // Examine Graph 
    x_DebugMsg("*****      Testing Graph Differences      *****\n");
    if ( g_Graph.IsIdenticalToFile(xfs( "%s/%s.gph", tgl.MissionDir, tgl.MissionName )))
        x_DebugMsg("***      Tested Graph Integrity:  OK        ***\n");
    else
    {
        x_DebugMsg("***      Tested Graph Integrity:  BAD DATA! ***\n");

        ASSERTS(FALSE, "Data in Graph file differs from data in memory!");
    }

    // Examine Priority queue
    x_DebugMsg("*****       Examining Priority Queue     ******\n");
    if (!m_PriorityQueue.IsEmpty())
    {
        x_DebugMsg(" Priority Queue Exited Prematurely!  non-empty or corrupted fib heap!\n");
        ASSERT(FALSE);
    }
    else
    {
        x_DebugMsg("***       Tested Priority Queue:  OK        ***\n");
    }

    ASSERT( g_Graph.DebugConnected() );

    if (!TriedAgain)
    {
        TriedAgain = TRUE;

        // Reset visited and retest.
        x_DebugMsg("***    Resetting visited and trying again   *** (GlobalCtr = %d)\n", m_GlobalCounter);
        ResetVisited(m_BotID);

        m_Flags |= BOT_PATH_FLAG_SEARCHING;
        return FindPath(Array, TRUE);
    }
    else
    {
        // Made it here, meaning findpath failed regardless of resetting visited flags.
        TriedAgain = FALSE;
        ASSERT(!"No Path found to destination."); // Big problem...
        m_Flags &= ~BOT_PATH_FLAG_SEARCHING;
        Timer.Stop();
    }
    m_Flags &= ~BOT_PATH_FLAG_SEARCHING;
    return m_Start;
}

//==============================================================================

u16 path_manager::DetermineCost(u16 NodeA, u16 NodeB)
{
    return (u16)(vector3(g_Graph.m_pNodeList[NodeA].Pos 
                           - g_Graph.m_pNodeList[NodeB].Pos).Length());
}

//==============================================================================

u16 path_manager::DetermineEdgeCost(u16 iEdge, u16 iStart, s32 PrevEdgeAerial)
{
    u16 Cost = 0;
    graph::edge Edge = g_Graph.m_pEdgeList[iEdge];
    if (iStart == Edge.NodeA)
    {
        Cost = Edge.CostFromA2B + ((Edge.Flags & m_TeamFlags) ? 200 : 0);
        if (Edge.Type == graph::EDGE_TYPE_AEREAL &&
            Edge.Flags & graph::EDGE_FLAGS_LOOSE &&
            g_Graph.m_pNodeList[Edge.NodeA].Pos.Y < 
                            g_Graph.m_pNodeList[Edge.NodeB].Pos.Y )
        {
            if ((Edge.CostFromA2B + PrevEdgeAerial) > m_MaxAirDist)
                Cost += 200;
        }
    }
    else 
    {
        Cost = Edge.CostFromB2A + 
            ((Edge.Flags & m_TeamFlags) ? 200 : 0);

        if (Edge.Type == graph::EDGE_TYPE_AEREAL &&
            Edge.Flags & graph::EDGE_FLAGS_LOOSE &&
            g_Graph.m_pNodeList[Edge.NodeB].Pos.Y < 
                            g_Graph.m_pNodeList[Edge.NodeA].Pos.Y )
        {
            if ((Edge.CostFromB2A + PrevEdgeAerial) > m_MaxAirDist)
                Cost += 200;
        }

        ASSERT(Edge.NodeB == iStart);
    }
    ASSERT(m_MaxAirDist > 15 && m_MaxAirDist <= 200);

    return Cost;
}

//==============================================================================

static
s32 CompareEdges( f32* A, f32* B )
{
    if( *A < *B ) return -1;
    return *A > *B;
}

//==============================================================================

xtimer GetClosestEdgeTime[3];
#define DETAILED_TIMING_GCET 0
xtimer GCET[10];
#if DETAILED_TIMING_GCET
void DisplayTimeStats()
{
    static f32 MIN_TIME_DISPLAY = 5.0f;
    if (GetClosestEdgeTime[0].ReadMs() > MIN_TIME_DISPLAY)
    {
        // Display breakdown detail.
// #################################################
        // TO DO
        x_DebugMsg("%s\nGCET: Total Fnc time: %5.2f\nInit     %5.2f   %2d   %5.2f\nWhile    %5.2f   %2d   %5.2f\nSetup    %5.2f   %2d   %5.2f\nCell     %5.2f   %2d   %5.2f\nMemmove  %5.2f   %2d   %5.2f\nEdge     %5.2f   %2d   %5.2f\nCollider %5.2f   %2d   %5.2f\nEnd      %5.2f   %2d   %5.2f\n1ST HALF %5.2f   %2d   %5.2f\n2ND HALF %5.2f   %2d   %5.2f\n%s\n",
                "##########################################################",
                GetClosestEdgeTime[0].ReadMs(),
                GCET[0].ReadMs(),
                GCET[0].GetNSamples(),
                GCET[0].GetAverageMs(),

                GCET[1].ReadMs(),
                GCET[1].GetNSamples(),
                GCET[1].GetAverageMs(),

                GCET[6].ReadMs(),
                GCET[6].GetNSamples(),
                GCET[6].GetAverageMs(),

                GCET[2].ReadMs(),
                GCET[2].GetNSamples(),
                GCET[2].GetAverageMs(),

                GCET[3].ReadMs(),
                GCET[3].GetNSamples(),
                GCET[3].GetAverageMs(),

                GCET[4].ReadMs(),
                GCET[4].GetNSamples(),
                GCET[4].GetAverageMs(),

                GCET[5].ReadMs(),
                GCET[5].GetNSamples(),
                GCET[5].GetAverageMs(),

                // End
                GCET[7].ReadMs(),
                GCET[7].GetNSamples(),
                GCET[7].GetAverageMs(),

                // 1st half
                GCET[8].ReadMs(),
                GCET[8].GetNSamples(),
                GCET[8].GetAverageMs(),

                // 2nd half
                GCET[9].ReadMs(),
                GCET[9].GetNSamples(),
                GCET[9].GetAverageMs(),

                "##########################################################");
    }
    GetClosestEdgeTime[0].Reset();
    GCET[0].Reset();
    GCET[1].Reset();
    GCET[2].Reset();
    GCET[3].Reset();
    GCET[4].Reset();
    GCET[5].Reset();
    GCET[6].Reset();
    GCET[7].Reset();
    GCET[8].Reset();
    GCET[9].Reset();
}
#endif
//==============================================================================
s16 path_manager::GetClosestEdge( const vector3& Pos )
{
#if DETAILED_TIMING_GCET
    GCET[0].Start();
#endif
    GetClosestEdgeTime[0].Start();
    xbool Indoor = PointInsideBuilding(Pos);


//  s32 InfiniteLoop = 900;
    s32 EdgesChecked = 0;

#if MARK_COSTLY_GCET
    static s32 MaxRayChecks = 0;
#endif
    s32 nRayChecks = 0;
    s32 i;
    static const vector3 CellSize(g_Graph.m_Grid.CellWidth,
                                    0,
                                 g_Graph.m_Grid.CellDepth);

    // visited flags.
    static u32 CellVisited[32];
    static u32 EdgeVisited[65];

    x_memset(CellVisited, 0, sizeof(CellVisited));
    x_memset(EdgeVisited, 0, sizeof(EdgeVisited));
    struct edge_sort
    {
        f32     Distance;
        s32     Index;
        vector3 V;
    };

    // Set up the initial search bbox.
    bbox ScanBox(Pos - CellSize/4, Pos + CellSize/4);
    ScanBox.Min.Y = g_Graph.m_Grid.Min.Y;
    ScanBox.Max.Y = g_Graph.m_Grid.Max.Y;

    // The current search cell.
    s32 CurrCellX, CurrCellZ;   // Grid coordinates of cell
    graph::grid_cell *CellData; // The actual cell data.


    // Prepare the edge list.

#define MAX_EDGE_LIST_SIZE 90
    edge_sort EdgeList[MAX_EDGE_LIST_SIZE];
    EdgeList[0].Index = -1;
    edge_sort BestEdge;
    xbool     BestEdgeChanged = FALSE;
    f32       DistToBest = -1;   // Actual distance to best edge (not squared).
    BestEdge.Distance = -1.0f;
    BestEdge.Index = -1;

    edge_sort ClosestEdge;
    ClosestEdge.Index = -1;
    ClosestEdge.Distance = -1.0f;
    edge_sort   CurrEdge;

    s32 k = 0;
    s32 nEdges = 0;
    collider            Ray;

    s32 CurrEdgeIdx;
    s32 StartX, EndX, StartZ, EndZ;

#if DETAILED_TIMING_GCET
    GCET[0].Stop();
#endif
    StartX = g_Graph.GetColumn(ScanBox.Min.X);
    EndX   = g_Graph.GetColumn(ScanBox.Max.X);
    StartZ = g_Graph.GetRow(ScanBox.Min.Z);
    EndZ   = g_Graph.GetRow(ScanBox.Max.Z);
    while (BestEdge.Index == -1 || BestEdgeChanged)
    {
        BestEdgeChanged = FALSE;
#if DETAILED_TIMING_GCET
        GCET[1].Start();
        GCET[8].Start();
#endif
        // Loop through every cell in our scan area.
        for (CurrCellX = StartX; CurrCellX <= EndX; CurrCellX++)
        {
            for (CurrCellZ = StartZ; CurrCellZ <= EndZ; CurrCellZ++)
            {
//                ASSERT(--InfiniteLoop > 0);
#if DETAILED_TIMING_GCET
                GCET[6].Start();
#endif
                // Check if we've already scanned this cell.
                if (CellVisited[CurrCellX] & (1<<CurrCellZ))
                {
                    // already been here.
                    continue;
                }
#if DETAILED_TIMING_GCET
                GCET[6].Stop();
                GCET[2].Start();
#endif
                // Mark this cell as visited.
                CellVisited[CurrCellX] |= (1<<CurrCellZ);

                // Set up current cell pointer.
                CellData = &g_Graph.m_Grid.Cell[CurrCellX][CurrCellZ];

                // Add the edges of this cell to the search.
                for (i = CellData->EdgeOffset; i < CellData->EdgeOffset + CellData->nEdges; i++)
                {
                    CurrEdgeIdx = g_Graph.m_Grid.GridEdgesList[i];

                    // Set the first one just in case we're desperate enough to need it.
                    if (EdgeList[0].Index == -1)
                        EdgeList[0].Index = CurrEdgeIdx;

                    // go on to the next one if we've already tested this edge.
                    if (EdgeVisited[CurrEdgeIdx/32] & (1<<(CurrEdgeIdx%32)))
                    {
#if DETAILED_TIMING_GCET
                        GCET[2].Stop();
#endif
                        continue;
                    }
#if DETAILED_TIMING_GCET
                    GCET[4].Start();
#endif

                    // Set as visited.
                    EdgeVisited[CurrEdgeIdx/32] |= (1<<(CurrEdgeIdx%32));
static xbool QuitOnNEdges = TRUE;
                    if (QuitOnNEdges)
                    if (EdgesChecked++ > MIN(200, EDGE_CHECK_PERCENTAGE*g_Graph.m_nEdges))
                    {
                        if(ClosestEdge.Index == -1)
                        {
                            GetClosestEdgeTime[0].Stop();
#if DETAILED_TIMING_GCET
                            GCET[1].Stop();
                            GCET[2].Stop();
                            GCET[4].Stop();
                            GCET[8].Stop();
                            DisplayTimeStats();
#endif
                            ASSERT(EdgeList[0].Index >= 0 && EdgeList[0].Index < g_Graph.m_nEdges);
                            return EdgeList[0].Index;
                        }
                        GetClosestEdgeTime[0].Stop();
                        ASSERT(ClosestEdge.Index >= 0 && ClosestEdge.Index < g_Graph.m_nEdges);
#if DETAILED_TIMING_GCET
                        GCET[1].Stop();
                        GCET[2].Stop();
                        GCET[4].Stop();
                        GCET[8].Stop();
                        DisplayTimeStats();
#endif
                        return ClosestEdge.Index;
                    }

                    const graph::edge&  Edge = g_Graph.m_pEdgeList[CurrEdgeIdx];

                    // If indoor, only consider indoor edges.
                    //if (Indoor && !(Edge.Flags & graph::EDGE_FLAGS_INDOOR))
                    //    continue;
                    // if outdoor, don't consider indoor edges.
                    if (!Indoor && (Edge.Flags & graph::EDGE_FLAGS_INDOOR))
                    {
#if DETAILED_TIMING_GCET
                        GCET[2].Stop();
                        GCET[4].Stop();
#endif
                        continue;
                    }

                    u32 TeamFlags = (m_TeamBits == 1) ?  graph::EDGE_FLAGS_TEAM_0_BLOCK 
                        : ( m_TeamBits == 2 ) ? graph::EDGE_FLAGS_TEAM_1_BLOCK
                        : graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK;
                    if (Edge.Flags & TeamFlags)
                    {
                        // This edge is blocked.
                        continue;
                    }

                    // Get the edge stats.
                    CurrEdge.V        = Pos.GetClosestVToLSeg( g_Graph.m_pNodeList[ Edge.NodeA ].Pos, g_Graph.m_pNodeList[ Edge.NodeB ].Pos );
                    CurrEdge.Distance = CurrEdge.V.LengthSquared();
                    CurrEdge.Index    = CurrEdgeIdx;

                    // Already got something to compare this one to?
                    if ((BestEdge.Index != -1) 
                        && CurrEdge.Distance >= BestEdge.Distance)
                    {
                        // Don't bother adding any edges that are no better than our current best.
#if DETAILED_TIMING_GCET
                        GCET[2].Stop();
                        GCET[4].Stop();
#endif
                        continue;
                    }

                    // Exit now if we can with a really close edge.
                    if( (CurrEdge.Distance < 0.1f) )

                        // Let's disregard blocked edges for now.
                        //&& 
                        //(!(g_Graph.m_pEdgeList[pSort[nEdges].Index].Flags & 
                        //    (graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK))) )
                    {
//                        x_DebugMsg("Ray Checks: %d\n", nRayChecks);
                        GetClosestEdgeTime[0].Stop();
#if DETAILED_TIMING_GCET
                        GCET[1].Stop();
                        GCET[2].Stop();
                        GCET[4].Stop();
                        GCET[8].Stop();
                        DisplayTimeStats();
#endif
                        ASSERT(CurrEdgeIdx >= 0 && CurrEdgeIdx < g_Graph.m_nEdges);
                        return (s16)CurrEdgeIdx;
                    }

static s32    MAX_CHECKS_PER_CELL = 25;
                    // Only keep track of the top N closest edges, & choose among them.
                    if (nEdges > MAX_CHECKS_PER_CELL
                        && CurrEdge.Distance > EdgeList[MAX_CHECKS_PER_CELL].Distance)
                    {
                        // already have n edges closer than this one.
#if DETAILED_TIMING_GCET
                        GCET[2].Stop();
                        GCET[4].Stop();
#endif
                        continue;
                    }

                    // Find the spot at which to add the edge into our list.
                    for (k = 0; k < nEdges; k++)
                    {
                        if (CurrEdge.Distance < EdgeList[k].Distance)
                        {
                            // Found our spot.
#if DETAILED_TIMING_GCET
                            GCET[2].Stop();
                            GCET[4].Stop();
#endif
                            break;
                        }
                    }

                    // Insert the edge, shove the rest of 'em over one.
#if DETAILED_TIMING_GCET
                    GCET[3].Start();
#endif
                    x_memmove(&EdgeList[k+1], &EdgeList[k], (sizeof(edge_sort))*(nEdges - k));
#if DETAILED_TIMING_GCET
                    GCET[3].Stop();
#endif
                    EdgeList[k].Distance = CurrEdge.Distance;
                    EdgeList[k].Index    = CurrEdge.Index;
                    EdgeList[k].V        = CurrEdge.V;

                    // Set the new count.
                    if (nEdges < MAX_EDGE_LIST_SIZE-1)
                        nEdges++;

#if DETAILED_TIMING_GCET
                    GCET[4].Stop();
#endif
                }// end edge loop through this cell.

                // Failsafe exit, in emergency situations only- went through the whole grid and there's no closest edge!
                // or every edge is blocked by a building in every direction!
#if 0
                if (ScanBox.Min.X <= g_Graph.m_Grid.Min.X
                    && ScanBox.Max.X >= g_Graph.m_Grid.Max.X
                    && ScanBox.Min.Z <= g_Graph.m_Grid.Min.Z
                    && ScanBox.Max.Z >= g_Graph.m_Grid.Max.Z
                    && BestEdge.Index == -1)
                    // we're in some trouble now.
                {
                    xbool VisitedAllCells = TRUE;
                    for (i = 0; i < 32; i++)
                    {
                        if (CellVisited[i] == (u32)(0xffffffff))
                            continue;
                        else
                        {
                            VisitedAllCells = FALSE;
                            break;
                        }
                    }

                    if (VisitedAllCells)
                    {
                        // No edge was found that passed the line of sight check against buildings.
                        ASSERTS(0, "Could not find a closest visible edge!");

                        ASSERT ( ClosestEdge.Index != -1 );  // okay this is ridiculous- the graph must be empty!
                        GetClosestEdgeTime[0].Stop();
                        GCET[8].Stop();
                        ASSERT(ClosestEdge.Index >= 0 && ClosestEdge.Index < g_Graph.m_nEdges);
                        return ClosestEdge.Index;
                    }
                }
#endif
                /*
                // Perhaps we can shrink our scan size?
                if (BestEdgeChanged)
                {
                    // Get the actual distance.
                    DistToBest = x_sqrt(BestEdge.Distance);
                    ScanBox.Min.X = Pos.X - DistToBest;
                    ScanBox.Min.Z = Pos.Z - DistToBest;
                    ScanBox.Max.X = Pos.X + DistToBest;
                    ScanBox.Max.Z = Pos.Z + DistToBest;

                    // Reset the loop to start over.  
                    CurrCellX = g_Graph.GetColumn(ScanBox.Min.X);
                    CurrCellZ = g_Graph.GetRow(ScanBox.Min.Z) - 1; // (incremented on next iteration)
                    BestEdgeChanged = FALSE;
                }
                // Continue onto our next cell.
*/
#if DETAILED_TIMING_GCET
                GCET[2].Stop();
#endif
            }
        }// END FULL SCAN OF ALL CELLS WITHIN OUR SCAN BOX.
#if DETAILED_TIMING_GCET
        GCET[8].Stop();
        GCET[9].Start();
#endif
        // Find the best edge within our scan box.
        // Scan the current list for a good edge.
        for (k = 0; k < nEdges; k++)
        {
#if DETAILED_TIMING_GCET
            GCET[5].Start();
#endif
#define MAX_RAY_CHECKS_BEFORE_QUIT 25
            if (nRayChecks > MAX_RAY_CHECKS_BEFORE_QUIT)
            {
#if MARK_COSTLY_GCET
                s_PathEditor.AddMarker(Pos, nRayChecks);
#endif
                nRayChecks = 0;
                break;
            }
            // We either don't have a best edge yet or all these edges are closer... RIGHT?
            ASSERT(BestEdge.Index == -1 || EdgeList[k].Distance < BestEdge.Distance);

            nRayChecks++;
            
            // Test for visibility.
            Ray.RaySetup    ( NULL, Pos + EdgeList[k].V, Pos, -1, TRUE);
            ObjMgr.Collide  ( object::ATTR_PERMANENT, Ray );

#if mreed
            collider OtherDirRay;
            OtherDirRay.RaySetup( NULL, Pos, Pos + EdgeList[k].V, -1, TRUE);
            ObjMgr.Collide  ( object::ATTR_PERMANENT, OtherDirRay );

            if ( Ray.HasCollided() != OtherDirRay.HasCollided() )
            {
                const vector3 EdgePos = Pos + EdgeList[k].V;
                // Hmmm, hopefully this is because the edge goes underground
                bot_log::Log( "GetClosestEdge: ray check varies by direction\n" );
                bot_log::Log( "   Pos( %5.2f, %5.2f, %5.2f )  EdgePos( %5.2f, %5.2f, %5.2f )\n", Pos.X, Pos.Y, Pos.Z, EdgePos.X, EdgePos.Y, EdgePos.Z );
                bot_log::Log( "   To Pos: %s   To Edge: %s\n", Ray.HasCollided() ? "Collided" : "Clear", OtherDirRay.HasCollided() ? "Collided" : "Clear" );
                bot_log::Log( "\n" );
            }
#endif

            // *********************************************
            // Check for collision with force field as well?
            // *********************************************

            if( Ray.HasCollided() == FALSE )
            {
                // GOOD EDGE!  
                // Since the edge list is already sorted by distance, the first edge
                // we find must be the best edge in the entire list.
                BestEdge.Distance = EdgeList[k].Distance;
                BestEdge.Index    = EdgeList[k].Index;
                BestEdge.V        = EdgeList[k].V;

                DistToBest = x_sqrt(BestEdge.Distance);
                BestEdgeChanged = TRUE;
                ScanBox.Min.X = Pos.X - DistToBest;
                ScanBox.Min.Z = Pos.Z - DistToBest;
                ScanBox.Max.X = Pos.X + DistToBest;
                ScanBox.Max.Z = Pos.Z + DistToBest;
#if DETAILED_TIMING_GCET
                GCET[5].Stop();
#endif
                break;
            }
#if DETAILED_TIMING_GCET
            GCET[5].Stop();
#endif
        }
#if DETAILED_TIMING_GCET
        GCET[7].Start();
#endif
        // Chuck the edges in our list.  We either got the best of the bunch, or
        // none of them are any good anyway.
        // Store the closest one just in case we don't have any other choice.
        if (nEdges && 
            (ClosestEdge.Index == -1 || EdgeList[0].Distance < ClosestEdge.Distance))
        {
            ClosestEdge.Distance = EdgeList[0].Distance;
            ClosestEdge.Index = EdgeList[0].Index;
            ClosestEdge.V = EdgeList[0].V;
            ASSERT(ClosestEdge.Index >= 0 && ClosestEdge.Index < g_Graph.m_nEdges);
            ASSERT(ClosestEdge.Distance > 0.0f);
        }
        nEdges = 0;

        // Now, adjust our scan box larger if we haven't found anything, or smaller if we have.
        if (BestEdge.Index == -1)
        {
            // Haven't found anything yet.  Increase our scan area.
        StartX = MAX(StartX - 1, 0);
        EndX   = MIN(EndX + 1,   31);
        StartZ = MAX(StartZ - 1, 0);
        EndZ   = MIN(EndZ + 1,   31);
/*            ScanBox.Min -= CellSize;
            ScanBox.Max += CellSize;
            ScanBox.Min = vector3(MAX(ScanBox.Min.X - CellSize.X, g_Graph.m_Grid.Min.X),
                                  ScanBox.Min.Y,
                                  MAX(ScanBox.Min.Z - CellSize.Z, g_Graph.m_Grid.Min.Z));
            ScanBox.Max = vector3(MIN(ScanBox.Max.X - CellSize.X, g_Graph.m_Grid.Max.X),
                                  ScanBox.Max.Y,
                                  MIN(ScanBox.Max.Z - CellSize.Z, g_Graph.m_Grid.Max.Z));
*/
        }
#if 0&& defined (acheng)
        else if (!BestEdgeChanged)
        {
            // About to exit the while loop.
            // Make sure we scanned all the cells in our scan box.
//            ASSERT(x_sqr(DistToBest) - BestEdge.Distance < 0.1f);
            for (i = g_Graph.GetColumn(ScanBox.Min.X); i <= g_Graph.GetColumn(ScanBox.Max.X); i++)
                for (j = g_Graph.GetRow(ScanBox.Min.Z); j <= g_Graph.GetRow(ScanBox.Max.Z); j++)
                {
                    ASSERT(CellVisited[i] & (1<<j));
                }
        }
#endif
#if DETAILED_TIMING_GCET
        GCET[9].Stop();
        GCET[7].Stop();
        GCET[1].Stop();
#endif
    }
#if MARK_COSTLY_GCET
    if (nRayChecks > MaxRayChecks)
    {
        MaxRayChecks = nRayChecks;
        x_printf("Hit %d Ray Casts in one call\n", MaxRayChecks);
    }
    if (nRayChecks > 5 )
    {
        s_PathEditor.AddMarker(Pos, nRayChecks);
    }
#endif
#if defined acheng
//    x_DebugMsg("Ray Checks: %d, Max: %d\n", nRayChecks, MaxRayChecks);
#endif

    GetClosestEdgeTime[0].Stop();
//################################################
    ASSERT(BestEdge.Index >= 0 && BestEdge.Index < g_Graph.m_nEdges);
    
#if DETAILED_TIMING_GCET
     DisplayTimeStats();
#endif
    return BestEdge.Index;
}

//==============================================================================

xbool path_manager::PointInsideBuilding( const vector3& Point ) 
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

// Test find path using node indices, or using points in the world?
#define STRESS_TEST_USING_NODE_INDICES 1 
xbool ShowBreakdown = 0;
xbool path_manager::RunStressTest( void )
{    
    s32 BlockedEdge;
    xarray<u16> Path;
    xarray<s32> Children;

    xbool ShowBlocked = 0;
    xbool ShowTimer = 1;

#if (STRESS_TEST_USING_NODE_INDICES)
    static s32 BlockFrequency = 2000;
#else
    static s32 BlockFrequency = 1000;
#endif

    static xtimer StressTimer;
    static s32 LastSec;    

    static s32 PathsFound = 0;
    static s32 PathsFoundPerBot[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

    if (!StressTimer.IsRunning())
    {
        StressTimer.Start();
        LastSec = (s32)StressTimer.ReadSec();
        ResetAllVisited();
    }
    s32 nBlocked;


    // Block an edge.
    if (x_rand()%BlockFrequency == 0)
    {
        BlockedEdge = x_rand()%g_Graph.m_nEdges;
        // Block a random edge.
        g_Graph.BlockEdge(BlockedEdge, graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK);

        if (ShowBlocked)
            x_DebugMsg("Blocked edge #%d  \n", BlockedEdge);
    }

#if (STRESS_TEST_USING_NODE_INDICES)
    do 
    {
        m_Start = x_rand()%g_Graph.m_nNodes;
        m_End =   x_rand()%g_Graph.m_nNodes;
    }
    while (m_Start == m_End);

    FindPath( Path, TRUE );
#else
    vector3 SourcePos;
    vector3 DestPos;

    // Find a random start and end vector.
    {
        //BOUNDS        1088.0000    -2048.0000      256.0000
        //              1824.0000     2048.0000     1728.0000
        SourcePos = vector3(x_frand(1200.0, 1770.0),500, x_frand(330.0, 890.0));
        DestPos =   vector3(x_frand(1200.0, 1770.0),500, x_frand(330.0, 890.0));

        // Get the appropriate height value.
        // Fire ray straight down
        vector3 Dir(0.0f, -1000.0f, 0.0f);

        vector3 Pos = SourcePos;

        collider  Ray;
        Ray.RaySetup( NULL, Pos, Pos+Dir );
        ObjMgr.Collide( object::ATTR_SOLID, Ray );

        vector3 Pos2 = Pos + Ray.GetCollisionT()*Dir;

        SourcePos.Y = Pos2.Y;
        
        Pos = DestPos;
        Ray.RaySetup( NULL, Pos, Pos+Dir );
        ObjMgr.Collide( object::ATTR_SOLID, Ray );

        Pos2 = Pos + Ray.GetCollisionT()*Dir;
        DestPos.Y = Pos2.Y;

    }

    FindPath(Path, SourcePos, DestPos, graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK, TRUE );

#endif

    ASSERT( !Searching() );
    if ( Path.GetCount() <= 0 )
        {
            ASSERT ( g_Graph.DebugConnected() );
            ASSERT (FALSE);
            return FALSE;
        }
    else
    {
        PathsFound++;
        PathsFoundPerBot[m_BotID]++;

        nBlocked =g_Graph.GetNumBlockedEdges();
        if (ShowTimer && (s32)StressTimer.ReadSec() != LastSec)
        {
            x_DebugMsg(
                "Running for %d seconds, %d Paths found, %d edges blocked (%2.2f %%)\n", 
                (s32)StressTimer.ReadSec(), 
                (s32)PathsFound,
                nBlocked,
                (((f32)nBlocked * 100.0f)/((f32)g_Graph.m_nEdges)));
            LastSec = (s32)StressTimer.ReadSec();

            if (ShowBreakdown)
            {
                x_DebugMsg(" PerBot:\n1)%d 2)%d 3)%d 4)%d 5)%d 6)%d 7)%d 8)%d 9)%d 10)%d 11)%d 12)%d 13)%d 14)%d 15)%d 16)%d\n",
                    PathsFoundPerBot[0],
                    PathsFoundPerBot[1],
                    PathsFoundPerBot[2],
                    PathsFoundPerBot[3],
                    PathsFoundPerBot[4],
                    PathsFoundPerBot[5],
                    PathsFoundPerBot[6],
                    PathsFoundPerBot[7],
                    PathsFoundPerBot[8],
                    PathsFoundPerBot[9],
                    PathsFoundPerBot[10],
                    PathsFoundPerBot[11],
                    PathsFoundPerBot[12],
                    PathsFoundPerBot[13],
                    PathsFoundPerBot[14],
                    PathsFoundPerBot[15]);
            }


        }
        return TRUE;
    }
}


//==============================================================================

xbool ExtrusionClearBetweenPoints( const vector3& Position1, 
                                   const vector3& Position2 ) 
{
    if( Position1 == Position2 )
        return( TRUE );

    if( (Position1 - Position2).LengthSquared() < 0.001f )
        return( TRUE );    

    collider  Ray;
    Ray.RaySetup( NULL, Position1, Position2, -1, TRUE );
    ObjMgr.Collide( object::ATTR_BUILDING | 
        object::ATTR_FORCE_FIELD | object::ATTR_PERMANENT, Ray );

    if( Ray.HasCollided() )
        return FALSE;
    else
        return TRUE;
}

//==============================================================================

static s32 FindFreeSlot( void )
{
    for( s32 i=0; i<MAX_BOTS; i++ )
    {
        if( !s_SlotFilled[i] )
        {
            s_SlotFilled[i] = TRUE;
            return( i );
        }
    }

    // there must be no free slots!

    DEMAND( FALSE );
    return( -1 );
}

//==============================================================================

void path_manager::FreeSlot( void )
{
    s_SlotFilled[m_BotID] = FALSE;
}

//==============================================================================
xtimer OldGetClosestEdgeTime[3];
//==============================================================================
s16 path_manager::OldGetClosestEdge( const vector3& Pos )
{
    GetClosestEdgeTime[1].Start();

#if 0
    {
        #define MAX_SORT_EDGES  10
        struct edge_sort
        {
            f32     Distance;
            s32     Index;
            vector3 V;
        };

        edge_sort   Edge[ MAX_SORT_EDGES ];
        f32         EdgeMax;
        s32         WorstEdgeIndex=-1;
        collider    Ray;
        s32         i,j;

        // Clear edges
        EdgeMax = F32_MAX;
        for( i=0; i<MAX_SORT_EDGES; i++ )
        {
            Edge[i].Distance = F32_MAX;
            Edge[i].Index    = -1;
        }

        // Loop through all edges
        for( i=0; i<g_Graph.m_nEdges; i++ )
        {        
            // Check if not blocked
            //if( !(g_Graph.m_pEdgeList[i].Flags & (graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK)) )
            {
                const graph::edge&  GEdge = g_Graph.m_pEdgeList[i];

                // Get closest point and distance to edge
                vector3 V = Pos.GetClosestVToLSeg( g_Graph.m_pNodeList[ GEdge.NodeA ].Pos, g_Graph.m_pNodeList[ GEdge.NodeB ].Pos );
                f32     D = V.LengthSquared();

                // Check if close enough for a trivial accept ray check
                if( D < 0.5f*0.5f )
                {
                    xbool Keep = TRUE;
                    
                    // Do ray check if not trivially close enough
                    if( D > 0.1f )
                    {
                        Ray.RaySetup    ( NULL, Pos+vector3(0.0f,0.75f,0.0f), Pos + V + vector3(0.0f,0.75f,0.0f) );
                        ObjMgr.Collide  ( object::ATTR_BUILDING, Ray );
                        Keep = (Ray.HasCollided() == FALSE);
                    }

                    if( Keep ) 
                    {
                        GetClosestEdgeTime[1].Stop();
                        return i;
                    }
                    //else
                    //    continue;
                }

                // Check if dist is within max of current list
                if( D <= EdgeMax )
                {
                    if( WorstEdgeIndex == -1 )
                    {
                        WorstEdgeIndex = 0;

                        // Find worst edge
                        for( j=1; j<MAX_SORT_EDGES; j++ )
                        if( Edge[j].Distance > Edge[WorstEdgeIndex].Distance )
                            WorstEdgeIndex = j;

                        EdgeMax = Edge[WorstEdgeIndex].Distance;
                    }

                    // Check if we should replace 
                    if( Edge[WorstEdgeIndex].Distance > D )
                    {
                        Edge[WorstEdgeIndex].Distance = D;
                        Edge[WorstEdgeIndex].V = V;
                        Edge[WorstEdgeIndex].Index = i;
                        WorstEdgeIndex = -1;
                    }
                }
            }
        }

        // Check final set for clear rays.
        for( i=0; i<MAX_SORT_EDGES; i++ )
        if( Edge[i].Index != -1 )
        {
            Ray.RaySetup    ( NULL, Pos + vector3(0.0f,0.75f,0.0f), Pos + Edge[i].V + vector3(0.0f,0.75f,0.0f) );
            ObjMgr.Collide  ( object::ATTR_BUILDING, Ray );
            if( Ray.HasCollided() == FALSE ) 
            {
                GetClosestEdgeTime[1].Stop();
                return Edge[i].Index;
            }
        }

        // Oh No!!!! it wasn't in our final set!!!
        // Do the old find closest edge...ewwwwww
        x_DebugMsg("USING OLD GET CLOSEST EDGE!!!\n");
        x_DebugMsg("%10.5f %10.5f %10.5f\n",Pos.X,Pos.Y,Pos.Z);
        
        for( i=0; i<MAX_SORT_EDGES; i++ )
        if( Edge[i].Index != -1 )
        {
            vector3 P = Edge[i].V+Pos;
            x_DebugMsg("%10.5f %10.5f %10.5f   %f\n",P.X,P.Y,P.Z,x_sqrt(Edge[i].Distance));
        }
    }
#endif


    {
        struct edge_sort
        {
            f32     Distance;
            s32     Index;
            vector3 V;
        };

        xbool       bInsideBuilding = PointInsideBuilding( Pos );
        s32         nEdges          = 0;
        edge_sort*  pSort           = NULL;
        s32         i;
        edge_sort   Best;

        Best.Distance = F32_MAX;

        //
        // Collect all possible edges
        //
        OldGetClosestEdgeTime[1].Start();
        {    
            f32  Distance = x_sqr( bInsideBuilding ? 50.0f : 500.0f );

            smem_StackPushMarker();
            pSort    = (edge_sort*)smem_StackAlloc( sizeof(edge_sort)*g_Graph.m_nEdges );
            ASSERT( pSort );

            // Consider all edges...
            for( i=0; i<g_Graph.m_nEdges; i++ )
            {        
                const graph::edge&  Edge = g_Graph.m_pEdgeList[i];

                pSort[nEdges].V        = Pos.GetClosestVToLSeg( g_Graph.m_pNodeList[ Edge.NodeA ].Pos, g_Graph.m_pNodeList[ Edge.NodeB ].Pos );
                pSort[nEdges].Distance = pSort[nEdges].V.LengthSquared();
                pSort[nEdges].Index    = i;

                if( (pSort[nEdges].Distance < 0.1f) &&
                    (!(g_Graph.m_pEdgeList[pSort[nEdges].Index].Flags & 
                        (graph::EDGE_FLAGS_TEAM_0_BLOCK | graph::EDGE_FLAGS_TEAM_1_BLOCK))) )
                {
                    smem_StackPopToMarker();//delete []pSort;
                    GetClosestEdgeTime[1].Stop();
                    OldGetClosestEdgeTime[1].Stop();
                    return i;
                }

                if( pSort[nEdges].Distance < Best.Distance )
                {
                    Best = pSort[nEdges];
                }

                if( pSort[nEdges].Distance < Distance ) 
                    nEdges++;
            }

            if( nEdges == 0 )
            {
                nEdges = 1;
                pSort[0] = Best;
                x_DebugMsg( ">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n" );
            }
            else
            {
                //ASSERT( nEdges > 0 ); // We need to have at least one
                x_qsort( pSort, nEdges, sizeof(edge_sort), (compare_fn*)CompareEdges );
            }
        }
        OldGetClosestEdgeTime[1].Stop();

        //
        // Find the candidate edge
        //
        OldGetClosestEdgeTime[2].Start();
        if( FALSE && bInsideBuilding )
        {
            ASSERTS( 0, "Not yet implemented" );
        }
        else
        {
            for( i=0; i<nEdges; i++ )
            {
                if ( g_Graph.m_pEdgeList[pSort[i].Index].Flags
                    & (graph::EDGE_FLAGS_TEAM_0_BLOCK
                        | graph::EDGE_FLAGS_TEAM_1_BLOCK) )
                {
                    // TODO: this blocks all for both teams always -- fix...
                    // Edge is blocked
                    continue;
                }
            
                collider            Ray;

                Ray.RaySetup    ( NULL, Pos, Pos + pSort[i].V );
                ObjMgr.Collide  ( object::ATTR_BUILDING, Ray );

                if( Ray.HasCollided() == FALSE ) break;
            }

            if ( i == nEdges )
            {
                // No edge found in previous loop. Ignore collisions,
                // and just pick the nearest edge.
                i = 0; 
            }

            s16 Index = pSort[i].Index;
            ASSERT( Index >= 0 );
            ASSERT( Index < g_Graph.m_nEdges );
            smem_StackPopToMarker();//delete []pSort;
            OldGetClosestEdgeTime[2].Stop();
            GetClosestEdgeTime[1].Stop();
//            x_DebugMsg("%4d %4d %f\n",i,Index,x_sqrt(pSort[i].Distance));
            return Index;
        }
        OldGetClosestEdgeTime[2].Stop();

    }

    ASSERT( FALSE );
    GetClosestEdgeTime[1].Stop();
    return 0;
}

//==============================================================================

void path_manager::TerminatePathFind ( xarray<u16>& Array )
{
    // Find the closest leaf node to our goal.
    graph::node* Closest, *pCurrNode, *pStart;
    pStart = &g_Graph.m_pNodeList[m_Start];
    s16     EdgeIndex;
    graph::edge* pCurrEdge;

    Closest = (graph::node *)m_PriorityQueue.ExtractMinPtr();
    vector3 EndPos = g_Graph.m_pNodeList[m_End].Pos;
    f32 DistSq = (Closest->Pos - EndPos).LengthSquared();
    ASSERT(Closest);

    f32 Dist;

    // This may take some time...
    while ((pCurrNode = (graph::node *)m_PriorityQueue.ExtractMinPtr()) != NULL)
    {
        Dist = (pCurrNode->Pos - EndPos).LengthSquared();
        if (Dist < DistSq)
        {
            DistSq = Dist;
            Closest = pCurrNode;
        }
    }

    m_UsingPartialPath = TRUE;
    m_PartialPathDst = Closest->Pos;
    pCurrNode = Closest;

    // We found our goal.  Fill out the xarray and return.
    Array.Clear();

    // Continue until we've hit the start.
    while (pCurrNode != pStart)
    {
        // Locate the next edge on the path to the start.
        EdgeIndex = pCurrNode->PlayerInfo[m_BotID].GetPrev();
        ASSERT(EdgeIndex < g_Graph.m_nEdges);

        pCurrEdge = &g_Graph.m_pEdgeList[EdgeIndex];

        // Insert the node index into the start of the list, so
        // that the array comes out in order.
        Array.Insert(0, EdgeIndex);

        // Reassign current node to backtrack to the start.
        if (&g_Graph.m_pNodeList[pCurrEdge->NodeA] == pCurrNode)
        {
            pCurrNode  = &g_Graph.m_pNodeList[pCurrEdge->NodeB];
        }
        else
        {
            ASSERT (&g_Graph.m_pNodeList[pCurrEdge->NodeB] == pCurrNode
                && "Error- Node's breadcrumb edge does not contain node.");
            pCurrNode  = &g_Graph.m_pNodeList[pCurrEdge->NodeA];
        }
    }
    ASSERT (pCurrNode == pStart);

//                    x_DebugMsg("%d. Path found from Node %d to Node %d, along %d edges, Bot ID %d\n", m_GlobalCounter, 
//                        m_Start, m_End, Array.GetCount(), m_BotID);

    // We've completed our pathfind- turn off the flag and return.
    m_Flags &= ~BOT_PATH_FLAG_SEARCHING;

    if (m_bAppendWaiting)
    {
        Append(Array);
        m_bAppendWaiting = FALSE;
    }

}