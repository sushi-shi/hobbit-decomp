//==============================================================================
//
//  CNH_Logic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "CNH_Logic.hpp"
#include "GameMgr.hpp"
#include "netlib/bitstream.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Projectiles\Flag.hpp"
#include "Objects\Projectiles\FlipFlop.hpp"
#include "AADS\Globals.hpp"

#include "..\AADS\Data\UI\Messages.h"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

cnh_logic   CNH_Logic;

//==============================================================================
//  FUNCTIONS
//==============================================================================

cnh_logic::cnh_logic( void )
{
}

//==============================================================================

cnh_logic::~cnh_logic( void )
{
}

//==============================================================================

void cnh_logic::Connect( s32 PlayerIndex )
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

void cnh_logic::ExitGame( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    s32 i;

    // Clear "capture player" from switches for this player.
    for( i = 0; i < 12; i++ )
    {
        s32 Index = IndexFromID( m_CapturePlayerID[i] );
        if( PlayerIndex == Index )
        {
            m_CapturePlayerID[i] = obj_mgr::NullID;
        }
    }
}

//==============================================================================

void cnh_logic::Disconnect( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

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

void cnh_logic::ChangeTeam( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    s32 i;

    // Clear "capture player" from switches for this player.
    for( i = 0; i < 12; i++ )
    {
        s32 Index = IndexFromID( m_CapturePlayerID[i] );
        if( PlayerIndex == Index )
        {
            m_CapturePlayerID[i] = obj_mgr::NullID;
        }
    }
}

//==============================================================================

void cnh_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    game_logic::PlayerDied( Pain );

    s32 i;

    // Clear "capture player" from switches for this player.
    for( i = 0; i < 12; i++ )
    {
        s32 PlayerIndex = IndexFromID( Pain.VictimID );
        s32 Index       = IndexFromID( m_CapturePlayerID[i] );
        if( (PlayerIndex != -1) && (PlayerIndex == Index) )
        {
            m_CapturePlayerID[i] = obj_mgr::NullID;
        }
    }

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void cnh_logic::SwitchTouched( object::id SwitchID, object::id PlayerID )
{
    flipflop* pFlipFlop = (flipflop*)ObjMgr.GetObjectFromID( SwitchID );

    s32 PlayerIndex = IndexFromID( PlayerID );
    s32 SwitchIndex = pFlipFlop->GetSwitch() - 4;   // -4?  See note in header.

    m_HoldTime       [SwitchIndex] = 0.0f;
    m_First10Seconds [SwitchIndex] = TRUE;
    m_HoldTeam       [SwitchIndex] = GameMgr.m_Score.Player[PlayerIndex].Team;
    m_CapturePlayerID[SwitchIndex] = PlayerID;

    GameMgr.ScoreForPlayer( PlayerID, SCORE_ACQUIRE_SWITCH, MSG_SCORE_TAKE_OBJECTIVE );
}

//==============================================================================

void cnh_logic::AdvanceTime( f32 DeltaTime )
{   
    s32 i;

    if( !tgl.ServerPresent )
        return;

    for( i = 0; i < 12; i++ )
    {
        // Held by any team yet?
        if( m_HoldTeam[i] == -1 )
            continue;

        // Advance the timer.
        m_HoldTime[i] += DeltaTime;

        // Held for initial full 10 seconds yet?
        if( m_First10Seconds[i] && (m_HoldTime[i] >= 10.0f) )
        {
            m_First10Seconds[i] = FALSE;

            s32 PlayerIndex = IndexFromID( m_CapturePlayerID[i] );
            if( (PlayerIndex != -1) && 
                (GameMgr.m_Score.Player[PlayerIndex].Team == m_HoldTeam[i]) )
            {
                GameMgr.ScoreForPlayer( m_CapturePlayerID[i], 
                                        SCORE_HOLD_SWITCH, 
                                        MSG_SCORE_CNH_HOLD_OBJECTIVE );
            }

            m_CapturePlayerID[i] = obj_mgr::NullID;

            GameMgr.m_Score.Team[ m_HoldTeam[i] ].Score += SCORE_HOLD_SWITCH_PER_SEC;
        
            // Set dirty bits.
            GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
            
            m_HoldTime[i] -= 10.0f;
        }

        // Held for a 1 second after initial 10?
        if( !m_First10Seconds[i] && (m_HoldTime[i] >= 1.0f) )
        {
            // Team gets points.
            GameMgr.m_Score.Team[ m_HoldTeam[i] ].Score += SCORE_HOLD_SWITCH_PER_SEC;
            
            // Set dirty bits.
            GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;

            m_HoldTime[i] -= 1.0f;
        }
    }
}

//==============================================================================

void cnh_logic::BeginGame( void )
{
    s32 i;

    if( !tgl.ServerPresent )
        return;

    // Clear timers for each switch.
    for( i = 0; i < 12; i++ )
    {
        m_HoldTeam[i] = -1;
        m_HoldTime[i] = -1.0f;
    }

    GameMgr.SetScoreLimit( 100000 );
}

//==============================================================================

void cnh_logic::EndGame( void )
{
    if( !tgl.ServerPresent )
        return;
}

//==============================================================================

void cnh_logic::AcceptUpdate( const bitstream& BitStream )
{
    s32 i;
    s32 Score;

    ASSERT( tgl.ClientPresent );

    //
    // Try to read the SPECIFIC_INIT data.
    //
    if( BitStream.ReadFlag() )
    {
        BitStream.ReadWString( GameMgr.m_Score.Team[0].Name );
        BitStream.ReadWString( GameMgr.m_Score.Team[1].Name );
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

            if     ( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score,    0,   4095 );
            else if( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score, 4096, 100000 );
            else                             BitStream.ReadS32( Score );

            GameMgr.m_Score.Team[i].Score = Score;
            GameMgr.m_Score.Team[i].Size  = GameMgr.m_Score.Team[i].Humans +
                                            GameMgr.m_Score.Team[i].Bots;
        }
    }
}

//==============================================================================

void cnh_logic::ProvideUpdate( bitstream& BS, u32& DirtyBits )
{
    s32 i;
    s32 Score;

    ASSERT( tgl.ServerPresent );

    //
    // Try to pack up the SPECIFIC_INIT data.
    //
    BS.OpenSection();
    if( BS.WriteFlag( DirtyBits & game_mgr::DIRTY_SPECIFIC_INIT ) )
    {
        BS.WriteWString( GameMgr.m_Score.Team[0].Name );
        BS.WriteWString( GameMgr.m_Score.Team[1].Name );
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

            Score = GameMgr.m_Score.Team[i].Score;
            if     ( BS.WriteFlag( IN_RANGE(    0, Score,   4095 ) ) )  BS.WriteRangedS32( Score,    0,   4095 );
            else if( BS.WriteFlag( IN_RANGE( 4096, Score, 100000 ) ) )  BS.WriteRangedS32( Score, 4096, 100000 );   
            else                                                        BS.WriteS32      ( Score );
        }
    }
    if( BS.CloseSection() )  DirtyBits &= ~game_mgr::DIRTY_SPECIFIC_DATA;
    else                     BS.WriteFlag( FALSE );
}

//==============================================================================
