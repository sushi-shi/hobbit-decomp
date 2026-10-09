//==============================================================================
//
//  Grenade.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Grenade.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "ParticleObject.hpp"
#include "objects\player\playerobject.hpp"
#include "GameMgr\\GameMgr.hpp"
#include "netlib/bitstream.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define FLASH_RADIUS        30.00f
#define MAX_FLASH_DURATION   3.00f
#define MAX_FLARE_DURATION   6.0f
#define FLARE_GRAVITY        0.25f
#define DEFAULT_SPEED       30.00f
#define FLARE_SPEED         10.00f
#define FLARE_DRAG           5.00f

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   GrenadeCreator;

obj_type_info   GrenadeTypeInfo( object::TYPE_GRENADE, 
                                 "Grenade", 
                                 GrenadeCreator, 
                                 0 );

f32 GrenadeRunFactor   =  0.3f;
f32 GrenadeRiseFactor  =  0.2f;
f32 GrenadeDragSpeed   = 25.0f;
f32 GrenadeArmTime     =  1.0f;

static s32 s_FlareCore = -1;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* GrenadeCreator( void )
{
    return( (object*)(new grenade) );
}

//==============================================================================

void grenade::Initialize( const vector3&   Position,      
                          const vector3&   Direction, 
                          const vector3&   OriginVel,
                                object::id OriginID,      
                                u32        TeamBits,      
                                f32        ExtraSpeed,
                                type       Type )
{
    vector3 Velocity;

    // Determine the velocity.
    if( Type == TYPE_LAUNCHED )
    {
        Velocity = Direction * ExtraSpeed;
    }
    else
    {
        Velocity = OriginVel + (Direction * ExtraSpeed);
    }

    // Use the ancestor (arcing) to do much of the work.
    arcing::Initialize( Position, Velocity, OriginID, OriginID, TeamBits );

    vector3 Pos = GetRenderPos();

    // create a sparkly trail effect for flares
    if( Type == TYPE_FLARE )
    {
        m_SmokeEffect.Create( &tgl.ParticlePool, 
              &tgl.ParticleLibrary, 
              PARTICLE_TYPE_FLARE_SPARKS, 
              Pos, 
              vector3(0,0,1), 
              NULL, 
              NULL );

        // set the "hot" bit
        m_AttrBits |= ATTR_HOT ;
    }

    if( Type == TYPE_LAUNCHED )
    {
        // Launched, not thrown, so create smoke trail.
        m_SmokeEffect.Create( &tgl.ParticlePool, 
                              &tgl.ParticleLibrary, 
                              PARTICLE_TYPE_GREN_SMOKE, 
                              Pos, 
                              vector3(0,0,1), 
                              NULL, 
                              NULL );
    }

    m_Rotation.Set( x_frand( R_0, R_360 ), x_frand( R_0, R_360 ), R_0 );

    // Take care of the stuff particular to the grenade.

    m_Type        = Type;
    m_RunFactor   = GrenadeRunFactor;
    m_RiseFactor  = GrenadeRiseFactor;
    m_DragSpeed   = GrenadeDragSpeed;
    m_HasExploded = FALSE;

    if( m_Type == TYPE_LAUNCHED )
        m_SoundID = SFX_WEAPONS_GRENADELAUNCHER_PROJECTILE_LOOP;

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_HANDGRENADE ) );
}

//==============================================================================

void grenade::OnAcceptInit( const bitstream& BitStream )
{
    u32 Int;

    // Do ancestor stuff.
    arcing::OnAcceptInit( BitStream );

    // Take care of the stuff particular to the grenade.
    BitStream.ReadU32( Int, 3 );
    m_Type = (type)Int;

    m_RunFactor   = GrenadeRunFactor;
    m_RiseFactor  = GrenadeRiseFactor;
    m_DragSpeed   = GrenadeDragSpeed;
    m_HasExploded = FALSE;

    m_Rotation.Set( x_frand( R_0,  R_360 ), x_frand( R_0,  R_360 ), R_0 );

    // Create sparkly trail.
    if( m_Type == TYPE_FLARE )
    {
        m_SmokeEffect.Create( &tgl.ParticlePool, 
              &tgl.ParticleLibrary, 
              PARTICLE_TYPE_FLARE_SPARKS, 
              GetRenderPos(), 
              vector3(0,0,1), 
              NULL, 
              NULL );

        // set the "hot" bit
        m_AttrBits |= ATTR_HOT ;
    }

    // Create smoke trail.
    if( m_Type == TYPE_LAUNCHED )
    {
        m_SmokeEffect.Create( &tgl.ParticlePool, 
                              &tgl.ParticleLibrary, 
                              PARTICLE_TYPE_GREN_SMOKE, 
                              GetRenderPos(), 
                              vector3(0,0,1), 
                              NULL, 
                              NULL );
    }

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_HANDGRENADE ) );
}

