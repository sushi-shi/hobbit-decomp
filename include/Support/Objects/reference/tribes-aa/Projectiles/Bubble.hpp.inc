//==============================================================================
//
//  Bubble.hpp
//
//==============================================================================

#ifndef BUBBLE_HPP
#define BUBBLE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class bubble : public object
{

public:

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
                                                         
    void        Initialize      ( const object::id  AnchorID,
                                        f32         Intensity );

    void        DebugRender     ( void );
    void        Render          ( void );

    object::id  GetAnchorID     ( void );
    void        RestartTimer    ( void );
    void        SetIntensity    (       f32      Intensity );
    void        SetDirection    ( const vector3& Direction );

protected:

    xbool           m_Ready;
    xbool           m_Mobile;
    radian3         m_WorldRot;
    vector3         m_LocalOffset;
    vector3         m_LocalScale;
    vector3         m_Direction;
    f32             m_Intensity;
    object::id      m_AnchorID;

    shape_instance  m_Shape;
    u32             m_DrawFlags;
    f32             m_Timer;
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

void    CreateShieldEffect      ( const object::id ObjectID, 
                                        f32        Intensity );

object* BubbleCreator           ( void );

//==============================================================================
#endif // BUBBLE_HPP
//==============================================================================
