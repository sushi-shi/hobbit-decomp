//==============================================================================
//
//  GameLogic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "GameLogic.hpp"
#include "GameMgr.hpp"
#include "MsgMgr.hpp"
#include "Objects\Projectiles\Generator.hpp"
#include <Support/StringMgr/StringMgr.hpp>

#include "AADS\fe_globals.hpp"
#include "AADS\Data\UI\Messages.h"

//==============================================================================
//  DEFINES
//==============================================================================

#define BEACON_RADIUS  2
#define DEFEND_RADIUS 10

#define CHANCE(n)   (x_irand(1,100) <= (n))

//==============================================================================
//  STORAGE
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================

s32 game_logic::IndexFromID( object::id PlayerID )
{
    s32 Index = PlayerID.Slot;

    if( (Index >= 0) && 
        (Index < 32) &&
        (GameMgr.m_Score.Player[Index].IsInGame) && 
        (GameMgr.m_PlayerID[Index] == PlayerID) &&
        (ObjMgr.GetObjectFromID(PlayerID)) )
    {
        return( Index );
    }

    return( -1 );
}

//==============================================================================

object::id game_logic::IDFromIndex( s32 PlayerIndex )
{
    if( (PlayerIndex >= 0) && (PlayerIndex < 32) )
        return( GameMgr.m_PlayerID[ PlayerIndex ] );
    else
        return( obj_mgr::NullID );
}

//==============================================================================

s32 game_logic::BitsToTeam( u32 TeamBits )
{
    s32 Team = 0;
    while( (TeamBits) && ((TeamBits & 0x01) == 0) )
    {
        TeamBits >>= 1;
        Team      += 1;
    }

    return( Team );
}

//==============================================================================

game_logic::game_logic( void )
{
}

//==============================================================================

game_logic::~game_logic( void )
{
}

//==============================================================================

void game_logic::AcceptUpdate( const bitstream&  )
{
}

//==============================================================================

void game_logic::ProvideUpdate( bitstream&, u32& DirtyBits )
{
    (void)DirtyBits;
}

//==============================================================================

void game_logic::AdvanceTime( f32 DeltaTime )
{
    for( s32 i = 0; i < 32; i++ )
        if( m_PlayerDead[i] )
            m_TimeDead[i] += DeltaTime;
}

//==============================================================================

void game_logic::BeginGame( void )
{
    for( s32 i = 0; i < 32; i++ )
    {
        m_PlayerDead[i] = FALSE;
        m_TimeDead  [i] = 0.0f;
    }
}

//==============================================================================

void game_logic::EndGame( void )
{
}

//==============================================================================

void game_logic::EnforceBounds( f32 DeltaTime )
{
    (void)DeltaTime;
}

//==============================================================================

void game_logic::Connect( s32 PlayerIndex )
{
    (void)PlayerIndex;
}

//==============================================================================

void game_logic::EnterGame( s32 PlayerIndex )
{
    (void)PlayerIndex;
}

//==============================================================================

void game_logic::ExitGame( s32 PlayerIndex )
{
    (void)PlayerIndex;
}

//==============================================================================

void game_logic::Disconnect( s32 PlayerIndex )
{
    (void)PlayerIndex;
}

//==============================================================================

void game_logic::ChangeTeam( s32 PlayerIndex )
{
    (void)PlayerIndex;
}

//==============================================================================

void game_logic::RegisterFlag( const vector3& Position, radian Yaw, s32 FlagTeam )
{
    (void)Position;
    (void)Yaw;
    (void)FlagTeam;
}

//==============================================================================

void game_logic::RegisterItem( object::id ItemID, const char* pTag )
{
    (void)ItemID;
    (void)pTag;
}

//==============================================================================

