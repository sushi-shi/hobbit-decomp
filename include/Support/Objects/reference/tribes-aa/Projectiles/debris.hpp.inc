//==============================================================================
//
//  Debris.hpp
//
//==============================================================================

#ifndef DEBRIS_HPP
#define DEBRIS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "Arcing.hpp"
#include "Particles/ParticleEffect.hpp"
#include "../vehicles/vehicle.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class debris : public arcing
{

public:

    enum chunk_type
    {
        BIG,
        MEDIUM,
        SMALL,
        VEHICLE_GRAVCYCLE1,
        VEHICLE_GRAVCYCLE2,
        VEHICLE_GRAVCYCLE3,
        VEHICLE_GRAVCYCLE4,
        VEHICLE_GRAVCYCLE5,
        VEHICLE_GRAVCYCLE6,
        VEHICLE_SHRIKE1,
        VEHICLE_SHRIKE2,
        VEHICLE_SHRIKE3,
        VEHICLE_SHRIKE4,
        VEHICLE_SHRIKE5,
        VEHICLE_SHRIKE6,
        VEHICLE_TANK1,
        VEHICLE_TANK2,
        VEHICLE_TANK3,
        VEHICLE_TANK4,
        VEHICLE_TANK5,
        VEHICLE_TANK6,
        VEHICLE_BOMBER1,
        VEHICLE_BOMBER2,
        VEHICLE_BOMBER3,
        VEHICLE_BOMBER4,
        VEHICLE_BOMBER5,
        VEHICLE_BOMBER6,
        VEHICLE_TRANSPORT1,
        VEHICLE_TRANSPORT2,
        VEHICLE_TRANSPORT3,
        VEHICLE_TRANSPORT4,
        VEHICLE_TRANSPORT5,
        VEHICLE_TRANSPORT6,
        VEHICLE_MPB1,
        VEHICLE_MPB2,
        VEHICLE_MPB3
    };

    update_code         AdvanceLogic        ( f32 DeltaTime );
                                                                     
    void                Initialize          ( const vector3&    WorldPos,
                                              const vector3&    Velocity,
                                                    chunk_type  Type,
                                                    s32         SmokeEffect );
                                            
    void                OnAdd               ( void );
    void                Render              ( void );
    void                RenderParticles     ( void );

protected:

    xbool               Impact              ( const collider::collision& Collision );

    radian3             m_Rot;
    radian3             m_DeltaRot;
    pfx_effect          m_SmokeEffect;
    chunk_type          m_Type;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* DebrisCreator( void );

//==============================================================================
#endif // DEBRIS_HPP
//==============================================================================
