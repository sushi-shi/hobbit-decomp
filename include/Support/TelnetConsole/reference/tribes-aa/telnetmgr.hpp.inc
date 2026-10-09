#ifndef __TELNETMGR_HPP
#define __TELNETMGR_HPP

#include "netlib/netlib.hpp"
#include "chatclient.hpp"
#include <xCore/x_files/x_threads.hpp>

#define TELNET_LISTEN_PORT  49152
#define TELNET_MAX_CLIENTS  4
#define TELNET_LOG_HISTORY  80
#define TELNET_LINE_WIDTH   256
enum stat_id
{
    STAT_ID_VOTES,
    STAT_ID_TEAMKILLS,
    STAT_ID_RANKING,
    STAT_ID_LAST,
};

class telnet_client;

class telnet_manager
{
public:
                    telnet_manager  (void);
    void            Init            (void);
    void            Kill            (void);
    void            Update          (f32 DeltaTime);
    void            AddDebugText    (const xwchar* pString);
    void            AddDebugText    (const char* pFormat,...);
    s32             GetInstance (void) { return m_TelnetPort.Port - TELNET_LISTEN_PORT;};
    void            SetStatistic    (s32 PlayerId, stat_id Statistic, s32 Value)    { m_PlayerStats[PlayerId][Statistic] =Value; }
    void            AddStatistic    (s32 PlayerId, stat_id Statistic, s32 Value=1)  { m_PlayerStats[PlayerId][Statistic]+=Value; }
    s32             GetStatistic    (s32 PlayerId, stat_id Statistic)               { return m_PlayerStats[PlayerId][Statistic]; }
    void            ClearStatistics (s32 PlayerId)                                  { x_memset(&m_PlayerStats[PlayerId],0,sizeof(m_PlayerStats[PlayerId]));}
    f32             GetLogTime      (s32 Index)                                     { return m_LogTextTime[Index];      }
    const char*     GetLogText      (s32 Index)                                     { return m_LogText[Index];          }
    s32             GetLogIndex     (void)                                          { return m_LogTextIndex;            }
    s32             GetLogSize      (void)                                          { return TELNET_LOG_HISTORY;        }
    void            SetAdminUser    (const char* pUser)                             { x_strncpy(m_AdminUser,pUser,sizeof(m_AdminUser)); }
    void            SetAdminPass    (const char* pPass)                             { x_strncpy(m_AdminPass,pPass,sizeof(m_AdminPass)); }
    const char*     GetAdminUser    (void)                                          { return m_AdminUser;               }
    const char*     GetAdminPass    (void)                                          { return m_AdminPass;               }
    void            SetAccessKey    (const char* pKey);
    void            GetAccessKey    (char* pKey);
    xbool           ValidateAccess  (void);

    void            TTYMessage      ( const char* pMessage );

private:
    void            DumpGameState   (f32 DeltaTime);


    telnet_client*  m_Clients[TELNET_MAX_CLIENTS];
    net_address     m_Local;
    net_address     m_TelnetPort;

    char            m_AdminUser[32];
    char            m_AdminPass[32];
    char            m_LogText[TELNET_LOG_HISTORY][TELNET_LINE_WIDTH];
    f32             m_LogTextTime[TELNET_LOG_HISTORY];
    s32             m_LogTextIndex;
    unsigned char   m_AccessKey[12];

    s32             m_PlayerStats[32][STAT_ID_LAST];
};

//const char* MakeNarrowString(char* Narrow,const xwchar* pWide);

xbool OpenTcpPort(net_address& Address, const net_address& Host);

extern telnet_manager g_TelnetManager;

#endif