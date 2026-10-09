//=========================================================================
//
//  GameServer.cpp
//
//=========================================================================

#include "entropy.hpp"
#include "globals.hpp"
#include "fe_globals.hpp"
#include "gameserver.hpp"
#include "gameclient.hpp"
#include "gameuser.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Player\DefaultLoadouts.hpp"
#include "LabelSets\Tribes2Types.hpp"
#include "ServerVersion.hpp"
#include "GameMgr\GameMgr.hpp"
#include "audiomgr\audio.hpp"
#include "hud\hud_manager.hpp"
#include "ClientServer.hpp"

xbool SHOW_SERVER_PACKETS = FALSE;

#ifdef X_DEBUG
//#define SHOW_PACKET_SENDS    
#endif

//static xtimer Timer1[MAX_CLIENTS];
//static xtimer Timer2[MAX_CLIENTS];

//=========================================================================

#define DMT_SHOW_STUFF

s32 g_ClientsDropped = 0;
s32 g_ClientsConnected = 0;

//=========================================================================

game_server::game_server( void )
{
    m_LastClientNote = tgl.LogicTimeMs;
    m_nClients = 0;
    m_AllowClients = TRUE;
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    {
        m_Client[i].Connected = FALSE;
        m_Client[i].Kick      = FALSE;
#if !defined(RELEASE_CANDIDATE)
        m_Client[i].Drop      = FALSE;
#endif
        m_Client[i].PlayerIndex[0] = -1;
        m_Client[i].PlayerIndex[1] = -1;
        m_Client[i].Name[ 0] = '\0';
        m_Client[i].Name[32] = '\0';
    }

    ClearBanList();

	g_ClientsDropped = 0;
	g_ClientsConnected = 0;
}

//=========================================================================

game_server::~game_server( void )
{
}

//=========================================================================

void game_server::InitNetworking( net_address& Addr, const xwchar* pTitle )
{
    ASSERT((u32)x_wstrlen(pTitle) < sizeof(m_Title));
    m_Address = &Addr;
    x_wstrcpy( m_Title, pTitle );
}

//=========================================================================

net_address game_server::GetAddress( void )
{
    return *m_Address;
}

//=========================================================================

void game_server::SendHeartbeat( void )
{
    // Loop through all clients
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
        m_Client[i].ConnManager.SendHeartbeat();
}

//=========================================================================

