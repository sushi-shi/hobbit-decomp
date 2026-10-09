//=========================================================================
//
//  ConnManager.cpp
//
//=========================================================================

#include "x_files.hpp"
#include "connmanager.hpp"
#include "globals.hpp"
#include "specialversion.hpp"
#include "serverversion.hpp"

//==============================================================================

#ifdef VERBOSE_NETWORK
extern const char* s_StateName[];
#endif

//==============================================================================

xbool SHOW_PACKET_INFO = FALSE;

//==============================================================================

ack_bits::ack_bits( void )
{
    Clear();
}

//------------------------------------------------------------------------------

void ack_bits::Clear( void )
{
    for( s32 i = 0; i < ACKSLOTS; i++ )
        Bits[i] = 0xFFFF;
}

//------------------------------------------------------------------------------

void ack_bits::Print( void )
{
    x_DebugMsg( "%04X", Bits[ACKSLOTS-1] );
    for( s32 i = ACKSLOTS-2; i >= 0; i-- )
        x_DebugMsg( ",%04X", Bits[i] );
}

//------------------------------------------------------------------------------

void ack_bits::ShiftLeft( s32 Shift )
{
    while( Shift > 0 )
    {
        Shift--;
        Bits[ACKSLOTS-1] <<= 1;
        for( s32 i = ACKSLOTS-2; i >= 0; i-- )
        {
            if( Bits[i  ] &  0x8000 )
                Bits[i+1] |= 0x0001;
            Bits[i] <<= 1;
        }
    }
}

//------------------------------------------------------------------------------

xbool ack_bits::Bit( s32 Position )
{
    s32 Slot = Position / (16  );
    s32 Off  = Position & (16-1);

    return( (Bits[Slot] & (1<<Off)) != 0 );
}

//------------------------------------------------------------------------------

void ack_bits::Set( s32 Position )
{
    s32 Slot = Position / (16  );
    s32 Off  = Position & (16-1);

    Bits[Slot] |= (1 << Off);
}

//------------------------------------------------------------------------------

void ack_bits::Read( bitstream& BS )
{
    s32 i;
    u32 Mask;

    if( BS.ReadFlag() )
    {
        // All bits are 1.  No problem.
        for( i = 0; i < ACKSLOTS; i++ )
            Bits[i] = 0xFFFF;
    }
    else
    {
        // There are some 0 bits.  Oh well.
        BS.ReadU32( Mask, ACKSLOTS );
        for( i = 0; i < ACKSLOTS; i++ )
            if( Mask & (1<<i) )
                BS.ReadU16( Bits[i] );
    }

    /*
    for( s32 i = 0; i < ACKSLOTS; i++ )
        BS.ReadU16( Bits[i] );
    */
}

//------------------------------------------------------------------------------

void ack_bits::Write( bitstream& BS )
{
    s32 i;
    u32 Mask = 0;

    ASSERT( ACKSLOTS <= 32 );

    for( i = 0; i < ACKSLOTS; i++ )
        if( Bits[i] != 0xFFFF )
            Mask |= (1 << i);

    if( Mask )
    {
        // There were some 0 bits in there.  Crap.
        BS.WriteFlag( FALSE );
        BS.WriteU32( Mask, ACKSLOTS );
        for( i = 0; i < ACKSLOTS; i++ )
            if( Mask & (1<<i) )
                BS.WriteU16( Bits[i] );
    }
    else
    {
        // All bits were 1.  Hurray!
        BS.WriteFlag( TRUE );
    }

    /*
    for( s32 i = 0; i < ACKSLOTS; i++ )
        BS.WriteU16( Bits[i] );
    */
}

//=============================================================================

void PrintBits( u32 V, s32 NBits )
{
    for( s32 i=0; i<NBits; i++ )
    {
        x_DebugMsg("%1d",(V&(1<<(NBits-i-1)))?(1):(0) );
    }
    x_DebugMsg("\n");
}

//=========================================================================

conn_manager::conn_manager( void )
{
}

//=========================================================================

conn_manager::~conn_manager( void )
{
}

//=========================================================================

