//==============================================================================
//
//  GameLogic.hpp
//
//==============================================================================

#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

//
// Destroy Enemy Assets
//

#define SCORE_DESTROY_ENEMY_GEN             3
#define SCORE_DESTROY_ENEMY_VSTATION        3
#define SCORE_DESTROY_ENEMY_FIX_INVEN       2
#define SCORE_DESTROY_ENEMY_FIX_TURRET      2
#define SCORE_DESTROY_ENEMY_FIX_SENSOR      2
#define SCORE_DESTROY_ENEMY_DEP_INVEN       1
#define SCORE_DESTROY_ENEMY_DEP_TURRET      1
#define SCORE_DESTROY_ENEMY_DEP_SENSOR      1
#define SCORE_DESTROY_ENEMY_DEP_BEACON      0
#define SCORE_DESTROY_SHRIKE                2
#define SCORE_DESTROY_THUNDERSWORD          3
#define SCORE_DESTROY_HAVOC                 2
#define SCORE_DESTROY_WILDCAT               2
#define SCORE_DESTROY_BEOWULF               3
#define SCORE_DESTROY_JERICHO               5

//
// Disable Team Assets
//

#define SCORE_DISABLE_TEAM_GEN             -5
#define SCORE_DISABLE_TEAM_VSTATION        -5
#define SCORE_DISABLE_TEAM_FIX_INVEN       -5
#define SCORE_DISABLE_TEAM_FIX_TURRET      -5
#define SCORE_DISABLE_TEAM_FIX_SENSOR      -5
#define SCORE_DISABLE_TEAM_DEP_INVEN       -1
#define SCORE_DISABLE_TEAM_DEP_TURRET      -1
#define SCORE_DISABLE_TEAM_DEP_SENSOR      -1

//
// Deploy Team Assets
//

#define SCORE_DEPLOY_TEAM_DEP_INVEN         1
#define SCORE_DEPLOY_TEAM_DEP_TURRET        1
#define SCORE_DEPLOY_TEAM_DEP_SENSOR        1

//
// Defend Team Assets
//

#define SCORE_DEFEND_TEAM_GEN               2
#define SCORE_DEFEND_TEAM_SWITCH            2

//
// Repair Team Assets
//

#define SCORE_REPAIR_TEAM_GEN               2
#define SCORE_REPAIR_TEAM_VSTATION          2
#define SCORE_REPAIR_TEAM_FIX_INVEN         2
#define SCORE_REPAIR_TEAM_FIX_TURRET        2
#define SCORE_REPAIR_TEAM_FIX_SENSOR        2
#define SCORE_REPAIR_TEAM_DEP_INVEN         1
#define SCORE_REPAIR_TEAM_DEP_TURRET        1
#define SCORE_REPAIR_TEAM_DEP_SENSOR        1

//
// Kill Players
//

#define SCORE_PLAYER_SUICIDE               -1
#define SCORE_PLAYER_DEATH                  0
#define SCORE_DM_PLAYER_DEATH              -1
#define SCORE_KILL_TEAM_PLAYER             -2
#define SCORE_KILL_ENEMY_PLAYER             1

//
// Miscellaneous
//

#define SCORE_ACQUIRE_SWITCH                2
#define SCORE_BEACON_ENEMY_ASSET            2
#define SCORE_FLARE_MISSILE                 1

//
// CTF
//

#define SCORE_CAPTURE_FLAG_TEAM           100
#define SCORE_CAPTURE_FLAG_PLAYER          10
#define SCORE_TAKE_FLAG_TEAM                1
#define SCORE_TAKE_FLAG_PLAYER              1
#define SCORE_DEFEND_FLAG                   3
#define SCORE_RETURN_FLAG                   2
#define SCORE_DEFEND_ALLY_CARRIER           2
#define SCORE_KILL_ENEMY_CARRIER            2

//
// CNH
//

#define SCORE_HOLD_SWITCH                   5
#define SCORE_HOLD_SWITCH_PER_SEC           1