void game_server::HandleLoginPacket( bitstream& BitStream, net_address& SenderAddr )
{
    s32 i,j;

    cs_login_req Login;
    cs_ReadLoginReq( Login, BitStream );

    x_DebugMsg("Client login request for %1d players\n",Login.NPlayers);

    // Tell client if wrong version
    if( Login.ServerVersion != SERVER_VERSION )
    {
        cs_login_resp Resp;
        Resp.RequestResp = -1;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
        x_DebugMsg("Server(%1d) refused Client(%1d) due to version\n",SERVER_VERSION,Login.ServerVersion);
        return;
    }

    // Validate password if server has one.
    if( fegl.ServerSettings.AdminPassword[0] &&
        x_wstrcmp( fegl.ServerSettings.AdminPassword, Login.Password ) )
    {
        cs_login_resp Resp;
        Resp.RequestResp = -2;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
        x_DebugMsg("Server(%1d) refused Client(%1d) due to password\n",SERVER_VERSION,Login.ServerVersion);
        return;
    }

    // Check to see if client machine has been banned.
    if( IsBanned( Login.SystemId ) )
    {
        cs_login_resp Resp;
        Resp.RequestResp = -3;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
        x_DebugMsg( "Server refused banned client(%d).\n", Login.SystemId );
        return;
    }

	if ( m_nClients >= fegl.ServerSettings.MaxClients )
	{
        cs_login_resp Resp;
        Resp.RequestResp = -5;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
        x_DebugMsg( "Client dropped because too many clients(%d).\n", Login.SystemId );
        return;
	}

    // Check if client is already in list
    s32 EmptyI = -1;
    for( i=0; i<MAX_CLIENTS; i++ )
    {
        if( SenderAddr == m_Client[i].Address )
            break;

        if( m_Client[i].Address.IsEmpty() ) 
            EmptyI = i;
    }

	if ( GetAveragePing() > 600.0f )
	{
        cs_login_resp Resp;
        Resp.RequestResp = -4;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
		g_ClientsConnected++;
		g_ClientsDropped++;
        x_DebugMsg( "Server dropped because it was too busy client(%d).\n", Login.SystemId );
        return;
	}

    if( (g_SM.CurrentState == STATE_SERVER_INGAME) &&
        (GameMgr.GetTimeRemaining() < 15.0f) )
    {
        cs_login_resp Resp;
        Resp.RequestResp = -4;
        Resp.NPlayers = 0;
        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
        x_DebugMsg( "Server dropped because it was near end of mission, client(%d).\n", Login.SystemId );
        return;
    }

    // It wasn't in the list
    if( i==MAX_CLIENTS )
    {
        // Check if we have room to add one
        if( GameMgr.RoomForPlayers( Login.NPlayers ) )
        {
            m_Client[EmptyI].NPlayers = Login.NPlayers;
            m_Client[EmptyI].SystemId = Login.SystemId;
            m_Client[EmptyI].Name[ 0] = '\0';
            m_Client[EmptyI].Name[32] = '\0';

            m_nClients++;

            for( j=0; j<Login.NPlayers; j++ )
            {
                // Store name
                x_wstrcpy( &m_Client[EmptyI].Name[j*32], Login.Data[j].Name );

                // Create a player object and connect.
                {
                    s32 PlayerIndex;
                    GameMgr.Connect( PlayerIndex, 
                                     EmptyI+1,
                                     Login.Data[j].Name,
                                     Login.TVRefreshRate,
                                     (player_object::character_type)Login.Data[j].CharacterType, // Character type
                                     (player_object::skin_type)Login.Data[j].SkinType,           // Skin type
                                     (player_object::voice_type)Login.Data[j].VoiceType );       // Voice type
                    ASSERT( PlayerIndex != -1 );
//                  GameMgr.EnterGame( PlayerIndex );
                    m_Client[EmptyI].PlayerIndex[j] = PlayerIndex;
                }
            }

            // Add to empty slot
            m_Client[EmptyI].Connected = TRUE;
            m_Client[EmptyI].Kick      = FALSE;
#if !defined(RELEASE_CANDIDATE)
            m_Client[EmptyI].Drop      = FALSE;
#endif
            m_Client[EmptyI].Address   = SenderAddr;
            m_Client[EmptyI].LastHeartbeat   = tgl.LogicTimeMs;
            m_Client[EmptyI].HeartbeatDelay  = 0;
            m_Client[EmptyI].PacketShipDelay = 0;

            // Initialize conn_manager
            m_Client[EmptyI].ConnManager.Init( *m_Address, 
                                               SenderAddr,
                                               &m_Client[EmptyI].UpdateManager,
                                               &m_Client[EmptyI].MoveManager,
                                               &m_Client[EmptyI].GameEventManager,EmptyI);

            m_Client[EmptyI].UpdateManager.SetClientControlled( m_Client[EmptyI].PlayerIndex[0],
                                                                m_Client[EmptyI].PlayerIndex[1]);

            x_DebugMsg("ConnManager inited %1d\n",EmptyI);

            // Decide on session ID
            s32 SessionID = x_irand(0,65536);
            m_Client[EmptyI].ConnManager.SetLoginSessionID(SessionID);

            // Send message back

            cs_login_resp Resp;
            Resp.RequestResp = 1;
            Resp.SessionID = SessionID;
            Resp.NPlayers = Login.NPlayers;
            x_wstrcpy( Resp.ServerName, m_Title );
            for( j=0; j<Resp.NPlayers; j++ )
            {
                x_wstrcpy( Resp.Data[j].Name, &m_Client[EmptyI].Name[j*32] );
                Resp.Data[j].PlayerIndex = m_Client[EmptyI].PlayerIndex[j];
            }
            
            bitstream BS;
            BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
            cs_WriteLoginResp( Resp, BS );
            BS.Encrypt(ENCRYPTION_KEY);
            BS.Send(*m_Address,SenderAddr);
            x_DebugMsg("CLIENT ADDED\n");
        }
        else
        {
            // Too many players
            cs_login_resp Resp;
            Resp.RequestResp = 0;
            Resp.NPlayers = 1;
            bitstream BS;
            BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
            cs_WriteLoginResp( Resp, BS );
            BS.Encrypt(ENCRYPTION_KEY);
            BS.Send(*m_Address,SenderAddr);
            x_DebugMsg("Server refused Client due to overcrowding\n");
        }
    }
    else
    // It was in the list
    {
        cs_login_resp Resp;
        Resp.RequestResp = 1;
        Resp.SessionID = m_Client[i].ConnManager.GetLoginSessionID();
        Resp.NPlayers = m_Client[i].NPlayers;
        x_wstrcpy( Resp.ServerName, m_Title );
        for( j=0; j<Resp.NPlayers; j++ )
        {
            x_wstrcpy( Resp.Data[j].Name, &m_Client[i].Name[j*32] );
            Resp.Data[j].PlayerIndex = m_Client[i].PlayerIndex[j];
        }

        bitstream BS;
        BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
        cs_WriteLoginResp( Resp, BS );
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send(*m_Address,SenderAddr);
    }
}

//=========================================================================

