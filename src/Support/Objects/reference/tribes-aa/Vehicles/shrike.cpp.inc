//==============================================================================
//
//  Shrike.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "..\AADS\Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "Shrike.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "audiomgr/audio.hpp"
#include "objects\projectiles\ShrikeShot.hpp"
#include "Objects/Projectiles/AutoAim.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "Data\UI\ui_strings.h"

//==============================================================================
//  DEFINES
//==============================================================================

#define     FIREDELAY           0.250f
#define     SHOT_ENERGY         0.125f

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   ShrikeCreator;

obj_type_info   ShrikeClassInfo( object::TYPE_VEHICLE_SHRIKE, 
                                    "Shrike", 
                                    ShrikeCreator, 
                                    object::ATTR_REPAIRABLE       | 
                                    object::ATTR_ELFABLE          |
                                    object::ATTR_SOLID_DYNAMIC |
                                    object::ATTR_VEHICLE |
                                    object::ATTR_DAMAGEABLE |
                                    object::ATTR_SENSED |
                                    object::ATTR_HOT |
                                    object::ATTR_LABELED );


//==============================================================================

static vector3 ShrikePilotEyeOffset_3rd( 0.0f, 3.0f, -15.0f );

static veh_physics s_ShrikePhysics = 
{
    FALSE,          // Is ground vehicle

    111.0f,         // Maximum speed of vehicle
    100.0f,         // Linear Acceleration
    0.9f,           // Drag coefficent
    7.5f,           // Acceleration due to gravity
    
    R_45,           // Maximum pitch rotation from player
    R_45,           // Maximum roll rotation from player
    
    R_230,          // Pitch acceleration
    R_230,          // Yaw acceleration
    R_340,          // Roll acceleration
    4.5f,           // Local rotational drag
    
    0.0f,           // World rotational acceleration
    0.0f,           // World rotational drag

    5000.0f,        // Mass
    0.25f,          // Perpendicular scale
    0.99f,          // Parallel scale
    0.4f ,          // Damage scale


    1.0f / 10.0f,   // EnergyRecharge   
    1.0f /  4.0f,   // BoosterConsume
    1.0f /  4.0f,   // WeaponRecharge ???????????????????????
    0.125f,         // WeaponConsume 
    0.25f,          // WeaponDelay
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

void Shrike_InitFX( void )
{
}   

object* ShrikeCreator( void )
{
    return( (object*)(new shrike) );
}


//==============================================================================
//  MEMBER FUNCTIONS
//==============================================================================
void shrike::OnAdd( void )
{
    // Call base class
    gnd_effect::OnAdd();

    // Set the vehicle type
    m_VehicleType = TYPE_SHRIKE;
    m_Fire1Delay  = m_pPhysics->WeaponDelay;

    // Set the label
    x_wstrcpy( m_Label, StringMgr( "ui", IDS_SHRIKE ) );

    m_BubbleScale( 8, 7, 10 );

    // set the audio target volume and ramp rate  MARC TWEAK RAMPRATE TO YOUR LIKING
    m_CurVol = 0.5f;
    m_TargetVol = 0.5f;
    m_RampRate = 0.2f;

}

//==============================================================================

void shrike::Initialize( const vector3& Pos, radian InitHdg, u32 TeamBits )
{
    gnd_effect::InitPhysics( &s_ShrikePhysics );
    
    // call the base class initializers with info specific to shrike
    gnd_effect::Initialize( SHAPE_TYPE_VEHICLE_SHRIKE,          // Geometry shape
                            SHAPE_TYPE_VEHICLE_SHRIKE_COLL,     // Collision shape
                            Pos,                                // position
                            InitHdg,                            // heading
                            2.0f,                               // Seat radius
                            TeamBits );
}

//==============================================================================

void shrike::SetupSeat( s32 Index, seat& Seat )
{
    (void)Index;
    ASSERT( Index == 0 );
    ASSERT( Seat.Type == SEAT_TYPE_PILOT );

    // Set types that can get in seat
    Seat.PlayerTypes[player_object::ARMOR_TYPE_LIGHT]  = TRUE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_MEDIUM] = TRUE ;
    Seat.PlayerTypes[player_object::ARMOR_TYPE_HEAVY]  = FALSE ;

    // All players are seated...
    Seat.CanLookAndShoot = FALSE ;
    Seat.PlayerAnimType  = ANIM_TYPE_CHARACTER_SITTING ;
}

