//==============================================================================
//
//  FlipFlop.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "FlipFlop.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "..\AADS\Globals.hpp"
#include "GameMgr\GameMgr.hpp"
#include "NetLib\BitStream.hpp"
#include "LabelSets/Tribes2Types.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   FlipFlopCreator;

obj_type_info   FlipFlopTypeInfo( object::TYPE_FLIPFLOP,
                                  "FlipFlop", 
                                  FlipFlopCreator, 
                                  object::ATTR_SOLID_DYNAMIC );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* FlipFlopCreator( void )
{
    return( (object*)(new flipflop) );
}

//==============================================================================

object::update_code flipflop::OnAdvanceLogic( f32 DeltaTime )
{    
    m_Shape.Advance( DeltaTime );
    m_TeamBits = GameMgr.GetSwitchBits( m_Switch );
    return( NO_UPDATE );
}

//==============================================================================

void flipflop::Initialize( const vector3& WorldPos,
                           const radian3& WorldRot,
                                 s32      Switch )
{   
    // Set the L2W.
    m_L2W.Identity();
    m_L2W.SetRotation   ( WorldRot );
    m_L2W.SetTranslation( WorldPos );

    // Set the switch circuit.
    m_Switch = Switch;

    // Setup the shape instance.
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_SWITCH ) );
    m_Shape.SetPos( WorldPos );
    m_Shape.SetRot( WorldRot );

    // Turn off the texture animation and manually set the frame.
    m_Shape.SetTextureAnimFPS( 0 );
    m_Shape.SetTextureFrame( 0 );

    // Set position and bbox.
    m_WorldPos  = WorldPos;
    m_WorldBBox = m_Shape.GetWorldBounds();

	// SB.
	// Shrink the bbox to fix the bug where the player gets stuck in 
	// "Equinox - Capture and Hold mode" - SE Stronghold. at 791.83, 74.19, 1329.34
	// NOTE - This is big enough so that heavy's don't get stuck also.
	// (the alternative is to move the bloody switch out of the nasty corner)
	f32 Shrink = 0.5f ;
	m_WorldBBox.Min.X += Shrink ; 
	m_WorldBBox.Min.Z += Shrink ; 
	m_WorldBBox.Max.X -= Shrink ; 
	m_WorldBBox.Max.Z -= Shrink ; 
}

//==============================================================================

void flipflop::DebugRender( void )
{
    u32 ContextBits = 0x00;
    object::id PlayerID( tgl.PC[tgl.WC].PlayerIndex, -1 );

    // Get the rendering context team bits.
    object* pObject = ObjMgr.GetObjectFromID( PlayerID );
    if( pObject )
        ContextBits = pObject->GetTeamBits();

    xcolor Color = (ContextBits & m_TeamBits) ? XCOLOR_GREEN : XCOLOR_RED;

//  draw_Point( m_WorldPos, Color );

    bbox BBox( vector3(-1,0,-1), vector3(1,2,1) );

    draw_SetL2W( m_L2W );
    draw_BBox  ( BBox, Color );
}

//==============================================================================

void flipflop::Render( void )
{
    // Visible?
    s32 Visible = IsVisible( m_WorldBBox );
    if( !Visible )
        return;

    // Update the TeamBits from the switch settings.
    m_TeamBits = GameMgr.GetSwitchBits( m_Switch );

    // Set the logo image.
    s32 Frame = (s32)m_TeamBits;
    if( m_TeamBits == 0xFFFFFFFF )
        Frame = 0;
    m_Shape.SetTextureFrame( (f32)Frame );

    // Setup fog color for this view and draw!
    m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    if (Visible == view::VISIBLE_PARTIAL)
        m_Shape.Draw( shape::DRAW_FLAG_CLIP | shape::DRAW_FLAG_FOG );
    else
        m_Shape.Draw( shape::DRAW_FLAG_FOG );
}

//==============================================================================

s32 flipflop::GetSwitch( void )
{
    return( m_Switch );
}

//==============================================================================