xbool game_logic::IsDefending( object::type Type, object::id PlayerID, object::id OriginID )
{
    if( !tgl.ServerPresent )
        return( FALSE );

    object* pVictim = ObjMgr.GetObjectFromID( PlayerID );
    object* pKiller = ObjMgr.GetObjectFromID( OriginID );
    object* pObject;
    
    if( !pKiller )
        return( FALSE );

    ASSERT( pVictim );

    if( !(pVictim->GetAttrBits() & object::ATTR_PLAYER) ||
        !(pKiller->GetAttrBits() & object::ATTR_PLAYER) )
        return( FALSE );

    ObjMgr.StartTypeLoop( Type );
    while( (pObject = ObjMgr.GetNextInType()) != NULL )
    {
        f32 DistanceSq = (pVictim->GetPosition() - pObject->GetPosition()).LengthSquared();
        
        // If...
        //  - victim within radius around object
        //  - object and killer are on SAME team
        //  - object and victim are NOT on same team
        if( (DistanceSq < (DEFEND_RADIUS * DEFEND_RADIUS)) && 
            ((pObject->GetTeamBits() & pKiller->GetTeamBits()) != 0x00) && 
            ((pObject->GetTeamBits() & pVictim->GetTeamBits()) == 0x00) )
        {
            break;
        }
    }     
    ObjMgr.EndTypeLoop();

    return( pObject != NULL );
}

//==============================================================================

void game_logic::PlayerSpawned( object::id PlayerID )
{
    (void)PlayerID;

    m_PlayerDead[ PlayerID.Slot ] = FALSE;
}

//==============================================================================

void game_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    MsgMgr.PlayerDied( Pain );

    s32 PlayerIndex = IndexFromID( Pain.VictimID );
    s32 KillerIndex = IndexFromID( Pain.OriginID );

    GameMgr.m_Score.Player[PlayerIndex].Deaths += 1;
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);

    m_PlayerDead[ PlayerIndex ] = TRUE;
    m_TimeDead  [ PlayerIndex ] = 0.0f;

    // Process the death.
    if( KillerIndex == -1 )
    {
        // Killer was not a player.  Was it another object?  Or a bad landing?
        if( Pain.OriginID == obj_mgr::NullID )
        {
            // Some other object.
        }
        else
        {
            // Bad landing.
        }
        
        GameMgr.ScoreForPlayer( Pain.VictimID, SCORE_PLAYER_DEATH );
    }
    else if( KillerIndex == PlayerIndex )
    {
        // Self inflicted death.
        GameMgr.ScoreForPlayer( Pain.VictimID, SCORE_PLAYER_SUICIDE, MSG_SCORE_SUICIDE );
    }
    else
    {
        // Death by somebody else's hand!  Same team?
        if( GameMgr.m_Score.Player[PlayerIndex].Team == GameMgr.m_Score.Player[KillerIndex].Team )
        {
            // Killed by teammate.
            GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_KILL_TEAM_PLAYER, MSG_SCORE_TEAMKILL );
            GameMgr.m_Score.Player[KillerIndex].TKs += 1;
        }
        else
        {
            // Killed by enemy.
            GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_KILL_ENEMY_PLAYER );
            GameMgr.m_Score.Player[KillerIndex].Kills += 1;
            GameMgr.m_ScoreBits |= (1 << KillerIndex);
            
            if( IsDefending( object::TYPE_GENERATOR, Pain.VictimID, Pain.OriginID ))
            {
                GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_DEFEND_TEAM_GEN, MSG_SCORE_DEFEND_TEAM_GEN );
            }
            
            if( IsDefending( object::TYPE_FLIPFLOP, Pain.VictimID, Pain.OriginID ))
            {
                GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_DEFEND_TEAM_SWITCH, MSG_SCORE_DEFEND_TEAM_SWITCH );
            }
        }
    }
}

//==============================================================================

void game_logic::PlayerOnVehicle( object::id PlayerID, object::id VehicleID )
{
    (void)PlayerID;
    (void)VehicleID;
}

//==============================================================================

