#include <xCore/x_files/x_files.hpp>
#include "telnetclient.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "GameMgr/LogMgr.hpp"

enum
{
    HM_MAIN=0,
    HM_ADMIN,
    HM_ENABLE,
    HM_DISABLE,
    HM_TOTAL,
};

static char* s_GameTypes[]=
{
    "None",
    "CTF",
    "CnH",
    "TDM",
    "DM",
    "HUNT",
    "SOLO",
};
//
// Game specific files needed so that we can gather
// stats about what's going on
//
#include "fe_globals.hpp"
#include "support/gamemgr/gamemgr.hpp"
#include "masterserver/masterserver.hpp"
#include "gameserver.hpp"
#include "telnetmgr.hpp"

extern server_manager*  pSvrMgr;

#if defined(TARGET_PC) 
#elif defined(TARGET_LINUX)
#include <unistd.h>
#include <signal.h>
#else

#error This is a PC only file. Please exclude it from the build rules

#endif

extern const char** s_HelpMessage[];

static telnet_client* s_pClient = NULL;

void    s_StartTelnetClient(void)
{
    telnet_client *pThis;
#if defined(TARGET_LINUX)
    // Ignore the sigpipe signal which is caused when no data is
    // available on a socket that has been force-closed by the receiving
    // side.
    signal(SIGPIPE, SIG_IGN);
#endif
    ASSERT(s_pClient);
    pThis = s_pClient;
    s_pClient = NULL;
    pThis->Main();
    ASSERT(FALSE);
}

/*
const char* MakeNarrowString(char* Narrow,const xwchar* pWide)
{
    char* pNarrow=Narrow;
    while (*pWide)
    {
        if (*pWide >=' ')
        {
            *pNarrow++=(char)*pWide++;
        }
        else
        {
            pWide++;
        }
    }
    *pNarrow=0;
    return Narrow;
}
*/

//******************************************************************
// Startup/shutdown routines
//******************************************************************
void telnet_client::Init(const net_address& Address)
{
    m_Retries = 4;
    m_AuthLevel = AUTHLEVEL_NONE;
    m_Username[0]=0x0;
    m_Password[0]=0x0;
    m_Exit = FALSE;
    s_pClient = this;
    m_UpdateInterval = 0.0f;
    m_DisplayMode = DISP_MODE_PLAYERS;
    m_LastDisplayMode = DISP_MODE_LAST;
    x_memset(m_LastBytesIn,0,sizeof(m_LastBytesIn));
    x_memset(m_LastBytesOut,0,sizeof(m_LastBytesOut));

    m_Remote = Address;
    m_Display.Init(this,Address,80,32);
    m_pInfoWindow = new telnet_display::window(0,4,80,16,WIN_FLAG_BORDER);
    m_pLogWindow = new telnet_display::window(0,19,m_Display.GetWidth()+2,11,WIN_FLAG_BORDER);
    m_pExtendedLogWindow = new telnet_display::window(0,4,m_Display.GetWidth()+2,26,WIN_FLAG_BORDER);
    m_pInputWindow = new telnet_display::window(0,29,m_Display.GetWidth()+2,3,WIN_FLAG_BORDER);
    m_RefreshTimer.Reset();
    m_RefreshTimer.Start();
    m_BasePlayerIndex = 0;
    m_BaseClientIndex = 0;

    m_pThread = new xthread(s_StartTelnetClient,"Telnet Client",8192,0);
    // Wait until the client starts up
    while (s_pClient)
    {
        x_DelayThread(1);
    }
}

//******************************************************************
void telnet_client::Kill(void)
{
    delete m_pInfoWindow;
    delete m_pInputWindow;
    delete m_pLogWindow;
    m_Display.Kill();
    delete m_pThread;
#if defined(TARGET_PC)
    closesocket(m_Remote.Socket);
#else
    close(m_Remote.Socket);
#endif

}

//******************************************************************
void telnet_client::Stop(void)
{
    m_Exit = TRUE;
    while(1)
    {
        x_DelayThread(100);
    }
}


//******************************************************************
xbool telnet_client::Update(f32 DeltaTime)
{
    m_UpdateInterval = DeltaTime;
    return m_Exit;
}

//******************************************************************
// Re-render the entire display
//******************************************************************

/*******************************************************************************

             11111111112222222222333333333344444444445555555555666666666677777777
   012345678901234567890123456789012345678901234567890123456789012345678901234567
  +------------------------------------------------------------------------------+
0 | Server Name: 123456789012345      Players: 10/16     Uptime: 1:23:45:00      |
1 | Current Map: CTF-Jacob's Ladder   Clients:  9/16    Logging: ENABLED         |
2 |   Game Time: 12:45 of 20:00      Avg Ping:  110    Password: 123456789012345 |
  +------------------------------------------------------------------------------+

*******************************************************************************/


void telnet_client::Refresh(void)
{
    const game_score& Score = GameMgr.GetScore();

    m_Display.SetWindow(NULL);
    m_Display.Clear();

    // Line 0
    {
        s32 Humans, Bots, Max;
        s32 days, hours, minutes, seconds;
        s64 time = (s64)x_GetTimeSec();

        seconds = (s32)(time % 60); time = time / 60;
        minutes = (s32)(time % 60); time = time / 60;
        hours   = (s32)(time % 24); time = time / 24;
        days    = (s32)time;

        GameMgr.GetPlayerCounts( Humans, Bots, Max );
        char servername[128];

        MakeNarrowString( servername, fegl.ServerSettings.ServerName, 128 );
        m_Display.PrintXY(  1, 0, "Server Name: %s", servername );
        m_Display.PrintXY( 35, 0, "Players: %2d/%2d", Humans, Max );
        m_Display.PrintXY( 54, 0, "Uptime: %d:%02d:%02d:%02d", days, hours, minutes, seconds );
    }

    // Line 1
    if( tgl.pServer )
    {
        s32 X = 62;

        m_Display.PrintXY(  1, 1, "Current Map: %s-%s", s_GameTypes[Score.GameType], tgl.MissionName );
        m_Display.PrintXY( 35, 1, "Clients: %2d/%2d", tgl.pServer->GetClientCount(), fegl.ServerSettings.MaxClients );
        m_Display.PrintXY( 53, 1, "Logging:" ); //, LogMgr.GetLogStatus() ? "ENABLED" : "DISABLED" );

        if( LogMgr.GetLogStatus() )
        {
            m_Display.PrintXY( X, 1, "FILE" );
            X += 5;

            if( LogMgr.GetIRCStatus() )
            {
                m_Display.PrintXY( X, 1, "/" );
                X += 2;
            }
        }

        if( LogMgr.GetIRCStatus() )
        {
            if( g_ChatManager.IsConnected() )
                m_Display.PrintXY( X, 1, "IRC" );
            else
            {
                m_Display.SetColor(RED);
                m_Display.PrintXY( X, 1, "IRC" );
                m_Display.SetColor();
            }
        }

        if( !LogMgr.GetLogStatus() && !LogMgr.GetIRCStatus() )
        {
            m_Display.PrintXY( X, 1, "DISABLED" );
        }
    }

    // Line 2
    if( tgl.pServer )
    {
        char Password[32];

        s32 seconds = (s32)GameMgr.GetTimeRemaining();
        s32 minutes = seconds / 60; 
        seconds = seconds % 60;

        MakeNarrowString( Password, fegl.ServerSettings.AdminPassword, 32 );

        m_Display.PrintXY(  3, 2, "Game Time: %2d:%02d of %d:00", minutes, seconds, GameMgr.GetTimeLimit() );
        m_Display.PrintXY( 34, 2, "Avg Ping: %d", (s32)tgl.pServer->GetAveragePing() );
        m_Display.PrintXY( 52, 2, "Password: %s", Password[0] ? Password : "" );
    }

    //
    // Info Window
    //

    m_Display.SetWindow(m_pInfoWindow);
    if( tgl.pServer )
    {
        s32 count=0;
        s32 index=0;

        switch (m_DisplayMode)
        {
        case DISP_MODE_PLAYERS:
            RefreshPlayers();
            /*
            if (m_BasePlayerIndex > 32)
            {
                m_BasePlayerIndex = 0;
            }
            m_LastPlayerIndex = RefreshPlayers(m_BasePlayerIndex);
            */
            break;
        case DISP_MODE_CLIENTS:
            if (m_AuthLevel == AUTHLEVEL_ADMIN)
            {
                if (m_BaseClientIndex > MAX_CLIENTS)
                {
                    m_BaseClientIndex = 0;
                }
                m_LastClientIndex = RefreshClients(m_BaseClientIndex);
            }
            else
            {
                m_DisplayMode = DISP_MODE_SERVER;
            }
            break;
        case DISP_MODE_SERVER:
            RefreshServer();
            break;
        case DISP_MODE_LOG:
            RefreshLog();
            break;
        default:
            ASSERT(FALSE);
        }
        if (m_DisplayMode >= DISP_MODE_LAST)
        {
            m_DisplayMode = DISP_MODE_PLAYERS;
        }

        m_LastDisplayMode = m_DisplayMode;
    }
    else
    {
        m_LastDisplayMode = DISP_MODE_LAST;
        m_Display.Clear();
    }
    if (m_DisplayMode != DISP_MODE_LOG)
    {
        m_Display.SetWindow(m_pLogWindow);
        RefreshLogText();
    }

    m_Display.SetWindow(NULL);
    SampleGameState();
}

