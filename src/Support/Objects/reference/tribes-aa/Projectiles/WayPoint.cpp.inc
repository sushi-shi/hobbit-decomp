//==============================================================================
//
//  WayPoint.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "WayPoint.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "..\AADS\Globals.hpp"
#include "NetLib\BitStream.hpp"
#include "GameMgr\GameMgr.hpp"
#include "HUD\hud_Icons.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   WayPointCreator;

obj_type_info   WayPointTypeInfo( object::TYPE_WAYPOINT,
                                  "WayPoint", 
                                  WayPointCreator, 
                                  0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* WayPointCreator( void )
{
    return( (object*)(new waypoint) );
}

//==============================================================================

void waypoint::Initialize( const vector3& WorldPos,
                                 s32      Switch,
                           const xwchar*  pLabel )
{   
    // Set position and bbox.
    m_WorldPos = WorldPos;
    m_WorldBBox( m_WorldPos, m_WorldPos );

    // Set the switch.
    m_Switch = Switch;

    // Set the label.
    if( pLabel )
    {
        x_wstrncpy( m_Label, pLabel, 31 );
        m_Label[31] = '\0';
    }
    else
    {
        m_Label[0] = '\0';
    }

    // Get the team bits.
    m_TeamBits = GameMgr.GetSwitchBits( m_Switch );

    // Not hidden.
    m_Hidden = FALSE;

    // Labeled?
    if( m_Hidden || (m_Label[0] == '\0') )
        m_AttrBits &= ~ATTR_LABELED;
    else
        m_AttrBits |=  ATTR_LABELED;
}

//==============================================================================

void waypoint::SetHidden( xbool Hidden )
{
    m_Hidden = Hidden;
    m_DirtyBits = 0x01;

    // Labeled?
    if( m_Hidden || (m_Label[0] == '\0') )
        m_AttrBits &= ~ATTR_LABELED;
    else
        m_AttrBits |=  ATTR_LABELED;
}

//==============================================================================

void waypoint::SetLabel( const xwchar* pLabel )
{
    if( pLabel )
    {
        x_wstrncpy( m_Label, pLabel, 31 );
        m_Label[31] = '\0';
    }
    else
    {
        m_Label[0] = '\0';
    }

    // Labeled?
    if( m_Hidden || (m_Label[0] == '\0') )
        m_AttrBits &= ~ATTR_LABELED;
    else
        m_AttrBits |=  ATTR_LABELED;
}

//==============================================================================

const xwchar* waypoint::GetLabel( void ) const
{
    return( m_Label );
}

//==============================================================================

void waypoint::DebugRender( void )
{
}

//==============================================================================

void waypoint::Render( void )
{   
    if( m_Hidden )
        return;

    m_TeamBits = GameMgr.GetSwitchBits( m_Switch );

    if( (m_Switch >= 4) && 
        (ObjMgr.GetTypeInfo( object::TYPE_FLIPFLOP )->InstanceCount > 0) )
    {
        HudIcons.Add( hud_icons::SWITCHPOINT, m_WorldPos, m_TeamBits );
    }
    else
    {
        HudIcons.Add( hud_icons::WAYPOINT,    m_WorldPos, m_TeamBits );
    }

/*
#ifdef TARGET_PC
    if( m_Hidden )
    {
        draw_Point( m_WorldPos, XCOLOR_BLUE );
        draw_Label( m_WorldPos, XCOLOR_WHITE, xfs( "\n\n%s", m_Label ) );
    }
#endif
*/
/*
    draw_Label( m_WorldPos, Color, xfs( "\n\n%d:%08X", m_Switch, m_TeamBits ) );
    x_printfxy( 10, 1, "%08X", ContextBits );
*/
}

//==============================================================================

object::update_code waypoint::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    (void)SecInPast;

    // Read hidden bit.
    m_Hidden = BitStream.ReadFlag();

    // Labeled?
    if( m_Hidden || (m_Label[0] == '\0') )
        m_AttrBits &= ~ATTR_LABELED;
    else
        m_AttrBits |=  ATTR_LABELED;

    return( NO_UPDATE );
}

//==============================================================================

void waypoint::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    // Write hidden bit.
    if( DirtyBits )
        BitStream.WriteFlag( m_Hidden );

    DirtyBits = 0;
}

//==============================================================================

void waypoint::OnAcceptInit( const bitstream& BitStream )
{
    BitStream.ReadVector ( m_WorldPos );
    BitStream.ReadS32    ( m_Switch, 5 );
    BitStream.ReadWString( m_Label );
    m_Hidden = BitStream.ReadFlag();

    // Labeled?
    if( m_Hidden || (m_Label[0] == '\0') )
        m_AttrBits &= ~ATTR_LABELED;
    else
        m_AttrBits |=  ATTR_LABELED;

    m_WorldBBox( m_WorldPos, m_WorldPos );
    m_TeamBits = GameMgr.GetSwitchBits( m_Switch );
}

//==============================================================================

void waypoint::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteVector ( m_WorldPos );
    BitStream.WriteS32    ( m_Switch, 5 );
    BitStream.WriteWString( m_Label );
    BitStream.WriteFlag   ( m_Hidden );
}

//==============================================================================
