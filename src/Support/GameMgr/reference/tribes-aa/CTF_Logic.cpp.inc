//==============================================================================
//
//  CTF_Logic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "CTF_Logic.hpp"
#include "GameMgr.hpp"
#include "MsgMgr.hpp"
#include "LogMgr.hpp"
#include "netlib/bitstream.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Projectiles\Flag.hpp"
#include "Objects\Projectiles\WayPoint.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include "HUD\hud_Icons.hpp"

#include "..\AADS\Data\UI\Messages.h"

//==============================================================================
//  DEFINES
//==============================================================================

#define DEFEND_FLAG_RADIUS  50

#define CHANCE(n)   (x_irand(1,100) <= (n))

//==============================================================================
//  STORAGE
//==============================================================================

ctf_logic   CTF_Logic;

//==============================================================================
//  FUNCTIONS
//==============================================================================

ctf_logic::ctf_logic( void )
{
    m_FlagStatus[0] = FLAG_STATUS_UNKNOWN;
    m_FlagStatus[1] = FLAG_STATUS_UNKNOWN;
}

//==============================================================================

ctf_logic::~ctf_logic( void )
{
}

//==============================================================================

void ctf_logic::DropFlag(       s32        FlagIndex, 
                                object::id OriginID,
                          const vector3&   Position, 
                          const vector3&   Velocity, 
                                radian     Yaw )
{
    x_DebugMsg( "[DropFlag(%d)]\n", FlagIndex );

    // Create a new flag at position with velocity.
    flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
    ASSERT( pFlag );

    pFlag->Initialize( Position, Yaw, Velocity, OriginID, (1 << FlagIndex) );
    ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
    m_FlagStatus[ FlagIndex ] = FLAG_STATUS_DROPPED;
    m_Flag      [ FlagIndex ] = pFlag->GetObjectID();

    // Sound.
    GameMgr.SendAudio( SFX_MISC_FLAG_DROP_GOOD, (1 << FlagIndex), SFX_MISC_FLAG_DROP_BAD );

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;

    // Extra sanity check code.
    {
        flag* pFlag = (flag*)ObjMgr.GetObjectFromID( m_Flag[FlagIndex] );
        ASSERT( pFlag );
    }
}

//==============================================================================

void ctf_logic::Connect( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    s32 Team = -1;

    // Need to choose a team for this player.
    if( GameMgr.m_Score.Team[0].Size <  GameMgr.m_Score.Team[1].Size )    Team = 0;
    if( GameMgr.m_Score.Team[0].Size >  GameMgr.m_Score.Team[1].Size )    Team = 1;
    if( GameMgr.m_Score.Team[0].Size == GameMgr.m_Score.Team[1].Size )
    {
        // See which team has fewer humans.
        if( GameMgr.m_Score.Team[0].Humans < GameMgr.m_Score.Team[1].Humans )   Team = 0;
        if( GameMgr.m_Score.Team[0].Humans > GameMgr.m_Score.Team[1].Humans )   Team = 1;

        if( Team == -1 )
        {
            // See which team has the lower score.
            if( GameMgr.m_Score.Team[0].Score < GameMgr.m_Score.Team[1].Score )  Team = 0;
            if( GameMgr.m_Score.Team[0].Score > GameMgr.m_Score.Team[1].Score )  Team = 1;

            // All else fails...
            if( Team == -1 )
                Team = PlayerIndex & 0x01;
        }
    }

    GameMgr.m_Score.Player[PlayerIndex].Team = Team;
    GameMgr.m_Score.Team[Team].Size++;

    if( GameMgr.m_Score.Player[PlayerIndex].IsBot )
        GameMgr.m_Score.Team[Team].Bots++;
    else
        GameMgr.m_Score.Team[Team].Humans++;

    // Set dirty bits.
    GameMgr.m_DirtyBits  |= game_mgr::DIRTY_SPECIFIC_DATA;
    GameMgr.m_PlayerBits |= (1 << PlayerIndex);
}

//==============================================================================

