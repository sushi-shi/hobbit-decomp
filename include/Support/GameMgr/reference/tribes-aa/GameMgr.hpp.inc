//==============================================================================
//
//  GameMgr.hpp
//
//==============================================================================

#ifndef GAMEMGR_HPP
#define GAMEMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "GameLogic.hpp"
#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "Objects\Projectiles\Turret.hpp"
#include "Objects\Player\PlayerObject.hpp"

//==============================================================================
//  TYPES
//==============================================================================

enum game_type
{
    GAME_NONE,
    GAME_CTF,
    GAME_CNH,
    GAME_TDM,
    GAME_DM,
    GAME_HUNTER,
    GAME_CAMPAIGN,
};

//==============================================================================

struct player_score
{
    xwchar  Name[16];           // Player bit.
    xbool   IsConnected;        // Player bit.
    xbool   IsInGame;           // Player bit.
    xbool   IsBot;              // Player bit.
    xbool   IsMale;             // Player bit.
    s32     Team;               // Player bit.
    s32     Machine;            // Player bit.

    s32     Score;              // Score  bit.
    s32     Kills;              // Score  bit.
    s32     Deaths;             // Score  bit.

    s32     TKs;                // Not sent.
    s32     Votes;              // Not sent.
    xbool   KickPending;        // Not sent.
    xbool   IsSplit;            // Not sent.
    char    NName[20];          // Not sent.

    f32     Recon;              // Not sent.  Reconnect data.
    s32     NetID;              // Not sent.  Reconnect data.
    u32     IPAddr;             // Not sent.
};

//==============================================================================

struct team_score
{
    xwchar  Name[16];           // Not sent.
    char    NName[20];          // Not sent.
    s32     Size;               // Not sent.
    s32     Humans;             // SPECIFIC_DATA
    s32     Bots;               // SPECIFIC_DATA
    s32     Score;              // SPECIFIC_DATA
};

//==============================================================================

struct game_score
{
    xwchar          MissionName[64];    // GENERIC_INIT
    game_type       GameType;           // GENERIC_INIT
    xbool           IsGameOver;         // [<any dirty bit> by GENERIC]
    xbool           IsTeamBased;        // Set internally on client.
    char            NMissionName[64];   // Not sent.
    team_score      Team[2];            // (see above)
    player_score    Player[32];         // (see above)
};

//==============================================================================

struct player_options
{
    f32                             TVRefreshRate;
    player_object::character_type   Character;
    player_object::skin_type        Skin;
    player_object::voice_type       Voice;
    xbool                           Bot;
};

//==============================================================================

char* MakeNarrowString( char* pNarrow, const xwchar* pWide, s32 BufferSize = 1024 );

//==============================================================================

class ctf_logic;
class cnh_logic;
class tdm_logic;
class dm_logic;
class hunt_logic;

//==============================================================================

