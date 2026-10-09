//==============================================================================
//
//  TDM_Logic.hpp
//
//==============================================================================

#ifndef TDM_LOGIC_HPP
#define TDM_LOGIC_HPP

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

class tdm_logic : public game_logic
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:

                tdm_logic       ( void );
virtual        ~tdm_logic       ( void );

virtual void    Initialize      ( void );

virtual void    AdvanceTime     ( f32 DeltaTime );
virtual void    BeginGame       ( void );

virtual void    Connect         ( s32 PlayerIndex );
virtual void    Disconnect      ( s32 PlayerIndex );

virtual void    PlayerDied      ( const pain& Pain );
virtual void    SwitchTouched   ( object::id SwitchID, object::id PlayerID );

virtual void    EnforceBounds   ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------

protected:
        
virtual void    AcceptUpdate    ( const bitstream& BitStream );
virtual void    ProvideUpdate   (       bitstream& BitStream, u32& DirtyBits );

};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern tdm_logic  TDM_Logic;

//==============================================================================
#endif // TDM_LOGIC_HPP
//==============================================================================