void ctf_logic::ExitGame( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    ASSERT( IN_RANGE( 0, PlayerIndex, 31 ) );

    // If the player is carrying the flag, then drop it.

    s32 Flag = -1;

    if( m_FlagStatus[0] == (flag_status)PlayerIndex )    Flag = 0;
    if( m_FlagStatus[1] == (flag_status)PlayerIndex )    Flag = 1;

    if( Flag != -1 )
    {
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromSlot( PlayerIndex );
        ASSERT( pPlayer );
        pPlayer->DetachFlag();
        pPlayer->SetFlagCount( 0 );
        DropFlag( Flag, 
                  ObjMgr.NullID,
                  pPlayer->GetPosition(), 
                  vector3(0,5,0), 
                  pPlayer->GetRot().Yaw );
        x_DebugMsg( ">>> Drop by exit game\n" );

        // Extra sanity check code.
        {
            flag* pFlag = (flag*)ObjMgr.GetObjectFromID( m_Flag[Flag] );
            ASSERT( pFlag );
        }
    }

    // Set dirty bits.
    GameMgr.m_DirtyBits  |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void ctf_logic::Disconnect( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    ASSERT( IN_RANGE( 0, PlayerIndex, 31 ) );

    s32 Team = GameMgr.m_Score.Player[PlayerIndex].Team;

    GameMgr.m_Score.Team[Team].Size--;

    if( GameMgr.m_Score.Player[PlayerIndex].IsBot )
        GameMgr.m_Score.Team[Team].Bots--;
    else
        GameMgr.m_Score.Team[Team].Humans--;


    // Set dirty bits.
    GameMgr.m_DirtyBits  |= game_mgr::DIRTY_SPECIFIC_DATA;
    GameMgr.m_PlayerBits |= (1 << PlayerIndex);
}

//==============================================================================

void ctf_logic::ChangeTeam( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    // If the player is carrying the flag, then drop it.

    s32 Flag = -1;

    if( m_FlagStatus[0] == (flag_status)PlayerIndex )    Flag = 0;
    if( m_FlagStatus[1] == (flag_status)PlayerIndex )    Flag = 1;

    if( Flag != -1 )
    {
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromSlot( PlayerIndex );
        ASSERT( pPlayer );
        pPlayer->DetachFlag();
        pPlayer->SetFlagCount( 0 );
        DropFlag( Flag, 
                  ObjMgr.NullID,
                  pPlayer->GetPosition(), 
                  vector3(0,5,0), 
                  pPlayer->GetRot().Yaw );
        x_DebugMsg( ">>> Drop by change team\n" );

        if( GameMgr.GameInProgress() )
            MsgMgr.Message( MSG_CTF_YOU_DROPPED_FLAG, PlayerIndex );
    }     
}

//==============================================================================

void ctf_logic::RegisterFlag( const vector3& Position, radian Yaw, s32 FlagTeam )
{
    if( !tgl.ServerPresent )
        return;

    if( FlagTeam == -1 )
        FlagTeam = 0;

    ASSERT( IN_RANGE( 0, FlagTeam, 1 ) );

    m_FlagStand[ FlagTeam ] = Position;
    m_FlagYaw  [ FlagTeam ] = Yaw;

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void ctf_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    const game_score& Score = GameMgr.GetScore();

    s32 PlayerIndex = IndexFromID( Pain.VictimID );
    s32 KillerIndex = IndexFromID( Pain.OriginID );

    player_object* pVictim = (player_object*)ObjMgr.GetObjectFromSlot( PlayerIndex );
    player_object* pKiller = (player_object*)ObjMgr.GetObjectFromSlot( KillerIndex );

    game_logic::PlayerDied( Pain );

    // Defend allied carrier score?
    if( pKiller && 
        pVictim && 
        (Score.Player[KillerIndex].Team != Score.Player[PlayerIndex].Team) )
    {
        s32 Team = Score.Player[KillerIndex].Team;
        if( m_FlagStatus[1-Team] >= 0 )
        {
            ASSERT( Score.Player[KillerIndex].Team == Score.Player[ m_FlagStatus[1-Team] ].Team );
            player_object* pCarrier = (player_object*)ObjMgr.GetObjectFromSlot( m_FlagStatus[1-Team] );
            if( pCarrier && (pCarrier != pKiller) )
            {
                vector3 Difference = pCarrier->GetPosition() - pVictim->GetPosition();
                if( Difference.LengthSquared() < SQR(DEFEND_FLAG_RADIUS) )
                {
                    GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_DEFEND_ALLY_CARRIER, MSG_SCORE_CTF_DEFEND_CARRIER );
                }
            }
        }
    }

    // Defend flag score?
    if( IsDefending( object::TYPE_FLAG, Pain.VictimID, Pain.OriginID ) )
    {
        GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_DEFEND_FLAG, MSG_SCORE_CTF_DEFEND_FLAG );
    }

    // If the player is carrying the flag, then drop it.

    s32 Flag = -1;
    if( m_FlagStatus[0] == (flag_status)PlayerIndex )    Flag = 0;
    if( m_FlagStatus[1] == (flag_status)PlayerIndex )    Flag = 1;

    if( Flag != -1 )
    {
        // Kill enemy carrier score?
        if( (KillerIndex != -1) && 
            (KillerIndex != PlayerIndex) &&
            (GameMgr.m_Score.Player[KillerIndex].Team != GameMgr.m_Score.Player[PlayerIndex].Team) )
            GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_KILL_ENEMY_CARRIER, MSG_SCORE_CTF_KILL_CARRIER );

        ASSERT( pVictim );

        // Drop it!
        pVictim->DetachFlag();
        pVictim->SetFlagCount( 0 );
        DropFlag( Flag,        
                  Pain.VictimID,
                  pVictim->GetPosition(), 
                  pVictim->GetVel() + vector3(0,5,0), 
                  pVictim->GetRot().Yaw );
        MsgMgr.Message( MSG_CTF_YOU_DROPPED_FLAG, PlayerIndex );
        x_DebugMsg( ">>> Drop by death\n" );
    }

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void ctf_logic::FlagTouched( object::id FlagID, object::id PlayerID )
{
    x_DebugMsg( "[FlagTouched(FlagID=%d,PlayerID=%d)]\n", FlagID.Slot, PlayerID.Slot );

    if( !tgl.ServerPresent )
        return;

    waypoint*       pWayPoint   = NULL;
    s32             PlayerIndex = IndexFromID( PlayerID );
    object*         pFlag       = ObjMgr.GetObjectFromID( FlagID );
    player_object*  pPlayer     = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    u32             FlagBits    = pFlag->GetTeamBits();
    u32             PlayerBits  = pPlayer->GetTeamBits();
    s32             FlagTeam    = BitsToTeam( FlagBits );
    s32             PlayerTeam  = BitsToTeam( PlayerBits );

    ASSERT( pFlag   );
    ASSERT( pPlayer );

    ASSERT( IN_RANGE( 0, FlagTeam,   1 ) );
    ASSERT( IN_RANGE( 0, PlayerTeam, 1 ) );

    if( pPlayer->IsVehiclePilot() )
        return;

    // Cap?
    if( (FlagTeam == PlayerTeam) &&
        (m_FlagStatus[   FlagTeam ] == FLAG_STATUS_ON_STAND) &&
        (m_FlagStatus[ 1-FlagTeam ] == (flag_status)PlayerIndex) )
    {
        x_DebugMsg( "[CAP]\n" );

        // Take the flag off the players back.
        pPlayer->DetachFlag();
        pPlayer->SetFlagCount( 0 );

        // Return the enemy flag.
        flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
        pFlag->Initialize( m_FlagStand[ 1-FlagTeam ],
                           m_FlagYaw  [ 1-FlagTeam ],
                           1 << (1-FlagTeam) );
        ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
        m_FlagStatus[ 1-FlagTeam ] = FLAG_STATUS_ON_STAND;
        m_Flag      [ 1-FlagTeam ] = pFlag->GetObjectID();

        // Update the way point.
        pWayPoint = (waypoint*)ObjMgr.GetObjectFromID( m_StandWayPt[ 1-FlagTeam ] );
        pWayPoint->SetHidden( TRUE );
        
        // Give points for the capture.
        
        GameMgr.ScoreForPlayer( PlayerID, SCORE_CAPTURE_FLAG_PLAYER, MSG_SCORE_CTF_CAP_FLAG );
        GameMgr.m_Score.Team  [ PlayerTeam  ].Score += SCORE_CAPTURE_FLAG_TEAM;

        /*
        // Announce the capture.
        Message( xfs( "%s captured the %s flag.", 
                      m_Player[PlayerIndex].Name,
                      s_CTF.TeamName[1-FlagTeam] ),
                 s_ChatGreen, s_ChatRed, PlayerBits );

        // Show the current scores.
        Message( xfs( "Current team scores:  %s:%d  %s:%d", 
                       s_CTF.TeamName[0], s_CTF.TeamScore[0],
                       s_CTF.TeamName[1], s_CTF.TeamScore[1] ),
                 s_ChatUrgent );
        */

        // Messages.
        MsgMgr.Message( MSG_CTF_TEAM_SCORES, 0, ARG_TEAM, PlayerTeam );
        MsgMgr.Message( MSG_CTF_TEAM_CAP,    PlayerTeam );
        MsgMgr.Message( MSG_CTF_ENEMY_CAP, 1-PlayerTeam );

        // Sound.
        GameMgr.SendAudio( SFX_MISC_FLAG_CAPTURE_GOOD, PlayerBits, SFX_MISC_FLAG_CAPTURE_BAD );
        if( PlayerTeam == 0 )   GameMgr.SendAudio( SFX_VOICE_ANN_TEAM_STORM_SCORES );
        else                    GameMgr.SendAudio( SFX_VOICE_ANN_TEAM_INFERNO_SCORES );

        // Auto chatter: "Positive" or "You rock".
        s32 i = GameMgr.RandomTeamMember( FlagTeam );
        if( i != -1 )
        {
            if( (i != PlayerIndex) && CHANCE(70) )
                MsgMgr.Message( MSG_AUTO_YOU_ROCK, 0, ARG_WARRIOR, i );
            else
                MsgMgr.Message( MSG_POSITIVE, 0, ARG_WARRIOR, i );
        }

        // Logging.
        LogMgr.LogSmallScore();

        // Set dirty bits.
        GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
    }

    // Return?
    if( (FlagTeam == PlayerTeam) &&
        (m_FlagStatus[ FlagTeam ] == FLAG_STATUS_DROPPED) )
    {
        x_DebugMsg( "[RETURN]\n" );

        // Create a new flag on the flag stand.
        // (Don't move the flag which was picked up, it is going to be deleted.)
        flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
        pFlag->Initialize( m_FlagStand[ FlagTeam ], 
                           m_FlagYaw  [ FlagTeam ], 
                           FlagBits );
        ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
        m_FlagStatus[ FlagTeam ] = FLAG_STATUS_ON_STAND;
        m_Flag      [ FlagTeam ] = pFlag->GetObjectID();

        // Get rid of the old flag.
        ObjMgr.DestroyObject( FlagID );

        // Update the way point.
        pWayPoint = (waypoint*)ObjMgr.GetObjectFromID( m_StandWayPt[ FlagTeam ] );
        pWayPoint->SetHidden( TRUE );
        
        // Give points for the return.
        GameMgr.ScoreForPlayer( PlayerID, SCORE_RETURN_FLAG, MSG_SCORE_CTF_RETURN_FLAG );

        /*
        // Announce the return.
        Message( xfs( "%s returned the %s flag.", 
                      m_Player[PlayerIndex].Name,
                      s_CTF.TeamName[FlagTeam] ),
                 s_ChatGreen, s_ChatRed, PlayerBits );
        */

        FlagReturned( FlagTeam );

        // Sound.
        GameMgr.SendAudio( SFX_MISC_FLAG_RETURN_GOOD, PlayerBits, SFX_MISC_FLAG_RETURN_BAD );

        // Set dirty bits.
        GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
    }

    // Take?
    if( FlagTeam != PlayerTeam )
    {          
        // Only give points if the flag was on the stand.
        if( m_FlagStatus[ FlagTeam ] == FLAG_STATUS_ON_STAND )
        {
            x_DebugMsg( "[TAKE FROM STAND]\n" );

            GameMgr.ScoreForPlayer( PlayerID, SCORE_TAKE_FLAG_PLAYER, MSG_SCORE_CTF_TAKE_FLAG );
            GameMgr.m_Score.Team  [ PlayerTeam  ].Score += SCORE_TAKE_FLAG_TEAM;

            // Auto chatter: I have the enemy flag.
            MsgMgr.Message( MSG_AUTO_HAVE_FLAG, PlayerTeam, ARG_WARRIOR, PlayerIndex );

            // Auto chatter: Retrieve our flag.
            s32 i = GameMgr.RandomTeamMember( FlagTeam );
            if( i != -1 )
            {
                if( CHANCE(50) )  MsgMgr.Message( MSG_AUTO_RETRIEVE_FLAG, FlagTeam, ARG_WARRIOR, i );
                else              MsgMgr.Message( MSG_AUTO_RECOVER_FLAG,  FlagTeam, ARG_WARRIOR, i );
            }

            // Message:  <Warrior> took your team's flag.
            MsgMgr.Message( MSG_CTF_PLAYER_TOOK_FLAG, FlagTeam, ARG_WARRIOR, PlayerIndex );

            // Logging.
            LogMgr.LogSmallScore();
        }
        else
        {
            x_DebugMsg( "[TAKE FROM GROUND]\n" );
        }

        // Assign the flag to the player.
        m_FlagStatus[ FlagTeam ] = (flag_status)PlayerIndex;
        pPlayer->AttachFlag( FlagTeam + 1 );
        pPlayer->SetFlagCount( 1 );
        m_Flag[ FlagTeam ] = PlayerID;

        // Update the way point.
        pWayPoint = (waypoint*)ObjMgr.GetObjectFromID( m_StandWayPt[ FlagTeam ] );
        pWayPoint->SetHidden( FALSE );

        // Get rid of the flag.
        ObjMgr.DestroyObject( FlagID );

        /*
        // Announce the pick up.
        Message( xfs( "%s has taken the %s flag.", 
                      m_Player[PlayerIndex].Name,
                      s_CTF.TeamName[FlagTeam] ), 
                 s_ChatGreen, s_ChatRed, PlayerBits );

        PopMessage( "You have the enemy flag!", PlayerIndex );
        */
        MsgMgr.Message( MSG_CTF_YOU_HAVE_FLAG, PlayerIndex );

        // Sound.
        GameMgr.SendAudio( SFX_MISC_FLAG_TAKEN_GOOD, PlayerBits, SFX_MISC_FLAG_TAKEN_BAD );

        // Set dirty bits.
        GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
    }                                                     
}