//==============================================================================

void shrike::OnAcceptInit( const bitstream& BitStream )
{
    // Make valid object
    Initialize(vector3(0,0,0), 0, 0) ;

    // Now call base call accept init to override and setup vars
    gnd_effect::OnAcceptInit( BitStream );
}

//==============================================================================

object::update_code shrike::Impact( vector3& Pos, vector3& Norm )
{
    (void)Pos;
    (void)Norm;

    return UPDATE;
}

//==============================================================================
void shrike::Get3rdPersonView( s32 Seat, vector3 &Pos, radian3 &Rot )
{
    (void)Seat;

    // Special case yaw
    radian3 FreeLookYaw = Rot ;

    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    ASSERT( m_GeomInstance.GetHotPointByType(HOT_POINT_TYPE_VEHICLE_EYE_POS) ) ;
    Pos = ShrikePilotEyeOffset_3rd;
    Pos.Rotate( m_Rot );
    Pos.Rotate( FreeLookYaw );
    Pos += m_GeomInstance.GetHotPointWorldPos(HOT_POINT_TYPE_VEHICLE_EYE_POS);
    Rot += m_Rot ;
}

//==============================================================================

void shrike::Render( void )
{
    // Begin with base render...
    gnd_effect::Render() ;

//    if (m_nSeatsFree == 0)
//        x_printfxy(2, 10, "Health = %.2f", m_Health );

}

//==============================================================================

void shrike::RenderParticles( void )
{
}

//==============================================================================

object::update_code shrike::OnAdvanceLogic  ( f32 DeltaTime )
{
    matrix4 EP1;

    // set the axis for exhaust
    vector3 Axis;

    // call base class first, remember return param
    object::update_code RetVal = gnd_effect::OnAdvanceLogic( DeltaTime );

    return( RetVal );
}

//==============================================================================

void shrike::OnRemove( void )
{
    gnd_effect::OnRemove();
}

//==============================================================================

vector3 shrike::GetBlendFirePos( s32 Index )
{
    return( m_GeomInstance.GetHotPointByIndexWorldPos( Index ) );
}

//==============================================================================

static f32 ShrikeShotOffset = 0.5f;

