//==============================================================================
//
//  Station.cpp
//
//==============================================================================

// TO DO: Message "Unit is not powered."

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Station.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "AudioMgr/Audio.hpp"
#include "GameMgr\GameMgr.hpp"   
#include "LabelSets/Tribes2Types.hpp"
#include "Objects\Projectiles\ParticleObject.hpp"
#include "Objects\Projectiles\VehiclePad.hpp"
#include "Objects\Projectiles\Bubble.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "..\AADS\Globals.hpp"
#include "..\AADS\GameServer.hpp"
#include "..\AADS\Data\UI\Messages.h"
#include "Objects\Projectiles\Beacon.hpp"

#include <Support/StringMgr/StringMgr.hpp>
#include "AADS/Data/UI/ui_strings.h"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   StationCreator;

//==============================================================================
    
obj_type_info   StationFixedTypeInfo   ( object::TYPE_STATION_FIXED, 
                                         "Fixed Station", 
                                         StationCreator, 
                                         object::ATTR_SOLID_DYNAMIC |
                                         object::ATTR_PERMANENT     | 
                                         object::ATTR_DAMAGEABLE    |
                                         object::ATTR_REPAIRABLE    |
                                         object::ATTR_ASSET         |
                                         object::ATTR_ELFABLE       |
                                         object::ATTR_LABELED );

//==============================================================================

obj_type_info   StationVehicleTypeInfo ( object::TYPE_STATION_VEHICLE,
                                         "Vehicle Station", 
                                         StationCreator, 
                                         object::ATTR_SOLID_DYNAMIC |
                                         object::ATTR_PERMANENT     | 
                                         object::ATTR_DAMAGEABLE    |
                                         object::ATTR_REPAIRABLE    |
                                         object::ATTR_ASSET         |
                                         object::ATTR_ELFABLE       |
                                         object::ATTR_LABELED );

//==============================================================================

obj_type_info   StationDeployedTypeInfo( object::TYPE_STATION_DEPLOYED, 
                                         "Deployed Station", 
                                         StationCreator, 
                                         object::ATTR_SOLID_DYNAMIC |
                                         object::ATTR_DAMAGEABLE    |
                                         object::ATTR_REPAIRABLE    |
                                         object::ATTR_ASSET         |
                                         object::ATTR_ELFABLE       |
                                         object::ATTR_LABELED );

                                         // Not PERMANENT.

//==============================================================================

                                  // Fixed,Vehicle,Deployed  
static f32  LockTime        [3] = {  1.5f,  1.0f,  1.0f };
static f32  DelayTime       [3] = {  1.06f,-0.0f,  1.0f };
static f32  CoolDownTime    [3] = {  0.5f,  0.5f,  0.3f };
static f32  TriggerRadius   [3] = {  0.8f,  0.8f,  0.5f };
static f32  TriggerHi       [3] = {  1.1f,  1.1f,  1.5f };
static f32  TriggerLo       [3] = {  0.1f,  0.1f,  1.1f };
static f32  ReleaseRadius   [3] = {  1.1f,  1.1f,  0.8f };
static f32  ReleaseHi       [3] = {  2.5f,  2.5f,  2.0f };
static f32  ReleaseLo       [3] = {  0.3f,  0.3f,  1.2f };

static s32  ShapeIndex      [3] = { SHAPE_TYPE_INVENT_STATION_HUMAN,   
                                    SHAPE_TYPE_INVENT_STATION_VEHICLE, 
                                    SHAPE_TYPE_INVENT_STATION_DEPLOY };

static s32  CollisionIndex  [3] = { SHAPE_TYPE_INVENT_STATION_HUMAN_COLL,   
                                    SHAPE_TYPE_INVENT_STATION_VEHICLE_COLL, 
                                    SHAPE_TYPE_INVENT_STATION_DEPLOY_COLL };

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* StationCreator( void )
{
    return( (object*)(new station) );
}

//==============================================================================

station::station( void )
{
    m_Handle = 0;
}

//==============================================================================

void station::OnRemove( void )
{
    if( m_Handle != 0 )
    {
        audio_Stop( m_Handle );
        m_Handle = 0;
    }
}

//==============================================================================

object::update_code station::OnAdvanceLogic( f32 DeltaTime )
{
    if( (m_Health > 0.0f) && 
        (m_Shape.GetAnimByType() == ANIM_TYPE_INVENT_DESTROYED) )
    {   
        m_Shape.SetAnimByType( ANIM_TYPE_INVENT_IDLE );
    }