//==============================================================================

xbool ctf_logic::FlagHitPlayer( object::id FlagID, object::id PlayerID )
{
    x_DebugMsg( "[FlagHitPlayer(FlagID=%d,PlayerID=%d)]\n", FlagID.Slot, PlayerID.Slot );

    if( !tgl.ServerPresent )
        return( FALSE );

    waypoint*       pWayPoint   = NULL;
    s32             PlayerIndex = IndexFromID( PlayerID );
    object*         pFlag       = ObjMgr.GetObjectFromID( FlagID );
    player_object*  pPlayer     = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    u32             FlagBits    = pFlag->GetTeamBits();
    u32             PlayerBits  = pPlayer->GetTeamBits();
    s32             FlagTeam    = BitsToTeam( FlagBits );
    s32             PlayerTeam  = BitsToTeam( PlayerBits );

    ASSERT( pFlag   );
    ASSERT( pPlayer );

    ASSERT( IN_RANGE( 0, FlagTeam,   1 ) );
    ASSERT( IN_RANGE( 0, PlayerTeam, 1 ) );

    if( pPlayer->IsVehiclePilot() )
        return( FALSE );

    // Return?
    if( (FlagTeam == PlayerTeam) &&
        (m_FlagStatus[ FlagTeam ] == FLAG_STATUS_DROPPED) )
    {
        x_DebugMsg( "[RETURN]\n" );

        // Create a new flag on the flag stand.
        // (Don't move the flag which hit player, it is going to be deleted.)
        flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
        pFlag->Initialize( m_FlagStand[ FlagTeam ], 
                           m_FlagYaw  [ FlagTeam ], 
                           FlagBits );
        ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
        m_FlagStatus[ FlagTeam ] = FLAG_STATUS_ON_STAND;
        m_Flag      [ FlagTeam ] = pFlag->GetObjectID();

        // Update the way point.
        pWayPoint = (waypoint*)ObjMgr.GetObjectFromID( m_StandWayPt[ FlagTeam ] );
        pWayPoint->SetHidden( TRUE );
        
        // Give points for the return.
        GameMgr.ScoreForPlayer( PlayerID, SCORE_RETURN_FLAG, MSG_SCORE_CTF_RETURN_FLAG );

        /*
        // Announce the return.
        Message( xfs( "%s returned the %s flag.", 
                      m_Player[PlayerIndex].Name,
                      s_CTF.TeamName[FlagTeam] ),
                 s_ChatGreen, s_ChatRed, PlayerBits );
        */

        FlagReturned( FlagTeam );

        // Sound.
        GameMgr.SendAudio( SFX_MISC_FLAG_RETURN_GOOD, PlayerBits, SFX_MISC_FLAG_RETURN_BAD );

        // Set dirty bits.
        GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;

        return( TRUE );
    }

    // Take?
    if( FlagTeam != PlayerTeam )
    {
        x_DebugMsg( "[TAKE FROM GROUND]\n" );

        // Assign the flag to the player.
        m_FlagStatus[ FlagTeam ] = (flag_status)PlayerIndex;
        pPlayer->AttachFlag( FlagTeam + 1 );
        pPlayer->SetFlagCount( 1 );
        m_Flag[ FlagTeam ] = PlayerID;

        /*
        // Announce the pick up.
        Message( xfs( "%s picked up the %s flag.", 
                      m_Player[PlayerIndex].Name,
                      s_CTF.TeamName[FlagTeam] ), 
                 s_ChatGreen, s_ChatRed, PlayerBits );
        */
        MsgMgr.Message( MSG_CTF_YOU_HAVE_FLAG, PlayerIndex );

        // Sound.
        GameMgr.SendAudio( SFX_MISC_FLAG_TAKEN_GOOD, PlayerBits, SFX_MISC_FLAG_TAKEN_BAD );

        // Set dirty bits.
        GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;

        return( TRUE );
    }

    return( FALSE );
}

