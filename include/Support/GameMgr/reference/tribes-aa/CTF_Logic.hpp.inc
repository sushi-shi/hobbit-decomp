//==============================================================================
//
//  CTF_Logic.hpp
//
//==============================================================================

#ifndef CTF_LOGIC_HPP
#define CTF_LOGIC_HPP

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

class ctf_logic : public game_logic
{

//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:

                ctf_logic       ( void );
virtual        ~ctf_logic       ( void );

virtual void    Connect         ( s32 PlayerIndex );
virtual void    ExitGame        ( s32 PlayerIndex );
virtual void    Disconnect      ( s32 PlayerIndex );
virtual void    ChangeTeam      ( s32 PlayerIndex );

virtual void    RegisterFlag    ( const vector3& Position, radian Yaw, s32 FlagTeam );

virtual void    PlayerDied      ( const pain& Pain );
virtual void    FlagTouched     ( object::id FlagID,   object::id PlayerID );
virtual xbool   FlagHitPlayer   ( object::id FlagID,   object::id PlayerID );
virtual void    FlagTimedOut    ( object::id FlagID );  
virtual void    ThrowFlags      ( object::id PlayerID );  
virtual void    SwitchTouched   ( object::id SwitchID, object::id PlayerID );
virtual void    ItemDeployed    ( object::id ItemID,   object::id PlayerID  );

virtual void    GetFlagStatus   ( u32 TeamBits, s32& Status, vector3& Position );

virtual void    EnforceBounds   ( f32 DeltaTime );

virtual void    BeginGame       ( void );
virtual void    EndGame         ( void );

virtual void    Render          ( void );




//------------------------------------------------------------------------------
virtual object::id GetFlagID    ( s32 Index );
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Private Functions
//------------------------------------------------------------------------------

protected:
        
virtual void    AcceptUpdate    ( const bitstream& BitStream );
virtual void    ProvideUpdate   (       bitstream& BitStream, u32& DirtyBits );

        void    DropFlag        (       s32        FlagIndex,
                                        object::id OriginID,
                                  const vector3&   Position, 
                                  const vector3&   Velocity, 
                                        radian     Yaw );

        void    FlagReturned    (       s32        FlagIndex );

//------------------------------------------------------------------------------
//  Private Types
//------------------------------------------------------------------------------

protected:

    enum flag_status
    {
        FLAG_STATUS_UNKNOWN   = -3,
        FLAG_STATUS_ON_STAND  = -2,
        FLAG_STATUS_DROPPED   = -1,
        FLAG_STATUS_PLAYER_00 =  0,        FLAG_STATUS_PLAYER_01 =  1,
        FLAG_STATUS_PLAYER_02 =  2,        FLAG_STATUS_PLAYER_03 =  3,
        FLAG_STATUS_PLAYER_04 =  4,        FLAG_STATUS_PLAYER_05 =  5,
        FLAG_STATUS_PLAYER_06 =  6,        FLAG_STATUS_PLAYER_07 =  7,
        FLAG_STATUS_PLAYER_08 =  8,        FLAG_STATUS_PLAYER_09 =  9,
        FLAG_STATUS_PLAYER_10 = 10,        FLAG_STATUS_PLAYER_11 = 11,
        FLAG_STATUS_PLAYER_12 = 12,        FLAG_STATUS_PLAYER_13 = 13,
        FLAG_STATUS_PLAYER_14 = 14,        FLAG_STATUS_PLAYER_15 = 15,
        FLAG_STATUS_PLAYER_16 = 16,        FLAG_STATUS_PLAYER_17 = 17,
        FLAG_STATUS_PLAYER_18 = 18,        FLAG_STATUS_PLAYER_19 = 19,
        FLAG_STATUS_PLAYER_20 = 20,        FLAG_STATUS_PLAYER_21 = 21,
        FLAG_STATUS_PLAYER_22 = 22,        FLAG_STATUS_PLAYER_23 = 23,
        FLAG_STATUS_PLAYER_24 = 24,        FLAG_STATUS_PLAYER_25 = 25,
        FLAG_STATUS_PLAYER_26 = 26,        FLAG_STATUS_PLAYER_27 = 27,
        FLAG_STATUS_PLAYER_28 = 28,        FLAG_STATUS_PLAYER_29 = 29,
        FLAG_STATUS_PLAYER_30 = 30,        FLAG_STATUS_PLAYER_31 = 31,
    };

//------------------------------------------------------------------------------
//  Private Data
//------------------------------------------------------------------------------

protected:

    flag_status     m_FlagStatus[2];    // SPECIFIC_DATA
    object::id      m_Flag[2];          // SPECIFIC_DATA

    vector3         m_FlagStand[2];     // SPECIFIC_INIT
    radian          m_FlagYaw[2];       // SPECIFIC_INIT

    object::id      m_StandWayPt[2];    // Not sent.
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern ctf_logic  CTF_Logic;

//==============================================================================
#endif // CTF_LOGIC_HPP
//==============================================================================
