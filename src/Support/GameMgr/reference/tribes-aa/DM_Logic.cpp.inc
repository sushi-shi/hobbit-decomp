//==============================================================================
//
//  DM_Logic.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "DM_Logic.hpp"
#include "GameMgr.hpp"
#include "MsgMgr.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Projectiles\Bounds.hpp"
#include "AADS\Globals.hpp"

#include "..\AADS\Data\UI\Messages.h"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

dm_logic   DM_Logic;

//==============================================================================
//  FUNCTIONS
//==============================================================================

dm_logic::dm_logic( void )
{
}

//==============================================================================

dm_logic::~dm_logic( void )
{
}

//==============================================================================

void dm_logic::Connect( s32 PlayerIndex )
{
    if( !tgl.ServerPresent )
        return;

    GameMgr.m_Score.Player[PlayerIndex].Team = PlayerIndex;

    // Set dirty bits.
    GameMgr.m_PlayerBits |= (1 << PlayerIndex);
}

//==============================================================================

void dm_logic::Initialize( void )
{
    bounds_SetFatal( TRUE );
}

//==============================================================================

void dm_logic::PlayerDied( const pain& Pain )
{
    if( !tgl.ServerPresent )
        return;

    MsgMgr.PlayerDied( Pain );

    s32 PlayerIndex = IndexFromID( Pain.VictimID );
    s32 KillerIndex = IndexFromID( Pain.OriginID );

    GameMgr.m_Score.Player[PlayerIndex].Deaths += 1;
    GameMgr.m_ScoreBits |= (1 << PlayerIndex);

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
            ASSERT( FALSE );
        }
        else
        {
            // Killed by enemy.
            GameMgr.ScoreForPlayer( Pain.OriginID, SCORE_KILL_ENEMY_PLAYER );
            GameMgr.m_Score.Player[KillerIndex].Kills += 1;
            GameMgr.m_ScoreBits |= (1 << KillerIndex);
        }
    }
}

//==============================================================================

void dm_logic::EnforceBounds( f32 DeltaTime )
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