//==============================================================================

void ctf_logic::FlagTimedOut( object::id FlagID ) 
{
    x_DebugMsg( "[FlagTimedOut(FlagID=%d)]\n", FlagID.Slot );

    if( !tgl.ServerPresent )
        return;

    waypoint*   pWayPoint = NULL;
    object*     pObject   = ObjMgr.GetObjectFromID( FlagID );
    u32         FlagBits  = pObject->GetTeamBits();
    s32         FlagTeam  = BitsToTeam( FlagBits );

    ASSERT( IN_RANGE( 0, FlagTeam, 1 ) );

    // Create a new flag on the flag stand.
    // (Don't move the flag which timed out, it is going to be deleted.)
    flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
    pFlag->Initialize( m_FlagStand[ FlagTeam ], 
                       m_FlagYaw  [ FlagTeam ], 
                       FlagBits );
    ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
    m_FlagStatus[ FlagTeam ] = FLAG_STATUS_ON_STAND;
    m_Flag      [ FlagTeam ] = pFlag->GetObjectID();

    // Update the way point.
    pWayPoint = (waypoint*)ObjMgr.GetObjectFromID( m_StandWayPt[ FlagTeam ] );
    pWayPoint->SetHidden( TRUE );

    /*
    // Announce the return.
    Message( xfs( "The %s flag has returned to base.",
                  s_CTF.TeamName[FlagTeam] ),
             s_ChatGreen, s_ChatRed, FlagBits );
    */

    // Sound.
    GameMgr.SendAudio( SFX_MISC_FLAG_RETURN_GOOD, FlagBits, SFX_MISC_FLAG_RETURN_BAD );

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void ctf_logic::ThrowFlags( object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    for( s32 i = 0; i < 2; i++ )
    {
        if( m_FlagStatus[i] == (flag_status)PlayerID.Slot )
        {
            player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
            ASSERT( pPlayer );

            vector3 Direction(0,5,20);
            Direction.Rotate( pPlayer->GetRot() );

            pPlayer->DetachFlag();
            pPlayer->SetFlagCount( 0 );
            DropFlag( i,        
                      PlayerID,
                      pPlayer->GetPosition() + vector3(0,2,0), 
                      pPlayer->GetVel() + Direction,
                      pPlayer->GetRot().Yaw );
            x_DebugMsg( ">>> Drop by drop flag\n" );

            // Set dirty bits.
            GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
        }
    }
}

//==============================================================================

void ctf_logic::SwitchTouched( object::id SwitchID, object::id PlayerID )
{
    (void)SwitchID;
    
    GameMgr.ScoreForPlayer( PlayerID, SCORE_ACQUIRE_SWITCH, MSG_SCORE_TAKE_OBJECTIVE );
}

//==============================================================================

void ctf_logic::ItemDeployed( object::id ItemID, object::id PlayerID )
{
    (void)PlayerID;

    object* pObject = ObjMgr.GetObjectFromID( ItemID );
    if( pObject && (pObject->GetType() == object::TYPE_MINE) )
    {
        // See if it is near its flag.
        for( s32 i = 0; i < 2; i++ )
        {
            vector3 Span = m_FlagStand[i] - pObject->GetPosition();
            if( (Span.LengthSquared() < 100.0f) && (pObject->GetTeamBits() & (1<<i)) )
            {
                MsgMgr.Message( MSG_CTF_FLAG_MINED, i );
            }
        }
    }
}

//==============================================================================

void ctf_logic::EnforceBounds( f32 DeltaTime )
{   
    (void)DeltaTime;

    s32 i;

    // Attempt to keep the flags in bounds.

    for( i = 0; i < 2; i++ )
    {
        vector3 Position;
        vector3 Velocity;

        // Flag on a player?
        if( m_FlagStatus[i] > FLAG_STATUS_DROPPED )
        {
            player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_Flag[i] );
            ASSERT( pPlayer );
            Position = pPlayer->GetPosition();
            
            if( (Position.X < GameMgr.m_Bounds.Min.X) ||
                (Position.X > GameMgr.m_Bounds.Max.X) ||
                (Position.Z < GameMgr.m_Bounds.Min.Z) ||
                (Position.Z > GameMgr.m_Bounds.Max.Z) )
            {
                MsgMgr.Message( MSG_CTF_YOU_DROPPED_FLAG, m_Flag[i].Slot );
                pPlayer->DetachFlag();
                pPlayer->SetFlagCount( 0 );
                DropFlag( i, 
                          m_Flag[i],
                          Position, 
                          vector3(0,10,0), 
                          pPlayer->GetRot().Yaw );
                x_DebugMsg( ">>> Drop by bounds\n" );
            }
        }   
        
        // Flag dropped?
        if( m_FlagStatus[i] == FLAG_STATUS_DROPPED )
        {
            xbool Dirty = FALSE;
            flag* pFlag = (flag*)ObjMgr.GetObjectFromID( m_Flag[i] );
            ASSERT( pFlag );

            Position = pFlag->GetPosition();
            Velocity = pFlag->GetVelocity();

            if( Position.X < GameMgr.m_Bounds.Min.X )
            {
                Velocity.X = 15.0f;
                Dirty      = TRUE;
            }

            if( Position.X > GameMgr.m_Bounds.Max.X )
            {
                Velocity.X = -15.0f;
                Dirty      = TRUE;
            }

            if( Position.Z < GameMgr.m_Bounds.Min.Z )
            {
                Velocity.Z = 15.0f;
                Dirty      = TRUE;
            }

            if( Position.Z > GameMgr.m_Bounds.Max.Z )
            {
                Velocity.Z = -15.0f;
                Dirty      = TRUE;
            }

            if( Dirty )
            {
                x_DebugMsg( "[EnforceBounds](Index=%d,ID=%d)\n", i, m_Flag[i].Slot );
                Velocity.Y = 15.0f;
                pFlag->SetVelocity( Velocity );
            }
        }
    }

    // Prevent vehicles from blocking access to the flags.
    for( i = 0; i < 2; i++ )
    {
        object* pObject;

        ObjMgr.Select( object::ATTR_VEHICLE, 
                       m_FlagStand[i], 
                       g_PainBase[pain::MISC_FLAG_TO_VEHICLE].DamageR0 );
        while( (pObject = ObjMgr.GetNextSelected()) )
        {
            ObjMgr.InflictPain( pain::MISC_FLAG_TO_VEHICLE,
                                pObject, 
                                m_FlagStand[i], 
                                ObjMgr.NullID, 
                                ObjMgr.NullID,
                                FALSE,
                                DeltaTime );            
        }
        ObjMgr.ClearSelection();
    }
}

