//==============================================================================
//
//  HUNT_Logic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "HUNT_Logic.hpp"
#include "GameMgr.hpp"
#include "MsgMgr.hpp"
#include "..\AADS\Data\UI\Messages.h"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Projectiles\Flag.hpp"
#include "AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define CHANCE(n)   (x_irand(1,100) <= (n))

//==============================================================================
//  STORAGE
//==============================================================================

hunt_logic   HUNT_Logic;
s32          FlagPoints[4] = { 1, 2, 4, 8 };

//==============================================================================
//  FUNCTIONS
//==============================================================================

hunt_logic::hunt_logic( void )
{
}

//==============================================================================

hunt_logic::~hunt_logic( void )
{
}

//==============================================================================

void hunt_logic::BeginGame( void )
{
    s32 i;

    for( i = 0; i < 32; i++ )
        GameMgr.m_Score.Player[i].Deaths = 1;  

    GameMgr.SetScoreLimit( 100000 );
}

//==============================================================================

void hunt_logic::Connect( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    GameMgr.m_Score.Player[PlayerIndex].Team = PlayerIndex;
    GameMgr.m_PlayerBits |= (1 << PlayerIndex);
}

//==============================================================================

void hunt_logic::EnterGame( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    GameMgr.m_Score.Player[PlayerIndex].Deaths = 1;     // Flags.
    GameMgr.m_ScoreBits  |= (1 << PlayerIndex);
}

//==============================================================================

void hunt_logic::EnforceBounds( f32 DeltaTime )
{   
    (void)DeltaTime;

    // Any player which goes out of bounds loses all flags except original one.
    for( s32 i = 0; i < 32; i++ )
    {
        // No flags (except original one)?
        if( GameMgr.m_Score.Player[i].Deaths == 1 )
            continue;

        object::id ID( i, -1 );
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( ID );

        if( !pPlayer )
            continue;     

        vector3 Position = pPlayer->GetPosition();

        if( (Position.X < GameMgr.m_Bounds.Min.X) ||
            (Position.Z < GameMgr.m_Bounds.Min.Z) ||
            (Position.X > GameMgr.m_Bounds.Max.X) ||
            (Position.Z > GameMgr.m_Bounds.Max.Z) )
        {
            // Drop 'em!
            DropFlags( pPlayer->GetPosition(), 
                       pPlayer->GetVel(), 
                       pPlayer->GetObjectID(),
                       GameMgr.m_Score.Player[i].Deaths - 1 );

            if( GameMgr.m_Score.Player[i].Deaths > 5 )
                MsgMgr.Message( MSG_HUNT_FLAGS_DROPPED + x_irand(0,3), 0,
                                ARG_WARRIOR, i, 
                                ARG_N, GameMgr.m_Score.Player[i].Deaths-1 );

            if( GameMgr.m_Score.Player[i].Deaths > 1 )
                MsgMgr.Message( MSG_HUNT_YOU_DROPPED_FLAGS, i,
                                ARG_N, GameMgr.m_Score.Player[i].Deaths-1 );

            // Player always has his own single flag.
            GameMgr.m_Score.Player[i].Deaths = 1;
            GameMgr.m_ScoreBits |= (1 << i);
            pPlayer->SetFlagCount( 1 );
            pPlayer->DetachFlag();
        }
    }

    // Attempt to keep the flags in bounds.
    {
        flag* pFlag;
        ObjMgr.StartTypeLoop( object::TYPE_FLAG );
        while( (pFlag = (flag*)ObjMgr.GetNextInType()) )
        {
            xbool   Dirty    = FALSE;
            vector3 Position = pFlag->GetPosition();
            vector3 Velocity = pFlag->GetVelocity();

            if( Position.X < GameMgr.m_Bounds.Min.X )
            {
                Velocity.X = x_frand( 5.0f, 15.0f );
                Dirty      = TRUE;
            }

            if( Position.X > GameMgr.m_Bounds.Max.X )
            {
                Velocity.X = -x_frand( 5.0f, 15.0f );
                Dirty      = TRUE;
            }

            if( Position.Z < GameMgr.m_Bounds.Min.Z )
            {
                Velocity.Z = x_frand( 5.0f, 15.0f );
                Dirty      = TRUE;
            }

            if( Position.Z > GameMgr.m_Bounds.Max.Z )
            {
                Velocity.Z = -x_frand( 5.0f, 15.0f );
                Dirty      = TRUE;
            }

            if( Dirty )
            {
                Velocity.Y = x_frand( 5.0f, 15.0f );
                pFlag->SetVelocity( Velocity );
            }
        }
        ObjMgr.EndTypeLoop();
    }
}

