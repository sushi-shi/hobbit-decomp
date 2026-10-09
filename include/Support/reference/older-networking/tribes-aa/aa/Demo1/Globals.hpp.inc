//==============================================================================
//
//  Globals.hpp
//
//==============================================================================

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "x_plus.hpp"
#include "x_time.hpp"
#include "x_color.hpp"
#include "netlib/netlib.hpp"
#include "Entropy.hpp"
#include "sky/sky.hpp"
#include "Shape/ShapeManager.hpp"
#include "ObjectMgr/ObjectMgr.hpp"
#include "Particles/ParticleLibrary.hpp"
#include "Particles/ParticlePool.hpp"
#include "Particles/postfx.hpp"
#include "Fog/fog.hpp"
#include "Damage/Damage.hpp"
#include "Objects\Terrain\Terrain.hpp"

//==============================================================================
// DEFINES
//==============================================================================

// Declare shp and xbmp output paths
#ifdef TARGET_PC

	#define SHP_PATH    "Data/Shapes/PC"        
	#define XBMP_PATH   "Data/Shapes/PC"        

#endif

#ifdef TARGET_PS2

	#define SHP_PATH    "Data/Shapes/PS2"        
	#define XBMP_PATH   "Data/Shapes/PS2"        

#endif

//==============================================================================

class game_server;
class game_client;
class game_user;
class ui_manager;
class hud_manager;
class master_server;

//==============================================================================

#define MAX_GAME_USERS 2
/*
enum game_stage
{
    GAME_STAGE_INIT,
    GAME_STAGE_INIT_FRONTEND,
    GAME_STAGE_FRONTEND,
    GAME_STAGE_INIT_SERVER,
    GAME_STAGE_INIT_CLIENT,
    GAME_STAGE_DECIDE_MISSION,
    GAME_STAGE_NEXT_MISSION,
    GAME_STAGE_LOAD_MISSION,
    GAME_STAGE_SYNC_WITH_SERVER,
    GAME_STAGE_INGAME,
    GAME_STAGE_UNLOAD_MISSION,
    GAME_STAGE_RETURN_TO_FE,
    GAME_STAGE_SERVER_COOL_DOWN,
    GAME_STAGE_SERVER_DISCONNECT,
    GAME_STAGE_CLIENT_DISCONNECT,
};
*/
//==============================================================================

struct fe_user_info
{
    net_address MyAddr;
    net_address ServerAddr;
    xbool       IsServer;
    xwchar      Server[32];
    xwchar      MyName[32];
    xbool       UsingConfig;
};

//==============================================================================

struct spawn_point
{
    u32     TeamBits;
    vector3 Pos;
    radian  Pitch;
    radian  Yaw;

    void Setup( u32 aTeamBits, f32 X, f32 Y, f32 Z, radian aPitch, radian aYaw )
    {
        TeamBits = aTeamBits;
        Pos.X = X;
        Pos.Y = Y;
        Pos.Z = Z;
        Pitch = aPitch;
        Yaw   = aYaw;
    }
};

#define MAX_SPAWN_POINTS 512

//==============================================================================

struct player_context
{
    view            View;
//  object::id      PlayerID;
    s32             PlayerIndex;
};

struct t2_globals
{
    // If set, it allows to skip huge TimeDelta in the pump_ProcessTime loop.
    xbool           JumpTime;

    // Available to general users
    s64				LogicTimeMs;
    f32             DeltaLogicTime;
    random			Random;
    xbool			DisplayStats;
    xbool           LogStats;
    xbool           ShowColliderCannon;
    s32             NRenders;
					
    xbool			ServerPresent;
    xbool			ClientPresent;
    xbool			UserPresent;
    xbool           Connected;

    //game_stage      GameStage;
    fe_user_info    UserInfo;

    // Private
    s32				EventRateRender;
    s32				EventRateInput;
    s32				EventRatePacket;
    s32				MinLogicStepSize;
    s32				MaxLogicStepSize;
    s64				LastRenderTime;
    s64				LastInputTime;
    s64				LastPacketTime;
    s64				LastLogicTime;
    xtick			ActualStartTime;
    xtick			ActualTime;
    s32				TicksPerMs;     
    s64				LastNetStatsResetTime;

    s32				EventTimeCount;
    s32				EventInputCount;
    s32				EventRenderCount;
    s32				EventPacketCount;
    char			EventString[33];
    s32				EventStringIndex;

    f32				HeartbeatDelay;
    f32				ServerPacketShipDelay;
    f32				ClientPacketShipDelay;
    s32				ConnManagerTimeout;

    master_server*  pMasterServer;
    game_server*	pServer;
    game_client*	pClient;
    game_user*		pUser[ MAX_GAME_USERS ];
    s32				NUsers;
    s32             MissionSequence;

