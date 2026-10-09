//==============================================================================
//
//  Pathkeeper.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "Globals.hpp"
#include "PathKeeper.hpp"
#include "PathFinder.hpp"
#include "Path.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "BotLog.hpp"
#include "MAI_Manager.hpp"

//==============================================================================
//  DEFINES
//==============================================================================
static const f32 COARSE_DENSITY = 250.0f;
static const f32 COARSE_DENSITY_SQ = COARSE_DENSITY * COARSE_DENSITY;
static const s32 FINE_DENSITY = 2;
//static s32 PATH_NUMBER = 0;
static const f32 COARSE_POINT_HILL_AVOID_STEP = 10.0f;

static       xbool       m_TeamBitsSet = FALSE;
extern graph            g_Graph;

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================
path_keeper::path_keeper( void ) :
    m_CurrentFinePointIndex   ( 0 )
    , m_WaitingForCoarsePath    ( FALSE )
    , m_WaitingForFinePath      ( FALSE )
    , m_WaitingForLookaheadPath ( FALSE )
    , m_UsingCoarsePath         ( FALSE )
    , m_UsingEditorPath         ( TRUE )
{
}

void path_keeper::Initialize( terr& Terr )
{
    m_TeamBitsSet = FALSE;
    m_Terr = &Terr;
    m_PathManager.Init( xfs( "%s/%s.gph", tgl.MissionDir, tgl.MissionName ) );
    //m_PathFinder.Initialize( Terr );
    m_nTroubleEdges = 0;
}

void path_keeper::PlotCourse( const vector3& Src, const vector3& Dst, f32 MaxAir )
{
//    bot_log::Log( "======================================================================\n" );
//    bot_log::Log( "PlotCourse:\n" );
//    bot_log::Log( "    From:    ( %7.2f, %7.2f, %7.2f ) To: (%7.2f, %7.2f, %7.2f ) )\n\n"
//        , Src[0], Src[1], Src[2], Dst[0], Dst[1], Dst[2] );
    if (!m_TeamBitsSet)
    {
        MAI_Mgr.SetBotTeamBits();

        // Set to true, until another bot gets reinitialized.
        m_TeamBitsSet = TRUE;
    }

    static f32 TotalTime = 0.0f;
    static s32 TotalCalls = 0;
    static f32 MaxTime = 0.0f;
    xtimer Timer;
    Timer.Reset();
    Timer.Start();

    if ( Dst != m_PrevDst )
    {
        // We're not coming off of a timeout
        m_nTroubleEdges = 0;
        m_PrevDst = Dst;
    }
    
    
    m_Src.Pos = Src;
    m_Dst.Pos = Dst;

    m_PathPointIdxs.Clear();
    m_EdgeIdxs.Clear();

    if ( (Src - Dst).LengthSquared() > 0.01f )
    {
        m_PathManager.FindPath( m_EdgeIdxs, Src, Dst, MaxAir );
    }

    m_UsingEditorPath = TRUE;
    m_CurEditorEdge = -1;

    if ( !m_PathManager.Searching() )
    {
        LogPath();
    }

    if ( Timer.ReadMs() > MaxTime ) MaxTime = Timer.ReadMs();
    TotalTime += Timer.ReadMs();
    ++TotalCalls;
    //x_printf( "PlotCourse: Max: %7.3f, Avg: %7.3f\n", MaxTime, TotalTime/TotalCalls );

#if 0
    m_FinePath.Clear();
    m_LookaheadPath.Clear();

    SetNextCoarsePoint( m_Src.Pos );
    m_WaitingForFinePath = !m_PathFinder.InitializeFindPath( FINE_DENSITY, m_Src.Pos, m_NextCoarsePoint.Pos );
#endif
}

