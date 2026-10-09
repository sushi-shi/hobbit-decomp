//==============================================================================
//
//  LogMgr.hpp
//
//==============================================================================

#ifndef LOGMGR_HPP
#define LOGMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "MsgMgr.hpp"
#include "AADS\fe_globals.hpp"
#include <xCore/x_files/x_stdio.hpp>

//==============================================================================
//  TYPES
//==============================================================================

struct log_info
{
    char*   String;
    s16     Arg1;
    s16     Arg2;
    s16     Style;
};

//==============================================================================

enum log_style
{
    STYLE_NONE          = -1,

    STYLE_NORMAL        =  0,
    STYLE_NEGATIVE,
    STYLE_GAME_NORMAL,
    STYLE_GAME_URGENT,
    STYLE_GAME_POSITIVE,
    STYLE_GAME_NEGATIVE,
    STYLE_ADMIN_NORMAL,
    STYLE_ADMIN_URGENT,
    STYLE_COMMENTERY,
    STYLE_CHAT,
    STYLE_CHAT_STORM,
    STYLE_CHAT_INFERNO,

    STYLE_WARRIOR,
    STYLE_WARRIOR_STORM,
    STYLE_WARRIOR_INFERNO,
    STYLE_WARRIOR_VICTIM,
    STYLE_WARRIOR_KILLER, 

    STYLE_TEAM_STORM,
    STYLE_TEAM_INFERNO,

    STYLE_TABLE_LABEL,
    STYLE_TABLE_SCORE,
};

// Begin and End for each code.  How to handle style per code?

//==============================================================================

char*   MakeNarrowString( char* pNarrow, const xwchar* pWide, s32 BufferSize );

//==============================================================================

class log_mgr
{

//------------------------------------------------------------------------------
//  Public Types

public:

    enum
    {   
        END_BY_VOTE,
        END_BY_ADMIN,
        END_BY_TIME,
        END_BY_SCORE,
    };

//------------------------------------------------------------------------------
//  Public Functions

public:

                    log_mgr         ( void );
                   ~log_mgr         ( void );

        void        AdvanceLogic    ( f32 DeltaTime );

        void        SetIRCStatus    ( xbool Enabled );
        void        SetLogStatus    ( xbool Enabled );
        void        SetLogDir       ( const char* pDir );

        xbool       GetIRCStatus    ( void );
        xbool       GetLogStatus    ( void );
const   char*       GetLogDir       ( void );

        void        LogMsg          ( const msg& Msg );

        void        LogStartOfGame  ( void );
        void        LogEndOfGame    ( s32 Cause );

        void        LogFullScore    ( void );
        void        LogSmallScore   ( void );
        void        LogStatus       ( void );
        void        LogComment      ( void );

        void        LogDisconnect   ( s32 PlayerIndex );
        void        LogConnect      ( s32 PlayerIndex, xbool Reconnect );

        void        TagPlayer       ( s32 PlayerIndex, X_FILE* pFile );

//------------------------------------------------------------------------------
//  Private Functions

protected:

        xbool       FetchArgValue   ( const msg& Msg, s16 Arg, s16& Value );

        void        UpdateTimeStamp ( void );

        void        AppendString    ( const char* pRead );
        void        AppendStringIRC ( const char* pRead );
        void        AppendStringUBB ( const char* pRead );
        void        AppendStringLog ( const char* pRead );

        void        AppendFormat    ( s32 Style, xbool Enter );
        void        AppendFormatIRC ( s32 Style, xbool Enter );
        void        AppendFormatUBB ( s32 Style, xbool Enter );
        void        AppendFormatLog ( s32 Style, xbool Enter );

        void        AppendField     ( s32 Style, const char* pRead );

        void        ResetStrings    ( void );
        void        Distribute      ( void );

        void        OpenLogFiles    ( void );
        void        CloseLogFiles   ( void );

        void        DumpTeamScore   ( void );
        void        DumpNonTeamScore( void );

//------------------------------------------------------------------------------
//  Private Storage

protected:

        char        m_LogDir   [256];
        char        m_IRCBuffer[256];
        char        m_TTYBuffer[256];
        char        m_LogBuffer[256];
        char        m_UBBBuffer[256];

        s32         m_StartYear, m_StartMon, m_StartDay;
        s32         m_StartHour, m_StartMin, m_StartSec;

        char*       m_pIRCWrite;
        char*       m_pTTYWrite;
        char*       m_pLogWrite;
        char*       m_pUBBWrite;

        X_FILE*     m_pLogFile;
        X_FILE*     m_pUBBFile;
        X_FILE*     m_pTagFile;

        xbool       m_DoIRC;
        xbool       m_DoLog;

        s32         m_BaseStyle;

        f32         m_TimeToFullScore;      // Every 1:30 or so
        f32         m_TimeToSmallScore;     // Every score change, 1:00 or so
        f32         m_TimeToStatus;         // Every 0:30 or so?
        f32         m_TimeToComment;        // Every 0:30 or so?
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern log_mgr  LogMgr;

//==============================================================================
#endif // LOGMGR_HPP
//==============================================================================
