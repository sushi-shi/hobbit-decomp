//==============================================================================
//
//  Generator.hpp
//
//==============================================================================

#ifndef GENERATOR_HPP
#define GENERATOR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Asset.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class generator : public asset
{

//------------------------------------------------------------------------------
//  Public Functions

public:

                generator       ( void );

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );

    void        Initialize      ( const vector3& WorldPos,
                                  const radian3& WorldRot,
                                        s32      Switch,
                                        s32      Power,
                                  const xwchar*  pLabel );

    void        Render          ( void );
    void        OnPain          ( const pain& Pain );

//------------------------------------------------------------------------------
//  Private Functions

protected:

virtual void    PowerOff        ( void );
virtual void    PowerOn         ( void );

virtual void    Disabled        ( object::id OriginID );
virtual void    Destroyed       ( object::id OriginID );
virtual void    Enabled         ( object::id OriginID );

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     GeneratorCreator( void );

//==============================================================================
#endif // GENERATOR_HPP
//==============================================================================