//==============================================================================

void ctf_logic::BeginGame( void )
{
    if( !tgl.ServerPresent )
        return;

    // Create flags (on the stands) and waypoints.

    xwstring  Base( " Base" );
    flag*     pFlag;
    waypoint* pWayPoint;
    xwchar    Label[32];

    // Flag.
    pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
    pFlag->Initialize( m_FlagStand[0], m_FlagYaw[0], 0x01 );
    ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
    m_Flag[0] = pFlag->GetObjectID();

    // Waypoint on the flag stand.
    pWayPoint = (waypoint*)ObjMgr.CreateObject( object::TYPE_WAYPOINT );
    pWayPoint->Initialize( m_FlagStand[0], 1, NULL );
    pWayPoint->SetHidden( TRUE );
    ObjMgr.AddObject( pWayPoint, obj_mgr::AVAILABLE_SERVER_SLOT );
    x_wstrcpy( Label, GameMgr.m_Score.Team[0].Name );
    x_wstrcat( Label, (const xwchar*)Base );
    pWayPoint->SetLabel( Label );
    m_StandWayPt[0] = pWayPoint->GetObjectID();

    // Flag.
    pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
    pFlag->Initialize( m_FlagStand[1], m_FlagYaw[1], 0x02 );
    ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );
    m_Flag[1] = pFlag->GetObjectID();

    // Waypoint on the flag stand.
    pWayPoint = (waypoint*)ObjMgr.CreateObject( object::TYPE_WAYPOINT );
    pWayPoint->Initialize( m_FlagStand[1], 2, NULL );
    pWayPoint->SetHidden( TRUE );
    ObjMgr.AddObject( pWayPoint, obj_mgr::AVAILABLE_SERVER_SLOT );
    x_wstrcpy( Label, GameMgr.m_Score.Team[1].Name );
    x_wstrcat( Label, (const xwchar*)Base );
    pWayPoint->SetLabel( Label );
    m_StandWayPt[1] = pWayPoint->GetObjectID();

    // Prepare the CTF data. 
    m_FlagStatus[0] = FLAG_STATUS_ON_STAND;
    m_FlagStatus[1] = FLAG_STATUS_ON_STAND;

    // Score limit.
    GameMgr.SetScoreLimit( 800 );
}

