//==============================================================================
//
//  MsgMgr.hpp
//
//==============================================================================
//  
//  <1> = his/her
//  <2> = himself/herself  
//  
//==============================================================================

#ifndef MSGMGR_HPP
#define MSGMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "NetLib/BitStream.hpp"

//==============================================================================
//  TYPES
//==============================================================================

enum msg_where
{
    WHERE_CHAT,
    WHERE_EXCHANGE,
    WHERE_ABOVE,
    WHERE_BELOW,
    WHERE_NONE,
};

//==============================================================================

enum msg_color  
{   
    COLOR_NEUTRAL,          // blue
    COLOR_URGENT,           // yellow
    COLOR_NEGATIVE,         // red
    COLOR_POSITIVE,         // green
    COLOR_ENEMY,            // RED
    COLOR_ALLY,             // GREEN
    COLOR_POSITIVE_ARG,     // red or green by team of arg data (special)
    COLOR_NEGATIVE_ARG,     // green or red by team of arg data (special)
    COLOR_NONE,
};

//==============================================================================

enum msg_audience
{
    HEAR_ALL,
    HEAR_TEAM,
    HEAR_LOCAL,
    HEAR_MACHINE,
    HEAR_SINGLE,
};

//==============================================================================

enum msg_arg
{
    ARG_KILLER,     // K
    ARG_VICTIM,     // V
    ARG_WARRIOR,    // W
    ARG_INLINE,     // I
    ARG_PICKUP,     // P
    ARG_N,          // N
    ARG_M,          // M
    ARG_TEAM,       // T
    ARG_OBJECTIVE,  // O
    ARG_VEHICLE,    // S (Ship)    
    ARG_NONE,
};

//==============================================================================

struct msg_info
{
    s16     Index;
    s16     Arg1;
    s16     Arg2;
    s16     Audience;
    s16     Where;
    s16     Color;
    s32     Sound;
};

//==============================================================================

struct msg
{
    s16     Index;
    s16     Target;     // on receive: player     on send: player or team
    s16     Arg1;
    s16     Value1;
    s16     Arg2;
    s16     Value2;
};

//==============================================================================

class msg_mgr
{

//------------------------------------------------------------------------------
//  Public Functions

public:

                    msg_mgr         ( void );
                   ~msg_mgr         ( void );

        void        Reset           ( void );

        void        PackMsg         ( const msg& Msg, bitstream& BitStream );
        void        UnpackMsg       (       msg& Msg, bitstream& BitStream );
        void        AcceptMsg       (                 bitstream& BitStream );
        void        PackArg         ( s16  Arg, s16  Value, bitstream& BitStream );
        void        UnpackArg       ( s16& Arg, s16& Value, bitstream& BitStream );
        void        DistributeMsg   (       msg& Msg );
        void        ProcessMsg      ( const msg& Msg );
        void        DisplayMsg      ( const msg& Msg );

        xbool       FetchArgValue   ( const msg& Msg, s16 Arg, s16& Value );

        void        PlayerDied      ( const pain& Pain );

        void        Message         ( s32 MsgType, 
                                      s32 Target,   // player or team
                                      s32 Arg1   = ARG_NONE,
                                      s32 Value1 = -1,
                                      s32 Arg2   = ARG_NONE,
                                      s32 Value2 = -1 );

        void        AdvanceLogic    ( f32 DeltaSec );

        void        InjectColor     ( xwchar*& pString, xcolor Color );
        xcolor      GetColor        ( s32 Type );

        xwchar*     GetInlineName   ( s32 Index );

//------------------------------------------------------------------------------
//  Private Functions

        void        VerifyArg       ( s32 MsgType, s32 Arg );

//------------------------------------------------------------------------------
//  Private Storage

        msg         m_Queue[4];
        msg         m_OnDeck;
        s32         m_QCount;
        s32         m_QRead;
        s32         m_QWrite;
        f32         m_Timer;
        s32         m_Playing; 

        s32         m_InlineLast;
        xwchar      m_InlineName[16][16];
};

// Looks like no message has more than two indepedent variables.
// Use s16 for msg_type, and s8 for msg_arg and values.

// Server:
//  - Source of most (if not all) messages.
//  - Routes messages to appropriate clients.
//
// Client:
//  - Must be able to decode messages into string, audio, color, etc.
//

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern msg_mgr  MsgMgr;

//==============================================================================
#endif // MSGMGR_HPP
//==============================================================================
