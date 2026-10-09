//==============================================================================
//
//  TDM_Logic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "TDM_Logic.hpp"
#include "GameMgr.hpp"
#include "MsgMgr.hpp"

#include "netlib/bitstream.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Projectiles\Bounds.hpp"
#include "AADS\Globals.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

tdm_logic   TDM_Logic;

//==============================================================================
//  FUNCTIONS
//==============================================================================

tdm_logic::tdm_logic( void )
{
}

//==============================================================================

tdm_logic::~tdm_logic( void )
{
}

//==============================================================================

void tdm_logic::BeginGame( void )
{
    if( !tgl.ServerPresent )
        return;

    GameMgr.SetScoreLimit( 5000 );
}

//==============================================================================

void tdm_logic::Initialize( void )
{
    bounds_SetFatal( TRUE );
}

//==============================================================================

void tdm_logic::AdvanceTime( f32 DeltaTime )
{
    game_logic::AdvanceTime( DeltaTime );

    for( s32 i = 0; i < 32; i++ )
    {
        if( m_PlayerDead[i] && (m_TimeDead[i] > 5.0f) )
        {
            player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromSlot( i );
            if( pPlayer )
            {
                if( pPlayer->IsDead() )
                {   
                    pPlayer->Respawn();
                }
                else
                {
                    m_PlayerDead[i] = FALSE;
                }
            }
        }
    }
}

//==============================================================================

void tdm_logic::Connect( s32 PlayerIndex )
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

void tdm_logic::Disconnect( s32 PlayerIndex )
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
}

//==============================================================================

void tdm_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    s32 PlayerIndex = IndexFromID( Pain.VictimID );
    s32 KillerIndex = IndexFromID( Pain.OriginID );

    if( PlayerIndex != -1 )
    {
        ASSERT( GameMgr.m_Score.Player[PlayerIndex].IsInGame );
        s32 PlayerTeam = GameMgr.m_Score.Player[PlayerIndex].Team;

        if( KillerIndex == -1 )
        {
            // No killer.  (Bounds, land too hard.)
            GameMgr.m_Score.Team[ PlayerTeam ].Score -= 1;
        }
        else
        {
            ASSERT( GameMgr.m_Score.Player[KillerIndex].IsInGame );
            s32 KillerTeam = GameMgr.m_Score.Player[KillerIndex].Team;

            if( PlayerTeam == KillerTeam )
            {
                // Team kill.
                GameMgr.m_Score.Team[ KillerTeam ].Score -= 1;
            }
            else
            {
                // Enemy kill.
                GameMgr.m_Score.Team[ KillerTeam ].Score += 1;
            }
        }
    }

    /*
    if( (KillerIndex != -1) && (PlayerIndex != -1) )
    {
        ASSERT( GameMgr.m_Score.Player[PlayerIndex].IsInGame );
        ASSERT( GameMgr.m_Score.Player[KillerIndex].IsInGame );

        s32 PlayerTeam = GameMgr.m_Score.Player[PlayerIndex].Team;
        s32 KillerTeam = GameMgr.m_Score.Player[KillerIndex].Team;

        if( PlayerTeam == KillerTeam )
            GameMgr.m_Score.Team[ KillerTeam ].Score -= 1;
        else
            GameMgr.m_Score.Team[ KillerTeam ].Score += 1;
    }
    */

    game_logic::PlayerDied( Pain );

    // Set dirty bits.
    GameMgr.m_DirtyBits |= game_mgr::DIRTY_SPECIFIC_DATA;
}

//==============================================================================

void tdm_logic::SwitchTouched( object::id SwitchID, object::id PlayerID )
{
    (void)SwitchID;
    
    GameMgr.ScoreForPlayer( PlayerID, SCORE_ACQUIRE_SWITCH );
}

//==============================================================================

void tdm_logic::AcceptUpdate( const bitstream& BitStream )
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

            if     ( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score, -255,   255 );
            else if( BitStream.ReadFlag() )  BitStream.ReadRangedS32( Score,  256,  5000 );
            else                             BitStream.ReadS32      ( Score );    

            GameMgr.m_Score.Team[i].Score = Score;
            GameMgr.m_Score.Team[i].Size  = GameMgr.m_Score.Team[i].Humans +
                                            GameMgr.m_Score.Team[i].Bots;
        }
    }
}

//==============================================================================

void tdm_logic::ProvideUpdate( bitstream& BS, u32& DirtyBits )
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
            if     ( BS.WriteFlag( IN_RANGE( -255, Score,  255 ) ) )  BS.WriteRangedS32( Score, -255,  255 );
            else if( BS.WriteFlag( IN_RANGE(  256, Score, 5000 ) ) )  BS.WriteRangedS32( Score,  256, 5000 );   
            else                                                      BS.WriteS32      ( Score );
        }
    }
    if( BS.CloseSection() )  DirtyBits &= ~game_mgr::DIRTY_SPECIFIC_DATA;
    else                     BS.WriteFlag( FALSE );
}

//==============================================================================

void tdm_logic::EnforceBounds( f32 DeltaTime )
{   
    // Any player which goes out of bounds begins to feel the burn.
    for( s32 i = 0; i < 32; i++ )
    {
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
            ObjMgr.InflictPain( pain::MISC_BOUNDS_HURT,
                                pPlayer, 
                                Position, 
                                ObjMgr.NullID, 
                                ID, 
                                TRUE,
                                DeltaTime );
        }           
    }
}

//==============================================================================