void path_keeper::Update( void ) 
{
#if DEBUG_DRAW_BLOCKED_CELLS
    eng_Begin();
    const s32 NumBlocked = m_PathFinder.m_BlockedCells.GetCount();
    for ( s32 Cell = 0; Cell < NumBlocked; ++Cell )
    {
        const f32 X = m_PathFinder.m_BlockedCells[Cell].X * 8.0f;
        const f32 Z = m_PathFinder.m_BlockedCells[Cell].Z * 8.0f;
        const f32 Y = m_Terr->GetHeight( Z, X);
        const f32 W = 0.5f;

        draw_Line( vector3( X, Y, Z ), vector3( X, Y+W, Z ) );
    }
    eng_End();
#endif

    if ( !m_PathManager.Searching() && !m_WaitingForFinePath )
    {
#if DEBUG_DISPLAY_PATHS
        DrawCurrentPaths();
#endif
        return;
    }


    if ( m_PathManager.Searching() )
    {
        m_PathManager.Update( m_EdgeIdxs );

        if ( !m_PathManager.Searching() )
        {
            // The path manager is done searching...
            LogPath();
            
            // We should always be able to find a path
//            ASSERT( m_EdgeIdxs.GetCount() > 0 );
        }
    }

#if 0 // removed when we moved to using editor paths as the coarse path
    if ( !m_UsingEditorPath && m_PathFinder.Update() ) 
    {
        if ( m_WaitingForFinePath )
        {
            m_WaitingForFinePath = false;
            m_FinePath = *m_PathFinder.GetPath();

            const s32 Num = m_FinePath.GetCount();
/*            bot_log::Log( "Found Fine Path\n\n" );
            bot_log::Log( "    Nodes: %i\n", Num );
            bot_log::Log( "    First Node: ( %7.2f, %7.2f, %7.2f )\n"
                , m_FinePath[0].Pos[0], m_FinePath[0].Pos[1], m_FinePath[0].Pos[2] );
            bot_log::Log( "    Last Node:  ( %7.2f, %7.2f, %7.2f )\n"
                , m_FinePath[Num-1].Pos[0], m_FinePath[Num-1].Pos[1], m_FinePath[Num-1].Pos[2] );
            PATH_NUMBER++;
            bot_log::Log( "    Path Number: %i\n", PATH_NUMBER );
            x_printf( "Found Fine Path Number %i\n", PATH_NUMBER );
            bot_log::Log( "\n" );
            
  */          m_CurrentFinePointIndex = 0;
            m_WaitingForLookaheadPath = false;
        }
        else
        {
            ASSERT( m_WaitingForLookaheadPath );
//            bot_log::Log( "Found Lookahead Path\n\n" );
            m_WaitingForLookaheadPath = false;
            m_LookaheadPath = *m_PathFinder.GetPath();
        }
    }
    else 
    {
        // nothing to do
        return;
    }
#endif
}

#if DEBUG_DISPLAY_PATHS
void path_keeper::DrawCurrentPaths( void )
{
    if ( m_WaitingForFinePath ) return;
    s32 i;
    const vector3 AddMe( 0.0f, 1.0f, 0.0f );
    eng_Begin();

    draw_Marker( m_Dst.Pos );
    
    if ( !m_UsingEditorPath )
    {
        for ( i = 0; i < m_FinePath.GetCount(); ++i )
        {
            draw_Sphere( m_FinePath[i].Pos + AddMe, 1.0f, XCOLOR_RED );
            if ( i != 0 )
            {
                draw_Line(
                    m_FinePath[i].Pos + AddMe
                    , m_FinePath[i - 1].Pos + AddMe
                    , XCOLOR_BLUE );
            }
        }
    }

    draw_Sphere( GetCurrentPoint().Pos + AddMe
        , m_UsingEditorPath ? 1.0f : 3.0f
        , XCOLOR_RED );
    
    eng_End();
}
#endif

//==============================================================================

