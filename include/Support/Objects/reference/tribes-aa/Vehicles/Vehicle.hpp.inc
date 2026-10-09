//==============================================================================
//
//  Vehicle.hpp
//
//==============================================================================

#ifndef __VEHICLE_HPP__
#define __VEHICLE_HPP__

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "Objects/Player/PlayerObject.hpp"

//==============================================================================
//  DIRTY BITS
//==============================================================================

// Dirty bits
#define VEHICLE_DIRTY_BIT_POS					PLAYER_DIRTY_BIT_VEHICLE_USER_BIT0
#define VEHICLE_DIRTY_BIT_ROT					PLAYER_DIRTY_BIT_VEHICLE_USER_BIT1
#define VEHICLE_DIRTY_BIT_BOOST					PLAYER_DIRTY_BIT_VEHICLE_USER_BIT2
#define VEHICLE_DIRTY_BIT_THRUST_ENERGY			PLAYER_DIRTY_BIT_VEHICLE_USER_BIT3
#define VEHICLE_DIRTY_BIT_STATE					PLAYER_DIRTY_BIT_VEHICLE_USER_BIT4
#define VEHICLE_DIRTY_BIT_SEAT_STATUS			PLAYER_DIRTY_BIT_VEHICLE_USER_BIT5
#define VEHICLE_DIRTY_BIT_HEALTH				PLAYER_DIRTY_BIT_VEHICLE_USER_BIT6
#define VEHICLE_DIRTY_BIT_FIRE					PLAYER_DIRTY_BIT_VEHICLE_USER_BIT7


// Bits that are used to initialize the vehicle
#define VEHICLE_DIRTY_BIT_INIT					    \
        (										    \
            VEHICLE_DIRTY_BIT_POS				|   \
            VEHICLE_DIRTY_BIT_ROT				|   \
            VEHICLE_DIRTY_BIT_BOOST				|   \
            VEHICLE_DIRTY_BIT_THRUST_ENERGY		|   \
            VEHICLE_DIRTY_BIT_STATE				|   \
            VEHICLE_DIRTY_BIT_SEAT_STATUS		|   \
            VEHICLE_DIRTY_BIT_HEALTH			    \
        )


//==============================================================================
//  TYPES
//==============================================================================

class vehicle : public object
{
// Defines
public:

        // List of seat types that player can be in
        enum seat_type
        {
            SEAT_TYPE_START=0,

            SEAT_TYPE_PILOT=SEAT_TYPE_START,    // Drives the vehicle
//          SEAT_TYPE_BOMBER,                   // Drops bombs
//          SEAT_TYPE_GUNNER,                   // Controls a turret
            SEAT_TYPE_PASSENGER,                // Just there for the ride

            SEAT_TYPE_END = SEAT_TYPE_PASSENGER,
            SEAT_TYPE_TOTAL
        } ;

        enum vehicle_type
        {
            TYPE_UNDEFINED = 0,
            TYPE_WILDCAT,
            TYPE_SHRIKE,
            TYPE_BEOWULF,
            TYPE_THUNDERSWORD,
            TYPE_HAVOC,
            TYPE_JERICHO
        };

        // Defines
        enum misc
        {
            MAX_SEATS = 10,     // Max # of passengers in any vehicle
            MAX_VELS  = 20      // # of velocities to take average over
        } ;


// Structures
public:
        // Simple seat structure used to keep track of people 
        struct seat
        {
            // Static data
            seat_type   Type ;              // Type of seat
            hot_point*  HotPoint ;          // Vehicle hot point of seat
            xbool       PlayerTypes[player_object::ARMOR_TYPE_TOTAL] ;   // Player types that can sit here
            s32         PlayerAnimType ;    // Player anim type to use
            xbool       CanLookAndShoot ;   // TRUE if player can look and shoot
            bbox        LocalBBox ;         // BBox around hot point

            // Dynamic data
            object::id  PlayerID ;          // Object ID of occupying player
        } ;

private:

    // Structure used for blending
    struct blend_move
    {
        vector3     DeltaPos ;              // Pos to start from
        radian3     DeltaRot ;              // Rot to start from
        f32         DeltaSpeed;             // speed
        
        f32         Blend ;                 // Blend (0=all blend pos, 1=all world pos)
        f32         BlendInc ;              // Speed of blend

