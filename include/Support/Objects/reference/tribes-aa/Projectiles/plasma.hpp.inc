//==============================================================================
//
//  Plasma.hpp
//
//==============================================================================

#ifndef PLASMA_HPP
#define PLASMA_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     PlasmaCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class plasma : public linear
{
    radian      m_Rot[3];       // rotations for flare
    f32         m_Scale[3];     // scaling
    f32         m_PingDir[3];   // pingpong direction

public:
        void        Setup           ( void );

        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnAdd           ( void );

        void        Initialize      ( const matrix4&   L2W, 
                                            u32        TeamBits,
                                            object::id OriginID );
        void        Render          ( void );
        update_code Impact          ( vector3& Point, vector3& Normal );

static  void        Init            ( void );
static  void        Kill            ( void );
static  void        BeginRender     ( void );
static  void        EndRender       ( void );
};

//==============================================================================
#endif // PLASMA_HPP
//==============================================================================
