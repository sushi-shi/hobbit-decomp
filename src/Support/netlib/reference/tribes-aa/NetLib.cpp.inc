//=============================================================================
//
//  NETLIB.CPP
//
//=============================================================================
#ifdef WIN32
#include <winsock.h>
#endif

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_debug.hpp>
#include "NetLib.hpp"

//=============================================================================

static s32 s_STAT_NPacketsSent;
static s32 s_STAT_NPacketsReceived;
static s32 s_STAT_NBytesSent;
static s32 s_STAT_NBytesReceived;
static s32 s_STAT_NAddressesBound;
extern xtimer NET_SendTime;
extern xtimer NET_ReceiveTime;

static xbool s_Inited = FALSE;

//=============================================================================

void    net_Init        ( void )
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

//=============================================================================

void    net_Kill        ( void )
{
    ASSERT( s_Inited );
    s_Inited = FALSE;

#ifdef WIN32
    WSACleanup();
#endif
}

//=============================================================================

xbool   net_IsInited    ( void )
{
    return s_Inited;
}

//=============================================================================

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

//=============================================================================

void    net_Unbind      ( net_address& Address )
{
    ASSERT( s_Inited );

    s_STAT_NAddressesBound--;

#ifdef WIN32
    closesocket( Address.Socket );
#endif
}

//=============================================================================

xbool   net_Receive     ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize )
{
    s32   RetSize;

    ASSERT( s_Inited );

#ifdef WIN32
    struct sockaddr_in sockfrom;
    int addrsize = sizeof(sockaddr_in);

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
        SenderAddress.Socket = INVALID_SOCKET;
        ASSERT( RetSize <= BufferSize );
        BufferSize = RetSize;

        s_STAT_NPacketsReceived++;
        s_STAT_NBytesReceived += BufferSize;
        return TRUE;
    }

#endif

    // No packet received
    return FALSE;
}

//=============================================================================

void    net_Send        ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize )
{
    ASSERT( s_Inited );

    s_STAT_NPacketsSent++;
    s_STAT_NBytesSent += BufferSize;

#ifdef WIN32
    struct sockaddr_in sockto;

    // address your package and stick a stamp on it :-)
    sockto.sin_family = PF_INET;
    sockto.sin_port = htons(DestAddress.Port);
    sockto.sin_addr.s_addr = DestAddress.IP;
    sendto( CallerAddress.Socket, (const char*)pBuffer, BufferSize, 0, (struct sockaddr*)&sockto, sizeof(sockto) );
    
#endif
}

//=============================================================================

void    net_GetStats    ( s32& PacketsSent,
                          s32& BytesSent, 
                          s32& PacketsReceived,
                          s32& BytesReceived,
                          s32& NAddressesBound,
                          f32& SendTime,
                          f32& ReceiveTime )
{
	// It's no wonder I was getting bizarre stats! It's
	// just downright wrong in the original. But keep the
	// code the same if we're not doing a dedicated build
	PacketsSent			= s_STAT_NPacketsSent;
	PacketsReceived		= s_STAT_NPacketsReceived;
	BytesSent			= s_STAT_NBytesSent;
	BytesReceived		= s_STAT_NBytesReceived;
	NAddressesBound		= s_STAT_NAddressesBound;
    SendTime = NET_SendTime.ReadMs();
    ReceiveTime = NET_ReceiveTime.ReadMs();
}

//=============================================================================

void net_address::Clear( void )
{
    IP      = 0;
    Port    = 0; 
    Socket  = INVALID_SOCKET;
}

//=============================================================================

net_address::net_address( void )
{
    Clear();
}

//=============================================================================

net_address::net_address( u32 aIP, s32 aPort, u32 aSocket )
{
    IP = aIP;
    Port = aPort;
    Socket = aSocket;
}

//=============================================================================

net_address::net_address( char* aIP, s32 aPort, u32 aSocket )
{
    IP = net_StrToIP( aIP );
    Port = aPort;
    Socket = aSocket;
}

//=============================================================================

void net_address::GetStrIP( char* IPStr )
{
    net_IPToStr( IP, IPStr );
}

//=============================================================================

void net_address::SetStrIP( char* IPStr )
{
    IP = net_StrToIP( IPStr );
}

//=============================================================================

void net_address::GetStrAddress( char* Str )
{
    net_AddressToStr( IP, Port, Str );
}

//=============================================================================

void net_address::SetStrAddress( char* Str )
{
    net_StrToAddress( Str, IP, Port );
}

//=============================================================================

u32 net_StrToIP( char* pStr )
{
    u32 IP=0;

    for( s32 i=0; i<4; i++ )
    {
        u32 D=0;
        char C = *pStr++;
        while( (C>='0') && (C<='9') )
        {
            D = D*10 + (C-'0');
            C = *pStr++;
        }
        IP |= (D<<(i*8));
    }

    return IP;
}

//=============================================================================

void net_IPToStr( u32 IP, char* pStr )
{
    for( s32 i=0; i<4; i++ )
    {
        byte B = (IP>>(i*8))&0xFF;
        if( B>=100 ) *pStr++ = '0'+(B/100);
        if( B>= 10 ) *pStr++ = '0'+((B%100)/10);
        *pStr++ = '0'+(B%10);
        if( i<3 ) *pStr++ = '.';
    }
    *pStr = 0;
}

//=============================================================================

void net_StrToAddress( char* pStr, u32& IP, u32& Port )
{
    char C;

    IP=0;
    Port=0;

    // Read IP
    for( s32 i=0; i<4; i++ )
    {
        u32 D=0;
        C = *pStr++;
        while( (C>='0') && (C<='9') )
        {
            D = D*10 + (C-'0');
            C = *pStr++;
        }
        IP |= (D<<(i*8));
    }

    // Read port
    if( C == ':' ) 
    {
        C = *pStr++;
        while( (C>='0') && (C<='9') )
        {
            Port = Port*10 + (C-'0');
            C = *pStr++;
        }
    }
}

//=============================================================================

void net_AddressToStr( u32 IP, u32 Port, char* pStr )
{
    // Write address
    for( s32 i=0; i<4; i++ )
    {
        byte B = (IP>>(i*8))&0xFF;
        if( B>=100 ) *pStr++ = '0'+(B/100);
        if( B>= 10 ) *pStr++ = '0'+((B%100)/10);
        *pStr++ = '0'+(B%10);
        if( i<3 ) *pStr++ = '.';
    }

    // Write port
    *pStr++ = ':';
    if( Port>=10000 ) *pStr++ = '0'+((Port%100000)/10000);
    if( Port>= 1000 ) *pStr++ = '0'+((Port%10000)/1000);
    if( Port>=  100 ) *pStr++ = '0'+((Port%1000)/100);
    if( Port>=   10 ) *pStr++ = '0'+((Port%100)/10);
    if( Port>=    0 ) *pStr++ = '0'+((Port%10)/1);
    *pStr = 0;
}

//=============================================================================

int operator == (const net_address& A1, const net_address& A2)
{
    return ((A1.IP==A2.IP) && (A1.Port==A2.Port))?(1):(0);
}

//=============================================================================