class game_mgr
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:

                game_mgr        ( void );
               ~game_mgr        ( void );

    void        DebugRender     ( void );
                
    void        AdvanceTime     ( f32 DeltaTime );
    void        UpdateSensorNet ( void );
                
    void        BeginGame       ( void );
    void        EndGame         ( void );   // Force the game to end.
    xbool       GameInProgress  ( void );
                
    void        SetGameType     ( game_type GameType, s32 CampaignMission = 0 );
    void        ChangeGameType  ( game_type GameType );
    game_type   GetGameType     ( void );

    void        SetNCampaignMissions( s32 NMissions );
    s32         GetNCampaignMissions( void );
    s32         GetCampaignMission  ( void );
    xbool       IsCampaignMission   ( void );
    xbool       IsTeamBasedGame     ( void );

    void        SetTimeLimit    ( s32 Minutes );
    void        SetScoreLimit   ( s32 MaxScore );
    void        SetPlayerLimits ( s32 MaxPlayers, s32 MinPlayers, xbool BotsEnabled );
    void        SetVehicleLimits( s32 MaxCycles,  s32 MaxShrikes, s32 MaxBombers, s32 MaxTransports );

    xbool       RoomForPlayers  ( s32 Players );
    void        Connect         ( s32&                          PlayerIndex, 
                                  s32                           Machine,
                                  s32                           NetID,
                                  u32                           IPAddress,
                                  const xwchar*                 pName,
                                  f32                           TVRefreshRate,
                                  player_object::character_type Character,
                                  player_object::skin_type      Skin,
                                  player_object::voice_type     Voice,
                                  xbool                         Bot,
                                  xbool                         SplitScreen );
    void        Disconnect      ( s32  PlayerIndex );
    object::id  EnterGame       ( s32  PlayerIndex );
    void        ExitGame        ( s32  PlayerIndex );
                
    void        InitiateKickVote( s32  PlayerIndex, s32 StarterIndex );
    void        InitiateMapVote ( s32  MapIndex,    s32 StarterIndex );
    void        CastVote        ( s32  PlayerIndex, xbool Vote );
    void        ChangeTeam      ( s32  PlayerIndex, xbool ByAdmin = TRUE );
    void        KickPlayer      ( s32  PlayerIndex, xbool ByAdmin = TRUE );
    void        ChangeMap       ( s32  MapIndex,    xbool ByAdmin = TRUE );
    s32         GetKickTarget   ( void );

    void        SetBounds       ( const bbox& Bounds );
    void        SetStartupPower ( void );
    void        InitPower       ( s32* pPowerCount );
    void        InitSwitch      ( u32* pSwitchBits );
                
    void        GetDirtyBits    ( u32& DirtyBits, u32& PlayerBits, u32& ScoreBits );
    void        ClearDirtyBits  ( void );
    void        AcceptUpdate    ( const bitstream& BitStream, f32 AvgPing );
    void        ProvideUpdate   (       bitstream& BitStream, 
                                  u32& DirtyBits, u32& PlayerBits, u32& ScoreBits );
                
    void        ProvideFinalScore   ( bitstream& BitStream );
    void        AcceptFinalScore    ( bitstream& BitStream );

    void        SetTeamDamage   ( f32 TeamDamage );
    f32         GetTeamDamage   ( void );
                
    u32         GetTeamBits     ( s32 PlayerIndex );
    u32         GetSwitchBits   ( s32 SwitchCircuit );
    xbool       GetPower        ( s32 PowerCircuit );
                
    const xwchar* GetTeamName   ( u32 TeamBits );

    s32         RandomTeamMember( s32 Team );

    void        PowerLoss       ( s32 PowerCircuit );
    void        PowerGain       ( s32 PowerCircuit );
    void        SwitchTouched   ( object::id SwitchID, object::id PlayerID );
                
    bbox        GetBounds       ( void );    
                
    s32         GetTimeLimit    ( void );
    f32         GetTimeRemaining( void );
    f32         GetGameProgress ( void );
    void        ScoreForPlayer  ( object::id PlayerID, s32 Score, s32 Message = -1 );

    void        GetPlayerCounts ( s32& Humans, s32& Bots, s32& Maximum );

    xbool       IsPlayerInVote  ( s32 PlayerIndex );
    xbool       GetVoteData     ( const xwchar*& pMessage, 
                                        s32&     For, 
                                        s32&     Against, 
                                        s32&     Missing, 
                                        s32&     PercentNeeded );

const game_score&   GetScore    ( void );

    void        RenderVolume    ( void );

//  void        SetReport       ( xbool  Enabled );
//  xbool       GetReport       ( void );

//  void        SetReportDir    ( const char* pDir );
//nst char*     GetReportDir    ( void );


    void        GetDeployStats      (       object::type  Type, 
                                            u32           TeamBits,
                                            s32&          CurrentCount,
                                            s32&          MaximumCount );

    xbool       AttemptDeployTurret (       object::id    PlayerID,   
                                      const vector3&      Point,      
                                      const vector3&      Normal,     
                                            object::type  SurfaceType,
                                            xbool         TestOnly = FALSE );

    xbool       AttemptDeployStation(       object::id    PlayerID,   
                                      const vector3&      Point,      
                                      const vector3&      Normal,     
                                            object::type  SurfaceType,
                                            xbool         TestOnly = FALSE );

    xbool       AttemptDeploySensor (       object::id    PlayerID,   
                                      const vector3&      Point,      
                                      const vector3&      Normal,     
                                            object::type  SurfaceType,
                                            xbool         TestOnly = FALSE );

    xbool       AttemptDeployBeacon (       object::id    PlayerID,   
                                      const vector3&      Point,      
                                      const vector3&      Normal,     
                                            object::type  SurfaceType,
                                            xbool         TestOnly = FALSE );

    xbool       AttemptDeployBarrel (       object::id    PlayerID,   
                                      const vector3&      Point,      
                                      const vector3&      Ray,     
                                            turret::kind  Kind );

//------------------------------------------------------------------------------
//  Private Types
//------------------------------------------------------------------------------

    enum 
    {
        DIRTY_GENERIC_INIT        = 0x00000001,
        DIRTY_GENERIC_SWITCH      = 0x00000002,
        DIRTY_GENERIC_POWER       = 0x00000004,
        DIRTY_GENERIC_TIME        = 0x00000008,
        DIRTY_GENERIC_VOTE_NEW    = 0x00000010,
        DIRTY_GENERIC_VOTE_UPDATE = 0x00000020,

        DIRTY_SPECIFIC_INIT       = 0x00010000,
        DIRTY_SPECIFIC_DATA       = 0x00020000,
                                  
        DIRTY_ALL                 = 0x0003003F,
    };

