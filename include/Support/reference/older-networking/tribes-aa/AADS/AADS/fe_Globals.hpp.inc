//==============================================================================
//
//  fe_Globals.hpp
//
//==============================================================================

#ifndef FE_GLOBALS_HPP
#define FE_GLOBALS_HPP

//==============================================================================

#include "Objects/Player/PlayerObject.hpp"

//==============================================================================
// DEFINES
//==============================================================================

#define FE_MAX_PLAYERS          3
#define FE_MAX_WARRIOR_NAME     16
#define FE_TOTAL_FAVORITES      15
#define FE_ARMOR_FAVORITES      5
//#define FE_MAX_MAP_CYCLE        32
#define FE_MAX_SERVER_NAME      16
#define FE_MAX_ADMIN_PASS       16
#define FE_MAX_BUDDIES          10

enum warrior_control_defines
{
    WARRIOR_CONTROL_SHIFT,
    WARRIOR_CONTROL_MOVE,
    WARRIOR_CONTROL_LOOK,
    WARRIOR_CONTROL_FREELOOK,
    WARRIOR_CONTROL_JUMP,
    WARRIOR_CONTROL_FIRE,
    WARRIOR_CONTROL_JET,
    WARRIOR_CONTROL_ZOOM,
    WARRIOR_CONTROL_NEXT_WEAPON,
    WARRIOR_CONTROL_GRENADE,
    WARRIOR_CONTROL_VOICE_MENU,
    WARRIOR_CONTROL_VOICE_CHAT,
    WARRIOR_CONTROL_MINE,
    WARRIOR_CONTROL_USE_PACK,
    WARRIOR_CONTROL_ZOOM_IN,
    WARRIOR_CONTROL_ZOOM_OUT,
    WARRIOR_CONTROL_USE_HEALTH,
    WARRIOR_CONTROL_TARGET_LASER,
    WARRIOR_CONTROL_DROP_WEAPON,
    WARRIOR_CONTROL_DROP_PACK,
    WARRIOR_CONTROL_DEPLOY_BEACON,
    WARRIOR_CONTROL_DROP_FLAG,
    WARRIOR_CONTROL_SUICIDE,
    WARRIOR_CONTROL_OPTIONS,
    WARRIOR_CONTROL_TARGET_LOCK,
    WARRIOR_CONTROL_COMPLIMENT,
    WARRIOR_CONTROL_TAUNT,
    WARRIOR_CONTROL_EXCHANGE,

    WARRIOR_CONTROL_SIZEOF
};

//==============================================================================
//  Mission definition structure
//==============================================================================

struct mission_def
{
    s32             GameType;
    const char*     Folder;
    s32             DisplayNameString;
};

//==============================================================================
//  Warrior Setup structure
//==============================================================================

struct warrior_control_data
{
    s32     ControlID;
    s32     GadgetID;
    xbool   IsShifted;
};

struct warrior_setup
{
    xwchar                          Name[FE_MAX_WARRIOR_NAME];
    player_object::character_type   CharacterType;
    player_object::armor_type       ArmorType;
    player_object::skin_type        SkinType;
    player_object::voice_type       VoiceType;
	s32								WeaponType;
	s32								PackType;
    player_object::loadout          Favorites[FE_TOTAL_FAVORITES];
    s32                             ActiveFavoriteCategory;
    s32                             ActiveFavorite[3];
    s32                             ActiveFavoriteArmor;
    u32                             ViewZoomFactor;
    s32                             LastPauseTab;
    s32                             ControlConfigID;
    player_object::control_info     ControlInfo;
    warrior_control_data            ControlEditData[WARRIOR_CONTROL_SIZEOF];
    xbool                           KeyboardEnabled;
    xbool                           DualshockEnabled;
    s32                             SensitivityX;
    s32                             SensitivityY;
    xbool                           InvertPlayerY;
    xbool                           InvertVehicleY;
    xbool                           View3rdPlayer;
    xbool                           View3rdVehicle;
    xwchar                          Buddies[FE_MAX_BUDDIES][FE_MAX_WARRIOR_NAME];
    s32                             nBuddies;
};

