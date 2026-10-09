//==============================================================================
//
//  ShrikeShot.hpp
//
//==============================================================================

#ifndef SHRIKE_SHOT_HPP
#define SHRIKE_SHOT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     ShrikeShotCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class shrike_shot : public linear
{

public:
        void        Setup           ( void );

        void        OnAcceptInit    ( const bitstream& BitStream );
        void        OnProvideInit   (       bitstream& BitStream );
        void        OnAdd           ( void );

        void        Initialize      ( const matrix4&   L2W, 
                                            u32        TeamBits,
                                            s32        ShotIndex,
                                            object::id OriginID,
                                            object::id ExcludeID = obj_mgr::NullID );
        void        Render          ( void );
        update_code Impact          ( vector3& Point, vector3& Normal );

static  void        BeginRender     ( void );
static  void        EndRender       ( void );
    
static  f32         GetMuzzleSpeed      ( void ) { return 250.0f; }
static  f32         GetMaxAge           ( void ) { return 1.75f; }
};

//==============================================================================
#endif // SHRIKE_SHOT_HPP
//==============================================================================
