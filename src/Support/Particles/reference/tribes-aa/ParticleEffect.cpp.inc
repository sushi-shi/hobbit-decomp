#include <xCore/Entropy/Entropy.hpp>
#include "ParticleEffect.hpp"
#include "aads/globals.hpp"

//=============================================================================
//  Particle globals
//=============================================================================

xcolor  g_PfxFogColor;

//=============================================================================
//  Particle effect
//=============================================================================

pfx_effect::pfx_effect( void )
{
    m_Created   = FALSE;
    m_pPool     = NULL;
    m_IsActive  = FALSE;
    m_Normal    = vector3(0,1,0);
    m_MaxAlpha  = 1.0f;
}

//=============================================================================

void pfx_effect::Reset( void )
{
    s32 i;

    for( i=0 ; i<MAX_EMITTERS ; i++ )
    {
        m_Emitters[i].Reset();
    }

    m_Created   = FALSE;
    m_pPool     = NULL;
    m_IsActive  = FALSE;
    m_Normal    = vector3(0,1,0);
}

//=============================================================================

pfx_effect::~pfx_effect( void )
{
}

//=============================================================================

xbool pfx_effect::Create( pfx_pool*         pPool,
                          pfx_library*      pLibrary,
                          s32               EffectID,
                          const vector3&    Position,
                          const vector3&    Normal, 
                          effect_proc       pProc,
                          effect_initproc   pInitProc)
{
    xbool   Success = TRUE;
    s32     i;

    ASSERT( !m_Created );
    ASSERT( pPool );

    // Set particle pool & library pointers
    m_pPool    = pPool;
    m_pLibrary = pLibrary;

    // Setup Positions
    m_Position = Position;
    m_Normal   = Normal;
    m_Velocity.Zero();

    // Get pointer to effect definition
    m_pEffectDef = pLibrary->GetEffectDef( EffectID );

    m_nEmitters = m_pEffectDef->nEmitters;
    ASSERT( m_nEmitters <= MAX_EMITTERS );

    // Setup emitters
    for( i=0 ; i<m_nEmitters ; i++ )
    {
        vector3 Axis( 0.0f, 1.0f, 0.0f );
//        Axis.RotateX( RandVar( RADIAN(45.0f), RADIAN( 30.0f) ) );
//        Axis.RotateY( RandVar( RADIAN( 0.0f), RADIAN(180.0f) ) );

        pfx_emitter_def*    pEmitterDef  = pLibrary->GetEmitterDef ( pLibrary->GetIndex( m_pEffectDef->iEmitterDef+i*2   ) );
        pfx_particle_def*   pParticleDef = pLibrary->GetParticleDef( pLibrary->GetIndex( m_pEffectDef->iEmitterDef+i*2+1 ) );

        m_Emitters[i].Create ( this, m_pPool, m_pLibrary, pEmitterDef, pParticleDef, m_Position);
        m_Emitters[i].SetAxis( Axis );
    }

    // Set created and active flags
    m_Created  = TRUE;
    m_IsActive = TRUE;
    m_Enabled  = TRUE;

    // set the callback procedure
    m_pProc = pProc;

    // if there is an init callback, call it
    if ( pInitProc != NULL )
        (*pInitProc)(*this);

    // Return sucess code
    return Success;
}

//=============================================================================

void pfx_effect::SetEnabled( xbool State )
{
    m_Enabled = State;
}

//=============================================================================

xbool pfx_effect::IsEnabled( void ) const
{
    return m_Enabled;
}

//=============================================================================

xbool pfx_effect::IsActive( void ) const
{
    ASSERT( m_Created );

    // Return TRUE if effect is still active
    return m_IsActive;
}

//=============================================================================

xbool pfx_effect::IsCreated( void ) const
{
    return m_Created;
}

//=============================================================================

const vector3& pfx_effect::GetPosition( void ) const
{
    return m_Position;
}

void pfx_effect::SetPosition( const vector3& Position )
{
    s32 i;

    m_Position = Position;

    // Loop through all emitters
    for( i=0 ; i<m_nEmitters ; i++ )
    {
        m_Emitters[i].SetPosition( Position );
    }
}

//=============================================================================

const vector3& pfx_effect::GetVelocity( void ) const
{
    return m_Velocity;
}

void pfx_effect::SetVelocity( const vector3& Velocity )
{
    s32 i;

    m_Velocity = Velocity;

    // Loop through all emitters
    for( i=0 ; i<m_nEmitters ; i++ )
    {
        m_Emitters[i].SetVelocity( Velocity );
    }
}


//=============================================================================

void pfx_effect::UpdateEffect( f32 DeltaTime )
{
    s32     i;

    ASSERT( m_Created );

    // Add Velocity to Position
    m_Position += m_Velocity * DeltaTime;

    // Clear Active Flag, it will be set by any active emitters
    m_IsActive = FALSE;

    // Loop through all emitters
    for( i=0 ; i<m_nEmitters ; i++ )
    {
        pfx_emitter& Emitter = m_Emitters[i];
        
        // call the callback
        if ( m_pProc != NULL )
            (*m_pProc)(*this, Emitter);
        
        Emitter.UpdateEmitter( DeltaTime );
        m_IsActive |= Emitter.IsActive();
    }
}

//=============================================================================

void pfx_effect::RenderEffect( )
{
    s32     i;

    ASSERT( m_Created );

    // Compute Fog color for effect, exit if fully fogged
    g_PfxFogColor = tgl.Fog.ComputeParticleFog( m_Position );
    if( g_PfxFogColor.A == 255 )
        return;

    // Disable Z-Write
#ifdef TARGET_PC
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
#endif

#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif
    
    pfx_emitter::m_ViewRot = eng_GetActiveView(0)->GetV2W().GetRotation();

    //shape::BeginDraw();
    // Loop through all emitters
    for( i=0 ; i<m_nEmitters ; i++ )
    {
        pfx_emitter& Emitter = m_Emitters[i];
        Emitter.RenderEmitter( );
    }
    //shape::EndDraw();

    // Enable Z-Write
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif

#ifdef TARGET_PC
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
#endif
}

//=============================================================================

int                     pfx_effect::GetNumEmitters      ( void ) { return m_nEmitters; }
pfx_emitter&            pfx_effect::GetEmitter          ( s32 i ){ ASSERT(i<m_nEmitters); return m_Emitters[i]; }
const vector3&          pfx_effect::GetNormal           ( void ) { return m_Normal; }
void                    pfx_effect::SetMaxAlpha         ( f32 Alpha )       { m_MaxAlpha = Alpha; }
f32                     pfx_effect::GetMaxAlpha         ( void )            { return m_MaxAlpha; }

//=============================================================================