//==============================================================================
//  externals
//==============================================================================

extern const char* gtd_CTF;
extern const char* gtd_CNH;
extern const char* gtd_DM;
extern const char* gtd_TDM;
extern const char* gtd_HUNTER;

extern mission_def fe_Missions[];

//==============================================================================

enum fe_state
{
    FE_STATE_UNINITIALIZED      = 0,                            // Front End has not been initialized

    FE_STATE_LEGAL,                                             // Legal Screen display
    FE_STATE_SIERRA,                                            // Sierra Logo display
    FE_STATE_INEVITABLE,                                        // Inevitable Logo display

    FE_STATE_MAIN_MENU,                                         // Main Menu display

    FE_STATE_GOTO_WARRIOR_SETUP,                                // Setup the Warrior Setup Chain
    FE_STATE_WARRIOR_SETUP,                                     // Warrior Setup display

    FE_STATE_GOTO_ONLINE,                                       // Goto Online Menu
    FE_STATE_GOTO_OFFLINE,                                      // Goto Offline Menu
    FE_STATE_ONLINE,                                            // Online Menu display
    FE_STATE_OFFLINE,                                            // Offline state.

    FE_STATE_GOTO_CAMPAIGN_BRIEFING,                            // Goto Campaign Menu
    FE_STATE_CAMPAIGN_BRIEFING,                                 // Campaign briefing Menu display

    FE_STATE_GOTO_CAMPAIGN,                                     // Goto Campaign Menu
    FE_STATE_CAMPAIGN,                                          // Campaign Menu display

    FE_STATE_ENTER_GAME,                                        // Shutdown and enter the game
    FE_STATE_ENTER_CAMPAIGN,                                    // Shutdown and enter the campaign

    FE_STATE_GOTO_SOUNDTEST,
    FE_STATE_SOUNDTEST,

    FE_STATE_GOTO_OPTIONS,                                      // Select options config
    FE_STATE_OPTIONS,                                           // Options display

    FE_STATE_GOTO_OPTIONS_MAIN_MENU,                            // Select options config
    FE_STATE_OPTIONS_MAIN_MENU,                                 // Options display

    FE_STATE_GOTO_ONLINE_MAIN_MENU,                             // Goto then online part of the main menu.
    FE_STATE_ONLINE_MAIN_MENU,                                  // Online mode.

    FE_STATE_GOTO_OFFLINE_MAIN_MENU,                            // Offline mode.
    FE_STATE_OFFLINE_MAIN_MENU,                                 // Offline mode.

    FE_STATE_GOTO_AUDIO_OPTIONS,                                // Select audio options config.
    FE_STATE_AUDIO_OPTIONS,                                     // Audio options display.

    FE_STATE_GOTO_NETWORK_OPTIONS,                              // Select network options config.
    FE_STATE_NETWORK_OPTIONS,                                   // Network options display.

    FE_STATE_GAME,                                              // Game is playing
};

enum fe_gamemode
{
    FE_MODE_NULL,
    FE_MODE_ONLINE_ONE_PLAYER,
    FE_MODE_ONLINE_TWO_PLAYER,
    FE_MODE_OFFLINE_ONE_PLAYER,
    FE_MODE_OFFLINE_TWO_PLAYER,
    FE_MODE_CAMPAIGN
};

//==============================================================================
//  Server Settings
//==============================================================================

struct server_settings
{
    s32                     GameType;                           // Game Type
    s32                     MapIndex;                           // Map Index

    s32                     MaxPlayers;                         // Maximum Players on server
    s32                     MaxClients;                         // Maximum clients connected to server

    xbool                   TargetLockEnabled;                  // TRUE for target lock server

    xbool                   BotsEnabled;                        // if TURE the following values apply
    s32                     uiBotsDifficulty;                   // 0->10
    f32                     BotsDifficulty;                     // 0->1
    s32                     BotsNum;                            // Number of Bots

