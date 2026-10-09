#ifndef __TELNET_CLIENT_HPP
#define __TELNET_CLIENT_HPP

#include "telnetdisplay.hpp"
#include <xCore/x_files/x_threads.hpp>
#include "gamemgr/gamemgr.hpp"

#define MAX_CLIENTS 32

#define AUTHLEVEL_NONE  (0)
#define AUTHLEVEL_USER  (1)
#define AUTHLEVEL_ADMIN (2)

class telnet_client
{
public:
    void            Init            (const net_address& Local);
    void            Kill            (void);
    void            Main            (void);
    xbool           Update          (f32 DeltaTime);
    void            Refresh         (void);
private:

    enum disp_mode
    {
        DISP_MODE_PLAYERS=0,
        DISP_MODE_CLIENTS,
        DISP_MODE_SERVER,
        DISP_MODE_LOG,

        DISP_MODE_LAST,
    };

    net_address     m_Local;
    net_address     m_Remote;
    xthread*        m_pThread;

    // Authentication
    s32             m_AuthLevel;
    s32             m_Retries;
    char            m_Username[TELNET_LINE_LENGTH];
    char            m_Password[TELNET_LINE_LENGTH];

    xbool           m_Exit;

    // Display
    f32             m_UpdateInterval;
    f32             m_RefreshInterval;
    xtimer          m_RefreshTimer;
    telnet_display  m_Display;
    s32             m_DisplayMode;
    s32             m_LastDisplayMode;
    telnet_display::window* m_pInfoWindow;
    telnet_display::window* m_pInputWindow;
    telnet_display::window* m_pLogWindow;
    telnet_display::window* m_pExtendedLogWindow;

    s32             m_LastPlayerIndex;
    s32             m_BasePlayerIndex;
    s32             m_LastClientIndex;
    s32             m_BaseClientIndex;

    s32             m_ClientCount;
    u64             m_LastBytesReceived;
    u64             m_LastBytesSent;
    u32             m_LastPacketsReceived;
    u32             m_LastPacketsSent;
    u64             m_LastBytesIn[MAX_CLIENTS];
    u64             m_LastBytesOut[MAX_CLIENTS];

    // Control
    char            WaitForCommand  (const char* Label, 
                                     const char** Help,
                                     const telnet_color foreground=WHITE,
                                     const telnet_color background=BLACK);
    s32             GetValue        (const char* pLabel);
    void            Stop            (void);
    // Command processing functions
    void            KickPlayer      (void);
    void            SwitchTeam      (void);
    void            SwitchMap       (void);
    void            SetClientCount  (void);
    void            SetPlayerCount  (void);
    void            SetBotCount     (void);
    void            SetVotePass     (void);
    void            SetDuration     (void);
    void            SetServerName   (void);
    void            SetTeamDamage   (void);
    void            SetReportOptions(void);
	void			SetIrcOptions	(void);
    void            TeamswitchPlayer(void);
    void            DropPlayer      (void);
    void            WriteConfigFile (void);
    void            ShutdownServer  (void);
    void            ChangePassword  (void);
    void            SelectGameType  (void);

    void            RefreshPlayers  (void);
    s32             RefreshClients  (s32 ClientIndex);
    void            RefreshServer   (void);
    void            RefreshLog      (void);
    void            SampleGameState (void);
    void            RefreshLogText  (void);
    s32             Dialog          (const char* pTitle, const char** pOptions);


friend telnet_display;
};



#endif