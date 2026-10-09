//==============================================================================
//
//  GenericShot.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "ParticleObject.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AudioMgr/Audio.hpp"
#include "netlib/bitstream.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include <Support/PointLight/PointLight.hpp>

#include "Generic.hpp"
#include "textures.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define FADE_TIME       ( 1.0f )

//==============================================================================
//  STORAGE
//==============================================================================

static generic_shot::kind  CurRenderMode;      

obj_create_fn   GenericShotCreator;

obj_type_info   GenericShotClassInfo( object::TYPE_GENERICSHOT, 
                                      "GenericShot", 
                                      GenericShotCreator, 
                                      0 );

static f32 GenericVelocity[3] = { 125.0f, 75.0f, 90.0f };

xbool  MeteorShower = FALSE;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* GenericShotCreator( void )
{
    return( (object*)(new generic_shot) );
}

//==============================================================================

f32 generic_shot::GetMuzzleSpeed      ( void )
{
    ASSERT( IN_RANGE( AA_TURRET, m_Kind, CLAMP_TURRET ) );

    if( MeteorShower && (m_Kind == PLASMA_TURRET) )
        return( 200.0f );

    return( GenericVelocity[m_Kind] );
}

//==============================================================================

f32 generic_shot::GetMaxAge           ( void )
{
    if( MeteorShower && (m_Kind == PLASMA_TURRET) )
        return( 15.0f );

    switch( m_Kind )
    {
        case AA_TURRET:                 return  3.0f; 
        case PLASMA_TURRET:             return  3.0f;
        case CLAMP_TURRET:              return  4.0f;
        default:
            ASSERT( FALSE );
    }

    // to alleviate warnings
    return 1.0f;
}

//==============================================================================

void generic_shot::Setup( void )
{
    m_Speed = GetMuzzleSpeed();
    m_Life  = GetMaxAge();
    m_SoundID = SFX_WEAPONS_PLASMA_PROJECTILE_LOOP;
}

//==============================================================================

void generic_shot::Initialize( const matrix4&     L2W, 
                                     u32          TeamBits, 
                                     kind         Kind, 
                                     object::id   OriginID,
                                     object::id   ExcludeID )
{
    // call default initializer
    m_Kind = Kind;

    Setup();
    linear::Initialize( L2W, TeamBits, OriginID, ExcludeID );
}

//==============================================================================

void generic_shot::OnAcceptInit( const bitstream& BitStream )
{
    // Read the type out of the stream
    s32 Kind;    
    BitStream.ReadRangedS32( Kind, AA_TURRET, CLAMP_TURRET );
    m_Kind = (generic_shot::kind)Kind;
    
    if( m_Kind == AA_TURRET )
    {
        m_ShotIndex = BitStream.ReadFlag();
    }

    // call default OnAcceptInit
    linear::OnAcceptInit( BitStream );

    Setup();
}

//==============================================================================

void generic_shot::OnProvideInit( bitstream& BitStream )
{
    // write the 'kind' 
    BitStream.WriteRangedS32( m_Kind, AA_TURRET, CLAMP_TURRET );

    if( m_Kind == AA_TURRET )
    {
        BitStream.WriteFlag( m_ShotIndex );
    }

    linear::OnProvideInit( BitStream );
}

//==============================================================================

void generic_shot::OnAdd( void )
{
    linear::OnAdd();
    vector3 Pos = GetRenderPos();

    // init the 'has expired' flag
    m_HasExpired = FALSE;

    // initialize based on type
    switch ( m_Kind )
    {
        case AA_TURRET:
            audio_Play( SFX_TURRET_AATURRET_FIRE, &Pos, AUDFLAG_LOCALIZED );
            m_Texture1 = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]AAShot") ;
            m_PointLightID = ptlight_Create( Pos, 3.0f, xcolor(225,225,255) );
            tgl.PostFX.AddFX( Pos, vector2(2.0f,2.0f), xcolor(255,255,255,127), 0.25f );
            break;

        case PLASMA_TURRET:
            audio_Play( SFX_TURRET_PLASMA_FIRE, &Pos, AUDFLAG_LOCALIZED );
            m_Texture1 = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]PlasmaShot") ;
            m_Rot = R_180;
            m_PointLightID = ptlight_Create( Pos, 4.0f, xcolor(192,192,255) );

            m_SmokeEffect.Create( &tgl.ParticlePool, 
                                  &tgl.ParticleLibrary, 
                                  PARTICLE_TYPE_PLASMA_TRAIL, 
                                  Pos, 
                                  vector3(0,0,1), 
                                  NULL, 
                                  NULL );

            break;

        case CLAMP_TURRET:
            audio_Play( SFX_TURRET_SPIDER_FIRE, &Pos, AUDFLAG_LOCALIZED );
            m_Texture1 = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]ClampShot") ;
            m_Rot = R_180;
            m_PointLightID = ptlight_Create( Pos, 2.0f, xcolor(255,192,192) );
            break;

        default:
            ASSERT( FALSE );
            break;
    }
}