    s32                     uiTeamDamage;                       // 0->100
    f32                     TeamDamage;                         // 0->1

    s32                     TimeLimit;                          // 10 -> 60
    s32                     VotePass;                           // 10 -> 90
    s32                     VoteTime;                           // 10 -> 30
    s32                     ObserverTimeout;                    // 60 -> 120
    s32                     WarmupTime;                         //  5 -> 20

//    s32                     NumMapsInCycle;                     // Number of maps in cycle
//    s32                     MapCycle[FE_MAX_MAP_CYCLE];         // Map Cycle

    xwchar                  ServerName[FE_MAX_SERVER_NAME];     // Server Name
    xwchar                  AdminPassword[FE_MAX_ADMIN_PASS];   // Admin Password
};

/*
struct saved_filter
{
    s32                     Checksum;
    xwchar                  BuddyString[32];
    s16                     MinPlayers;
    s16                     MaxPlayers;
    s16                     GameType;
};
*/
struct saved_options
{
    s32                     Checksum;
    f32                     MusicVolume;
    f32                     EffectsVolume;
    f32                     VoiceVolume;
    f32                     MasterVolume;
    xbool                   SurroundEnabled;
    xbool                   AutoConnect;
    s16                     LastNetConfig;
    server_settings         ServerSettings;
//    saved_filter            LastFilter;
    s32                     SortKey;
	xbool					AutoSave;
    s32                     HighestCampaign;
	s8						UseAutoAimHint;
	s8						UseAutoAimHelp;
};

//==============================================================================
//  fegl
//==============================================================================

struct t2_fe_globals
{
    fe_state                State;                              // State of the Front End
    fe_state                GotoGameState;                      // Game State to goto after warrior select

    s32                     UIUserID;                           // UserID for ui system
    xbool                   MissionLoadDialogActive;            // Mission Load Dialog exists on dialog stack
    xbool                   MissionEndDialogActive;             // Mission End Dialog exists on dialog stack

    s32                     nLocalPlayers;                      // Number of local players selected
    s32                     iWarriorSetup;                      // Index of Warrior currently being setup
    warrior_setup           WarriorSetups[FE_MAX_PLAYERS];      // Warrior setups

    mission_def*            pMissionDef;                        // Pointer to mission def to run
    mission_def*            pNextMissionDef;                    // Pointer to next mission to run
    char                    MissionName[32];                    // Folder in which to find the mission
    s32                     GameType;                           // See GameMgr for enum
    s32                     LastPlayedCampaign;                 // Index of the last played mission
    s32                     CampaignMission;                    // 1->13
    s32                     NCampaignMissions;                  // Total number of campaign missions
    s32                     NTrainingMissions;                  // Total number of training missions

    server_settings         ServerSettings;                     // See above
//    saved_filter            CurrentFilter;

    xbool                   AudioSurround;                      // TRUE for surround mode
    xbool                   AutoConnect;                        // TRUE if to automatically connect to the net on startup
    s32                     LastNetConfig;
    u32                     PortNumber;
    s32                     SortKey;
	xbool					WarriorModified;					// TRUE if warrior has been changed
	xbool					OptionsModified;					// TRUE if options have been modified
	xbool					IngameModified;						// TRUE if in-game data has been modified
	xbool					FilterModified;						// TRUE if a filter has been modified
	xbool					AutoSaveEnabled;					// TRUE if we can autosave without asking
    xbool                   BotInvulnerable;
    xbool                   PlayerInvulnerable;
	xbool					DeliberateDisconnect;				// TRUE if we're doing a deliberate disconnect from the network (no dialog will appear)
	xbool					ModemConnection;					// TRUE if we can detect we're on a modem

    fe_gamemode             GameMode;                           // See enum above
    xbool                   GameModeOnline;                     // TRUE = online game mode
    xbool                   GameHost;                           // TRUE = was server, FALSE = was client
};

//==============================================================================

extern t2_fe_globals fegl;

//==============================================================================
#endif //FE_GLOBALS_HPP
//==============================================================================

