//==============================================================================
//
//  AutoAim.hpp
//
//==============================================================================

#ifndef AUTOAIM_HPP
#define AUTOAIM_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "Objects\Player\PlayerObject.hpp"

//==============================================================================
//  VARIABLES
//==============================================================================

extern xbool UseAutoAimHelp;
extern xbool UseAutoAimHint;

//==============================================================================
//  PROTOTYPES
//==============================================================================

void  RenderAutoAimHint ( void );
xbool GetAutoAimPoint   ( const player_object* pPlayer, 
                                vector3&       Dir, 
                                vector3&       Point, 
                                object::id&    TargetID );
xbool ComputeAutoAim    ( const player_object* pPlayer, 
                          const object*        pTarget,
                                xbool          Blend,
                                f32&           Score,
                                vector3&       TargetCenter,
                                vector3&       AimDir,
                                vector3&       AimPos,
                                vector3&       ArcPivot );

//==============================================================================
#endif // AUTOAIM_HPP
//==============================================================================
           
           