//******************************************************************
s32 telnet_client::Dialog(const char* pTitle, const char** pOptions)
{
    s32     LineCount;
    s32     LineWidth;
    s32     WindowWidth,WindowHeight;
    s32     x,y;
    s32     CurrentLine,TopLine;
    char    ch;
    xbool   done;
    
    const char** pList;
    telnet_display::window* pWindow;

    m_Display.SetWindow(NULL);

    LineCount=0;
    LineWidth = x_strlen(pTitle);
    pList = pOptions;
    while(*pList)
    {
        if (x_strlen(*pList)>LineWidth)
        {
            LineWidth=x_strlen(*pList);
        }
        LineCount++;
        pList++;
    }


    WindowHeight = MIN(LineCount+2,m_Display.GetHeight()/2);
    WindowWidth = MIN(LineWidth+2,m_Display.GetWidth()/2);

    x = (m_Display.GetWidth() - WindowWidth)/2;
    y = (m_Display.GetHeight() - WindowHeight)/2;

    pWindow = new telnet_display::window(x,y,WindowWidth,WindowHeight,WIN_FLAG_BORDER|WIN_FLAG_MODAL);

    m_Display.SetWindow(pWindow);
    TopLine = 0;
    CurrentLine = 0;

    done = FALSE;

    while(!done)
    {
        m_Display.SetColor(BLUE,YELLOW);
        m_Display.Clear();
        m_Display.SetColor(BLUE,YELLOW);
        m_Display.SetJustify(LEFT);
        m_Display.PrintXY(0,-1,pTitle);
        m_Display.SetJustify(CENTER);
        pList = pOptions;

        if (CurrentLine < TopLine)
        {
            TopLine = CurrentLine;
        }
        if (CurrentLine-TopLine >= m_Display.GetHeight())
        {
            TopLine++;
        }

        pList = pOptions+TopLine;

        for (y = 0;y<m_Display.GetHeight();y++)
        {
            if ( *pList==NULL )
                break;
            if (y+TopLine == CurrentLine)
            {
                m_Display.SetColor(BLACK,WHITE);
            }
            else
            {
                m_Display.SetColor();
            }
            m_Display.ClearLine(y);
            m_Display.PrintXY(m_Display.GetWidth()/2,y,*pList);

            pList++;
        }

        ch = m_Display.GetChar();
        switch(ch)
        {
        case TELNET_CHAR_UP:
        case 'u':
            CurrentLine = MAX(CurrentLine-1,0);
            break;
        case TELNET_CHAR_DOWN:
        case 'd':
            CurrentLine = MIN(CurrentLine+1,LineCount-1);
            break;
        case '\n':
        case '\r':
            done = TRUE;
            break;
        case '\x1b':
            done = TRUE;
            CurrentLine = -1;
            break;
        }

        m_Display.GotoXY(0,CurrentLine-TopLine);
        m_Display.Flush(FALSE);
    }

    m_Display.SetWindow(NULL);
    delete pWindow;
    return CurrentLine;
}
//******************************************************************
s32 telnet_client::GetValue(const char* pLabel)
{
    s32 Value;
    const char* pLine;

    m_Display.SetWindow(m_pInputWindow);
    m_Display.Clear();
    m_Display.PrintXY(0,m_Display.GetHeight()-1,"%s: ",pLabel);
    pLine = m_Display.GetLine();

    if( (!pLine) || (*pLine <'0') || (*pLine>'9') )
    {
        return -1;
    }

    Value = 0;
    while ( (*pLine>='0') && (*pLine<='9') )
    {
        Value = Value * 10 + *pLine-'0';
        pLine++;
    }
    return Value;
}

//******************************************************************
char telnet_client::WaitForCommand(const char* Label, 
                                   const char** Help, 
                                   const telnet_color foreground,
                                   const telnet_color background)
{
    char ch;

    while(1)
    {
        Refresh();
        m_Display.SetWindow(m_pInputWindow);
        m_Display.Clear();
        m_Display.PrintXY(0,m_Display.GetHeight()-1,"%s: ",Label);
        ch = x_toupper(m_Display.GetChar());

        if (ch=='\t')
        {
            switch(m_DisplayMode)
            {
            case DISP_MODE_PLAYERS:
                m_DisplayMode = DISP_MODE_SERVER;
                /*
                m_BasePlayerIndex = m_LastPlayerIndex+1;
                if (m_BasePlayerIndex > 32)
                {
                    m_DisplayMode = DISP_MODE_SERVER;
                    m_BaseClientIndex = 0;
                }
                */
                break;
            case DISP_MODE_SERVER:
                m_DisplayMode = DISP_MODE_LOG;
                break;
            case DISP_MODE_LOG:
                if (m_AuthLevel != AUTHLEVEL_ADMIN)
                {
                    m_DisplayMode = DISP_MODE_PLAYERS;
                    m_BasePlayerIndex = 0;
                }
                else
                {
                    m_DisplayMode = DISP_MODE_CLIENTS;
                    m_BaseClientIndex = 0;
                }
                break;
            case DISP_MODE_CLIENTS:
                m_BaseClientIndex = m_LastClientIndex+1;
                if ( (m_BaseClientIndex > MAX_CLIENTS) || (m_AuthLevel != AUTHLEVEL_ADMIN) )
                {
                    m_DisplayMode = DISP_MODE_PLAYERS;
                    m_BaseClientIndex = 0;
                }
                break;
            }
        }
        else if (ch=='H')
        {
            m_Display.SetWindow(m_pInfoWindow);
            m_Display.SetColor(BLUE,WHITE);
            m_Display.Clear();
            m_Display.SetColor(BLUE,WHITE);
            const char** pText;

            pText = Help;
            while (*pText)
            {
                m_Display.Print(*pText);
                pText++;
            }
            while(!m_Display.GetChar());
        }
        else if (ch!=0)
        {
            return ch;
        }
    }
}
//******************************************************************
// Command mode processors
//******************************************************************

