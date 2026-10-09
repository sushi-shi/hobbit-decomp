//==============================================================================
//
//  Nexus.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Nexus.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "AudioMgr/Audio.hpp"
#include "LabelSets/Tribes2Types.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   NexusCreator;

obj_type_info   NexusTypeInfo( object::TYPE_NEXUS, 
                               "Nexus", 
                               NexusCreator, 
                               object::ATTR_PERMEABLE );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* NexusCreator( void )
{
    return( (object*)(new nexus) );
}

//==============================================================================

void nexus::Initialize( const vector3& WorldPos,
                              f32      ScaleY )
{   
    //vector3 Bottom = WorldPos;
    //vector3 Top    = WorldPos;

    //Bottom.X -= 0.25f;
    //Bottom.Z -= 0.25f;

    //Top.X += 0.25f;
    //Top.Z += 0.25f;
    //Top.Y += 8.0f * ScaleY;

    // Set position and bbox.
    m_WorldPos = WorldPos;
    //m_WorldBBox( Bottom, Top );

    // Setup shape instance
    m_ShapeInstance.SetShape(tgl.GameShapeManager.GetShapeByType(SHAPE_TYPE_SCENERY_NEXUS_EFFECT)) ;
    ASSERT(m_ShapeInstance.GetShape()) ;
    m_ShapeInstance.SetPos(WorldPos) ;
    m_ShapeInstance.SetScale(vector3(1, ScaleY, 1)) ;
    m_WorldBBox = m_ShapeInstance.GetWorldBounds() ;
    m_GlowTime = 0 ;
    m_AmbientHandle = 0;
}

//==============================================================================

void nexus::DebugRender( void )
{
    Render();
}

//==============================================================================

void nexus::Render( void )
{
    //// Is view?
    s32 InView = IsVisible(m_WorldBBox) ;
    if (!InView)
        return ;

    // Setup draw flags
    u32 DrawFlags = shape::DRAW_FLAG_FOG ;

    // Clip?
    if (InView == view::VISIBLE_PARTIAL)
        DrawFlags |= shape::DRAW_FLAG_CLIP ;

    // Calc fog
    m_ShapeInstance.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );

    // Draw the shape
    m_ShapeInstance.Draw(DrawFlags) ;


    //if( !IsVisible( m_WorldBBox ) )
        //return;

    //draw_ClearL2W();

    //xcolor Color;
    //Color.Random();
    //draw_BBox ( m_WorldBBox, Color );
    //draw_Point( m_WorldPos,  Color );
}

//==============================================================================

object::update_code  nexus::OnAdvanceLogic  ( f32 DeltaTime )
{
    // Animate effect
    m_ShapeInstance.Advance(DeltaTime) ;

    // Glow
    m_GlowTime += DeltaTime ;
    f32 Col = (1.0f + x_cos(m_GlowTime * R_360)) * 0.5f ; // Make 0->1
    f32 RGB = 1.0f + Col ;          // Brightens on the PS2
    f32 A   = 0.5f + (Col*0.5f) ;   // Fade in and out
    m_ShapeInstance.SetColor(vector4(RGB, RGB, RGB, A)) ;

    return NO_UPDATE ;
}

//==============================================================================

void nexus::OnAdd( void )
{
    ASSERT(!m_AmbientHandle);
    m_AmbientHandle = audio_Play( SFX_MISC_NEXUS_IDLE,
                                  &m_WorldPos,
                                  AUDFLAG_PERSISTENT );
}

//==============================================================================

void nexus::OnRemove( void )
{
    audio_Stop( m_AmbientHandle );
    m_AmbientHandle = 0;
}

//==============================================================================