    m_Shape.Advance( DeltaTime );
    m_Timer += DeltaTime;

    if( tgl.ServerPresent )
        return( ServerLogic( DeltaTime ) );
    else
        return( ClientLogic( DeltaTime ) );
}

//==============================================================================

object::update_code station::ServerLogic( f32 DeltaTime )
{
    s32 ResultCode;
    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_PlayerID );

    asset::OnAdvanceLogic( DeltaTime );

    // Make sure we don't cripple a player by leaving him in a funky state.
    if( m_Disabled )
    {
        if( m_State != STATE_IDLE )
        {
            if( pPlayer )
            {
                if( pPlayer->Player_GetState() == player_object::PLAYER_STATE_AT_INVENT )
                    pPlayer->Player_SetupState( player_object::PLAYER_STATE_IDLE );
                pPlayer->SetArmed( TRUE );
            }
            EnterState( STATE_IDLE );
        }
    }

    // Make sure player is still there.
    if( pPlayer && pPlayer->IsDead() )
    {
        m_PlayerID = obj_mgr::NullID;
        pPlayer    = NULL;
    }

    // If the station thinks the player is there, but the player does not think 
    // he is at a station, then the station must forget about him.
    if( pPlayer && 
        (pPlayer->Player_GetState() != player_object::PLAYER_STATE_AT_INVENT) &&
        ((m_State == STATE_PLAYER_EQUIP) || (m_State == STATE_PRE_VEHICLE_REQUEST)) )
    {
        m_PlayerID = obj_mgr::NullID;
        pPlayer    = NULL;
    }

    // If this is a dead deployable station, return DESTROY.
    if( (m_Health == 0.0f) && (m_Kind == DEPLOYED) )
    {
        return( DESTROY );
    }

