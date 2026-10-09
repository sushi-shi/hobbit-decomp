//==============================================================================
//
//  ParticleEmitter.hpp
//
//==============================================================================

#ifndef PARTICLEEMITTER_HPP
#define PARTICLEEMITTER_HPP

//=============================================================================
//  Includes
//=============================================================================

#include <xCore/x_files/x_files.hpp>
#include "ParticleDefs.hpp"
#include "ParticlePool.hpp"
#include "ParticleLibrary.hpp"
#include <Support/Shape/ShapeInstance.hpp>

class pfx_effect;

//=============================================================================
//  Particle emitter
//=============================================================================

class pfx_emitter
{
public:
                            pfx_emitter         ( void );
                           ~pfx_emitter         ( void );

    xbool                   Create              ( pfx_effect*          pEffect,
                                                  pfx_pool*            pParticlePool,
                                                  pfx_library*         pParticleLibrary,
                                                  pfx_emitter_def*     pEmitterDef,
                                                  pfx_particle_def*    pParticleDef,
                                                  const vector3&       Position );

    void                    Reset               ( void );

    const vector3&          GetPosition         ( void ) const;
    const vector3&          GetVelocity         ( void ) const;
    const vector3&          GetAcceleration     ( void ) const;
    const vector3&          GetAxis             ( void ) const;
    f32                     GetAge              ( void ) { return m_Age; }
    f32                     GetLife             ( void ) { return m_Life; }
    pfx_emitter_def::etype  GetType             ( void ) { return m_pEmitterDef->Type; }
    void                    SetPosition         ( const vector3& Position );
    void                    SetVelocity         ( const vector3& Position );
    void                    SetAcceleration     ( const vector3& Position );
    void                    SetAxis             ( const vector3& Axis );
    void                    UseAxis             ( xbool Use );
    void                    SetRotation         ( f32 Rotation ) { m_Rotation = Rotation; }
    void                    SetAzimuth          ( radian Azim, radian Variance );
    void                    SetElevation        ( radian Elev, radian Variance );

    xbool                   IsActive            ( void ) const;

    void                    UpdateEmitter       ( f32 DeltaTime );
    void                    RenderEmitter       ( void );

    void                    SetMaxAlpha         ( f32 Alpha )       { m_MaxAlpha = Alpha; }
    f32                     GetMaxAlpha         ( void )            { return m_MaxAlpha; }

    // to reduce view rotation calculations
    static radian3          m_ViewRot; 

protected:
    
    inline
    void                    ReadKey             ( pfx_key& out, f32 t );

    void                    UpdatePoint         ( f32 DeltaTime );
    void                    UpdateShell         ( f32 DeltaTime );
    void                    UpdateShape         ( f32 DeltaTime );
    void                    RenderPoint         ( void );
    void                    RenderOrientedPoint ( void );
    void                    RenderShell         ( void );
    void                    RenderShape         ( void );

protected:

    static shape_instance   m_Instance;                 // reusable shape instance
    s32                     m_ShapeID;
    pfx_effect*             m_pEffect;                  // Pointer to effect
    pfx_pool*               m_pParticlePool;            // Particle pool to use for particles
    pfx_library*            m_pParticleLibrary;         // Particle library this emitter comes from
    pfx_emitter_def*        m_pEmitterDef;              // Particle emitter descriptor
    pfx_particle_def*       m_pParticleDef;             // Particle descriptor

    xbool                   m_Created;                  // TRUE when created

    pfx_particle*           m_pParticles;               // Pointer to array of particles
    s32                     m_MaxParticles;             // Size of particle array
    s32                     m_ActiveParticles;          // Number of active particles

    vector3                 m_Position;                 // Position of emitter
    vector3                 m_Velocity;                 // Velocity of emitter
    vector3                 m_Acceleration;             // Acceleration of emitter

    vector3                 m_Axis;                     // Axis of emission
    xbool                   m_UseAxis;
    vector3                 m_A, m_B;                   // for rotating emitter axis to normal

    f32                     m_MaxAlpha;                 // for adjusting the max alpha per emitter
    f32                     m_Rotation; 
    f32                     m_Age;                      // Age of particle, starts at 0 and increments with time
    f32                     m_NextEmissionTime;         // Time of next emission (local time)
    f32                     m_Life;                     // Life expectancy of emitter
};

//=============================================================================
#endif // PARTICLEEMITTER_HPP
//=============================================================================