//==============================================================================

void generic_shot::BeginRender( generic_shot::kind Kind )
{
    // remember our current mode
    CurRenderMode = Kind;

    // initialize based on type
    switch( Kind )
    {
        case AA_TURRET:
            // render the tail first
            draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
            break;

        case CLAMP_TURRET:
        case PLASMA_TURRET:
            // render the head first
            draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
            break;

        default:
            ASSERT( FALSE );
    }    

    // always additive alpha for these puppies
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif

}

//==============================================================================

void generic_shot::EndRender( void )
{
    draw_End();
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif

}

//==============================================================================

void generic_shot::Render( void )
{
    if( m_Age == 0.0f )
        return;

    vector3 Pos = GetRenderPos();

    vector3 EndPt;
    xcolor  Color;
    f32     Alpha;

    // should only call this after valid mode is set in call to BeginRender
    ASSERT( m_Kind == CurRenderMode );

    switch( m_Kind )
    {
        case AA_TURRET:
            // set the current texture
            draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(m_Texture1).GetXBitmap() );
            EndPt = -m_Movement;
            EndPt.NormalizeAndScale( 5.0f );
            draw_OrientedQuad( Pos, Pos + EndPt, vector2(0,0), vector2(1,1), XCOLOR_WHITE, XCOLOR_WHITE, 0.25f );

            tgl.PostFX.AddFX( Pos, vector2(0.5f,0.5f), xcolor(195,195,255,128) );
            tgl.PostFX.AddFX( Pos, vector2(1.5f,1.5f), xcolor(195,195,255, 38) );
            break;

        case PLASMA_TURRET:
            if ( m_HasExpired )
                break;

            // fade out
            Color = XCOLOR_WHITE;
            if( m_Age > (m_Life - FADE_TIME) )
            {
                Alpha = MIN( 1.0f, (m_Life - m_Age) / FADE_TIME );
                Alpha = MAX( 0.0f, Alpha );
                m_SmokeEffect.SetMaxAlpha( Alpha );

                // fade out the plasma
                Color.A = (u8)(255 * Alpha);
            }
            else
                Alpha = 1.0f;

            // set the current texture
            draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(m_Texture1).GetXBitmap() );
            // draw 3 overlapping, rotating sprites
            draw_SpriteUV( Pos, vector2(2.0f,2.0f), vector2(0,0), vector2(1,1), Color,  m_Rot );
            draw_SpriteUV( Pos, vector2(2.0f,2.0f), vector2(0,0), vector2(1,1), Color, -m_Rot );
            draw_SpriteUV( Pos, vector2(2.0f,2.0f), vector2(0,0), vector2(1,1), Color,  m_Rot * 0.5f );
            draw_SpriteUV( Pos, vector2(2.0f,2.0f), vector2(0,0), vector2(1,1), Color, -m_Rot * 0.5f );
            // add the glow
            tgl.PostFX.AddFX( Pos, vector2( 5.0f, 5.0f ), xcolor( 225, 225, 255, (u8)(38 * Alpha) ) );
            //tgl.PostFX.AddFX( Pos, vector2( 3.0f, 3.0f ), xcolor( 225, 225, 255, (u8)(38 * Alpha) ), 0.06f );
            m_Rot += tgl.DeltaLogicTime;
            break;

        case CLAMP_TURRET:
            // set the current texture
            draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(m_Texture1).GetXBitmap() );
            // draw 3 overlapping, rotating sprites
            draw_SpriteUV( Pos, vector2(0.3f,0.3f), vector2(0,0), vector2(1,1), XCOLOR_WHITE,  m_Rot );
            draw_SpriteUV( Pos, vector2(0.3f,0.3f), vector2(0,0), vector2(1,1), XCOLOR_WHITE, -m_Rot );
            m_Rot += tgl.DeltaLogicTime;
            tgl.PostFX.AddFX( Pos, vector2(1.5f,1.5f), xcolor(255,225,225,38) );
            break;

        default:
            ASSERT( FALSE );
            break;
    }
}

//==============================================================================

void generic_shot::RenderParticles( void )
{
    if( m_SmokeEffect.IsCreated() )
        m_SmokeEffect.RenderEffect();
}

//==============================================================================