void game_server::ProcessPacket( bitstream& BitStream, net_address& SenderAddr )
{
    // Get what type of a packet it is
    s32 PacketType;
    BitStream.ReadS32(PacketType);
    //x_DebugMsg("Server received packet type %08X\n",PacketType);

    switch( PacketType )
    {

    case CONN_PACKET_TYPE_DISCONNECT:
        {
            s32 i;

            cs_disconnect Disconnect;
            cs_ReadDisconnect( Disconnect, BitStream );

            if( SHOW_SERVER_PACKETS )
                x_DebugMsg("**** Received Disconnect request or response\n");

            // Send disconnect order
            {
                x_DebugMsg("**** Sending Disconnect request\n");

                // Setup packet
                cs_disconnect Disconnect;
                bitstream BS;
                Disconnect.Reason = 0;
                BS.WriteU32( CONN_PACKET_TYPE_DISCONNECT );
                cs_WriteDisconnect( Disconnect, BS );

                // Send to clients
                BS.Encrypt(ENCRYPTION_KEY);
                BS.Send( *m_Address, SenderAddr );
            }            

            // Cull from list
            for( i=0; i<MAX_CLIENTS; i++ )
            if( m_Client[i].Connected )
            {
                if( m_Client[i].Address == SenderAddr )
                {
                    CullClient(i);
                }
            }
            if( i==MAX_CLIENTS )
                x_DebugMsg("Could not find client to cull\n");
        }
        break;

    case CONN_PACKET_TYPE_END_MISSION:
        break;

    case CONN_PACKET_TYPE_MISSION_REQ:
        if( SHOW_SERVER_PACKETS )
            x_DebugMsg("**** Mission Request\n");
        if( g_SM.MissionRunning || g_SM.MissionLoading )
        {
            cs_mission_resp Mission;
            
            x_strcpy( Mission.MissionName, tgl.MissionName );
            Mission.GameType = (s32)GameMgr.GetGameType();
            Mission.Round = tgl.Round;

            bitstream BS;
            BS.WriteS32( CONN_PACKET_TYPE_MISSION_REQ );
            cs_WriteMissionResp( Mission, BS );
            BS.Encrypt(ENCRYPTION_KEY);
            BS.Send(*m_Address,SenderAddr);
        }
        break;

    case CONN_PACKET_TYPE_LOGIN:
        if( SHOW_SERVER_PACKETS )
            x_DebugMsg("**** Received Login Request\n");
        //if( tgl.GameStage == GAME_STAGE_INGAME )
            HandleLoginPacket( BitStream, SenderAddr );
        break;

    case CONN_PACKET_TYPE_MANAGER_DATA:
        if( SHOW_SERVER_PACKETS )
            x_DebugMsg("**** Received Manager Data\n");
        if( !g_SM.MissionRunning )
            break;
        // FALL THRU

    case CONN_PACKET_TYPE_HB_REQUEST:
    case CONN_PACKET_TYPE_HB_RESPONSE:
        {
            // Rewind packet
            BitStream.SetCursor(0);

            // Find client it belongs to
            for( s32 i=0; i<MAX_CLIENTS; i++ )
            if( SenderAddr == m_Client[i].Address )
            {
                ASSERT( m_Client[i].Connected );
                m_Client[i].ConnManager.HandleIncomingPacket(BitStream);
                break;
            }
        }
        break;

    default:
        char Addr[32];
        SenderAddr.GetStrAddress(Addr);
        x_DebugMsg( "Unknown packet received. Sent by %1s\n", Addr );
        break;
    }
}

//=========================================================================

void game_server::CullClient( s32 ID )
{
    char IP[32];

    ASSERT( m_Client[ID].Connected );

    m_Client[ID].Address.GetStrIP(IP);
    m_Client[ID].Connected = FALSE;
    m_Client[ID].Kick      = FALSE;
#if !defined(RELEASE_CANDIDATE)
    m_Client[ID].Drop      = FALSE;
#endif
    m_Client[ID].ConnManager.Kill();
    m_Client[ID].Address.Clear();
    m_Client[ID].Name[0] = 0;

    for( s32 j=0; j<m_Client[ID].NPlayers; j++ )
    {
        GameMgr.ExitGame  ( m_Client[ID].PlayerIndex[j] );
        GameMgr.Disconnect( m_Client[ID].PlayerIndex[j] );            
        m_Client[ID].PlayerIndex[j] = -1;
        m_Client[ID].PlayerObjID[j] = obj_mgr::NullID;
    }

    m_nClients--;

    x_DebugMsg("CLIENT %s DROPPED\n",IP);
}

//=========================================================================
f32 g_ShipDelayPingLimit = 500.0f;
f32 g_ShipDelayModifier = 2.5f;
xtimer s_KickTimer;
f32		s_KickTimeout=10.0f;