void telnet_client::KickPlayer      (void)
{
    s32 Option;
    char PlayerNames[32][32];
    s32  PlayerIndexes[32];
    char* pPlayerNames[33];

    s32 NameIndex;


    if (g_SM.CurrentState != STATE_SERVER_INGAME)
        return;
    NameIndex = 0;

    // Build the list of players that can be kicked. Borrowed from dlg_player.cpp
    const game_score& Score      = GameMgr.GetScore();

    for( s32 i=0; i<32; i++ )
    {
        // Note: the following are the comments associated with the routine in dlg_player.
        // Except certain rules do not apply since:
        //      We don't exist in the game, there will never be anyone on machine 0 and
        //      splitscreen is irrelevant.
        // To be able to kick, the victim must be:
        //  - Not yourself
        //  - In the game
        //  - Not a bot
        //  - Not on machine 0 (the server)
        //  - Not on the same machine as yourself (split screen protection)
#if 1
        if( (Score.Player[i].IsInGame) &&
            (Score.Player[i].IsBot == FALSE) )
#else
        if( (Score.Player[i].IsInGame) )
#endif
        {
            x_strcpy( PlayerNames[NameIndex], Score.Player[i].NName );
            PlayerIndexes[NameIndex] = i;
            pPlayerNames [NameIndex] = PlayerNames[NameIndex];
            NameIndex++;
        }
    }

    pPlayerNames[NameIndex] = NULL;

    // No players to kick, so let's forget about it!
    if( NameIndex == 0 )
        return;

    // Find which player the user wants to kick
    Option = Dialog( "SELECT PLAYER TO KICK", (const char**)pPlayerNames );

    // If -ve then operation was cancelled.
    if( Option >= 0 )
    {
        // Verify the player selected is actually the player we
        // want to kick. The player may have disappeared while the
        // dialog box was up and we don't want to kick the wrong
        // person!

        if( x_strcmp( PlayerNames[Option], 
                      Score.Player[PlayerIndexes[Option]].NName ) != 0 )
        {
            g_TelnetManager.AddDebugText("Kick failed.");
            return;
        }

        g_TelnetManager.AddDebugText( "Kicked player %s", PlayerNames[Option] );
        GameMgr.KickPlayer( PlayerIndexes[Option] );
    }
}

//-------------------------------------------------------------------
void telnet_client::TeamswitchPlayer    (void)
{
    s32 Option;
    char PlayerNames[32][32];
    s32  PlayerIndexes[32];
    char* pPlayerNames[33];

    s32 NameIndex;


    if (g_SM.CurrentState != STATE_SERVER_INGAME)
        return;
    NameIndex = 0;

    // Build the list of players that can be kicked. Borrowed from dlg_player.cpp
    const game_score& Score      = GameMgr.GetScore();

    for( s32 i=0; i<32; i++ )
    {
        // Note: the following are the comments associated with the routine in dlg_player.
        // Except certain rules do not apply since:
        //      We don't exist in the game, there will never be anyone on machine 0 and
        //      splitscreen is irrelevant.
        // To be able to kick, the victim must be:
        //  - Not yourself
        //  - In the game
        //  - Not a bot
        //  - Not on machine 0 (the server)
        //  - Not on the same machine as yourself (split screen protection)
#if 1
        if( (Score.Player[i].IsInGame) &&
            (Score.Player[i].IsBot == FALSE) )
#else
        if( (Score.Player[i].IsInGame) )
#endif
        {
            // Convert from wide character to narrow.
            x_strcpy( PlayerNames[NameIndex], Score.Player[i].NName );
            PlayerIndexes[NameIndex] = i;
            pPlayerNames [NameIndex] = PlayerNames[NameIndex];
            NameIndex++;
        }
    }

    pPlayerNames[NameIndex]=NULL;

    // No players to kick, so let's forget about it!
    if( NameIndex == 0 )
        return;

    // Find which player the user wants to kick
    Option = Dialog( "TEAMSWITCH PLAYER", (const char**)pPlayerNames );

    // If -ve then operation was cancelled.
    if( Option >= 0 )
    {
        // Verify the player selected is actually the player we
        // want to kick. The player may have disappeared while the
        // dialog box was up and we don't want to kick the wrong
        // person!

        if( x_strcmp( PlayerNames[Option],
                      Score.Player[PlayerIndexes[Option]].NName ) != 0 )
        {
            g_TelnetManager.AddDebugText( "Teamswitch Failed." );
            return;
        }

        g_TelnetManager.AddDebugText( "Team switched player %s", PlayerNames[Option] );
        GameMgr.ChangeTeam( PlayerIndexes[Option] );
    }
}
//-------------------------------------------------------------------
void telnet_client::DropPlayer  (void)
{
    s32 Option;
    char PlayerNames[32][32];
    s32  PlayerIndexes[32];
    char* pPlayerNames[33];

    s32 NameIndex;


    if (g_SM.CurrentState != STATE_SERVER_INGAME)
        return;
    NameIndex = 0;

    // Build the list of players that can be kicked. Borrowed from dlg_player.cpp
    const game_score& Score      = GameMgr.GetScore();

    for( s32 i=0; i<32; i++ )
    {
        // Note: the following are the comments associated with the routine in dlg_player.
        // Except certain rules do not apply since:
        //      We don't exist in the game, there will never be anyone on machine 0 and
        //      splitscreen is irrelevant.
        // To be able to kick, the victim must be:
        //  - Not yourself
        //  - In the game
        //  - Not a bot
        //  - Not on machine 0 (the server)
        //  - Not on the same machine as yourself (split screen protection)
#if 1
        if( (Score.Player[i].IsInGame) &&
            (Score.Player[i].IsBot == FALSE) )
#else
        if( (Score.Player[i].IsInGame) )
#endif
        {
            // Convert from wide character to narrow.
            x_strcpy( PlayerNames[NameIndex], Score.Player[i].NName );
            PlayerIndexes[NameIndex] = i;
            pPlayerNames [NameIndex] = PlayerNames[NameIndex];
            NameIndex++;
        }
    }

    pPlayerNames[NameIndex] = NULL;

    // No players to kick, so let's forget about it!
    if( NameIndex == 0 )
        return;

    // Find which player the user wants to kick
    Option = Dialog( "SELECT PLAYER TO DROP", (const char**)pPlayerNames );

    // If -ve then operation was cancelled.
    if( Option >= 0 )
    {
        // Verify the player selected is actually the player we
        // want to kick. The player may have disappeared while the
        // dialog box was up and we don't want to kick the wrong
        // person!

        if( x_strcmp( PlayerNames[Option],
                      Score.Player[PlayerIndexes[Option]].NName ) != 0 )
        {
            g_TelnetManager.AddDebugText( "Drop player failed." );
            return;
        }

        g_TelnetManager.AddDebugText( "Dropped player %s.", PlayerNames[Option] );
        tgl.pServer->DropPlayer( PlayerIndexes[Option] );
    }
}
//-------------------------------------------------------------------
void telnet_client::SwitchMap       (void)
{
    char MissionNames[32][32];
    s32  MissionIndexes[32];
    char* pMissionNames[33];
    s32 Index;
    s32 Map;

    Index = 0;

    if (g_SM.CurrentState != STATE_SERVER_INGAME)
        return;
    // Add all the maps to the list
    xwstring    MissionName;
    for( s32 i=0 ; ; i++ )
    {
        if( fe_Missions[i].GameType == GAME_NONE )
            break;

        if( fegl.GameType == fe_Missions[i].GameType )
        {
            MakeNarrowString( MissionNames[Index],
                              StringMgr( "MissionName", fe_Missions[i].DisplayNameString ),
                              32 );
            pMissionNames [Index] = MissionNames[Index];
            MissionIndexes[Index] = i;
            Index++;
        }
    }

    pMissionNames[Index] = NULL;

    if( Index == 0 )
        return;

    Map = Dialog( "SELECT MAP", (const char**)pMissionNames );
    if( Map < 0 )
        return;
    g_TelnetManager.AddDebugText( "Switching to map %s", MissionNames[Map] );
    GameMgr.ChangeMap( MissionIndexes[Map] );
}
//-------------------------------------------------------------------
void telnet_client::SetClientCount  (void)
{
    s32 Value;
    s32 MaxClients;

    if (m_AuthLevel == AUTHLEVEL_ADMIN)
    {
        MaxClients = MAX_CLIENTS;
    }
    else
    {
        MaxClients = 16;
    }

    Value = GetValue(xfs("Set Max Client count to [1..%d]",MaxClients));

    if ( (Value > 0) && (Value <= MaxClients) )
    {
        g_TelnetManager.AddDebugText("Set client count to %d",Value);
        fegl.ServerSettings.MaxClients  = Value;
    }

}
//-------------------------------------------------------------------
void telnet_client::SetPlayerCount  (void)
{
    s32 Value;
    s32 MaxPlayers;


    if (m_AuthLevel == AUTHLEVEL_ADMIN)
    {
        MaxPlayers = 32;
    }
    else
    {
        MaxPlayers = 16;
    }

    Value = GetValue(xfs("Set Max Player count to [1..%d]",MaxPlayers));

    if ( (Value > 0) && (Value <= MaxPlayers) )
    {
        fegl.ServerSettings.MaxPlayers = Value;
        GameMgr.SetPlayerLimits(fegl.ServerSettings.MaxPlayers,
                                fegl.ServerSettings.BotsNum,
                                fegl.ServerSettings.BotsEnabled);
        g_TelnetManager.AddDebugText("Set player count to %d",Value);
    }
}
//------------------------------------------------------------------