/*
    // If this station is disabled, bail out now.
    if( m_Disabled )
    {
        return( NO_UPDATE );
    }
*/
    
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:

            // If there is a player set, then that player is "denied".
            // See if we can expire the denial.
            if( (m_PlayerID.Slot != -1) && (m_Timer > 2.0f) )
                m_PlayerID = obj_mgr::NullID;

            // Look for a player in the trigger area.
            ResultCode = LookForPlayer();

            // Denied?
            if( ResultCode >= 0 )
            {
                MsgMgr.Message( ResultCode, m_PlayerID.Slot );
                m_Timer = 0.0f;
            }

            // Accepted?
            if( ResultCode == -1 )
            {
                // New PlayerID, so get new pPlayer.
                pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_PlayerID );

                // Cover our ass in several ways.
                ASSERT( pPlayer );
                ASSERT( !pPlayer->IsDead() );
                ASSERT( pPlayer->Player_GetState() != player_object::PLAYER_STATE_AT_INVENT );

                pPlayer->Player_SetupState( player_object::PLAYER_STATE_AT_INVENT );
                pPlayer->SetArmed( FALSE );

                //------------------------------------------
                if( m_Kind == FIXED )
                {
                    pPlayer->WarpToPos( m_SnapPosition );
                    EnterState( STATE_PLAYER_EQUIP );
                }

                //------------------------------------------
                if( m_Kind == DEPLOYED )
                {
                    DeployBeacon();
                    EnterState( STATE_PLAYER_EQUIP );
                }

                //------------------------------------------
                if( m_Kind == VEHICLE )
                {
                    radian3 Rotation = m_WorldRot;
                    Rotation.Yaw += R_180;   
                    pPlayer->SetRot( Rotation );
                    pPlayer->WarpToPos( m_SnapPosition );
                    EnterState( STATE_PRE_VEHICLE_REQUEST );
                }
            }

            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_EQUIP:

            // A player is "locked" on the pad now.  During this time, the
            // armour is repaired.

            if( pPlayer )
            {
                ObjMgr.InflictPain( (m_Kind == FIXED)
                                        ? pain::REPAIR_VIA_FIXED_INVEN
                                        : pain::REPAIR_VIA_DEPLOYED_INVEN,
                                    pPlayer->GetPosition(),
                                    m_ObjectID, 
                                    pPlayer->GetObjectID(),
                                    DeltaTime );
            }

            // The player's loadout changes after the delay time.
            if( (m_Timer               >  DelayTime[ m_Kind ]) && 
                ((m_Timer - DeltaTime) <= DelayTime[ m_Kind ]) &&
                (pPlayer) )
            {
                pPlayer->InventoryReload( m_Kind == DEPLOYED );
                pGameLogic->StationUsed( m_PlayerID );
            }

            // The player is released from the station after the lock time.
            if( m_Timer > LockTime[ m_Kind ] )
            {
                if( pPlayer )
                    pPlayer->Player_SetupState( player_object::PLAYER_STATE_IDLE );
                EnterState( STATE_PLAYER_REPAIR ); 
            }

            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_REPAIR:

            // A player is still standing on the pad.  During this time, the
            // armour is repaired.

            if( pPlayer )
            {
                ObjMgr.InflictPain( (m_Kind == FIXED)
                                        ? pain::REPAIR_VIA_FIXED_INVEN
                                        : pain::REPAIR_VIA_DEPLOYED_INVEN,
                                    pPlayer->GetPosition(),
                                    m_ObjectID, 
                                    pPlayer->GetObjectID(),
                                    DeltaTime );

                // Did player leave the release box?
                if( !m_ReleaseBBox.Intersect( pPlayer->GetPosition() ) )
                {
                    EnterState( STATE_COOL_DOWN );
                    pPlayer->SetArmed( TRUE );
                }
            }
            else
            {
                // No player.  Chill.
                EnterState( STATE_COOL_DOWN );
            }

            break;

        //----------------------------------------------------------------------
        case STATE_PRE_VEHICLE_REQUEST:

            // During the pre-vehicle state, the station is unfolding and the
            // player is locked.  Time to move on?
            
            if( m_Timer > LockTime[ m_Kind ] )
            {
                if( pPlayer )
                {
                    pPlayer->Player_SetupState( player_object::PLAYER_STATE_IDLE );
                    EnterState( STATE_VEHICLE_REQUEST ); 
                }
                else
                {
                    EnterState( STATE_COOL_DOWN ); 
                }
            }

            break;

        //----------------------------------------------------------------------
        case STATE_VEHICLE_REQUEST:

            // A player stepped on the pad and it has had time to unfold.  See
            // if the player is still there.  If so, attempt to create the 
            // vehicle.

            if( pPlayer )
            {
                if( m_Timer > 2.0f )
                {
                    vehicle_pad*            pVPad;
                    vehicle_pad::vpad_code  Code;
                    object::type            Type;

                    pVPad = (vehicle_pad*)ObjMgr.GetObjectFromID( m_VPadID );
                    Type  = pPlayer->GetInventoryLoadout().VehicleType;
                    Code  = pVPad->CreateVehicle( Type, m_PlayerID );

                    switch( Code )
                    {
                    case vehicle_pad::VPAD_OK:
                        pGameLogic->StationUsed( m_PlayerID );
                        EnterState( STATE_POST_VEHICLE_REQUEST );
                        break;

                    case vehicle_pad::VPAD_BUSY:
                        //MsgMgr.Message( MSG_VEHICLE_BLOCKED, m_PlayerID.Slot );
                        break;

                    case vehicle_pad::VPAD_BLOCKED:
                        MsgMgr.Message( MSG_VEHICLE_BLOCKED, m_PlayerID.Slot );
                        break;

                    case vehicle_pad::VPAD_VEHICLE_NOT_AVAILABLE:
                        MsgMgr.Message( MSG_VEHICLE_LIMIT, m_PlayerID.Slot );
                        break;

                    default:
                        ASSERT( FALSE );
                    }

                    m_Timer = 0.0f;
                }
                else
                {
                    // See if player walked away.
                    if( !m_ReleaseBBox.Intersect( pPlayer->GetPosition() ) )
                    {
                        EnterState( STATE_COOL_DOWN );
                        pPlayer->SetArmed( TRUE );
                    }
                }
            }
            else
            {
                // No player.  Oh, well.
                EnterState( STATE_COOL_DOWN ); 
            }

            break;

        //----------------------------------------------------------------------
        case STATE_POST_VEHICLE_REQUEST:

            // A player has started a vehicle creation, but is still standing on
            // the vehicle station.  When he leaves, re-arm him and enter the
            // cool down state.

            if( pPlayer )
            {
                // Did player leave the release box?
                if( !m_ReleaseBBox.Intersect( pPlayer->GetPosition() ) )
                {
                    EnterState( STATE_COOL_DOWN );
                    pPlayer->SetArmed( TRUE );
                }
            }
            else
            {
                // No player.  Chill.
                EnterState( STATE_COOL_DOWN );
            }

            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:  
            if( m_Timer > 2.0f )
                EnterState( STATE_IDLE );
            break;

        //----------------------------------------------------------------------
        case STATE_COOL_DOWN:            
            if( m_Timer > CoolDownTime[ m_Kind ] )
            {
                EnterState( STATE_IDLE );
            }
            break;
    }

    return( NO_UPDATE );
}

