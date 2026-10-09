//==============================================================================
//
//  PlayerOnline.cpp
// 
//==============================================================================
#if defined(bwatson)
#define X_SUPPRESS_LOGS
#endif
#include <Support/Objects/Player.hpp>
#include <Support/InputMgr/GamePad.hpp>
#include <Support/Objects/ParticleEmiter.hpp>
#include <Support/Objects/Render/PostEffectMgr.hpp>
#include <Support/Objects/SpawnPoint.hpp>
#include <Support/Objects/Event.hpp>
#include <Support/Sound/EventSoundEmitter.hpp>
#include <Support/TemplateMgr/TemplateMgr.hpp>
#include <Support/Characters/Character.hpp>
#include <Support/Characters/Conversation_Packet.hpp>
#include <Support/GameLib/StatsMgr.hpp>
#include <Support/GameLib/RenderContext.hpp>
#include <Support/Dictionary/Global_Dictionary.hpp>
#include <Support/Objects/WeaponSniper.hpp>
#include <Support/Objects/ThirdPersonCamera.hpp>
#include <Support/Objects/WeaponSMP.hpp>
#include <Support/Objects/Corpse.hpp>
#include <Support/NetworkMgr/NetObjMgr.hpp>
#include <Support/NetworkMgr/Voice/VoiceMgr.hpp>
#include <Support/Objects/Ladders/Ladder_Field.hpp>
#include <Support/Objects/GrenadeProjectile.hpp>
#include <Support/Objects/GravChargeProjectile.hpp>
#include <Support/Objects/JumpingBeanProjectile.hpp>
#include <Support/Render/LightMgr.hpp>
#include <Support/Objects/Door.hpp>
#include <Support/Objects/Projector.hpp>
#include <Support/Objects/WeaponMutation.hpp>
#include <Support/StateMgr/StateMgr.hpp>
#include <Support/NetworkMgr/GameMgr.hpp>
#include <Support/Objects/HudObject.hpp>
#include <Support/Characters/ActorEffects.hpp>
#include <Support/Configuration/GameConfig.hpp>
#include <Support/Objects/Turret.hpp>
#include <Support/Objects/WeaponShotgun.hpp>
#include <Support/GameLib/DebugCheats.hpp>
#include <Support/Objects/FocusObject.hpp>
#include <Support/PerceptionMgr/PerceptionMgr.hpp>
#include <Support/Objects/LoreObject.hpp>
#include <Support/Objects/Camera.hpp>

#ifdef X_EDITOR
#include "../Apps/Editor/Project.hpp"
#else
#include <Support/NetworkMgr/MsgMgr.hpp>
#include <Support/Menu/DebugMenu2.hpp>
#endif


static const f32    s_CheckForGameSpeakTime     = 0.25f;


void player::GatherGameSpeakGuid( void )
{
    if ( s_CheckForGameSpeakTime > g_ObjMgr.GetGameDeltaTime( m_GameSpeakCounter ) )
    {
        return ;
    }

    // If already speaking, nothing.
    if ( m_bSpeaking )
    {
        return ;
    }

    if ( m_SpeakToGuid != 0 )
    {
        object* pSpeakTo = g_ObjMgr.GetObjectByGuid( m_SpeakToGuid ) ;

        // Has the object we are talking to been destroyed?
        if ( pSpeakTo == NULL )
        {
            m_SpeakToGuid = 0 ;
            return ;
        }
    }

    m_GameSpeakCounter = g_ObjMgr.GetGameTime() ;

    // Gather all NPC's who the player can speak with in a Bbox around the player.

    // Create the bbox.
    vector3 BoxCenter = GetPosition() ;
    BoxCenter.GetY() += .5f * GetCollisionHeight() ;
    f32     BoxRadius = 200.0f ;
    bbox    GameSpeakBox( BoxCenter, BoxRadius ) ;

    // Some variables for the collision check.
    slot_id CollidedObjectSlot = SLOT_NULL;
    object* pPotentialTarget = NULL;

    // Check the collisions.
    g_ObjMgr.SelectBBox      ( object::ATTR_CHARACTER_OBJECT, GameSpeakBox );

    // Scan the box for living objects.
    for(    CollidedObjectSlot = g_ObjMgr.StartLoop(); 
        CollidedObjectSlot != SLOT_NULL; 
        CollidedObjectSlot = g_ObjMgr.GetNextResult( CollidedObjectSlot ) )
    {
        pPotentialTarget = g_ObjMgr.GetObjectBySlot( CollidedObjectSlot );

        // Check the factions for the character object in the bbox.
        factions TargetObjectFaction = SMP_UTIL_GetFactionForGuid( pPotentialTarget->GetGuid() ) ;

        if(  IsFriendlyFaction( TargetObjectFaction ) )
        {
            ASSERT( pPotentialTarget->IsKindOf( character::GetRTTI() ) ) ;

            character* pCharacter = ( character* ) pPotentialTarget;
            if ( pCharacter->SetPotentialListener( TRUE ) )
            {
                m_SpeakToGuid = pPotentialTarget->GetGuid() ;
            }
            break ;
        }       
    }

    g_ObjMgr.EndLoop();

    // Clear out SpeakToGuid if there is no potential target.
    if ( !pPotentialTarget && m_SpeakToGuid  )
    {
        ASSERT( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->IsKindOf( character::GetRTTI() ) ) ;
        object* pObject = g_ObjMgr.GetObjectByGuid( m_SpeakToGuid ) ;
        character* pCharacter = ( character* ) pObject ;
        pCharacter->SetPotentialListener( FALSE ) ;
        m_SpeakToGuid = 0 ;
    }
}

