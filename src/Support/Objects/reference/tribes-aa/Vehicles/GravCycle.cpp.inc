//==============================================================================
//
//  GravCycle.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "..\AADS\Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "GravCycle.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "audiomgr/audio.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "Data\UI\ui_strings.h"

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   GravCycleCreator;

obj_type_info   GravCycleClassInfo( object::TYPE_VEHICLE_WILDCAT, 
                                    "GravCycle", 
                                    GravCycleCreator, 
                                    object::ATTR_REPAIRABLE       | 
                                    object::ATTR_ELFABLE          |
                                    object::ATTR_SOLID_DYNAMIC |
                                    object::ATTR_VEHICLE |
                                    object::ATTR_DAMAGEABLE |
                                    object::ATTR_SENSED |
                                    object::ATTR_HOT |
                                    object::ATTR_LABELED );


//==============================================================================

static veh_physics s_GravCyclePhysics =
{
    TRUE,           // Is Ground vehicle

    175.0f,         // Maximum speed of vehicle
    125.0f,         // Linear Acceleration
    0.8f,           // Drag coefficent
    70.0f,          // Acceleration due to gravity
    
    R_1,            // Maximum pitch rotation from player
    R_40,           // Maximum roll rotation from player
    
    0.0f,           // Pitch acceleration
    R_250 *  2.0f,  // Yaw acceleration
    R_150 *  3.0f,  // Roll acceleration
    4.5f,           // Local rotational drag
    
    R_180 * 15.0f,  // World rotational acceleration
    4.5f ,          // World rotational drag
    
    500.0f,         // Mass
    0.30f,          // Perpendicular scale
    0.99f,          // Parallel scale
    0.5f,           // Damage scale


    1.0f /  7.0f,   // EnergyRecharge   
    1.0f /  7.5f,   // BoosterConsume
    1.0f,           // WeaponRecharge
    1.00f,          // WeaponConsume 
    1.0f,           // WeaponDelay
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

void GravCycle_InitFX( void )
{
}


object* GravCycleCreator( void )
{
    return( (object*)(new gravcycle) );
}


//==============================================================================
//  MEMBER FUNCTIONS
//==============================================================================
void gravcycle::OnAdd( void )
{
    gnd_effect::OnAdd();

    // Set the vehicle type
    m_VehicleType = TYPE_WILDCAT;
    m_ColBoxYOff  = 0.65f;

    // find the exhaust points
    // Loop through all model hot points
    model* Model = m_GeomInstance.GetModel() ;
    ASSERT(Model) ;

    // set the other 2 sounds to low volume for now
    //audio_SetVolume     ( m_AudioEng, 0.5f );

    // Set the label
    x_wstrcpy( m_Label, StringMgr( "ui", IDS_WILDCAT ) );

    m_BubbleScale( 3.5f, 3.5f, 5.0f );

    // set the audio target volume and ramp rate  MARC TWEAK RAMPRATE TO YOUR LIKING
    m_CurVol = 0.5f;
    m_TargetVol = 0.5f;
    m_RampRate = 0.2f;

}

//==============================================================================

void gravcycle::Initialize( const vector3& Pos, radian InitHdg, u32 TeamBits )
{
    gnd_effect::InitPhysics( &s_GravCyclePhysics );
    
    // call the base class initializers with info specific to gravcycle
    gnd_effect::Initialize( SHAPE_TYPE_VEHICLE_WILDCAT,         // Geometry shape
                            SHAPE_TYPE_VEHICLE_WILDCAT_COLL,    // Collision shape
                            Pos,                                // position
                            InitHdg,                            // heading
                            2.0f,                               // Seat radius
                            TeamBits );
}

//==============================================================================

void gravcycle::SetupSeat( s32 Index, seat& Seat )
{
    (void)Index;
    ASSERT( Index == 0 );
    ASSERT( Seat.Type == SEAT_TYPE_PILOT );

    // Setup character types that can get in seat
    Seat.PlayerTypes[player_object::ARMOR_TYPE_LIGHT]  = TRUE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_MEDIUM] = FALSE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_HEAVY]  = FALSE ;

    Seat.CanLookAndShoot = FALSE ;
    Seat.PlayerAnimType  = ANIM_TYPE_CHARACTER_PILOT_GRAV_CYCLE ;
}

//==============================================================================

void gravcycle::OnAcceptInit( const bitstream& BitStream )
{
    // Make valid object
    Initialize(vector3(0,0,0), 0, 0) ;

    // Now call base call accept init to override and setup vars
    gnd_effect::OnAcceptInit( BitStream );
}

//==============================================================================

object::update_code gravcycle::Impact( vector3& Pos, vector3& Norm )
{
    (void)Pos;
    (void)Norm;

    return UPDATE;
}

//==============================================================================

void gravcycle::Render( void )
{
    // Begin with base render...
    gnd_effect::Render() ;

//    if (m_nSeatsFree == 0)
//        x_printfxy(2, 10, "Health = %.2f", m_Health );

}

//==============================================================================

void gravcycle::RenderParticles( void )
{
}

//==============================================================================

object::update_code gravcycle::OnAdvanceLogic  ( f32 DeltaTime )
{
    matrix4 EP1;

    // set the axis for exhaust
    vector3 Axis;

    // call base class first, remember return param
    object::update_code RetVal = gnd_effect::OnAdvanceLogic( DeltaTime );

    EP1 = m_GeomInstance.GetHotPointL2W( HOT_POINT_TYPE_VEHICLE_EXHAUST2 );
    Axis.Set( 0.0f, 0.0f, -1.0f );
    EP1.SetTranslation( vector3(0.0f, 0.0f, 0.0f) );
    Axis = EP1 * Axis;

    Axis.RotateX( x_frand(-R_5, R_5) );
    Axis.RotateY( x_frand(-R_5, R_5) );

    return RetVal;
    
}

void gravcycle::OnRemove( void )
{
    gnd_effect::OnRemove();
}

void gravcycle::ApplyMove ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move )
{
    // Call base class to position the turret etc
    gnd_effect::ApplyMove( pPlayer, SeatIndex, Move );

    // play audio for jetting
    if ( ( Move.JetKey ) && ( m_AudioAfterBurn == -1 ) && ( m_Energy > 10.0f) )
        m_AudioAfterBurn =  audio_Play( SFX_VEHICLE_WILDCAT_AFTERBURNER_LOOP, &m_WorldPos, AUDFLAG_PERSISTENT );

}

//==============================================================================

f32 gravcycle::GetPainRadius( void ) const
{
    return( 2.0f );
}

//==============================================================================
