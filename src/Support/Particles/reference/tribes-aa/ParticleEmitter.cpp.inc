#include <xCore/Entropy/Entropy.hpp>
#include "ParticleEmitter.hpp"
#include "ParticleEffect.hpp"
#include <Support/Shape/ShapeInstance.hpp>
#include "aads\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include <Support/Shape/Poly.hpp>

#ifdef TARGET_PS2
xbool pfx_UseDraw = FALSE;
#else
xbool pfx_UseDraw = TRUE;
#endif

xbool pfx_UseAlpha = TRUE;

#define MAX_VERTS               144      // MUST be divisible by 6

static vector2 s_UVS[6] =
{
    vector2( 1.0f, 0.0f ),
    vector2( 0.0f, 0.0f ),
    vector2( 0.0f, 1.0f ),
    vector2( 1.0f, 0.0f ),
    vector2( 0.0f, 1.0f ),
    vector2( 1.0f, 1.0f ),
};

extern xcolor g_PfxFogColor;

//=============================================================================
// The reusable shape instance
//=============================================================================

shape_instance  pfx_emitter::m_Instance;
radian3         pfx_emitter::m_ViewRot;

//=============================================================================
//  Random Helper Functions
//=============================================================================

static f32 RandVar( f32 Nominal, f32 Variance )
{
    return ((f32)x_rand()/X_RAND_MAX) * (Variance * 2.0f) - Variance + Nominal;
}

//=============================================================================
//  Particle emitter
//=============================================================================

pfx_emitter::pfx_emitter( void )
{
    m_Created           = FALSE;
    m_ActiveParticles   = 0;
    m_Life              = 0.0f;
    m_Age               = 0.0f;
}

//=============================================================================

void pfx_emitter::Reset( void )
{
    m_Created           = FALSE;
    m_ActiveParticles   = 0;
    m_Life              = 0.0f;
    m_Age               = 0.0f;
}

//=============================================================================

pfx_emitter::~pfx_emitter( void )
{
}

//=============================================================================

xbool pfx_emitter::Create( pfx_effect*          pEffect,
                           pfx_pool*            pParticlePool,
                           pfx_library*         pParticleLibrary,
                           pfx_emitter_def*     pEmitterDef,
                           pfx_particle_def*    pParticleDef,
                           const vector3&       Position )
{
    xbool   Success = TRUE;

    ASSERT( !m_Created );
    ASSERT( pEffect );
    ASSERT( pParticlePool );
    ASSERT( pEmitterDef );
    ASSERT( pParticleDef );
    ASSERT( pParticleDef->nKeys > 0 );

    // Init parameters
    m_pEffect          = pEffect;
    m_pEmitterDef      = pEmitterDef;

/*
    // Precalc some stuff for rotating emitter axis
    vector3 FXN = pEffect->GetNormal();
    if ( ( FXN.X == 0.0f ) &&
         ( FXN.Y == 0.0f ) &&
         ( FXN.Z == 0.0f ) )
    {
        FXN.Set(0,1,0);
    }

    plane   P;
    P.Setup( Position, FXN );
    P.GetOrthoVectors( m_A, m_B );

    // Allocate particles if needed
    m_MaxParticles     = m_pEmitterDef->MaxParticles;
    if( m_MaxParticles > 0 )
    {
        m_pParticles       = m_pParticlePool->AllocateParticles( m_MaxParticles ) ;
        m_ActiveParticles  = 0;

        // Clear particles
        if( m_MaxParticles > 0 )
            x_memset( m_pParticles, 0, sizeof(pfx_particle)*m_MaxParticles );

        //ASSERT( m_MaxParticles > 0 );
    }
*/

    // Init times
    m_Age              = 0.0f;
//    m_NextEmissionTime = RandVar( m_pEmitterDef->EmitDelay, m_pEmitterDef->EmitDelayVariance   );
    m_Life             = RandVar( m_pEmitterDef->Life,      m_pEmitterDef->LifeVariance        ) + RandVar( pParticleDef->Life, pParticleDef->LifeVariance );

    // Set created flag
    m_Created          = TRUE;

    // Return success code
    return Success;
}

//=============================================================================

const vector3& pfx_emitter::GetPosition( void ) const
{
    return m_Position;
}

void pfx_emitter::SetPosition( const vector3& Position )
{
    m_Position = Position;
}

//=============================================================================

const vector3& pfx_emitter::GetVelocity( void ) const
{
    return m_Velocity;
}

void pfx_emitter::SetVelocity( const vector3& Velocity )
{
    m_Velocity = Velocity;
}

//=============================================================================

const vector3& pfx_emitter::GetAcceleration( void ) const
{
    return m_Acceleration;
}

void pfx_emitter::SetAcceleration( const vector3& Acceleration )
{
    m_Acceleration = Acceleration;
}

//=============================================================================

const vector3& pfx_emitter::GetAxis( void ) const
{
    return m_Axis;
}

//=============================================================================

void pfx_emitter::SetAxis( const vector3& Axis )
{
    m_Axis = Axis;
}

//=============================================================================
void pfx_emitter::UseAxis( xbool Use )
{
    m_UseAxis = Use;
}

//=============================================================================

xbool pfx_emitter::IsActive( void ) const
{
    xbool   IsActive;

    // Check if there are active particles or the potential for new particles
    // Immortal emitters are assumed to always have potential for more particles
    // so IsActive will tell is there are particles active
    if( (m_pEmitterDef->Flags & PFX_EF_IMMORTAL) )
        IsActive = (m_ActiveParticles > 0);
    else
        IsActive = ((m_Age < m_Life) || (m_ActiveParticles > 0));

    // Return active code
    return IsActive;
}

//=============================================================================
void pfx_emitter::SetAzimuth( radian Azim, radian Variance )
{
    m_pEmitterDef->EmitAzimuth = Azim;
    m_pEmitterDef->EmitAzimuthVariance = Variance;
}

//=============================================================================
void pfx_emitter::SetElevation( radian Elev, radian Variance )
{
    m_pEmitterDef->EmitElevation = Elev;
    m_pEmitterDef->EmitElevationVariance = Variance;
}

//=============================================================================

void pfx_emitter::UpdatePoint( f32 DeltaTime )
{
    ASSERT( m_Created );

    m_Age += DeltaTime;
    if( (m_Age <= m_Life) || ((m_pEmitterDef->Flags & PFX_EF_IMMORTAL) && m_pEffect->IsEnabled()) )
        m_ActiveParticles = 1;
    else
        m_ActiveParticles = 0;
}

//=============================================================================

void pfx_emitter::UpdateShell( f32 DeltaTime )
{
    ASSERT( m_Created );

    m_Age      += DeltaTime;
}

//=============================================================================

void pfx_emitter::UpdateShape( f32 DeltaTime )
{
    ASSERT( m_Created );

    m_Age      += DeltaTime;
}

//=============================================================================

void pfx_emitter::UpdateEmitter( f32 DeltaTime )
{
    switch( m_pEmitterDef->Type )
    {
    case pfx_emitter_def::POINT:
    case pfx_emitter_def::ORIENTED_POINT:
        UpdatePoint( DeltaTime );
        break;

    case pfx_emitter_def::SHELL:
        UpdateShell( DeltaTime );
        break;

    case pfx_emitter_def::SHOCKWAVE:
        break;

    case pfx_emitter_def::SHAPE:
        UpdateShape( DeltaTime );
        break;

    default:
        ASSERT( 0 );
    }
}

//=============================================================================

void pfx_emitter::RenderEmitter( void )
{
    return;
}

//=============================================================================