//==============================================================================

object::update_code station::ClientLogic( f32 DeltaTime )
{    
    asset::OnAdvanceLogic( DeltaTime );

    if( m_Disabled )
        return( NO_UPDATE );

    // State setup on client.
    if( m_State != m_PreChangeState )
    {
        EnterState( m_State );
        m_PreChangeState = m_State;
    }

    // State behavior on client.
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_EQUIP:
            // A player is getting new armour and equipment right now.
            // See if that process is complete.
            if( m_Timer > LockTime[ m_Kind ] )
                EnterState( STATE_PLAYER_REPAIR ); 
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_REPAIR:
            break;

        //----------------------------------------------------------------------
        case STATE_PRE_VEHICLE_REQUEST:
            if( m_Timer > LockTime[ m_Kind ] )
                EnterState( STATE_VEHICLE_REQUEST ); 
            break;

        //----------------------------------------------------------------------
        case STATE_VEHICLE_REQUEST:
            break;

        //----------------------------------------------------------------------
        case STATE_POST_VEHICLE_REQUEST:
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            if( m_Timer > 2.0f )
                EnterState( STATE_IDLE );
            break;

        //----------------------------------------------------------------------
        case STATE_COOL_DOWN: 
            // Playing an animation to reset the station.  Done?
            if( m_Timer > CoolDownTime[ m_Kind ] )
                EnterState( STATE_IDLE );
            break;
    }

    return( NO_UPDATE );
}


//==============================================================================

void station::EnterState( state State )
{
    m_Timer = 0.0f;

    if( State == m_State )
        return;
  
    m_State = State;

    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_IDLE );
            m_DirtyBits |= 0x01;
            if( m_Handle != 0 )
            {
                audio_Stop( m_Handle );
                m_Handle = 0;
            }
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_EQUIP:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );
            m_DirtyBits |= 0x01;
            m_Handle = 0;
            if( m_Kind == FIXED )
                m_Handle = audio_Play( SFX_POWER_INVENTORYSTATION_USE_LOOP, 
                                       &m_WorldPos );
            else
                audio_Play( SFX_POWER_DEPLOYABLE_STATION_USE, &m_WorldPos );
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_REPAIR:
            break;

        //----------------------------------------------------------------------
        case STATE_PRE_VEHICLE_REQUEST:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );
            m_DirtyBits |= 0x01;
            audio_Play( SFX_POWER_VEHICLESTATION_USE, &m_WorldPos );
            {
                vehicle_pad* pVPad = (vehicle_pad*)ObjMgr.GetObjectFromID( m_VPadID );
                if( pVPad )
                    pVPad->Alert();
            }
            break;

        //----------------------------------------------------------------------
        case STATE_VEHICLE_REQUEST:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );
            // Attempts to create the vehicle will occur every 2 seconds.  
            // Force the first attempt to be immediate.
            m_Timer = 2.0f;
            break;

        //----------------------------------------------------------------------
        case STATE_POST_VEHICLE_REQUEST:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );
            m_DirtyBits |= 0x01;
            // Set the streaming sound in motion.
            audio_SetPitch( m_Handle, 1.0f );
            m_Handle = 0;
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_DEPLOY );
            m_DirtyBits |= 0x01;
            audio_Play( SFX_PACK_INVENTORY_DEPLOY, &m_WorldPos );
            break;

        //----------------------------------------------------------------------
        case STATE_COOL_DOWN:
            m_Shape.SetAnimByType( ANIM_TYPE_INVENT_DEACTIVATE );
            m_DirtyBits |= 0x01;
            if( m_Kind == FIXED )
            {
                audio_Stop( m_Handle );
                m_Handle = 0;
                audio_Play( SFX_POWER_INVENTORYSTATION_USE_OFF, &m_WorldPos );
            }
            if( m_Kind == VEHICLE )
            {
                audio_Play( SFX_POWER_VEHICLESTATION_OFF, &m_WorldPos );
            }
            break;
    }
}

