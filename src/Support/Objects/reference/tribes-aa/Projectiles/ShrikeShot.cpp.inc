//==============================================================================
//
//  ShrikeShot.cpp
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
#include "NetLib\BitStream.hpp"

#include "ShrikeShot.hpp"
#include "textures.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define POINT_LIGHT_RADIUS  4.0f
#define POINT_LIGHT_COLOR   xcolor(242,64,225,128)

#define FIZZLE  2.0f

//==============================================================================
//  LOCAL STORAGE
//==============================================================================

static    s32       BeamTex = -1;        // VRAM texture ID for the core
static    s32       HeadTex = -1;        // VRAM texture ID for the core

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   ShrikeShotCreator;

obj_type_info   ShrikeShotClassInfo( object::TYPE_SHRIKESHOT, 
                                     "ShrikeShot", 
                                     ShrikeShotCreator, 
                                     0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* ShrikeShotCreator( void )
{
    return( (object*)(new shrike_shot) );
}

//==============================================================================

void shrike_shot::Setup( void )
{
    m_Speed = GetMuzzleSpeed();
    m_Life  = GetMaxAge();
    m_SoundID = SFX_WEAPONS_SPINFUSOR_PROJECTILE_LP;
}

//==============================================================================

void shrike_shot::Initialize( const matrix4&    L2W, 
                                    u32         TeamBits, 
                                    s32         ShotIndex,
                                    object::id  OriginID, 
                                    object::id  ExcludeID )
{
    m_ShotIndex = ShotIndex;

    Setup();

    // call default initializer
    linear::Initialize( L2W, TeamBits, OriginID, ExcludeID );
}

//==============================================================================

void shrike_shot::OnAcceptInit( const bitstream& BitStream )
{
    Setup();
    m_ShotIndex = BitStream.ReadFlag();
    linear::OnAcceptInit( BitStream );
}

//==============================================================================

void shrike_shot::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteFlag( m_ShotIndex );
    linear::OnProvideInit( BitStream );
}

//==============================================================================

void shrike_shot::OnAdd( void )
{
    vector3 Pos;

    linear::OnAdd();

    if( m_ShotIndex == 0 )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_OriginID );
        if( pObject )
        {
            Pos = pObject->GetBlendPos();
            audio_Play( SFX_VEHICLE_SHRIKE_WEAPON_FIRE, &Pos, AUDFLAG_LOCALIZED );
        }
    }

    // Setup point light
    Pos = GetRenderPos();
    m_PointLightID = ptlight_Create( Pos, POINT_LIGHT_RADIUS, POINT_LIGHT_COLOR );
}

//==============================================================================

void shrike_shot::BeginRender( void )
{
    if ( BeamTex == -1 )
    {
        BeamTex = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName( "[A]ShrikeBolt" );  // extension not needed!
        HeadTex = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName( "[A]ShrikeBoltHead" );  // extension not needed!
    }
}

//==============================================================================

void shrike_shot::EndRender( void )
{
}

//==============================================================================

void shrike_shot::Render( void )
{
    if( m_Age == 0.0f )
        return;

    vector3 Pos   = GetRenderPos();
    xcolor  Color = XCOLOR_WHITE;

    // render the head first
    draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif

    draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(HeadTex).GetXBitmap() );
    draw_SpriteUV( Pos, vector2(0.15f, 0.15f), vector2(0,0), vector2(1,1), XCOLOR_WHITE );
    draw_End();

    draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif
    
    draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(BeamTex).GetXBitmap() );
    // render the body
    vector3 NegMov = -m_Movement;
    NegMov.Normalize();
    NegMov *= 9.0f;

    Color.Set( 225, 65, 225, 0 );
    draw_OrientedQuad( Pos + NegMov, Pos, vector2(0,0), vector2(1,1), Color, XCOLOR_WHITE, 0.15f );

    draw_End();

#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif
}

//==============================================================================

object::update_code shrike_shot::Impact( vector3& Point, vector3& Normal )
{
    if( m_Age < GetMaxAge() )
    {
        SpawnExplosion( pain::WEAPON_SHRIKE_GUN,
                        Point, 
                        Normal,
                        m_ScoreID, 
                        m_ImpactObjectID );

        return( DESTROY );
    }
    else
        return( UPDATE );
}

//==============================================================================