const graph::node& path_keeper::GetCurrentPoint ( void )
{
    if ( m_UsingEditorPath )
    {
        const s32 NumEdges = m_EdgeIdxs.GetCount();

        if ( !m_PathManager.Searching() && m_EdgeIdxs.GetCount() == 0)
        {
#if defined (acheng)
//            x_DebugMsg("Using 0-edge path:: GetCurrentPoint Call");
#endif
            return m_Dst;
        }

        if ( m_PathManager.Searching() )
        {
            return m_Dst;
        }
        else if ( m_CurEditorEdge < 0 )
        {
            const graph::edge& TempEdge
                = g_Graph.m_pEdgeList[m_EdgeIdxs[0]];

            m_StartNode.Pos = m_Src.Pos.GetClosestPToLSeg(
                g_Graph.m_pNodeList[TempEdge.NodeA].Pos
                , g_Graph.m_pNodeList[TempEdge.NodeB].Pos );

            PopUpToGround( m_StartNode.Pos );
            
            return m_StartNode;
        }
        else if ( m_CurEditorEdge < NumEdges - 1 )
        {
            return g_Graph.GetEdgeEnd( m_EdgeIdxs, m_CurEditorEdge );
        }
        else if ( m_CurEditorEdge == NumEdges - 1 )
        {
            const graph::edge& TempEdge
                = g_Graph.m_pEdgeList[m_EdgeIdxs[NumEdges-1]];

            m_EndNode.Pos = m_Dst.Pos.GetClosestPToLSeg(
                g_Graph.m_pNodeList[TempEdge.NodeA].Pos
                , g_Graph.m_pNodeList[TempEdge.NodeB].Pos );

            PopUpToGround( m_EndNode.Pos );
            
            return m_EndNode;
        }
        else // --> m_CurEditorEdge == NumEdges
        {
            return m_Dst;
        }
    }
    else if ( m_WaitingForFinePath )
    {
        return m_NextCoarsePoint;
    }
    else 
    {
        if ( m_CurrentFinePointIndex >= m_FinePath.GetCount() )
        {
            return m_Dst;
        }
        else
        {
            return m_FinePath[m_CurrentFinePointIndex];
        }
    }
}

const graph::node& path_keeper::GetPrevPoint ( void )
{
    if ( m_UsingEditorPath )
    {
        if ( !m_PathManager.Searching() && m_EdgeIdxs.GetCount() == 0 )
        {
#if defined (acheng)
//            x_DebugMsg("Using 0-edge path:: GetPrevPoint Call");
#endif
            return m_Src;
        }

        if ( m_CurEditorEdge == m_EdgeIdxs.GetCount() )
        {
            return m_EndNode;
        }
        else if ( m_CurEditorEdge >= 0  && m_EdgeIdxs.GetCount() != 1)
        {
            return g_Graph.GetEdgeStart( m_EdgeIdxs, m_CurEditorEdge );
        }
        else 
        {
            // either we're not on the first edge yet, or there's no graph
            // yet, so return Src either way
            return m_Src;
        }
    }
    if ( m_WaitingForFinePath )
    {
        return GetCurrentPoint();
    }
    else if ( m_CurrentFinePointIndex <= 0 )
    {
        return m_Src;
    }
    else
    {
        return m_FinePath[m_CurrentFinePointIndex - 1];
    }
}

const graph::node& path_keeper::GetNextPoint ( void )
{
    if ( m_UsingEditorPath )
    {
        if ( !m_PathManager.Searching() && m_EdgeIdxs.GetCount() == 0 )
        {
#if defined (acheng)
//            x_DebugMsg("Using 0-edge path:: GetNextPoint Call");
#endif

            return m_Dst;
        }
        const s32 NextEditorEdge = m_CurEditorEdge + 1;
        if ( NextEditorEdge < m_EdgeIdxs.GetCount() && m_EdgeIdxs.GetCount() != 1)
        {
            return g_Graph.GetEdgeEnd( m_EdgeIdxs, NextEditorEdge );
        }
        else 
        {
            // either this is the last edge, or there's no graph yet, either
            // way, return Dst
            return m_Dst;
        }
    }
    if ( m_WaitingForFinePath )
    {
        return GetCurrentPoint();
    }
    else if ( m_CurrentFinePointIndex >= m_FinePath.GetCount() - 1 )
    {
        return m_Dst;
    }
    else
    {
        return m_FinePath[m_CurrentFinePointIndex + 1];
    }
}

//==============================================================================

const graph::node& path_keeper::GetDst ( void )
{
    return m_Dst;
}