void conn_manager::Init( net_address& Src,
                         net_address& Dst,
                         update_manager* pUM,
                         move_manager* pMM,
                         game_event_manager* pGEM,s32 ClientIndex)
{
    m_SrcAddress        = Src;
    m_DstAddress        = Dst;
    m_NPacketsSent      =  0;
    m_NPacketsDropped   =  0;
    m_NPacketsReceived  =  0;
    m_NPacketsUnusable  =  0;
	m_ClientIndex		= ClientIndex;
	m_PacketShipInterval= DEFAULT_SHIP_INTERVAL;
	m_PacketLossIndex	= 0;
	x_memset(m_PacketAck,0,sizeof(m_PacketAck));
	x_memset(m_PacketNak,0,sizeof(m_PacketNak));

    m_pUM               = pUM;
    m_pMM               = pMM;
    m_pGEM              = pGEM;

    m_pUM->Init(ClientIndex);
    m_pMM->Init();
    m_pGEM->Init();


    m_DoLogging   = FALSE;
    m_LogOutgoing = FALSE;
    m_LogIncoming = FALSE;
    m_DestState   = STATE_NULL;
	m_BytesSent		= 0;
	m_BytesReceived	= 0;
	m_ConnectTime.Reset();
	m_ConnectTime.Start();

    ClearConnection();
}

//=========================================================================

void conn_manager::ClearConnection( void )
{
    s32 i;
    m_IsConnected       = TRUE;
    m_LastSeqReceived   = -1;
    m_LastSeqSent       = -1;
    m_LastAckReceived   = -1;
    m_LastAckSent       = -1;
    m_AckBits.Clear();

    m_LastConfirmAliveTime = tgl.LogicTimeMs;
    m_HeartbeatsSent    = 0;
    m_AveragePing       = 16;

    for( i=0; i<CONN_NUM_PACKETS; i++ )
        m_Packet[i].Seq = -1;

    m_LastHeartbeatRequest = -1;
    for( i=0; i<CONN_NUM_HEARTBEATS; i++ )
        m_HeartbeatPing[i] = -1;

    m_StatsIndex = 0;
    x_memset( m_Stats, 0, sizeof(m_Stats) );
}

//=========================================================================

void conn_manager::Kill( void )
{
    m_pUM->Kill();
}

//=========================================================================

void conn_manager::HandlePacketNotify( s32 PacketID, xbool Received )
{
    // Find the packet
    s32 P;
    for( P=0; P<CONN_NUM_PACKETS; P++ )
    if( m_Packet[P].Seq == PacketID )
        break;

    if( P==CONN_NUM_PACKETS )
    {
        //x_DebugMsg("**********************************\n");
        //x_DebugMsg("Notify for packet already cleared\n");
        //x_DebugMsg("**********************************\n");
        return;
    }

    if( m_Packet[P].DoNotAck )
    {
        x_DebugMsg("Ack for DoNotAck Packet\n");
        return;
    }

    //ASSERT( P < CONN_NUM_PACKETS );

    // Handle notify for this packet
    if( Received )
    {
		m_PacketAck[m_PacketLossIndex]++;
        //x_DebugMsg("Packet ack %1d %1d\n",P,m_Packet[P].Seq);
        if( SHOW_PACKET_INFO )
            x_DebugMsg("************** packet %1d acknowledged RECEIVED\n",PacketID);
    }
    else
    {
		m_PacketNak[m_PacketLossIndex]++;
		x_DebugMsg("Packet dropped %1d %1d\n",P,m_Packet[P].Seq);
        if( SHOW_PACKET_INFO )
            x_DebugMsg("************** packet %1d acknowledged DROPPED\n",PacketID);
        m_NPacketsDropped++;
    }

    if( tgl.ServerPresent )
    {
        m_pUM ->PacketAck( m_Packet[P], Received );
        m_pGEM->PacketAck( m_Packet[P], Received );
    }

    if( tgl.ClientPresent )
    {
        m_pMM->PacketAck( m_Packet[P], Received );
    }

    m_Packet[P].Seq = -1;
}

//=========================================================================
f32 conn_manager::GetPacketLoss(void)
{
	s32 i;
	s32 naked;
	s32 acked;
	s32 total;
	f32 pc;

	naked = 0;
	acked = 0;
	total = 0;
	for (i=0;i<PACKET_LOSS_HISTORY;i++)
	{
		naked += m_PacketNak[i];
		acked += m_PacketAck[i];
	}

	if (naked+acked)
	{
		pc = (f32)naked / (f32)(naked+acked);
	}
	else
	{
		pc = 0.0f;
	}

	return pc;
}

//=========================================================================

s32 g_PacketSeq = 0;

