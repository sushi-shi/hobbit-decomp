//==============================================================================
//
//  Transport.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "..\AADS\Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "Transport.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "audiomgr/audio.hpp"
#include "Objects/Projectiles/Turret.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "Data\UI\ui_strings.h"

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   TransportCreator;

obj_type_info   TransportClassInfo( object::TYPE_VEHICLE_HAVOC, 
                                    "Transport", 
                                    TransportCreator, 
                                    object::ATTR_REPAIRABLE       | 
                                    object::ATTR_ELFABLE          |
                                    object::ATTR_SOLID_DYNAMIC |
                                    object::ATTR_VEHICLE |
                                    object::ATTR_DAMAGEABLE |
                                    object::ATTR_SENSED |
                                    object::ATTR_HOT |
                                    object::ATTR_LABELED );


//==============================================================================

static veh_physics s_TransportPhysics =
{
    FALSE,          // Is ground vehicle

    33.0f,          // Maximum speed of vehicle
    50.0f,          // Linear Acceleration
    1.5f,           // Drag coefficent
    5.0f,           // Acceleration due to gravity
    
    R_30,           // Maximum pitch rotation from player
    R_30,           // Maximum roll rotation from player
    
    R_180,          // Pitch acceleration
    R_180,          // Yaw acceleration
    R_180,          // Roll acceleration
    6.0f,           // Local rotational drag
    
    0.0f,           // World rotational acceleration
    0.0f,           // World rotational drag
    
    20000.0f,       // Mass
    0.15f,          // Perpendicular scale
    0.99f,          // Parallel scale
    0.1f,           // Damage scale

    1.0f / 10.0f,   // EnergyRecharge   
    1.0f /  6.0f,   // BoosterConsume
    1.0f,           // WeaponRecharge
    1.00f,          // WeaponConsume 
    1.0f,           // WeaponDelay
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

void Transport_InitFX( void )
{
}

object* TransportCreator( void )
{
    return( (object*)(new transport) );
}


//==============================================================================
//  MEMBER FUNCTIONS
//==============================================================================

void transport::OnAdd( void )
{
    gnd_effect::OnAdd();

    // Set the vehicle type
    m_VehicleType = TYPE_HAVOC;
    
    // Set the label
    x_wstrcpy( m_Label, StringMgr( "ui", IDS_HAVOC ) );

    m_BubbleScale( 15, 15, 18 );

    // set the audio target volume and ramp rate  MARC TWEAK RAMPRATE TO YOUR LIKING
    m_CurVol    = 0.5f;
    m_TargetVol = 0.5f;
    m_RampRate  = 0.05f;
}

//==============================================================================

void transport::Initialize( const vector3& Pos, radian InitHdg, u32 TeamBits )
{
    gnd_effect::InitPhysics( &s_TransportPhysics );
    
    // call the base class initializers with info specific to transport
    gnd_effect::Initialize( SHAPE_TYPE_VEHICLE_HAVOC,           // Geometry shape
                            SHAPE_TYPE_VEHICLE_HAVOC_COLL,      // Collision shape
                            Pos,                                // position
                            InitHdg,                            // heading
                            2.0f,                               // Seat radius
                            TeamBits );

    if( tgl.ServerPresent )
    {
        matrix4 L2W;
        turret* pTurret;

        L2W = m_GeomInstance.GetHotPointL2W( HOT_POINT_TYPE_TURRET_MOUNT );

        pTurret = (turret*)ObjMgr.CreateObject( object::TYPE_TURRET_FIXED );
        pTurret->MPBInitialize( L2W, m_TeamBits );
        ObjMgr.AddObject( pTurret, obj_mgr::AVAILABLE_SERVER_SLOT );
        m_TurretID = pTurret->GetObjectID();
    }
}

//==============================================================================

void transport::SetupSeat( s32 Index, seat& Seat )
{
    (void)Index ;

    switch(Seat.Type)
    {
        case SEAT_TYPE_PILOT:

            Seat.PlayerTypes[player_object::ARMOR_TYPE_LIGHT]  = TRUE ;
            Seat.PlayerTypes[player_object::ARMOR_TYPE_MEDIUM] = TRUE ;
            Seat.PlayerTypes[player_object::ARMOR_TYPE_HEAVY]  = FALSE ;
            Seat.CanLookAndShoot = FALSE ;
            Seat.PlayerAnimType  = ANIM_TYPE_CHARACTER_SITTING ;
            break ;

        default:

            Seat.PlayerTypes[player_object::ARMOR_TYPE_LIGHT]  = TRUE ;
            Seat.PlayerTypes[player_object::ARMOR_TYPE_MEDIUM] = TRUE ;
            Seat.PlayerTypes[player_object::ARMOR_TYPE_HEAVY]  = TRUE ;

            Seat.CanLookAndShoot = TRUE ;
            Seat.PlayerAnimType  = ANIM_TYPE_CHARACTER_IDLE ;
            break ;
    }
}

//==============================================================================

void transport::OnAcceptInit( const bitstream& BitStream )
{
    // Make valid object
    Initialize(vector3(0,0,0), 0, 0) ;

    BitStream.ReadObjectID( m_TurretID );

    // Now call base call accept init to override and setup vars
    gnd_effect::OnAcceptInit( BitStream );
}

//==============================================================================

void transport::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteObjectID( m_TurretID );
    gnd_effect::OnProvideInit( BitStream );
}

