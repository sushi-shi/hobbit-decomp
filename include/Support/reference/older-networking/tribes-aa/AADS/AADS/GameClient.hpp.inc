//=========================================================================
//
//  GameClient.hpp
//
//=========================================================================
#ifndef GAMECLIENT_HPP
#define GAMECLIENT_HPP

#include "x_types.hpp"
#include "netlib/netlib.hpp"
#include "netlib/bitstream.hpp"
#include "connmanager.hpp"
#include "Objects\Player\PlayerObject.hpp"


//=========================================================================

class game_client
{
    net_address*        m_Address;
    net_address*        m_ServerAddress;
    s64                 m_LastServerNote;
    xwchar              m_Title[64];
                        
    s32                 m_NPlayers;
    xwchar              m_Name[64];
    object::id          m_PlayerObjID[2];
                        
    xwchar              m_ServerTitle[32];
    xbool               m_LoginRefused;
                        
    xbool               m_LoggedIn;
    xbool               m_InSync;
    f32                 m_LastConnectCheck;
    s64                 m_LastHeartbeat;
    conn_manager        m_ConnManager;
    update_manager      m_UpdateManager;
    move_manager        m_MoveManager;
    game_event_manager  m_GameEventManager;
    f32                 m_HeartbeatDelay;
    f32                 m_PacketShipDelay;
	s32					m_ClientIndex;

public:
    game_client(void);
    ~game_client(void);

    void        InitNetworking  ( net_address&  Addr, 
                                  net_address&  ServerAddr,
                                  const xwchar* pTitle,
                                  const xwchar* pName,
								  const s32 ClientIndex);

    net_address GetAddress      ( void );
    object::id  GetPlayerObjID  ( s32 ID ) const { return( m_PlayerObjID[ID] ); };
    void        ProcessPacket   ( bitstream& BitStream, net_address& SenderAddr );
    void        ProcessTime     ( f32 DeltaSec );
    f32         GetPing         ( void );

    xbool       IsLoggedIn      ( void ) {return m_LoggedIn;}
    xbool       IsLoginRefused  ( void ) {return m_LoginRefused;}

    void        RequestMission  ( void );

    void        UnloadMission   ( void );

    xbool       Disconnect      ( void );
    xbool       IsInSync        ( void ) { return m_InSync; }

    void        ProcessHeartbeat( f32 DeltaSec );

    void        BuildPingGraph      ( xbitmap &Bitmap ) { m_ConnManager.BuildPingGraph(Bitmap);};

    void        EndMission      ( void );
};

//=========================================================================
#endif
//=========================================================================

