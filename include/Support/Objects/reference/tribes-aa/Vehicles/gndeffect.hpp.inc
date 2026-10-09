//==============================================================================
//
//  GndEffect.hpp
//  Base class for ground effect vehicles
//
//==============================================================================

#ifndef GNDEFFECT_HPP
#define GNDEFFECT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "Objects/Player/PlayerObject.hpp"
#include "Vehicle.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================

struct veh_physics
{
    xbool   IsGroundBased;

    // Linear Motion
    f32     MaxForwardSpeed;
    f32     LinearAccel;
    f32     LinearDrag;
    f32     Gravity;

    // Angular Limits
    radian  MaxPitch;
    radian  MaxRoll;

    // Angular Motion for Local Rotations
    radian  AccelPitch;
    radian  AccelYaw;
    radian  AccelRoll;
    f32     LocalDrag;
    
    // Angular Motion for World Rotations
    radian  WorldAccel;
    f32     WorldDrag;
    
    // Collision Constants
    f32     Mass;
    f32     PerpScale;
    f32     ParScale;
    f32     DamageScale;

    // Rates per vehicle type
    f32     EnergyRecharge;           // percent per second
    f32     BoosterConsume;           // percent per second
    f32     WeaponRecharge;           // percent per second
    f32     WeaponConsume;            // percent per shot
    f32     WeaponDelay;              // seconds
};

struct veh_control
{
    f32     ThrustX;
    f32     ThrustY;
    f32     Pitch;
    f32     Yaw;
    f32     Roll;
    f32     Boost;
};

struct veh_input
{
    f32     SideThrust;
    f32     ForeThrust;
    f32     Pitch;
    f32     Yaw;
    f32     Roll;
    f32     Boost;
};

//==============================================================================

class gnd_effect : public vehicle
{
// Defines
public:
        // List of states
        enum state
        {
            STATE_START=0,
            
            STATE_SPAWN = STATE_START,  // Fading in on platform
            STATE_MOVING,               // Moving around
            
            STATE_END = STATE_MOVING,
            STATE_TOTAL
        } ;


// Functions
public:

        void            EjectPlayers        ( void ) ;
        void            Explode             ( const pain& Pain );
        update_code     AdvanceLogic        ( f32 DeltaTime );
                                        
virtual update_code     OnAdvanceLogic      ( f32 DeltaTime );

virtual void            BeginSetDirtyBits   ( void ) ;
virtual void            EndSetDirtyBits     ( void ) ;
virtual void            Truncate            ( void ) ;
virtual void            WriteDirtyBitsData  ( bitstream& BitStream, u32& DirtyBits, f32 Priority, xbool MoveUpdate ) ;
virtual xbool           ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast ) ;
        
        update_code     OnAcceptUpdate      ( const bitstream& BitStream, f32 SecInPast );
        void            OnProvideUpdate     (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
                                            
virtual void            OnAcceptInit        ( const bitstream& BitStream );
        void            OnProvideInit       (       bitstream& BitStream );
virtual void            OnAdd               ( void );
virtual void            OnRemove            ( void );
                                            
virtual void            OnAddVelocity       ( const vector3& Velocity );
                                            
virtual void            Initialize          ( const s32      GeomShapeType,    // Shape to user for geometry
                                              const s32      CollShapeType,    // Shape to use for collision
                                              const vector3& Pos,              // Initial position
                                              const radian   InitHdg,          // Initial heading
                                              const f32      SeatRadius,       // Radius of seats
                                                    u32      TeamBits );

virtual void            Initialize          ( const vector3& Pos, radian InitHdg, u32 TeamBits );


virtual void            ApplyMove           ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move ) ;
                                            
virtual void            ApplyPhysics        ( f32 DeltaTime ) ;

virtual void            Render              ( void ) ;

virtual void            RenderParticles     ( void ) ;
                                            
        void            OnPain              ( const pain& Pain ); 

virtual update_code     Impact              ( vector3& Pos, vector3& Norm ) = 0;
                                            
virtual f32             GetMass             ( void ) const { return m_pPhysics->Mass;  }
        
        f32             GetStateTime        ( void ) const { return m_StateTime ;  }

        void            InitPhysics         ( veh_physics* pPhysics );
        void            SetInput            ( veh_input& Input );
        
        void            GetGroundInfo       ( collider::collision& Collision );
        void            PlayCollisionAudio  ( const collider::collision& Collision, const vector3& DeltaVel );
        
        void            GetWorldRotation    ( const vector3&     GroundNormal,
                                              radian3&           World );
        
        void            ComputeAngular      ( const veh_input&   Input,
                                              const radian3&     World,
                                              f32                DeltaTime );
        
        void            ComputeLinear       ( const veh_input&   Input,
                                              f32                DeltaTime );
        
        xbool           SettleVehicle       ( const vector3&     GroundNormal );
        xbool           IsSettled           ( void ) { return( m_IsSettling ); }
        
        void            MoveVehicle         ( f32 DeltaTime );
        
        xbool           StepVehicle         ( const collider::collision& Collision,
                                              const vector3&             Move,
                                                    bbox&                BBox,
                                                    collider&            Collider );
        
        void            DebugBitStream      ( f32 DeltaTime );
        
        xbool           IsGroundBased       ( void ) { return( m_pPhysics->IsGroundBased ); }

virtual void            UpdateAssets        ( void );                                              

//==========================================================================
// Static data
//==========================================================================
public:

    // Static data
    static vector3    s_LastWorldPos;
    static vector3    s_LastVel;
    static radian3    s_LastWorldRot;
    static quaternion s_LastWorldRotVel;
    static radian3    s_LastLocalRot;
    static radian3    s_LastLocalRotVel;
    static f32        s_LastBoost;
    static vector2    s_LastThrust;
    static f32        s_LastEnergy;
    static f32        s_LastWeaponEnergy;
    static state      s_LastState;
    static f32        s_LastStateTime;
    static f32        s_LastHealth;

//==========================================================================
// Data
//==========================================================================
protected:

    // State vars
    state               m_State;                    // Current state
    f32                 m_StateTime;                // Time in current state

    veh_physics*        m_pPhysics;                 // pointer to vehicle physics constants

    veh_control         m_Control;                  // controller input

    f32                 m_GroundEffect;             // Parametric value for ground effect - 1 is on ground
    
    xbool               m_OnGround;                 // if vehicle is physically on ground
    xbool               m_InGroundEffect;           // if vehicle is in ground effect
    xbool               m_IsSettling;               // if vehicle should settle
    xbool               m_IsImmobile;               // only used by MPB currently, but any could use it

    radian3             m_WorldRot;                 // world rotation (for ground based vehicles only)
    quaternion          m_WorldRotVel;              // world rotational velocity (for ground based vehicles only)
    radian3             m_LocalRot;                 // any rotations local to vehicle
    radian3             m_LocalRotVel;              // rotational velocity in local space of vehicle

    // effect related
    vector3             m_BubbleScale;              // size of bubble for effects
};

//==============================================================================
#endif // GNDEFFECT_HPP
//==============================================================================
    