//==============================================================================

void hunt_logic::PlayerSpawned( object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    game_logic::PlayerSpawned( PlayerID );

    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    s32 PlayerIndex = IndexFromID( PlayerID );

    pPlayer->SetFlagCount( 1 );
    GameMgr.m_Score.Player[ PlayerIndex ].Deaths = 1;
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);
}

//==============================================================================

void hunt_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    game_logic::PlayerDied( Pain );

    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( Pain.VictimID );
    s32 PlayerIndex = IndexFromID( Pain.VictimID );

    // The call to game_logic::PlayerDied added 1 to the Deaths.  Compensate.
    GameMgr.m_Score.Player[PlayerIndex].Deaths -= 1;
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);

    // Remove flag from player's back if present.
    if( GameMgr.m_Score.Player[PlayerIndex].Deaths >= 5 )
        pPlayer->DetachFlag();

    // Drop the flags.
    DropFlags( pPlayer->GetPosition(), 
               pPlayer->GetVel(),
               Pain.VictimID,
               GameMgr.m_Score.Player[PlayerIndex].Deaths );

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths > 5 )
        MsgMgr.Message( MSG_HUNT_FLAGS_DROPPED + x_irand(0,3), 0,
                        ARG_WARRIOR, PlayerIndex, 
                        ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths );

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths > 1 )
        MsgMgr.Message( MSG_HUNT_YOU_DROPPED_FLAGS, PlayerIndex,
                        ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths );

    GameMgr.m_Score.Player[PlayerIndex].Deaths = 0;
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);
    pPlayer->SetFlagCount( 0 );

    GameMgr.SendAudio( SFX_MISC_HUNTERS_FLAG_DROP, pPlayer->GetBlendPos() );
}

//==============================================================================

void hunt_logic::FlagTouched( object::id FlagID, object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    flag*          pFlag   = (flag*)         ObjMgr.GetObjectFromID( FlagID   );
    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );

    if( !pFlag )    return;
    if( !pPlayer )  return;

    s32 PlayerIndex = IndexFromID( PlayerID );

    GameMgr.m_Score.Player[PlayerIndex].Deaths += FlagPoints[ pFlag->GetValue() ];
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths >= 5 )
        pPlayer->AttachFlag( 0 );

    pPlayer->SetFlagCount( GameMgr.m_Score.Player[PlayerIndex].Deaths );

    MsgMgr.Message( MSG_HUNT_YOUR_FLAGS, PlayerIndex,
                    ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths );

    GameMgr.SendAudio( SFX_MISC_HUNTERS_FLAG_GRAB, pPlayer->GetBlendPos() );

    ObjMgr.DestroyObject( FlagID );
}

//==============================================================================

xbool hunt_logic::FlagHitPlayer( object::id FlagID, object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return( FALSE );

    flag*          pFlag   = (flag*)         ObjMgr.GetObjectFromID( FlagID   );
    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );

    if( !pFlag )    return( FALSE );
    if( !pPlayer )  return( FALSE );

    s32 PlayerIndex = IndexFromID( PlayerID );

    GameMgr.m_ScoreBits |= (1 << PlayerIndex);
    GameMgr.m_Score.Player[PlayerIndex].Deaths += FlagPoints[ pFlag->GetValue() ];

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths >= 5 )
        pPlayer->AttachFlag( 0 );

    pPlayer->SetFlagCount( GameMgr.m_Score.Player[PlayerIndex].Deaths );

    MsgMgr.Message( MSG_HUNT_YOUR_FLAGS, PlayerIndex,
                    ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths );

    GameMgr.SendAudio( SFX_MISC_HUNTERS_FLAG_GRAB, pPlayer->GetBlendPos() );

    return( TRUE );
}