xbool path_keeper::AdvancePoint ( void )
{
    if ( m_UsingEditorPath )
    {
        ++m_CurEditorEdge;

        if ( m_CurEditorEdge > m_EdgeIdxs.GetCount() )
        {
            m_CurEditorEdge = m_EdgeIdxs.GetCount();
        }

//        bot_log::Log( "Advancing to Edge[%i]\n", m_CurEditorEdge );
    }
    else 
    {
        ++m_CurrentFinePointIndex;

        // is it time to start our lookahead search?
        if ( !m_WaitingForFinePath
            && !m_WaitingForLookaheadPath
            && m_CurrentFinePointIndex == m_FinePath.GetCount() ) // at the end?

        {
            // Start search?
            m_LookaheadPath.Clear();

            // If we're not at the end of the path...
            if ( m_NextCoarsePoint.Pos != m_Dst.Pos )
            {
                // need another fine path
//                bot_log::Log( "AdvancePoint finding a path to next CoarsePoint\n" );

                vector3 Src = m_NextCoarsePoint.Pos;
                SetNextCoarsePoint( m_NextCoarsePoint.Pos );
            
                m_WaitingForLookaheadPath
                    = !m_PathFinder.InitializeFindPath(
                        FINE_DENSITY
                        , Src
                        , m_NextCoarsePoint.Pos );
            }
        }
    
        // are we at the end of the fine path?
        if ( m_CurrentFinePointIndex == m_FinePath.GetCount() )
        {
            // do we have a lookahead path to use?
            if ( !m_WaitingForLookaheadPath )
            {
                // Transfer the lookahead path into the fine path
//                bot_log::Log( "moving lookahead path (length %i) into fine path\n\n"
//                    , m_LookaheadPath.GetCount() );
                m_FinePath.Clear();
                m_FinePath = m_LookaheadPath;
                ASSERT( m_FinePath.GetCount() == m_LookaheadPath.GetCount() );
                m_LookaheadPath.Clear();
                m_CurrentFinePointIndex = 0;
                m_WaitingForFinePath = false;
            }
            else
            {
                // Continue to the same point...
                --m_CurrentFinePointIndex;

            // Transfer the waiting to the fine path
//                bot_log::Log( "transfering waiting to fine path\n\n" );
                m_WaitingForLookaheadPath = false;
                m_WaitingForFinePath = true;
            }
        }
    }
    
    return true;
}

void path_keeper::SetNextCoarsePoint( const vector3& Src )
{
    const f32 DistanceToDstSquared = (m_Dst.Pos - Src).LengthSquared();
    if ( DistanceToDstSquared < COARSE_DENSITY_SQ )
    {
        // cool, the next one is our destination
        m_NextCoarsePoint.Pos = m_Dst.Pos;
    }
    else
    {
        vector3 ToDst = m_Dst.Pos - Src;
        ToDst.Normalize();
        ToDst *= COARSE_DENSITY;
        m_NextCoarsePoint.Pos = Src + ToDst;

        const f32 MAX_POINT_HEIGHT = MAX( Src.Y, m_Dst.Pos.Y );

        // check to make sure we're not putting this point on top of some
        // huge hill...
        if ( m_Terr->GetHeight( m_NextCoarsePoint.Pos.Z, m_NextCoarsePoint.Pos.X )
            > MAX_POINT_HEIGHT )
        {
            ToDst.Normalize();
            ToDst *= COARSE_POINT_HILL_AVOID_STEP;
        }
        
        while ( (m_NextCoarsePoint.Pos - Src).LengthSquared() < DistanceToDstSquared
            && m_Terr->GetHeight( m_NextCoarsePoint.Pos.Z, m_NextCoarsePoint.Pos.X )
            > MAX_POINT_HEIGHT )
        {
            m_NextCoarsePoint.Pos += ToDst;
        }
    }

//    bot_log::Log( "Next Coarse Point set: (%7.2f, %7.2f, %7.2f)\n"
//        , m_NextCoarsePoint.Pos.X, m_NextCoarsePoint.Pos.Y, m_NextCoarsePoint.Pos.Z );
}

f32 path_keeper::GetElapsedTime( void )
{
    return( m_PathFinder.GetElapsedTime() );
}

void path_keeper::ForceEditorPath ( const xarray<u16>& PathPointIdxs )
{
    ASSERT( PathPointIdxs.GetCount() > 0 );
    m_PathPointIdxs = PathPointIdxs;
    m_UsingEditorPath = TRUE;
    m_Src = g_Graph.m_pNodeList[PathPointIdxs[0]];
    m_Dst = g_Graph.m_pNodeList[PathPointIdxs.GetCount() - 1];
}