void game_server::ProcessTime( f32 DeltaSec )
{
    (void)DeltaSec;
    s32 i, j;

    //
    // Do heartbeats, and kicks too
    //
    for( i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        m_Client[i].HeartbeatDelay += DeltaSec;
        if( m_Client[i].HeartbeatDelay >= tgl.HeartbeatDelay )
        {
            #ifdef SHOW_PACKET_SENDS
            x_DebugMsg("Sending Heartbeat\n");
            #endif
            
            if( m_Client[i].Kick )
            {
                // Setup packet
                cs_disconnect Disconnect;
                bitstream BS;
                Disconnect.Reason = 1;
                BS.WriteU32( CONN_PACKET_TYPE_DISCONNECT );
                cs_WriteDisconnect( Disconnect, BS );
                // Send to client
                BS.Encrypt(ENCRYPTION_KEY);
                BS.Send( *m_Address, m_Client[i].Address );
                BanClient( m_Client[i].SystemId );
				s_KickTimer.Stop();
				s_KickTimer.Reset();
            }
#if !defined(RELEASE_CANDIDATE)
            else
            if( m_Client[i].Drop )
            {
                // Setup packet
                cs_disconnect Disconnect;
                bitstream BS;
                Disconnect.Reason = 2;
                BS.WriteU32( CONN_PACKET_TYPE_DISCONNECT );
                cs_WriteDisconnect( Disconnect, BS );
                // Send to client
                BS.Encrypt(ENCRYPTION_KEY);
                BS.Send( *m_Address, m_Client[i].Address );
				s_KickTimer.Stop();
				s_KickTimer.Reset();
            }
#endif
            else
            {
                m_Client[i].ConnManager.SendHeartbeat();
            }

            m_Client[i].HeartbeatDelay -= tgl.HeartbeatDelay;
        }
    }

	// If our average client ping time is greater than 1000ms, 
	// for 10 seconds then we look for the last client that 
	// joined and kick them.
	if (GetAveragePing() > 1000.0f)
	{
		if (s_KickTimer.IsRunning())
		{
			if (s_KickTimer.ReadSec() > s_KickTimeout)
			{
				for( i=0; i<MAX_CLIENTS; i++ )
				{
					if( m_Client[i].Connected )
					{
						// Setup packet
						cs_disconnect Disconnect;
						bitstream BS;
						Disconnect.Reason = 2;
						BS.WriteU32( CONN_PACKET_TYPE_DISCONNECT );
						cs_WriteDisconnect( Disconnect, BS );
						// Send to client
						BS.Encrypt(ENCRYPTION_KEY);
						BS.Send( *m_Address, m_Client[i].Address );
						CullClient(i);
						// 20 seconds until we drop another client
						s_KickTimeout = 15.0f;
						g_ClientsDropped++;
						break;
					}
					s_KickTimer.Stop();
					s_KickTimer.Reset();
				}
			}
		}
		else
		{
			s_KickTimer.Reset();
			s_KickTimer.Start();
		}
	}
	else
	{
		s_KickTimer.Stop();
		s_KickTimer.Reset();
		s_KickTimeout = 10.0f;
	}

    //
    // Cull disconnected clients
    //
    for( i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        xbool Cull = FALSE;

        if( !m_Client[i].ConnManager.IsConnected() )
        {
			g_ClientsDropped ++;
            Cull = TRUE;
            x_DebugMsg("ConnManager closed connection\n");
        }

        if( !m_Client[i].UpdateManager.IsConnected() )
        {
            Cull = TRUE;
            x_DebugMsg("UpdateManager closed connection\n");
        }

        if( Cull )
        {
            //SendMessage(xfs("%s's connection has been dropped.\n",m_Client[i].Name));

            CullClient(i);
        }
    }

    if( g_SM.MissionRunning )
    {
        //
        // Tell update manager about client positions
        //
        for( j=0; j<MAX_CLIENTS; j++ )
        if( m_Client[j].Connected )
        {
            for( i=0; i<2; i++ )
            {
                player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_Client[j].PlayerObjID[i] );
                if( pPlayer )
                    m_Client[j].UpdateManager.SetClientPos( i, pPlayer->GetPosition() );
            }
        }

        //
        // Setup updates of objects
        //
        for( i = 0; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
        {
            object::id ObjID( i, -1 );
            object* pObj = ObjMgr.GetObjectFromID( ObjID );
        
            // Check if a mission loaded object has died
            if( (pObj==NULL) || (pObj->GetObjectID().Seq != m_MissionLoadedSeq[i]) )
            {
                m_MissionLoaded[i] = FALSE;
            }

            if( pObj )
                ObjID.Seq = pObj->GetObjectID().Seq;
        
            for( j=0; j<MAX_CLIENTS; j++ )
            if( m_Client[j].Connected )
            {
				m_Client[j].UpdateManager.UpdateObject( ObjID, 
														m_MissionLoaded[i], 
														pObj, 
														(m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_SYNC) );
				/*
                // Decide if we should be watching this object or not
                if( m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_SYNC )
                {
                    s32 Type = (pObj) ? (pObj->GetType()):(object::TYPE_NULL);
                    switch( Type )
                    {
                        case object::TYPE_DISK:
                        case object::TYPE_PLASMA:
                        case object::TYPE_BULLET:
                        case object::TYPE_GRENADE:
                        case object::TYPE_MORTAR:
                        case object::TYPE_LASER:
                        case object::TYPE_BLASTER:
                        case object::TYPE_MISSILE:
                        case object::TYPE_GENERICSHOT:
                        case object::TYPE_SHRIKESHOT:
                        case object::TYPE_BOMBERSHOT:
                        case object::TYPE_BOMB:
                        case object::TYPE_CORPSE:
                        case object::TYPE_PARTICLE_EFFECT:
                        case object::TYPE_DEBRIS:
                        case object::TYPE_BUBBLE:
                                break;
                        default:
                            m_Client[j].UpdateManager.UpdateObject( ObjID, m_MissionLoaded[i], pObj );
                            break;
                    }
                }
                else
                if( m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_INGAME )
                {
                    m_Client[j].UpdateManager.UpdateObject( ObjID, m_MissionLoaded[i], pObj );
                }
*/
/*
                // Only track updates once client is in a receiving state
                if( ( m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_SYNC ) ||
                    ( m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_INGAME ) )
                {
                    m_Client[j].UpdateManager.UpdateObject( ObjID, m_MissionLoaded[i], pObj );
                }
*/
            }

            if( pObj )
                pObj->ClearDirtyBits();
        }

        //
        // Setup updates for game manager
        //
        {
            u32 DirtyBits;
            u32 PlayerBits;
            u32 ScoreBits;

            // DAT - Get dirty bits
            GameMgr.GetDirtyBits( DirtyBits, PlayerBits, ScoreBits );

            // DAT - Tell GM to clear dirty bits
            GameMgr.ClearDirtyBits();

            for( j=0; j<MAX_CLIENTS; j++ )
            if( m_Client[j].Connected )
            {
                m_Client[j].GameEventManager.UpdateGMDirtyBits( DirtyBits, PlayerBits, ScoreBits );
            }
        }

        //
        // Ship packets to clients
        //
        {
            for( i=0; i<MAX_CLIENTS; i++ )
            if( m_Client[i].Connected )
            {
                s32 Count=0;

                m_Client[i].PacketShipDelay += DeltaSec;

//              x_DebugMsg( "(%d) Delay:%5.3f  DeltaSec:%5.3f  Timer:%5.3f", 
//                          i, m_Client[i].PacketShipDelay, DeltaSec,
//                          Timer1[i].TripSec() );

                f32 ShipDelay = tgl.ServerPacketShipDelay;
                    
                // This is deliberately ignored
                if( m_Client[i].ConnManager.GetPing() > g_ShipDelayPingLimit )
                {
                    #ifdef SHOW_PACKET_SENDS
                    x_DebugMsg("PING TOO HIGH\n");
                    #endif
                    ShipDelay *= g_ShipDelayModifier;
                }

                while( m_Client[i].PacketShipDelay >= m_Client[i].ConnManager.GetShipInterval())
                {
                    //x_DebugMsg("Considering Client %1d\n",m_Client[i].ConnManager.GetGameStage());

                    if( ( m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_SYNC ) ||
                        ( m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_INGAME ) )
                    {
                        //x_DebugMsg("ClientGameStage %1d %1d\n",i,m_Client[i].ConnManager.GetGameStage());
                        #ifdef SHOW_PACKET_SENDS    
                        //x_DebugMsg("Sending outgoing packet\n");
                        #endif

                        bitstream BS;
                        m_Client[i].ConnManager.HandleOutgoingPacket( BS );
						//x_DebugMsg("Sending Client Packet, ShipDelay %f, Ping %f\n",ShipDelay,m_Client[i].ConnManager.GetPing());

                        // DEBUG
                        {
//                          x_DebugMsg( "  Trip:%5.3f", Timer2[i].TripSec() );
                        }
                    }                   

                    m_Client[i].PacketShipDelay -= m_Client[i].ConnManager.GetShipInterval();
                    Count++;
                }

//              x_DebugMsg( "\n" );

                if( Count > 1 )
                    x_DebugMsg("Multiple packets sent in one frame!!!\n");
            }
        }

        //
        // Ship in sync packets
        //
        for( i=0; i<MAX_CLIENTS; i++ )
        if( m_Client[i].Connected )
        {
            if( m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_SYNC )
            {
                s32 NL = m_Client[i].UpdateManager.GetNumLifeEntries();

                // Time to create the players?
                if( NL == 0 )
                {
                    const game_score& Score = GameMgr.GetScore();

                    for( j=0; j<m_Client[i].NPlayers; j++ )
                    {
                        if( !(Score.Player[ m_Client[i].PlayerIndex[j] ].IsInGame) )
                        {
                            s32 PlayerIndex = m_Client[i].PlayerIndex[j];
                            ASSERT( PlayerIndex != -1 );
                            GameMgr.EnterGame( PlayerIndex );
							g_ClientsConnected++;
                            x_DebugMsg("PLAYER %d ON CLIENT %d ENTER GAME\n", j, i );
                        }
                    }
                }

                /*
                if( (NL == 0) && (x_irand(0,100)<30) )
                {
                    #ifdef SHOW_PACKET_SENDS
                    x_DebugMsg("!!!!!! Sending INSYNC Message\n");
                    #endif
                    // Setup packet
                    cs_insync InSync;
                    bitstream BS;
                    InSync.Reason = 0;
                    BS.WriteU32( CONN_PACKET_TYPE_INSYNC );
                    cs_WriteInSync( InSync, BS );

                    // Send to clients
                    BS.Encrypt(ENCRYPTION_KEY);
                    BS.Send( *m_Address, m_Client[i].Address);
                }
                */
            }
        }
    }
}

