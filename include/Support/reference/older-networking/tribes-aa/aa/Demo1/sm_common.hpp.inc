//==============================================================================
//
//  sm_common.hpp
//
//==============================================================================
#ifndef SM_COMMON_HPP
#define SM_COMMON_HPP
//==============================================================================

enum state_enum
{
    STATE_NULL,
    STATE_COMMON_INIT,
    STATE_COMMON_INIT_TITLES,
    STATE_COMMON_INIT_FRONTEND,
    STATE_COMMON_FRONTEND,
    
    STATE_SERVER_INIT_SERVER,
    STATE_SERVER_LOAD_MISSION,
    STATE_SERVER_INGAME,
    STATE_SERVER_COOL_DOWN,
    STATE_SERVER_SHUTDOWN,
    
    STATE_CAMPAIGN_INIT_CAMPAIGN,
    STATE_CAMPAIGN_LOAD_MISSION,
    STATE_CAMPAIGN_INGAME,
    STATE_CAMPAIGN_COOL_DOWN,
    STATE_CAMPAIGN_SHUTDOWN,
    
    STATE_CLIENT_INIT_CLIENT,
    STATE_CLIENT_REQUEST_MISSION,
    STATE_CLIENT_LOAD_MISSION,
    STATE_CLIENT_SYNC,
    STATE_CLIENT_INGAME,
    STATE_CLIENT_UNLOAD,
    STATE_CLIENT_RETURN_TO_FE,
    STATE_CLIENT_DISCONNECT,

    // NOTE: If the states are altered, update sm_GetState() accordingly.
};

//==============================================================================

struct sm_common
{
    state_enum      CurrentState;
    state_enum      NewState;

    f32             SecInState;
    
    xbool           MissionRunning;
    xbool           MissionLoading;
    xbool           MissionLoaded;

    xbool           ClientKicked;
    xbool           ClientQuit;
    xbool           ServerMissionEnded;
    xbool           ServerShutdown;
    xbool           Disconnected;
    xbool           CampaignMissionEnded;
    xbool           LoginFailed;
	xbool			ServerTooBusy;
	xbool			TooManyClients;
	xbool			NetworkDown;
    xbool           ClientBanned;
    xbool           InvalidPassword;
    xbool           ServerFull;
};

//==============================================================================

void        sm_Time           ( f32 DeltaSec );
void        sm_SetState       ( state_enum StateType );
void        sm_UnloadMission  ( void );
const char* sm_GetState       ( void );

//==============================================================================

extern sm_common g_SM;
extern char* MissionPrefix[];

//==============================================================================
//==============================================================================
//==============================================================================
// PRIVATE
//==============================================================================
//==============================================================================
//==============================================================================

typedef void begin_fn( void );
typedef void end_fn( void );
typedef void advance_fn( f32 DeltaTime );

struct sm_state
{
    state_enum      StateType;
    begin_fn*       pBeginFunc;
    advance_fn*     pAdvanceFunc;
    end_fn*         pEndFunc;
};

//==============================================================================
#endif
//==============================================================================

