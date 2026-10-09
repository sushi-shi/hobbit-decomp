//==============================================================================
//
//  Plasma.cpp
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

#include "Plasma.hpp"
#include "textures.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define POINT_LIGHT_RADIUS  4.0f
#define POINT_LIGHT_COLOR   xcolor(242,225,91,128)

#define FIZZLE  2.0f

//==============================================================================
//  LOCAL STORAGE
//==============================================================================

static    s32       CoreTex;        // VRAM texture ID for the core
//static    xbitmap   CoreBmp;
static    s32       s_GlowAlpha = 38;
static    f32       s_GlowSize = 3.0f;

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   PlasmaCreator;

obj_type_info   PlasmaClassInfo( object::TYPE_PLASMA, 
                                 "Plasma", 
                                 PlasmaCreator, 
                                 0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* PlasmaCreator( void )
{
    return( (object*)(new plasma) );
}

//==============================================================================

void plasma::Setup( void )
{
    const player_object::weapon_info& WeaponInfo
        = player_object::GetWeaponInfo(
            player_object::INVENT_TYPE_WEAPON_PLASMA_GUN );
    m_Speed = WeaponInfo.MuzzleSpeed;
    m_Life  = WeaponInfo.MaxAge;

    m_SoundID = SFX_WEAPONS_PLASMA_PROJECTILE_LOOP;

    m_Rot[0] = x_frand( R_0, R_360 );
    m_Rot[1] = x_frand( R_0, R_360 );
    m_Rot[2] = x_frand( R_0, R_360 );
    
    m_Scale[0] = x_frand( -0.5f, 0.5f );
    m_Scale[1] = x_frand( -0.5f, 0.5f );
    m_Scale[2] = x_frand( -0.5f, 0.5f );
    
    m_PingDir[0] = x_frand( 0.01f, 0.03f );
    m_PingDir[1] = x_frand( 0.01f, 0.03f );
    m_PingDir[2] = x_frand( 0.01f, 0.03f );
}

//==============================================================================

void plasma::Initialize( const matrix4& L2W, u32 TeamBits, object::id OriginID )
{
    Setup();

    // call default initializer
    linear::Initialize( L2W, TeamBits, OriginID );
}

//==============================================================================

void plasma::OnAcceptInit( const bitstream& BitStream )
{
    Setup();

    // call default OnAcceptInit
    linear::OnAcceptInit( BitStream );
}

//==============================================================================

void plasma::OnAdd( void )
{
    linear::OnAdd();
    vector3 Pos = GetRenderPos();

    // Determine if a local player fired this blaster
    xbool Local = FALSE;
    object* pCreator = ObjMgr.GetObjectFromID( m_OriginID );
    if( pCreator && (pCreator->GetType() == object::TYPE_PLAYER) )
    {
        player_object* pPlayer = (player_object*)pCreator;
        Local = pPlayer->IsLocal();
    }

    // Play firing sound
    if( Local )
    {
        audio_Play( SFX_WEAPONS_PLASMA_FIRE );
    }
    else
    {
        audio_Play( SFX_WEAPONS_PLASMA_FIRE, &Pos );
    }

    // Setup point light
    m_PointLightID = ptlight_Create( Pos, POINT_LIGHT_RADIUS, POINT_LIGHT_COLOR );
}

//==============================================================================

void plasma::BeginRender( void )
{
    /*
    draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif

    // render core
    draw_SetTexture( CoreBmp );
    */
}

//==============================================================================

void plasma::EndRender( void )
{
    /*
    draw_End();

#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif
    */
}

//==============================================================================

void plasma::Render( void )
{
    /*
    vector2 Size;
    xcolor  Color = XCOLOR_WHITE;
    vector3 Pos   = GetRenderPos();

    if( m_Age < FIZZLE )
        Color.A = 255;
    else
        Color.A = (u32)( 255 * (1.0f - ( (m_Age - FIZZLE) / (player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_PLASMA_GUN ).MaxAge - FIZZLE) )) );

    Size = vector2(0.7f,0.7f) + vector2(m_Scale[0], m_Scale[0]) ;
    draw_SpriteUV( Pos, Size, vector2(0,0), vector2(0.5f,0.5f), Color, m_Rot[0] );

    // render flares
    Size = vector2(1.2f,1.2f) + vector2(m_Scale[0], m_Scale[0]) ;
    draw_SpriteUV( Pos, Size, vector2(0.5f,0.0f), vector2(1.0f,0.5f), Color, m_Rot[0] );
    Size = vector2(1.2f,1.2f) + vector2(m_Scale[1], m_Scale[1]);
    draw_SpriteUV( Pos, Size, vector2(0.0f,0.5f), vector2(0.5f,1.0f), Color, m_Rot[1] );
    Size = vector2(1.2f,1.2f) + vector2(m_Scale[2], m_Scale[2]);
    draw_SpriteUV( Pos, Size, vector2(0.5f,0.5f), vector2(1.0f,1.0f), Color, m_Rot[2] );

    // add the glow to the postFX
    xcolor GlowColor;
    GlowColor.Set( 255, 255, 166, s_GlowAlpha );
    tgl.PostFX.AddFX( Pos, vector2(s_GlowSize, s_GlowSize), GlowColor );
    tgl.PostFX.AddFX( Pos, vector2(s_GlowSize / 2.0f, s_GlowSize / 2.0f), GlowColor );

    // update rotations
    m_Rot[0] += 0.01f;
    m_Rot[1] -= 0.012f;
    m_Rot[2] += 0.015f;

    // ping pong scale
    s32 i;

    for( i = 0; i < 3; i++ )
    {
        m_Scale[i] += m_PingDir[i];

        if ( ( m_Scale[i] >  0.5f ) || 
             ( m_Scale[i] < -0.5f ) )
        {
            m_PingDir[i] = -m_PingDir[i];
        }
    }

    // add the glow to the postFX
    GlowColor.Set( 255, 255, 166, s_GlowAlpha );
    tgl.PostFX.AddFX( Pos, vector2(s_GlowSize, s_GlowSize), GlowColor );
    tgl.PostFX.AddFX( Pos, vector2(s_GlowSize / 2.0f, s_GlowSize / 2.0f), GlowColor );
    */
}

//==============================================================================

object::update_code plasma::Impact( vector3& Point, vector3& Normal )
{
    if( m_Age < player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_PLASMA_GUN ).MaxAge )
    {
        SpawnExplosion( pain::WEAPON_PLASMA,
                        Point, 
                        Normal,
                        m_OriginID,
                        m_ImpactObjectID );

        return( DESTROY );
    }
    else
        return( UPDATE );
}

//==============================================================================

void plasma::Init( void )
{
//    CoreTex = LoadProjTexture( CoreBmp, "Plasma/[A]Plasma" );
}

//==============================================================================

void plasma::Kill( void )
{
//    vram_Unregister( CoreBmp );
//    CoreBmp.Kill();
}

//==============================================================================
