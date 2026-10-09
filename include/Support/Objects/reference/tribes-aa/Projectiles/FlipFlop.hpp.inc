//==============================================================================
//
//  FlipFlop.hpp
//
//==============================================================================

#ifndef FLIPFLOP_HPP
#define FLIPFLOP_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class flipflop : public object
{

public:

    update_code OnAdvanceLogic  ( f32 DeltaTime );
                                                         
    void        Initialize      ( const vector3& WorldPos,
                                  const radian3& WorldRot,
                                        s32      Switch );

    void        DebugRender     ( void );
    void        Render          ( void );

    s32         GetSwitch       ( void );

protected:
    matrix4         m_L2W; 
    s32             m_Switch;
    shape_instance  m_Shape; 
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     FlipFlopCreator( void );

//==============================================================================
#endif // FLIPFLOP_HPP
//==============================================================================
