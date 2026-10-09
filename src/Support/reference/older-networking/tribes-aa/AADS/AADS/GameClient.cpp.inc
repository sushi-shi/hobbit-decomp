//=========================================================================
//
//  GameClient.cpp
//
//=========================================================================

//#define SHOW_DMT_STUFF

//=========================================================================

#include "entropy.hpp"
#include "globals.hpp"
#include "fe_globals.hpp"
#include "netlib/bitstream.hpp"
#include "gameserver.hpp"
#include "gameclient.hpp"
#include "gameuser.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Player\DefaultLoadouts.hpp"
#include "LabelSets\Tribes2Types.hpp"
#include "ServerVersion.hpp"
#include "audiomgr\audio.hpp"
#include "hud\hud_manager.hpp"
#include "GameMgr\GameMgr.hpp"
#include "sm_common.hpp"
#include "ClientServer.hpp"

xbool SHOW_CLIENT_PACKETS = FALSE;

extern xwchar g_JoinPassword[];

//=========================================================================

game_client::game_client( void )
{
    m_LastServerNote = tgl.LogicTimeMs;
    m_LoginRefused = FALSE;
    m_LoggedIn = FALSE;
    m_LastConnectCheck = 0;
    m_HeartbeatDelay = 0;
    m_PacketShipDelay= 0;
    m_PlayerObjID[0] = obj_mgr::NullID;
    m_PlayerObjID[1] = obj_mgr::NullID;

    m_NPlayers = tgl.NRequestedPlayers;
    m_InSync = FALSE;
}

//=========================================================================

game_client::~game_client( void )
{
}

//=========================================================================

void game_client::InitNetworking( net_address& Addr, 
                                  net_address& ServerAddr,
                                  const xwchar* pTitle,
                                  const xwchar* pName,
								  const s32 ClientIndex)
{
    m_Address = &Addr;
    m_ServerAddress = &ServerAddr;
	m_ClientIndex = ClientIndex;
    ASSERT((u32)x_wstrlen(pTitle) < sizeof(m_Title));
    ASSERT((u32)x_wstrlen(pName) < sizeof(m_Name));

    x_wstrcpy(m_Title,pTitle);
    x_wstrcpy(m_Name,pName);
    m_ConnManager.Init(Addr,ServerAddr,&m_UpdateManager,&m_MoveManager,&m_GameEventManager,ClientIndex);
}

//=========================================================================

net_address game_client::GetAddress( void )
{
    return *m_Address;
}

//=========================================================================

xbool game_client::Disconnect( void )
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
    BS.Send( *m_Address, *m_ServerAddress);

    return (!m_LoggedIn);
}

//=========================================================================

