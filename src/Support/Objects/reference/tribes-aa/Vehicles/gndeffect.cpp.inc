//==============================================================================
//
//  gnd_effect.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "..\AADS\Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include <Support/ObjectMgr/Object.hpp>
#include "audiomgr/audio.hpp"
#include "../projectiles/ParticleObject.hpp"
#include "../projectiles/debris.hpp"
#include "GndEffect.hpp"
#include "LabelSets\Tribes2Types.hpp"
#include "Objects\Projectiles\Bubble.hpp"
#include "GameMgr\\GameMgr.hpp"
#include "fe_globals.hpp"

//==============================================================================
// PHYSICS DEFINES
//==============================================================================

f32    StickEpsilon      =  0.1f;       // Deadzone on stick
f32    MovingEpsilon     =  0.001f;     // Minimum moving distance
f32    BackOffAmount     =  0.001f;     // Amount to back off from collision plane
f32    MinFaceDist       =  0.01f;      // Minimum distance from face
f32    SpikeTestDist     =  0.25f;      // Distance below plane for spike test
f32    MaxStepHeight     =  0.1f;       // Maximum height we can step over
f32    BBoxRadiusScale   =  0.4f;       // Shrink the vehicle bounding box
f32    SettleSpeed       =  8.0f;       // Start settling vehicle under this speed
f32    SettleNormalY     =  0.75f;      // Amount of Up-ness required in a surface normal to settle vehicle
f32    SettleTime        =  0.50f;      // Duration of settling after which vehicle will be stationary
f32    ReverseThrustMul  =  0.25f;      // Reduce the thrust force when going in reverse
f32    SidewardThrustMul =  0.5f;       // Reduce the thrust force when going sideways
f32    SidewardRollMul   =  0.5f;       // Amount of roll to add to vehicle when moving sideways
f32    MinRollMul        =  0.5f;       // Clamp the minimum roll so we always get some
f32    EnergyMul         = 25.0f;       // Percentage of energy expended per second when boosting
f32    MinEnergyForBoost =  0.25f;      // Minimum energy required to boost
f32    MaxGroundHeight   = 10.0f;       // Distance to check for ground
f32    MinGroundHeight   =  2.0f;       // Distance under which vehicle has full ground effect
f32    TurboBoostMul     =  0.25f;      // Turbo boost multiplier
f32    HeavyImpact       =  0.5f;       // Percentage amount to trigger heavy  collision audio
f32    MediumImpact      =  0.3f;       // Percentage amount to trigger medium collision audio
f32    LightImpact       =  0.2f;       // Percentage amount to trigger light  collision audio
f32    MinRotMul         =  0.50f;      // Minimum amount of ground effect when applied to rotational velocity
f32    ZeroGVelocityT    =  0.25f;      // Parametric velocity after which vehicle experiences zero gravity
f32    MinZoomFactor     =  0.25f;      // Minimum zoom factor so we always get at least some movement
radian MaxVehicleAngle   =  R_70;       // Maximum amount of pitch or roll for vehicle attitude
radian ThrustLimitAngle  =  R_85;       // Thrust drops off on surfaces this steep
xbool  LimitVehicleAngle =  TRUE;       // Turn clamping on for the vehicles pitch and roll

//==============================================================================
// DEFINES
//==============================================================================

// Moving vars
#define MOVING_EPSILON                 0.001f       // Min moving dist
                                   
// State defines                    
#define SPAWN_TIME                     4.0f         // Spawn fade in time (secs)
                                
// Blending defines             
#define MAX_BLEND_DIST                20.0f         // Max dist before snapping to server pos
#define MAX_BLEND_TIME                 0.2f         // Max secs to blend to server pos
                                    
// Limits                           
#define VEL_MIN                     -120.0f
#define VEL_MAX                      120.0f
#define VEL_CAP_OFF                    0.1f
                                    
#define ROT_MIN                       -R_180
#define ROT_MAX                        R_180
                                    
#define ROT_VEL_MIN                   -R_180
#define ROT_VEL_MAX                    R_180
#define ROT_VEL_CAP_OFF                0.0005f

#define ROT_VEQ_MIN                   -1.0f
#define ROT_VEQ_MAX                    1.0f
                                    
#define BOOST_MIN                      0.0f
#define BOOST_MAX                      1.0f
                                    
#define THRUST_MIN                    -1.0f
#define THRUST_MAX                     1.0f
#define THRUST_CAP_OFF                 0.1f
                                    
#define ENERGY_MIN                     0.0f
#define ENERGY_MAX                   100.0f
                                    
#define WEAPON_MIN                     0.0f
#define WEAPON_MAX                     1.0f
                                    
#define HEALTH_MIN                     0.0f
#define HEALTH_MAX                   100.0f
                                    
#define STATE_TIME_MIN                 0.0f
#define STATE_TIME_MAX                 5.0f

// Bit sizes when sending to a controlled vehicle
#define INPUT_VEL_BITS              16
#define INPUT_ROT_BITS              12
#define INPUT_ROT_VEL_BITS          16
#define INPUT_ROT_VEQ_BITS          12
#define INPUT_BOOST_BITS             1
#define INPUT_THRUST_BITS            8
#define INPUT_ENERGY_BITS           18
#define INPUT_WEAPON_BITS            7
#define INPUT_HEALTH_BITS            7
#define INPUT_STATE_TIME_BITS        8

// Bit sizes when sending to a ghost vehicle
#define GHOST_VEL_BITS              16
#define GHOST_ROT_BITS               8
#define GHOST_ROT_VEL_BITS          12
#define GHOST_ROT_VEQ_BITS          12
#define GHOST_BOOST_BITS             1
#define GHOST_THRUST_BITS            5
#define GHOST_ENERGY_BITS            6
#define GHOST_WEAPON_BITS            6
#define GHOST_HEALTH_BITS            6
#define GHOST_STATE_TIME_BITS        4

//==============================================================================
// DATA
//==============================================================================

// For easy dirty bits setting
vector3             gnd_effect::s_LastWorldPos;
vector3             gnd_effect::s_LastVel;
radian3             gnd_effect::s_LastWorldRot;
quaternion          gnd_effect::s_LastWorldRotVel;
radian3             gnd_effect::s_LastLocalRot;
radian3             gnd_effect::s_LastLocalRotVel;
f32                 gnd_effect::s_LastBoost;
vector2             gnd_effect::s_LastThrust;
f32                 gnd_effect::s_LastEnergy;
f32                 gnd_effect::s_LastWeaponEnergy;
gnd_effect::state   gnd_effect::s_LastState;
f32                 gnd_effect::s_LastStateTime;
f32                 gnd_effect::s_LastHealth;

struct vehicle_debug
{
    xbool NoDamage;
    xbool ShowBitStream;
    xbool DrawBBox;
    xbool DrawAxis;
    xbool ShowImpacts;
    xbool ShowCollision;
};

vehicle_debug g_VehicleDebug =
{
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
};

static bbox                s_VehicleBBox;
static vector3             s_Vector;
static vector3             s_StepVel;
static vector3             s_StepMove;
static bbox                s_StepBBox;
static f32                 s_StepDeltaHeight;
static collider::collision s_StepCollision;
static collider::collision s_Collision;
static xbool               s_GotStepInfo = FALSE;

//==============================================================================
// FUNCTIONS
//==============================================================================

void gnd_effect::EjectPlayers( void )
{
    s32             i;
    player_object*  pPlayer;

    // Only update server players
    if( !tgl.ServerPresent )
        return;

    // Check all seats
    for( i = 0; i < m_nSeats; i++ )
    {
        // Is there a player in this seat?
        pPlayer = GetSeatPlayer( i );
        if( pPlayer )
        {
            // Player should still be in vehicle!
            ASSERT( pPlayer->Player_GetState() == player_object::PLAYER_STATE_VEHICLE );

            // Force player to jump out of vehicle
            pPlayer->Player_SetupState( player_object::PLAYER_STATE_JUMP );

            // The player will take care of updating the vehicle seat..
            // (See Player_Exit_VEHICLE in PlayerStates.cpp)
        }
    }
}

//==============================================================================

void gnd_effect::Explode( const pain& Pain )
{
    // Can only be called on server!
    ASSERT(tgl.ServerPresent) ;

    // Eject any remaining players
    EjectPlayers() ;

    // List of pain types by vehicle type.
    pain::type PainType[] =
    {
        (pain::type)-1,                 // undefined vehicle type
        pain::EXPLOSION_GRAV_CYCLE,
        pain::EXPLOSION_SHRIKE,
        pain::EXPLOSION_TANK,
        pain::EXPLOSION_BOMBER,
        pain::EXPLOSION_TRANSPORT,
        pain::EXPLOSION_MPB,
    };

    SpawnExplosion( PainType[ m_VehicleType ],
                    m_WorldPos, 
                    vector3(0,1,0), 
                    Pain.OriginID,
                    m_ObjectID );
}

//==============================================================================

object::update_code gnd_effect::AdvanceLogic( f32 DeltaTime )
{
    // Do movement
    ApplyPhysics( DeltaTime ) ;

    return UPDATE;
}

//==============================================================================

object::update_code gnd_effect::OnAdvanceLogic  ( f32 DeltaTime )
{
    if( g_VehicleDebug.ShowBitStream )
        DebugBitStream( DeltaTime );

    // Update average velocity
    ComputeAverageVelocity() ;
    
    // see if the vehicle should be destroyed
    CheckDestroyConditions( DeltaTime );

    // explode and destroy ship when appropriate
    if ( (tgl.pServer) && ( (m_Health == 0.0f) || (m_TrueTime > 15.0f) ) )
    {
        return DESTROY;
    }

    // Get controlling player (if any)
    player_object* pPlayer = IsPlayerControlled() ;

    // Update blend
    if (m_BlendMove.Blend != 0)
    {
        // Next blend
        m_BlendMove.Blend += m_BlendMove.BlendInc * DeltaTime ;
        if (m_BlendMove.Blend < 0)
            m_BlendMove.Blend = 0 ;
    }

    // Update state
    m_StateTime += DeltaTime ;
    if (m_StateTime > STATE_TIME_MAX)
        m_StateTime = STATE_TIME_MAX ;

    // Truncate ready for sending across the net
    Truncate() ;

