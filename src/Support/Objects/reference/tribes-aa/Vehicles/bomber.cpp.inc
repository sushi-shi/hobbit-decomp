//==============================================================================
//
//  Bomber.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "../AADS/Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib/BitStream.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "Bomber.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Audiomgr/audio.hpp"
#include "Objects/Projectiles/bombershot.hpp"
#include "Objects/Projectiles/Bomb.hpp"
#include "Objects/Projectiles/Aimer.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "Data\UI\ui_strings.h"

//==============================================================================

static veh_physics s_BomberPhysics =
{
    FALSE,          // Is ground vehicle

    62.0f,          // Maximum speed of vehicle
    75.0f,          // Linear Acceleration
    1.2f,           // Drag coefficent
    7.5f,           // Acceleration due to gravity

    R_45,           // Maximum pitch rotation from player
    R_30,           // Maximum roll rotation from player
    
    R_230,          // Pitch acceleration
    R_230,          // Yaw acceleration
    R_230,          // Roll acceleration
    6.0f,           // Local rotational drag
    
    0.0f,           // World rotational acceleration
    0.0f,           // World rotational drag

    10000.0f,       // Mass
    0.15f,          // Perpendicular scale
    0.99f,          // Parallel scale
    0.3f,           // Damage scale

    1.0f / 16.0f,   // EnergyRecharge   
    1.0f /  6.0f,   // BoosterConsume
    1.0f /  8.0f,   // WeaponRecharge  ????????????????????????????
    0.35f,          // WeaponConsume 
    0.75f,          // WeaponDelay
};

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BomberCreator;

obj_type_info   BomberClassInfo( object::TYPE_VEHICLE_THUNDERSWORD, 
                                    "Bomber", 
                                    BomberCreator, 
                                    object::ATTR_REPAIRABLE       | 
                                    object::ATTR_ELFABLE          |
                                    object::ATTR_SOLID_DYNAMIC |
                                    object::ATTR_VEHICLE |
                                    object::ATTR_DAMAGEABLE |
                                    object::ATTR_SENSED |
                                    object::ATTR_HOT |
                                    object::ATTR_LABELED );

static vector3 s_BomberBombingViewOffset                (0.0f, -1.5f, -1.4f) ;

static vector3 s_BomberTurretBaseOffset                 (0.0f, -0.75f, -1.4f) ;

static vector3 s_BomberTurret1stPersonViewPivotOffset   (0.0f, 0.0f,  0.0f) ;
static vector3 s_BomberTurret1stPersonViewPivotDist     (0.0f, -0.08f, -0.1f) ;

static vector3 s_BomberTurret3rdPersonViewPivotOffset   (0.0f, 0.0f,  0.0f) ;
static vector3 s_BomberTurret3rdPersonViewPivotDist     (0.0f, -0.2f, -8.0f) ;

static vector3 BomberPilotEyeOffset_3rd( 0, 9, -18 );
static radian3 BomberPilotEyeAngle_3rd( 0.5f, 0, 0);

//==============================================================================
//  FUNCTIONS
//==============================================================================

void Bomber_InitFX( void )
{
}


object* BomberCreator( void )
{
    return( (object*)(new bomber) );
}


//==============================================================================
//  MEMBER FUNCTIONS
//==============================================================================

void bomber::OnAdd( void )
{
    gnd_effect::OnAdd();

    // Set the vehicle type
    m_VehicleType = TYPE_THUNDERSWORD;
    m_ColBoxYOff  = 1.5f;

    // Set the label
    x_wstrcpy( m_Label, StringMgr( "ui", IDS_THUNDERSWORD ) );

    m_BubbleScale( 10, 10, 15 );

    // set the audio target volume and ramp rate  MARC TWEAK RAMPRATE TO YOUR LIKING
    m_CurVol = 0.5f;
    m_TargetVol = 0.5f;
    m_RampRate = 0.2f;
}

//==============================================================================

void bomber::Initialize( const vector3& Pos, radian InitHdg, u32 TeamBits )
{
    gnd_effect::InitPhysics( &s_BomberPhysics );

    // call the base class initializers with info specific to bomber
    gnd_effect::Initialize( SHAPE_TYPE_VEHICLE_THUNDERSWORD,        // Geometry shape
                            SHAPE_TYPE_VEHICLE_THUNDERSWORD_COLL,   // Collision shape
                            Pos,                                    // position
                            InitHdg,                                // heading
                            2.0f, TeamBits );                          // Seat radius
    m_bHasTurret        = FALSE ;
}

//==============================================================================

void bomber::SetupSeat( s32 Index, seat& Seat )
{
    (void)Index;
    ASSERT( Index == 0 );
    ASSERT( Seat.Type == SEAT_TYPE_PILOT );

    Seat.PlayerTypes[player_object::ARMOR_TYPE_LIGHT]  = TRUE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_MEDIUM] = TRUE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_HEAVY]  = FALSE ;

    Seat.CanLookAndShoot = FALSE ;
    Seat.PlayerAnimType  = ANIM_TYPE_CHARACTER_SITTING ;
}