void telnet_client::SetVotePass( void )
{
    s32 Value;

    Value = GetValue( "Set Vote Pass percentage to [10..100]" );

    if( (Value > 10) && (Value <= 100) )
    {
        // 10 - 100
        fegl.ServerSettings.VotePass = Value;
        g_TelnetManager.AddDebugText( "Set vote pass to %d%%", Value );
    }
}

//------------------------------------------------------------------
void telnet_client::SetBotCount     (void)
{
    s32 Value;

    Value = GetValue("Set Max Bot count to [0..16]");

    if ( (Value > 0) && (Value < MAX_CLIENTS) )
    {
        GameMgr.SetPlayerLimits(fegl.ServerSettings.MaxPlayers,
                                fegl.ServerSettings.BotsNum,
                                fegl.ServerSettings.BotsEnabled);
        g_TelnetManager.AddDebugText("Set bot count to %d",Value);
    }
}

//------------------------------------------------------------------
void telnet_client::SetTeamDamage   (void)
{
    s32 Value;

    Value = GetValue("Set Team Damage to [0..100]");

    if ( (Value >= 0) && (Value <= 100) )
    {

        fegl.ServerSettings.TeamDamage = (f32)Value / 100.0f;
        g_TelnetManager.AddDebugText("Set team damage to %d",Value);
    }
}
//-------------------------------------------------------------------
void telnet_client::SetDuration     (void)
{
    s32 Value;

    Value = GetValue("Set game Duration [1..60]");

    if ( (Value > 1) && (Value <= 60) )
    {
        fegl.ServerSettings.TimeLimit = Value;
        g_TelnetManager.AddDebugText("Set duration to %d",Value);
    }
}
//------------------------------------------------------------------
void telnet_client::ShutdownServer  (void)
{
    char* pOptions[]=
    {
        "Cancel Shutdown",
        "Shutdown Server",
        NULL,
    };

    s32 Result;

    Result = Dialog("CONFIRM SERVER SHUTDOWN",(const char**)pOptions);

    if (Result==1)
    {
        // Shutdown the game server
        g_TelnetManager.AddDebugText("Shutting down the server.");
        g_SM.ServerShutdown = TRUE;
        g_SM.MissionRunning = FALSE;
    }
}

//-------------------------------------------------------------------
void telnet_client::SwitchTeam      (void)
{
}

//-------------------------------------------------------------------
void telnet_client::SetIrcOptions   (void)
{
    static char* YesNo[]=
    {
        "No",
        "Yes",
        NULL
    };

    s32 Enable;
    char ipstr[64];

    const char* pChannel;

    Enable = Dialog("ENABLE IRC LOGGING?",(const char**)YesNo);
    if (Enable<0)
        return;
    
    if (Enable==0)
    {
        g_TelnetManager.AddDebugText("Disabled IRC chat session.");
        g_ChatManager.CloseSession();
        LogMgr.SetIRCStatus( FALSE );
        return;
    }

    Refresh();
    m_Display.SetWindow(m_pInputWindow);
    m_Display.Clear();
    net_address Address;

    Address = g_ChatManager.GetHost();
    net_IPToStr(Address.IP,ipstr);
    m_Display.Print("IRC Chat Server [%s]:",ipstr);
    pChannel = m_Display.GetLine();
    if (pChannel && (*pChannel != 0) )
    {
        if (Address.Port == 0)
        {
            Address.Port = 6667;
        }
        Address.IP = net_ResolveName(pChannel);
        if (Address.IP == 0)
        {
            g_TelnetManager.AddDebugText("Unable to resolve IRC server name '%s'\n",pChannel);
            return;
        }
        else
        {
            g_ChatManager.SetHost(Address);
        }
    }

    m_Display.Clear();
    m_Display.Print("IRC Chat Channel [%s]:",g_ChatManager.GetChannel());
    pChannel = m_Display.GetLine();
    if (pChannel && (*pChannel != 0) )
    {
        g_ChatManager.SetChannel(pChannel);
    }

    m_Display.Clear();
    m_Display.Print("IRC Nick Name [%s]:",g_ChatManager.GetNick());
    pChannel = m_Display.GetLine();
    if (pChannel && (*pChannel != 0) )
    {
        g_ChatManager.SetNick(pChannel);
    }

    net_IPToStr(g_ChatManager.GetHost().IP,ipstr);
    m_Display.Clear();
    m_Display.Print("Please wait. Connecting to IRC server...");
    m_Display.Flush(FALSE);

    if (g_ChatManager.OpenSession())
    {
        g_TelnetManager.AddDebugText("IRC Logging enabled. Server %s:%d\n",
                                        ipstr,
                                        g_ChatManager.GetHost().Port);

        g_TelnetManager.AddDebugText("channel #%s, nickname %s\n",
                                        g_ChatManager.GetChannel(),
                                        g_ChatManager.GetNick());
        LogMgr.SetIRCStatus( TRUE );
    }
    else
    {
        g_TelnetManager.AddDebugText("IRC Logging failed. Server %s:%d, channel #%s\n",
                                        ipstr,
                                        g_ChatManager.GetHost().Port,
                                        g_ChatManager.GetChannel());
    }
}
//-------------------------------------------------------------------
void telnet_client::SetReportOptions (void)
{
    static char* YesNo[]=
    {
        "No",
        "Yes",
        NULL
    };

    s32 Enable;
    const char* pDir;

    Enable = Dialog("ENABLE FILE LOGGING?",(const char**)YesNo);
    if (Enable<0)
        return;
    if( Enable == 0 )
    {
        LogMgr.SetLogStatus( FALSE );
        g_TelnetManager.AddDebugText( "Disabled file logging." );
        return;
    }

    Refresh();
    m_Display.SetWindow(m_pInputWindow);
    m_Display.Clear();
    m_Display.Print( "Log Directory [%s]:", LogMgr.GetLogDir() );
    pDir = m_Display.GetLine();
    if( pDir && (*pDir != 0) )
    {
        LogMgr.SetLogDir( pDir );
    }
    LogMgr.SetLogStatus( TRUE );
    g_TelnetManager.AddDebugText( "File logging enabled.  Output path is '%s'\n", LogMgr.GetLogDir() );
}