object::update_code generic_shot::Impact( vector3& Point, vector3& Normal )
{

    // skip if we need to continue processing smoke
    if( m_HasExpired )
    {
        if( m_SmokeEffect.IsCreated() )
        {
            if( !m_SmokeEffect.IsActive() )
                return( DESTROY );
            else
                return( UPDATE );
        }
        else
            return( DESTROY );
    }

    m_HasExpired = TRUE;

    // This list must be in sync with the enum 'kind'.
    pain::type PainType[] = 
    { 
        pain::TURRET_AA, 
        pain::TURRET_PLASMA, 
        pain::TURRET_CLAMP,
    };

    ASSERT( IN_RANGE( AA_TURRET, m_Kind, CLAMP_TURRET ) );

    if( m_Age < GetMaxAge() )
    {
        if( MeteorShower && (m_Kind == PLASMA_TURRET) )
        {
            SpawnExplosion( pain::WEAPON_SATCHEL,
                            Point, 
                            vector3(0,1,0),
                            obj_mgr::NullID,
                            m_ImpactObjectID );

            SpawnExplosion( pain::EXPLOSION_TURRET_FIXED,
                            Point, 
                            Normal,
                            obj_mgr::NullID,
                            m_ImpactObjectID );
        }
        else
        {
            SpawnExplosion( PainType[m_Kind],
                            Point, 
                            Normal,
                            m_ScoreID, // <<---------- Not the OriginID (for remote turrets)
                            m_ImpactObjectID );
        }

        m_SmokeEffect.SetEnabled( FALSE );

        if ( m_Kind != PLASMA_TURRET )
            return( DESTROY );
        else
            return( UPDATE );
    }
    else
        return( UPDATE );
}

//==============================================================================

object::update_code generic_shot::OnAdvanceLogic( f32 DeltaTime )
{
    vector3 Before = GetPosition();

    object::update_code RetCode = linear::OnAdvanceLogic( DeltaTime );
    vector3 Pos = GetRenderPos();

    vector3 After = GetPosition();

    // Special AA "flak" code
    if( m_Kind == AA_TURRET )
    {
        object* pObject = ObjMgr.GetObjectFromSlot( m_TargetID.Slot );
        if( pObject )
        {   
            f32     T;
            vector3 Normal = m_Movement;
            plane   Plane;

            Normal.Normalize();
            Plane.Setup( pObject->GetPainPoint(), Normal );

            if( Plane.Intersect( T, Before, After ) &&
                IN_RANGE( 0.0f, T, 1.0f ) )
            {
                vector3 D = After - Before;
                vector3 P = Before + T*D;

                if( pObject->GetAttrBits() & ATTR_PLAYER )
                {
                    SpawnExplosion( pain::TURRET_AA, 
                                    P, 
                                    Normal,
                                    obj_mgr::NullID, 
                                    obj_mgr::NullID );
                }
                else
                {
                    SpawnExplosion( pain::WEAPON_MISSILE, 
                                    P, 
                                    Normal,
                                    obj_mgr::NullID, 
                                    obj_mgr::NullID );
                }
                return( DESTROY );
            }
        }
    }

    // Update position of smoke emitter.
    if ( m_SmokeEffect.IsCreated() )
    {
        m_SmokeEffect.SetVelocity( (Pos - m_SmokeEffect.GetPosition()) / DeltaTime );
        m_SmokeEffect.UpdateEffect( DeltaTime );
    }

    // Destroy object once the smoke clears
    if( m_HasExpired )
    {
        if( m_SmokeEffect.IsCreated() )
        {
            if( !m_SmokeEffect.IsActive() )
                return( DESTROY );
            else
                RetCode = UPDATE;
        }
        else
            return( DESTROY );
    }

    // Debugging
    /*
    {
        object* pTarget = ObjMgr.GetObjectFromSlot( m_TargetID.Slot );
        if( pTarget )
        {
            vector3 Target = pTarget->GetPosition();
            vector3 Delta  = m_WorldPos - Target;
            x_DebugMsg( "Shot:%03d (%7.2f,%7.2f,%7.2f) -- Target:%03d(%7.2f,%7.2f,%7.2f) -- Delta(%7.2f,%7.2f,%7.2f) -- Dist:%7.2f\n",
                        m_ObjectID.Slot, m_WorldPos.X, m_WorldPos.Y, m_WorldPos.Z, 
                        m_TargetID.Slot, Target.X, Target.Y, Target.Z, 
                        Delta.X, Delta.Y, Delta.Z, 
                        Delta.Length() );
        }
    }
    */

    return RetCode;
}

//==============================================================================