//==============================================================================

void station::Render( void )
{
    if( (m_Kind == DEPLOYED) && (m_Health == 0.0f) )
        return;

    // Visible?
    if( !RenderPrep() )
        return;

    // Set the animation.
    switch( m_State )
    {
        //----------------------------------------------------------------------
        case STATE_IDLE:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 0.0f );
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_EQUIP:
            m_Shape.GetCurrentAnimState().SetFrameParametric( m_Activate * m_Timer / LockTime[ m_Kind ] );
            break;

        //----------------------------------------------------------------------
        case STATE_PLAYER_REPAIR:
            break;

        //----------------------------------------------------------------------
        case STATE_PRE_VEHICLE_REQUEST:
            m_Shape.GetCurrentAnimState().SetFrameParametric( m_Activate * m_Timer / LockTime[ m_Kind ] );
            break;

        //----------------------------------------------------------------------
        case STATE_VEHICLE_REQUEST:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 1.0f );
            break;

        //----------------------------------------------------------------------
        case STATE_POST_VEHICLE_REQUEST:
            m_Shape.GetCurrentAnimState().SetFrameParametric( 1.0f );
            break;

        //----------------------------------------------------------------------
        case STATE_DEPLOYING:
            break;

        //----------------------------------------------------------------------
        case STATE_COOL_DOWN:
            m_Shape.GetCurrentAnimState().SetFrameParametric( m_Activate * m_Timer / CoolDownTime[ m_Kind ] );
            break;

        //----------------------------------------------------------------------
        default:
            ASSERT( FALSE );
            break;
    }

    // Turn off the damage texture on the glowy swooshes
    m_Shape.Draw( m_ShapeDrawFlags | shape::DRAW_FLAG_TURN_OFF_SELF_ILLUM_MAT_DETAIL, m_DmgTexture, m_DmgClutHandle );

    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

void station::DebugRender( void )
{   
    draw_BBox( m_TriggerBBox, XCOLOR_GREEN  );
    draw_BBox( m_ReleaseBBox, XCOLOR_YELLOW );
    draw_BBox( m_WorldBBox,   XCOLOR_RED    );

    if( m_Kind != DEPLOYED )
        draw_Point( m_SnapPosition, XCOLOR_BLUE );

    switch( m_Kind )
    {
    case DEPLOYED:
        {
            vector3 Point( GetRotation() );
            Point *= 1.25f;
            draw_Point( m_WorldPos - Point, XCOLOR_YELLOW );
            break;
        }
    case FIXED:
    case VEHICLE:
        {
            vector3 Point( GetRotation() );
            Point *= 3.00f;
            draw_Point( m_WorldPos - Point, XCOLOR_YELLOW );
            break;
        }
    }

    asset::DebugRender();
}

//==============================================================================
// Initialize to be used when loading Fixed Inventory Stations or Vehicle 
// Stations (and the attached Vehicle Pad) from Mission.txt.

void station::Initialize( const vector3& WorldPos,
                          const radian3& WorldRot,
                                s32      Power,
                                s32      Switch,
                          const xwchar*  pLabel )
{
    vector3 Pos = WorldPos;
    
    // "Drop" the station on the surface it is against.
    {
        collider::collision Collision;
        collider            Collider;
        f32                 Delta = 0.5f;

        if( m_pTypeInfo->Type == TYPE_STATION_VEHICLE )
            Delta = 4.0f;    

        // Get local Y-axis of station.
        matrix4 M = matrix4( WorldRot );
        vector3 X, Y, Z;
    
        M.GetColumns( X, Y, Z );
    
        Y *= Delta;
    
        vector3 S = WorldPos + Y;
        vector3 E = WorldPos - Y;
    
        Collider.RaySetup( NULL, S, E );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Collider );
    
        if( Collider.GetCollision( Collision ) )
        {
            Pos = Collision.Point;   
        }
        else
        {
            ASSERT( FALSE );
        }
    }

    // Vehicle stations require some extra effort.
    // (Can't use m_Kind, 'cause its not set until CommonInit.)
    if( m_pTypeInfo->Type == TYPE_STATION_VEHICLE )
    {
        xwstring Label( "Vehicle Pad" );
        vehicle_pad* pVPad = (vehicle_pad*)ObjMgr.CreateObject( object::TYPE_VPAD );
        pVPad->Initialize( Pos, WorldRot, Switch, (const xwchar*)Label );
        ObjMgr.AddObject( pVPad, obj_mgr::AVAILABLE_SERVER_SLOT );

        m_VPadID = pVPad->GetObjectID();

        vector3 Offset( 0.0f, 0.0f, 20.0f );
        Offset.Rotate( WorldRot );
        Pos += Offset;
    }

    asset::Initialize( Pos, WorldRot, Switch, Power, pLabel );

    CommonInit();

    // Prepare for ambient audio.
    m_AmbientID = SFX_POWER_INVENTORYSTATION_HUMM_LOOP;

    EnterState( STATE_IDLE );
}