//==============================================================================

//==============================================================================

void player::OnGameSpeak( void )
{
    ASSERT( m_ActivePlayerPad != -1 );

    // If we are already speaking, we need to verify that the gamespeak emitter is still
    // a valid object.
    if ( m_bSpeaking )
    {
        if ( g_ObjMgr.GetObjectByGuid( m_GameSpeakEmitterGuid ) == NULL )
        {
            m_bSpeaking = FALSE;
        }
    }


    // If there is no one to speak to, nothing.
    if ( m_SpeakToGuid == 0 )
        return ;

    // If already speaking, nothing.
    if ( m_bSpeaking )
    {
        return ;
    }

    object* pSpeakTo = g_ObjMgr.GetObjectByGuid( m_SpeakToGuid ) ;

    // Has the object we are talking to been destroyed?
    if ( pSpeakTo == NULL )
    {
        m_SpeakToGuid = 0 ;
        return ;
    }

    character* pCharacter = NULL ;

    // If object he wants to talk to is already speaking, do nothing.
    if ( m_SpeakToGuid )
    {
        ASSERT( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->IsKindOf( character::GetRTTI() ) ) ;

        pCharacter = ( character* ) pSpeakTo ;
    }


    conversation_packet Request;
    char SoundDescriptorName[64] ;

    if( g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_SPEAK_FOLLOW_STAY ).WasValue )
    {
        switch ( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->GetType() )
        {
        case TYPE_FRIENDLY_SCIENTIST:
            x_sprintf( SoundDescriptorName, "Request_Sci_Follow" ) ;
            break ;
        default:
            return ;

        }
        Request.m_ConvType = CONV_REQUEST_FOLLOW ;
    }
    else
        if( g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_SPEAK_USE_ACTIVATE ).WasValue )
        {
            // Just make sure we didn't get here without having someone to talk to.
            ASSERT( pCharacter ) ;

            switch ( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->GetType() )
            {
            case TYPE_FRIENDLY_SCIENTIST:
                x_sprintf( SoundDescriptorName, "Request_Sci_Activate_Item" ) ;
                break ;
            default:
                return ;
            }


            Request.m_ConvType =  CONV_REQUEST_ACTIVATE_ITEM ;
        }
        else
            if( g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_SPEAK_COVER_ME ).WasValue )
            {
                // Just make sure we didn't get here without having someone to talk to.
                ASSERT( pCharacter ) ;

                switch ( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->GetType() )
                {
                case TYPE_FRIENDLY_SCIENTIST:
                    x_sprintf( SoundDescriptorName, "Request_Stay_Here" ) ;
                    break ;
                default:
                    return ;
                }
                Request.m_ConvType = CONV_REQUEST_STAY ;        
            }
            else
                if( g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_SPEAK_ATTACK_COVER ).WasValue )
                {
                    return ;
                }
                else
                {
                    return ;
                }

                // We create the emitter.  
                m_GameSpeakEmitterGuid = g_ObjMgr.CreateObject( event_sound_emitter::GetObjectType() );
                object* pSndObj = g_ObjMgr.GetObjectByGuid( m_GameSpeakEmitterGuid );
                event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType( *pSndObj );
                Request.m_SpeakerGuid = GetGuid() ;
                Request.m_SoundEmitterGuid = m_GameSpeakEmitterGuid ;
                m_bSpeaking = TRUE;

                vector3 vPos = GetPosition() ;
                EventEmitter.PlayEmitter( SoundDescriptorName, vPos, GetZone1(), 
                    event_sound_emitter::SINGLE_SHOT, GetGuid() ) ; 

                ASSERT( g_ObjMgr.GetObjectByGuid( m_SpeakToGuid )->IsKindOf( character::GetRTTI() ) ) ;

                // Now sent the packet to the character who needs it.
                ASSERT( pCharacter ) ;
                pCharacter->SetActiveListener( TRUE ) ;
                pCharacter->Listen( Request ) ;
}

xbool JIM_BOOL_TEST = FALSE;
//==============================================================================

void player::VoteCast( s32 Vote )
{
    (void)Vote;

    LOG_MESSAGE( "player::VoteCast", "Vote:%d", Vote );

#ifndef X_EDITOR
    if( g_NetworkMgr.IsServer() )
    {
        GameMgr.CastVote( m_NetSlot, Vote );
    }
    else
    {
        m_VoteAction    = 1;
        m_VoteArgument  = Vote;
        m_NetDirtyBits |= VOTE_ACTION_BIT;
    }
#endif
}

//=========================================================================

void player::VoteStartKick( s32 Kick )
{
    (void)Kick;

    LOG_MESSAGE( "player::VoteStartKick", "Kick:%d", Kick );

#ifndef X_EDITOR
    if( g_NetworkMgr.IsServer() )
    {
        GameMgr.StartVoteKick( Kick, m_NetSlot );
    }
    else
    {
        m_VoteAction    = 2;
        m_VoteArgument  = Kick;
        m_NetDirtyBits |= VOTE_ACTION_BIT;
    }
#endif
}

//=========================================================================

void player::VoteStartMap( s32 Map )
{
    (void)Map;

    LOG_MESSAGE( "player::VoteStartMap", "Map:%d", Map );

#ifndef X_EDITOR
    if( g_NetworkMgr.IsServer() )
    {
        GameMgr.StartVoteMap( Map, m_NetSlot );
    }
    else
    {
        m_VoteAction    = 3;
        m_VoteArgument  = Map;
        m_NetDirtyBits |= VOTE_ACTION_BIT;
    }
#endif
}

//=========================================================================

