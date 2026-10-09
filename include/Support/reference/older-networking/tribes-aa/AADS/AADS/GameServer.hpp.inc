//=========================================================================
//
//  GameServer.hpp
//
//=========================================================================
#ifndef GAMESERVER_HPP
#define GAMESERVER_HPP

#include "x_files.hpp"
#include "netlib/netlib.hpp"
#include "netlib/bitstream.hpp"
#include "connmanager.hpp"
#include "updatemanager.hpp"
#include "objectmgr/objectmgr.hpp"

//=========================================================================
#define MAX_CLIENTS 32

//=========================================================================
struct client_stats
{
	s32						PlayerCount;
	object::id				PlayerID[2];
	f32						Ping;
	f32						ConnectTime;
	f32						ShipInterval;
	s32						BytesIn;
	s32						BytesOut;
	net_address				Address;
};

class game_server
{
private:

    struct client_instance
    {
        xbool               Connected;
        xbool               Kick;
        xbool               Drop;
        net_address         Address;
        s32                 NPlayers;
        xwchar              Name[64];
        s64                 LastHeartbeat;
        conn_manager        ConnManager;
        update_manager      UpdateManager;
        move_manager        MoveManager;
        game_event_manager  GameEventManager;
        object::id          PlayerObjID[2];
        s32                 PlayerIndex[2];    // Index in GameMgr.
        f32                 PacketShipDelay;
        f32                 HeartbeatDelay;
        s32                 SystemId;
    };

    net_address*            m_Address;
    s64                     m_LastClientNote;
    xwchar                  m_Title[64];

    xbool                   m_AllowClients;
    client_instance         m_Client[MAX_CLIENTS];
    s32                     m_nClients;
    xbool                   m_MissionLoaded[ obj_mgr::MAX_SERVER_OBJECTS ];
    s32                     m_MissionLoadedSeq[ obj_mgr::MAX_SERVER_OBJECTS ];

    s32                     m_BanList[16];
    s32                     m_BanWait[16];

public:
    s32                     m_MissionLoadedIndex;

public:
    game_server(void);
    ~game_server(void);

    void        InitNetworking  ( net_address& Addr, const xwchar* pTitle );
    net_address GetAddress      ( void );
    void        SyncMissionLoad ( void );
    void        ProcessPacket   ( bitstream& BitStream, net_address& SenderAddr );
    void        ProcessTime     ( f32 DeltaSec );
    void        HandleLoginPacket( bitstream& BitStream, net_address& SenderAddr );
    void        SendHeartbeat   ( void );

    void        KickPlayer      ( s32 PlayerIndex );
    void        DropPlayer      ( s32 PlayerIndex );

    void        SendMsg         ( const msg& Msg );
//  void        SendMessage     ( const char* Str, xcolor Color=XCOLOR_WHITE, u32 TeamBits=0xFFFFFFFF );
//  void        SendPopMessage  ( const char* Str, xcolor Color=XCOLOR_WHITE, s32 PlayerIndex = -1 );

    void        SendAudio       ( s32 SampleID, s32 PlayerIndex );
    void        SendAudio       ( s32 SampleID, u32 TeamBits=0xFFFFFFFF );
    void        BroadcastEndMission( void );
    void        CullClient      ( s32 ID );

    void        UnloadMission    ( void );
    xbool       CoolDown        ( void );
    xbool       DisconnectEveryone( void );

    void        BanClient       ( s32 ClientSystemId );
    xbool       IsBanned        ( s32 ClientSystemId );
    void        ClearBanList    ( void );
    void        RelaxBanList    ( void ); 
    s32         GetBanCount     ( void );

    void        DisplayPings    ( s32 L );
    void        DumpStats       ( X_FILE* fp = NULL );
    void        AdvanceStats    ( void );
    void        ShowLifeDelay   ( void );
	f32			GetAveragePing  ( void );
	s32			GetClientCount	( void );
	xbool		GetClientStats	( s32 index, client_stats& Stats );
};

//=========================================================================
#endif
//=========================================================================