//==============================================================================

void grenade::OnProvideInit( bitstream& BitStream )
{
    // Do ancestor stuff.
    arcing::OnProvideInit( BitStream );

    // Add our own stuff.
    BitStream.WriteU32( m_Type, 3 );
}

//==============================================================================

void grenade::OnAdd( void )
{
    // call base class
    arcing::OnAdd();

    vector3 Pos = GetRenderPos();

    // Determine if a local player fired this blaster
    xbool Local = FALSE;
    object* pCreator = ObjMgr.GetObjectFromID( m_OriginID );
    if( pCreator && (pCreator->GetType() == object::TYPE_PLAYER) )
    {
        player_object* pPlayer = (player_object*)pCreator;
        Local = pPlayer->IsLocal();
    }

	// Play sound!
    if( Local )
    {
        if( m_Type == TYPE_LAUNCHED )
            audio_Play(SFX_WEAPONS_GRENADELAUNCHER_FIRE );
        else
            audio_Play(SFX_WEAPONS_GRENADE_THROW );
    }
    else
    {
        if( m_Type == TYPE_LAUNCHED )
            audio_Play(SFX_WEAPONS_GRENADELAUNCHER_FIRE, &Pos );
        else
            audio_Play(SFX_WEAPONS_GRENADE_THROW, &Pos);
    }

    // default max age (irrelevant, all but flare won't last this long anyway)
    m_MaxAge = 100.0f;

    // then modify gravity and drag to suit our needs
    if( m_Type == TYPE_FLARE )
    {
        m_Gravity = FLARE_GRAVITY;
        m_Drag    = FLARE_DRAG;

        // flare doesn't live as long
        m_MaxAge = MAX_FLARE_DURATION;
    }
}

//==============================================================================