void conn_manager::HandleIncomingPacket( bitstream& BitStream )
{
	if (m_PacketLossTimer.ReadSec() > 0.1f)
	{
		m_PacketLossIndex++;
		if (m_PacketLossIndex >= PACKET_LOSS_HISTORY)
		{
			m_PacketLossIndex=0;
		}
		m_PacketAck[m_PacketLossIndex]=0;
		m_PacketNak[m_PacketLossIndex]=0;
		m_PacketLossTimer.Stop();
	}

	if (!m_PacketLossTimer.IsRunning())
	{
		m_PacketLossTimer.Reset();
		m_PacketLossTimer.Start();
	}

    if( m_DoLogging )
    {
        if( m_LogIncoming == NULL )
        {
            char SrcStr[64];
            char DstStr[64];
            m_SrcAddress.GetStrIP(SrcStr);
            m_DstAddress.GetStrIP(DstStr);
            m_LogIncoming = x_fopen(xfs("To(%1s)From(%1s).bin",SrcStr,DstStr),"wb");
        }

        if( m_LogIncoming )
        {
            s32 NBytes = BitStream.GetNBytes();
            x_fwrite( &NBytes, 4, 1, m_LogIncoming );
            x_fwrite( BitStream.GetDataPtr(), NBytes, 1, m_LogIncoming );
        }
    }
	m_BytesReceived += BitStream.GetNBytes();

    // Throw out packet if not ours
    u32 KnownBytes;
    BitStream.ReadU32(KnownBytes);

    // Check if it is a heartbeat request
    if( KnownBytes == CONN_PACKET_TYPE_HB_REQUEST )
    {
        s32 Index;
        s32 State;
        BitStream.ReadS32( Index );
        BitStream.ReadS32( State );
        m_DestState = (state_enum)State;

        // If client use new send rates
        if( tgl.ClientPresent )
        {
            f32 HD,CPSD;
            BitStream.ReadF32( HD );        // DMT TO DO - Maybe not send this every time
            BitStream.ReadF32( CPSD );      // DMT TO DO - Or reduce bits used.

            if( HD != tgl.HeartbeatDelay )
                x_DebugMsg("HeartbeatDelay changed by server to %f\n",HD);

#if 0
            if( CPSD != tgl.pClient->tgl.ClientPacketShipDelay )
                x_DebugMsg("ClientPacketShipDelay changed by server to %f\n",CPSD);

            tgl.ClientPacketShipDelay = CPSD;
#endif
            tgl.HeartbeatDelay = HD;
            #ifdef VERBOSE_NETWORK
            x_DebugMsg( "<--- RECV HBREQ %d  State %d(%s)  HBDelay %g  PktDelay %g\n",
                        Index, State, s_StateName[ State ], HD, CPSD );
            #endif
        }
        else
        {
            #ifdef VERBOSE_NETWORK
            x_DebugMsg( "<--- RECV HBREQ %d  State %d(%s)\n",
                        Index, State, s_StateName[ State ] );
            #endif
        }

        if(     (Index > m_LastHeartbeatRequest)
            ||  ((Index < 0) && (m_LastHeartbeatRequest > 0)) )
            m_LastHeartbeatRequest = Index;

        bitstream BS;
        BS.WriteU32( CONN_PACKET_TYPE_HB_RESPONSE );
        BS.WriteS32( Index );
        BS.WriteS32( g_SM.CurrentState );
        BS.Encrypt( ENCRYPTION_KEY );
		m_BytesSent += BS.GetNBytesUsed();
        BS.Send( m_SrcAddress, m_DstAddress );

        #ifdef VERBOSE_NETWORK
        x_DebugMsg( "---> SEND HBRSP %d  State %d(%s)\n",
                    Index, g_SM.CurrentState, s_StateName[ g_SM.CurrentState ] );
        #endif

        return;
    }

    // Check if it is a heartbeat response
    if( KnownBytes == CONN_PACKET_TYPE_HB_RESPONSE )
    {
        ReceiveHeartbeat( BitStream );
        return;
    }

    if( KnownBytes != CONN_PACKET_TYPE_MANAGER_DATA )
    {
        m_NPacketsUnusable++;
        x_DebugMsg( "packet dropped for unknown starting bytes\n" );
        return;
    }

    //
    // This is a known packet type
    //
    m_NPacketsReceived++;
    //x_DebugMsg("NReceived %1d\n",m_NPacketsReceived );
    // Read packet header
    s32         pkSeqNum;
    s32         pkHighestAck;
    ack_bits pkAckBits;
    u32         SessionID;

    BitStream.ReadU32( SessionID, 8 );
    BitStream.ReadS32( pkSeqNum, 24 );
    BitStream.ReadS32( pkHighestAck, 24 );
    pkAckBits.Read( BitStream );

    g_PacketSeq = pkSeqNum;

    // Verbosity.
    #ifdef VERBOSE_NETWORK
    {
        x_DebugMsg( "<<== RECV %d <", pkSeqNum );
        pkAckBits.Print();
        x_DebugMsg( "> HighAck %d\n", pkHighestAck );
    }
    #endif

    //x_DebugMsg("RECV---- SES(%1d) SEQ(%1d)(%1d,%1d) TYPE(%08X) PING(%1.2f)\n",SessionID,pkSeqNum,m_LastSeqReceived,m_LastSeqReceived+128,KnownBytes,GetPing());

    if( SHOW_PACKET_INFO )
    {
        x_DebugMsg("%5d RECV PACKET(%1d) %1d\n",(s32)tgl.LogicTimeMs,pkSeqNum,BitStream.GetNBytes());
    }

    //x_DebugMsg("HANDLE PACKET: %5d %5d ",pkSeqNum,pkHighestAck,pkAckBits);
    //PrintBits(pkAckBits,16);

    // Check if session number is wrong
    if( SessionID != ((u32)m_LoginSessionID & 0xFF) )
    {
        // bad packet
        m_NPacketsUnusable++;
        x_DebugMsg("packet dropped for bad session number %1d\n",pkSeqNum);
        x_DebugLog("\n\n--------------------------\n\n    PACKET DROPPED! SESSION ID WRONG! (%d,%d)\n\n--------------------------\n\n",SessionID,((u32)m_LoginSessionID & 0xFF));
        return;
    }

    // Check if sequence number is outside the known window
    if( ((s32)pkSeqNum <= m_LastSeqReceived) ||
        ((s32)pkSeqNum  > m_LastSeqReceived+128) )
    {
        // bad packet
        m_NPacketsUnusable++;
        x_DebugMsg("packet %08X dropped for bad seq number %1d (%1d,%1d)\n",KnownBytes,pkSeqNum,m_LastSeqReceived+1,m_LastSeqReceived+128);
        return;
    }

    //
    // Store that we have received a new packet
    //
    {
        // Shift up ack bits so all packets between new and last have zeros
        m_AckBits.ShiftLeft( pkSeqNum - m_LastSeqReceived );

        // Mark this packet as being received
        m_AckBits.Set( 0 );

        // Remember we have received up to this point
        m_LastSeqReceived = pkSeqNum;
    }

    // Confirm that the connection is still alive
    m_LastConfirmAliveTime = tgl.LogicTimeMs;

    //
    // React to packet's ack bits
    //
    if( (s32)pkHighestAck != -1 )
    {
        for( s32 i=m_LastAckReceived+1; i<=(s32)pkHighestAck; i++ )
        {
            s32 Shift = (pkHighestAck-i);
            xbool Ack;

            if( Shift > 60 )
                x_DebugMsg( "*** Big shift for pkt %d: %d\n", i, Shift );

            if( Shift > ACKBITS )
            {
                x_DebugMsg( "****************** FATAL shift for pkt %d: %d\n", i, Shift );
                m_IsConnected = FALSE;
                return;
            }

            if( Shift > ACKBITS )
                Ack = FALSE;
            else
                Ack = pkAckBits.Bit( Shift );

            HandlePacketNotify( i, Ack );
        }

        // Remember highest ack
        m_LastAckReceived = pkHighestAck;
    }

    // Tell other managers to dissect packet
    ReadManagerData( BitStream );
}

