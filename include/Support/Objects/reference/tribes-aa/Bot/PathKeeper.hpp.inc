//==============================================================================
//
//  PathKeeper.hpp
//
//==============================================================================
#ifndef PATHKEEPER_HPP
#define PATHKEEPER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "PathFinder.hpp"
#include "Path.hpp"
#include "PathManager.hpp"
#include "Objects/Terrain/Terrain.hpp"
#include "Graph.hpp"

//==============================================================================
//  DEFINES
//==============================================================================
#define DEBUG_DISPLAY_PATHS 0
#define DEBUG_COST_ANALYSIS 0

//==============================================================================
//  TYPES
//==============================================================================

class path_keeper 
{
public:
    path_keeper( void );

    void            Initialize      ( terr& Terr );
    void            PlotCourse      ( const vector3& Src, const vector3& Dst,
                                        f32 MaxAir);
    void            Update          ( void );
    const graph::node&  GetCurrentPoint ( void );
    const graph::node&  GetPrevPoint    ( void );
    const graph::node&  GetNextPoint    ( void );
    const graph::node&  GetDst          ( void );
    
    graph::edge     GetCurrentEdge  ( void ) const;
    s32             GetNumEdges     ( void ) const {
                        return m_EdgeIdxs.GetCount(); }
    xbool           AdvancePoint    ( void ); // false if there is no next point
    xbool           IsWaiting       ( void ) {
                        return m_PathManager.Searching()
                            || m_WaitingForCoarsePath
                            || m_WaitingForFinePath; };
    f32             GetElapsedTime  ( void );
    void            ForceEditorPath ( const xarray<u16>& PathPointIdxs );
    xbool           BotShouldBeInAir( void );
    xbool           FollowingTightEdge( void ) const;
    xbool           FollowingSkiEdge( void );
    void            LoadGraph       (const char* pFilename);
    s32             GetBotID        ( void )  const {
                        return m_PathManager.m_BotID; }
    void            FreeSlot            ( void );
    void            BlockCurrentEdge( u8 Flags
                        = graph::EDGE_FLAGS_TEAM_0_BLOCK
                        | graph::EDGE_FLAGS_TEAM_1_BLOCK );
    void            BlockEdge       ( s32 EdgeIdx, u8 Flags );
    xbool           DebugGraphIsConnected( void ) const;
    xbool           RunStressTest( void ) {
                        return m_PathManager.RunStressTest();}

    s16             GetClosestEdge( const vector3& Pos ) { return m_PathManager.GetClosestEdge(Pos);}
    s32             GetNPointsInRadius(
                        s32 nMAX
                        , f32 Radius
                        , const vector3& Pos
                        , vector3* pPoints ) const;
    s32             GetNNodesInRadius(
                        s32 nMAX
                        , f32 Radius
                        , const vector3& Pos
                        , s32* pPoints ) const;
    s32             InsertPointOrdered( s32 nMAX
                        , s32 nPoints
                        , const vector3& Pos
                        , const vector3& NewPoint
                        , vector3* pPoints
                        , f32 DistSQ ) const;
    s32             InsertNodeOrdered( s32 nMAX
                        , s32 nNodes
                        , const vector3& Pos
                        , s32 NewNode
                        , s32* pNodes
                        , f32 DistSQ ) const;
    xarray<u16>&    GetEdgeIdxs() { return m_EdgeIdxs; }
    // Assigns team bits for blocked edge usage.
    void            SetTeamBits ( u32 TeamBits );

private:
#if DEBUG_DISPLAY_PATHS
    void DrawCurrentPaths( void );
#endif
    void            SetNextCoarsePoint  ( const vector3& Src );
    void            LogPath( void ) const;
    void            PopUpToGround( vector3& Point ) const;

    graph::node m_Src;
    graph::node m_Dst;
    vector3     m_PrevDst;
    graph::node m_NextCoarsePoint;
    bot_path    m_CoarsePath;
    bot_path    m_FinePath;
    bot_path    m_LookaheadPath;
    path_finder m_PathFinder;
    s32         m_CurrentFinePointIndex;
    xbool       m_WaitingForCoarsePath;
    xbool       m_WaitingForFinePath;
    xbool       m_WaitingForLookaheadPath;
    xbool       m_UsingCoarsePath;
    terr*       m_Terr;

    xarray<u16> m_PathPointIdxs;
    xarray<u16> m_EdgeIdxs;
    path_manager m_PathManager;
    xbool       m_UsingEditorPath;
    s32         m_CurEditorEdge;
    graph::node m_CurNode;
    graph::node m_StartNode;
    graph::node m_EndNode;

    s32         m_nTroubleEdges;
    s32         m_TroubleEdgeList[16];

    u32         m_TeamBits;
};

#endif //ndef PATHKEEPER_HPP
