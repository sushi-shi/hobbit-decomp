//==============================================================================
//
//  VehiclePad.hpp
//
//==============================================================================

#ifndef VEHICLEPAD_HPP
#define VEHICLEPAD_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "Objects\Player\PlayerObject.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class station;

class vehicle_pad : public object
{

//------------------------------------------------------------------------------
//  Public Types

public:

    enum vpad_code
    {
        VPAD_OK,
        VPAD_BUSY,
        VPAD_BLOCKED,
        VPAD_VEHICLE_NOT_AVAILABLE,
    };

//------------------------------------------------------------------------------
//  Public Functions

public:

                    vehicle_pad     ( void );

        update_code OnAdvanceLogic  ( f32 DeltaTime );
        update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
        void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
        void        OnCollide       ( u32 AttrBits, collider& Collider );
        void        UnbindTargetID  ( object::id TargetID );

        void        OnRemove        ( void );    

        void        Initialize      ( const vector3& WorldPos,
                                      const radian3& WorldRot,
                                            s32      Switch,
                                      const xwchar*  pLabel );
        
        void        DebugRender     ( void );
        void        Render          ( void );
        void        RenderSpawnFX   ( void );

        void        Alert           ( void );
        void        BeginSequence   ( void );
        vpad_code   CreateVehicle   ( object::type VehicleType, object::id PlayerID );

const   xwchar*     GetLabel        ( void ) const;

//------------------------------------------------------------------------------
//  Private Types

protected:

    enum state { IDLE, BUSY, };

//------------------------------------------------------------------------------
//  Private Data

protected:

    state           m_State;
    f32             m_Timer;
    s32             m_Switch;
    s32             m_Handle;           // Audio handle for streaming sound.
    object::type    m_VehicleType;      // Type of vehicle being created.
    object::id      m_PlayerID;
    radian3         m_WorldRot;
    xwchar          m_Label[32];
    shape_instance  m_Shape; 
    shape_instance  m_Collision;

//------------------------------------------------------------------------------
//  Private Functions

protected:

        void        CreateVehicle   ( void );
        void        PowerOn         ( void );
        void        PowerOff        ( void );

friend station;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     VehiclePadCreator( void );

//==============================================================================
#endif // VEHICLEPAD_HPP
//==============================================================================