//==============================================================================

void hunt_logic::NexusTouched( object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    s32 PlayerIndex = IndexFromID( PlayerID );

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths > 1 )
    {
        s32 Score = 0;
        for( s32 i = 1; i <= GameMgr.m_Score.Player[PlayerIndex].Deaths; i++ )
            Score += SCORE_CASH_IN_FLAG * i;
            
        GameMgr.ScoreForPlayer( PlayerID, Score );

        MsgMgr.Message( MSG_HUNT_YOUR_SCORE, PlayerIndex,
                        ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths,
                        ARG_M, Score );

        if( CHANCE(50) )
        {
            MsgMgr.Message( MSG_HUNT_FLAGS_TO_NEXUS, 0, 
                            ARG_WARRIOR, PlayerIndex, 
                            ARG_N, GameMgr.m_Score.Player[PlayerIndex].Deaths );
        }
        else
        {
            MsgMgr.Message( MSG_HUNT_POINTS, 0, 
                            ARG_WARRIOR, PlayerIndex, 
                            ARG_N, Score );
        }

        GameMgr.m_Score.Player[PlayerIndex].Deaths = 1;
        GameMgr.m_ScoreBits |= (1 << PlayerIndex);

        pPlayer->DetachFlag();
        pPlayer->SetFlagCount( 1 );
        GameMgr.SendAudio( SFX_MISC_NEXUS_CAP, pPlayer->GetBlendPos() );
    }
}

//==============================================================================

void hunt_logic::ThrowFlags( object::id PlayerID )
{
    if( !tgl.ServerPresent )
        return;

    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    s32 PlayerIndex = IndexFromID( PlayerID );

    if( !pPlayer )  return;

    if( GameMgr.m_Score.Player[PlayerIndex].Deaths > 1 )
    {
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );

        vector3 Direction(0,5,20);
        Direction.Rotate( pPlayer->GetRot() );

        // Drop 'em!
        DropFlags( pPlayer->GetPosition() + vector3(0,2,0), 
                   pPlayer->GetVel() + Direction, 
                   pPlayer->GetObjectID(),
                   GameMgr.m_Score.Player[PlayerIndex].Deaths - 1 );

        // Player always has his own single flag.
        GameMgr.m_Score.Player[PlayerIndex].Deaths = 1;
        GameMgr.m_ScoreBits |= (1 << PlayerIndex);
        pPlayer->SetFlagCount( 1 );
        pPlayer->DetachFlag();
    }
}

//==============================================================================

void hunt_logic::DropFlags( const vector3&   aPosition, 
                            const vector3&   aVelocity,
                                  object::id aOriginID,
                                  s32        aCount )
{
    vector3 Position = aPosition;
    Position.Y += 1.5f;

    // Drop the flags.
    while( aCount > 0 )
    {
        vector3 Velocity = aVelocity;
        s32     N;
        
        Velocity.X  += x_frand( -5.0f, 5.0f );
        Velocity.Y  += x_frand(  2.0f, 5.0f );
        Velocity.Z  += x_frand( -5.0f, 5.0f );

        N = x_irand(0,3);
        while( FlagPoints[N] > aCount )
            N--;

        flag* pFlag = (flag*)ObjMgr.CreateObject( object::TYPE_FLAG );
        pFlag->Initialize( Position, x_frand( R_0, R_360 ), Velocity, aOriginID, 0, N );
        ObjMgr.AddObject( pFlag, obj_mgr::AVAILABLE_SERVER_SLOT );

        aCount -= FlagPoints[N];
    }
}

//==============================================================================
