//==============================================================================
//
//  Blaster.cpp
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

#include "Objects\Player\PlayerObject.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "netlib/bitstream.hpp"

#include "Blaster.hpp"
#include "textures.hpp"

//==============================================================================
//  STATICS
//==============================================================================

//static    xbitmap   Blastex, Blastrail, BlastHead;

#define POINT_LIGHT_RADIUS  4.0f
#define POINT_LIGHT_COLOR   xcolor(255,200,200,128)

#define ARMTIME         0.4f
#define TRAILDURATION   0.15f

#define BOUNCES         4

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BlasterCreator;

obj_type_info   BlasterClassInfo( object::TYPE_BLASTER, 
                                  "Blaster", 
                                  BlasterCreator, 
                                  0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

//==============================================================================
//  Global initialization (called only once ever)

void BlasterInit( void )
{
    // load the textures
}

//==============================================================================

object* BlasterCreator( void )
{
    return( (object*)(new blaster) );
}

//==============================================================================

void blaster::Setup( void )
{
    const player_object::weapon_info& WeaponInfo
        = player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_BLASTER );
    
    m_Speed = WeaponInfo.MuzzleSpeed;
    m_Life  = WeaponInfo.MaxAge;
    
    m_SoundID = SFX_WEAPONS_PLASMA_PROJECTILE_LOOP;

    m_CurLeg  = 0;
    m_Bounces = 0;
    m_LastStartLeg  = 0;
    m_OriginalStart = TRUE;
}

//==============================================================================

void blaster::Initialize( const matrix4& L2W, u32 TeamBits, object::id OriginID )
{
    Setup();

    // call default initializer
    linear::Initialize( L2W, TeamBits, OriginID );

    AddLeg( m_Start, TRUE );

    tgl.PostFX.AddFX( m_Start, vector2(0.5f, 0.5f), xcolor(255,255,255,127), 0.25f );
}

//==============================================================================

void blaster::OnAcceptInit( const bitstream& BitStream )
{
    s32   i, j;
    s32   StartPoints = 0;
    xbool GotBounces = FALSE;
    s32   Index[ MAX_LEGS ];

    Setup();

    // call default OnAcceptInit
    linear::OnAcceptInit( BitStream );
             
    // Now, let the blaster fun and games begin!

    while( BitStream.ReadFlag() )
    {
        GotBounces = TRUE;
        BitStream.ReadRangedS32 ( i, 0, MAX_LEGS );
        BitStream.ReadWorldPosCM( m_Legs[i].m_Pos );
        BitStream.ReadRangedF32 ( m_Legs[i].m_Life, 3, 0.0f, TRAILDURATION );
        m_Legs[i].m_Bounce = TRUE;
        Index[StartPoints] = i;
        StartPoints++;
    }

    if( GotBounces )
    {
        BitStream.ReadRangedS32( m_LastStartLeg, 0, MAX_LEGS );
        if( BitStream.ReadFlag() )
        {
            // Original position is still in m_Legs[0].  Overwrite it with the
            // current muzzle position (from class linear).
            if( m_Blending )
            {
                m_Legs[0].m_Pos = m_BlendPos;
                tgl.PostFX.AddFX( m_BlendPos, vector2(0.5f, 0.5f), xcolor(255,255,255,127), 0.25f );
            }
        }

        // 
        m_Legs[m_LastStartLeg].m_Bounce = TRUE;
        m_Legs[m_LastStartLeg].m_Life   = TRAILDURATION;
        m_Legs[m_LastStartLeg].m_Pos    = m_Start;
        Index[StartPoints] = m_LastStartLeg;
        StartPoints++;

        // Since we have bounced, there is no more blending.
        m_Blending = FALSE;
        m_BlendVector( 0, 0, 0 );

        // Initialize the current leg index.
        m_CurLeg = m_LastStartLeg+1;

        // Time to do some interpolation between bounce points.
        for( i = 1; i < StartPoints; i++ )
        {
            f32 Scale = 1.0f / (f32)(Index[i] -  Index[i-1]);

            vector3 StepP = m_Legs[Index[i]].m_Pos - m_Legs[Index[i-1]].m_Pos;
            StepP *= Scale;

            f32 StepL = m_Legs[Index[i]].m_Life - m_Legs[Index[i-1]].m_Life;
            StepL *= Scale;

            for( j = Index[i-1]+1; j < Index[i]; j++ )
            {
                m_Legs[j].m_Pos  = m_Legs[j-1].m_Pos  + StepP;
                m_Legs[j].m_Life = m_Legs[j-1].m_Life + StepL;
            }
        }
    }
    else
    {
        // There were no bounces.  Blending should behave normally.
        AddLeg( m_Start + m_BlendVector );
        tgl.PostFX.AddFX( m_Start + m_BlendVector, vector2(0.5f, 0.5f), xcolor(255,255,255,127), 0.25f );
    }
}

