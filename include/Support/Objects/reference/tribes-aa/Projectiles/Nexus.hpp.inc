//==============================================================================
//
//  Nexus.hpp
//
//==============================================================================

#ifndef NEXUS_HPP
#define NEXUS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class nexus : public object
{

public:

    void            OnAdd           ( void );
    void            OnRemove        ( void );
                    
    void            Initialize      ( const vector3& WorldPos,
                                            f32      ScaleY );
                    
    void            DebugRender     ( void );
    void            Render          ( void );

    update_code     OnAdvanceLogic  ( f32 DeltaTime );

protected:
    f32             m_ScaleY;
    s32             m_AmbientHandle;

    shape_instance  m_ShapeInstance ;
    f32             m_GlowTime ;

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     NexusCreator( void );

//==============================================================================
#endif // NEXUS_HPP
//==============================================================================