graph::edge path_keeper::GetCurrentEdge( void ) const
{
    ASSERT( g_Graph.m_nEdges > 0 );

    if( m_CurEditorEdge == -1 ) 
        return g_Graph.m_pEdgeList[m_EdgeIdxs[0]];

    if ( m_CurEditorEdge >= m_EdgeIdxs.GetCount() )
    {
        return g_Graph.m_pEdgeList[m_EdgeIdxs[m_EdgeIdxs.GetCount()-1]];
    }

    return g_Graph.m_pEdgeList[m_EdgeIdxs[m_CurEditorEdge]];
}


// TRUE if the bot should be in the air based on the current edge (aerial
// or not)
xbool path_keeper::BotShouldBeInAir( void )
{
    if ( m_EdgeIdxs.GetCount() <= 0 ) 
    {
        return FALSE;
    }
    
    if ( m_CurEditorEdge < 0 || m_CurEditorEdge >= m_EdgeIdxs.GetCount() )
    {
        // if we're going up, then we need to check if we can see the
        // current point from the previous point
        const vector3 CurrentPoint = GetCurrentPoint().Pos;
        const vector3 PrevPoint = GetPrevPoint().Pos;

        if ( CurrentPoint.Y < PrevPoint.Y )
        {
            return FALSE; // going down, no need to be in the air
        }
        else if ( (CurrentPoint - PrevPoint).LengthSquared() < 0.001f )
        {
            return FALSE; // haven't really gone anywhere
        }
        else
        {
            // check for ray collision between prev and cur points
            collider        Ray;
            Ray.RaySetup    ( NULL, PrevPoint, CurrentPoint );
            ObjMgr.Collide  ( object::ATTR_SOLID_STATIC, Ray );

            if ( Ray.HasCollided() == TRUE )
            {
                // see if the collision point is beyond the point we want to see
                collider::collision Collision;
                Ray.GetCollision( Collision );
                const f32 DistSQ = (CurrentPoint - PrevPoint).LengthSquared();
                const f32 CollisionDistSQ
                    = (PrevPoint - Collision.Point).LengthSquared();
                return CollisionDistSQ > DistSQ;
            }
            else
            {
                return TRUE;
            }
        }
    }

    return (GetCurrentEdge().Type == graph::EDGE_TYPE_AEREAL)
        || ((GetCurrentEdge().Type == graph::EDGE_TYPE_SKI) 
            && (GetCurrentPoint().Pos.Y > GetPrevPoint().Pos.Y));
}

xbool path_keeper::FollowingTightEdge( void ) const
{
    if ( m_CurEditorEdge < 0
        || m_CurEditorEdge >= m_EdgeIdxs.GetCount()
        || m_EdgeIdxs.GetCount() <= 0 )
    {
        // we don't have an edge yet
        return TRUE; // play it safe
    }
    
    vector3 PosA = g_Graph.m_pNodeList[GetCurrentEdge().NodeA].Pos;
    vector3 PosB = g_Graph.m_pNodeList[GetCurrentEdge().NodeB].Pos;
    return !((GetCurrentEdge().Flags & graph::EDGE_FLAGS_LOOSE) 
        && ((PosA-PosB).LengthSquared() > SQR( 15.0f )) );
}

xbool path_keeper::FollowingSkiEdge( void )
{
    if ( m_CurEditorEdge < 0
        || m_CurEditorEdge >= m_EdgeIdxs.GetCount()
        || m_EdgeIdxs.GetCount() <= 0 )
    {
        // we don't have an edge yet
        return FALSE; // play it safe
    }
    
    return( (GetCurrentEdge().Type == graph::EDGE_TYPE_SKI)
        && (GetCurrentPoint().Pos.Y < GetPrevPoint().Pos.Y) );
}

void path_keeper::LoadGraph(const char* pFilename)
{
    m_PathManager.Init(pFilename);
}

void path_keeper::LogPath( void ) const
{
//    bot_log::Log( "PathManager found path:\n" );
//    bot_log::Log( "%i Edges\n", m_EdgeIdxs.GetCount() );
//    int i;

//    if (m_EdgeIdxs.GetCount() == 1) return;
//    for ( i = 0; i < m_EdgeIdxs.GetCount(); ++i )
//    {
//        const vector3 Start = g_Graph.GetEdgeStart( m_EdgeIdxs, i ).Pos;
//        const vector3 End = g_Graph.GetEdgeEnd( m_EdgeIdxs, i ).Pos;

        //bot_log::Log(
        //    "Edge[%i] from (%7.2f, %7.2f, %7.2f) to (%7.2f, %7.2f, %7.2f)\n"
        //    , i, Start.X, Start.Y, Start.Z, End.X, End.Y, End.Z );
//    }
}