//=========================================================================

void conn_manager::HandleOutgoingPacket( bitstream& BitStream )
{
    ASSERT( m_pUM );

    if( m_IsConnected == FALSE )
        return;

    // Allocate a packet
    s32 P;
    for( P=0; P<CONN_NUM_PACKETS; P++ )
    if( m_Packet[P].Seq == -1 )
        break;

    if( P==CONN_NUM_PACKETS )
    {
        // Flush some of the packets
        for( s32 i=0; i<CONN_NUM_PACKETS; i++ )
        {
            HandlePacketNotify( m_LastAckReceived, FALSE );
            m_LastAckReceived++;
        }

        m_IsConnected = FALSE;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("ConnManager: Too many un-recv packets\n");
        x_DebugMsg("*************************************\n");
        return;
    }

    m_LastSeqSent++;
    //x_DebugMsg("NSent %1d\n",m_LastSeqSent );

    m_Packet[P].Seq = m_LastSeqSent;

    // Clear packet values
    m_Packet[P].DoNotAck = FALSE;
    m_Packet[P].NLife = 0;
    m_Packet[P].NUpdates = 0;
    m_Packet[P].FirstUpdate = -1;

    //x_DebugMsg("BUILDING PKT %1d\n",m_LastSeqSent);

    // Pack in known bytes
    BitStream.WriteU32( CONN_PACKET_TYPE_MANAGER_DATA );

    // Pack in session number
    BitStream.WriteU32( (u32)m_LoginSessionID, 8 );

    // Pack in new sequence number
    BitStream.WriteS32( m_LastSeqSent, 24 );

    // Pack in new acknowledge number
    BitStream.WriteS32( m_LastSeqReceived, 24 );

    // Pack in ack bits
    m_AckBits.Write( BitStream );

    //x_DebugMsg("SEND(%5d) SES(%1d) SEQ(%1d)\n",(s32)tgl.LogicTimeMs,m_LoginSessionID&0xFF,m_LastSeqSent);
    //x_DebugMsg("SEND---- SES(%1d) SEQ(%1d)                      TYPE(%08X) PING(%f)\n",m_LoginSessionID&0xFF,m_LastSeqSent,CONN_PACKET_TYPE_MANAGER_DATA,GetPing());
    //x_DebugMsg("SEND---- SES(%1d) SEQ(%1d)                                                  TYPE(%08X) PING(%1.2f)\n",m_LoginSessionID&0xFF,m_LastSeqSent,CONN_PACKET_TYPE_MANAGER_DATA,GetPing());
    //x_DebugMsg( "    ---- " );

    ASSERT( m_pUM );

    // Tell other managers to pack in their data
    WriteManagerData( m_Packet[P], BitStream );

    //x_DebugMsg( "(L:%d,U:%d)\n", m_Packet[P].NLife, m_Packet[P].NUpdates );

    ASSERT( m_pUM );

	m_BytesSent += BitStream.GetNBytesUsed();
    BitStream.Send( m_SrcAddress, m_DstAddress);

    if( SHOW_PACKET_INFO )
    {
        x_DebugMsg("%5d SEND PACKET(%1d) %1d\n",(s32)tgl.LogicTimeMs,m_Packet[P].Seq,BitStream.GetNBytesUsed());
    }

    if( m_DoLogging )
    {
        if( m_LogOutgoing == NULL )
        {
            char SrcStr[64];
            char DstStr[64];
            m_SrcAddress.GetStrIP(SrcStr);
            m_DstAddress.GetStrIP(DstStr);
            m_LogOutgoing = x_fopen(xfs("To(%1s)From(%1s).bin",DstStr,SrcStr),"wb");
        }

        if( m_LogOutgoing )
        {
            s32 NBytes = BitStream.GetNBytesUsed();
            x_fwrite( &NBytes, 4, 1, m_LogOutgoing );
            x_fwrite( BitStream.GetDataPtr(), NBytes, 1, m_LogOutgoing );
        }
    }

    // Verbosity.
    #ifdef VERBOSE_NETWORK
    {
        x_DebugMsg( "==>> SEND %d <", m_LastSeqSent );
        m_AckBits.Print();
        x_DebugMsg( "> HighAck %d\n", m_LastSeqReceived );
    }
    #endif

    //x_printf("(%2d,%4d) ",P,BitStream.GetNBytesUsed());
    m_NPacketsSent++;
}