//=========================================================================

void game_server::DisplayPings( s32 L )
{
    x_printfxy(0,L++,"SERVER'S CLIENTS");
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        x_printfxy(0,L++,"%1d] %1f",i,m_Client[i].ConnManager.GetPing());
    }
}

//=========================================================================

f32 game_server::GetAveragePing(void)
{
	f32 average = 0.0f;
	s32 count = 0;
	s32 i;

	for (i=0;i<MAX_CLIENTS;i++)
	{
		if ( (m_Client[i].Connected) && (m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_INGAME) )
		{
			count++;
			average += m_Client[i].ConnManager.GetPing();
		}
	}

	if (count)
	{
		return average / count;
	}
	return 0.0f;
}

//=========================================================================

void game_server::ShowLifeDelay( void )
{
    s32 L = 0;
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        x_printfxy( 0, L, "%2d] %5.1f", i, m_Client[i].ConnManager.GetPing() );
        m_Client[i].UpdateManager.ShowLifeDelay(L);
        L++;
    }
}

//=========================================================================

void game_server::SyncMissionLoad( void )
{
    s32 i;
    for( i = 0; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
    {
        m_MissionLoaded[i] = FALSE;
        m_MissionLoadedSeq[i] = -1;
    }

    for( i = 32; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
    {
        object::id ObjID( i, -1 );
        object* pObj = ObjMgr.GetObjectFromID(ObjID);
        if( pObj )
        {
            m_MissionLoaded[i] = TRUE;
            m_MissionLoadedSeq[i] = pObj->GetObjectID().Seq;
            m_MissionLoadedIndex = i;
        }
    }
}

//=========================================================================

void game_server::SendMsg( const msg& Msg )
{
    if( Msg.Target < tgl.NLocalPlayers )
    {
        if( (g_SM.CurrentState == STATE_SERVER_INGAME) || 
            (g_SM.CurrentState == STATE_CAMPAIGN_INGAME) )
        {
            MsgMgr.ProcessMsg( Msg );
        }
    }
    else
    {
        s32 i, j;

        // Send it to indicated player.
        for( j = 0; j < MAX_CLIENTS; j++ )
        if( (m_Client[j].Connected) &&
            (m_Client[j].ConnManager.GetDestState() == STATE_CLIENT_INGAME) )
        {
            for( i = 0; i < m_Client[j].NPlayers; i++ )
            if( m_Client[j].PlayerIndex[i] == Msg.Target )
            {
                m_Client[j].UpdateManager.SendMsg( Msg );
                return;
            }
        }
    }
}

//=========================================================================
/*
void game_server::SendMessage( const char* Str, xcolor Color, u32 TeamBits )
{
    // Build a message packet
    s32 j,i;
    byte Msg[512];
    bitstream BS;
    BS.Init(Msg,512);
    BS.WriteS32( CONN_PACKET_TYPE_MESSAGE );
    BS.WriteS32( 0, 8 );
    BS.WriteColor( Color );
    BS.WriteString( Str );

    // Loop through clients and send the message
    BS.Encrypt(ENCRYPTION_KEY);

    for( j=0; j<MAX_CLIENTS; j++ )
    if( m_Client[j].Connected )
    {
        for( i=0; i<m_Client[j].NPlayers; i++ )
        if(m_Client[j].PlayerIndex[i] != -1)
        {
            u32 TB = GameMgr.GetTeamBits( m_Client[j].PlayerIndex[i] );
            if( TB & TeamBits )
            {
                BS.Send(*m_Address,m_Client[j].Address);
                break;
            }
        }
    }

    for( j = 0; j < tgl.NLocalPlayers; j++ )
    {
        u32 TB = GameMgr.GetTeamBits( tgl.PC[j].PlayerIndex );
        if( TB & TeamBits )
        {
//            tgl.pHUDManager->AddChatLine( Color, Str );
            break;
        }
    }
}

//=========================================================================

void game_server::SendPopMessage( const char* Str, xcolor Color, s32 PlayerIndex )
{
    // Build a message packet
    s32  j,i;
    byte Msg[512];
    bitstream BS;
    BS.Init(Msg,512);
    BS.WriteS32( CONN_PACKET_TYPE_MESSAGE );
    BS.WriteS32( 1, 4 );
    BS.WriteColor( Color );
    BS.WriteString( Str );

    BS.Encrypt(ENCRYPTION_KEY);

    if( PlayerIndex == -1 )
    {
        // Send it to everybody.
        for( j = 0; j < MAX_CLIENTS; j++ )
        if( m_Client[j].Connected )
        {
            BS.Send( *m_Address, m_Client[j].Address);
        }

//        tgl.pHUDManager->DisplayMessage( Str, Color, 5.0f, 0.75f );
    }
    else
    {
        // Send it to indicated player.
        for( j = 0; j < MAX_CLIENTS; j++ )
        if( m_Client[j].Connected  )
        {
            for( i=0; i<m_Client[i].NPlayers; i++ )
            if( m_Client[j].PlayerIndex[i] == PlayerIndex )
            {
                BS.Send( *m_Address, m_Client[j].Address);
                break;
            }
        }
        
        for( j = 0; j < tgl.NLocalPlayers; j++ )
        {
            if( tgl.PC[j].PlayerIndex == PlayerIndex )
            {
//                tgl.pHUDManager->DisplayMessage( Str, Color, 5.0f, 0.75f );
            }
        }
    }
}

//=========================================================================
*/
void game_server::SendAudio( s32 AudioID, s32 PlayerIndex )
{
    // Build a message packet
    s32 j,i;
    byte Msg[512];
    bitstream BS;
    BS.Init( Msg, 512 );
    BS.WriteS32( CONN_PACKET_TYPE_MESSAGE );
    BS.WriteS32( 2, 4 );
    BS.WriteS32( AudioID );
    BS.Encrypt(ENCRYPTION_KEY);

    // Loop through clients and send the message to the correct player
    for( j=0; j<MAX_CLIENTS; j++ )
    if( m_Client[j].Connected )
    {
        for( i=0; i<m_Client[j].NPlayers; i++ )
        if( m_Client[j].PlayerIndex[i] == PlayerIndex )
        {
            BS.Send( *m_Address, m_Client[j].Address);
            break;
        }   
    }

    // Play sound locally if needed.
    for( j = 0; j < tgl.NLocalPlayers; j++ )
    {
        if( tgl.PC[j].PlayerIndex == PlayerIndex )
        {
            audio_Play( AudioID );
            break;
        }
    }
}

//=========================================================================

void game_server::SendAudio( s32 AudioID, u32 TeamBits )
{
    // Build a message packet
    s32 j,i;
    byte Msg[512];
    bitstream BS;
    BS.Init(Msg,512);
    BS.WriteS32( CONN_PACKET_TYPE_MESSAGE );
    BS.WriteS32( 2, 4 );
    BS.WriteS32( AudioID );

    // Loop through clients and send the message
    BS.Encrypt(ENCRYPTION_KEY);
    for( j=0; j<MAX_CLIENTS; j++ )
    if( m_Client[j].Connected )
    {
        for( i=0; i<m_Client[j].NPlayers; i++ )
        if(m_Client[j].PlayerIndex[i] != -1)
        {
            u32 TB = GameMgr.GetTeamBits( m_Client[j].PlayerIndex[i] );
            if( TB & TeamBits )
            {
                BS.Send(*m_Address,m_Client[j].Address);
                break;
            }
        }
    }

    for( j = 0; j < tgl.NLocalPlayers; j++ )
    {
        u32 TB = GameMgr.GetTeamBits( tgl.PC[j].PlayerIndex );
        if( TB & TeamBits )
        {
            s32 Handle;
            Handle = audio_Play( AudioID );
            break;
        }
    }
}

//=========================================================================

void game_server::DumpStats( X_FILE* fp )
{
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        if( fp )
        {
            x_fprintf(fp,"------- %1d ---------\n",i);
        }
        else
        {
            x_DebugMsg("------- %1d ---------\n",i);
        }
        m_Client[i].ConnManager.DumpStats(fp);
    }
}

//=========================================================================

void game_server::AdvanceStats( void )
{
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
        m_Client[i].ConnManager.AdvanceStats();
}

//=========================================================================

void game_server::BroadcastEndMission( void )
{
    // Setup packet
    cs_end_mission EndMission;

    EndMission.Round = tgl.Round;

    // Send to clients
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        EndMission.SessionID = m_Client[i].ConnManager.GetLoginSessionID();

        bitstream BS;
        BS.WriteU32( CONN_PACKET_TYPE_END_MISSION );
        cs_WriteEndMission( EndMission, BS );

        GameMgr.ProvideFinalScore( BS );

        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send( *m_Address, m_Client[i].Address );

        #ifdef DMT_SHOW_STUFF
        x_DebugMsg( "---> SEND 'END OF MISSION' TO CLIENT %d\n", i );
        #endif
    }
}