//======================================================================
//======================================================================
void path_keeper::BlockCurrentEdge( u8 Flags )
{
    if ( m_PathManager.Searching() || m_EdgeIdxs.GetCount() == 0)
    {
        // Block zero-edge paths
        m_PathManager.BlockZeroEdgePaths();
#if defined (acheng)
//        x_DebugMsg("****** Blocked 0-edge paths PathKeeper******\n");
#endif
        return;
    }
    
    ASSERT( m_EdgeIdxs.GetCount() >= 1 );
    
    if ( m_CurEditorEdge >= 0
        && m_CurEditorEdge < m_EdgeIdxs.GetCount() )
    {
        // Our current edge is actually an edge in the graph
        BlockEdge( m_EdgeIdxs[m_CurEditorEdge], Flags );
    }
    else if ( m_CurEditorEdge <= 0 )
    {
        // We haven't yet made it to our first edge
        // Block our first edge instead (the one we're going to)
        BlockEdge( m_EdgeIdxs[0], Flags );
    }
    else if ( m_CurEditorEdge >= m_EdgeIdxs.GetCount() )
    {
        // We are between our last edge and our final destination
        // Block our previous edge instead (the one we're coming from)
        BlockEdge( m_EdgeIdxs[m_EdgeIdxs.GetCount() - 1], Flags );
    }
    else 
    {
        ASSERT( 0 ); // the three ifs above should keep this from happening
    }
}

//======================================================================
//======================================================================
void path_keeper::BlockEdge( s32 EdgeIdx, u8 Flags )
{
    // Check to see if this shows up in our TroubleEdgeList
    xbool AlreadyInList = FALSE;
    s32 i;

    for ( i = 0; i < m_nTroubleEdges; ++i )
    {
        if ( m_TroubleEdgeList[i] == EdgeIdx )
        {
            AlreadyInList = TRUE;
            break;
        }
    }

    if ( AlreadyInList )
    {
        // This is the second time this edge has given us trouble,
        // so block it
        g_Graph.BlockEdge( EdgeIdx, Flags );
    }
    else
    {
        static const s32 MAX_TROUBLE_EDGES = 16;
        // This is the first time this edge has given us trouble,
        // so add it to the TroubleEdgeList and move on.
        if ( m_nTroubleEdges >= MAX_TROUBLE_EDGES )
        {
            // This is a problem, shouldn't have that much trouble....
            // Overwrite one of the troubled edges at random. [DMT]

            m_TroubleEdgeList[ x_irand(0,m_nTroubleEdges-1) ] = EdgeIdx;
        }
        else
        {
            m_TroubleEdgeList[m_nTroubleEdges] = EdgeIdx;
            ++m_nTroubleEdges;
        }
    }
}

//======================================================================
// PopUpToGround()
//
// Put the given point on the ground if it is below ground and not in a
// building.
//======================================================================
void path_keeper::PopUpToGround( vector3& Point ) const
{
    f32 GroundHeight = m_Terr->GetHeight( Point.Z, Point.X );

    if ( Point.Y < GroundHeight
        && !path_manager::PointInsideBuilding( Point ) )
    {
        // Pop the point up
        Point.Y = GroundHeight;
    }
}

//==============================================================================
xbool path_keeper::DebugGraphIsConnected( void ) const
{
    return g_Graph.DebugConnected();
}


//==============================================================================
// GetNPointsInRadius()
//==============================================================================
s32 path_keeper::GetNPointsInRadius( s32 nMAX
    , f32 Radius
    , const vector3& Pos
    , vector3* pPoints ) const
{
    s32 i;
    s32 NumPoints = 0;
    const f32 RadiusSQ = SQR( Radius );

    for ( i = 0; i < g_Graph.m_nNodes ; ++i )
    {
        const vector3& NewPoint = g_Graph.m_pNodeList[i].Pos;
        const f32 DistSQ = ( NewPoint - Pos).LengthSquared();
        if ( DistSQ <= RadiusSQ && g_Graph.NodeIsOnGround( i ) )
        {
            NumPoints = InsertPointOrdered(
                nMAX
                , NumPoints
                , Pos
                , NewPoint
                , pPoints
                , DistSQ );
        }
    }

    return NumPoints;
}