    // Advance state
    switch(m_State)
    {
        case STATE_SPAWN:

            // Done?
            if (m_StateTime > SPAWN_TIME)
            {
                m_StateTime = SPAWN_TIME ;
                m_State     = STATE_MOVING ;
                m_DirtyBits |= VEHICLE_DIRTY_BIT_STATE ;

                // Teleport player into pilot seat
                player_object* pOwner = (player_object*)ObjMgr.GetObjectFromID( m_Owner );

                // Allow players to enter vehicle
                SetAllowEntry( TRUE );
                
                // Attempt to teleport the player into the seat.
                if( (pOwner) &&                
                    (pOwner->IsDead() == FALSE) &&
                    (pOwner->IsAttachedToVehicle() == FALSE) &&
                    (pOwner->GetTeamBits() & m_TeamBits) )
                {
                    // Get pilot seat
                    seat& Seat = m_Seats[0];
                    ASSERT( Seat.PlayerID == obj_mgr::NullID );
                    
                    // Move player to the seat
                    vector3 OldPos  = pOwner->GetPos();
                    vector3 SeatPos = GetSeatPos( 0 );
                    pOwner->SetPos( SeatPos );

                    // Attempt to connect player to vehicle
                    s32 SeatIndex = HitByPlayer( pOwner );
                    
                    // Check if we couldnt get into vehicle (eg. wrong armor class or too big a pack)
                    if( SeatIndex == -1 )
                    {
                        pOwner->SetPos( OldPos );
                    }
                }
            }

            // Fade instance in
            m_GeomInstance.SetColor(xcolor(255,255,255, (u8)((255.0f * m_StateTime) / SPAWN_TIME))) ;

            return NO_UPDATE ;

        default:
            ASSERT(0) ;

        case STATE_MOVING:

            // Advance animation
            m_GeomInstance.Advance(DeltaTime) ;

            // Make instance solid
            m_GeomInstance.SetColor(XCOLOR_WHITE) ;

            // Skip movement if controlled by a player and this is the server
            // (server will receive move from movemanager)
            if ( (pPlayer) && (tgl.pServer) )
                return NO_UPDATE ;

            // Skip movement if controlled by a player on this machine
            // (controlling player will call ApplyMove and ApplyPhysics)
            if ( (pPlayer) && ((pPlayer->GetProcessInput()) || (pPlayer->GetType() == object::TYPE_BOT)) )
                return NO_UPDATE ;

            // Apply a null move to clear input vars
            player_object::move NullMove ;
            NullMove.Clear() ;
            NullMove.DeltaTime = DeltaTime ;
            ApplyMove(NULL, 0, NullMove) ;    // Apply pilot move

            // Perform physics...
            return( AdvanceLogic( DeltaTime ) );
    }
}

//==============================================================================

void gnd_effect::BeginSetDirtyBits( void )
{
    s_LastWorldPos      = m_WorldPos;
    s_LastVel           = m_Vel;
    s_LastWorldRot      = m_WorldRot;
    s_LastWorldRotVel   = m_WorldRotVel;
    s_LastLocalRot      = m_LocalRot;
    s_LastLocalRotVel   = m_LocalRotVel;
    s_LastBoost         = m_Control.Boost;
    s_LastThrust        = vector2( m_Control.ThrustX, m_Control.ThrustY );
    s_LastEnergy        = m_Energy;
    s_LastWeaponEnergy  = m_WeaponEnergy;
    s_LastState         = m_State;
    s_LastStateTime     = m_StateTime;
    s_LastHealth        = m_Health;
}

//==============================================================================

void gnd_effect::EndSetDirtyBits( void )
{
    xbool Moved = FALSE;

    // Position + velocity
    if( (s_LastWorldPos != m_WorldPos) || (s_LastVel != m_Vel) )
    {
        m_DirtyBits |= VEHICLE_DIRTY_BIT_POS;
        Moved = TRUE;
    }

    // Rotation + rotational velocity
    if( IsGroundBased() )
    {
        xbool HasWorldRotVelChanged = (s_LastWorldRotVel.X != m_WorldRotVel.X) ||
                                      (s_LastWorldRotVel.Y != m_WorldRotVel.Y) || 
                                      (s_LastWorldRotVel.Z != m_WorldRotVel.Z) || 
                                      (s_LastWorldRotVel.W != m_WorldRotVel.W);
    
        // Note: to save bandwidth ground based vehicles dont have any local pitch rotations
        if( (s_LastWorldRot      != m_WorldRot)      || HasWorldRotVelChanged                          ||
            (s_LastLocalRot.Yaw  != m_LocalRot.Yaw)  || (s_LastLocalRotVel.Yaw  != m_LocalRotVel.Yaw)  ||
            (s_LastLocalRot.Roll != m_LocalRot.Roll) || (s_LastLocalRotVel.Roll != m_LocalRotVel.Roll) )
        {
            m_DirtyBits |= VEHICLE_DIRTY_BIT_ROT;
            Moved = TRUE;
        }
    }
    else
    {
        if( (s_LastLocalRot != m_LocalRot) || (s_LastLocalRotVel != m_LocalRotVel) )
        {
            m_DirtyBits |= VEHICLE_DIRTY_BIT_ROT;
            Moved = TRUE;
        }
    }

    // Boost
    if( s_LastBoost != m_Control.Boost )
        m_DirtyBits |= VEHICLE_DIRTY_BIT_BOOST;
    
    // Thrust + energy
    if( (s_LastThrust       != vector2( m_Control.ThrustX, m_Control.ThrustY )) ||
        (s_LastEnergy       != m_Energy)                                        ||
        (s_LastWeaponEnergy != m_WeaponEnergy) )
        m_DirtyBits |= VEHICLE_DIRTY_BIT_THRUST_ENERGY;
        
    // State
    if( (s_LastState != m_State) || (s_LastStateTime != m_StateTime) )
        m_DirtyBits |= VEHICLE_DIRTY_BIT_STATE;

    // Health
    if( s_LastHealth != m_Health )
        m_DirtyBits |= VEHICLE_DIRTY_BIT_HEALTH;

    // If the vehicle moved, tell all the connected players.
    // This fixes the following situation:
    // 1) passenger player's logic is advanced and sets up correct position on the vehicle
    // 2) pilot player's logic is advances, which moved the vehicle, and makes the passenger out of sync!
    if( Moved )
    {
        // Keep instances in sync
        UpdateInstances() ;

        // Check all seats except the pilot seat
        for( s32 i=1; i<m_nSeats; i++ )
        {
            // Is there a player in this seat?
            player_object* pPlayer = GetSeatPlayer( i );
            if ( pPlayer )
                pPlayer->VehicleMoved( this );   // Update the player position
        }
    }
}

//==============================================================================