//-------------------------------------------------------------------
void telnet_client::SelectGameType  (void)
{
    static char* GameTypes[]=
    {
        "Capture the Flag",
        "Capture and Hold",
        "Team Deathmatch",
        "Deathmatch",
        "Hunter",
        NULL
    };

    s32 GameType;

    if (g_SM.CurrentState != STATE_SERVER_INGAME)
        return;

    GameType = Dialog("SELECT GAME TYPE",(const char**)GameTypes);
    if (GameType<0)
        return;
    g_TelnetManager.AddDebugText("Setting game type to %s",GameTypes[GameType]);
    fegl.GameType = GameType+1;
    GameMgr.EndGame();
    GameMgr.ChangeGameType( (game_type)fegl.GameType );
}

//-------------------------------------------------------------------
extern server_manager* pSvrMgr;
void ChangeServerName( const char* pName, xbool Telnet );
const char* GetServerName( void );

void telnet_client::SetServerName(void)
{
    m_Display.SetWindow(m_pInputWindow);
    m_Display.Clear();
    m_Display.Print("New Server Name:");
    const char* pName;

    pName = m_Display.GetLine();
    if (!pName || *pName==0x0)
        return;

    /*
    // Convert narrow to wide string!
    s32 i = 0;
    while (pName[i] && (i<16) )
    {
        fegl.ServerSettings.ServerName[i]=pName[i];
        i++;
    }
    fegl.ServerSettings.ServerName[i]=0x0;
    */
    ChangeServerName( pName, TRUE );

    pSvrMgr->SetName(fegl.ServerSettings.ServerName);
    tgl.pMasterServer->SetName(fegl.ServerSettings.ServerName);
    tgl.pMasterServer->SetServer(TRUE);

    g_TelnetManager.AddDebugText( "Server name changed to %s", GetServerName() );
}
//-------------------------------------------------------------------
void telnet_client::WriteConfigFile(void)
{
    char Filename[128];
    xwchar* pServername;
    char* pFilename;
    X_FILE *fp;


    pServername = fegl.ServerSettings.ServerName;
    pFilename = Filename;

    while (*pServername)
    {
        if ( (*pServername <=32) || (*pServername >= 128) )
        {
        }
        else
        {
            *pFilename++=(char)*pServername;
        }
        pServername++;
    }

    x_strcpy(pFilename,".settings.txt");

    fp = x_fopen(Filename,"wa");
    if (!fp)
    {
        g_TelnetManager.AddDebugText("Error: Unable to create config file '%s'\n",Filename);
        return;
    }
    char tempstr[64];

    g_TelnetManager.GetAccessKey(tempstr);
    x_fprintf(fp,"KEY           \"%s\"\n",tempstr);
    x_fprintf(fp,"GAMETYPE      \"%s\"\n",s_GameTypes[GameMgr.GetGameType()] );
    x_fprintf(fp,"MAXPLAYERS    %d\n",fegl.ServerSettings.MaxPlayers );
    x_fprintf(fp,"MAXCLIENTS    %d\n",fegl.ServerSettings.MaxClients );
    MakeNarrowString( tempstr, fegl.ServerSettings.ServerName );
    while (tempstr[0] && (tempstr[0]<=' ') && ((u8)tempstr[0]<128) )
    {
        x_strcpy(tempstr,tempstr+1);
    }
    x_fprintf(fp,"SERVERNAME    \"%s\"\n",tempstr );
    MakeNarrowString( tempstr, fegl.ServerSettings.AdminPassword );
    x_fprintf(fp,"PASSWORD      \"%s\"\n",tempstr );
    x_fprintf(fp,"VOTEPASS      %d\n", fegl.ServerSettings.VotePass );
    x_fprintf(fp,"TEAMDAMAGE    %d\n", (s32)(fegl.ServerSettings.TeamDamage*100.0f) );
    x_fprintf(fp,"TIMELIMIT     %d\n",GameMgr.GetTimeLimit() );
    x_fprintf(fp,"LOGGING       \"%s\" \"%s\"\n",LogMgr.GetLogStatus()?"ON":"OFF",LogMgr.GetLogDir());
    x_fprintf(fp,"TELNET        \"%s\" \"%s\"\n",g_TelnetManager.GetAdminUser(),g_TelnetManager.GetAdminPass() );
    x_fprintf(fp,"PORT          %d\n",fegl.PortNumber);
    if (g_ChatManager.GetHost().IP)
    {
        net_IPToStr(g_ChatManager.GetHost().IP,tempstr);
        x_fprintf(fp,"IRC-SERVER    \"%s\" %d\n",tempstr,g_ChatManager.GetHost().Port);
        x_fprintf(fp,"IRC-CHANNEL   \"%s\"\n",g_ChatManager.GetChannel());
        x_fprintf(fp,"IRC-NICKNAME  \"%s\"\n",g_ChatManager.GetNick());
    }
    x_fclose(fp);
    g_TelnetManager.AddDebugText("Configuration saved to '%s'\n",Filename);

}

//-------------------------------------------------------------------
void telnet_client::ChangePassword  (void)
{
    m_Display.SetWindow(m_pInputWindow);
    m_Display.Clear();
    m_Display.Print("New Server Password [blank for none]:");
    const char* pPassword;

    pPassword = m_Display.GetLine();
    if (!pPassword)
        return;
    // Convert narrow to wide string!
    s32 i;

    i=0;
    while (pPassword[i])
    {
        fegl.ServerSettings.AdminPassword[i]=pPassword[i];
        i++;
    }
    fegl.ServerSettings.AdminPassword[i]=0x0;

    if (*pPassword==0)
    {
        pPassword = "<NO PASSWORD>";
    }
    g_TelnetManager.AddDebugText("Password changed to %s",pPassword);
}

//******************************************************************
// Main loop, thread body
//******************************************************************

