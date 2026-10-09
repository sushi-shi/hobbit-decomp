//=========================================================================
//
// SHIELD.HPP
//
//=========================================================================
#ifndef SHIELD_HPP
#define SHIELD_HPP

//=========================================================================
// INCLUDES
//=========================================================================

#include <Support/Shape/ShapeInstance.hpp>
#include "NetLib\BitStream.hpp"



//=========================================================================
// CLASSES
//=========================================================================

class shield
{
// Data
private:
    u32             m_DirtyBit ;    // Dirty bit that represents shield (if any)
    
    shape_instance  m_Instance ;    // Shield instance
    
    f32             m_FlashTime ;   // Time for flash
    xcolor          m_Color ;       // Color of flash

    f32             m_Alpha ;       // Current alpha
    f32             m_FadeSpeed ;   // Alpha fade speed


// Constructor/destructor
public:
    shield() ;
    ~shield() ;

// Functions
public:

    // Initialize functions
    void Initialize         ( u32 DirtyBit = 0 ) ;

    // "Look" functions
    void SetShape           ( s32 ShapeType ) ;
    void SetPos             ( vector3 Pos ) ;
    void SetScale           ( vector3 Scale ) ;
    void SetRot             ( radian3 Rot ) ;
    void SetColor           ( xcolor  Col ) ;

    // Logic functions
    void Advance            ( f32 DeltaTime ) ;
    void Render             ( void ) ;
    void Flash              ( u32& DirtyBits, f32 Time, xcolor Color ) ;

    // Net functions
    void OnProvideUpdate    (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void OnAcceptUpdate     ( const bitstream& BitStream, f32 SecInPast ) ;
} ;



//=========================================================================

//=========================================================================
#endif //   #ifndef SHIELD_HPP
//=========================================================================