//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------

protected:

    void    CreatePlayer        ( s32 PlayerIndex );
                                
    void    EnforceBounds       ( f32 DeltaTime );
    void    BalanceTeams        ( f32 DeltaTime );
    void    ShuffleTeams        ( void );
    void    UpdateTeamNames     ( void );
                                
    void    SendAudio           ( s32 Sound,  const vector3& Position );
    void    SendAudio           ( s32 Sound,  s32 PlayerIndex );
    void    SendAudio           ( s32 Sound,  u32 TeamBits = 0xFFFFFFFF );
    void    SendAudio           ( s32 Sound1, u32 TeamBits1, s32 Sound2 );
                                
    void    UpdateBotCount      ( void );    
    void    AddBot              ( void );
    void    RemoveBot           ( void );
                                
    void    RegisterName        ( s32 PlayerIndex, const xwchar* pName );

    void    VotePasses          ( void );
    void    VoteFails           ( void );
    void    AnnounceVotePasses  ( void );
    void    AnnounceVoteFails   ( void );
    void    ExecuteVote         ( void );
    void    EndVoting           ( void );
    void    ShutDownVote        ( void );  
    void    BuildVoteMessage    ( void );

//------------------------------------------------------------------------------
//  Private Storage
//------------------------------------------------------------------------------

protected:

    // The Initialized flag is static to ensure that only one game manager is 
    // ever instantiated.

static  xbool   m_Initialized;

    xbool       m_DebugRender;
    
    game_type   m_GameType;                     // GENERIC_INIT
    f32         m_TeamDamageFactor;             // GENERIC_INIT
    xbool       m_Campaign;                     // Not sent.
    s32         m_CampaignMission;              // Not sent.
    s32         m_NCampaignMissions;            // Not sent.
                                                                          
    game_score  m_Score;                        // (see above)
    game_score  m_FinalScore;                   // Only used between games.

    s32         m_NPlayers;                     // Maintained locally.
    s32         m_NHumans;                      // Maintained locally.
    s32         m_NBots;                        // Maintained locally.

    s32         m_MaxPlayers;                   // Not sent.
    s32         m_MinPlayers;                   // Not sent.
    xbool       m_BotsEnabled;                  // Not sent.

    f32         m_TimeRemaining;                // GENERIC_TIME
    f32         m_TimeUpdateTimer;              // Not sent.
    xbool       m_GameInProgress;               // Not sent.

    s32         m_MaxScore;                     // Not sent.
    s32         m_TimeLimit;                    // DIRTY_GENERIC_INIT (minutes)

    s32         m_VehicleLimit[6];              // Not sent.

    u32         m_DirtyBits;                    // Not sent.
    u32         m_PlayerBits;                   // Not sent.
    u32         m_ScoreBits;                    // Not sent.

    u32         m_SwitchBits[16];               // GENERIC_SWITCH
    s32         m_PowerCount[16];               // GENERIC_POWER

    bbox        m_Bounds;                       // Set locally.
    bbox        m_DeathBounds;                  // Set locally.

    object::id  m_PlayerID[32];                 // Player bit.

    player_options  m_Options[32];              // Not sent.
    s32             m_DefaultLoadOut;           // Not sent.

    s32         m_BotNameBase;                  // Not sent.
    s32         m_BioDermOffset;                // Not sent.

    xbool       m_Said60;                       // Not sent.
    xbool       m_Said30;                       // Not sent.
    xbool       m_Said10;                       // Not sent.

    xbool       m_VoteInProgress;               // GENERIC_VOTE_NEW
    s32         m_KickPlayer;                   // GENERIC_VOTE_NEW
    s32         m_ChangeMap;                    // GENERIC_VOTE_NEW
    u32         m_VotePlayers;                  // GENERIC_VOTE_NEW
    s32         m_VotePercent;                  // GENERIC_VOTE_NEW 0-100
    s32         m_VotesFor;                     // GENERIC_VOTE_UPDATE
    s32         m_VotesAgainst;                 // GENERIC_VOTE_UPDATE
    s32         m_VotesMissing;                 // GENERIC_VOTE_UPDATE
    xbool       m_VoteHasPassed;                // Not sent.
    f32         m_VoteTimer;                    // Not sent.
    u32         m_VoteBits;                     // Not sent.
    xwchar      m_VoteMessage[64];              // Not sent.

//------------------------------------------------------------------------------
//  Friends
//------------------------------------------------------------------------------

    friend game_logic;
    friend ctf_logic;
    friend cnh_logic;
    friend tdm_logic;
    friend dm_logic;
    friend hunt_logic;
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern game_mgr     GameMgr;
extern game_logic*  pGameLogic;

//==============================================================================
#endif // GAMEMGR_HPP
//==============================================================================