//=========================================================================

xbool conn_manager::IsConnected( void )
{
    if( (tgl.LogicTimeMs - m_LastConfirmAliveTime) > tgl.ConnManagerTimeout )
    {
        m_IsConnected = FALSE;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("ConnManager: ConfirmAlive timeout\n");
        x_DebugMsg("*************************************\n");
    }

    return m_IsConnected;
}

//=========================================================================

void conn_manager::ReceiveHeartbeat( bitstream& BitStream )
{
    // Look up which heartbeat it was
    s32 Index;
    BitStream.ReadS32(Index);

    #ifdef VERBOSE_NETWORK
    x_DebugMsg( "<--- RECV HBRSP %d", Index );
    #endif

    //x_DebugMsg("HEARTBEAT: Response received %1d\n",Index);

    // If index is out of range it is probably from an old mission
    if( Index > m_HeartbeatsSent )
    {
        #ifdef VERBOSE_NETWORK
        x_DebugMsg( " *** IGNORED *** OUT OF RANGE ***\n" );
        #endif
        return;
    }

    // Decide if it's too far in the past
    ASSERT(Index <= m_HeartbeatsSent);
    if( (m_HeartbeatsSent - Index) > CONN_NUM_HEARTBEATS )
    {
        #ifdef VERBOSE_NETWORK
        x_DebugMsg( " *** IGNORED *** TOO OLD ***\n" );
        #endif
        return;
    }

    // Read current state
    s32 State;
    BitStream.ReadS32(State);
    m_DestState = (state_enum)State;

    #ifdef VERBOSE_NETWORK
    x_DebugMsg( "  State %d(%s)", State, s_StateName[State] );
    #endif

    // Confirm that the connection is still alive
    m_LastConfirmAliveTime = tgl.LogicTimeMs;

    // Look up time heartbeat traveled
    //s32 Time = (s32)(tgl.LogicTimeMs - m_HeartbeatSentTime[Index%CONN_NUM_HEARTBEATS]);
    f32 Time = x_TicksToMs( x_GetTime() - (xtick)m_HeartbeatSentTime[Index%CONN_NUM_HEARTBEATS] );

    // Store ping time
    m_HeartbeatPing[Index%CONN_NUM_HEARTBEATS] = Time;
	m_LastPing = Time;

    // Update ping
    s32 N = MIN(CONN_NUM_HEARTBEATS,m_HeartbeatsSent);
    m_AveragePing = 0;
    for( s32 i=0; i<N; i++ )
    {
        m_AveragePing += m_HeartbeatPing[ (Index + (CONN_NUM_HEARTBEATS-i))%CONN_NUM_HEARTBEATS ];
    }
    m_AveragePing *= (1.0f/N);

    #ifdef VERBOSE_NETWORK
    x_DebugMsg( "  Ping %g\n", m_AveragePing );
    #endif

    // Display ping buffer
    if( 0 )
    {
        x_printf("PINGS ");
        for( s32 j=0; j<CONN_NUM_HEARTBEATS; j++ )
        {
            x_printf("%5.1f",m_HeartbeatPing[j]);
        }
        x_printf("\n");
    }

	UpdateShipInterval();

    //if( m_AveragePing > 10000 )
    //    m_AveragePing = (f32)Time;
    //else
    //    m_AveragePing = (m_AveragePing*0.90f + (f32)Time*0.10f);

    //m_AveragePing = (f32)Time;
}