void shrike::ApplyMove ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move )
{
    // Call default
    gnd_effect::ApplyMove( pPlayer, SeatIndex, Move );

    // Fire?
    if (pPlayer)
    {
        // Get seat info
        const seat& Seat = GetSeat( SeatIndex );

        // Only the pilot can fire
        if( Seat.Type == SEAT_TYPE_PILOT )
        {
            if( ( m_Fire1Delay <= 0.0f ) && ( m_WeaponEnergy > m_pPhysics->WeaponConsume ) )
            {
                if( Move.FireKey && tgl.ServerPresent )
                {
                    // This section is a combination of the original shrike  
                    // firing code and player_object::GetTweakedWeaponFireL2W.

                    collider    Collider;
                    vector3     ViewPos;
                    radian3     ViewRot;
                    f32         ViewFOV;
                    radian3     Orient;
                    matrix4     L2W;
                    vector3     ViewZ;
                    vector3     Start;
                    vector3     End;
                    vector3     Dir;
                    vector3     Offset[2];
                    object::id  TargetID;                      

                    pPlayer->GetView( ViewPos, ViewRot, ViewFOV );
                    ViewZ.Set( ViewRot.Pitch, ViewRot.Yaw );

                    Offset[0]( -ShrikeShotOffset, 0.0f, 0.0f );
                    Offset[1](  ShrikeShotOffset, 0.0f, 0.0f );
                    Offset[0].Rotate( ViewRot );
                    Offset[1].Rotate( ViewRot );

                    xbool GotPoint = FALSE;

                    GotPoint = GetAutoAimPoint( (const player_object*)pPlayer, Dir, End, TargetID );
    
                    // If we do not have an auto aim target (for whatever reason), then fire
                    // from the tip of the weapon to where the line of sight hits something 
                    // solid.
                    if( !GotPoint )
                    {
                        Offset[0]( -ShrikeShotOffset, 0.0f, 0.0f );
                        Offset[1](  ShrikeShotOffset, 0.0f, 0.0f );
                        Offset[0].Rotate( ViewRot );
                        Offset[1].Rotate( ViewRot );

                        End = ViewPos + (ViewZ * 1000.0f);
        
                        Collider.ClearExcludeList() ;
                        Collider.RaySetup( NULL, ViewPos, End, m_ObjectID.GetAsS32() );
                        Collider.Exclude( pPlayer->GetObjectID().GetAsS32() );
                        ObjMgr.Collide( ATTR_SOLID, Collider );
    
                        // Move the end point up if we hit something.
                        if( Collider.HasCollided() )
                        {
                            collider::collision Collision;
                            Collider.GetCollision( Collision );
                            End = Collision.Point;
                        }
                    }
                    else
                    {
                        Offset[0]( 0.0f, 0.0f, 0.0f );
                        Offset[1]( 0.0f, 0.0f, 0.0f );
                    }

                    for( int i = 0; i < 2; i++ )
                    {
                        Start = GetBlendFirePos( i );
                        Dir   = End + Offset[i] - Start;
                        Dir.GetPitchYaw( Orient.Pitch, Orient.Yaw );
                        Orient.Roll = R_0;

                        L2W.Zero();
                        L2W.SetRotation( Orient );
                        L2W.SetTranslation( Start );

		                shrike_shot* pShot = (shrike_shot*)ObjMgr.CreateObject( object::TYPE_SHRIKESHOT );

                        pShot->Initialize( L2W, m_TeamBits, i, m_ObjectID, m_ObjectID );
                        pShot->SetScoreID( pPlayer->GetObjectID() );
            
                        ObjMgr.AddObject( pShot, obj_mgr::AVAILABLE_SERVER_SLOT );
                    }

                    // reset fire delay
                    m_Fire1Delay = m_pPhysics->WeaponDelay;

                    // decrement weapon energy
                    AddWeaponEnergy( -m_pPhysics->WeaponConsume );
                }
            }

            // decrement our timer
            m_Fire1Delay -= Move.DeltaTime;
        }
    }
}

//==============================================================================

f32 shrike::GetPainRadius( void ) const
{
    return( 3.0f );
}

//==============================================================================
/*
const matrix4 player_object::GetTweakedWeaponFireL2W( void ) const
{
    vector3     ViewPos;
    radian3     ViewRot;
    f32         ViewFOV;
    radian3     Orient;
    matrix4     L2W;
    vector3     ViewZ;
    vector3     Start;
    vector3     End;

    GetView( ViewPos, ViewRot, ViewFOV );
    ViewZ.Set( ViewRot.Pitch, ViewRot.Yaw );        

    xbool GotPoint = FALSE;

    if( UseAutoAimHelp )
    {
        vector3     Dir;
        object::id  TargetID;
        GotPoint = GetAutoAimPoint( (const player_object*)this, Dir, End, TargetID );
    }

    // If we do not have an auto aim target (for whatever reason), then fire
    // from the tip of the weapon to where the line of sight hits something 
    // solid.
    if( !GotPoint )
    {
        End = ViewPos + (ViewZ * 1000.0f);

        s_Collider.ClearExcludeList() ;
        s_Collider.RaySetup  ( NULL, ViewPos, End, m_ObjectID.GetAsS32() );
        ObjMgr.Collide( ATTR_SOLID, s_Collider );

        // Move the end point up if we hit something.
        if( s_Collider.HasCollided() )
        {
            collider::collision Collision;
            s_Collider.GetCollision( Collision );
            End = Collision.Point;
        }
    }

    // Establish orientation based on where we are shooting.
    Start = m_WeaponFireL2W.GetTranslation();
    End  -= Start;
    End.GetPitchYaw( Orient.Pitch, Orient.Yaw );
    Orient.Roll = m_WeaponFireRot.Roll;

    L2W.Zero();
    L2W.Setup( vector3(1,1,1), Orient, Start );

    return( L2W );
}

*/