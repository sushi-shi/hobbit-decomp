//==============================================================================
//
//  WayPoint.hpp
//
//==============================================================================

#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class waypoint : public object
{

public:

    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
                                                         
    void        Initialize      ( const vector3& WorldPos,
                                        s32      Switch,
                                  const xwchar*  pLable = NULL );

    void        SetHidden       ( xbool          Hidden );

    void        SetLabel        ( const xwchar*  pLabel );
    const xwchar* GetLabel      ( void ) const;

    void        DebugRender     ( void );
    void        Render          ( void );

protected:
    s32         m_Status;
    s32         m_Switch;
    xbool       m_Hidden;
    xwchar      m_Label[32];
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     WayPointCreator( void );

//==============================================================================
#endif // WAYPOINT_HPP
//==============================================================================