//=========================================================================

void conn_manager::SendHeartbeat( void )
{
    m_HeartbeatsSent++;
    m_HeartbeatSentTime[ m_HeartbeatsSent%CONN_NUM_HEARTBEATS ] = (s64)x_GetTime();//tgl.LogicTimeMs;

    //x_DebugMsg("HEARTBEAT: Sending request %1d\n",m_HeartbeatsSent);
    bitstream BS;
    BS.WriteU32( CONN_PACKET_TYPE_HB_REQUEST );
    BS.WriteS32( m_HeartbeatsSent );
    BS.WriteS32( g_SM.CurrentState );

    if( tgl.ServerPresent )
    {
        BS.WriteF32( tgl.HeartbeatDelay );
        BS.WriteF32( GetShipInterval() );

        #ifdef VERBOSE_NETWORK
        x_DebugMsg( "---> SEND HBREQ %d  State %d(%s)  HBDelay %g  PktDelay %g\n",
                    m_HeartbeatsSent,
                    g_SM.CurrentState,
                    s_StateName[ g_SM.CurrentState ],
                    tgl.HeartbeatDelay,
                    GetPacketShipInterval() );
        #endif
    }
    else
    {
        #ifdef VERBOSE_NETWORK
        x_DebugMsg( "---> SEND HBREQ %d  State %d(%s)\n",
                    m_HeartbeatsSent,
                    g_SM.CurrentState,
                    s_StateName[ g_SM.CurrentState ] );
		#endif
    }

    BS.Encrypt(ENCRYPTION_KEY);
	m_BytesSent += BS.GetNBytesUsed();
    BS.Send(m_SrcAddress,m_DstAddress);

    {
        //char HBDst[32];
        //m_DstAddress.GetStrIP(HBDst);
        //x_DebugMsg("Heartbeat sent to %s\n",HBDst);
    }
}

//=========================================================================

f32 conn_manager::GetPing( void )
{
    return m_AveragePing;
}

//=========================================================================

state_enum conn_manager::GetDestState( void )
{
    return m_DestState;
}
//
// Returns a ptr to a bitmap containing a graph of the ping times
// Horizontal axis is time, vertical axis is 10ms steps.
//
void conn_manager::BuildPingGraph(xbitmap &Bitmap)
{
#if 0
    s32 i;
    s32 Index;
    s8  *ptr;
    s32 j;
    s32 Height;

    ASSERT(Bitmap.GetBPP() == 8);

    Index = (m_HeartbeatsSent % CONN_NUM_HEARTBEATS);


    for (i=0;i<Bitmap.GetWidth();i++)
    {
        if (i> CONN_NUM_HEARTBEATS)
        {
            Height = 0;
        }
        else
        {
            Height = (s32)(m_HeartbeatPing[Index] * 100.0f);
            // Ping times in 10ms steps
            if (Height > Bitmap.GetHeight())
            {
                Height = Bitmap.GetHeight();
            }
        }
        ptr = (s8 *)Bitmap.GetPixelData() + i;
        for (j=0;j<Bitmap.GetHeight()-Height;j++)
        {
            *ptr = 0x0;
            ptr += Bitmap.GetWidth();
        }
        for (j=0;j<Height;j++)
        {
            *ptr = 0x1;
            ptr += Bitmap.GetWidth();
        }

        Index++;
        if (Index >= CONN_NUM_HEARTBEATS)
            Index=0;


    }
#else
    (void)Bitmap;
#endif
}
//=========================================================================

