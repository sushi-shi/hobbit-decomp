//==============================================================================
//
//  ParticleEffect.hpp
//
//==============================================================================

#ifndef PARTICLEEFFECT_HPP
#define PARTICLEEFFECT_HPP

//=============================================================================
//  Includes
//=============================================================================

#include <xCore/x_files/x_files.hpp>
#include "ParticlePool.hpp"
#include "ParticleEmitter.hpp"
#include "ParticleLibrary.hpp"

//=============================================================================
//  Defines
//=============================================================================

#define MAX_EMITTERS        12



//=============================================================================
//  Types
//=============================================================================
class pfx_effect;

typedef void (*effect_initproc)( pfx_effect& Effect );
typedef void (*effect_proc)( pfx_effect& Effect, pfx_emitter& Emitter );


//=============================================================================
//  Particle effect
//=============================================================================

class pfx_effect
{
public:
                            pfx_effect          ( void );
                           ~pfx_effect          ( void );

    xbool                   Create              ( pfx_pool*         pPool,
                                                  pfx_library*      pLibrary,
                                                  s32               EffectID,
                                                  const vector3&    Position,
                                                  const vector3&    Normal,
                                                  effect_proc       pProc = NULL,
                                                  effect_initproc   pInitProc = NULL );

    void                    Reset               ( void );

    void                    SetEnabled          ( xbool State );
    xbool                   IsEnabled           ( void ) const;
    xbool                   IsActive            ( void ) const;
    xbool                   IsCreated           ( void ) const;

    const vector3&          GetPosition         ( void ) const;
    void                    SetPosition         ( const vector3& Position );
    const vector3&          GetVelocity         ( void ) const;
    void                    SetVelocity         ( const vector3& Velocity );
    int                     GetNumEmitters      ( void );
    pfx_emitter&            GetEmitter          ( s32 i );
    const vector3&          GetNormal           ( void );

    void                    UpdateEffect        ( f32 DeltaTime );
    void                    RenderEffect        ( void );

    void                    SetMaxAlpha         ( f32 Alpha );
    f32                     GetMaxAlpha         ( void );

protected:
    pfx_effect_def*         m_pEffectDef;               // Pointer to Effect definition

    xbool                   m_Created;                  // TRUE when created
    xbool                   m_IsActive;                 // TRUE when still active
    xbool                   m_Enabled;                  // TRUE when enabled
    pfx_pool*               m_pPool;                    // Particle pool to allocate particles from
    pfx_library*            m_pLibrary;                 // Particle library this effect comes from
    effect_proc             m_pProc;                    // Effect callback procedure
    f32                     m_MaxAlpha;                 // for fading out an effect smoothly

    vector3                 m_Position;                 // Position of effect
    vector3                 m_Velocity;                 // Velocity of effect
    vector3                 m_Normal;                   // Normal for emission orientation

    s32                     m_nEmitters;                // Number of emitters
    pfx_emitter             m_Emitters[MAX_EMITTERS];   // Array of emitters

    bbox                    m_Bounds;                   // Bounding box
};

//=============================================================================
#endif // PARTICLEEFFECT_HPP
//=============================================================================