//=========================================================================

void game_server::UnloadMission( void )
{                              
    for( s32 i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        m_Client[i].UpdateManager.Reset();
        m_Client[i].MoveManager.Reset();
        m_Client[i].ConnManager.ClearPacketAcks();
    }

    RelaxBanList();
}

//=========================================================================

xbool game_server::CoolDown( void )
{
    s32 i;

    if( (tgl.NRenders%30)==0 )
        BroadcastEndMission();

    // Check if all clients are in DecideMission
    for( i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        if( (m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_SYNC) ||
            (m_Client[i].ConnManager.GetDestState() == STATE_CLIENT_LOAD_MISSION) )
        {
            // This client is going to screw everything up.  Dump him.
            CullClient( i );
        }

        if( m_Client[i].ConnManager.GetDestState() != STATE_CLIENT_REQUEST_MISSION ) 
            break;
    }

    if( i==MAX_CLIENTS )
    {

        #ifdef STEP_SESSION_ID
        // Increment all the session ids
        for( i=0; i<MAX_CLIENTS; i++ )
        if( m_Client[i].Connected )
        {
            s32 CurrentID = m_Client[i].ConnManager.GetLoginSessionID();
            m_Client[i].ConnManager.SetLoginSessionID( CurrentID+1 );
            x_DebugMsg("Stepping Session ID for client(%d) %d -> %d\n",i,CurrentID,CurrentID+1);
        }
        #endif

        return( TRUE );
    }
    else
    {
        return( FALSE );
    }    
}

