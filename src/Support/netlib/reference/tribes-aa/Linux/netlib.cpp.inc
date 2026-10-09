//==============================================================================
//
//  NETLIB.CPP
//
//==============================================================================


#include <xCore/x_files/x_files.hpp>

#ifndef TARGET_LINUX
#error This should not be compiled for this target. It is a linux only file.
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <asm/errno.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "..\NetLib.hpp"

extern int errno;
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

void    net_LinuxInit        ( void )
{
    ASSERT( !s_Inited );
    s_Inited = TRUE;

    s_STAT_NPacketsSent     = 0;
    s_STAT_NPacketsReceived = 0;
    s_STAT_NBytesSent       = 0;
    s_STAT_NBytesReceived   = 0;
    s_STAT_NAddressesBound  = 0;
}

//==============================================================================

void    net_LinuxKill        ( void )
{
    ASSERT( s_Inited );
    s_Inited = FALSE;
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

    struct sockaddr_in addr;
    s32 sd_dg;

    // create an address and bind it
    x_memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(StartPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // create a socket
    sd_dg = socket( PF_INET, SOCK_DGRAM, 0 );

    // attempt to bind to the port
    while ( bind( sd_dg, (struct sockaddr *)&addr, sizeof(addr) ) < 0 )
    {
        // increment port if that port is assigned
        if ( errno == EADDRINUSE )
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
    ioctl( sd_dg, FIONBIO, &dwNoBlock );

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

    close( Address.Socket );
}

//==============================================================================

xbool   net_LinuxReceive   ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize )
{
    s32   RetSize;

    ASSERT( s_Inited );

    struct sockaddr_in sockfrom;
    socklen_t addrsize = sizeof(sockaddr_in);

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
		if (errno != EAGAIN)
		x_DebugMsg("Receive returned %d, errno=%d(%s)\n",RetSize,errno,strerror(errno));
        BufferSize = 0;
    }

    // No packet received
    return FALSE;
}

//==============================================================================

void    net_LinuxSend      ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize )
{
    s32 status;
    ASSERT( s_Inited );

    s_STAT_NPacketsSent++;
    s_STAT_NBytesSent += BufferSize;

    struct sockaddr_in sockto;

    // address your package and stick a stamp on it :-)
    x_memset(&sockto,0,sizeof(struct sockaddr_in));
    sockto.sin_family = PF_INET;
    sockto.sin_port = htons(DestAddress.Port);
    sockto.sin_addr.s_addr = DestAddress.IP;
    status = sendto( CallerAddress.Socket, (const char*)pBuffer, BufferSize, 0, (struct sockaddr*)&sockto, sizeof(sockto) );
    if (status<=0)
    {
        x_DebugMsg("SendTo returned an error code %d(%s)\n",errno,strerror(errno));
    }
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
	s32 fd;
	struct sockaddr_in *addr;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if	(fd < 0) 
	{
		ASSERTS(FALSE,"net_GetInterfaceInfo: Unable to create socket (no network?)\n");
		return;
	}

	x_strcpy(ifr.ifr_name, "eth0");

	if	(ioctl(fd, SIOCGIFADDR, (char *) &ifr) < 0) 
	{

		x_DebugMsg("ioctl SIOCGIFADDR failed,errno=%d(%s)\n",errno,strerror(errno));
		close(fd);
		return;
	}
	info.Address   = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

	if	(ioctl(fd, SIOCGIFBRDADDR, (char *) &ifr) < 0) 
	{
		x_DebugMsg("ioctl SIOCGIFBRDADDR failed,errno=%d(%s)\n",errno,strerror(errno));
		close(fd);
		return;
	}

	info.Broadcast = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

	if	(ioctl(fd, SIOCGIFNETMASK, (char *) &ifr) < 0) 
	{
		x_DebugMsg("ioctl SIOCGIFNETMASK failed,errno=%d(%s)\n",errno,strerror(errno));
		close(fd);
		return;
	}

	info.Netmask = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
	close(fd);
}

u32     net_ResolveName( const char* pStr )
{
    struct hostent* hostent;
    struct in_addr in_addrIP;

    hostent = gethostbyname(pStr);

    if ( hostent == NULL )
        return 0;

    in_addrIP = *(struct in_addr *)(hostent->h_addr);
    return in_addrIP.s_addr;
}

void    net_ResolveIP( u32 IP, char* pStr )
{
    struct in_addr in_addrIP;
    struct hostent* hostent;

    in_addrIP.s_addr = IP;
    hostent = gethostbyaddr(&in_addrIP,sizeof(struct in_addr),PF_INET);

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
