//==============================================================================
//
//  ParticlePool.hpp
//
//==============================================================================

#ifndef PARTICLEPOOL_HPP
#define PARTICLEPOOL_HPP

//=============================================================================
//  Includes
//=============================================================================

#include <xCore/x_files/x_files.hpp>

//=============================================================================
//  Particle
//=============================================================================

struct pfx_particle
{
    vector3             Position;                       // Current position
    vector3             Velocity;                       // Velocity, applied to position
    f32                 Life;                           // Total time alive
    f32                 Angle;                          // Angle of rotation
    f32                 Age;                            // Increments, dies when > Life
};

//=============================================================================
//  Particle pool
//=============================================================================

class pfx_pool
{
public:
                        pfx_pool                    ( void );
                       ~pfx_pool                    ( void );

    xbool               Init                        ( s32 NumParticles );
    void                Kill                        ( void );

    s32                 GetNumAllocatedParticles    ( void );
    s32                 GetNumFreeParticles         ( void );
    s32                 GetFragmentation            ( void );

    pfx_particle*       AllocateParticles           ( s32& MaxParticles );
    void                FreeParticles               ( pfx_particle* pParticles, s32 Count );

protected:
    pfx_particle*       m_pParticles;               // Pointer to particles in pool
    s32                 m_NumParticles;             // Number of particles in pool
    s32                 m_NumFree;                  // Number of free particles

    xbool               m_Created;                  // TRUE when created
};

//=============================================================================
#endif // PARTICLEPOOL_HPP
//=============================================================================
