//==============================================================================
//
//  ParticleDefs.hpp
//
//==============================================================================

#ifndef PARTICLEDEFS_HPP
#define PARTICLEDEFS_HPP

//=============================================================================
//  Includes
//=============================================================================

#include <xCore/x_files/x_files.hpp>

//=============================================================================
//  Defines
//=============================================================================

#define PFX_NUM_KEYS 3

//=============================================================================
//  Animation Key for particle
//=============================================================================

struct pfx_key
{
    f32     S;                                          // Scale
    f32     I;                                          // Texture Frame Index
    u8      R;                                          // Red
    u8      G;                                          // Green
    u8      B;                                          // Blue
    u8      A;                                          // Alpha
};

//=============================================================================
// Particle definition
//=============================================================================

#define PFX_PF_ADDITIVE         (1<<0)                  // Particle has additive Alpha
#define PFX_PF_MOVETOVIEWER     (1<<1)                  // Particle moves towards viewer TODO: Hack remove later
#define PFX_PF_HASGLOW          (1<<2)                  // particle should be rendered with PostFX

struct pfx_particle_def
{
    u32             Flags;                              // Flag Bits, see defines above
    f32             DragCoefficient;                    // Drag Coefficient
    f32             GravityCoefficient;                 // Gravity Coefficient
    f32             InheritVelCoefficient;              // Inherit velocity from emitter
    f32             Life;                               // Life of particle
    f32             LifeVariance;                       // Variance
    f32             Rotation;                           // Rotation of particle
    f32             RotationVariance;                   // Variance
    xbitmap*        m_pTextures;                        // Pointer to array of textures
    s16             nKeys;                              // Number of keys
    s16             iKey;                               // Index of first key
};

//=============================================================================
//  Emitter definition
//=============================================================================

#define PFX_EF_IMMORTAL         (1<<0)                  // Emitter is immortal, does not age
#define PFX_EF_RELATIVE         (1<<1)                  // Emitter is relative to the owning effect

struct pfx_emitter_def
{
    enum etype
    {
        UNDEFINED,
        POINT,
        ORIENTED_POINT,
        SHELL,
        SHOCKWAVE,
        SHAPE
    };

    etype           Type;                               // Type of emitter
    s32             ShapeID;                            // Shape ID for SHAPE types

    u32             Flags;                              // Flag Bits, see defines above
    s32             MaxParticles;                       // Maximum number of particles active for emitter

    f32             Life;                               // Life of emitter
    f32             LifeVariance;                       // Variance (+/-)

    f32             EmitDelay;                          // Time to delay before emissions start
    f32             EmitDelayVariance;                  // Variance (+/-)
                                                        
    f32             EmitInterval;                       // Time between emissions
    f32             EmitIntervalVariance;               // Variance (+/-)
                                                        
    f32             EmitOffset;                         // Distance from origin of emission to create particle
    f32             EmitOffsetVariance;                 // Variance (+/-)
                                                        
    f32             EmitVelocity;                       // Velocity of emission
    f32             EmitVelocityVariance;               // Variance (+/-)
                                                        
    f32             EmitElevation;                      // Elevation of emission (Axis is taken as the vertical, 0 degrees)
    f32             EmitElevationVariance;              // Variance (+/-)
                                                        
    f32             EmitAzimuth;                        // Azimuth of emission (Viewer is at 0 degrees)
    f32             EmitAzimuthVariance;                // Variance (+/-)
};

//=============================================================================
//  Effect definition
//=============================================================================

struct pfx_effect_def
{
    u32             Flags;                              // Flag Bits, see defines above

    s16             nEmitters;                          // Number of emitters
    s16             iEmitterDef;                        // Index of first emitterdef index
};

//=============================================================================
#endif // PARTICLEDEFS_HPP
//=============================================================================
