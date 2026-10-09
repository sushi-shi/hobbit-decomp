//==============================================================================
//
//  CNH_Logic.hpp
//
//==============================================================================

#ifndef CNH_LOGIC_HPP
#define CNH_LOGIC_HPP

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

class cnh_logic : public game_logic
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:

                cnh_logic       ( void );
virtual        ~cnh_logic       ( void );

virtual void    Connect         ( s32 PlayerIndex );
virtual void    ExitGame        ( s32 PlayerIndex );
virtual void    Disconnect      ( s32 PlayerIndex );
virtual void    ChangeTeam      ( s32 PlayerIndex );

virtual void    PlayerDied      ( const pain& Pain );
virtual void    SwitchTouched   ( object::id SwitchID, object::id PlayerID  );

virtual void    AdvanceTime     ( f32 DeltaTime );

virtual void    BeginGame       ( void );
virtual void    EndGame         ( void );

//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------

protected:
        
virtual void    AcceptUpdate    ( const bitstream& BitStream );
virtual void    ProvideUpdate   (       bitstream& BitStream, u32& DirtyBits );

//------------------------------------------------------------------------------
//  Private Storage
//------------------------------------------------------------------------------

protected:

    // NOTE: There are 16 switch circuits in the GameMgr.  However, the first
    //       4 are reserved for special uses.  So, only 12 are left.

    f32         m_HoldTime       [12];  // Time held.  Reset every sec after 1st 10.
    xbool       m_First10Seconds [12];  // First 10 sec. interval?
    s32         m_HoldTeam       [12];  // Which team has this switch?
    object::id  m_CapturePlayerID[12];  // Who acquired this objective?
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern cnh_logic  CNH_Logic;

//==============================================================================
#endif // CNH_LOGIC_HPP
//==============================================================================