void game_logic::ItemDisabled( object::id ItemID, object::id OriginID ) 
{
    if( !tgl.ServerPresent )
        return;

    object* pPlayer = ObjMgr.GetObjectFromID( OriginID );
    object* pItem   = ObjMgr.GetObjectFromID(  ItemID  );
    s32     Score   =  0;
    s32     Message = -1;

    if( !pPlayer )
        return;

    ASSERT( pItem );

    if( !(pPlayer->GetAttrBits() & object::ATTR_PLAYER) )
        return;

    if( pPlayer->GetTeamBits() & pItem->GetTeamBits() )
    {
        switch( pItem->GetType() )
        {
            case object::TYPE_GENERATOR            : Score = SCORE_DISABLE_TEAM_GEN;         Message = MSG_SCORE_DISABLE_TEAM_GEN;          break;
            case object::TYPE_STATION_VEHICLE      : Score = SCORE_DISABLE_TEAM_VSTATION;    Message = MSG_SCORE_DISABLE_TEAM_VSTATION;     break;
            case object::TYPE_STATION_FIXED        : Score = SCORE_DISABLE_TEAM_FIX_INVEN;   Message = MSG_SCORE_DISABLE_TEAM_FSTATION;     break;
            case object::TYPE_TURRET_FIXED         : Score = SCORE_DISABLE_TEAM_FIX_TURRET;  Message = MSG_SCORE_DISABLE_TEAM_FTURRET;      break; 
            case object::TYPE_TURRET_SENTRY        : Score = SCORE_DISABLE_TEAM_FIX_TURRET;  Message = MSG_SCORE_DISABLE_TEAM_FTURRET;      break;
            case object::TYPE_SENSOR_MEDIUM        : Score = SCORE_DISABLE_TEAM_FIX_SENSOR;  Message = MSG_SCORE_DISABLE_TEAM_FSENSOR;      break;
            case object::TYPE_SENSOR_LARGE         : Score = SCORE_DISABLE_TEAM_FIX_SENSOR;  Message = MSG_SCORE_DISABLE_TEAM_FSENSOR;      break;
            case object::TYPE_STATION_DEPLOYED     : Score = SCORE_DISABLE_TEAM_DEP_INVEN;   Message = MSG_SCORE_DISABLE_TEAM_RSTATION;     break;
            case object::TYPE_TURRET_CLAMP         : Score = SCORE_DISABLE_TEAM_DEP_TURRET;  Message = MSG_SCORE_DISABLE_TEAM_RTURRET;      break;
            case object::TYPE_SENSOR_REMOTE        : Score = SCORE_DISABLE_TEAM_DEP_SENSOR;  Message = MSG_SCORE_DISABLE_TEAM_RSENSOR;      break;
        }                                                                                    
    }                                                                                        

    GameMgr.ScoreForPlayer( OriginID, Score, Message );
}

//==============================================================================