//==============================================================================

void blaster::OnProvideInit( bitstream& BitStream )
{
    xbool GotBounces = FALSE;
    s32   i;

    linear::OnProvideInit( BitStream );

    for( i = 0; i < m_LastStartLeg; i++ )
    {
        if( m_Legs[i].m_Bounce )
        {
            GotBounces = TRUE;
            BitStream.WriteFlag( TRUE );
            BitStream.WriteRangedS32 ( i, 0, MAX_LEGS );
            BitStream.WriteWorldPosCM( m_Legs[i].m_Pos );
            BitStream.WriteRangedF32 ( m_Legs[i].m_Life, 3, 0.0f, TRAILDURATION );
        }
    }

    BitStream.WriteFlag( FALSE );

    if( GotBounces )
    {
        BitStream.WriteRangedS32( m_LastStartLeg, 0, MAX_LEGS );
        BitStream.WriteFlag( m_OriginalStart );
    }
}

//==============================================================================

void blaster::OnAdd( void )
{
    linear::OnAdd();

    vector3 Pos = GetRenderPos();

    // Setup point light
    m_PointLightID = ptlight_Create( Pos, POINT_LIGHT_RADIUS, POINT_LIGHT_COLOR );
    m_HasExploded  = FALSE;   

    // add a lingering glow at the fire point
    // tgl.PostFX.AddFX( Pos, vector2(0.7f, 0.7f), xcolor(255,225,225,60), 0.5f );
    
    // Determine if a local player fired this blaster
    xbool Local = FALSE;
    object* pCreator = ObjMgr.GetObjectFromID( m_OriginID );
    if( pCreator && (pCreator->GetType() == object::TYPE_PLAYER) )
    {
        player_object* pPlayer = (player_object*)pCreator;
        Local = pPlayer->IsLocal();
    }

    if( Local )
    {
        audio_Play( SFX_WEAPONS_BLASTER_FIRE );
    }
    else
    {
        audio_Play( SFX_WEAPONS_BLASTER_FIRE, 
                    m_Blending ? &m_BlendPos : &m_WorldPos );
    }

    if( m_CurLeg > 1 )
        audio_Play( SFX_WEAPONS_BLASTER_PROJECTILE_RICOCHET, &m_Start );

    // misc.
    const player_object::weapon_info& WeaponInfo
        = player_object::GetWeaponInfo(
            player_object::INVENT_TYPE_WEAPON_BLASTER );

    m_Speed       = WeaponInfo.MuzzleSpeed;
    m_Life        = WeaponInfo.MaxAge;
    m_HasImpacted = FALSE;
}         
         
//==============================================================================

void blaster::BeginRender( void )
{
    /*
    //draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif
    draw_SetTexture( Blastrail );
    */
}

//==============================================================================

void blaster::EndRender( void )
{
    /*
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif
    */
}

//==============================================================================

