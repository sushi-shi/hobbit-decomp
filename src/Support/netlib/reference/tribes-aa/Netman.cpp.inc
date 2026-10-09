//=============================================================================
//
//  NETMAN.CPP
//
//=============================================================================
#include <stdio.h>
#include "netman.hpp"
#include "netlib.hpp"
#include <xCore/x_files/x_debug.hpp>
#include "bitstream.hpp"

static xbool            s_Inited = FALSE;
static net_address      s_Address;
static net_fnptr        s_DefaultPacketHandler = NULL;
static net_connection*  s_pConnection=NULL;
static s32              s_NConnections=0;
static s32              s_NPacketsReceived=0;


//=============================================================================

void netman_Init                ( void )
{
    ASSERT( !s_Inited );
    s_Inited = TRUE;

    if( !net_IsInited() )
        net_Init();

    VERIFY( net_Bind( s_Address ) );

    s_pConnection = NULL;
    s_NConnections = 0;
    s_NPacketsReceived = 0;
}

//=============================================================================

void netman_Kill                ( void )
{
    ASSERT( s_Inited );
    ASSERT( s_NConnections==0 );
    s_Inited = FALSE;

    net_Unbind( s_Address );

    net_Kill();
}

//=============================================================================

net_address netman_GetAddress          ( void )
{
    return s_Address;
}

//=============================================================================

void netman_Send                ( net_address&  DestAddress, 
                                  bitstream&    BitStream )
{
    ASSERT( s_Inited );

    net_Send( s_Address, 
              DestAddress, 
              BitStream.GetDataPtr(), 
              BitStream.GetNBytesUsed() );
}

//=============================================================================

void netman_DispatchAllPackets ( void )
{
    bitstream BitStream;
    net_address SenderAddress;
    byte Buffer[1024];
    s32 MaxPackets = 32;
    
    ASSERT( s_Inited );

    while( MaxPackets-- )
    {
        s32 Size = 1024;
        
        if( !net_Receive( s_Address, SenderAddress, Buffer, Size ) )
            break;

        BitStream.Init( Buffer, Size );

        netman_DispatchPacket( SenderAddress, BitStream );
    }
}

//=============================================================================

void netman_DispatchPacket      ( net_address&  SenderAddress,
                                  bitstream&    BitStream )
{
    ASSERT( s_Inited );

    s_NPacketsReceived++;
    //if( (s_NPacketsReceived%4)==0 ) return;

    // Check if there is a connection registered with this
    // sender address.  If so hand the packet over.
    net_connection* pConn = s_pConnection;
    while( pConn != NULL )
    {
        net_address Addr = pConn->GetDstAddress();
        if( SenderAddress == Addr )
        {
            pConn->HandlePacket( BitStream );
            return;
        }
        pConn = pConn->m_pNext;
    }

    // No registered connection so hand over to default handler
    if( s_DefaultPacketHandler )
        s_DefaultPacketHandler( SenderAddress, BitStream );
}

//=============================================================================

void netman_RegisterHandler ( net_fnptr pFunction )
{
    ASSERT( s_Inited );
    s_DefaultPacketHandler = pFunction;
}

//=============================================================================

void netman_Heartbeat ( void )
{
    // Run through all connections and notify about passage of time
    ASSERT( s_Inited );

    net_connection* pConn = s_pConnection;
    while( pConn != NULL )
    {
        pConn->HandleHeartbeat();
        pConn = pConn->m_pNext;
    }
}

//=============================================================================

void netman_AddConnection       ( net_connection& Connection )
{
    // Add connection to list
    if( s_pConnection ) s_pConnection->m_pPrev = &Connection;
    Connection.m_pNext = s_pConnection;
    Connection.m_pPrev = NULL;
    s_pConnection = &Connection;
    s_NConnections++;
}

//=============================================================================

void netman_DelConnection       ( net_connection& Connection )
{
    // Remove connection from the list
    if( Connection.m_pNext ) Connection.m_pNext->m_pPrev = Connection.m_pPrev;
    if( Connection.m_pPrev ) Connection.m_pPrev->m_pNext = Connection.m_pNext;
    Connection.m_pNext = NULL;
    Connection.m_pPrev = NULL;
}

//=============================================================================
//=============================================================================
//=============================================================================
// IMPLEMENTATION OF NET_CONNECTION
//=============================================================================
//=============================================================================
//=============================================================================

net_connection::net_connection ( void )
{
    m_pNext = NULL;
    m_pPrev = NULL;
    m_SrcAddress.Clear();
    m_DstAddress.Clear();
}

//=============================================================================

net_connection::~net_connection( void )
{
}

//=============================================================================

void net_connection::HandleInit( net_address& Src, net_address& Dst )
{
    m_SrcAddress = Src;
    m_DstAddress = Dst;
}

//=============================================================================

void net_connection::HandleKill( void )
{
}

//=============================================================================

void net_connection::HandlePacket( bitstream& BitStream )
{
}

//=============================================================================

void net_connection::HandleHeartbeat( void )
{
}

//=============================================================================

void net_connection::SendPacket( bitstream& BitStream )
{
    netman_Send( m_DstAddress, BitStream );
}

//=============================================================================