void game_logic::ItemDestroyed( object::id ItemID, object::id OriginID )
{
    if( !tgl.ServerPresent )
        return;

    object* pPlayer = ObjMgr.GetObjectFromID( OriginID );
    object* pItem   = ObjMgr.GetObjectFromID(  ItemID  );
    s32     Score   =  0;
    s32     Message = -1;

    if( !pPlayer )
        return;

    ASSERT( pItem );

    if( !(pPlayer->GetAttrBits() & object::ATTR_PLAYER) )
        return;

    // Player and destroyed item on opposite teams?
    if( !(pPlayer->GetTeamBits() & pItem->GetTeamBits()) )
    {                                                                                        
        switch( pItem->GetType() )                                                           
        {                                                                                    
            case object::TYPE_GENERATOR            : Score = SCORE_DESTROY_ENEMY_GEN;        Message = MSG_SCORE_DESTROY_ENEMY_GEN;         break;
            case object::TYPE_STATION_VEHICLE      : Score = SCORE_DESTROY_ENEMY_VSTATION;   Message = MSG_SCORE_DESTROY_ENEMY_VSTATION;    break;
            case object::TYPE_STATION_FIXED        : Score = SCORE_DESTROY_ENEMY_FIX_INVEN;  Message = MSG_SCORE_DESTROY_ENEMY_FSTATION;    break;
            case object::TYPE_TURRET_FIXED         : Score = SCORE_DESTROY_ENEMY_FIX_TURRET; Message = MSG_SCORE_DESTROY_ENEMY_FTURRET;     break; 
            case object::TYPE_TURRET_SENTRY        : Score = SCORE_DESTROY_ENEMY_FIX_TURRET; Message = MSG_SCORE_DESTROY_ENEMY_FTURRET;     break;
            case object::TYPE_SENSOR_MEDIUM        : Score = SCORE_DESTROY_ENEMY_FIX_SENSOR; Message = MSG_SCORE_DESTROY_ENEMY_FSENSOR;     break;
            case object::TYPE_SENSOR_LARGE         : Score = SCORE_DESTROY_ENEMY_FIX_SENSOR; Message = MSG_SCORE_DESTROY_ENEMY_FSENSOR;     break;
            case object::TYPE_STATION_DEPLOYED     : Score = SCORE_DESTROY_ENEMY_DEP_INVEN;  Message = MSG_SCORE_DESTROY_ENEMY_RSTATION;    break;
            case object::TYPE_TURRET_CLAMP         : Score = SCORE_DESTROY_ENEMY_DEP_TURRET; Message = MSG_SCORE_DESTROY_ENEMY_RTURRET;     break;
            case object::TYPE_SENSOR_REMOTE        : Score = SCORE_DESTROY_ENEMY_DEP_SENSOR; Message = MSG_SCORE_DESTROY_ENEMY_RSENSOR;     break;
            case object::TYPE_BEACON               : Score = SCORE_DESTROY_ENEMY_DEP_BEACON;                                                break;
            case object::TYPE_VEHICLE_SHRIKE       : Score = SCORE_DESTROY_SHRIKE;           Message = MSG_SCORE_DESTROY_ENEMY_FIGHTER;     break;
            case object::TYPE_VEHICLE_THUNDERSWORD : Score = SCORE_DESTROY_THUNDERSWORD;     Message = MSG_SCORE_DESTROY_ENEMY_BOMBER;      break;
            case object::TYPE_VEHICLE_HAVOC        : Score = SCORE_DESTROY_HAVOC;            Message = MSG_SCORE_DESTROY_ENEMY_TRANSPORT;   break;
            case object::TYPE_VEHICLE_WILDCAT      : Score = SCORE_DESTROY_WILDCAT;          Message = MSG_SCORE_DESTROY_ENEMY_GRAVCYCLE;   break;
            case object::TYPE_VEHICLE_BEOWULF      : Score = SCORE_DESTROY_BEOWULF;                                                         break;
            case object::TYPE_VEHICLE_JERICHO2     : Score = SCORE_DESTROY_JERICHO;                                                         break;
        }
    }

    // Score and associated message.
    GameMgr.ScoreForPlayer( OriginID, Score, Message );

    if( (GameMgr.GetGameType() != GAME_CAMPAIGN) && (GameMgr.IsTeamBasedGame()) )
    {
        // Player and destroyed item on opposite teams?
        // How about "<thing> destroyed" for the player's team.
        if( (pPlayer->GetTeamBits() & pItem->GetTeamBits()) == 0 )
        {
            s32 Index  = -1;
            s32 Chance =  0;

            switch( pItem->GetType() )                                                           
            {                                                                                    
                case object::TYPE_GENERATOR       : Chance = 90;  Index = MSG_AUTO_GENERATOR_DESTROYED;        break;
                case object::TYPE_STATION_VEHICLE : Chance = 90;  Index = MSG_AUTO_VEHICLE_STATION_DESTROYED;  break;
                case object::TYPE_TURRET_FIXED    : Chance = 75;  Index = MSG_AUTO_TURRETS_DESTROYED;          break;
                case object::TYPE_SENSOR_MEDIUM   : Chance = 50;  Index = MSG_AUTO_SENSORS_DESTROYED;          break;
                case object::TYPE_SENSOR_LARGE    : Chance = 50;  Index = MSG_AUTO_SENSORS_DESTROYED;          break;
            }                                                                                       

            if( (Index != -1) && CHANCE(Chance) )
            {
                // If "Gen destroyed", and this kills a circuit, then maybe "Enemy base disabled".
                if( (Index == MSG_AUTO_GENERATOR_DESTROYED) && CHANCE(90) && 
                    (GameMgr.GetPower( ((generator*)pItem)->GetPowerCircuit() ) == 0) )
                {
                    Index = MSG_AUTO_ENEMY_BASE_DISABLED;
                }

                // Let 'er rip.
                MsgMgr.Message( Index, BitsToTeam( pPlayer->GetTeamBits() ),
                                ARG_WARRIOR, OriginID.Slot );
            }
        }

        // Player and destroyed item on opposite teams?
        // How about "Repair our <thing>" for the other team.
        if( (pPlayer->GetTeamBits() & pItem->GetTeamBits()) == 0 )
        {
            s32 Index  = -1;
            s32 Chance =  0;

            switch( pItem->GetType() )                                                           
            {                                                                                    
                case object::TYPE_GENERATOR       : Chance = 90;  Index = MSG_AUTO_REPAIR_GENERATOR;         break;
                case object::TYPE_STATION_VEHICLE : Chance = 90;  Index = MSG_AUTO_REPAIR_VEHICLE_STATION;   break;
                case object::TYPE_STATION_FIXED   : Chance = 75;  Index = MSG_AUTO_REPAIR_BASE;              break;
                case object::TYPE_TURRET_FIXED    : Chance = 75;  Index = MSG_AUTO_REPAIR_TURRETS;           break;
                case object::TYPE_SENSOR_MEDIUM   : Chance = 50;  Index = MSG_AUTO_REPAIR_SENSORS;           break;
                case object::TYPE_SENSOR_LARGE    : Chance = 50;  Index = MSG_AUTO_REPAIR_SENSORS;           break;
            }                                                                                       
         
            if( (Index != -1) && CHANCE(Chance) )
            {
                // Maybe switch "Repair gen" to "Enemy in base".
                if( (Index == MSG_AUTO_REPAIR_GENERATOR) && CHANCE(50) &&
                    (GameMgr.GetPower( ((generator*)pItem)->GetPowerCircuit() ) != 0) )
                {
                    Index = MSG_AUTO_ENEMY_IN_BASE;
                }

                // Let 'er rip.
                s32 Team = BitsToTeam( pItem->GetTeamBits() );
                s32 i = GameMgr.RandomTeamMember( Team );
                if( i != -1 )
                    MsgMgr.Message( Index, Team, ARG_WARRIOR, i );
            }
        }
    }
}