xbool grenade::Impact( const collider::collision& Collision )
{   
    vector3 Pos        = Collision.Point;
    object* pObjectHit = (object*)Collision.pObjectHit;

    ASSERT( pObjectHit );

    s32 HSound;

    arcing::Impact( Collision );

    if( m_Type == TYPE_FLARE )
    {
        return( FALSE );
    }

    m_Rotation.Set( x_frand( R_0, R_360 ), x_frand( R_0, R_360 ), R_0 );
    
    if( !m_HasExploded )
    {
        if( (m_Age > GrenadeArmTime) || 
            (pObjectHit->GetAttrBits() & object::ATTR_DAMAGEABLE) )
        {
            Explode( Collision.Plane.Normal );
            return( FALSE );
        }

        // Play the TINK sound.
        switch( pObjectHit->GetType() )
        {
        case TYPE_BUILDING:
            HSound = audio_Play( SFX_WEAPONS_GRENADE_BOUNCE_HARD, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;

        default:
            HSound = audio_Play( SFX_WEAPONS_GRENADE_BOUNCE_SOFT, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;
        }
    }
    
    return( FALSE );
}

//==============================================================================

object::update_code grenade::AdvanceLogic( f32 DeltaTime )
{
    object::update_code RetCode = arcing::AdvanceLogic( DeltaTime );

    vector3 Pos = GetRenderPos();

    // Update position of smoke emitter.
    if( m_SmokeEffect.IsCreated() )
    {
        m_SmokeEffect.SetVelocity( (Pos - m_SmokeEffect.GetPosition()) / DeltaTime );
        m_SmokeEffect.UpdateEffect( DeltaTime );
    }

    // Blow up after sitting there or after being in the world for just too 
    // darn long.
    xbool GoBoom = FALSE;
    if( !m_HasExploded )
    {
        if( (m_Type != TYPE_FLARE) &&
            (m_Age > GrenadeArmTime) && 
            ((m_State == STATE_SETTLED) || (m_State == STATE_SLIDING)) )
        {
             GoBoom = TRUE;
        }

        if( m_Age > m_MaxAge )
             GoBoom = TRUE;
    }

    if( GoBoom )
    {
        Explode( vector3(0,1,0) );
    }

    // Destroy grenade.
    if( m_HasExploded )
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

    // Let the ancestor class handle it from here.
    return( RetCode );
}

//==============================================================================

void grenade::Render( void )
{
    // Don't draw if exploded.
    if( m_HasExploded )
        return;

    // Don't draw flares.
    if( m_Type == TYPE_FLARE )
        return;

    /*
    // Orient to movement.
    radian3 r;
    if( m_State != STATE_SETTLED )
    {
        r = m_Rotation * m_Age * 0.01f;
        r.Roll = m_Rotation.Roll;
        m_Rotation = r;
    }
    else
    {
        r = m_Rotation;
    }
    */

    // Setup shape ready for render prep
    m_Shape.SetPos( GetRenderPos() );
    m_Shape.SetRot( m_Rotation );

    // Visible?
    if( !RenderPrep() )
        return;

    // Draw it!
    m_Shape.Draw( m_ShapeDrawFlags );
}

//==============================================================================

void grenade::RenderParticles( void )
{
    // Render smoke trail
    if( !m_Hidden )
    {
        // but only if visible
        if( !IsVisible( m_WorldBBox ) )
            return;

        if( m_SmokeEffect.IsCreated() )
            m_SmokeEffect.RenderEffect();
    }
}

//==============================================================================

void grenade::RenderFlare( void )
{
    // should only ever be called for flare types
    ASSERT( m_Type == TYPE_FLARE );

    // init our textures on the first call
    if( s_FlareCore == -1 )
    {
        s_FlareCore = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName( "[A]flarecore" );
    }

    // Only bother if we're a flare
    if( !m_Hidden && !m_HasExploded )
    {
        xcolor color;

        // First, draw the hot points at the beginning and end
        draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
        #ifdef TARGET_PS2
        gsreg_Begin();
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
        gsreg_SetZBufferUpdate(FALSE);
        gsreg_SetClamping( FALSE );
        gsreg_End();
        #endif

        // set the color
        color.Set( 255,192,192,255 );

        // set the alpha
        if( m_Age > (m_MaxAge - 1.0f) )
            color.A = (u8)((m_MaxAge - m_Age) * 128);

        // set the core
        draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(s_FlareCore).GetXBitmap() );

        // render a few copies of it
        vector3 Pos = GetRenderPos();
        draw_SpriteUV( Pos, vector2(1.00f, 1.00f), vector2(0,0), vector2(1,1), color,  m_Age );
        draw_SpriteUV( Pos, vector2(0.8f, 0.8f), vector2(0,0), vector2(1,1), color, -m_Age );
        draw_SpriteUV( Pos, vector2(0.6f, 0.6f), vector2(0,0), vector2(1,1), color,  m_Age + R_90 );

        tgl.PostFX.AddFX( Pos, vector2( 2.0f, 2.0f ), xcolor( 225, 225, 255, 38 ) );

        draw_End();
    }
}

//==============================================================================

void grenade::FlashPlayers( void )
{
    collider       Segment;
    player_object* pPlayer;

    ObjMgr.Select( object::ATTR_PLAYER, 
                   m_WorldPos,
                   FLASH_RADIUS );

    while( (pPlayer = (player_object*)ObjMgr.GetNextSelected()) )
    {
        vector3 ViewPos;
        radian3 ViewRot;
        f32     ViewFOV;

        // Check LOS
        Segment.RaySetup( this, m_WorldPos, pPlayer->GetBlendPainPoint(), -1, TRUE );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Segment );

        if( Segment.HasCollided() )
        {
            // This player cannot see the flash, next!
            continue;
        }        

        // get the view
        pPlayer->Get1stPersonView( ViewPos, ViewRot, ViewFOV );

        vector3 ViewDir(0.0f, 0.0f, 1.0f);
        ViewDir.Rotate( ViewRot );

        vector3 TargDir = m_WorldPos - pPlayer->GetPosition();
        TargDir.Normalize();

        // compute the flash value as dot product of view dir and target dir
        pPlayer->FlashScreen( x_abs(TargDir.Dot(ViewDir) * MAX_FLASH_DURATION), XCOLOR_WHITE );
    }

    ObjMgr.ClearSelection();
}

//==============================================================================

void grenade::Explode( const vector3& Normal )
{
    pain::type PainType = pain::PAIN_END_OF_LIST;

    // What shall we do for this grenade?
    switch( m_Type )
    {
        case TYPE_LAUNCHED:
            PainType = pain::WEAPON_GRENADE;
            break;

        case TYPE_BASIC:
            PainType = pain::WEAPON_GRENADE_HAND;
            break;
        
        case TYPE_FLASH:
            FlashPlayers();
            PainType = pain::WEAPON_GRENADE_FLASH;
            break;

        case TYPE_CONCUSSION:
            PainType = pain::WEAPON_GRENADE_CONCUSSION;
            break;

        default:
            // Do not assert here.  Flares go thru here.
            break;
    }

    // Boom!
    if( PainType != pain::PAIN_END_OF_LIST )
    {
        SpawnExplosion( PainType,
                        GetRenderPos(), 
                        Normal,
                        m_OriginID,
                        m_ObjectHitID );
    }

    if( m_SmokeEffect.IsCreated() )
        m_SmokeEffect.SetEnabled( FALSE );

    m_HasExploded = TRUE;
    m_Velocity(0,0,0);
}

//==============================================================================

void grenade::DestroyFlare( void )
{
    // flare grenades should disappear when hit
    m_Age = MAX_FLARE_DURATION;
}