void telnet_client::Main(void)
{
    s32  i;
    char ch;

    m_Display.Clear();
    m_Display.SetColor(YELLOW);
    m_Display.Print("AADS (Aerial Assault Dedicated Server) Remote Control Console\n");
    m_Display.Print("Version 0.21 compiled %s at %s\n",__DATE__,__TIME__);
    m_Display.SetColor();

    while (m_AuthLevel==AUTHLEVEL_NONE)
    {
        if (m_Retries == 0)
        {
            Stop();
        }
        m_Display.ClearLine(10);
        m_Display.PrintXY(0,10,"Username: ");

        const char* pUsername = m_Display.GetLine();
        if (pUsername)
        {
            x_strcpy(m_Username,pUsername);
        }
        else
        {
            break;
        }
        if (m_Username[0])
        {
            m_Display.ClearLine(11);
            m_Display.PrintXY(0,11,"Password: ");
            const char* pPassword = m_Display.GetLine(TRUE);
            if (pPassword)
            {
                x_strcpy(m_Password,pPassword);
            }
            else
            {
                break;
            }

            i = 0;
            if (x_strcmp(m_Username,"inevitable")==0)
            {
                // Inevitable level admin control. We use this inline mode
                // as it'll hide the admin password a bit better.
                if ( (m_Password[0]=='p') && (m_Password[1]=='e') &&
                     (m_Password[2]=='e') && (m_Password[3]=='k') &&
                     (m_Password[4]=='a') && (m_Password[5]=='b') &&
                     (m_Password[6]=='o') && (m_Password[7]=='o') &&
                     (m_Password[8]==0x0) )
                {
                    m_AuthLevel = AUTHLEVEL_ADMIN;
                }
            }
            else if ( g_TelnetManager.GetAdminUser()[0] &&
                      g_TelnetManager.GetAdminPass()[0] && 
                      (x_strcmp(m_Username,g_TelnetManager.GetAdminUser())==0) &&
                      (x_strcmp(m_Password,g_TelnetManager.GetAdminPass())==0) )
            {
                m_AuthLevel = AUTHLEVEL_USER;
            }

            if (m_AuthLevel==AUTHLEVEL_NONE)
            {
                switch( m_Retries )
                {
                case 4: m_Display.Print( "\n\n" );
                        m_Display.Print( "*** Invalid Username/Password ***\n" );
                        m_Display.Print( "WARNING: Type VERY CAREFULLY to avoid engaging security measures." );
                        break;

                case 3: m_Display.SetColor(YELLOW);
                        m_Display.Print( "\n\n\n\n\n" );
                        m_Display.Print( "*** Unauthorized Access Suspected ***\n" );
                        m_Display.Print( "IP address logged for investigation.\n" );
                        break;

                case 2: m_Display.SetColor(RED);
                        m_Display.Print( "\n\n\n\n\n\n\n\n\n" );
                        m_Display.Print( "*** Security Measures Engaged ***\n" );
                        m_Display.Print( "Uploading Hacker Tracker v3.2 virus ... 100%% complete\n" );
                        m_Display.Print( "Have a nice day!\n" );
                        break;
                }

                m_Retries--;
            }
        }
    }
    m_Display.SetColor();

    if (m_AuthLevel == AUTHLEVEL_NONE)
    {
        Stop();
    }
    else
    {
        Refresh();
    }

    while( !m_Exit )
    {
        ch = WaitForCommand("Command", 
                            s_HelpMessage[m_AuthLevel-1],
                            YELLOW,
                            BLUE);
        switch(ch)
        {
        case 'K':
            KickPlayer();
            break;
        case 'M':
            SwitchMap();
            break;
        case  'C':
            SetClientCount();
            break;
        case 'P':
            SetPlayerCount();
            break;
#if 0
        case 'B':
            SetBotCount();
            break;
#endif
        case 'D':
            SetDuration();
            break;
        case 'S':
            ShutdownServer();
            break;
        case 'Y':
            ChangePassword();
            break;
        case 'R':
            m_Display.Flush(TRUE);
            break;
        case 'G':
            SelectGameType();
            break;
        case 'L':
            SetReportOptions();
            break;
        case 'I':
            SetIrcOptions();
            break;
        case 'T':
            SetTeamDamage();
            break;
        case 'F':
            TeamswitchPlayer();
            break;
        case 'J':
            DropPlayer();
            break;
        case 'V':
            SetVotePass();
            break;
        case 'Q':
            m_Display.SetWindow(NULL);
            m_Display.Clear();
            /*
            m_Display.Print("Game over man! Game Over!\n"
                            "Goodbye and thank you for hosting Tribes:AADS.\n"
                            "Please remember to check for updated versions at\n"
                            "www.tribesaa.com. Current version is %s.\n",VERSION_NUMBER);
            */
            m_Exit = TRUE;
            break;
        case 'N':
            SetServerName();
            break;
        case 'W':
            WriteConfigFile();
            break;
        case '1':
            if (m_DisplayMode == DISP_MODE_PLAYERS)
            {
                // Change the index for this screen starting position
                m_BasePlayerIndex = m_LastPlayerIndex+1;
            }
            else
            {
                m_DisplayMode = DISP_MODE_PLAYERS;
                m_BasePlayerIndex = 0;
            }
            break;
        case '2':
            m_DisplayMode = DISP_MODE_SERVER;
            break;
        case '3':
            m_DisplayMode = DISP_MODE_LOG;
            break;
        case '4':
            if (m_AuthLevel == AUTHLEVEL_ADMIN)
            {
                if (m_DisplayMode == DISP_MODE_CLIENTS)
                {
                    m_BaseClientIndex = m_LastClientIndex+1;
                    // Change the index for this screen starting position
                }
                else
                {
                    m_DisplayMode = DISP_MODE_CLIENTS;
                    m_BaseClientIndex = 0;
                }
            }
            break;
        }
    }
    Stop();
}

//-------------------------------------------------------------------
// Main update functions
//-------------------------------------------------------------------
void  telnet_client::RefreshLogText(void)
{
    m_Display.Clear();

    s32 index;
    f32 LogTime;
    s32 i;

    index = g_TelnetManager.GetLogIndex();

    for (i=m_Display.GetHeight()-1;i>=0;i--)
    {
        index--;
        if (index<0)
        {
            index = g_TelnetManager.GetLogSize()-1;
        }
        const char* pLogText;

        pLogText = g_TelnetManager.GetLogText(index);
        LogTime  = g_TelnetManager.GetLogTime(index);

        if (*pLogText)
        {
            m_Display.PrintXY(0,i,"%2.2f: %s",LogTime,pLogText);
        }

    }
}

/*******************************************************************************
             11111111112222222222333333333344444444445555555555666666666677777777
   012345678901234567890123456789012345678901234567890123456789012345678901234567

-1+------------NAME-SCORE-KILL-TK-VT-PING-------------NAME-SCORE-KILL-TK-VT-PING-+
 0|      Team Storm 12345 1234 12 12 1234     Team Inferno 12345 1234 12 12 1234 |
 1| 123456789ABCDEF  1234  123 12 12 1234  123456789ABCDEF  1234  123 12 12 1234 |

*******************************************************************************/