//==============================================================================

object::update_code transport::Impact( vector3& Pos, vector3& Norm )
{
    (void)Pos;
    (void)Norm;

    return UPDATE;
}

//==============================================================================

void transport::Render( void )
{
    // Begin with base render...
    gnd_effect::Render() ;

//    if (m_nSeatsFree == 0)
//        x_printfxy(2, 10, "Health = %.2f", m_Health );

    if( m_IsVisible )
    {
        if( m_TurretID != obj_mgr::NullID )
        {
            asset* pAsset;

            pAsset = (asset*)ObjMgr.GetObjectFromID( m_TurretID );
            if( pAsset )
            {
                if( m_TurretID.Seq == -1 )
                    m_TurretID.Seq = pAsset->GetObjectID().Seq;
                pAsset->MPBSetColors( m_GeomInstance );
            }
        } 
    }
}

//==============================================================================

void transport::RenderParticles( void )
{
}

//==============================================================================

object::update_code transport::OnAdvanceLogic  ( f32 DeltaTime )
{
    matrix4 EP1;

    // set the axis for exhaust
    vector3 Axis;

    // call base class first, remember return param
    object::update_code RetVal = gnd_effect::OnAdvanceLogic( DeltaTime );

    EP1 = m_GeomInstance.GetHotPointL2W( HOT_POINT_TYPE_VEHICLE_EXHAUST1 );
    Axis.Set( 0.0f, 0.0f, -1.0f );
    EP1.SetTranslation( vector3(0.0f, 0.0f, 0.0f) );
    Axis = EP1 * Axis;

    Axis.RotateX( x_frand(-R_5, R_5) );
    Axis.RotateY( x_frand(-R_5, R_5) );

    // update audio
    m_TargetVol = m_Control.ThrustY * 0.20f + 0.80f;
    f32 RampDelta = m_RampRate * DeltaTime;

    return RetVal;
}

//==============================================================================

void transport::UpdateInstances( void )
{
    // Call base class
    gnd_effect::UpdateInstances() ;

    // Update the assets' position.  
    if( m_TurretID != obj_mgr::NullID )
    {
        asset* pAsset;

        pAsset = (asset*)ObjMgr.GetObjectFromID( m_TurretID );
        if( pAsset )
        {
            if( m_TurretID.Seq == -1 )
                m_TurretID.Seq = pAsset->GetObjectID().Seq;

            pAsset->MPBSetL2W( m_GeomInstance.GetHotPointL2W( HOT_POINT_TYPE_TURRET_MOUNT ) );
        }
    }
}

//==============================================================================

void transport::OnRemove( void )
{
    ObjMgr.DestroyObject( m_TurretID ); 
    gnd_effect::OnRemove();
}

//==============================================================================

void transport::ApplyMove ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move )
{
    // Call base class to position the turret etc
    gnd_effect::ApplyMove( pPlayer, SeatIndex, Move );

    // play audio for jetting
    if ( ( Move.JetKey ) && ( m_AudioAfterBurn == -1 ) && ( m_Energy > 10.0f) )
        m_AudioAfterBurn =  audio_Play( SFX_VEHICLE_HAVOC_AFTERBURNER_LOOP, &m_WorldPos, AUDFLAG_PERSISTENT );

}

//==============================================================================

f32 transport::GetPainRadius( void ) const
{
    return( 5.0f );
}

//==============================================================================

void transport::UpdateAssets( void )
{
    // Update the assets' position.  
    if( m_TurretID != obj_mgr::NullID )
    {
        // make sure hotpoints are up to date
        UpdateInstances();

        asset* pAsset;

        pAsset = (asset*)ObjMgr.GetObjectFromID( m_TurretID );
        if( pAsset )
        {
            if( m_TurretID.Seq == -1 )
                m_TurretID.Seq = pAsset->GetObjectID().Seq;

            pAsset->MPBSetL2W( m_GeomInstance.GetHotPointL2W( HOT_POINT_TYPE_TURRET_MOUNT ) );
        }
    }
}

//==============================================================================

void transport::SetTeamBits( u32 TeamBits )
{
    m_TeamBits = TeamBits;

    object* pObject = ObjMgr.GetObjectFromID( m_TurretID );
    if( pObject )
        pObject->SetTeamBits( TeamBits );
}

//==============================================================================