        // Initializes all members
        void Clear ( void )
        {
            DeltaPos.Zero() ;
            DeltaRot.Zero() ;

            Blend    = 0 ;
            BlendInc = 0 ;
        }
    } ;


// Functions
public:

                        vehicle             ( void );

        // Initialization functions
virtual void            Initialize          ( const s32      GeomShapeType,     // Shape to user for geometry
                                              const s32      CollShapeType,     // Shape to use for collision
                                              const vector3& Pos,               // Initial position
                                              const radian   InitHdg,           // Initial heading
                                              const f32      SeatRadius,        // Seat radius 
                                                    u32      TeamBits );        

void                    SetOwner            ( object::id     OwnerID )          { m_Owner = OwnerID; }

        // Misc functions

        f32             GetHealth           ( void ) const;
        f32             GetEnergy           ( void ) const;
        xbool           IsShielded          ( void ) const { return( TRUE ); }
        const xwchar*   GetLabel            ( void ) const;

        // Logic functions
        void            ComputeAverageVelocity( void ) ;
        vector3         GetAverageVelocity    ( void ) const { return m_AverageVelocity ; }

        // Render functions                 
virtual void            UpdateInstances     ( void ) ;
                                            
virtual void            Render              ( void ) ;
virtual void            DebugRender         ( void ) ;

virtual void            Get3rdPersonView    ( s32 SeatIndex, vector3& Pos, radian3& Rot );
virtual void            Get1stPersonView    ( s32 SeatIndex, vector3& Pos, radian3& Rot );

        // Seat functions
virtual void            SetupSeat           ( s32 Index, seat& Seat ) ; // Override for your vehicle!
        void            SetupSeats          ( f32 SeatRadius ) ;
        const seat&     GetSeat             ( s32 Index ) const ;
        vector3         GetSeatPos          ( s32 Index ) ;
virtual void            FreeSeat            ( s32 Index ) ;
        player_object*  GetSeatPlayer       ( s32 Index ) ;
        player_object*  IsPlayerControlled  ( void ) ;
        s32             GetNumberOfSeats    ( void ) const          { return m_nSeats ; }
        void            SetAllowEntry       ( xbool State );

        // Collision functions
virtual void            OnCollide           ( u32 AttrBits, collider& Collider ) ;
virtual s32             HitByPlayer         ( player_object* pPlayer ) ;
virtual matrix4*        GetNodeL2W          ( s32 ID, s32 NodeIndex ) ;                                            
                 
        // Movement functions               
virtual vector3         GetBlendPos         ( void ) const          { return m_DrawPos ; }          
virtual radian3         GetBlendRot         ( void ) const          { return m_DrawRot ; }
virtual radian3         GetRotation         ( void ) const          { return m_Rot ; }
virtual vector3         GetVelocity         ( void ) const          { return m_Vel ; }

        const radian3&  GetRot              ( void ) ;
        const radian3&  GetTurretRot        ( void ) ;
        void            SetTurretRot        ( const radian3& Rot ) ;

virtual f32             GetTurretSpeedScaler( void ) ;
        radian          GetTurretMinPitch   ( void ) ;
        radian          GetTurretMaxPitch   ( void ) ;
        xbool           ObjectInWayOfCollision( const bbox& WorldBBox ) ;
        void            ReturnTurretToRest  ( f32 DeltaTime ) ;

        const vector3&  GetDrawPos          ( void ) ;
        const radian3&  GetDrawRot          ( void ) ;
        const radian3&  GetDrawTurretRot    ( void ) ;
        
        const vector3&  GetVel              ( void ) { return m_Vel; }
        void            SetVel              ( const vector3& Vel );

        void            AddWeaponEnergy     ( f32 Amount );
        f32             GetWeaponEnergy     ( void );

        // Net functions
        void            TruncateRangedF32   ( f32& Value, s32 NBits, f32 Min, f32 Max ) ;
        void            WriteRangedF32      ( bitstream& BitStream, f32 Value, s32 NBits, f32 Min, f32 Max ) ;
        void            ReadRangedF32       ( const bitstream& BitStream, f32& Value, s32 NBits, f32 Min, f32 Max ) ;