void telnet_client::RefreshPlayers( void )
{
    s32 i;
    s32 PlayerCount = 0;

    client_stats      Stats;
    const game_score& Score = GameMgr.GetScore();

    m_Display.SetColor( YELLOW, BLUE );
    m_Display.Clear();

    m_Display.SetJustify();

    // Draw title bar

    m_Display.PrintXY( 12, -1, "NAME"  );
    m_Display.PrintXY( 17, -1, "SCORE" );
    m_Display.PrintXY( 23, -1, "KILL"  );
    m_Display.PrintXY( 28, -1, "TK"    );
    m_Display.PrintXY( 31, -1, "VT"    );
    m_Display.PrintXY( 34, -1, "PING"  );

    m_Display.PrintXY( 51, -1, "NAME"  );
    m_Display.PrintXY( 56, -1, "SCORE" );
    m_Display.PrintXY( 62, -1, "KILL"  );
    m_Display.PrintXY( 67, -1, "TK"    );
    m_Display.PrintXY( 70, -1, "VT"    );
    m_Display.PrintXY( 73, -1, "PING"  );

    if( Score.IsTeamBased )
    {
        s32 T;

        s32 Kill[2] = { 0, 0 };
        s32 TKs [2] = { 0, 0 };
        s32 Vote[2] = { 0, 0 };
        s32 Ping[2] = { 0, 0 };
        s32 Size[2] = { 0, 0 };

        s32 X   [2] = { 0,     39    };
        s32 Y   [2] = { 1,      1    };

//      char* Name[2] = { "TEAM STORM", "TEAM INFERNO" };

        // Display all player information and collect totals, too.

        for( i = 0; i < 32; i++ )
        {
            m_Display.SetColor( YELLOW, BLUE );

            if( !Score.Player[i].IsConnected )
                continue;

            T = Score.Player[i].Team;

            if( Y[T] > m_Display.GetHeight() )
                continue;

            // CTF - SPECIAL CASE
            if( Score.GameType == GAME_CTF )
            {
                s32     Status;
                vector3 Position;
                pGameLogic->GetFlagStatus( (u32)(2-T), Status, Position );
                if( Status == i )
                {
                    m_Display.SetColor( WHITE, BLUE );
                }
            }

            m_Display.PrintXY( X[T] +  1, Y[T], "%15s%6d%5d%3d%3d",
                                                Score.Player[i].NName,
                                                Score.Player[i].Score,
                                                Score.Player[i].Kills,
                                                Score.Player[i].TKs,
                                                Score.Player[i].Votes );

            client_stats Stats;
            if( tgl.pServer && (tgl.pServer->GetClientStats( Score.Player[i].Machine, Stats )) )
            {
                if( Stats.PlayerCount > 1 )  m_Display.SetColor( GREEN,  BLUE );
                else                         m_Display.SetColor( YELLOW, BLUE );
                m_Display.PrintXY( X[T] + 33, Y[T], "%5d", (s32)Stats.Ping );
                Ping[T] += (s32)Stats.Ping;
            }

            Kill[T] += Score.Player[i].Kills;
            TKs [T] += Score.Player[i].TKs;
            Vote[T] += Score.Player[i].Votes;
            Size[T] += 1;
            Y   [T] += 1;
        }
        
        // Now for the TEAM information.
        for( T = 0; T < 2; T++ )
        {
            m_Display.SetColor( WHITE, BLACK );

            // CTF - SPECIAL CASE
            if( Score.GameType == GAME_CTF )
            {
                s32     Status;
                vector3 Position;
                pGameLogic->GetFlagStatus( (u32)(T+1), Status, Position );
                if( Status == -2 )  // On stand
                {
                    m_Display.SetColor( GREEN, BLACK );
                }
                if( Status == -1 ) // Afield
                {
                    m_Display.SetColor( YELLOW, BLACK );
                }
                if( Status >= 0 ) // In enemy hands
                {
                    m_Display.SetColor( RED, BLACK );
                }
            }

            m_Display.PrintXY( X[T] +  1, 0, "TEAM %10s", Score.Team[T].NName );

            m_Display.SetColor( WHITE, BLACK );

            m_Display.PrintXY( X[T] + 16, 0, "%6d%5d%3d%3d%5d",
                                             Score.Team[T].Score,
                                             Kill[T],
                                             TKs[T],
                                             Vote[T],
                                             (Size[T] > 0)
                                                ? (Ping[T] / Size[T])
                                                : (        0        ) );
        }
    }

    if( !Score.IsTeamBased )
    {
        m_Display.SetColor( YELLOW, BLUE );

        s32 X[2] = { 0, 39 };
        s32 Y[2] = { 0,  0 };

        s32 C;

        for( i = 0; i < 32; i++ )
        {
            m_Display.SetColor( YELLOW, BLUE );

            if( !Score.Player[i].IsConnected )
                continue;

            // Which column?
            if( Y[0] > Y[1] )  C = 1;
            else               C = 0;

            if( Y[C] > m_Display.GetHeight() )
                continue;

            m_Display.PrintXY( X[C] +  1, Y[C], "%15s%6d%5d%3d%3d",
                                                Score.Player[i].NName,
                                                Score.Player[i].Score,
                                                Score.Player[i].Kills,
                                                Score.Player[i].TKs,
                                                Score.Player[i].Votes );

            client_stats Stats;
            if( tgl.pServer && (tgl.pServer->GetClientStats( Score.Player[i].Machine, Stats )) )
            {
                if( Stats.PlayerCount > 1 )  m_Display.SetColor( GREEN,  BLUE );
                else                         m_Display.SetColor( YELLOW, BLUE );
                m_Display.PrintXY( X[C] + 33, Y[C], "%5d", (s32)Stats.Ping );
            }

            // Next row in this column.
            Y[C] += 1;
        }
    }
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
s32 telnet_client::RefreshClients(s32 ClientIndex)
{
    s32 LineCount;
    s32 i;
    client_stats Stats;
    s32 ClientCount=0;

    if (m_AuthLevel !=AUTHLEVEL_ADMIN )
    {
        return 0;
    }
    m_Display.SetColor(WHITE,RED);
    m_Display.Clear();
    m_Display.SetColor(WHITE,RED);
    // Draw title bar
    m_Display.SetJustify(CENTER);
    m_Display.PrintXY( 0+(4/2),-1,"CONN");
    m_Display.PrintXY( 4+(22/2),-1,"ADDRESS");
    m_Display.PrintXY(26+(7/2),-1,"INT.");
    m_Display.PrintXY(33+(7/2),-1,"IN");
    m_Display.PrintXY(40+(7/2),-1,"OUT");
    m_Display.PrintXY(47+(7/2),-1,"PING");
    m_Display.PrintXY(54+(8/2),-1,"DURATION");

    m_Display.SetJustify();

    LineCount = m_Display.GetHeight();

    s32 xpos,ypos;

    xpos = 0;
    ypos = 0;
    for (i=ClientIndex;i<MAX_CLIENTS;i++)
    {
        s32 ConnId = MAX_CLIENTS-1-i;
        xbool IsPresent;
        IsPresent = tgl.pServer->GetClientStats(ConnId,Stats);

        if (IsPresent)
        {
            ClientCount++;
            char ipstring[64];

            net_IPToStr(Stats.Address.IP,ipstring);
            s32 time;
            s32 hours,minutes,seconds;

            time = (s32)Stats.ConnectTime;

            seconds = (s32)(time % 60); time = time / 60;
            minutes = (s32)(time % 60); time = time / 60;
            hours   = (s32)time;

            m_Display.SetJustify(CENTER);
            m_Display.PrintXY(xpos+ 0+(4/2),ypos, "%d",ConnId);
            m_Display.PrintXY(xpos+ 4+(22/2),ypos,"%s:%d",ipstring,Stats.Address.Port);
            m_Display.PrintXY(xpos+26+(7/2),ypos,"%2.02f",Stats.ShipInterval);
            m_Display.PrintXY(xpos+33+(7/2),ypos,"%d",(u32)((u32)(Stats.BytesIn-m_LastBytesIn[i])/m_RefreshInterval));
            m_Display.PrintXY(xpos+40+(7/2),ypos,"%d",(u32)((u32)(Stats.BytesOut-m_LastBytesOut[i])/m_RefreshInterval));
            m_Display.PrintXY(xpos+47+(7/2),ypos,"%d",(u32)Stats.Ping);
            m_Display.PrintXY(xpos+54+(9/2),ypos,"%d:%02d:%02d",hours,minutes,seconds);

            m_Display.SetJustify();
            ypos++;
            if (ypos >= m_Display.GetHeight())
                break;
        }
    }

    m_Display.SetColor();
    return i;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
extern s32 g_ClientsDropped;
extern s32 g_ClientsConnected;
void telnet_client::RefreshServer(void)
{
    client_stats Stats;
    s32 ClientCount=0;
    s32 ClientIndex = 0;
    s32 ypos=0;


    u32 PacketsSent,PacketsReceived;
    u64 BytesSent, BytesReceived;
    u32 AddressesBound;
    f32 SendTime,ReceiveTime;
    char postfix;

    net_GetStats(PacketsSent,BytesSent,PacketsReceived,BytesReceived,
                 AddressesBound,SendTime,ReceiveTime);
    u32 outsec,insec;

    insec = (u32)(BytesReceived-m_LastBytesReceived);
    outsec  = (u32)(BytesSent - m_LastBytesSent);

    m_Display.SetColor(BLUE,GREEN);
    m_Display.Clear();
    m_Display.SetColor(BLUE,GREEN);

    ypos = 0;
    m_Display.ClearLine(ypos);
    m_Display.PrintXY(0,ypos,   "  GAME TYPE: %s",s_GameTypes[GameMgr.GetGameType()]);;
    m_Display.PrintXY(26,ypos,  "   MAP NAME: %s",tgl.MissionName);
    m_Display.PrintXY(52,ypos,  "      GAMES: %d",0);

    ypos++;
    m_Display.PrintXY(0,ypos,   "   PROGRESS: %d%%",(s32)(GameMgr.GetGameProgress()*100.2f));
    m_Display.PrintXY(26,ypos,  " GAME STATE: %s",sm_GetState());

    ypos++;
    if (BytesReceived>1024*1024)
    {
        postfix = 'K';
        BytesReceived /= 1024;
        if (BytesReceived >1024*1024)
        {
            postfix='M';
        }
    }
    else
    {
        postfix = ' ';
    }
    m_Display.PrintXY(0,ypos,   "  CONNECTED: %d",g_ClientsConnected);
    m_Display.PrintXY(26,ypos,  "   BYTES IN: %d%c",(s32)BytesReceived,postfix);

    if (BytesSent>1024*1024)
    {
        postfix = 'K';
        BytesSent /= 1024;
        if (BytesSent>1024*1024)
        {
            BytesSent/=1024;
            postfix='M';
        }
    }
    else
    {
        postfix = ' ';
    }
    m_Display.PrintXY(52,ypos,  "  BYTES OUT: %d%c",(s32)BytesSent,postfix);

    ypos++;
    m_Display.PrintXY(0,ypos,   "    DROPPED: %d",g_ClientsDropped);
    m_Display.PrintXY(26,ypos,  "     IN/SEC: %d",(u32)(insec/m_RefreshInterval));
    m_Display.PrintXY(52,ypos,  "    OUT/SEC: %d",(u32)(outsec/m_RefreshInterval));

    ypos++;
    m_Display.PrintXY(0,ypos,   "    LOOKUPS: %d",pSvrMgr->GetLookupRequests());
    m_Display.PrintXY(26,ypos,  " PACKETS IN: %d",PacketsReceived);
    m_Display.PrintXY(52,ypos,  "PACKETS OUT: %d",PacketsSent);

    ypos++;
//    m_Display.PrintXY(0,ypos,   "     UNIQUE: %d",pSvrMgr->GetUniqueCount());
    m_Display.PrintXY(26,ypos,  " PKT IN/SEC: %d",(s32)((PacketsReceived-m_LastPacketsReceived)/m_RefreshInterval));
    m_Display.PrintXY(52,ypos,  "PKT OUT/SEC: %d",(s32)((PacketsSent-m_LastPacketsSent)/m_RefreshInterval));

    ypos++;

    char pwstring[64];

    MakeNarrowString( pwstring, fegl.ServerSettings.AdminPassword );
    if( pwstring[0] == 0x0 )
    {
        x_strcpy( pwstring, "<NO PASSWORD>" );
    }

    m_Display.PrintXY( 0,ypos,  "  VOTE PASS: %d%%",fegl.ServerSettings.VotePass);
    m_Display.PrintXY(26,ypos,  "   PASSWORD: %s",pwstring);

    ypos++;                      
    m_Display.PrintXY(0,ypos,   "  FILE LOGS: %s",LogMgr.GetLogStatus()?"Enabled":"Disabled");
    m_Display.PrintXY(26,ypos,  "   LOG PATH: \"%s\"",LogMgr.GetLogDir());

    {
        char ipstr[64];
        ypos++;
        net_IPToStr(g_ChatManager.GetHost().IP,ipstr);
        m_Display.PrintXY(0,ypos,  "   IRC CHAT: %s",g_ChatManager.IsConnected()?"Enabled":"Disabled");
        m_Display.PrintXY(26,ypos, " IRC SERVER: %s",ipstr);
        ypos++;
        m_Display.PrintXY(0,ypos,  "    CHANNEL: %s",g_ChatManager.GetChannel());
        m_Display.PrintXY(26,ypos, "   NICKNAME: %s",g_ChatManager.GetNick());
    }

    m_Display.SetColor();
}

void telnet_client::RefreshLog(void)
{

    m_Display.SetWindow(m_pExtendedLogWindow);
    m_Display.SetColor(WHITE,MAGENTA);
    RefreshLogText();
}

//-------------------------------------------------------------------
// Updates all internal logging of the game state to get the deltas
// between updates of data change.
void telnet_client::SampleGameState(void)
{
    s32 i;
    // Update client tracking
    client_stats Stats;
    if (m_RefreshTimer.ReadSec() < 1.0f)
        return;

    m_ClientCount = 0;
    m_RefreshTimer.Stop();
    m_RefreshInterval = m_RefreshTimer.ReadSec();
    m_RefreshTimer.Reset();
    m_RefreshTimer.Start();
    if (m_RefreshInterval < 0.10f)
    {
        m_RefreshInterval = 0.1f;
    }
    for (i=0;i<MAX_CLIENTS;i++)
    {
        s32 ConnId = MAX_CLIENTS-1-i;

        if (tgl.pServer && tgl.pServer->GetClientStats(ConnId,Stats))
        {
            m_LastBytesIn[i]=Stats.BytesIn;
            m_LastBytesOut[i]=Stats.BytesOut;
            m_ClientCount ++;
        }
        else
        {
            m_LastBytesIn[i]=0x0;
            m_LastBytesOut[i]=0x0;
        }
    }

    
    // Update server tracking
    u32 AddressesBound;
    f32 SendTime,ReceiveTime;

    net_GetStats(m_LastPacketsSent,m_LastBytesSent,
                 m_LastPacketsReceived,m_LastBytesReceived,
                 AddressesBound,SendTime,ReceiveTime);
}

// Help for low authentication users
const char* s_MainMenuHelp[]=
{
    " SERVER MANAGEMENT:                 GAME MANAGEMENT:\n",
    "   N - Change server name             M - Change map (immediate)\n",
    "   Y - Change server password         T - Change team damage\n",
    "   C - Change max clients             V - Change vote percentage\n",
    "   P - Change max players             D - Change game duration\n",
    "   I - Change IRC radio options       L - Change file logging\n",                   
    "   W - Write configuration file \n",
    "   Q - Quit telnet session          DISPLAY MANAGEMENT:\n",                         
    "   S - Shut down server               R - Refresh entire display\n",
    "                                      1 - Player statistics\n",                  
    " PLAYER MANAGEMENT:                   2 - Server information\n",                    
    "   F - Teamchange player (Flip)       3 - Extended text log\n",                     
    "   K - Kick (and ban) player          <TAB> cycles through the display modes.\n",   
    "   J - Drop (not ban) player \n",            
    NULL
};

// Help for admin authenticated users
const char* s_AdminMenuHelp[]=
{
    " SERVER MANAGEMENT:                 GAME MANAGEMENT:\n",
    "   N - Change server name             M - Change map (immediate)\n",
    "   Y - Change server password         T - Change team damage\n",
    "   C - Change max clients             V - Change vote percentage\n",
    "   P - Change max players             D - Change game duration\n",
    "   I - Change IRC radio options       L - Change file logging\n",
    "   W - Write configuration file       I - Change IRC logging\n",
    "   Q - Quit telnet session \n",
    "   S - Shut down server             DISPLAY MANAGEMENT:\n",
    "                                      R - Refresh entire display\n",
    " PLAYER MANAGEMENT:                   1 - Player statistics\n",
    "   F - Teamchange player (Flip)       2 - Server information\n",
    "   K - Kick (and ban) player          3 - Extended text log\n",
    "   J - Drop (not ban) player          4 - Client statistics\n",
    NULL
};

const char** s_HelpMessage[HM_TOTAL]=
{
    s_MainMenuHelp,
    s_AdminMenuHelp,
};