//
// Hunter
//

#define SCORE_CASH_IN_FLAG                  1

//==============================================================================
//  TYPES
//==============================================================================

class game_mgr;

//==============================================================================

class game_logic
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------
   
public:
                game_logic      ( void );
virtual        ~game_logic      ( void );
                
virtual void    RegisterFlag    ( const vector3& Position, radian Yaw, s32 FlagTeam );
virtual void    RegisterItem    ( object::id ItemID, const char* pTag );

virtual void    PlayerSpawned   ( object::id PlayerID );
virtual void    PlayerDied      ( const pain& Pain );
virtual void    PlayerOnVehicle ( object::id PlayerID, object::id VehicleID );
virtual void    ItemDisabled    ( object::id ItemID,   object::id OriginID  );   
virtual void    ItemDestroyed   ( object::id ItemID,   object::id OriginID  );
virtual void    ItemEnabled     ( object::id ItemID,   object::id PlayerID  );
virtual void    ItemDeployed    ( object::id ItemID,   object::id PlayerID  );
virtual void    ItemAutoLocked  ( object::id ItemID,   object::id PlayerID  );
virtual void    ItemRepairing   ( object::id ItemID,   object::id PlayerID  ); 
virtual void    ItemRepaired    ( object::id ItemID,   object::id PlayerID, xbool Score );
virtual void    PickupTouched   ( object::id PickupID, object::id PlayerID  );
virtual void    FlagTouched     ( object::id FlagID,   object::id PlayerID  );
virtual xbool   FlagHitPlayer   ( object::id FlagID,   object::id PlayerID  );
virtual void    FlagTimedOut    ( object::id FlagID   );
virtual void    NexusTouched    ( object::id PlayerID );
virtual void    ThrowFlags      ( object::id PlayerID );
virtual void    StationUsed     ( object::id PlayerID );
virtual void    WeaponFired     ( object::id PlayerID = obj_mgr::NullID );
virtual void    WeaponExchanged ( object::id PlayerID );
virtual void    SatchelDetonated( object::id PlayerID );

virtual void    GetFlagStatus   ( u32 TeamBits, s32& Status, vector3& Position );
virtual object::id GetFlagID    ( s32 Index );

virtual void    Render          ( void );

//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------
   
protected:

virtual void    Initialize      ( void );

// ATTENTION:  This function call MUST set m_Score.Player[?].Team and take
//             care of m_Score.Team[?].Size.
virtual void    Connect         ( s32 PlayerIndex );
virtual void    EnterGame       ( s32 PlayerIndex );
virtual void    ExitGame        ( s32 PlayerIndex );
virtual void    Disconnect      ( s32 PlayerIndex );

virtual void    ChangeTeam      ( s32 PlayerIndex );

virtual void    AdvanceTime     ( f32 DeltaTime );
virtual void    BeginGame       ( void );
virtual void    EndGame         ( void );

virtual void    EnforceBounds   ( f32 DeltaTime );

virtual void    SwitchTouched   ( object::id SwitchID, object::id PlayerID  );

virtual void    AcceptUpdate    ( const bitstream& BitStream );
virtual void    ProvideUpdate   (       bitstream& BitStream, u32& DirtyBits );

//      void    WriteFinalScore ( void );
//rtual void    WriteScoreData  ( X_FILE* pFile );

    s32         IndexFromID     ( object::id PlayerID );
    object::id  IDFromIndex     ( s32 PlayerIndex );
    s32         BitsToTeam      ( u32 TeamBits );

    xbool       IsDefending     ( object::type  Type, 
                                  object::id    PlayerID, 
                                  object::id    OriginID );

//------------------------------------------------------------------------------
//  Private Data
//------------------------------------------------------------------------------

        xbool   m_PlayerDead[32];
        f32     m_TimeDead  [32];

//------------------------------------------------------------------------------
//  Friends
//------------------------------------------------------------------------------

    friend game_mgr;
};

//==============================================================================
#endif // CTF_LOGIC_HPP
//==============================================================================