//=========================================================================

xbool game_server::DisconnectEveryone( void )
{
    s32 i;
    xbool ClientExists=FALSE;

    // Check if any clients are still connected
    for( i=0; i<MAX_CLIENTS; i++ )
    if( m_Client[i].Connected )
    {
        ClientExists = TRUE;

        // Setup packet
        cs_disconnect Disconnect;
        bitstream BS;
        Disconnect.Reason = 0;
        BS.WriteU32( CONN_PACKET_TYPE_DISCONNECT );
        cs_WriteDisconnect( Disconnect, BS );

        // Send to clients
        BS.Encrypt(ENCRYPTION_KEY);
        BS.Send( *m_Address, m_Client[i].Address);
    }

    return (ClientExists == FALSE);
}

//=========================================================================

void game_server::KickPlayer( s32 PlayerIndex )
{
    s32 i;

    for( i = 0; i < MAX_CLIENTS; i++ )
    {
        if( (m_Client[i].Connected) &&
            ((m_Client[i].PlayerIndex[0] == PlayerIndex) ||
             (m_Client[i].PlayerIndex[1] == PlayerIndex)) )
             m_Client[i].Kick = TRUE;
    }
}

//=========================================================================
#if !defined(RELEASE_CANDIDATE)
void game_server::DropPlayer( s32 PlayerIndex )
{
    s32 i;

    for( i = 0; i < MAX_CLIENTS; i++ )
    {
        if( (m_Client[i].Connected) &&
            ((m_Client[i].PlayerIndex[0] == PlayerIndex) ||
             (m_Client[i].PlayerIndex[1] == PlayerIndex)) )
             m_Client[i].Drop = TRUE;
    }
}
#endif