void blaster::Render( void )
{
    /*
    xcolor      Color1, Color2;
    xcolor      GlowColor1, GlowColor2;
    vector3     TmpMov, TmpVec;

    if( m_HasImpacted )
        return;

    vector3 Pos = GetRenderPos();

    // update audio pos
    // audio_SetPosition( m_SfxHandle, &Pos );

    Color1 = XCOLOR_WHITE;
    Color2 = XCOLOR_WHITE;
    GlowColor1 = XCOLOR_WHITE;
    GlowColor2 = XCOLOR_WHITE;

    Color1.A = 128;

    // render head

    if ( !m_HasExploded )
    {
        draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
        draw_SetTexture( BlastHead );
        draw_Sprite( Pos, vector2(0.25f, 0.25f), Color1 );
        draw_End();
    }

    // render tail
    s32 i;
    
    draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );

    draw_SetTexture( Blastrail );

    // initialize our first leg
    Color1.A = (u8)(128.0f * (m_Legs[0].m_Life / TRAILDURATION));
    GlowColor1.A = (u8)(Color1.A * 0.29f);

    for ( i = 0; i < (m_CurLeg-1); i++ )
    {
        Color2.A = (u8)(128.0f * (m_Legs[i+1].m_Life / TRAILDURATION));
        GlowColor2.A = (u8)(Color2.A * 0.29f);
        draw_OrientedQuad( m_Legs[i].m_Pos, m_Legs[i+1].m_Pos, vector2(0,0), vector2(1,1), Color1, Color2, 0.10f );

        //tgl.PostFX.AddFX( m_Legs[i].m_Pos, m_Legs[i+1].m_Pos, 0.15f, xcolor(255,225,225,(u8)(48.0f * (m_Legs[0].m_Life / TRAILDURATION))) );
        //tgl.PostFX.AddFX( m_Legs[i].m_Pos, vector2(0.3f, 0.3f), xcolor(255,225,225,(u8)(38.0f * (m_Legs[i+1].m_Life / TRAILDURATION))) );
        //tgl.PostFX.AddFX( m_Legs[i].m_Pos, vector2(0.3f, 0.2f), xcolor(255,225,225,38) );
        
        //if ( i == 0 )
            //tgl.PostFX.AddFX( m_Legs[0].m_Pos, vector2(0.3f, 0.3f), xcolor(255,225,225,(u8)(38.0f * (m_Legs[0].m_Life / TRAILDURATION))) );
        //draw_OrientedQuad( m_Legs[i].m_Pos, m_Legs[i+1].m_Pos, vector2(0,0), vector2(1,1), GlowColor1, GlowColor2, 0.30f );
        
        // the alpha at the end of this leg becomes the alpha at the start of the next
        Color1.A = Color2.A;
        GlowColor1.A = GlowColor2.A;
    }

    // render the last leg (to present position)
    if ( !m_HasExploded )
    {
        Color2.A = 255;
        draw_OrientedQuad( m_Legs[m_CurLeg-1].m_Pos, Pos, vector2(0,0), vector2(1,1), Color1, Color2, 0.10f );

        // render core
        draw_SetTexture( Blastex );
        TmpMov = m_Movement;
        TmpMov.Normalize();
    
        TmpVec = ( Pos - m_Start );

        TmpMov *= 6.0f;

        if ( TmpVec.LengthSquared() < (6.0f * 6.0f) )
            draw_OrientedQuad( m_Start + m_BlendVector, Pos, vector2(0,0), vector2(1,1), XCOLOR_WHITE, XCOLOR_WHITE, 0.10f );    
        else
            draw_OrientedQuad( Pos - TmpMov, Pos, vector2(0,0), vector2(1,1), XCOLOR_WHITE, XCOLOR_WHITE, 0.08f );    
    }
    
    draw_End();
    */
}

//==============================================================================

object::update_code blaster::Impact( vector3& Point, vector3& Normal )
{
    m_ExcludeID = ObjMgr.NullID;
    m_Blending  = FALSE;

    // if we've already exploded, no need for impact logic
    if( m_HasExploded )
    {
        return UPDATE;
    }