void conn_manager::WriteManagerData( conn_packet& Packet, bitstream& BitStream )
{
    s32 Cursor;
    s32 NGameManagerBits=0;
    s32 NMoveBits=0;
    s32 NLifeBits=0;
    s32 NUpdateBits=0;

    Packet.NMoves = 0;
    Packet.NLife = 0;
    Packet.NUpdates = 0;
    Packet.GMDirtyBits  = 0x00;
    Packet.GMPlayerBits = 0x00;
    Packet.GMScoreBits  = 0x00;


    if( tgl.ServerPresent )
    {
        Cursor = BitStream.GetCursor();
        m_pGEM->PackGameManager( Packet, BitStream );
        ASSERT( BitStream.Overwrite() == FALSE );
        NGameManagerBits = BitStream.GetCursor() - Cursor;
    }

    if( tgl.ClientPresent )
    {
        Cursor = BitStream.GetCursor();
        BitStream.OpenSection();
        if( BitStream.WriteFlag( TRUE ) )
        {
            m_pMM->PackMoves( Packet, BitStream, CONN_MOVES_PER_PACKET );
        }
        if( !BitStream.CloseSection() ) BitStream.WriteFlag( FALSE );
        ASSERT( BitStream.Overwrite() == FALSE );
        NMoveBits = BitStream.GetCursor() - Cursor;
    }

    if( tgl.ServerPresent )
    {
        ASSERT( m_pUM );

        Cursor = BitStream.GetCursor();
        BitStream.OpenSection();
        if( BitStream.WriteFlag( BitStream.GetNBitsFree() > 32 ) )
        {
            m_pUM->PackLife( Packet, BitStream, CONN_LIVES_PER_PACKET );
        }
        if( !BitStream.CloseSection() ) BitStream.WriteFlag( FALSE );
        ASSERT( BitStream.Overwrite() == FALSE );
        NLifeBits = BitStream.GetCursor() - Cursor;

        Cursor = BitStream.GetCursor();
        BitStream.OpenSection();
        if( BitStream.WriteFlag( BitStream.GetNBitsFree() > 32 ) )
        {
            m_pUM->PackUpdates( Packet, BitStream, 100 );
        }
        if( !BitStream.CloseSection() ) BitStream.WriteFlag( FALSE );
        ASSERT( BitStream.Overwrite() == FALSE );
        NUpdateBits = BitStream.GetCursor() - Cursor;
    }

    m_Stats[m_StatsIndex].LifeSent    += Packet.NLife;
    m_Stats[m_StatsIndex].UpdatesSent += Packet.NUpdates;
    m_Stats[m_StatsIndex].MovesSent   += Packet.NMoves;
    m_Stats[m_StatsIndex].BytesSent   += BitStream.GetNBytesUsed();
    m_Stats[m_StatsIndex].PacketsSent += 1;

    //x_DebugMsg("WMD: %1d %1d %1d %1d\n",NGameManagerBits,NMoveBits,NLifeBits,NUpdateBits);
}

//=========================================================================

void conn_manager::ReadManagerData( bitstream& BitStream )
{
    if( tgl.ClientPresent )
    {
        m_pGEM->UnpackGameManager( BitStream, m_AveragePing*0.5f/1000.0f );
    }

    if( tgl.ServerPresent )
    {
        if( BitStream.ReadFlag() )
            m_pMM->UnpackMoves( BitStream );
    }

    if( tgl.ClientPresent )
    {
        if( BitStream.ReadFlag() )
            m_pUM->UnpackLife( BitStream );

        if( BitStream.ReadFlag() )
            m_pUM->UnpackUpdates( BitStream, m_AveragePing*0.5f/1000.0f );
    }
}

//=========================================================================