//==============================================================================

void ctf_logic::EndGame( void )
{
    if( !tgl.ServerPresent )
        return;

    // Detach any flags from players.

    if( m_FlagStatus[0] >= FLAG_STATUS_PLAYER_00 )
    {
        object::id ID = m_Flag[ 0 ];
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( ID );
        if( pPlayer )
        {
            pPlayer->DetachFlag();
            pPlayer->SetFlagCount( 0 );
        }
    }

    if( m_FlagStatus[1] >= FLAG_STATUS_PLAYER_00 )
    {
        object::id ID = m_Flag[ 1 ];
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( ID );
        if( pPlayer )
        {
            pPlayer->DetachFlag();
            pPlayer->SetFlagCount( 0 );
        }
    }


    // FLAG BUG
    // FLAG BUG
    m_Flag[0].Slot = -1;    m_Flag[0].Seq = -1;
    m_Flag[1].Slot = -1;    m_Flag[1].Seq = -1;
    // FLAG BUG
    // FLAG BUG


    if( ABS( GameMgr.m_Score.Team[0].Score - GameMgr.m_Score.Team[1].Score ) > 480 )
        GameMgr.ShuffleTeams();
}

//==============================================================================

void ctf_logic::GetFlagStatus( u32 TeamBits, s32& Status, vector3& Position )
{
    s32     FlagID;
    object* pObj;

    FlagID = (s32)TeamBits - 1;
    pObj   = ObjMgr.GetObjectFromID( m_Flag[FlagID] );
    Status = (s32)m_FlagStatus[ FlagID ];

    if( (pObj == NULL) || (Status == FLAG_STATUS_UNKNOWN) )
        return;

    Position = pObj->GetPosition();
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

object::id ctf_logic::GetFlagID( s32 Index )
{
    return( m_Flag[Index] );
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

void ctf_logic::FlagReturned( s32 FlagIndex )
{
    // Chatter.  "Flag is secure" or "Defend our flag".

    xbool   Secure = TRUE;
    object* pObject;

    ObjMgr.Select( object::ATTR_PLAYER, m_FlagStand[ FlagIndex ], 75.0f );

    while( Secure && (pObject = ObjMgr.GetNextSelected()) )
    {
        if( (pObject->GetTeamBits() & (1 << FlagIndex)) == 0 )
            Secure = FALSE;
    }
    ObjMgr.ClearSelection();

    {
        s32 i = GameMgr.RandomTeamMember( FlagIndex );
        if( i != -1 )
        {
            if( Secure )  MsgMgr.Message( MSG_AUTO_FLAG_SECURE,     FlagIndex, ARG_WARRIOR, i );
            else          MsgMgr.Message( MSG_AUTO_DEFEND_OUR_FLAG, FlagIndex, ARG_WARRIOR, i );
        }
    }
}

//==============================================================================

void ctf_logic::Render( void )
{
    s32         i;
    object::id  ID( tgl.PC[tgl.WC].PlayerIndex, -1 );
    object*     pObject;

    // Get the player.
    pObject = ObjMgr.GetObjectFromID( ID );

    // Did we find the "player"?
    if( !pObject )
        return;

    // Render each flag icon.
    for( i = 0; i < 2; i++ )
    {
        // If the local player has the flag, then continue.
        if( ID == m_Flag[i] )
            continue;

        // Get the object which represents the location of the flag.
        object* pFlagObj = ObjMgr.GetObjectFromID( m_Flag[i] );

        // Did we get it?
        if( !pFlagObj )
            continue;

        // Is it a flag?
        if( pFlagObj->GetType() == object::TYPE_FLAG )
        {
            vector3 Position = pFlagObj->GetPosition();
            Position.Y += 1.0f;

            HudIcons.Add( hud_icons::FLAG, Position, (1 << i) );
            if( m_FlagStatus[i] == FLAG_STATUS_DROPPED )
                HudIcons.Add( hud_icons::FLAG_HALO, Position, hud_icons::YELLOW );
        }
        
        // Is it a player?
        if( pFlagObj->GetAttrBits() & object::ATTR_PLAYER )
        {
            vector3 Position = ((player_object*)pFlagObj)->GetDrawPos();
            Position.Y += 1.7f;

            HudIcons.Add( hud_icons::FLAG,      Position,  (1 << i) );
            HudIcons.Add( hud_icons::FLAG_HALO, Position, ~(1 << i) );
        }
    }
}

//==============================================================================

void ctf_logic::AcceptUpdate( const bitstream& BitStream )
{
    s32 i;
    s32 Score;

    ASSERT( tgl.ClientPresent );

    //
    // Try to read the SPECIFIC_INIT data.
    //
    if( BitStream.ReadFlag() )
    {
        BitStream.ReadWString  ( GameMgr.m_Score.Team[0].Name );
        BitStream.ReadVector   ( m_FlagStand[0] );
        BitStream.ReadRangedF32( m_FlagYaw[0], 10, R_0, R_360 );

        BitStream.ReadWString  ( GameMgr.m_Score.Team[1].Name );
        BitStream.ReadVector   ( m_FlagStand[1] ); 
        BitStream.ReadRangedF32( m_FlagYaw[1], 10, R_0, R_360 );
    }

    //
    // Try to read SPECIFIC_DATA data.
    //
    if( BitStream.ReadFlag() )
    {         
        for( i = 0; i < 2; i++ )
        {
            BitStream.ReadRangedS32( GameMgr.m_Score.Team[i].Humans, 0, 32 );
            BitStream.ReadRangedS32( GameMgr.m_Score.Team[i].Bots,   0, 32 );
            BitStream.ReadRangedS32( (s32&)m_FlagStatus[i],         -3, 31 );
            BitStream.ReadObjectID ( m_Flag[i] );

            if     ( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score,    0,  1023 );
            else if( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score, 1024, 10000 );
            else                             BitStream.ReadS32      ( Score );

            GameMgr.m_Score.Team[i].Score = Score;
            GameMgr.m_Score.Team[i].Size  = GameMgr.m_Score.Team[i].Humans +
                                            GameMgr.m_Score.Team[i].Bots;
        }
    }
}

//==============================================================================

void ctf_logic::ProvideUpdate( bitstream& BS, u32& DirtyBits )
{
    s32 i;
    s32 Score;

    //
    // Try to pack up the SPECIFIC_INIT data.
    //
    BS.OpenSection();
    if( BS.WriteFlag( DirtyBits & game_mgr::DIRTY_SPECIFIC_INIT ) )
    {
        m_FlagYaw[0] = x_ModAngle( m_FlagYaw[0] );
        BS.WriteWString  ( GameMgr.m_Score.Team[0].Name );
        BS.WriteVector   ( m_FlagStand[0] );
        BS.WriteRangedF32( m_FlagYaw[0], 10, R_0, R_360 );

        m_FlagYaw[1] = x_ModAngle( m_FlagYaw[1] );
        BS.WriteWString  ( GameMgr.m_Score.Team[1].Name );
        BS.WriteVector   ( m_FlagStand[1] );
        BS.WriteRangedF32( m_FlagYaw[1], 10, R_0, R_360 );
    }
    if( BS.CloseSection() )  DirtyBits &= ~game_mgr::DIRTY_SPECIFIC_INIT;
    else                     BS.WriteFlag( FALSE );

    //
    // Try to pack up SPECIFIC_DATA data.
    //
    BS.OpenSection();
    if( BS.WriteFlag( DirtyBits & game_mgr::DIRTY_SPECIFIC_DATA ) )
    {          
        for( i = 0; i < 2; i++ )
        {
            BS.WriteRangedS32( GameMgr.m_Score.Team[i].Humans, 0, 32 );
            BS.WriteRangedS32( GameMgr.m_Score.Team[i].Bots,   0, 32 );
            BS.WriteRangedS32( m_FlagStatus[i],               -3, 31 );
            BS.WriteObjectID ( m_Flag[i] );

            Score = GameMgr.m_Score.Team[i].Score;
            if     ( BS.WriteFlag( IN_RANGE(    0, Score,  1023 ) ) )  BS.WriteRangedS32( Score,    0,  1023 );
            else if( BS.WriteFlag( IN_RANGE( 1024, Score, 10000 ) ) )  BS.WriteRangedS32( Score, 1024, 10000 );   
            else                                                       BS.WriteS32      ( Score );
        }
    }
    if( BS.CloseSection() )  DirtyBits &= ~game_mgr::DIRTY_SPECIFIC_DATA;
    else                     BS.WriteFlag( FALSE );
}

//==============================================================================