void game_client::ProcessPacket( bitstream& BitStream, net_address& SenderAddr )
{
    /*
    if( !(SenderAddr == *m_ServerAddress) )
    {
        #ifdef SHOW_DMT_STUFF
        x_DebugMsg( "<<** RECV packet from which did not originate from server\n" );
        #endif
        return;
    }

    // Get what type of a packet is it
    s32 PacketType;
    BitStream.ReadS32( PacketType );
    //x_DebugMsg("------ Client received packet type %08X, Size %1d, Ping %f\n",PacketType,BitStream.GetNBytes(),m_ConnManager.GetPing());

    switch( PacketType )
    {

    case CONN_PACKET_TYPE_INSYNC:
        {
            #ifdef SHOW_DMT_STUFF
            x_DebugMsg( "<--- RECV INSYNC\n" );
            #endif
            m_InSync = TRUE;
        }
        ASSERT( FALSE );
        break;

    case CONN_PACKET_TYPE_DISCONNECT:
        {
            #ifdef SHOW_DMT_STUFF
            x_DebugMsg( "<--- RECV DISCONNECT REQ\n");
            #endif

            cs_disconnect DisconnectReason;
            cs_ReadDisconnect( DisconnectReason, BitStream );

            // Was the client kicked
            if( DisconnectReason.Reason == 0 )
			{
                g_SM.ServerShutdown = TRUE;
            }    
            if( DisconnectReason.Reason == 1 )
			{
                g_SM.ClientKicked = TRUE;
			}
			else if ( DisconnectReason.Reason == 2 )
			{
				g_SM.ServerTooBusy = TRUE;
			}

            Disconnect();
            m_LoggedIn = FALSE;
        }
        break;

    case CONN_PACKET_TYPE_END_MISSION:
        #ifdef SHOW_DMT_STUFF
        x_DebugMsg("<--- RECV END OF MISSION\n");
        #endif

        if( g_SM.MissionRunning )
        {
            cs_end_mission EndMission;
            cs_ReadEndMission( EndMission, BitStream );

            s32 CurrentSessionID = m_ConnManager.GetLoginSessionID();
            x_DebugMsg("END MISSION MSG: %d %d\n",EndMission.SessionID,CurrentSessionID);

            if( (EndMission.SessionID&0xFF) == (CurrentSessionID&0xFF) )
            {
                ASSERT( EndMission.Round == tgl.Round );
                GameMgr.EndGame();
                GameMgr.AcceptFinalScore( BitStream );
                g_SM.ServerMissionEnded = TRUE;

                #ifdef STEP_SESSION_ID
                s32 CurrentID = m_ConnManager.GetLoginSessionID();
                m_ConnManager.SetLoginSessionID( CurrentID+1 );
                x_DebugMsg("Stepping SessionID %d -> %d\n",CurrentID,CurrentID+1);
                #endif
            }
        }
        break;

    case CONN_PACKET_TYPE_MISSION_REQ:
        #ifdef SHOW_DMT_STUFF
        x_DebugMsg("<--- RECV END OF MISSION REQUEST\n" );
        #endif
        cs_mission_resp Mission;
        cs_ReadMissionResp( Mission, BitStream );
        x_strcpy( fegl.MissionName, Mission.MissionName );
        fegl.GameType = Mission.GameType;
        tgl.Round = Mission.Round;
        break;

    case CONN_PACKET_TYPE_LOGIN:
        #ifdef SHOW_DMT_STUFF
        x_DebugMsg("<--- RECV LOGIN RESPONSE\n" );
        #endif
        if( !m_LoggedIn )
        {
            cs_login_resp Login;
            cs_ReadLoginResp( Login, BitStream );

			if( Login.RequestResp == -5 )
			{
				m_LoginRefused = TRUE;
				g_SM.TooManyClients = TRUE;
				x_DebugMsg("   TOO MANY CLIENTS ON SERVER!\n");
				return;
			}
			else
			if( Login.RequestResp == -4 )
			{
				m_LoginRefused = TRUE;
                g_SM.ServerTooBusy = TRUE;
                x_DebugMsg( "     SERVER WAS TOO BUSY!\n" );
                return;
			}
			else
            if( Login.RequestResp == -3 )
            {
                m_LoginRefused = TRUE;
                g_SM.ClientBanned = TRUE;
                x_DebugMsg( "     CLIENT IS CURRENTLY BANNED!\n" );
                return;
            }
            else
            if( Login.RequestResp == -2 )
            {
                m_LoginRefused = TRUE;
                g_SM.InvalidPassword = TRUE;
                x_DebugMsg( "     Invalid password!\n" );
                return;
            }
            else
            if( Login.RequestResp == -1 )
            {
                m_LoginRefused = TRUE;
                x_DebugMsg( "     Server and Client are different versions!\n" );
                return;
            }
            else
            if( Login.RequestResp == 0 )
            {
                m_LoginRefused = TRUE;
                g_SM.ServerFull = TRUE;
                x_DebugMsg( "     Server is full\n" );
                return;
            }

            // Tell connection manager session ID
            m_ConnManager.SetLoginSessionID( Login.SessionID );
            //x_printf("Client connected to %1s\n",Login.ServerName );

            x_wstrcpy( m_ServerTitle, Login.ServerName );
            m_LoggedIn = TRUE;
            for( s32 i=0; i<Login.NPlayers; i++ )
            {
                m_PlayerObjID[i].Slot = Login.Data[i].PlayerIndex;
                m_PlayerObjID[i].Seq  = -1;
                x_wstrcpy(m_Name,Login.Data[i].Name);
            }
        }
        break;

    case CONN_PACKET_TYPE_MESSAGE:
        if( SHOW_CLIENT_PACKETS )
            x_DebugMsg("**** Received String/Audio Message\n");
        {
            s32 Type;
            BitStream.ReadS32( Type, 4 );

            if( Type == 0 )
            {
                xcolor Color;
                char Msg[256];
                BitStream.ReadColor(Color);
                BitStream.ReadString(Msg);
                
//                tgl.pHUDManager->AddChatLine( Color, Msg );
            }
            else
            if( Type == 1 )
            {
                xcolor Color;
                char Msg[256];
                BitStream.ReadColor(Color);
                BitStream.ReadString(Msg);
                
//                tgl.pHUDManager->DisplayMessage( Msg, Color, 5.0f, 0.75f );
            }
            else
            if( Type == 2 )
            {
                s32 AudioID;
                BitStream.ReadS32( AudioID );
                audio_Play( AudioID );
            }
        }
        break;

    case CONN_PACKET_TYPE_HB_REQUEST:
    case CONN_PACKET_TYPE_HB_RESPONSE:
        {
            // Rewind packet
            BitStream.SetCursor(0);
            m_ConnManager.HandleIncomingPacket(BitStream);
        }
        break;

    case CONN_PACKET_TYPE_MANAGER_DATA:
        {
            if( SHOW_CLIENT_PACKETS )
                x_DebugMsg("**** Received Manager Data\n");

            if( (g_SM.CurrentState == STATE_CLIENT_SYNC) ||
                (g_SM.CurrentState == STATE_CLIENT_INGAME) )
            {
                // Rewind packet
                BitStream.SetCursor(0);
                m_ConnManager.HandleIncomingPacket(BitStream);
            }
            else
            {
                x_DebugMsg("MANAGER_DATA in wrong game stage\n");
            }
        }
        break;

    default:
        x_DebugMsg("Unknown packet type was received\n");
        break;
    }
    */
}

//=========================================================================