        // sensor net functions
        void            ClearSensedBits     ( void )            { m_SensedBits  = 0x00; }
        void            AddSensedBits       ( u32 SensedBits )  { m_SensedBits |= SensedBits; }
        u32             GetSensedBits       ( void ) const      { return( m_SensedBits ); }

        // Missile track/lock status
virtual void            MissileTrack        ( void );
virtual void            MissileLock         ( void );

        // Virtual functions that must be implemented by any inheriting classes
virtual void            WriteDirtyBitsData  ( bitstream& BitStream, u32& DirtyBits, f32 Priority, xbool MoveUpdate ) = 0 ;
virtual xbool           ReadDirtyBitsData   ( const bitstream& BitStream, f32 SecInPast ) = 0 ;
virtual void            ApplyMove           ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move ) = 0 ;
virtual void            ApplyPhysics        ( f32 DeltaTime ) = 0 ;

        void            CheckDestroyConditions  ( f32 DeltaTime );

        xbool           HasEnemyOccupant    ( u32 TeamBits );

// Data
protected:

        // State vars
        f32                 m_Health ;                  // Current health
        f32                 m_Energy ;                  // Current energy
        f32                 m_WeaponEnergy;             // Energy req'd for firing (0.0 - 1.0)
        vector3             m_Vel;                      // velocity vector
        vector3             m_Speed;                    // actual speed (actual movement of vehicle in 1 sec) with blending
        xwchar              m_Label[32];                // the label

        // Average velocity vars
        s32                 m_VelocityID;                       // Current slot in velocity table
        vector3             m_AverageVelocity;                  // The computed average velocity
        vector3             m_Velocities[ MAX_VELS ] ;    // Table of previous velocities (circular list)
        
        // Seating vars
        seat                m_Seats[MAX_SEATS] ;        // Current seat information
        s32                 m_nSeats ;                  // Total number of seats
        s32                 m_nSeatsFree ;              // Total seats un-occupied   
        xbool               m_AllowEntry;               // Allow players to enter vehicle (only after spawning complete)
        
        // Instance vars
        shape_instance      m_GeomInstance;             // The vehicle geometry shape instance
        shape_instance      m_CollInstance ;            // The simplified collision shape instance
        bbox                m_GeomBBox ;                // Local bounding box around geometry
        f32                 m_ColBoxYOff;               // Collision Box Y offset

        // Render vars
        radian3             m_Rot ;                     // Current rotation
        xbool               m_bHasTurret ;              // TRUE if vehicle has a turret onboard
        radian3             m_TurretRot ;               // Current turret rotation
        blend_move          m_BlendMove ;               // Offset to blend drawing
        vector3             m_DrawPos ;                 // Current position vehicle is drawn at
        radian3             m_DrawRot ;                 // Current rotation vehicle is drawn at
        f32                 m_DrawSpeed ;

        s32                 m_IsVisible ;               // Can the camera see the vehicle?
        u32                 m_DrawFlags ;               // Draw flags to use

        texture*            m_DamageTexture ;           // Damage texture to use (or NULL)
        s32                 m_DamageClutHandle ;        // Damage clut to use (or -1)
        render_material_fn* m_RenderMaterialFunc ;      // Special fx render material (or NULL)

        // Turret definition vars
        f32                 m_TurretSpeedScaler ;       // Controls input speed
        radian              m_TurretMinPitch ;          // Min pitch of turret
        radian              m_TurretMaxPitch ;          // Max pitch of turret

        // Audio interpolation vars
        f32                 m_CurVol;                   // the current volume
        f32                 m_TargetVol;                // the target volume for the sound
        f32                 m_RampRate;                 // the rate at which the gap is closed (units/sec)

        // Firing  vars
        f32                 m_Fire1Delay ;              // Delay before firing/bombing
        f32                 m_Fire2Delay ;              // Delay before firing/bombing

        // Sensor net vars
        u32                 m_SensedBits;               // TeamBits of enemies which sense player

        // Vehicle type
        vehicle_type        m_VehicleType;

        // Vehicle "owner"
        object::id          m_Owner;

        // Vehicle Timer
        f32                 m_CheckTime;                // is it time to check again?
        f32                 m_TrueTime;                 // how long has the condition been TRUE?
};


//==============================================================================
#endif // __VEHICLE_HPP__
//==============================================================================