    xbool			DoRender;
    xbool           DisplayObjectStats;

    sky				Sky;
    vector3         Sun;
    xcolor          Direct;
    xcolor          Ambient;

	shape_manager	GameShapeManager;
	shape_manager	EnvShapeManager;

    spawn_point     SpawnPoint[MAX_SPAWN_POINTS];
    s32             NSpawnPoints;

    pfx_library     ParticleLibrary;
    pfx_pool        ParticlePool;

    ui_manager*     pUIManager;
    hud_manager*    pHUDManager;

    xtimer          ObjMgrLogicTime;
    xtimer          ServerProcessTime;
    xtimer          ClientProcessTime;
    xtimer          RenderCPUTime;
    xtimer          RenderTerrainCPUTime;
    xtimer          RenderBuildingsCPUTime;
    xtimer          RenderBuildingsPrepCPUTime;
    xtimer          RenderShapesCPUTime;
    xtimer          RenderSkyCPUTime;
    xtimer          RenderProjectilesCPUTime;
    xtimer          RenderParticlesCPUTime;
    xtimer          FogSetupCPUTime;
    xbool           RenderBuildingGrid;

    fog             Fog;

    // Context (either player or free cam) information.
    s32             NRequestedPlayers;
    s32             NLocalPlayers;
    s32             WC;             // "Which Context"
    player_context  PC[2];          // "Player Context", use PC[WC]
    view            FreeCam[2];     //
    xbool           UseFreeCam[2];

    view&           GetView( void )
    { return( UseFreeCam[WC] ? FreeCam[WC] : PC[WC].View ); }

    view&           GetView( s32 WhichContext )
    { return( UseFreeCam[WhichContext] ? FreeCam[WhichContext] : PC[WhichContext].View ); }

    // Directory of current mission
    char            MissionDir[64];
    char            MissionPrefix[8];
    char            MissionName[32];
    f32             MissionLoadCompletion;

    // Bot info
    s32             NBots;

    // Main loop control variables.
    xbool           ExitApp;        // TRUE: Time to exit the application.    
    xbool           Playing;        // TRUE: Not in the front end.
    xbool           GameRunning;    // TRUE: In a mission (may be loading).
    xbool           GameActive;     // TRUE: Player is active in mission.

    // Which round?
    s32             Round;

    s32             MusicSampleId;  // Music type for this level (or -1)
    // Sound package identifiers for unloading
    s32             MusicId;
    s32             EffectsPackageId;
    s32             MusicPackageId;
    s32             VoicePackageId;
	s32				TrainingPackageId;

    f32             MasterVolume;
    f32             MusicVolume;
    f32             EffectsVolume;
    f32             VoiceVolume;

    // Terrain
    terr*           pTerr;

    // Damage texture system
    damage          Damage ;
    
    // Single player mission success flag
    xbool           MissionSuccess;
    s32             HighestCampaignMission;

    // Environment type
    enum environment
    {
        ENVIRONMENT_DESERT,
        ENVIRONMENT_LUSH,
        ENVIRONMENT_ICE,
        ENVIRONMENT_LAVA,
        ENVIRONMENT_BADLAND,
        
        ENVIRONMENT_TOTAL
    } ;

    environment     Environment ;

    // Post Render FX (for glowy spots and such)
    postfx          PostFX ;
};



//==============================================================================
// GLOBAL FUNCTIONS
//==============================================================================

// Returns TRUE if object is occluded by the terrain (ie. do not draw)
xbool IsOccludedByTerrain( const bbox& BBox, const vector3& EyePos );


// Performs view volume test only
// Returns:
//  view::VISIBLE_NONE    (0) = Not visible
//  view::VISIBLE_FULL    (1) = Visible (no clipping needed)
//  view::VISIBLE_PARTIAL (2) = Partially visible (ie. check for clipping)
s32 IsInViewVolume( const bbox& BBox ) ;

// Performs view volume and occlusion test
// Returns:
//  view::VISIBLE_NONE    (0) = Not visible
//  view::VISIBLE_FULL    (1) = Visible (no clipping needed)
//  view::VISIBLE_PARTIAL (2) = Partially visible (ie. check for clipping)
// NOTE: Only use on objects that are expensive to render. eg. players, vehicles etc.
s32 IsVisible( const bbox& BBox );


//==============================================================================

struct zone_set
{
    byte    BuildingID[4];
    byte    ZoneID[4];
    s32     NZones;
    xbool   InZoneZero;
};

void    ComputeZoneSet( zone_set& ZSet, const bbox& BBox, xbool JustCenter = TRUE );
s32     IsVisibleInZone( const zone_set& ZSet, const bbox& BBox );

//==============================================================================

extern t2_globals tgl;

//==============================================================================
#endif
//==============================================================================