    if( m_Life > 0.001f )
    {
        /*
        // for the clients...
        if( m_HasExploded && !tgl.ServerPresent )
        {
            StopMoving( Point );
            return UPDATE;            
        }
        */
    
        // for the server...
        if( (m_ImpactObjectAttr & ATTR_DAMAGEABLE) ||
            (m_Bounces > BOUNCES) ||
            (((player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_BLASTER ).MaxAge) - (m_Life - m_Age)) > ARMTIME) )
        {
            if( tgl.ServerPresent )
            {
                SpawnExplosion( pain::WEAPON_BLASTER,
                                Point, 
                                Normal,
                                m_OriginID, 
                                m_ImpactObjectID );

                StopMoving( Point );
            }
            else
            {
                m_HasImpacted = TRUE;    // hide on the client 
                if( m_PointLightID != -1 )
                {
                    ptlight_Destroy( m_PointLightID );
                    m_PointLightID = -1;
                }
            }

            return UPDATE;            
        }
        else
        {
            // Ricochet
            vector3 TmpMov;

            m_Bounces++;
            TmpMov     = -m_Movement;
            m_Movement = m_ImpactPlane.ReflectVector( m_Movement );

            TmpMov.Normalize();

            // calculate next leg and collision surface if there is one
            m_Start    = Point + (TmpMov * 0.1f); // move off the collision plane
            m_WorldPos = m_Start;
            m_Life    -= m_Age;
            m_Age      = 0.0f;

            // add this collision point for drawing the trail
            AddLeg( m_Start, TRUE );
            m_LastStartLeg = m_CurLeg-1;

            audio_Play( SFX_WEAPONS_BLASTER_PROJECTILE_RICOCHET, &m_WorldPos );

            // prevent degenerate rays
            if( m_Life > 0.001f )
            {
                vector3 End = m_Start + (m_Movement * m_Life);

                // See if there is any static collision in this linear's future.
                collider            Ray;
                collider::collision Collision;

                Ray.RaySetup    ( this, m_Start, End );
                ObjMgr.Collide  ( ATTR_SOLID_STATIC, Ray );
                Ray.GetCollision( Collision );

                if( Collision.Collided )
                {
                    m_Limit = m_Life * Collision.T;
                    m_ImpactObjectID = ((object*)(Collision.pObjectHit))->GetObjectID();
                    m_ImpactPlane = Collision.Plane;
                    m_ImpactPoint = Collision.Point;
                }
                else
                {
                    m_Limit = m_Life * 2.0f;
                    m_ImpactPoint = End;
                    m_ImpactPlane.Setup( End, vector3(0,1,0) );
                }

                return( UPDATE );
            }
            else
            {
                StopMoving( Point );
                return( UPDATE );
            }
        }
    }
    else
    {
        StopMoving( Point );
        return( UPDATE );
    }
}

//==============================================================================

void blaster::StopMoving( vector3& Point )
{
    m_Blending = FALSE;

    // this part is for the clients
    AddLeg( Point, TRUE );

    // this fakes out linear
    m_Start = Point;
    m_Age   = 0.0f;
    m_Life  = 0.0f;

    // can't kill the point light but I can shrink it
    ptlight_SetRadius( m_PointLightID, 0.1f );

    m_HasExploded = TRUE;
}

//==============================================================================

void blaster::AddLeg( const vector3& Vert, xbool Bounce )
{
    // slide old legs off the list to make way
    while( m_CurLeg >= MAX_LEGS )
    {
        s32 i;

        m_CurLeg--;
        m_LastStartLeg--;
        m_OriginalStart = FALSE;

        for( i = 0; i < (MAX_LEGS-1); i++ )
            m_Legs[i] = m_Legs[i+1];
    }

    m_Legs[m_CurLeg].m_Pos    = Vert;
    m_Legs[m_CurLeg].m_Life   = TRAILDURATION;
    m_Legs[m_CurLeg].m_Bounce = Bounce;

    m_CurLeg++;
}

//==============================================================================

object::update_code blaster::OnAdvanceLogic( f32 DeltaTime )
{
    s32 i;
    xbool HasDied = FALSE;

    // cycle through each leg of the trail and age it
    for( i = 0; i < m_CurLeg; i++ )
    {
        m_Legs[i].m_Life -= DeltaTime;
        
        if ( m_Legs[i].m_Life < 0.0f )
        {
            m_Legs[i].m_Life = 0.0f;
            HasDied = TRUE;
        }
    }

    // This should keep long legs from drawing long-ass trails
    if( HasDied && (!m_HasExploded) )
        AddLeg( GetRenderPos() );

    // check to see if all nodes are dead
    if( m_HasExploded )
    {
        HasDied = TRUE;

        for( i = 0; i < m_CurLeg; i++ )
        {
            if( m_Legs[i].m_Life != 0.0f )
            {
                HasDied = FALSE;
                break;
            }
        }

        if( HasDied )
            return DESTROY;
    }

    // call base class AdvanceLogic
    return linear::OnAdvanceLogic( DeltaTime );
}

//==============================================================================

void blaster::Init( void )
{
    //LoadProjTexture( Blastex,   "Blaster/[A]BlastProj" );
    //LoadProjTexture( Blastrail, "Blaster/[A]Blastrail" );
    //LoadProjTexture( BlastHead, "Blaster/[A]Head" );
}

//==============================================================================

void blaster::Kill( void )
{
    //vram_Unregister( Blastex );
    //vram_Unregister( Blastrail );
    //vram_Unregister( BlastHead );
    //Blastex.Kill();
    //Blastrail.Kill();
    //BlastHead.Kill();
}

//==============================================================================