//==============================================================================
// GetNNodesInRadius()
//==============================================================================
s32 path_keeper::GetNNodesInRadius( s32 nMAX
    , f32 Radius
    , const vector3& Pos
    , s32* pNodes ) const
{
    s32 i;
    s32 NumNodes = 0;
    const f32 RadiusSQ = SQR( Radius );

    for ( i = 0; i < g_Graph.m_nNodes ; ++i )
    {
        const vector3& NewPoint = g_Graph.m_pNodeList[i].Pos;
        const f32 DistSQ = ( NewPoint - Pos).LengthSquared();
        if ( DistSQ <= RadiusSQ && g_Graph.NodeIsOnGround( i ) )
        {
            NumNodes = InsertNodeOrdered(
                nMAX
                , NumNodes
                , Pos
                , i
                , pNodes
                , DistSQ );
        }
    }

    return NumNodes;
}

//==============================================================================
// InsertPointOrdered()
// Insert NewPoint into pPoints in ascending order of distance from Pos.
// DistSQ refers to the dist sq. between Pos and NewPoint
// The list has a nMAX cap on points, and nPoints currently in it
//==============================================================================
s32 path_keeper::InsertPointOrdered( s32 nMAX
    , s32 nPoints
    , const vector3& Pos
    , const vector3& NewPoint
    , vector3* pPoints
    , f32 DistSQ ) const
{
    ASSERT( DistSQ >= 0.0f );
    ASSERT( nPoints <= nMAX );

    // Throw NewPoint on the end of the list
    if ( nPoints == nMAX )
    {
        pPoints[nPoints - 1] = NewPoint;
    }
    else
    {
        pPoints[nPoints] = NewPoint;
        ++nPoints;
    }

    // Swap NewPoint down the list until it is in its proper location
    s32 i;
    const s32 End = nPoints - 1;
    vector3 Temp;

    for ( i = End; i > 0; --i )
    {
        if ( (pPoints[i-1] - Pos).LengthSquared() > DistSQ )
        {
            // Swap pPoints[i-1] with pPoints[i]
            Temp = pPoints[i-1];
            pPoints[i-1] = pPoints[i];
            pPoints[i] = Temp;
        }
        else
        {
            // Were done
            break;
        }
    }

    return nPoints;
}

//==============================================================================
// InsertNodeOrdered()
// Insert NewNode into pNodes in ascending order of distance from Pos.
// DistSQ refers to the dist sq. between Pos and NewNode
// The list has a nMAX cap on points, and nPoints currently in it
//==============================================================================
s32 path_keeper::InsertNodeOrdered( s32 nMAX
    , s32 nNodes
    , const vector3& Pos
    , s32 NewNode
    , s32* pNodes
    , f32 DistSQ ) const
{
    ASSERT( DistSQ >= 0.0f );
    ASSERT( nNodes <= nMAX );

    // Throw NewPoint on the end of the list
    if ( nNodes == nMAX )
    {
        pNodes[nNodes - 1] = NewNode;
    }
    else
    {
        pNodes[nNodes] = NewNode;
        ++nNodes;
    }

    // Swap NewNode down the list until it is in its proper location
    s32 i;
    const s32 End = nNodes - 1;
    s32 Temp;

    for ( i = End; i > 0; --i )
    {
        if ( (g_Graph.m_pNodeList[pNodes[i-1]].Pos - Pos).LengthSquared()
            > DistSQ )
        {
            // Swap pNodes[i-1] with pNodes[i]
            Temp = pNodes[i-1];
            pNodes[i-1] = pNodes[i];
            pNodes[i] = Temp;
        }
        else
        {
            // Were done
            break;
        }
    }

    return nNodes;
}



//==============================================================================
// FreeSlot
//==============================================================================

void path_keeper::FreeSlot( void )
{
    m_PathManager.FreeSlot();
}


//==============================================================================
 
void path_keeper::SetTeamBits ( u32 TeamBits )
{
    m_TeamBits = TeamBits;
    m_PathManager.SetTeamBits( TeamBits );
}