//=========================================================================

void game_server::BanClient( s32 ClientSystemId )
{
    s32 i;

    // Find either the entry for this Id or a free entry.
    for( i = 0; i < 16; i++ )
    {
        if( (m_BanList[i] == ClientSystemId) || (m_BanWait[i] == 0) )
        {
            m_BanList[i] = ClientSystemId;
            m_BanWait[i] = 2;
            return;
        }
    }
}

//=========================================================================

xbool game_server::IsBanned( s32 ClientSystemId )
{
    s32 i;

    for( i = 0; i < 16; i++ )
    {
        if( (m_BanList[i] == ClientSystemId) && (m_BanWait[i] > 0) )
            return( TRUE );
    }

    return( FALSE );
}

//=========================================================================

void game_server::ClearBanList( void )
{   
    s32 i;

    for( i = 0; i < 16; i++ )
    {
        m_BanList[i] = 0;
        m_BanWait[i] = 0;
    }
}

//=========================================================================

void game_server::RelaxBanList( void )
{
    s32 i;

    for( i = 0; i < 16; i++ )
    {
        if( m_BanWait[i] > 0 )
            m_BanWait[i]--;
    }
}

//=========================================================================

s32 game_server::GetBanCount( void )
{
    s32 i;
    s32 Banned = 0;

    for( i = 0; i < 16; i++ )
    {
        if( m_BanWait[i] > 0 )
            Banned++;
    }

    return( Banned );
}

//=========================================================================

s32 game_server::GetClientCount( void )
{
	return m_nClients;
}