void gnd_effect::Truncate( void )
{
    // Get ready for setting dirty bits
    BeginSetDirtyBits();

    // Truncate vel
    TruncateRangedF32( m_Vel.X,             INPUT_VEL_BITS,         VEL_MIN,        VEL_MAX );
    TruncateRangedF32( m_Vel.Y,             INPUT_VEL_BITS,         VEL_MIN,        VEL_MAX );
    TruncateRangedF32( m_Vel.Z,             INPUT_VEL_BITS,         VEL_MIN,        VEL_MAX );
                                                                                     
    // Truncate rot                                                 
    TruncateRangedF32( m_WorldRot.Pitch,    INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
    TruncateRangedF32( m_WorldRot.Yaw,      INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
    TruncateRangedF32( m_WorldRot.Roll,     INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
    TruncateRangedF32( m_LocalRot.Pitch,    INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
    TruncateRangedF32( m_LocalRot.Yaw,      INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
    TruncateRangedF32( m_LocalRot.Roll,     INPUT_ROT_BITS,         ROT_MIN,        ROT_MAX );
                                                                    
    // Truncate rotational velocity                                 
    TruncateRangedF32( m_WorldRotVel.X,     INPUT_ROT_VEQ_BITS,     ROT_VEQ_MIN,    ROT_VEQ_MAX );
    TruncateRangedF32( m_WorldRotVel.Y,     INPUT_ROT_VEQ_BITS,     ROT_VEQ_MIN,    ROT_VEQ_MAX );
    TruncateRangedF32( m_WorldRotVel.Z,     INPUT_ROT_VEQ_BITS,     ROT_VEQ_MIN,    ROT_VEQ_MAX );
    TruncateRangedF32( m_WorldRotVel.W,     INPUT_ROT_VEQ_BITS,     ROT_VEQ_MIN,    ROT_VEQ_MAX );
    TruncateRangedF32( m_LocalRotVel.Pitch, INPUT_ROT_VEL_BITS,     ROT_VEL_MIN,    ROT_VEL_MAX );
    TruncateRangedF32( m_LocalRotVel.Yaw,   INPUT_ROT_VEL_BITS,     ROT_VEL_MIN,    ROT_VEL_MAX );
    TruncateRangedF32( m_LocalRotVel.Roll,  INPUT_ROT_VEL_BITS,     ROT_VEL_MIN,    ROT_VEL_MAX );
                                                                    
    // Truncate boost                                               
    TruncateRangedF32( m_Control.Boost,     INPUT_BOOST_BITS,       BOOST_MIN,      BOOST_MAX );
                                                                    
    // Truncate thrust                                              
    TruncateRangedF32( m_Control.ThrustX,   INPUT_THRUST_BITS,      THRUST_MIN,     THRUST_MAX );
    TruncateRangedF32( m_Control.ThrustY,   INPUT_THRUST_BITS,      THRUST_MIN,     THRUST_MAX );
                                                                    
    // Truncate energy                                              
    TruncateRangedF32( m_Energy,            INPUT_ENERGY_BITS,      ENERGY_MIN,     ENERGY_MAX );
    TruncateRangedF32( m_WeaponEnergy,      INPUT_WEAPON_BITS,      WEAPON_MIN,     WEAPON_MAX );

    // Truncate state time
    TruncateRangedF32( m_StateTime,         INPUT_STATE_TIME_BITS,  STATE_TIME_MIN, STATE_TIME_MAX );

    // Set dirty bits
    EndSetDirtyBits();
}

//==============================================================================

void gnd_effect::WriteDirtyBitsData( bitstream& BitStream, u32& DirtyBits, f32 Priority, xbool MoveUpdate )
{
    (void)Priority;

    // Move update?
    if( BitStream.WriteFlag( MoveUpdate ) )
    {
        // Sending to a controlled vehicle...
        // (Unused bits are actually the player dirty bits, so leave them!)

        // Position
        DirtyBits &= ~VEHICLE_DIRTY_BIT_POS;
        BitStream.WriteVector( m_WorldPos );

        // Velocity
        WriteRangedF32( BitStream, m_Vel.X, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );
        WriteRangedF32( BitStream, m_Vel.Y, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );
        WriteRangedF32( BitStream, m_Vel.Z, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );

        // Rotation
        DirtyBits &= ~VEHICLE_DIRTY_BIT_ROT;
        if( IsGroundBased() )
        {
            WriteRangedF32( BitStream, m_WorldRot.Pitch,    INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_WorldRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_WorldRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_LocalRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_LocalRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            
            WriteRangedF32( BitStream, m_WorldRotVel.X,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            WriteRangedF32( BitStream, m_WorldRotVel.Y,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            WriteRangedF32( BitStream, m_WorldRotVel.Z,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            WriteRangedF32( BitStream, m_WorldRotVel.W,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            WriteRangedF32( BitStream, m_LocalRotVel.Yaw,   INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            WriteRangedF32( BitStream, m_LocalRotVel.Roll,  INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
        }
        else
        {
            WriteRangedF32( BitStream, m_LocalRot.Pitch,    INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_LocalRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_LocalRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            WriteRangedF32( BitStream, m_LocalRotVel.Pitch, INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            WriteRangedF32( BitStream, m_LocalRotVel.Yaw,   INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            WriteRangedF32( BitStream, m_LocalRotVel.Roll,  INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
        }

        // Boost vars
        DirtyBits &= ~VEHICLE_DIRTY_BIT_BOOST;
        WriteRangedF32( BitStream, m_Control.Boost, INPUT_BOOST_BITS, BOOST_MIN, BOOST_MAX );

        // Thrust + Energy
        DirtyBits &= ~VEHICLE_DIRTY_BIT_THRUST_ENERGY;
        WriteRangedF32( BitStream, m_Control.ThrustX, INPUT_THRUST_BITS, THRUST_MIN, THRUST_MAX );
        WriteRangedF32( BitStream, m_Control.ThrustY, INPUT_THRUST_BITS, THRUST_MIN, THRUST_MAX );
        WriteRangedF32( BitStream, m_Energy,          INPUT_ENERGY_BITS, ENERGY_MIN, ENERGY_MAX );
        WriteRangedF32( BitStream, m_WeaponEnergy,    INPUT_WEAPON_BITS, WEAPON_MIN, WEAPON_MAX );

        // Health
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_HEALTH) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_HEALTH;

            // Non-zero?
            if( BitStream.WriteFlag( m_Health > 0 ) )
                WriteRangedF32( BitStream, m_Health, INPUT_HEALTH_BITS, HEALTH_MIN, HEALTH_MAX );
        }
   
        // State
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_STATE) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_STATE;
            BitStream.WriteRangedU32( (u32)m_State, STATE_START, STATE_END );
            WriteRangedF32( BitStream, m_StateTime, INPUT_STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX );
        }
    }
    else
    {
        // Sending to a ghost vehicle...

        // Make sure unused bits are cleared!!
        DirtyBits &= PLAYER_DIRTY_BIT_VEHICLE_USER_BITS;

        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_POS) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_POS;
            
            // Position
            BitStream.WriteWorldPosCM( m_WorldPos );

            // Velocity
            WriteRangedF32( BitStream, m_Vel.X, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
            WriteRangedF32( BitStream, m_Vel.Y, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
            WriteRangedF32( BitStream, m_Vel.Z, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
        }

        // Rotation
        if (BitStream.WriteFlag((DirtyBits & VEHICLE_DIRTY_BIT_ROT) != 0))
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_ROT;
            if( IsGroundBased() )
            {
                WriteRangedF32( BitStream, m_WorldRot.Pitch,    GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_WorldRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_WorldRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_LocalRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_LocalRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                
                WriteRangedF32( BitStream, m_WorldRotVel.X,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                WriteRangedF32( BitStream, m_WorldRotVel.Y,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                WriteRangedF32( BitStream, m_WorldRotVel.Z,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                WriteRangedF32( BitStream, m_WorldRotVel.W,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                WriteRangedF32( BitStream, m_LocalRotVel.Yaw,   GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                WriteRangedF32( BitStream, m_LocalRotVel.Roll,  GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            }
            else
            {
                WriteRangedF32( BitStream, m_LocalRot.Pitch,    GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_LocalRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_LocalRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                WriteRangedF32( BitStream, m_LocalRotVel.Pitch, GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                WriteRangedF32( BitStream, m_LocalRotVel.Yaw,   GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                WriteRangedF32( BitStream, m_LocalRotVel.Roll,  GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            }
        }

        // Boost
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_BOOST) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_BOOST;
            WriteRangedF32( BitStream, m_Control.Boost, GHOST_BOOST_BITS, BOOST_MIN, BOOST_MAX );
        }

        // Thrust + Energy
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_THRUST_ENERGY) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_THRUST_ENERGY;
            WriteRangedF32( BitStream, m_Control.ThrustX, GHOST_THRUST_BITS, THRUST_MIN, THRUST_MAX );
            WriteRangedF32( BitStream, m_Control.ThrustY, GHOST_THRUST_BITS, THRUST_MIN, THRUST_MAX );
            WriteRangedF32( BitStream, m_Energy,          GHOST_ENERGY_BITS, ENERGY_MIN, ENERGY_MAX );
            WriteRangedF32( BitStream, m_WeaponEnergy,    GHOST_WEAPON_BITS, WEAPON_MIN, WEAPON_MAX );
        }

        // Health
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_HEALTH) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_HEALTH;

            // Non-zero?
            if( BitStream.WriteFlag( m_Health > 0 ) )
                WriteRangedF32( BitStream, m_Health, GHOST_HEALTH_BITS, HEALTH_MIN, HEALTH_MAX );
        }

        // State
        if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_STATE) != 0 ) )
        {
            DirtyBits &= ~VEHICLE_DIRTY_BIT_STATE;
            BitStream.WriteRangedU32( (u32)m_State, STATE_START, STATE_END );
            WriteRangedF32( BitStream, m_StateTime, GHOST_STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX );
        }
    }

    // Seat status
    if( BitStream.WriteFlag( (DirtyBits & VEHICLE_DIRTY_BIT_SEAT_STATUS) != 0 ) )
    {
        DirtyBits &= ~VEHICLE_DIRTY_BIT_SEAT_STATUS;

        // Write all seat ID's
        for( s32 i=0; i<m_nSeats; i++)
            BitStream.WriteObjectID( m_Seats[i].PlayerID );
    }

    // Check to make sure all the dirty bits have been cleared as expected!
    //if( Priority == 1 )
    //{
    //    if (MoveUpdate)
    //        ASSERT((DirtyBits & PLAYER_DIRTY_BIT_VEHICLE_USER_BITS) == 0) ;
    //    else
    //        ASSERT(DirtyBits == 0) ;
    //}
}

xbool gnd_effect::ReadDirtyBitsData( const bitstream& BitStream, f32 SecInPast )
{
    (void)SecInPast;
    
    // Copy movement vars that maybe read
    vector3    WorldPos    = m_WorldPos;
    vector3    Vel         = m_Vel;
    radian3    WorldRot    = m_WorldRot;
    quaternion WorldRotVel = m_WorldRotVel;
    radian3    LocalRot    = m_LocalRot;
    radian3    LocalRotVel = m_LocalRotVel;
    f32        Boost       = m_Control.Boost;
    vector2    Thrust      = vector2( m_Control.ThrustX, m_Control.ThrustY );
    f32        Energy      = m_Energy;
    f32        WeaponEnergy= m_WeaponEnergy;
    
    xbool   MoveUpdate;

    // Move update?
    if( (MoveUpdate = BitStream.ReadFlag()) )
    {
        // Reading to a controlled vehicle...

        // Position
        BitStream.ReadVector( WorldPos );
        ReadRangedF32( BitStream, Vel.X, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );
        ReadRangedF32( BitStream, Vel.Y, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );
        ReadRangedF32( BitStream, Vel.Z, INPUT_VEL_BITS, VEL_MIN, VEL_MAX );

        // Rotation
        if( IsGroundBased() )
        {
            ReadRangedF32( BitStream, WorldRot.Pitch,    INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, WorldRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, WorldRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, LocalRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, LocalRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            
            ReadRangedF32( BitStream, WorldRotVel.X,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            ReadRangedF32( BitStream, WorldRotVel.Y,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            ReadRangedF32( BitStream, WorldRotVel.Z,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            ReadRangedF32( BitStream, WorldRotVel.W,     INPUT_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
            ReadRangedF32( BitStream, LocalRotVel.Yaw,   INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            ReadRangedF32( BitStream, LocalRotVel.Roll,  INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
        }
        else
        {
            ReadRangedF32( BitStream, LocalRot.Pitch,    INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, LocalRot.Yaw,      INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, LocalRot.Roll,     INPUT_ROT_BITS,     ROT_MIN,     ROT_MAX );
            ReadRangedF32( BitStream, LocalRotVel.Pitch, INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            ReadRangedF32( BitStream, LocalRotVel.Yaw,   INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            ReadRangedF32( BitStream, LocalRotVel.Roll,  INPUT_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
        }
        
        // Boost
        ReadRangedF32( BitStream, Boost, INPUT_BOOST_BITS, BOOST_MIN, BOOST_MAX );

        // Thrust + Energy
        ReadRangedF32( BitStream, Thrust.X,      INPUT_THRUST_BITS, THRUST_MIN, THRUST_MAX );
        ReadRangedF32( BitStream, Thrust.Y,      INPUT_THRUST_BITS, THRUST_MIN, THRUST_MAX );
        ReadRangedF32( BitStream, Energy,        INPUT_ENERGY_BITS, ENERGY_MIN, ENERGY_MAX );
        ReadRangedF32( BitStream, WeaponEnergy,  INPUT_WEAPON_BITS, WEAPON_MIN, WEAPON_MAX );

        // Health
        if( BitStream.ReadFlag() )
        {
            // Non-zero?
            if( BitStream.ReadFlag() )
            {
                // Read health
                ReadRangedF32( BitStream, m_Health, INPUT_HEALTH_BITS, HEALTH_MIN, HEALTH_MAX );

                // Incase health is truncated to zero
                if( m_Health == 0 )
                    m_Health = 1;
            }
            else
                m_Health = 0;
        }

        // State
        if( BitStream.ReadFlag() )
        {
            u32 TempU32;
            BitStream.ReadRangedU32( TempU32, STATE_START, STATE_END );
            m_State = (state)TempU32;

            ReadRangedF32( BitStream, m_StateTime, INPUT_STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX );
        }
    }
    else
    {
        // Reading to a ghost vehicle...

        // Position vars
        if( BitStream.ReadFlag() )
        {
            // Position
            BitStream.ReadWorldPosCM( WorldPos );

            // Velocity
            ReadRangedF32( BitStream, Vel.X, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
            ReadRangedF32( BitStream, Vel.Y, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
            ReadRangedF32( BitStream, Vel.Z, GHOST_VEL_BITS, VEL_MIN, VEL_MAX );
        }

        // Rotation vars
        if (BitStream.ReadFlag())
        {
            if( IsGroundBased() )
            {
                ReadRangedF32( BitStream, WorldRot.Pitch,    GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, WorldRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, WorldRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, LocalRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, LocalRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );

                ReadRangedF32( BitStream, WorldRotVel.X,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                ReadRangedF32( BitStream, WorldRotVel.Y,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                ReadRangedF32( BitStream, WorldRotVel.Z,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                ReadRangedF32( BitStream, WorldRotVel.W,     GHOST_ROT_VEQ_BITS, ROT_VEQ_MIN, ROT_VEQ_MAX );
                ReadRangedF32( BitStream, LocalRotVel.Yaw,   GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                ReadRangedF32( BitStream, LocalRotVel.Roll,  GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            }
            else
            {
                ReadRangedF32( BitStream, LocalRot.Pitch,    GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, LocalRot.Yaw,      GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, LocalRot.Roll,     GHOST_ROT_BITS,     ROT_MIN,     ROT_MAX );
                ReadRangedF32( BitStream, LocalRotVel.Pitch, GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                ReadRangedF32( BitStream, LocalRotVel.Yaw,   GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
                ReadRangedF32( BitStream, LocalRotVel.Roll,  GHOST_ROT_VEL_BITS, ROT_VEL_MIN, ROT_VEL_MAX );
            }
        }

        // Boost vars
        if( BitStream.ReadFlag() )
        {
            ReadRangedF32( BitStream, Boost, GHOST_BOOST_BITS, BOOST_MIN, BOOST_MAX );
        }

        // Thrust + Energy
        if( BitStream.ReadFlag() )
        {
            ReadRangedF32( BitStream, Thrust.X,      GHOST_THRUST_BITS, THRUST_MIN, THRUST_MAX );
            ReadRangedF32( BitStream, Thrust.Y,      GHOST_THRUST_BITS, THRUST_MIN, THRUST_MAX );
            ReadRangedF32( BitStream, Energy,        GHOST_ENERGY_BITS, ENERGY_MIN, ENERGY_MAX );
            ReadRangedF32( BitStream, WeaponEnergy,  GHOST_WEAPON_BITS, WEAPON_MIN, WEAPON_MAX );
        }

        // Health
        if( BitStream.ReadFlag() )
        {
            // Non-zero?
            if( BitStream.ReadFlag() )
            {
                // Read health
                ReadRangedF32( BitStream, m_Health, GHOST_HEALTH_BITS, HEALTH_MIN, HEALTH_MAX );

                // Incase health is truncated to zero
                if( m_Health == 0 )
                    m_Health = 1;
            }
            else
                m_Health = 0;
        }

        // State
        if( BitStream.ReadFlag() )
        {
            // State
            u32 TempU32;
            BitStream.ReadRangedU32( TempU32, STATE_START, STATE_END );
            m_State = (state)TempU32;

            ReadRangedF32( BitStream, m_StateTime, GHOST_STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX );
        }
    }

    // Always update on move
    xbool UpdateMovement = MoveUpdate;

    // Controlled by a player?
    player_object* pPlayer = IsPlayerControlled();
    if( pPlayer )
    {
        // Only update if this is a ghost
        if( !pPlayer->GetProcessInput() )
            UpdateMovement = TRUE;
    }
    else
    {
        // Always update if not player controlled
        UpdateMovement = TRUE;
    }

    // Setup new movement vars?
    if( UpdateMovement )
    {
        m_WorldPos        = WorldPos;
        m_Vel             = Vel;
        m_WorldRot        = WorldRot;
        m_WorldRotVel     = WorldRotVel;
        m_LocalRot        = LocalRot;
        m_LocalRotVel     = LocalRotVel;
        m_Control.Boost   = Boost;
        m_Control.ThrustX = Thrust.X;
        m_Control.ThrustY = Thrust.Y;
        m_Energy          = Energy;
        m_WeaponEnergy    = WeaponEnergy;
    }

    // Player status
    if( BitStream.ReadFlag() )
    {
        // Read all seat ID's
        for( s32 i=0; i<m_nSeats; i++)
            BitStream.ReadObjectID( m_Seats[i].PlayerID );
    }

    // Setup bounds incase pos has been updated
    m_WorldBBox = m_GeomBBox;
    m_WorldBBox.Translate( m_WorldPos );

    return MoveUpdate;
}

//==============================================================================

object::update_code gnd_effect::OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast )
{
    s32 i ;

    // Calculate current draw info incase of blending
    vector3 DrawPos = m_WorldPos + (m_BlendMove.Blend * m_BlendMove.DeltaPos);
    radian3 DrawRot = m_Rot      + (m_BlendMove.Blend * m_BlendMove.DeltaRot);

    // Read new info...
    ReadDirtyBitsData( BitStream, SecInPast ); 
    
    // Keep instances in sync
    UpdateInstances();
   
    // Skip prediction if it's controlled by a player on this machine!
    player_object* pPlayer = IsPlayerControlled();
    if ((pPlayer) && (pPlayer->GetProcessInput()))
        return UPDATE;

    // Run the logic to catch up
    while( SecInPast > 0.0f )
    {
        // Advance logic, destroy if requested...
        if (AdvanceLogic( MIN( SecInPast, (1.0f / 30.0f) ) ) == DESTROY)
            return DESTROY ;

        // Time time slice
        SecInPast -= (1.0f / 30.0f) ;
    }

    // Is it valid to blend?
    if(     (m_State == STATE_MOVING)
        &&  ((m_WorldPos - DrawPos).LengthSquared() <= (MAX_BLEND_DIST * MAX_BLEND_DIST)) )
    {
        // Blend from last pos and rot
        m_BlendMove.DeltaPos = DrawPos - m_WorldPos ;
        
        m_BlendMove.DeltaRot = DrawRot - m_Rot ;
        m_BlendMove.DeltaRot.ModAngle2() ;

        m_BlendMove.Blend    = 1;     // Start from last pos
        m_BlendMove.BlendInc = -1.0f / MAX_BLEND_TIME ;
    }

    // Keep instances in sync
    UpdateInstances() ;

    // Check all seats except the pilot seat
    for( i=1; i<m_nSeats; i++ )
    {
        // Is there a player in this seat?
        player_object* pPlayer = GetSeatPlayer( i );
        if ( pPlayer )
            pPlayer->VehicleMoved( this );   // Update the player position
    }

    return UPDATE;
}

//==============================================================================

void gnd_effect::OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    // Send data (not a move though...)
    WriteDirtyBitsData( BitStream, DirtyBits, Priority, FALSE ) ;
}

//==============================================================================

void gnd_effect::OnAcceptInit ( const bitstream& BitStream )
{
    // Simply read all dirty bit data
    BitStream.ReadU32( m_TeamBits );
    ReadDirtyBitsData( BitStream, 0 ) ;
}

//==============================================================================

void gnd_effect::OnProvideInit ( bitstream& BitStream )
{
    // Simply send all dirty bit data...
    u32 DirtyBits = VEHICLE_DIRTY_BIT_INIT ;
    BitStream.WriteU32( m_TeamBits );
    WriteDirtyBitsData( BitStream, DirtyBits, 1.0f, TRUE ) ;
}

//==============================================================================

void gnd_effect::OnAdd ( void )
{
    m_TrueTime       = 0.0f;
    m_CheckTime      = 0.0f;
    m_ColBoxYOff     = 0.0f;
    m_IsImmobile     = FALSE;
    m_OnGround       = FALSE;
    m_InGroundEffect = FALSE;
    m_IsSettling     = FALSE;
}

//==============================================================================

void gnd_effect::OnRemove ( void )
{
    // Just incase there are any players left lying around...
    // (due to dodgey vehicle logic...)
    EjectPlayers() ;
}
                                
//==============================================================================

void gnd_effect::Initialize ( const s32      GeomShapeType,    // Shape to user for geometry
                              const s32      CollShapeType,    // Shape to use for collision
                              const vector3& Pos,              // Initial position
                              const radian   InitHdg,          // Initial heading
                              const f32      SeatRadius,       // Radius of seats
                                    u32      TeamBits )
{
    // Call base class
    vehicle::Initialize(GeomShapeType, CollShapeType, Pos, InitHdg, SeatRadius, TeamBits );

    // Clear movement vars
    m_Vel.Zero();           // velocity vector
    
    x_memset( &m_Control, 0, sizeof( m_Control ) );
    
    m_GroundEffect = 0.0f;
    m_WorldRot     = radian3( R_0, R_0, R_0 ); 
    m_WorldRotVel  = quaternion( radian3( R_0, R_0, R_0 ) );
    m_LocalRotVel  = radian3( R_0, R_0, R_0 );
    m_LocalRot     = radian3( R_0, InitHdg, R_0 );
    m_Speed.Zero();
    
    // Setup state
    m_State     = STATE_SPAWN ;
    m_StateTime = 0 ;
    m_GeomInstance.SetAnimByType(ANIM_TYPE_VEHICLE_SPAWN) ;

    // Send all data
    m_DirtyBits = VEHICLE_DIRTY_BIT_INIT ;
}

//==============================================================================

void gnd_effect::Initialize( const vector3& , radian , u32 )
{
    ASSERT( FALSE );
}

//==============================================================================

void gnd_effect::InitPhysics( veh_physics* pPhysics )
{
    // Set the pointer to the physics constants for this vehicle type
    m_pPhysics = pPhysics;
}

//==============================================================================

void gnd_effect::ApplyMove( player_object* pPlayer, s32 SeatIndex, player_object::move& Move )
{
    // Flag logic has happened this frame
    m_AttrBits |= object::ATTR_LOGIC_APPLIED ;

    // Get seat info
    const seat& Seat = GetSeat( SeatIndex );

    // Only apply the move if it's coming from the pilot or when there's
    // no player controlling the vehicle so that the energy + weapon energy recharge
    if ( ( Seat.Type == SEAT_TYPE_PILOT ) || ( pPlayer == NULL ) )
    {
        // Auto record dirty bits
        BeginSetDirtyBits();

        // No need worrying about input if we're dead.
        if( m_Health > 0.0f )
        {
            f32 ZoomFactor = 1.0f;

            if( pPlayer && (GetType() != object::TYPE_VEHICLE_THUNDERSWORD) )
            {
                // Compute scale factor for when player is Zoomed
                // Convert zoom factor from 1->20 to parametric
                ZoomFactor = pPlayer->GetCurrentZoomFactor() - 1.0f;
                ZoomFactor = 1.0f - (ZoomFactor / (20.0f - 1.0f));
                
                // At full zoom allow at least some movement
                ZoomFactor = MAX( ZoomFactor, MinZoomFactor );
            }

            //-----------------------
            // Set controller inputs
            //-----------------------
           
            // Set thrust vector for movement
            m_Control.ThrustX = Move.MoveX;// * ZoomFactor;
            m_Control.ThrustY = Move.MoveY;// * ZoomFactor;
            
            // Set turbo boost
            m_Control.Boost = Move.JetKey ? 1.0f : 0.0f;

            // Check for turn (ignore if holding down free look)
            if( Move.FreeLookKey )
            {
                m_Control.Pitch = 0.0f;
                m_Control.Yaw   = 0.0f;
                m_Control.Roll  = 0.0f;
            }
            else
            {
                m_Control.Pitch = Move.LookY * ZoomFactor;
                m_Control.Yaw   = Move.LookX * ZoomFactor;
                m_Control.Roll  = Move.LookX * ZoomFactor;
            }
        }
        else
        {
            m_Control.ThrustX = 0.0f;
            m_Control.ThrustY = 0.0f;
        }

        // Consume energy when boosting
        if( m_Control.Boost > 0.0f )
        {
            if( m_Energy > 0.0f )
            {
                m_Energy -= m_pPhysics->BoosterConsume * 100.0f * Move.DeltaTime;
    
                if( m_Energy < 0.0f )
                    m_Energy = 0.0f;
            }
        }
        else
        {
            // Replenish energy when not turboing
            if( m_Energy < 100.0f )
            {
                m_Energy += m_pPhysics->EnergyRecharge * 100.0f * Move.DeltaTime;
    
                if( m_Energy > 100.0f )
                    m_Energy = 100.0f;
            }
        }
        
        // Replenish weapon energy
        if( m_WeaponEnergy < 1.0f )
        {
            /*
            x_printf( "grneffect::ApplyMove()" );
            x_printf( "    E(%g) = E(%g) + R(%g) * T(%g)\n",
                      m_WeaponEnergy + m_pPhysics->WeaponRecharge * Move.DeltaTime,
                      m_WeaponEnergy,
                      m_pPhysics->WeaponRecharge,
                      Move.DeltaTime );
            */
            m_WeaponEnergy += m_pPhysics->WeaponRecharge * Move.DeltaTime;
    
            if( m_WeaponEnergy > 1.0f )
                m_WeaponEnergy = 1.0f;
        }
        
        // Set dirty bits
        EndSetDirtyBits() ;

        // Truncate ready for sending over net
        Truncate() ;

        // Update instances
        UpdateInstances() ;
    }
}

//==============================================================================

void gnd_effect::GetGroundInfo( collider::collision& Collision )
{
    // Get the local Y-Axis
    vector3 LocalY( 0.0f, 1.0f, 0.0f );
    LocalY.Rotate ( m_Rot );
    
    // Cast a ray from vehicle along the local negative Y-axis to see if the ground is there
    vector3 S( m_WorldPos );
    vector3 E( S + (LocalY * -MaxGroundHeight) );

    collider Collider;
    Collider.RaySetup( this, S, E );
    ObjMgr.Collide( ATTR_SOLID, Collider );

    Collider.GetCollision( Collision );

    // Set the flag indicating whether vehicle is in ground effect
    m_InGroundEffect = Collision.Collided;
    m_OnGround       = FALSE;
    m_GroundEffect   = 0.0f;

    // Set the flag indicating whether vehicle is on the ground
    if( Collision.Collided )
    {
        // Get the height above ground
        vector3 V  = Collision.Point - S;
        f32 Height = V.Length();

        // Check if the vehicle is physically on the ground
        f32 Length = m_WorldPos.Y - m_WorldBBox.Min.Y;
        
        if( Height < Length )
        {
            m_OnGround = TRUE;
        }

        // Calculate ground effect (1->0 as height increases)
        if( Height > MinGroundHeight )
        {
            m_GroundEffect = 1.0f - ((Height - MinGroundHeight) / (MaxGroundHeight - MinGroundHeight));
        }
        else
            m_GroundEffect = 1.0f;
    }
}

//==============================================================================

void gnd_effect::SetInput( veh_input& Input )
{
    veh_physics& Physics = *m_pPhysics;
    
    x_memset( &Input, 0, sizeof( Input ) );

    player_object* pPlayer = IsPlayerControlled();
    
    if( pPlayer )
    {
        // Calculate pitch yaw roll from right stick input
        Input.Pitch =  m_Control.Pitch * Physics.MaxPitch;
        Input.Yaw   =  m_Control.Yaw   * Physics.AccelYaw;
        Input.Roll  = -m_Control.Roll  * Physics.MaxRoll;

        // Set parametric thrust value based on left stick input
        Input.ForeThrust = m_Control.ThrustY;
        Input.SideThrust = m_Control.ThrustX * SidewardThrustMul;
        
        // Limit reverse thrust
        if( Input.ForeThrust < 0.0f )
            Input.ForeThrust *= ReverseThrustMul;
            
        // Set the boost if we have enough energy
        Input.Boost = (m_Energy > MinEnergyForBoost) ? m_Control.Boost : 0.0f;

        // Reduce pitch the closer the vehicle gets to its max operating ceiling
        f32 MaxHeight    = GameMgr.GetBounds().Max.Y;
        f32 CeilingStart = MaxHeight / 2.0f;
        
        if( m_WorldPos.Y > CeilingStart )
        {
            // Clamp the height
            if( m_WorldPos.Y > MaxHeight )
                m_WorldPos.Y = MaxHeight;
            
            // Only limit the pitch-up so we can at least descend
            if( Input.Pitch < 0.0f )
            {
                f32 K = (m_WorldPos.Y - CeilingStart) / (MaxHeight - CeilingStart);
                Input.Pitch *= 1.0f - K;
            }
        }

        // Apply a roll for sideways movement
        Input.Roll += SidewardRollMul * -m_Control.ThrustX * Physics.MaxRoll;

        // Calculate parametric velocity
        f32 VelocityT = MINMAX( 0.0f, (m_Vel.Length() / Physics.MaxForwardSpeed), 1.0f );

        // Scale the roll based on velocity
        Input.Roll *= MAX( MinRollMul, VelocityT );

        // Reduce control when airborne to turn vehicle into a projectile
        if( Physics.IsGroundBased )
        {
            Input.Pitch *= m_GroundEffect;
            Input.Yaw   *= m_GroundEffect;
            Input.Roll  *= m_GroundEffect;
        }
    }
}

//==============================================================================

void gnd_effect::GetWorldRotation( const vector3& GroundNormal,
                                   radian3&       World )
{
    vector3 Normal( GroundNormal );

    if( m_InGroundEffect == FALSE )
    {
        // If vehicle is not in ground effect it is a free falling body.
        // Compute a vector perpendicular to local-X axis and velocity vector.
        // This will give us a dummy ground "up" vector for the free falling body.
        vector3 LocalX( 1.0f, 0.0f, 0.0f );
        LocalX.Rotate ( m_Rot );
        
        if( m_Vel.Length() > 0.01f )
        {
            Normal = m_Vel.Cross( LocalX );
        }
        else
        {
            vector3 LocalZ( 0.0f, 0.0f, 1.0f );
            LocalZ.Rotate ( m_Rot );
            Normal = LocalZ.Cross( LocalX );
        }
    }
    
    // Clamp the pitch and roll angles
    if( LimitVehicleAngle )
    {
        radian Pitch, Yaw;
        Normal.GetPitchYaw( Pitch, Yaw );
        
        radian Ang = -(R_90 - MaxVehicleAngle);

        if( Pitch > Ang )
            Pitch = Ang;
        
        // Calculate the new normal with the clamped pitch
        Normal = vector3( 0.0f, 0.0f, 1.0f );
        Normal.Rotate( radian3( Pitch, Yaw, R_0 ) );
    }
    
    // Get ground rotation. When vehicle is on level ground (ground normal = WorldY)
    // return the identity quaternion.
    vector3 WorldY( 0.0f, 1.0f, 0.0f );
    vector3 Axis;
    radian  Angle;
    WorldY.GetRotationTowards( Normal, Axis, Angle );

    quaternion GroundRot( Axis, Angle );
    World    = GroundRot.GetRotation();
}    

//==============================================================================

void gnd_effect::ComputeAngular( const veh_input& Input,
                                 const radian3&   World,
                                 f32              DeltaTime )
{
    veh_physics& Physics = *m_pPhysics;

    //
    // Compute the local vehicle rotations due to player input
    //
    
    f32 AccelPitch = ((Input.Pitch - m_LocalRot.Pitch) / Physics.MaxPitch) * Physics.AccelPitch;
    f32 AccelRoll  = ((Input.Roll  - m_LocalRot.Roll ) / Physics.MaxRoll ) * Physics.AccelRoll;
    f32 AccelYaw   = Input.Yaw;

    // Calculate angular velocity        
    m_LocalRotVel.Pitch += AccelPitch * DeltaTime;
    m_LocalRotVel.Yaw   += AccelYaw   * DeltaTime;
    m_LocalRotVel.Roll  += AccelRoll  * DeltaTime;
    
    // Apply drag
    m_LocalRotVel.Pitch -= m_LocalRotVel.Pitch * Physics.LocalDrag * DeltaTime;
    m_LocalRotVel.Yaw   -= m_LocalRotVel.Yaw   * Physics.LocalDrag * DeltaTime;
    m_LocalRotVel.Roll  -= m_LocalRotVel.Roll  * Physics.LocalDrag * DeltaTime;
    
    // Apply velocity
    m_LocalRot.Pitch    += m_LocalRotVel.Pitch * DeltaTime;
    m_LocalRot.Yaw      += m_LocalRotVel.Yaw   * DeltaTime;
    m_LocalRot.Roll     += m_LocalRotVel.Roll  * DeltaTime;
    
    // Keep normalized
    m_LocalRot.ModAngle2();
    m_LocalRotVel.ModAngle2();

    //
    // Compute the world rotation due to ground orientation or freefall
    //
    
    quaternion Orientation( m_WorldRot );

    if( Physics.IsGroundBased == TRUE )
    {
        quaternion Target     ( World );
        quaternion DeltaRot   ( Orientation );
        quaternion RotVelocity( m_WorldRotVel );
    
        // Compute a rotation relative to the current orientation that will take us to the target orientation
        DeltaRot.Invert();
        DeltaRot *= Target;
    
        // Convert the quaternion to angle-axis format so we can scale the rotation
        radian  Angle = DeltaRot.GetAngle();
        vector3 Axis  = DeltaRot.GetAxis();
    
        // Fixup the angle from 0-360 range to -180 to 180
        Angle = x_ModAngle2( Angle );
    
        // Scale the acceleration by the distance to target
        f32 T = Angle / R_180;

        // Scale the acceleration by the ground effect
        T *= MAX( MinRotMul, m_GroundEffect );
    
        // Compute angular velocity
        RotVelocity *= quaternion( Axis, T * Physics.WorldAccel * DeltaTime );
        RotVelocity.Normalize();
    
        // Compute and apply Drag to angular velocity
        Angle = RotVelocity.GetAngle();
        Axis  = RotVelocity.GetAxis();
        RotVelocity *= quaternion( Axis, -Angle * Physics.WorldDrag * DeltaTime );
        RotVelocity.Normalize();
    
        // Apply velocity to current orientation    
        Angle = RotVelocity.GetAngle();
        Axis  = RotVelocity.GetAxis();
        Orientation *= quaternion( Axis, Angle * DeltaTime );
        Orientation.Normalize();
    
        m_WorldRot    = Orientation.GetRotation();
        m_WorldRotVel = RotVelocity;

        if( m_IsSettling )
        {
            // Blend world rotation to zero
            f32 Settle    = m_Vel.Length() / SettleSpeed;
            vector3 Axis  = m_WorldRotVel.GetAxis();
            radian  Angle = m_WorldRotVel.GetAngle();
            m_WorldRotVel.Setup( Axis, Angle * Settle );
        }

        // Keep normalized
        m_WorldRot.ModAngle2();

        // Apply the local vehicle rotations
        Orientation *= quaternion( m_LocalRot );
    }
    else
    {
        // Airborne vehicles can just use the local rotations
        Orientation = quaternion( m_LocalRot );
    }

    // Extract the final rotation used for rendering
    m_Rot = Orientation.GetRotation();
    m_Rot.ModAngle2();
}

//==============================================================================

void gnd_effect::ComputeLinear( const veh_input& Input, f32 DeltaTime )
{
    veh_physics& Physics = *m_pPhysics;

    // Get the local axes of the vehicle.
    vector3 LocalX( 1.0f, 0.0f, 0.0f );
    vector3 LocalY( 0.0f, 1.0f, 0.0f );
    vector3 LocalZ( 0.0f, 0.0f, 1.0f );
    vector3 WorldY( 0.0f, 1.0f, 0.0f );
    
    LocalX.Rotate( m_Rot );
    LocalY.Rotate( m_Rot );
    LocalZ.Rotate( m_Rot );
    
    // Rotate the vertical boost vector relative to the move stick.
    vector3 TBoost( 0.0f, 1.0f, 0.0f );
    TBoost.Rotate ( radian3( R_85 * m_Control.ThrustY, R_0, R_85 * m_Control.ThrustX ) );
    TBoost.Rotate ( m_Rot );
    
    // Scale down the normal foreward acceleration
    f32 BoostMul = TurboBoostMul * Physics.LinearAccel;
    
    if( Physics.IsGroundBased )
    {
        // Dont allow ground based vehicles to boost vertically
        vector3 Boost ( TBoost );
        Boost.Normalize();
    
        f32 Dot = Boost.Dot( LocalY );
        BoostMul *= 1.0f - ABS( Dot );
    }

    // For airborne vehicles scale down the effects of gravity relative to the current velocity
    f32 VelocityT  = MINMAX( 0.0f, (m_Vel.Length() / Physics.MaxForwardSpeed), 1.0f );
    f32 GravityMul = 1.0f;
    if( Physics.IsGroundBased == FALSE )
    {
        if( VelocityT > ZeroGVelocityT ) GravityMul  = 0.0f;
        else                             GravityMul *= 1.0f - VelocityT;
    }

    // Reduce the effects of gravity the slower the vehicle gets to reduce jitter
    if( m_IsSettling )
    {
        f32 T = m_Vel.Length() / SettleSpeed;
        GravityMul *= T;
    }

    // Calculate linear velocity
    m_Vel -= WorldY * (GravityMul       * Physics.Gravity     * DeltaTime);
    m_Vel += LocalZ * (Input.ForeThrust * Physics.LinearAccel * DeltaTime);
    m_Vel += LocalX * (Input.SideThrust * Physics.LinearAccel * DeltaTime);
    m_Vel += TBoost * (Input.Boost      * BoostMul            * DeltaTime);
    
    // Apply drag    
    f32 Drag = Physics.LinearDrag;
    
    if( Physics.IsGroundBased )
    {
        // If ground vehicle is airborne we should remove the drag
        if( m_InGroundEffect == FALSE )
            Drag = 0.0f;
    }
    
    m_Vel -= m_Vel * Drag * DeltaTime;

    // Cap off small velocities to reduce network traffic
    if( ABS( m_Vel.X ) < VEL_CAP_OFF ) m_Vel.X = 0.0f;
    if( ABS( m_Vel.Y ) < VEL_CAP_OFF ) m_Vel.Y = 0.0f;
    if( ABS( m_Vel.Z ) < VEL_CAP_OFF ) m_Vel.Z = 0.0f;
}

//==============================================================================

xbool gnd_effect::SettleVehicle( const vector3& GroundNormal )
{
    // Determine if there was any input from player
    xbool IsControlled = FALSE;
    
    if( (ABS( m_Control.ThrustX ) > StickEpsilon) ||
        (ABS( m_Control.ThrustY ) > StickEpsilon) ||
        (     m_Control.Boost     > 0.0f) )
        IsControlled = TRUE;

    // If vehicle passes the following tests, then we can go ahead and
    // settle it on the ground to avoid jitter.
    if( (IsControlled   == FALSE)         &&
        (m_OnGround     == TRUE )         &&
        (m_State        != STATE_SPAWN)   &&
        (m_Vel.Length() <  SettleSpeed)   &&
        (GroundNormal.Y >  SettleNormalY) )
        return( TRUE );
    
    return( FALSE );
}

//==============================================================================

vector3 GetSpikeNormal( collider::collision& Collision, bbox& BBox )
{
    vector3 Normal( Collision.Plane.Normal );
    vector3 Corner;
    
    Corner.Y = BBox.Min.Y;
    
    for( s32 i=0; i<4; i++ )
    {
        switch( i )
        {
            case 0 :
                Corner.X = BBox.Min.X;
                Corner.Z = BBox.Min.Z;
                break;
        
            case 1 :
                Corner.X = BBox.Max.X;
                Corner.Z = BBox.Min.Z;
                break;
        
            case 2 :
                Corner.X = BBox.Min.X;
                Corner.Z = BBox.Max.Z;
                break;
        
            case 3 :
                Corner.X = BBox.Max.X;
                Corner.Z = BBox.Max.Z;
                break;
        }
    
        f32 Dist = Collision.Plane.Distance( Corner );
        
        // Check if we have any points behind the collision plane 
        if( Dist < -SpikeTestDist )
        {
            // Return the world up vector instead of the plane normal.
            // This will stop us getting stuck on the spike.
            Normal = vector3( 0.0f, 1.0f, 0.0f );
            break;
        }
    }
    
    return( Normal );
}

//==============================================================================

xbool gnd_effect::StepVehicle( const collider::collision& Collision,
                               const vector3&             Move,
                                     bbox&                BBox,
                                     collider&            Collider )
{
    // Only test walls that are sufficiently steep
    if( ABS( Collision.Plane.Normal.Y ) > x_cos( ThrustLimitAngle ) )
        return( FALSE );

    // Move forward a portion of the original movement
    vector3 StepForward( Move * 0.1f );
    
    // Dont step if we are not moving forward enough
    if( (SQR( StepForward.X ) + SQR( StepForward.Z )) < SQR( MovingEpsilon ) )
        return( FALSE );
    
    // Move forward at least a set amount
    vector3 MoveDir( Move );
    MoveDir.Normalize();
    StepForward += MoveDir * (MinFaceDist * 2.0f);
    
    if( StepForward.LengthSquared() < collider::MIN_EXT_DIST_SQR )
        return( FALSE );
    
    f32 Y = BBox.Min.Y;
    if( Collision.Point.Y < Y )
        Y = Collision.Point.Y;
    
    // Compute delta to take us to highest point of collision
    f32 DeltaHeight = MAX( Collider.GetHighPoint() - Y, 0.0f );

    // Check if the step is too big to move over
    if( DeltaHeight > MaxStepHeight )
        return( FALSE );

    // Move slightly higher than we need to so we back off from the surface
    DeltaHeight += MinFaceDist * 2.0f;
    
    if( DeltaHeight < collider::MIN_EXT_DIST )
        return( FALSE );

    // Attempt to step up!    
    vector3 StepUp( 0.0f, DeltaHeight, 0.0f );
    collider::collision StepCollision;

    s_StepDeltaHeight = DeltaHeight;
    
    Collider.ExtSetup( this, BBox, StepUp );
    ObjMgr.Collide( ATTR_SOLID, Collider );
    Collider.GetCollision( StepCollision );
    
    // If we hit something above us then we cant move the vehicle up
    if( StepCollision.Collided )
        return( FALSE );

    // Lift the vehicle up so we can attempt to drive forwards
    m_WorldPos  +=  StepUp;
    BBox.Translate( StepUp );
    
    // Attempt to step forward slightly!
    Collider.ExtSetup( this, BBox, StepForward );
    ObjMgr.Collide( ATTR_SOLID, Collider );
    Collider.GetCollision( StepCollision );

    // If we hit something in front of us then we cant perform the step over
    if( StepCollision.Collided )
        return( FALSE );

    return( TRUE );
}

//==============================================================================

void gnd_effect::MoveVehicle( f32 DeltaTime )
{
    // Setup vehicle bounding box
    bbox BBox( m_WorldBBox.GetCenter(), m_WorldBBox.GetRadius() * BBoxRadiusScale );
    BBox.Translate( vector3( 0.0f, m_ColBoxYOff, 0.0f ) );

    // Collision variables
    collider::collision Collision;
    collider Ext;
    Ext.ClearExcludeList();

    // Set initial movement vector
    vector3 Move = m_Vel * DeltaTime;
    
    // Exclude all vehicle occupants from collision testing
    for( s32 i=0; i < m_nSeats; i++ )
    {
        object* pOccupant = GetSeatPlayer( i );
        if( pOccupant )
            Ext.Exclude( pOccupant->GetObjectID().GetAsS32() );
    }

    //
    // Movement Loop
    //

    vector3 OldVel        = m_Vel;
    xbool   Bailout       = FALSE;
    f32     RemainingTime = DeltaTime;
    s32     MaxIterations = 5;

    collider::collision FirstCollision;
    FirstCollision.Collided = FALSE;

    do
    {
        // Clear collision
        Collision.Collided = FALSE;

        // Break out of the collision loop if we dont move enough
        if( Move.LengthSquared() < SQR( MovingEpsilon ) )
            break;
        
        // Setup the extrusion the direction of travel
        Ext.ExtSetup( this, BBox, Move );
        
        // Check for collisions with extrusion
        ObjMgr.Collide( ATTR_SOLID, Ext );
        
        if( Ext.HasCollided() )
        {
            Ext.GetCollision( Collision );
            
            object* pObjectHit = (object*)Collision.pObjectHit;
            ASSERT( pObjectHit );

            // Special case: for any objects that have a spike in their collision
            // geometry, we return a world up vector so we can slide off the tip
            if( pObjectHit->GetType() == object::TYPE_STATION_FIXED )
                Collision.Plane.Normal = GetSpikeNormal( Collision, BBox );

            if( pObjectHit->GetAttrBits() & (object::ATTR_VEHICLE | object::ATTR_PLAYER) )
            {
                // Vehicle has hit a player or another vehicle.
                // No need to do any collision response here!
                // Apply velocity changes and pain
                ObjMgr.ProcessImpact( this, Collision );
                FirstCollision = Collision;
                Bailout        = TRUE;
                
                // Move vehicle to point of impact
                vector3 Movement = Move * Collision.T;
                m_WorldPos   += Movement;
                BBox.Translate( Movement );
            }
            else
            {
                // Back off from the collision plane slightly to avoid floating point problems
                Collision.T -= BackOffAmount / Move.Length();
                Collision.T  = MAX( 0, Collision.T );

                // Move vehicle to point of impact
                vector3 Movement = Move * Collision.T;
                m_WorldPos  +=  Movement;
                BBox.Translate( Movement );

                // Check if we have hit a step and attempt to step over it
                if( pObjectHit->GetAttrBits() & ATTR_SOLID_STATIC )
                {
                    // No need to perform step checking on terrain
                    if( pObjectHit->GetType() != object::TYPE_TERRAIN )
                    {
                        if( StepVehicle( Collision, Move, BBox, Ext ) )
                        {
                            if( g_VehicleDebug.ShowCollision )
                            {
                                //x_printf  ( "Stepping!\n" );
                                //x_DebugMsg( "Stepping!\n" );
                            
                                if( s_GotStepInfo == FALSE )
                                {
                                    s_GotStepInfo   = TRUE;
                                    s_StepCollision = Collision;
                                    s_StepVel       = m_Vel;
                                    s_StepMove      = Move;
                                    s_StepBBox      = BBox;
                                }
                            }
                            continue;
                        }
                    }
                }
                
                // Apply velocity changes and pain
                ObjMgr.ProcessImpact( this, Collision );

                // Get the time left after the collision
                RemainingTime *= 1 - Collision.T;

                // Calculate next move
                Move = m_Vel * RemainingTime;
                
                // Direct the movement away from the collision plane
                Move += Collision.Plane.Normal * BackOffAmount;
                
                if( FirstCollision.Collided == FALSE )
                    FirstCollision = Collision;
            }
        }
        else
        {
            // No collision detected so we can just move the vehicle
            m_WorldPos += Move;
        }

        // To avoid an infinite collision loop when inside a crease,
        // we limit the number of iterations
        MaxIterations--;
        
    } while( (Bailout == FALSE) && Collision.Collided && MaxIterations );

    //if( MaxIterations == 0 )
    //{
    //    x_printfxy( 2, 2, "Max iterations" );
    //    m_WorldPos = StartPos;
    //    m_Vel.Zero();
    //}
    
    s_VehicleBBox = BBox;
    s_Collision   = Collision;

    if( FirstCollision.Collided )
    {
        vector3 DeltaVel = m_Vel - OldVel;
            
        // Play any impact sounds
        PlayCollisionAudio( FirstCollision, DeltaVel );
    }
}

//==============================================================================

f32 MinImpactVel = 3.0f;

void gnd_effect::PlayCollisionAudio( const collider::collision& Collision, const vector3& DeltaVel )
{
    veh_physics& Physics = *m_pPhysics;
    
    if( DeltaVel.Length() < MinImpactVel )
        return;
    
    f32     ImpactT     = DeltaVel.Length() / Physics.MaxForwardSpeed;
    vector3 ImpactPoint = Collision.Point;
    object* pObjectHit  = (object*)Collision.pObjectHit;

    // Play any vehicle impact sounds
    if( pObjectHit )
    {
        if( pObjectHit->GetType() == TYPE_TERRAIN )
        {
            if( ImpactT > HeavyImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_DEBRIS_03, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_04, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Heavy terrain collision!\n" );
            }
            else
            if( ImpactT > MediumImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_DEBRIS_02, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_02, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Medium terrain collision!\n" );
            }
            else
            if( ImpactT > LightImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_DEBRIS_01, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_01, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Light terrain collision!\n" );
            }
        }
        else
        {
            if( ImpactT > HeavyImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_IMPACT_04, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_04, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Heavy building collision!\n" );
            }
            else
            if( ImpactT > MediumImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_IMPACT_03, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_02, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Medium building collision!\n" );
            }
            else
            if( ImpactT > LightImpact )
            {
                audio_Play( SFX_VEHICLE_COLLISION_IMPACT_01, &ImpactPoint );
                if( x_rand() & 1 )
                    audio_Play( SFX_VEHICLE_COLLISION_BOOM_01, &ImpactPoint );
                
                if( g_VehicleDebug.ShowImpacts )
                    x_printf( "Light building collision!\n" );
            }
        }
    }
}                    

//==============================================================================

void gnd_effect::ApplyPhysics( f32 DeltaTime )
{
    veh_physics& Physics = *m_pPhysics;
    
    #ifdef jcossigny
    // Debugging aids
    if( tgl.UseFreeCam[0] )
    {
        // Let pad2 control the vehicle when pad1 is watching in free camera mode
        player_object* pPlayer = IsPlayerControlled();

        if( pPlayer )
        {
            m_Control.ThrustX = -input_GetValue ( INPUT_PS2_STICK_LEFT_X,  1 );
            m_Control.ThrustY =  input_GetValue ( INPUT_PS2_STICK_LEFT_Y,  1 );
            m_Control.Pitch   =  input_GetValue ( INPUT_PS2_STICK_RIGHT_Y, 1 );
            m_Control.Yaw     = -input_GetValue ( INPUT_PS2_STICK_RIGHT_X, 1 );
            m_Control.Roll    = -input_GetValue ( INPUT_PS2_STICK_RIGHT_X, 1 );
            m_Control.Boost   =  input_IsPressed( INPUT_PS2_BTN_R2,        1 ) ? 1.0f : 0.0f;
        }
    }
    #endif

    // Get ready for setting dirty bits
    BeginSetDirtyBits();

    vector3 DrawPos = m_WorldPos + (m_BlendMove.Blend * m_BlendMove.DeltaPos);
    radian3 DrawRot = m_Rot      + (m_BlendMove.Blend * m_BlendMove.DeltaRot);

    // Get the ground details
    collider::collision GroundCollision;
    GetGroundInfo( GroundCollision );

    // Create inputs to the physics system from controller input
    veh_input Input;
    SetInput( Input );

    // The world rotation of the vehicle
    radian3 World( R_0, R_0, R_0 );

    // Ground based vehicle special case    
    if( Physics.IsGroundBased )
    {
        vector3 Normal( 0.0f, 1.0f, 0.0f );
        object* pObjectHit = (object*)GroundCollision.pObjectHit;

        if( pObjectHit )
        {
            // Only orient to the ground rotation if its the right kind of ground!
            if( (pObjectHit->GetAttrBits() & ATTR_SOLID_STATIC) ||
                (pObjectHit->GetType()     & TYPE_VPAD) )
                Normal = GroundCollision.Plane.Normal;
        }
        
        // Get a world rotation for the vehicle based on the environment
        GetWorldRotation( Normal, World );

        // Scale down the thrust the further out of ground effect the vehicle gets
        Input.ForeThrust *= m_GroundEffect;
        Input.SideThrust *= m_GroundEffect;

        if( m_InGroundEffect == TRUE )
        {
            // Cut off the thrust if we are on too steep a surface
            if( GroundCollision.Plane.Normal.Y < x_sin( R_90 - ThrustLimitAngle ) )
            {
                Input.ForeThrust = 0.0f;
            }
        }
    }

    // Check if vehicle should be settled
    m_IsSettling = SettleVehicle( GroundCollision.Plane.Normal );

    // Calculate the final vehicle rotation
    ComputeAngular( Input, World, DeltaTime );

    // Calculate velocity vector
    ComputeLinear( Input, DeltaTime );

    vector3 OldPos( m_WorldPos );
    
    // Update the vehicle position whilst checking for collisions
    MoveVehicle( DeltaTime );
    
    // Compute vehicle speed
    m_Speed = (m_WorldPos - OldPos) / DeltaTime;
    //x_printfxy( 10, 3, "Speed = %1.2f\n", m_Speed.Length() );
    
    // Update bounding box
    m_WorldBBox = m_GeomBBox;
    m_WorldBBox.Translate( m_WorldPos );

    // Set dirty bits
    EndSetDirtyBits();

    // Truncate ready for sending over net
    Truncate();

    // If this is the server and the controlling player is not on the server, then blend!
    // If this is a client, and the controlled player is not on this client, 
    // blending happens in the receive update function.
    player_object* pPlayer = IsPlayerControlled();
    
    if( tgl.pServer && pPlayer && (!pPlayer->GetProcessInput()) )
    {
        // Is it valid to blend?
        if( (m_State == STATE_MOVING) &&
            ((m_WorldPos - DrawPos).LengthSquared() <= (MAX_BLEND_DIST * MAX_BLEND_DIST)) )
        {
            // Blend from last pos and rot
            m_BlendMove.DeltaPos = DrawPos - m_WorldPos;
            
            m_BlendMove.DeltaRot = DrawRot - m_Rot;
            m_BlendMove.DeltaRot.ModAngle2();
            
            m_BlendMove.Blend    = 1;     // Start from last pos
            m_BlendMove.BlendInc = -1.0f / MAX_BLEND_TIME;
        }
    }

    UpdateInstances();

    // Tell the object manager to update this object since we may be coming from a player update
    // in which case the object manager needs to be told the vehicle has moved!
    ObjMgr.UpdateObject( this );
}

//==============================================================================

void gnd_effect::OnAddVelocity( const vector3& Velocity )
{
    // Skip if immobile
    if ( m_IsImmobile )
        return ;

    // Keep last vel for dirty bit setting
    vector3 LastVel = m_Vel ;

    // Update vel
    m_Vel += Velocity;

    // Truncate ready for sending across net
    Truncate() ;

    // Has vel changed?
    if (LastVel != m_Vel)
        m_DirtyBits |= VEHICLE_DIRTY_BIT_POS;
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

void gnd_effect::OnPain( const pain& Pain )
{
    // Already dead?
    if( m_Health == 0.0f )
        return;

    if( g_VehicleDebug.NoDamage )
        return;

    // Vehicle must be invulnerable during spawn state
    if( m_State == STATE_SPAWN )
        return;
    
    /*    
    // Calculate force to apply to vehicle
    f32 Force = Pain.Force;
    Force /= m_pPhysics->Mass;

    // Move the vehicle?
    if( (Pain.LineOfSight) && (Force > 0.0f) && (!m_IsImmobile ) )
    {
        vector3 ForceDir = m_WorldBBox.GetCenter() - Pain.Center;
        ForceDir.Normalize();
        m_Vel += ForceDir * Force;
        m_DirtyBits |= VEHICLE_DIRTY_BIT_POS;

        // C'mon, Baby!  Let's do the twist!
        if( Pain.PainType != pain::MISC_FLAG_TO_VEHICLE )
        {
            // add rotational velocity if necessary
            // transform collision point to object space
            matrix4 tmp;
            tmp.SetTranslation( m_WorldBBox.GetCenter() );
            tmp.SetRotation( m_Rot );
            vector3 DamPt;
            tmp.InvertSRT();
            DamPt = (tmp * Pain.Center);
            
            radian3 tmprot;

            // pitch
            tmprot.Pitch = (DamPt.Y * DamPt.Z) * 0.001f;     // tweak value, temp

            // yaw
            tmprot.Yaw = DamPt.X * DamPt.Z * 0.001f;

            // roll
            tmprot.Roll = DamPt.X * DamPt.Y * 0.001f;

            //m_RotVel += tmprot;   

            m_DirtyBits |= VEHICLE_DIRTY_BIT_ROT;
        }
    }
    */

    {
        // Damage (or repair) the vehicle?
        if( ((Pain.LineOfSight) || (Pain.PainType == pain::MISC_FLAG_TO_VEHICLE)) && 
            ((Pain.MetalDamage != 0.0f) || (Pain.EnergyDamage != 0.0f)) )
        {
            // Update health and energy.
            m_Health -= Pain.MetalDamage  * 100.0f;
            m_Energy -= Pain.EnergyDamage * 100.0f;

            f32 MinH = fegl.PlayerInvulnerable ? 20.0f : 0.0f;

            m_Energy = MINMAX( 0.0f, m_Energy, 100.0f );
            m_Health = MINMAX( MinH, m_Health, 100.0f );

            m_DirtyBits |= VEHICLE_DIRTY_BIT_HEALTH;
            m_DirtyBits |= VEHICLE_DIRTY_BIT_THRUST_ENERGY;
        }

        // Show a shield effect?
        if( (m_Energy > 0.0f) && (Pain.EnergyDamage > 0.0f) )
        {
            CreateShieldEffect( m_ObjectID, m_Energy / 100.0f );
        }
    }

    // Keep accels and vels in limits for sending over the net...
    Truncate();

    if( (m_Health == 0.0f) && (tgl.pServer) )
    {
        Explode( Pain );
        pGameLogic->ItemDestroyed( m_ObjectID, Pain.OriginID );
    }
}

//==============================================================================

void vehicle_RenderSpawnEffect( material&            Material, 
                                texture_draw_info&   TexDrawInfo,
                                material_draw_info&  DrawInfo,
                                s32&                 CurrentLoadedNode )
{
    // Static vars for material
    static xbool    InitSpawnMaterial = FALSE ;
    static material SpawnMaterial ;

    // Setup shield material if not yet used
    if (!InitSpawnMaterial)
    {
        // Get shield
        shape* ShieldShape = tgl.GameShapeManager.GetShapeByType(SHAPE_TYPE_MISC_SHIELD) ;
        ASSERT(ShieldShape) ;

        // Get shield model
        ASSERT(ShieldShape->GetModelCount()) ;
        model& ShieldModel = ShieldShape->GetModel(0) ;
        
        // Get shield material
        ASSERT(ShieldModel.GetMaterialCount()) ;
        material& ShieldMaterial = ShieldModel.GetMaterial(0) ;

        // Copy material and set what we want
        SpawnMaterial = ShieldMaterial ;
        SpawnMaterial.SetAdditiveFlag(TRUE) ;
        SpawnMaterial.SetColor(vector4(0,0,0, 1.0f)) ; // Essentially turns off the diffuse

        // No need to initialize anymore
        InitSpawnMaterial = TRUE ;
    }

    // Get vehicle this effect is attached too
    gnd_effect* Vehicle = (gnd_effect*)DrawInfo.RenderMaterialParam ;
    ASSERT(Vehicle) ;
    ASSERT(Vehicle->GetAttrBits() & object::ATTR_VEHICLE) ;

    // Setup current material and draw it
    Material.Activate( DrawInfo, TexDrawInfo, CurrentLoadedNode) ;
    Material.Draw( DrawInfo, CurrentLoadedNode ) ;

    // Copy the draw info so we can mess with it without destroying the original
    material_draw_info SpawnDrawInfo = DrawInfo ;
    SpawnDrawInfo.Flags |= shape::DRAW_FLAG_REF_RANDOM ;
    SpawnDrawInfo.Color = vector4(0,0,0, 0.5f) ; // make shield 50%

    // Now setup spawn material, but draw with the other materials display lists
    SpawnMaterial.Activate( SpawnDrawInfo, CurrentLoadedNode ) ;
    Material.Draw( SpawnDrawInfo, CurrentLoadedNode ) ;

    // Must restore original material
    Material.Activate( DrawInfo, TexDrawInfo, CurrentLoadedNode) ;
}
                                
//==============================================================================

void GetBBoxVerts( const bbox& BBox, vector3* pVert )
{
    // Compute edge vectors
    vector3 X( BBox.Max.X - BBox.Min.X, 0.0f, 0.0f );
    vector3 Y( 0.0f, BBox.Max.Y - BBox.Min.Y, 0.0f );

    // Find the corner vertices of bounding box
    pVert[0] = BBox.Min;
    pVert[1] = pVert[0] + X;
    pVert[2] = pVert[1] + Y;
    pVert[3] = pVert[0] + Y;
    pVert[4] = BBox.Max;
    pVert[5] = pVert[4] - Y;
    pVert[6] = pVert[5] - X;
    pVert[7] = pVert[4] - X;
}

//==============================================================================

void gnd_effect::Render( void )
{
#if 0
    // Set material render effects function
    switch(m_State)
    {
        case STATE_SPAWN:
            m_RenderMaterialFunc = vehicle_RenderSpawnEffect ;
            break ;
            
        default:
            m_RenderMaterialFunc = NULL ;
            break ;
    }
#endif

    // Call base class
    vehicle::Render() ;

    // Render debugging stuff
    if( g_VehicleDebug.DrawBBox )
        draw_BBox( s_VehicleBBox, XCOLOR_YELLOW );

    if( g_VehicleDebug.DrawAxis )
    {
        const f32 AxisLength = 4.0f;
        
        vector3 LocalX( 1.0f, 0.0f, 0.0f );
        vector3 LocalY( 0.0f, 1.0f, 0.0f );
        vector3 LocalZ( 0.0f, 0.0f, 1.0f );
        
        LocalX.Rotate( m_Rot );
        LocalY.Rotate( m_Rot );
        LocalZ.Rotate( m_Rot );
        
        draw_Begin( DRAW_LINES, 0 );

        draw_Color( XCOLOR_RED );
        draw_Vertex( m_WorldPos );
        draw_Vertex( m_WorldPos + (LocalX * AxisLength) );
    
        draw_Color( XCOLOR_GREEN );
        draw_Vertex( m_WorldPos );
        draw_Vertex( m_WorldPos + (LocalY * AxisLength) );
    
        draw_Color( XCOLOR_BLUE );
        draw_Vertex( m_WorldPos );
        draw_Vertex( m_WorldPos + (LocalZ * AxisLength) );
        
        draw_Color( XCOLOR_YELLOW );
        draw_Vertex( m_WorldPos );
        draw_Vertex( m_WorldPos + (s_Vector * AxisLength) );
        
        draw_End();
    }
    
    if( g_VehicleDebug.ShowCollision )
    {
        vector3 P0( s_Collision.Point );
        vector3 P1( P0 + (s_Collision.Plane.Normal * 2.0f) );
    
        draw_Point( s_Collision.Point, XCOLOR_RED );
        draw_Line( P0, P1, XCOLOR_GREEN );
    
        if( s_GotStepInfo )
        {
            draw_BBox ( s_StepBBox, XCOLOR_YELLOW );
            draw_Point( s_StepCollision.Point, XCOLOR_GREEN );
            draw_Point( s_StepCollision.Point + vector3( 0.0f, s_StepDeltaHeight, 0.0f ), XCOLOR_RED );
    
            vector3 Verts[8];
            vector3 V0 = s_StepMove;
            vector3 V1 = s_StepVel - V0;
            
            GetBBoxVerts( s_StepBBox, Verts );
            
            draw_Line( Verts[0],      Verts[0] + V0, XCOLOR_GREEN );
            draw_Line( Verts[1],      Verts[1] + V0, XCOLOR_GREEN );
            draw_Line( Verts[2],      Verts[2] + V0, XCOLOR_GREEN );
            draw_Line( Verts[3],      Verts[3] + V0, XCOLOR_GREEN );
            draw_Line( Verts[4],      Verts[4] + V0, XCOLOR_GREEN );
            draw_Line( Verts[5],      Verts[5] + V0, XCOLOR_GREEN );
            draw_Line( Verts[6],      Verts[6] + V0, XCOLOR_GREEN );
            draw_Line( Verts[7],      Verts[7] + V0, XCOLOR_GREEN );
        
            draw_Line( Verts[0] + V0, Verts[0] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[1] + V0, Verts[1] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[2] + V0, Verts[2] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[3] + V0, Verts[3] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[4] + V0, Verts[4] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[5] + V0, Verts[5] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[6] + V0, Verts[6] + V1, XCOLOR_YELLOW );
            draw_Line( Verts[7] + V0, Verts[7] + V1, XCOLOR_YELLOW );
        }
    }
}

//==============================================================================

void gnd_effect::RenderParticles( void )
{
}

//==============================================================================

void gnd_effect::DebugBitStream( f32 DeltaTime )
{
    if( tgl.ServerPresent )
    {
        u32         DirtyBits ;
        f32         Priority ;
        bitstream   BitStream ;
        s32         x=0,y=4, Bits, Bytes ;
    
        // Get controlling player (if any)
        player_object* pPlayer = IsPlayerControlled() ;
    
        x_printfxy(x,y++, "LogicDeltaTime     :%f ", DeltaTime) ;
    
        y++ ;
        if (pPlayer)
            DirtyBits = pPlayer->GetDirtyBits() ;
        else
            DirtyBits = m_DirtyBits ;
        Priority  = 1 ;
        BitStream.SetCursor(0) ;
        WriteDirtyBitsData( BitStream, DirtyBits, Priority, FALSE ) ;
        Bits = BitStream.GetCursor() ;
        Bytes = (Bits+7)/8 ;
        x_printfxy(x,y++, "REG Update:Pri0 Bits:%d Bytes:%d", Bits, Bytes) ;
        ASSERT((DirtyBits & PLAYER_DIRTY_BIT_VEHICLE_USER_BITS) == 0) ;
    
        // NOTE: This used to be 0xFFFFFFFF but it seems bit 31 (VEHICLE_DIRTY_BIT_FIRE) is not used anymore.
        // To avoid hitting the ASSERT when printing the max, we dont set bit 31.
        DirtyBits = 0x7FFFFFFF ;
        Priority  = 1 ;
        BitStream.SetCursor(0) ;
        WriteDirtyBitsData( BitStream, DirtyBits, Priority, FALSE ) ;
        Bits = BitStream.GetCursor() ;
        Bytes = (Bits+7)/8 ;
        x_printfxy(x,y++, "MAX Update:Pri0 Bits:%d Bytes:%d", Bits, Bytes) ;
        ASSERT((DirtyBits & PLAYER_DIRTY_BIT_VEHICLE_USER_BITS) == 0) ;
    
        y++ ;
        if (pPlayer)
            DirtyBits = pPlayer->GetDirtyBits() ;
        else
            DirtyBits = m_DirtyBits ;
        Priority  = 1 ;
        BitStream.SetCursor(0) ;
        WriteDirtyBitsData( BitStream, DirtyBits, Priority, TRUE ) ;
        Bits = BitStream.GetCursor() ;
        Bytes = (Bits+7)/8 ;
        x_printfxy(x,y++, "REG MoveUpdate:Pri0 Bits:%d Bytes:%d", Bits, Bytes) ;
        ASSERT((DirtyBits & PLAYER_DIRTY_BIT_VEHICLE_USER_BITS) == 0) ;
    
        DirtyBits = 0x7FFFFFFF ;
        Priority  = 1 ;
        BitStream.SetCursor(0) ;
        WriteDirtyBitsData( BitStream, DirtyBits, Priority, TRUE ) ;
        Bits = BitStream.GetCursor() ;
        Bytes = (Bits+7)/8 ;
        x_printfxy(x,y++, "MAX MoveUpdate:Pri0 Bits:%d Bytes:%d", Bits, Bytes) ;
        ASSERT((DirtyBits & PLAYER_DIRTY_BIT_VEHICLE_USER_BITS) == 0) ;
    }
}
    
//==============================================================================

void gnd_effect::UpdateAssets( void )
{
}

//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================

/*
f32 DistToPlane( const plane& Plane, const vector3& S, const vector3& E )
{
    f32 T = 0;

    if( Plane.Intersect( T, S, E ) )
    {
        return( ABS( T ) );
    }
    else
    {
        // Ray doesnt intersect plane so return a huge value
        return( 999999.0f );
    }
}

vector3 GetBBoxEdgePoint( const bbox& BBox, const vector3& Heading )
{
    vector3 Center = BBox.GetCenter();
    vector3 S( Center );
    vector3 E( Center + Heading );
    
    f32 PZ = DistToPlane( plane( vector3(  0.0f,  0.0f, -1.0f ), BBox.Max.Z ), S, E );
    f32 NZ = DistToPlane( plane( vector3(  0.0f,  0.0f,  1.0f ), BBox.Min.Z ), S, E );

    f32 PX = DistToPlane( plane( vector3( -1.0f,  0.0f,  0.0f ), BBox.Max.X ), S, E );
    f32 NX = DistToPlane( plane( vector3(  1.0f,  0.0f,  0.0f ), BBox.Min.X ), S, E );

    f32 PY = DistToPlane( plane( vector3(  0.0f, -1.0f,  0.0f ), BBox.Max.Y ), S, E );
    f32 NY = DistToPlane( plane( vector3(  0.0f,  1.0f,  0.0f ), BBox.Min.Y ), S, E );

    f32 MinD = MIN( MIN( MIN( MIN( MIN( PZ, NZ ), PX ), NX ), PY ), NY );

    if( MinD > 99999.0f )
        return( Center );

    return( Center + (Heading * MinD) );
}
*/



//==============================================================================