void conn_manager::AdvanceStats( void )
{
    // Move to next stats
    m_StatsIndex = (m_StatsIndex+1)%CONN_NUM_STATS;

    // Clear stats
    m_Stats[m_StatsIndex].LifeSent    = 0;
    m_Stats[m_StatsIndex].UpdatesSent = 0;
    m_Stats[m_StatsIndex].MovesSent   = 0;
    m_Stats[m_StatsIndex].BytesSent   = 0;
    m_Stats[m_StatsIndex].PacketsSent = 0;
}

//=========================================================================

void conn_manager::DumpStats( X_FILE* fp )
{
    conn_stats S;
    x_memset(&S,0,sizeof(conn_stats));

    // Sum up all collected stats
    for( s32 i=0; i<CONN_NUM_STATS; i++ )
    {
        S.BytesSent += m_Stats[i].BytesSent;
        S.LifeSent  += m_Stats[i].LifeSent;
        S.MovesSent  += m_Stats[i].MovesSent;
        S.PacketsSent += m_Stats[i].PacketsSent;
        S.UpdatesSent += m_Stats[i].UpdatesSent;
    }

    if( fp == NULL )
    {
        x_DebugMsg("Connmanager stats:\n");
        x_DebugMsg("Packets   %5d\n",S.PacketsSent);
        x_DebugMsg("BytesSent %5d  BSPP %5d\n",S.BytesSent,(S.PacketsSent)?(S.BytesSent/S.PacketsSent):(0));
        x_DebugMsg("LifeSent  %5d\n",S.LifeSent);
        x_DebugMsg("MovesSent %5d\n",S.MovesSent);
        x_DebugMsg("Updates   %5d\n",S.UpdatesSent);
    }
    else
    {
        x_fprintf(fp,"Connmanager stats:\n");
        x_fprintf(fp,"Packets   %5d\n",S.PacketsSent);
        x_fprintf(fp,"BytesSent %5d  BSPP %5d\n",S.BytesSent,(S.PacketsSent)?(S.BytesSent/S.PacketsSent):(0));
        x_fprintf(fp,"LifeSent  %5d\n",S.LifeSent);
        x_fprintf(fp,"MovesSent %5d\n",S.MovesSent);
        x_fprintf(fp,"Updates   %5d\n",S.UpdatesSent);
    }
}

//=========================================================================

void conn_manager::ClearPacketAcks( void )
{
    s32 P;
    for( P=0; P<CONN_NUM_PACKETS; P++ )
    if( m_Packet[P].Seq != -1 )
        m_Packet[P].DoNotAck = TRUE;
}

//=========================================================================

void conn_manager::SetLoginSessionID( s32 ID )
{
    ClearConnection();
    m_LoginSessionID = ID;
    x_DebugMsg( "********* SESSION ID: %d\n", ID );
}

//=========================================================================

s32 conn_manager::GetLoginSessionID( void )
{
    return m_LoginSessionID;
}

//=========================================================================
f32 conn_manager::GetShipInterval(void)
{
	return m_PacketShipInterval;
}

//=========================================================================
void conn_manager::UpdateShipInterval(void)
{
#if 1
	f32 interval;

	if (m_LastPing > 100.0f)
		interval = ((m_LastPing)/2.0f)/1000.0f;
	else
		interval = 1.0f/12.0f;

	if ( interval < m_PacketShipInterval)
	{
		m_PacketShipInterval -= (SHIP_INTERVAL_INCREMENT/3);
	}
	if ( interval > m_PacketShipInterval)
	{
		m_PacketShipInterval += (SHIP_INTERVAL_INCREMENT/3);
	}

	if (m_PacketShipInterval < DEFAULT_SHIP_INTERVAL )
	{
		m_PacketShipInterval = DEFAULT_SHIP_INTERVAL;
	}
	if (m_PacketShipInterval > (1.0f/4.0f) )
	{
		m_PacketShipInterval = 1.0f/4.0f;
	}

	if (m_LastPing > 600.0f)
	{
		m_PacketShipInterval = 1.0f/4.0f;
	}
#else
    if ( m_LastPing > HIGH_PING_THRESHOLD )
    {
		m_PacketShipInterval += SHIP_INTERVAL_INCREMENT;
    }

	if ( m_LastPing < LOW_PING_THRESHOLD )
	{
		m_PacketShipInterval -= SHIP_INTERVAL_INCREMENT;
	}

	if (m_PacketShipInterval < DEFAULT_SHIP_INTERVAL)
		m_PacketShipInterval = DEFAULT_SHIP_INTERVAL;

	if ( (m_PacketShipInterval > MAX_SHIP_INTERVAL) || (GetPing() > 1500.0f) )
	{
		m_PacketShipInterval = MAX_SHIP_INTERVAL;
	}
#endif
}