//==============================================================================

void game_logic::ItemEnabled( object::id ItemID, object::id PlayerID )
{
    (void)ItemID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::ItemRepaired( object::id ItemID, object::id PlayerID, xbool DoScore )
{
    if( !tgl.ServerPresent )
        return;

    if( !DoScore )
        return;

    object* pPlayer = ObjMgr.GetObjectFromID( PlayerID );
    object* pItem   = ObjMgr.GetObjectFromID(  ItemID  );
    s32     Score   =  0;
    s32     Message = -1;

    ASSERT( pPlayer );
    ASSERT( pItem   );

    if( !(pPlayer->GetAttrBits() & object::ATTR_PLAYER) )
        return;

    if( pPlayer->GetTeamBits() & pItem->GetTeamBits() )
    {
        switch( pItem->GetType() )
        {
            case object::TYPE_GENERATOR            : Score = SCORE_REPAIR_TEAM_GEN;         Message = MSG_SCORE_REPAIR_TEAM_GEN;      break;
            case object::TYPE_STATION_VEHICLE      : Score = SCORE_REPAIR_TEAM_VSTATION;    Message = MSG_SCORE_REPAIR_TEAM_VSTATION; break;
            case object::TYPE_STATION_FIXED        : Score = SCORE_REPAIR_TEAM_FIX_INVEN;   Message = MSG_SCORE_REPAIR_TEAM_FSTATION; break;
            case object::TYPE_TURRET_FIXED         : Score = SCORE_REPAIR_TEAM_FIX_TURRET;  Message = MSG_SCORE_REPAIR_TEAM_FTURRET;  break; 
            case object::TYPE_TURRET_SENTRY        : Score = SCORE_REPAIR_TEAM_FIX_TURRET;  Message = MSG_SCORE_REPAIR_TEAM_FTURRET;  break;
            case object::TYPE_SENSOR_MEDIUM        : Score = SCORE_REPAIR_TEAM_FIX_SENSOR;  Message = MSG_SCORE_REPAIR_TEAM_FSENSOR;  break;
            case object::TYPE_SENSOR_LARGE         : Score = SCORE_REPAIR_TEAM_FIX_SENSOR;  Message = MSG_SCORE_REPAIR_TEAM_FSENSOR;  break;
            case object::TYPE_STATION_DEPLOYED     : Score = SCORE_REPAIR_TEAM_DEP_INVEN;   Message = MSG_SCORE_REPAIR_TEAM_RSTATION; break;
            case object::TYPE_TURRET_CLAMP         : Score = SCORE_REPAIR_TEAM_DEP_TURRET;  Message = MSG_SCORE_REPAIR_TEAM_RTURRET;  break;
            case object::TYPE_SENSOR_REMOTE        : Score = SCORE_REPAIR_TEAM_DEP_SENSOR;  Message = MSG_SCORE_REPAIR_TEAM_RSENSOR;  break;
        }

        GameMgr.ScoreForPlayer( PlayerID, Score, Message );
    }    
}

//==============================================================================

void game_logic::ItemRepairing( object::id ItemID, object::id PlayerID )
{
    (void)ItemID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::ItemDeployed( object::id ItemID, object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    object* pObject = ObjMgr.GetObjectFromID( ItemID );
    ASSERT( pObject );
    
    if( pObject->GetType() == object::TYPE_BEACON )
    {
        object* pAsset;

        ObjMgr.Select( object::ATTR_ASSET, pObject->GetPosition(), BEACON_RADIUS );
        while( (pAsset = ObjMgr.GetNextSelected()) != NULL )
        {
            if( !(pAsset->GetTeamBits() & pObject->GetTeamBits()) )
            {
                break;
            }
        }
        ObjMgr.ClearSelection();
    
        if( pAsset )
        {
            GameMgr.ScoreForPlayer( PlayerID, SCORE_BEACON_ENEMY_ASSET );
        }
    }
}

//==============================================================================

void game_logic::ItemAutoLocked( object::id ItemID, object::id PlayerID )
{
    (void)ItemID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::PickupTouched( object::id PickupID, object::id PlayerID )
{
    (void)PickupID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::SwitchTouched( object::id SwitchID, object::id PlayerID )
{
    (void)SwitchID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::FlagTouched( object::id FlagID, object::id PlayerID )
{
    (void)FlagID;
    (void)PlayerID;
}

//==============================================================================

void game_logic::StationUsed( object::id PlayerID )
{
    (void)PlayerID;
}

//==============================================================================

xbool game_logic::FlagHitPlayer( object::id FlagID, object::id PlayerID )
{
    (void)FlagID;
    (void)PlayerID;
    return( FALSE );
}

//==============================================================================

void game_logic::FlagTimedOut( object::id FlagID ) 
{
    (void)FlagID;
}

//==============================================================================

void game_logic::NexusTouched( object::id PlayerID ) 
{
    (void)PlayerID;
}

//==============================================================================

void game_logic::ThrowFlags( object::id PlayerID ) 
{
    (void)PlayerID;
}

//==============================================================================

void game_logic::GetFlagStatus( u32 TeamBits, s32& Status, vector3& Position )
{
    (void)TeamBits;
    (void)Position;
    Status = -3;
}

//==============================================================================

object::id game_logic::GetFlagID( s32 Index )
{
    object::id Result;
    (void)Index;
    return( Result );
}

//==============================================================================

void game_logic::Render( void )
{
}

//==============================================================================

void game_logic::Initialize( void )
{
}

//==============================================================================

void game_logic::WeaponFired( object::id PlayerID )
{
    (void)PlayerID;
}

//==============================================================================

void game_logic::WeaponExchanged( object::id PlayerID )
{
    (void)PlayerID;
}

//==============================================================================

void game_logic::SatchelDetonated( object::id PlayerID )
{
    (void)PlayerID;
}

//==============================================================================