void game_client::ProcessTime( f32 DeltaSec )
{
    /*
    m_LastConnectCheck -= DeltaSec;

    //x_printf("%1d %1d (%1d,%1d) (%1d,%1d)\n",tgl.NLocalPlayers,m_NPlayers,m_PlayerObjID[0].Slot,m_PlayerObjID[0].Seq,m_PlayerObjID[1].Slot,m_PlayerObjID[1].Seq);

    // Check connection
    if( m_LastConnectCheck <=0 )
    {
        m_LastConnectCheck = 3.0f;

        if( m_LoggedIn )
        {
            //x_DebugMsg("***************** Checking \n");
            // Check if player has been created on this side
            for( s32 i=0; i<2; i++ )
            {
                if( m_PlayerObjID[i].Seq == -1 )
                {
                    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_PlayerObjID[i] );
                    if( pPlayer )
                    {
                        x_DebugMsg("***************** Player arrived \n");

                        pPlayer->ClientInitialize( TRUE,
                                                   i,
											       &m_MoveManager );

                        m_PlayerObjID[i].Seq = pPlayer->GetObjectID().Seq;

                        tgl.PC[tgl.NLocalPlayers++].PlayerIndex = pPlayer->GetObjectID().Slot;
                    }
                }
            }

            if( tgl.NLocalPlayers == m_NPlayers )
                m_InSync = TRUE;

            // Check on heartbeat
            if( !m_ConnManager.IsConnected() )
            {
                x_DebugMsg("Login lost\n");
                m_LoggedIn = FALSE;
                m_LoginRefused = TRUE;  // TEMPORARY - Do NOT try to reconnect for now.
            }

        }
        else
        {
            if( !m_LoginRefused && (g_SM.CurrentState == STATE_CLIENT_INIT_CLIENT) )
            {
                cs_login_req Login;

                Login.NPlayers      = m_NPlayers;
                Login.ServerVersion = SERVER_VERSION;   
                Login.SystemId      = net_GetSystemId();
                x_wstrcpy( Login.Password, g_JoinPassword );

                for( s32 i=0; i<Login.NPlayers; i++ )
                {
                    x_wstrcpy( Login.Data[i].Name, fegl.WarriorSetups[i].Name );
                    Login.Data[i].CharacterType         = fegl.WarriorSetups[i].CharacterType;
                    Login.Data[i].SkinType              = fegl.WarriorSetups[i].SkinType;
                    Login.Data[i].VoiceType             = fegl.WarriorSetups[i].VoiceType;
                }

                Login.TVRefreshRate = eng_GetTVRefreshRate() ;

                // Try to log in
                bitstream BS;
                BS.WriteS32( CONN_PACKET_TYPE_LOGIN );
                cs_WriteLoginReq( Login, BS );
                BS.Encrypt(ENCRYPTION_KEY);
                BS.Send(*m_Address,*m_ServerAddress);

                x_DebugMsg("Sending login request\n");
            }
        }
    }

    //
    // Ship packets to server
    //
    if( (!m_LoginRefused) && (g_SM.MissionRunning || (g_SM.CurrentState == STATE_CLIENT_SYNC)) )
    {
        m_PacketShipDelay += DeltaSec;
        while( m_PacketShipDelay >= tgl.ClientPacketShipDelay )
        {
            //x_DebugMsg("%5d CLIENT SENDING PACKET\n",(s32)tgl.LogicTimeMs);
            m_PacketShipDelay -= tgl.ClientPacketShipDelay;
            bitstream BS;
            m_ConnManager.HandleOutgoingPacket( BS );
        }
    }
    */
}

//=========================================================================

void game_client::ProcessHeartbeat( f32 DeltaSec )
{
    //
    // Do heartbeat
    //

    if( !m_LoginRefused )
    {
        m_HeartbeatDelay -= DeltaSec;
        if( m_HeartbeatDelay <= 0 )
        {
            //if( tgl.GameStage == GAME_STAGE_INGAME )
                m_HeartbeatDelay = tgl.HeartbeatDelay;
            //else
            //    m_HeartbeatDelay = 0.125f;

            m_ConnManager.SendHeartbeat();
        }
    }
}

//=========================================================================

f32 game_client::GetPing( void )
{
    return m_ConnManager.GetPing();
}

//=========================================================================

void game_client::RequestMission( void )
{
    bitstream BS;
    BS.WriteS32( CONN_PACKET_TYPE_MISSION_REQ );
    BS.Encrypt(ENCRYPTION_KEY);
    BS.Send(*m_Address,*m_ServerAddress);
}

//=========================================================================

void game_client::UnloadMission( void )
{
    s32 i;

    m_MoveManager.Reset();
    m_UpdateManager.Reset();

    for( i=0; i<2; i++ )
        m_PlayerObjID[i].Seq = -1;
}

//=========================================================================

void game_client::EndMission( void )
{
    m_PlayerObjID[0].Seq = -1;
    m_PlayerObjID[1].Seq = -1;
    tgl.PC[0].PlayerIndex = -1;
    tgl.PC[1].PlayerIndex = -1;
    tgl.NLocalPlayers = 0;
}

//=========================================================================



