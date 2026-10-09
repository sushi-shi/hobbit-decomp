//==============================================================================
//
//  BomberShot.cpp
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

#include "BomberShot.hpp"
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

static    s32       HeadTex = -1;        // VRAM texture ID for the core

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BomberShotCreator;

obj_type_info   BomberShotClassInfo( object::TYPE_BOMBERSHOT, 
                                 "BomberShot", 
                                 BomberShotCreator, 
                                 0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BomberShotCreator( void )
{
    return( (object*)(new bomber_shot) );
}

//==============================================================================

void bomber_shot::Setup( void )
{
    m_Speed = GetMuzzleSpeed();
    m_Life  = GetMaxAge();
    m_SoundID    = SOUNDTYPE_NONE; 

    m_MotionInherit = GetMotionInherit();
}

//==============================================================================

void bomber_shot::Initialize( const matrix4&    L2W, 
                              const vector3&    Movement, 
                                    u32         TeamBits, 
                                    object::id  OriginID,
                                    object::id  ExcludeID )
{
    Setup();

    // call default initializer
    linear::Initialize( L2W, Movement, TeamBits, OriginID, ExcludeID );
}

//==============================================================================

void bomber_shot::OnAcceptInit( const bitstream& BitStream )
{
    Setup();

    // call default OnAcceptInit
    linear::OnAcceptInit( BitStream );
}

//==============================================================================

void bomber_shot::OnAdd( void )
{
    linear::OnAdd();

    audio_Play( SFX_TURRET_AATURRET_FIRE, &m_WorldPos, AUDFLAG_LOCALIZED );

    // Setup point light
    m_PointLightID = ptlight_Create( m_WorldPos, POINT_LIGHT_RADIUS, POINT_LIGHT_COLOR );
}

//==============================================================================

void bomber_shot::BeginRender( void )
{
    if ( HeadTex == -1 )
    {
        HeadTex = tgl.GameShapeManager.GetTextureManager().GetTextureIndexByName("[A]Spot") ;
    }
    // render the head first
    draw_Begin( DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA );
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_SetZBufferUpdate(FALSE);
    gsreg_End();
#endif

    draw_SetTexture( tgl.GameShapeManager.GetTextureManager().GetTexture(HeadTex).GetXBitmap() );
}

//==============================================================================

void bomber_shot::EndRender( void )
{
    draw_End();
#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetZBufferUpdate(TRUE);
    gsreg_End();
#endif

}

//==============================================================================

void bomber_shot::Render( void )
{

    xcolor      Color;

    Color = XCOLOR_WHITE;
    draw_SpriteUV( m_WorldPos, vector2(0.25f, 0.25f), vector2(0,0), vector2(1,1), XCOLOR_WHITE );
    tgl.PostFX.AddFX( m_WorldPos, vector2(1.0f,1.0f), XCOLOR_WHITE );
}

//==============================================================================

object::update_code bomber_shot::Impact( vector3& Point, vector3& Normal )
{
    if( m_Age < GetMaxAge() )
    {
        SpawnExplosion( pain::WEAPON_BOMBER_GUN,
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

