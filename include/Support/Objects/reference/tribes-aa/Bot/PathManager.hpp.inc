#ifndef PATH_MANAGER_HPP
#define PATH_MANAGER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "graph.hpp"
#include "FibHeapAC.hpp"


void  PathManager_LoadGraph          ( const char* pFilename );
xbool PathManager_IsGraphLoaded      ( void );
void  Path_Manager_InitializePathMng ( void );
void  Path_Manager_ForceLoad         ( void );

class path_manager
{
//==============================================================================
//  PATH MANAGER TYPES
//==============================================================================
    public:    
    
        enum            
        {
            BOT_PATH_FLAG_SEARCHING    = (1<<0),
        };


//==============================================================================
//  PATH MANAGER FUNCTIONS
//==============================================================================
    public:
                path_manager    ( void );
                ~path_manager    ( void );
        void    Init            ( const char* pFilename = "temp.gph" );

        void    FindPath        ( xarray<u16> &Array, const vector3& Start, 
                                        const vector3& End, f32 MaxAir,
                                        u8 Flags
                                            = graph::EDGE_FLAGS_TEAM_0_BLOCK
                                            | graph::EDGE_FLAGS_TEAM_1_BLOCK
                                            , xbool AllAtOnce = FALSE );

        u16     Update          ( xarray<u16> &Array, xbool AllAtOnce = FALSE );

        xbool   Searching       ( void ) const
                                   { return m_Flags & BOT_PATH_FLAG_SEARCHING; }
    
        s16     GetClosestEdge  ( const vector3& Pos );

        s16     OldGetClosestEdge  ( const vector3& Pos );

        void    BlockEdge( s32 EdgeId );
        void    BlockZeroEdgePaths()    { m_BlockedZeroEdgePath = TRUE; }
        
        u16     GetStartIndex     ( void ) const 
                                {       return m_Start;       }
        u16     GetEndIndex     ( void ) const 
                                {       return m_End;       }

        void    FreeSlot            ( void );

        static xbool    PointInsideBuilding     ( const vector3& Point );

        xbool   DebugGraphIsConnected( void );
        xbool   RunStressTest ( void );

        void    SetTeamBits     ( u32 TeamBits ) { m_TeamBits = TeamBits; }

        //
        //  Partial Path find data
        //

        xbool   IsUsingPartialPath ( void )     { return m_UsingPartialPath; }
const vector3&  GetPartialPathTarget ( void )   { return m_PartialPathDst; }
        void    ClearPartialPath    ( void )    { m_UsingPartialPath = FALSE; }

//==============================================================================
// timing & debugging stuff
//==============================================================================
    public:
        s32     m_BotID;

    protected:
        u32     m_TeamBits;
        xbool   m_Initialized;
        f32     m_TimeSegment;
        f32     m_TimeComplete;

        u8      m_GlobalCounter;  // Used for checking visited nodes
        u16     m_Start;          // Index of start node
        u16     m_End;            // Index of end node
        u16     m_Flags;

        xbool   m_bAppendWaiting;
        s32     m_iStartEdge;
        s32     m_iEndEdge;

        f32     m_MaxAirDist;

        u8      m_TeamFlags;
        fheap   m_PriorityQueue;
        
        xbool   m_BlockedZeroEdgePath;
        xbool   TriedAgain;

        // Partial path data
        xbool       m_UsingPartialPath;
        vector3     m_PartialPathDst;

//==============================================================================
//  PATH MANAGER INTERNAL FUNCTIONS
//==============================================================================
    protected:
        void    Reset           ( void );
        void    ResetVisited(s32 BotID);
        void    ResetAllVisited();
        u16     DetermineCost(u16 NodeA, u16 NodeB);
        u16     DetermineEdgeCost(u16 Edge, u16 Start, s32 PrevEdgeAerial);
        u16     GetIdxNearestPoint( vector3 Point ) const;

        // FindPath returns the index of the starting node.
        u16     FindPath        ( xarray<u16> &Array, xbool AllAtOnce = FALSE );

        // Find the closest node to our target and quit with that.
        void    TerminatePathFind ( xarray<u16>& Array );

        void    Append( xarray<u16> &Array);
};

#endif
