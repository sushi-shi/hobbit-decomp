//=========================================================================
//
//  ConnManager.hpp
//
//=========================================================================

#ifndef CONNMANAGER_HPP
#define CONNMANAGER_HPP

#include "x_files.hpp"
#include "netlib/netlib.hpp"
#include "netlib/bitstream.hpp"
#include "sm_common.hpp"

//=========================================================================
// MANAGERS
//=========================================================================

#include "updatemanager.hpp"
#include "movemanager.hpp"
#include "gameeventmanager.hpp"

//=========================================================================

#define CONN_PACKET_TYPE_LOGIN           (0xABCD0001)
#define CONN_PACKET_TYPE_HB_REQUEST      (0xABCD0002)
#define CONN_PACKET_TYPE_HB_RESPONSE     (0xABCD0003)
#define CONN_PACKET_TYPE_MANAGER_DATA    (0xABCD0004)
#define CONN_PACKET_TYPE_MESSAGE         (0xABCD0005)
#define CONN_PACKET_TYPE_MISSION_REQ     (0xABCD0006)
#define CONN_PACKET_TYPE_END_MISSION     (0xABCD0007)
#define CONN_PACKET_TYPE_GAME_STAGE      (0xABCD0008)
#define CONN_PACKET_TYPE_DISCONNECT      (0xABCD0009)
#define CONN_PACKET_TYPE_INSYNC          (0xABCD000A)
#define CONN_PACKET_TYPE_CS              (0xABCD000B)

#define CONN_NUM_HEARTBEATS     8
#define CONN_NUM_PACKETS        128
#define CONN_NUM_STATS          32
#define CONN_LIVES_PER_PACKET   8
#define CONN_MOVES_PER_PACKET   16

#define PACKET_LOSS_HISTORY 16

// Default ship interval of 100ms
#define DEFAULT_SHIP_INTERVAL (1.0f / 10.0f)
// Maximum ship interval of 200ms
#define MAX_SHIP_INTERVAL	  (1.0f /  4.0f)

#define SHIP_INTERVAL_INCREMENT	(1.0f / 100.0f)
// Threshold at which ship intervals will be modified
#define LOW_PING_THRESHOLD		(290.0f)
#define HIGH_PING_THRESHOLD		(310.0f)
#define LOGIN_PING_CUTOFF		(600.0f)

//==============================================================================

#define ACKSLOTS    8
#define ACKBITS     (ACKSLOTS * 16)

//------------------------------------------------------------------------------

class ack_bits
{
private:
    u16     Bits[ ACKSLOTS ];

public:
            ack_bits    ( void );
    void    Clear       ( void );
    void    Print       ( void );
    void    ShiftLeft   ( s32 Shift );
    xbool   Bit         ( s32 Position );
    void    Set         ( s32 Position );
    void    Read        ( bitstream& BS );
    void    Write       ( bitstream& BS );
};

//=========================================================================

struct conn_packet
{
    // Packet info
    s32 Seq;

    // Update manager life info
    s16 LifeID[CONN_LIVES_PER_PACKET];
    s16 NLife;
    u16 LifeGroupSeq;

    // Update manager update info
    s16 FirstUpdate;
    s16 NUpdates;

    // Move manager list
    s16 MoveID[CONN_MOVES_PER_PACKET];
    s16 MoveSeq[CONN_MOVES_PER_PACKET];
    s16 NMoves;

    // Game manager dirty bits
    u32 GMDirtyBits;
    u32 GMPlayerBits;
    u32 GMScoreBits;

    // Marked as skip-ack
    xbool DoNotAck;
};

struct conn_stats
{
    s32 LifeSent;
    s32 UpdatesSent;
    s32 MovesSent;
    s32 BytesSent;
    s32 PacketsSent;
};

class conn_manager
{

private:
    net_address m_SrcAddress;
    net_address m_DstAddress;
	s32			m_ClientIndex;
    s64         m_LastConfirmAliveTime;

    s32         m_LoginSessionID;

    s32         m_LastSeqReceived;
    s32         m_LastSeqSent;
    s32         m_LastAckReceived;
    s32         m_LastAckSent;
    ack_bits    m_AckBits;

    s32         m_NPacketsReceived;
    s32         m_NPacketsUnusable;
    s32         m_NPacketsSent;
    s32         m_NPacketsDropped;

	s32			m_BytesReceived;
	s32			m_BytesSent;
	xtimer		m_ConnectTime;
    xbool       m_IsConnected;

    s32         m_HeartbeatsSent;
    s64         m_HeartbeatSentTime[CONN_NUM_HEARTBEATS];
    f32         m_HeartbeatPing[CONN_NUM_HEARTBEATS];
    s32         m_LastHeartbeatRequest;

    f32         m_AveragePing;
    state_enum  m_DestState;

    conn_packet m_Packet[CONN_NUM_PACKETS];
	s16			m_PacketAck[PACKET_LOSS_HISTORY];
	s16			m_PacketNak[PACKET_LOSS_HISTORY];
	xtimer		m_PacketLossTimer;
	s32			m_PacketLossIndex;
	f32			m_PacketShipInterval;
	f32			m_LastPing;

    update_manager* m_pUM;
    move_manager* m_pMM;
    game_event_manager* m_pGEM;

    s32         m_StatsIndex;
    conn_stats  m_Stats[CONN_NUM_STATS];

    xbool       m_DoLogging;
    X_FILE*     m_LogOutgoing;
    X_FILE*     m_LogIncoming;


    void    HandlePacketNotify  ( s32 PacketID, xbool Received );
    void    WriteManagerData    ( conn_packet& Packet, bitstream& BitStream );
    void    ReadManagerData     ( bitstream& BitStream );
    void    ReceiveHeartbeat    ( bitstream& BitStream );
    // This is really a debug only function but my guess is that it'll eventually
    // appear in the final game

public:
            conn_manager        ( void );
            ~conn_manager       ( void );

    void    Init                ( net_address& Src,
                                  net_address& Dst,
                                  update_manager* pUM,
                                  move_manager* pMM,
                                  game_event_manager* pGEM,s32 ClientIndex);

    void    SetLoginSessionID   ( s32 ID );
    s32     GetLoginSessionID   ( void );

    void    ClearConnection     ( void );

    void    Kill                ( void );
    void    SendHeartbeat       ( void );
    void    HandleIncomingPacket( bitstream& BitStream );
    void    HandleOutgoingPacket( bitstream& BitStream );
    void    AdvanceStats        ( void );
    void    DumpStats           ( X_FILE* fp = NULL );

    xbool   IsConnected         ( void );

    f32     GetPing             ( void );
	f32		GetPacketLoss		( void );
	f32		GetShipInterval		( void );
	s32		GetBytesSent		( void ) { return m_BytesSent;};
	s32		GetBytesReceived	( void ) { return m_BytesReceived;};
	f32		GetConnectTime		( void ) { return m_ConnectTime.ReadSec();};
	void	UpdateShipInterval	( void );

    state_enum GetDestState     ( void );

    void    ClearPacketAcks     ( void );
    void    BuildPingGraph      ( xbitmap &Bitmap );

};

//=========================================================================
#endif
//=========================================================================

