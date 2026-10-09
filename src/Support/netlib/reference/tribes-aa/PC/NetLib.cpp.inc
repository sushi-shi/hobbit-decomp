//==============================================================================
//
//  NETLIB.CPP
//
//==============================================================================

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <xCore/x_files/x_files.hpp>
#include "..\NetLib.hpp"

//==============================================================================

static u32 s_STAT_NPacketsSent;
static u32 s_STAT_NPacketsReceived;
static u64 s_STAT_NBytesSent;
static u64 s_STAT_NBytesReceived;
static u32 s_STAT_NAddressesBound;
extern xtimer NET_SendTime;
extern xtimer NET_ReceiveTime;

static xbool s_Inited = FALSE;

//==============================================================================

void    net_PCInit        ( void )
{
    ASSERT( !s_Inited );
    s_Inited = TRUE;

    s_STAT_NPacketsSent     = 0;
    s_STAT_NPacketsReceived = 0;
    s_STAT_NBytesSent       = 0;
    s_STAT_NBytesReceived   = 0;
    s_STAT_NAddressesBound  = 0;

#ifdef WIN32
    WSADATA wsadata;

    // initialize the appropriate sockets
    WSAStartup( 0x0101, &wsadata );
   
#endif

}

//==============================================================================

void    net_PCKill        ( void )
{
    ASSERT( s_Inited );
    s_Inited = FALSE;

#ifdef WIN32
    WSACleanup();
#endif
}

//==============================================================================

xbool   net_IsInited    ( void )
{
    return s_Inited;
}

//==============================================================================

xbool    net_Bind        ( net_address& NewAddress, u32 StartPort )
{
    ASSERT( s_Inited );

    // Clear address in case we need to exit early
    NewAddress.Clear();

#ifdef WIN32
    struct sockaddr_in addr;
    SOCKET sd_dg;

    // create an address and bind it
    x_memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(StartPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // create a socket
    sd_dg = socket( PF_INET, SOCK_DGRAM, 0 );

    // attempt to bind to the port
    while ( bind( sd_dg, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR )
    {
        // increment port if that port is assigned
        if ( WSAGetLastError() == WSAEADDRINUSE )
        {
            StartPort++;
            addr.sin_port = htons( StartPort );
        }
        // if some other error, nothing we can do...abort
        else
        {
            return FALSE;
            //StartPort = net_NOSLOT;
            //break;
        }
    }

    // set this socket to non-blocking, so we can poll it
    u_long  dwNoBlock = TRUE;
    ioctlsocket( sd_dg, FIONBIO, &dwNoBlock );

    // fill out the slot structure
    // first, determine local IP address
    char tbuf[128];
    hostent* pHe;
    struct in_addr in_IP;

    gethostname( tbuf, 127 );
    pHe = gethostbyname( tbuf );

    in_IP = *(struct in_addr *)(pHe->h_addr);

    NewAddress.IP = in_IP.s_addr;
    NewAddress.Port = StartPort;//htons( StartPort );
    NewAddress.Socket = (u32)sd_dg;

#endif
    s_STAT_NAddressesBound++;
    return TRUE;
}
xbool    net_BindBroadcast( net_address& NewAddress, u32 StartPort )
{
    u_long dwBroadcast;
    if (net_Bind(NewAddress,StartPort))
    {
        dwBroadcast = TRUE;
        setsockopt(NewAddress.Socket,SOL_SOCKET,SO_BROADCAST,(char *)&dwBroadcast,sizeof(u_long));
        return TRUE;
    }
    return FALSE;
}

//==============================================================================

void    net_Unbind      ( net_address& Address )
{
    ASSERT( s_Inited );

    s_STAT_NAddressesBound--;

#ifdef WIN32
    closesocket( Address.Socket );
#endif
}

//==============================================================================

xbool   net_PCReceive   ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize )
{
    s32   RetSize;

    ASSERT( s_Inited );

#ifdef WIN32
    struct sockaddr_in sockfrom;
    int addrsize = sizeof(sockaddr_in);

    BufferSize = MAX_PACKET_SIZE;

    // receive any incoming packet
    RetSize = recvfrom( CallerAddress.Socket, 
                        (char*)pBuffer, 
                        BufferSize, 
                        0, 
                        (struct sockaddr *)&sockfrom, 
                        &addrsize );

    // if a packet was received
    if ( RetSize > 0 )
    {
        // fill out the "From" with the appropriate information
        SenderAddress.IP = sockfrom.sin_addr.s_addr;
        SenderAddress.Port = ntohs(sockfrom.sin_port);
        SenderAddress.Socket = BAD_SOCKET;
        ASSERT( RetSize <= BufferSize );
        BufferSize = RetSize;

        s_STAT_NPacketsReceived++;
        s_STAT_NBytesReceived += BufferSize;
        return TRUE;
    }
    else
    {
        BufferSize = 0;
    }

#endif

    // No packet received
    return FALSE;
}

//==============================================================================

void    net_PCSend      ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize )
{
    s32 status;
    ASSERT( s_Inited );

    s_STAT_NPacketsSent++;
    s_STAT_NBytesSent += BufferSize;

#ifdef WIN32
    struct sockaddr_in sockto;

    // address your package and stick a stamp on it :-)
    x_memset(&sockto,0,sizeof(struct sockaddr_in));
    sockto.sin_family = PF_INET;
    sockto.sin_port = htons(DestAddress.Port);
    sockto.sin_addr.s_addr = DestAddress.IP;
    status = sendto( CallerAddress.Socket, (const char*)pBuffer, BufferSize, 0, (struct sockaddr*)&sockto, sizeof(sockto) );
    if (status<=0)
    {
        x_DebugMsg("SendTo returned an error code %d\n",status);
    }
#endif
}