//==============================================================================
// Initialize to be used when a Deployable Inventory Station is created on the
// server side.

void station::Initialize( const vector3&   Position,                                   
                                radian     Yaw,
                                object::id PlayerID,
                                u32        TeamBits )
{
    radian3 Rot( R_0, Yaw, R_0 );

    asset::Initialize( Position, Rot, -1, 0, StringMgr( "ui", IDS_REMOTE_INVENTORY ) );

    m_TeamBits  = TeamBits;
    m_PlayerID  = PlayerID;
    m_AttrBits &= ~ATTR_PERMANENT;

    CommonInit();

    // Prepare for ambient audio.
    m_AmbientID = 0;

    EnterState( STATE_DEPLOYING );
}

//==============================================================================

void station::MPBInitialize( const matrix4 L2W,
                                   u32     TeamBits )
{
    vector3 Pos = L2W.GetTranslation();
    radian3 Rot = L2W.GetRotation();
    
    asset::Initialize( Pos, Rot, -1, 0, NULL );

    m_TeamBits = TeamBits;

    CommonInit();
    m_RechargeRate = 10.0f;

    // Prepare for ambient audio.
    m_AmbientID = SFX_POWER_INVENTORYSTATION_HUMM_LOOP;

    EnterState( STATE_IDLE );
}

//==============================================================================

void station::OnAcceptInit( const bitstream& BitStream )
{
    s32 State;

    asset::OnAcceptInit( BitStream );
    BitStream.ReadS32( State, 4 );  

    CommonInit();

    x_wstrcpy( m_Label, StringMgr( "ui", IDS_REMOTE_INVENTORY ) );

    // Prepare for ambient audio.
    m_AmbientID = SFX_POWER_INVENTORYSTATION_HUMM_LOOP;

    m_State = STATE_UNDEFINED;
    EnterState( (state)State );
}

//==============================================================================

void station::CommonInit( void )
{
    m_Kind = (kind)(m_pTypeInfo->Type - TYPE_STATION_FIXED);

    m_PlayerID       = obj_mgr::NullID;
    m_PreChangeState = STATE_UNDEFINED;
    m_Timer          = 0.0f;

    if( m_Kind == DEPLOYED )
        m_AttrBits &= ~object::ATTR_PERMANENT;

    SetupTriggers();

    // Setup shape instance
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( ShapeIndex[ m_Kind ] ) );
    ASSERT( m_Shape.GetShape() );
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );

    // Need to know what portion of the activate occurs before the "loop to" point.
    {
        m_Shape.SetAnimByType( ANIM_TYPE_INVENT_ACTIVATE );
        anim* pAnim = m_Shape.GetCurrentAnimState().GetAnim();
        ASSERT( pAnim );
        m_Activate = (f32)pAnim->GetLoopToFrame() / (f32)(pAnim->GetFrameCount()-1);
        m_Shape.SetAnimByType( ANIM_TYPE_INVENT_IDLE );
    }

    // Setup collision instance.
    m_Collision.SetShape( tgl.GameShapeManager.GetShapeByType( CollisionIndex[ m_Kind ] ) );
    ASSERT( m_Collision.GetShape() );
    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    // Set final world bbox.
    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;

    ComputeZoneSet( m_ZoneSet, m_WorldBBox, TRUE );

    // Recharge rate.
    {
        f32 RechargeRate[3] = { 0.05f, 0.05f, 0.1f };
        m_RechargeRate = RechargeRate[m_Kind];
    }

    // Set up the bubble effects values.
    switch( m_Kind )
    {
    case FIXED:
    case VEHICLE:   
        m_BubbleOffset( 0, 1, 0 );
        m_BubbleScale ( 6, 6, 6 );
        break;

    case DEPLOYED:
        m_BubbleOffset( 0.0f, 1.0f, 0.0f );
        m_BubbleScale ( 2.5f, 2.5f, 2.5f );
        break;
    }
}