//==============================================================================

void bomber::OnAcceptInit( const bitstream& BitStream )
{
    // Make valid object
    Initialize(vector3(0,0,0), 0, 0 );

    // Now call base call accept init to override and setup vars
    gnd_effect::OnAcceptInit( BitStream );
}

//==============================================================================

object::update_code bomber::Impact( vector3& Pos, vector3& Norm )
{
    (void)Pos;
    (void)Norm;

    return UPDATE;
}

//==============================================================================

void bomber::Render( void )
{
}

//==============================================================================

void bomber::RenderParticles( void )
{
}

//==============================================================================

object::update_code bomber::OnAdvanceLogic  ( f32 DeltaTime )
{
    matrix4 EP1;

    // set the axis for exhaust
    vector3 Axis;

    // call base class first, remember return param
    object::update_code RetVal = gnd_effect::OnAdvanceLogic( DeltaTime );

    return RetVal;
}

//==============================================================================

void bomber::OnRemove( void )
{
    gnd_effect::OnRemove();
}       

//==============================================================================

void bomber::Get3rdPersonView( s32 SeatIndex, vector3& Pos, radian3& Rot )
{
    (void)SeatIndex;
//  vector3 PivotPos, PivotDist ;

    // Special case yaw
    radian3 FreeLookYaw = Rot ;

    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Different seats have different views
    ASSERT( m_GeomInstance.GetHotPointByType(HOT_POINT_TYPE_VEHICLE_EYE_POS) ) ;
    Pos = BomberPilotEyeOffset_3rd;
    Pos.Rotate( m_Rot );
    Pos.Rotate( FreeLookYaw );
    Pos += m_GeomInstance.GetHotPointWorldPos(HOT_POINT_TYPE_VEHICLE_EYE_POS);
    Rot += BomberPilotEyeAngle_3rd + m_Rot ;
}

//==============================================================================

vector3 bomber::GetBlendFirePos( s32 Index )
{
    (void)Index;
    ASSERT( Index == 0 );
    vector3 Pos;
    GetBombPoint( Pos );
    return( Pos );
}

//==============================================================================

void bomber::ApplyMove ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move )
{
    // Call base class to position the turret etc
    gnd_effect::ApplyMove( pPlayer, SeatIndex, Move );

    // play audio for jetting
    if ( ( Move.JetKey ) && ( m_AudioAfterBurn == -1 ) && ( m_Energy > 10.0f) )
        m_AudioAfterBurn =  audio_Play( SFX_VEHICLE_THUNDERSWORD_AFTERBURNER_LOOP, &m_WorldPos, AUDFLAG_PERSISTENT );

    // Update weapons?
    if ((pPlayer) && (tgl.ServerPresent))
    {
        // Update fire delay
        m_Fire1Delay -= Move.DeltaTime ;
        if (m_Fire1Delay < 0)
            m_Fire1Delay = 0 ;
    
        // Only create on the server
        if ( (Move.FireKey) && (m_Fire1Delay == 0) && ( m_WeaponEnergy > m_pPhysics->WeaponConsume ) )
        {
            // Throw bomb!
            bomb* pBomb = (bomb*)ObjMgr.CreateObject( object::TYPE_BOMB );
            pBomb->Initialize( GetBlendFirePos(0), m_ObjectID, m_ObjectID, pPlayer->GetTeamBits() );
            pBomb->SetScoreID( pPlayer->GetObjectID() );
            ObjMgr.AddObject( pBomb, obj_mgr::AVAILABLE_SERVER_SLOT );

            // Wait before firing again
            m_Fire1Delay = m_pPhysics->WeaponDelay;

            // decrement weapon energy
            AddWeaponEnergy( -m_pPhysics->WeaponConsume );

            // Tell all clients to do fire anim
            //m_DirtyBits |= VEHICLE_DIRTY_BIT_FIRE ;
        }
    }
}

//==============================================================================

void bomber::WriteDirtyBitsData  ( bitstream& BitStream, u32& DirtyBits, f32 Priority, xbool MoveUpdate )
{
    // Call base class
    gnd_effect::WriteDirtyBitsData(BitStream, DirtyBits, Priority, MoveUpdate) ;
}

//==============================================================================

xbool bomber::ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast )
{
    // Call base class
    return gnd_effect::ReadDirtyBitsData(BitStream, SecInPast) ;
}

//==============================================================================

f32 bomber::GetPainRadius( void ) const
{
    return( 5.0f );
}

//==============================================================================

void bomber::GetBombPoint( vector3& Pos )
{
    vector3 Offset = s_BomberBombingViewOffset ;
    Offset.Rotate(m_GeomInstance.GetRot()) ;
    Pos = Offset + GetDrawPos();
}

//==============================================================================

