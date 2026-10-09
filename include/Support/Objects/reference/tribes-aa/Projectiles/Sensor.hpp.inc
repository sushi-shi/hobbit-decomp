//==============================================================================
//
//  Sensor.hpp
//
//==============================================================================

#ifndef SENSOR_HPP
#define SENSOR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Asset.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class sensor : public asset
{

//------------------------------------------------------------------------------
//  Public Functions

public:

    update_code OnAdvanceLogic  ( f32 DeltaTime );

    void        Initialize      ( const vector3&   WorldPos,
                                  const radian3&   WorldRot,
                                        s32        Switch,
                                        s32        Power,
                                  const xwchar*    pLabel );

    void        Initialize      ( const vector3&   WorldPos,
                                  const vector3&   Normal,
                                        u32        TeamBits );

    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );

    void        Destroyed       ( object::id OriginID );

    void        Render          ( void );

    f32         GetSenseRadius  ( void ) const;

//------------------------------------------------------------------------------
//  Private Functions

protected:

    void        CommonInit      ( void );

//------------------------------------------------------------------------------
//  Private Data

protected:

    s32         m_Size;
    f32         m_Timer;
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     SensorCreator( void );

//==============================================================================
#endif // SENSOR_HPP
//==============================================================================