//==============================================================================

void station::SetupTriggers( void )
{
    // Prepare offset.
    vector3 Offset( 0, 0, 0 );
    if( m_Kind == DEPLOYED )
        Offset.Z = -1.25f;
    Offset.RotateY( m_WorldRot.Yaw );

    // Compute trigger bbox
    m_TriggerBBox.Min.X = m_WorldPos.X - TriggerRadius[ m_Kind ];
    m_TriggerBBox.Min.Y = m_WorldPos.Y - TriggerLo    [ m_Kind ];
    m_TriggerBBox.Min.Z = m_WorldPos.Z - TriggerRadius[ m_Kind ];
    m_TriggerBBox.Max.X = m_WorldPos.X + TriggerRadius[ m_Kind ];
    m_TriggerBBox.Max.Y = m_WorldPos.Y + TriggerHi    [ m_Kind ];
    m_TriggerBBox.Max.Z = m_WorldPos.Z + TriggerRadius[ m_Kind ];
    m_TriggerBBox.Translate( Offset );

    // Compute release bbox
    m_ReleaseBBox.Min.X = m_WorldPos.X - ReleaseRadius[ m_Kind ];
    m_ReleaseBBox.Min.Y = m_WorldPos.Y - ReleaseLo    [ m_Kind ];
    m_ReleaseBBox.Min.Z = m_WorldPos.Z - ReleaseRadius[ m_Kind ];
    m_ReleaseBBox.Max.X = m_WorldPos.X + ReleaseRadius[ m_Kind ];
    m_ReleaseBBox.Max.Y = m_WorldPos.Y + ReleaseHi    [ m_Kind ];
    m_ReleaseBBox.Max.Z = m_WorldPos.Z + ReleaseRadius[ m_Kind ];
    m_ReleaseBBox.Translate( Offset );

    // Compute snap position.
    if( m_Kind != DEPLOYED )
    {
        m_SnapPosition = m_WorldPos;
        m_SnapPosition.Y += 0.2f;
    }
}

//==============================================================================

void station::OnProvideInit( bitstream& BitStream )
{
    asset::OnProvideInit( BitStream );
    BitStream.WriteS32  ( m_State, 4 );
}

//==============================================================================

void station::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    // Always use collision shape for fixed inventory station to avoid
    // any collision with the invisible swoosh effects
    if (m_Kind == FIXED)
    {
        ASSERT(m_Collision.GetShape()) ;
        m_Collision.ApplyCollision( Collider );
        return ;
    }

    // Call base class
    asset::OnCollide(AttrBits, Collider) ;
}

//==============================================================================

object::update_code station::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    s32 Int;

    asset::OnAcceptUpdate( BitStream, SecInPast );

    if( BitStream.ReadFlag() )
    {
        m_PreChangeState = m_State;
        BitStream.ReadS32( Int, 5 );  
        EnterState( (state)Int );
    }

    return( ClientLogic( SecInPast ) );
}

//==============================================================================

void station::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( DirtyBits == 0xFFFFFFFF )
        DirtyBits =  0x80000001;

    asset::OnProvideUpdate( BitStream, DirtyBits, Priority );

    if( BitStream.WriteFlag( DirtyBits & 0x01 ) )
    {
        BitStream.WriteS32( m_State, 5 );
    }
    
    DirtyBits &= ~0x01;
}

//==============================================================================

