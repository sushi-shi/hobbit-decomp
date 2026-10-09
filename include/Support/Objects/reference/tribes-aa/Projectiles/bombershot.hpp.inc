//==============================================================================
//
//  BomberShot.hpp
//
//==============================================================================

#ifndef BOMBER_SHOT_HPP
#define BOMBER_SHOT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     BomberShotCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class bomber_shot : public linear
{

public:
        void        Setup           ( void );

        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnAdd           ( void );

        void        Initialize      ( const matrix4&   L2W, 
                                      const vector3&   Movement,
                                            u32        TeamBits,
                                            object::id OriginID,
                                            object::id ExcludeID ) ;

        void        Render          ( void );
        update_code Impact          ( vector3& Point, vector3& Normal );

static  void        BeginRender     ( void );
static  void        EndRender       ( void );
    
static  f32         GetMotionInherit    ( void ) { return 0.9f; }
static  f32         GetMuzzleSpeed      ( void ) { return 135.0f; }
static  f32         GetMaxAge           ( void ) { return 3.0f; }
};

//==============================================================================
#endif // BOMBER_SHOT_HPP
//==============================================================================
