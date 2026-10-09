//==============================================================================
//
//  Beacon.hpp
//
//==============================================================================

#ifndef BEACON_HPP
#define BEACON_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class beacon : public object
{

public:

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );

    void        OnAdd           ( void );                   
    void        OnPain          ( const pain& Pain );

    void        OnCollide       ( u32 AttrBits, collider& Collider );
    
    void        Initialize      ( const vector3& WorldPos,
                                  const vector3& Normal,
                                        u32      TeamBits );

    void        Initialize      ( const vector3& WorldPos,
                                        u32      TeamBits,
                                        object::id Owner = obj_mgr::NullID );

    void        ToggleMode      ( void );
    void        SetPosition     ( const vector3& Position );

    void        DebugRender     ( void );
    void        Render          ( void );

    f32         GetHealth       ( void ) const { return( 1.0f ); }
    f32         GetPainRadius   ( void ) const;
    vector3     GetPainPoint    ( void ) const;
    
    xbool       IsMarker        ( void ) const { return( m_Mode == 2 ); }

protected:

    radian3         m_WorldRot;
    vector3         m_Normal;
    xbool           m_Dead;
    s32             m_Mode;
    object::id      m_Owner;
    shape_instance  m_Shape; 
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     BeaconCreator( void );

//==============================================================================
#endif // BEACON_HPP
//==============================================================================
