//==============================================================================
//
//  ParticleObject.hpp
//
//==============================================================================

#ifndef PARTICLE_OBJECT_HPP
#define PARTICLE_OBJECT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "Particles\ParticleEffect.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class particle_object : public object
{

public:

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    void        OnAdd           ( void );
  
    void        Initialize      ( const vector3&    Position, 
                                  const vector3&    Normal,
                                        pain::type  PainType,
                                  const object::id  ObjectHitID,
                                        f32         Intensity );

    void        Render          ( void );

protected:

    f32             m_Age;
    vector3         m_Normal;
    object::type    m_ObjectHitType;
    pain::type      m_PainType;
    f32             m_Intensity;
    xbool           m_CreateVisuals;
    xbool           m_DebrisCreated;

    pfx_effect      m_ParticleEffect;

    void        PlayBulletSound ( void );
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     ParticleObjectCreator( void );

void        SpawnExplosion  ( pain::type        PainType,    
                              const vector3&    Position,
                              const vector3&    Normal,
                              object::id        OriginID,
                              object::id        VictimID = obj_mgr::NullID,
                              f32               Scalar   = 1.0f );

void        SpawnDebris     (       s32*        pCount,
                                    f32         Velocity,
                                    xbool       Full360,
                              const vector3&    WorldPos );
            
void        SpawnDebris     (       pain::type  PainType,
                              const vector3&    WorldPos );

//==============================================================================
#endif // PARTICLE_OBJECT_HPP
//==============================================================================