s32 station::LookForPlayer( void )
{
    s32 ResultCode = -2;
    player_object* pPlayer;

    ObjMgr.Select( ATTR_PLAYER, m_TriggerBBox );
    while( (pPlayer = (player_object*)ObjMgr.GetNextSelected()) )
    {
        if( m_TriggerBBox.Intersect( pPlayer->GetPosition() ) )
        {
            // We have a player touching the trigger bbox.

            // See if the player is alive.
            if( pPlayer->IsDead() )
                continue;

            // If we are in "denied mode", then we have a player id already.
            // See if its the same player, and if so, ignore him.
            if( m_PlayerID == pPlayer->GetObjectID() )
                continue;

            // See if the teambits work.
            if( m_TeamBits & pPlayer->GetTeamBits() )
            {
                // So far so good.  Keep digging.
                if( m_PowerOn )
                {
                    if( m_Disabled )
                    {
                        // Disabled.  Denied!
                        ResultCode = MSG_STATION_DISABLED;
                    }
                    else
                    {
                        // Ah!  All is in harmony.
                        ResultCode = -1;
                    }
                }
                else
                {
                    // No power.  Denied!
                    ResultCode = MSG_STATION_NOT_POWERED;
                }
            }
            else
            {
                // Wrong team.  Denied!
                ResultCode = MSG_ACCESS_DENIED;
            }

            // Special case:  Bots are not allowed on vehicle stations.
            if( (m_Kind == VEHICLE) && (pPlayer->GetType() == object::TYPE_BOT) )
                ResultCode = -2;

            // We got what we came for.
            m_PlayerID = pPlayer->GetObjectID();
            break;
        }
    }
    ObjMgr.ClearSelection();

    return( ResultCode );
}

//==============================================================================

void station::Destroyed( object::id OriginID )
{
    pain::type PainType[] = 
    { 
        pain::EXPLOSION_STATION_INVEN,   
        pain::EXPLOSION_STATION_VEHICLE, 
        pain::EXPLOSION_STATION_DEPLOYED,
    };

    if( m_Explode )
    {
        SpawnExplosion( PainType[m_Kind],
                        m_WorldBBox.GetCenter(),
                        vector3(0,1,0),
                        OriginID );
    }
    asset::Destroyed( OriginID );

    m_Shape.SetAnimByType( ANIM_TYPE_INVENT_DESTROYED );
}

//==============================================================================

void station::Disabled( object::id OriginID )
{
    if( m_Kind == VEHICLE )
    {
        vehicle_pad* pVPad = (vehicle_pad*)ObjMgr.GetObjectFromID( m_VPadID );
        if( pVPad )
            pVPad->PowerOff();
    }

    asset::Disabled( OriginID );
}

//==============================================================================

void station::Enabled( object::id OriginID )
{
    if( m_Kind == VEHICLE )
    {
        vehicle_pad* pVPad = (vehicle_pad*)ObjMgr.GetObjectFromID( m_VPadID );
        if( pVPad )
            pVPad->PowerOn();
    }

    asset::Enabled( OriginID );
}

//==============================================================================

void station::MPBSetL2W( const matrix4& L2W )
{
    asset::MPBSetL2W( L2W );
    SetupTriggers();
}

//==============================================================================

object::id station::OccupiedBy( void )
{
    if ( m_State != STATE_IDLE )
    {
        return m_PlayerID;
    }
    else
    {
        return obj_mgr::NullID;
    }
}

//==============================================================================

vector3 station::GetPainPoint( void ) const
{
    vector3 Result = m_WorldPos;
    Result.Y += 0.1f;
    return( Result );
}

//==============================================================================

void station::DeployBeacon( void ) const
{
    // See if there is already a beacon nearby.
    ObjMgr.Select( ATTR_BEACON, m_WorldPos, 1.1f );
    if( ObjMgr.GetNextSelected() == NULL )
    {    
        vector3  A;
        vector3  B;
        vector3  C( 0.0f, 1.0f, x_frand( 0.5f, 1.0f ) );

        C.RotateY( x_frand( R_0, R_360 ) );
        A    = m_WorldPos + C;
        B    = A;
        B.Y -= 2.0f;
        
        collider Ray;
        Ray.RaySetup( NULL, A, B );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
        if( !Ray.HasCollided() )
        {
            // Rotate randomly, and try again.
            C.RotateY( x_frand( R_90, R_270 ) );
            A    = m_WorldPos + C;
            B    = A;
            B.Y -= 2.0f;
            Ray.RaySetup( NULL, A, B );
            ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
        }
        if( Ray.HasCollided() )
        {
            collider::collision Collision;
            Ray.GetCollision( Collision );
            beacon* pBeacon = (beacon*)ObjMgr.CreateObject( object::TYPE_BEACON );
            pBeacon->Initialize( Collision.Point, Collision.Plane.Normal, m_TeamBits );
            ObjMgr.AddObject( pBeacon, obj_mgr::AVAILABLE_SERVER_SLOT );
        }
    }
    ObjMgr.ClearSelection();
}

//==============================================================================
