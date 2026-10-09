//==============================================================================
//
//  HUNT_Logic.hpp
//
//==============================================================================

#ifndef HUNT_LOGIC_HPP
#define HUNT_LOGIC_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "GameLogic.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  TYPES
//==============================================================================

class hunt_logic : public game_logic
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:

                hunt_logic      ( void );
virtual        ~hunt_logic      ( void );

virtual void    BeginGame       ( void );
virtual void    Connect         ( s32 PlayerIndex );
virtual void    EnterGame       ( s32 PlayerIndex );
virtual void    PlayerDied      ( const pain& Pain );
virtual void    PlayerSpawned   ( object::id PlayerID );
virtual void    FlagTouched     ( object::id FlagID,   object::id PlayerID );
virtual xbool   FlagHitPlayer   ( object::id FlagID,   object::id PlayerID );
virtual void    NexusTouched    ( object::id PlayerID );
virtual void    ThrowFlags      ( object::id PlayerID );  

virtual void    EnforceBounds   ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------

protected:
       
        void    DropFlags       ( const vector3&   Position, 
                                  const vector3&   Velocity,
                                        object::id OriginID,
                                        s32        Count );
    
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern hunt_logic  HUNT_Logic;

//==============================================================================
#endif // HUNT_LOGIC_HPP
//==============================================================================
