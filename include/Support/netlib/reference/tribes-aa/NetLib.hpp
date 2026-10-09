//==============================================================================
//
//  NETLIB.HPP
//
//==============================================================================

#ifndef NETLIB_HPP
#define NETLIB_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "x_types.hpp"

//==============================================================================
//  DEFINITIONS
//==============================================================================

#define STARTING_PORT   (8000)

#ifndef BAD_SOCKET
#define BAD_SOCKET      (u32)(~0)
#endif

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE (1024)
#endif

#define NET_FLAGS_BLOCKING          (1<<0)

//==============================================================================
//  TYPES
//==============================================================================

struct net_address
{

    u32 IP;
    u32 Port;
    u32 Socket;
    u32 Flags;

            net_address    ( void );
            net_address    ( u32   IP, s32 Port, u32 Socket = BAD_SOCKET );
            net_address    ( char* IP, s32 Port, u32 Socket = BAD_SOCKET );

    void    Clear          ( void );
    xbool   IsEmpty        ( void );
    void    GetStrIP       ( char* IPStr );
    void    GetStrAddress  ( char* Str );
    void    SetStrIP       ( char* IPStr );
    void    SetStrAddress  ( char* Str );
    void    SetBlocking    ( xbool Block);
    xbool   GetBlocking    ( void );

    friend int operator == ( const net_address& A1, const net_address& A2 );
};

//
// Mirrors a structure in netdefs.hpp in machine specific code block
//
enum {
        CONNECT_STATUS_IDLE = 0,
        CONNECT_STATUS_NOTPRESENT,
        CONNECT_STATUS_CONNECTED,
        CONNECT_STATUS_DISCONNECTED,
        CONNECT_STATUS_DIALING,
        CONNECT_STATUS_NEGOTIATING,
        CONNECT_STATUS_AUTHENTICATING,
        CONNECT_STATUS_RETRYING,
        CONNECT_STATUS_NODIALTONE,
        CONNECT_STATUS_TIMEDOUT,
};

enum {
    ATTACH_STATUS_IDLE  =0x0,       // This is not really a valid status but just used during init phase
    ATTACH_STATUS_ATTACHED,         // Interface has attached
    ATTACH_STATUS_DETACHED,         // Interface has detached
    ATTACH_STATUS_STARTED,          // Interface is started
    ATTACH_STATUS_STOPPED,          // Interface is stopped
    ATTACH_STATUS_ERROR,            // A general error occurred
    ATTACH_STATUS_NOCONFIG,         // No configuration available for this interface
    ATTACH_STATUS_CONFIGURED,       // Interface was successfully configured
    ATTACH_STATUS_UNKNOWN,          // An undefined event code was encountered
};


struct connect_status
{
    s32		nRetries;                       // Number of retries attempted so far
    s32		TimeoutRemaining;               // Amount of timeout remaining in seconds
    s32		Status;                         // Various flags showing status of connection
    s32		ConnectSpeed;                   // Speed at which we connected
	char	ErrorText[128];					// Error message
};

//==============================================================================

struct interface_info
{
    u32 Address;
    u32 Netmask;
    u32 Broadcast;
    u32 Nameserver;
};

//
// This should be identical to the config structure in iopnet_config.h
//
#define MAX_CONFIG_NAME_COUNT   8
#define MAX_CONFIG_NAME_LENGTH  128

typedef struct s_net_config_list
{
    s32     Count;
    s8      Available[MAX_CONFIG_NAME_COUNT];
    char    Name[MAX_CONFIG_NAME_COUNT][MAX_CONFIG_NAME_LENGTH];
} net_config_list;

//==============================================================================
//  FUNCTIONS
//==============================================================================

void    net_Init            ( void );
void    net_Kill            ( void );
xbool   net_IsInited        ( void );
                            
xbool   net_Bind            ( net_address& NewAddress, u32 StartPort=STARTING_PORT );
xbool   net_BindBroadcast   ( net_address& NewAddress, u32 StartPort=STARTING_PORT );
void    net_Unbind          ( net_address& Address );
                            
xbool   net_Receive         ( net_address&  CallerAddress,
                              net_address&  SenderAddress,
                              void*         pBuffer,
                              s32&          BufferSize );
                            
void    net_Send            ( const net_address&  CallerAddress, 
                              const net_address&  DestAddress, 
                              const void*         pBuffer, 
                              s32                 BufferSize );
                            
                          
void    net_ClearStats      ( void );
void    net_GetStats        ( s32& PacketsSent,
                              s32& BytesSent, 
                              s32& PacketsReceived,
                              s32& BytesReceived,
                              s32& NAddressesBound,
                              f32& SendTime,
                              f32& ReceiveTime);
                            
void    net_GetInterfaceInfo( s32 ID, interface_info& Info );
u32     net_StrToIP         ( const char* pStr );
void    net_StrToAddress    ( const char* pStr, u32& IP, u32& Port );
void    net_IPToStr         ( u32 IP, char* pStr );
void    net_AddressToStr    ( u32 IP, u32 Port, char* pStr );
void    net_AddressToStr    ( u32 IP, char* pStr );
void    net_ResolveIP       ( u32 IP, char* pStr );
u32     net_ResolveName     ( const char* pStr);
void    net_GetConnectStatus (connect_status &Status);
void    net_ActivateConfig  (xbool on);
s32     net_GetConfigList   (const char *pPath,net_config_list *pConfigList);
s32     net_SetConfiguration(const char *pPath,s32 configindex);
s32     net_GetAttachStatus (s32 &InterfaceId);
void    net_Encrypt         (byte *pBuffer,s32 &Length,s32 Seed);
xbool   net_Decrypt         (byte *pBuffer,s32 &Length,s32 Seed);
s32		net_GetSystemId		(void);
u16		net_Checksum(byte *pBuffer,s32 length);

//==============================================================================
#endif // NETLIB_HPP
//==============================================================================