//==============================================================================

void    net_ClearStats      ( void )
{
#if 0
    s_STAT_NPacketsSent = 0;
    s_STAT_NPacketsReceived = 0;
    s_STAT_NBytesSent = 0;
    s_STAT_NBytesReceived = 0;
    s_STAT_NAddressesBound = 0;
    NET_SendTime.Reset();
    NET_ReceiveTime.Reset();
#endif
}

//==============================================================================

void    net_GetStats    ( u32& PacketsSent,
                          u64& BytesSent, 
                          u32& PacketsReceived,
                          u64& BytesReceived,
                          u32& NAddressesBound,
                          f32& SendTime,
                          f32& ReceiveTime )
{
    PacketsSent         = s_STAT_NPacketsSent;
    PacketsReceived     = s_STAT_NPacketsReceived;
    BytesSent           = s_STAT_NBytesSent;
    BytesReceived       = s_STAT_NBytesReceived;
    NAddressesBound     = s_STAT_NAddressesBound;
    SendTime = NET_SendTime.ReadMs();
    ReceiveTime = NET_ReceiveTime.ReadMs();
}

//==============================================================================


void net_GetInterfaceInfo(s32 id,interface_info &info)
{

    INTERFACE_INFO InterfaceList[8];
    unsigned long nBytesReturned;
    s32 status;
    SOCKET socket;
    s32 nInterfaces;

    info.Address    = 0;
    info.Broadcast  = 0;
    info.Nameserver = 0;
    info.Netmask    = 0;

    socket = WSASocket(AF_INET,SOCK_DGRAM,0,0,0,0);
    if (socket == SOCKET_ERROR)
    {
        return;
    }
    status = WSAIoctl(socket,SIO_GET_INTERFACE_LIST,0,0,&InterfaceList,sizeof(InterfaceList),&nBytesReturned,0,0);
    closesocket(socket);
    if (status == SOCKET_ERROR)
    {
        return;
    }
    
    nInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

    if (id<0)
    {
        for (id=0;id<nInterfaces;id++)
        {
            if ( (InterfaceList[id].iiFlags & IFF_LOOPBACK)==0 )
            {
                break;
            }
        }
        // 
        // if we can't find any appropriate interfaces, let's just use the
        // loopback if it's present.
        //
        if (id==nInterfaces)
        {
            id=0;
        }
    }
    info.Address    = InterfaceList[id].iiAddress.AddressIn.sin_addr.S_un.S_addr;
    info.Netmask    = InterfaceList[id].iiNetmask.AddressIn.sin_addr.S_un.S_addr;
    info.Broadcast  = (info.Address & info.Netmask) | ~info.Netmask;
}

u32     net_ResolveName( const char* pStr )
{
    LPHOSTENT hostent;
    struct in_addr in_addrIP;

    hostent = gethostbyname(pStr);

    if ( hostent == NULL )
        return 0;

    in_addrIP = *(struct in_addr *)(hostent->h_addr_list[0]);
    return in_addrIP.S_un.S_addr;
}

void    net_ResolveIP( u32 IP, char* pStr )
{
    struct in_addr in_addrIP;
    LPHOSTENT hostent;

    in_addrIP.S_un.S_addr = IP;
    hostent = gethostbyaddr((char FAR *)&in_addrIP,sizeof(struct in_addr),PF_INET);

    if (hostent==NULL)
    {
        *pStr = 0;
    }
    else
    {
        x_strcpy(pStr,hostent->h_name);
    }
}


void    net_ActivateConfig(xbool on)
{
    (void)on;
}

void	net_GetConnectStatus(connect_status &status)
{
	x_memset(&status,0,sizeof(connect_status));
}

s32 net_GetConfigList(const char *pPath,net_config_list *pConfigList)
{
    (void)pPath;
    x_memset(pConfigList,0,sizeof(net_config_list));
    return 0;
}

s32 net_SetConfiguration(const char *pPath,s32 configindex)
{
    (void)pPath;
    (void)configindex;
    return 0;
}

s32 net_GetAttachStatus(s32 &InterfaceId)
{
    InterfaceId = 0;
    return 0;
}

s32 net_GetSystemId(void)
{
	return 0;
}
