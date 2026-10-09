#include <xCore/x_files/x_files.hpp>
#include "..\netlib.hpp"
#include "ps2\iop\netdefs.hpp"
#include "ps2\iop\netvars.hpp"
#include "ps2\iop\iop.hpp"
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_threads.hpp>

#include <eekernel.h>
#include <libgraph.h>

#define VALID_PACKET_SIZE (MAX_PACKET_SIZE+4)

net_vars *g_Net=NULL;
extern xtimer NET_SendTime;
extern xtimer NET_ReceiveTime;

static monitored_port * FindPort(s32 port,monitored_port **pPrev = NULL);
void                    net_PeriodicSend(void);
void                    net_PeriodicRecv(void);
net_sendrecv_request *  AcquireBuffer(void);


#ifdef X_DEBUG
#define MAX_LOG_EVENTS  64

static xmesgq* s_pqEvents;

typedef struct s_iop_events
{
    char  Message[80];
    f32 TimeIndex;
} iop_event;

iop_event EventList[MAX_LOG_EVENTS];

void net_Log(const char *EventName)
{
    iop_event *pEvent;

    pEvent = (iop_event *)s_pqEvents->Recv(MQ_BLOCK);
    ASSERT(pEvent);
    x_strcpy(pEvent->Message,EventName);
    pEvent->TimeIndex = (x_GetTime() & 0xffffff); 
    s_pqEvents->Send(pEvent,MQ_BLOCK);
}

void net_DumpLog(void)
{
    s32 i;
    iop_event *pEvent;

    x_DebugMsg("--------------- NET EVENT DUMP --------------\n");
    for (i=0;i<MAX_LOG_EVENTS;i++)
    {
        pEvent = (iop_event *)s_pqEvents->Recv(MQ_BLOCK);
        x_DebugMsg("[%4.4f] %s\n",pEvent->TimeIndex,pEvent->Message);
        s_pqEvents->Send(pEvent,MQ_BLOCK);
    }
        x_CheckThreads(TRUE);
//    BREAK;
}
#else
#define net_Log(_a_)
#define net_DumpLog()
#endif

static net_sendrecv_request s_ReceiveBuffer PS2_ALIGNMENT(64);
//-----------------------------------------------------------------------------
void net_PS2Init(void)
{
    net_sendrecv_request *pRequest;
    s32                 i;

    //
    // if already initialized, just bail right now.
    //
    if (g_Net)
    {
        x_DebugMsg("net_PS2Init: Attempt to initialize network subsystem again\n");
        return;
    }
    g_Net = (net_vars *)x_malloc(sizeof(net_vars));

#ifdef X_DEBUG
    s_pqEvents = new xmesgq(MAX_LOG_EVENTS);
    for (i=0;i<MAX_LOG_EVENTS;i++)
    {
        s_pqEvents->Send(&EventList[i],MQ_BLOCK);
    }
#endif
    g_Net->m_pqAvailable = new xmesgq(MAX_PACKET_REQUESTS);
    g_Net->m_pqSendPending = new xmesgq( MAX_PACKET_REQUESTS);
    g_Net->m_pRequests      = (net_sendrecv_request *)x_malloc(sizeof(net_sendrecv_request)*MAX_PACKET_REQUESTS);
    g_Net->m_pPortList      = NULL;
    ASSERT(g_Net->m_pRequests);
    pRequest = g_Net->m_pRequests;
    for (i=0;i<MAX_PACKET_REQUESTS;i++)
    {
        g_Net->m_pqAvailable->Send(pRequest,MQ_BLOCK);
        pRequest++;
    }

    iop_SendSyncRequest(NETCMD_INIT);
    x_memset(g_Net->m_Info,0,sizeof(g_Net->m_Info));

    g_Net->m_pSendThread = new xthread(net_PeriodicSend,"Net Periodic Send",8192,1);
    g_Net->m_pRecvThread = new xthread(net_PeriodicRecv,"Net Periodic Recv",8192,1);

	x_DebugMsg("Net init reported a system id of 0x%08x\n",net_GetSystemId());
  

}

//-----------------------------------------------------------------------------
void net_PS2Kill(void)
{
    net_address addr;
    // If already killed, just exit now
    if (!g_Net)
        return;

    while (g_Net->m_pPortList)
    {
        x_DebugMsg("net_PS2Kill: WARNING: Port %d left open, closing\n",g_Net->m_pPortList->m_Port);
        addr.Port = g_Net->m_pPortList->m_Port;
        net_Unbind(addr);
    }
    iop_SendSyncRequest(NETCMD_KILL);
    delete g_Net->m_pSendThread;
    delete g_Net->m_pRecvThread;

    delete g_Net->m_pqAvailable;
    delete g_Net->m_pqSendPending;
#ifdef X_DEBUG
    delete s_pqEvents;
#endif
    x_free(g_Net->m_pRequests);
    x_free(g_Net);
    g_Net = NULL;
}

//-----------------------------------------------------------------------------
xbool   net_IsInited    ( void )
{
    return (g_Net!=NULL);
}

//-----------------------------------------------------------------------------
void    net_GetConnectStatus (connect_status &Status)
{
    net_connect_status     DialStatus;

    ASSERT(g_Net);

    iop_SendSyncRequest(NETCMD_GETCONNECTSTATUS,NULL,0,&DialStatus,sizeof(net_connect_status));
    Status.nRetries         = DialStatus.nRetries;
    Status.Status           = DialStatus.Status;
    Status.ConnectSpeed     = DialStatus.ConnectSpeed;
    Status.TimeoutRemaining = DialStatus.TimeoutRemaining;
	x_strcpy(Status.ErrorText,DialStatus.ErrorText);
}

//-----------------------------------------------------------------------------
void net_GetInterfaceInfo(s32 id,interface_info &info)
{
    ASSERT(g_Net);

    if (id==-1)
    {
        for (id=0;id<MAX_NET_DEVICES;id++)
        {
            if (g_Net->m_Info[id].Address)
            {
                info = g_Net->m_Info[id];
				if (info.Address)
				{
					return;
				}
            }
        }
        id=0;
    }
    info = g_Net->m_Info[id];

//    iop_SendSyncRequest(NETCMD_READCONFIG,&id,sizeof(s32),&info,sizeof(interface_info));
}

//-----------------------------------------------------------------------------
xbool   net_Bind        ( net_address& NewAddress, u32 StartPort=STARTING_PORT )
{
    u32 port;
    s32 status[2];
    monitored_port *pPort;

    port = StartPort;

    g_Net->m_BindBusy = TRUE;
    while (1)
    {
        iop_SendSyncRequest(NETCMD_BIND,&port,sizeof(s32),status,sizeof(status));
        if (status[0] != -1)
            break;
        port++;
        ASSERT(port < StartPort + 256);
    }
    NewAddress.Port = port;
    NewAddress.Socket = status[0];
    NewAddress.IP = status[1];
    if (NewAddress.Socket==0xFFFFFFFF)
    {
        g_Net->m_BindBusy = FALSE;
        return FALSE;
    }

    pPort = (monitored_port *)x_malloc(sizeof(monitored_port));
    ASSERT(pPort);

    pPort->m_pqPending = new xmesgq(MAX_PACKET_REQUESTS);

    pPort->m_Address = NewAddress.IP;
    pPort->m_Port    = NewAddress.Port;
    pPort->m_pNext = g_Net->m_pPortList;
    g_Net->m_pPortList = pPort;

    //x_DebugMsg("Bound port %d, sock=%08x, ip=%08x\n",NewAddress.Port,NewAddress.Socket,NewAddress.IP);
    g_Net->m_NAddressesBound++;
    // If we get a udp socket number of -1, this means
    // we didn't get anything bound properly.
    g_Net->m_BindBusy = FALSE;
    x_DebugMsg("net_Bind: %d bytes free on iop\n",iop_GetFreeMemory());
    return( NewAddress.Socket != 0xFFFFFFFF );
}

//-----------------------------------------------------------------------------
xbool    net_BindBroadcast( net_address& NewAddress, u32 StartPort )
{
    return net_Bind(NewAddress,StartPort);
}

//-----------------------------------------------------------------------------
void    net_Unbind      ( net_address& Address )
{
    u32 status;
    monitored_port *pPort,*pPrev;

    // We may be making multiple calls so let's keep the request
    // block around for re-use.
    pPort = FindPort(Address.Port,&pPrev);
    if (!pPort)
    {
        x_DebugMsg("net_Unbind: Attempt to close port %d when not open\n",Address.Port);
        return;
    }

    if (pPrev)
    {
        pPrev->m_pNext = pPort->m_pNext;
    }
    else
    {
        g_Net->m_pPortList = pPort->m_pNext;
    }

    g_Net->m_NAddressesBound--;

    delete pPort->m_pqPending;
    iop_SendSyncRequest(NETCMD_UNBIND,&Address.Socket,sizeof(s32),&status,sizeof(s32));
    Address.Port = 0xFFFFFFFF;
    Address.Socket = 0;
    x_free(pPort);
}

s32 net_receive_count=0;
//-----------------------------------------------------------------------------
xbool   net_PS2Receive  ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize )
{
    net_sendrecv_request *pPacket;
    monitored_port *pPort;

    ASSERT(CallerAddress.Socket);
    ASSERT(CallerAddress.Port);
    ASSERT(CallerAddress.IP);

    net_receive_count++;

    pPort = FindPort(CallerAddress.Port);

    if (pPort)
    {
        pPort->m_Flags = CallerAddress.Flags;
        pPacket=(net_sendrecv_request *)pPort->m_pqPending->Recv(MQ_NOBLOCK);
        if (pPacket)
        {
#ifdef X_DEBUG
            u32 *pData;
            pData = (u32 *)pPacket->m_Data;
            net_Log(xfs("Receive from %08x:%d,data=%08x:%08x",pPacket->m_Header.Address,pPacket->m_Header.Port,*pData++,*pData++));
#endif
            if (pPacket->m_Header.Length > VALID_PACKET_SIZE)
            {
                x_DebugMsg("netmgr: Packet from port %d, too big, truncated\n",pPort->m_Port);
                pPacket->m_Header.Length = VALID_PACKET_SIZE;
            }
            BufferSize = pPacket->m_Header.Length;
            x_memcpy(pBuffer,pPacket->m_Data,BufferSize);
            SenderAddress.IP = pPacket->m_Header.Address;
            SenderAddress.Port = pPacket->m_Header.Port;
            SenderAddress.Socket = pPacket->m_Header.Socket;
            g_Net->m_PacketsReceived++;
            g_Net->m_BytesReceived += pPacket->m_Header.Length;
            g_Net->m_pqAvailable->Send(pPacket,MQ_BLOCK);
            return TRUE;
        }
        else
        {
            //net_Log(xfs("Receive but no packet,port = %d",CallerAddress.Port));
            BufferSize=0;
            return FALSE;
        }
    }

    ASSERTS(g_Net->m_BindBusy,"Attempted to read an unopened port");
    return FALSE;
}

//-----------------------------------------------------------------------------
void    net_PS2Send     ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*         pBuffer, 
                          s32                 BufferSize )
{
    net_sendrecv_request *pPacket;

    // Get a packet that we need to send data with
    pPacket = (net_sendrecv_request *)g_Net->m_pqAvailable->Recv(MQ_BLOCK);
    ASSERT(pPacket);

    ASSERT(BufferSize <= VALID_PACKET_SIZE);
    x_memcpy(pPacket->m_Data,pBuffer,BufferSize);
    // Set up the header to be sent to the IOP
    pPacket->m_Header.Address = DestAddress.IP;
    pPacket->m_Header.Port    = DestAddress.Port;
    pPacket->m_Header.Socket  = CallerAddress.Socket;
    pPacket->m_Header.Length  = BufferSize;
#ifdef X_DEBUG
    u32 *pData;
    pData = (u32 *)pPacket->m_Data;
    net_Log(xfs("Send to %08x:%d,data=%08x:%08x",pPacket->m_Header.Address,pPacket->m_Header.Port,*pData++,*pData++));
#endif
    g_Net->m_pqSendPending->Send(pPacket,MQ_BLOCK);
    g_Net->m_PacketsSent++;
    g_Net->m_BytesSent += BufferSize;
}

//-----------------------------------------------------------------------------
void    net_ClearStats      ( void )
{
#if 0
    g_Net->m_PacketsSent = 0;
    g_Net->m_BytesSent = 0;
    g_Net->m_PacketsReceived = 0;
    g_Net->m_BytesReceived = 0;
    g_Net->m_NAddressesBound = 0;
    NET_SendTime.Reset();
    NET_ReceiveTime.Reset();
#endif
}


//-----------------------------------------------------------------------------
void    net_GetStats    ( s32& PacketsSent,
                          s32& BytesSent, 
                          s32& PacketsReceived,
                          s32& BytesReceived,
                          s32& NAddressesBound,
                          f32& SendTime,
                          f32& ReceiveTime )
{
    PacketsSent = g_Net->m_PacketsSent;
    BytesSent = g_Net->m_BytesSent;
    PacketsReceived = g_Net->m_PacketsReceived;
    BytesReceived = g_Net->m_BytesReceived;
    NAddressesBound = g_Net->m_NAddressesBound;
    SendTime = NET_SendTime.ReadMs();
    ReceiveTime = NET_ReceiveTime.ReadMs();

}

//-----------------------------------------------------------------------------
u32     net_ResolveName( char const * pStr )
{
    u32 ipaddr;

    net_Log(xfs("Resolve name %s",pStr));
    iop_SendSyncRequest(NETCMD_RESOLVENAME,(void *)pStr,x_strlen(pStr)+1,&ipaddr,sizeof(s32));

    return ipaddr;
}

//-----------------------------------------------------------------------------
void    net_ResolveIP( u32 IP, char* pStr )
{
    net_Log(xfs("Resolve ID %08x",IP));
    iop_SendSyncRequest(NETCMD_RESOLVEIP,&IP,sizeof(u32),pStr,MAX_NAME_LENGTH);
}


//-----------------------------------------------------------------------------
//------------- Utility functions, just internal
//-----------------------------------------------------------------------------
static monitored_port *FindPort(s32 port,monitored_port **pPrev)
{
    monitored_port *pPort,*pPrevPort;

    pPort=g_Net->m_pPortList;
    pPrevPort = NULL;

    while (pPort)
    {
        if (pPort->m_Port == port)
        {
            if (pPrev)
            {
                *pPrev = pPrevPort;
            }
            return pPort;
        }
        pPrevPort = pPort;
        pPort = pPort->m_pNext;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
net_sendrecv_request *AcquireBuffer(void)
{
    net_sendrecv_request *pRequest;
    monitored_port       *pPort,*pLongestPort;
    s32                   nRequests;
    s32                 status;
	s32					i;
	pRequest = NULL;

    xtimer t;

    t.Start();
	pRequest = (net_sendrecv_request *)g_Net->m_pqAvailable->Recv(MQ_NOBLOCK);
    pPort = g_Net->m_pPortList;
    nRequests = 0;
    pLongestPort = NULL;
    if (pRequest)
        return pRequest;

    while (1)
    {
        if (pPort)
        {
            if (pPort->m_pqPending->ValidEntries() >= nRequests)
            {
                nRequests = pPort->m_pqPending->ValidEntries();
                pLongestPort = pPort;
            }
        }
        else
        {
            break;
        }
        pPort = pPort->m_pNext;
    }

    if ( pLongestPort && nRequests )
    {
        // We're going to steal from a port queue. We continue stealing until
        // the number of entries in the port queue fall below the threshold for
        // a block purge
        nRequests = 0;
        while (1)
        {
            pRequest = (net_sendrecv_request *)pLongestPort->m_pqPending->Recv(MQ_NOBLOCK);
            ASSERT(pRequest);

            nRequests++;

            status = g_Net->m_pqAvailable->Send(pRequest,MQ_NOBLOCK);
            ASSERT(status);

            if (pLongestPort->m_pqPending->ValidEntries() <= PACKET_PURGE_THRESHOLD)
                break;
        }
        pRequest = (net_sendrecv_request *)g_Net->m_pqAvailable->Recv(MQ_BLOCK);
        ASSERT(pRequest);
        t.Stop();
        x_DebugMsg("netlib: Ran out of buffers, purged %d requests from port %d, delay %2.2fms\n",nRequests,pLongestPort->m_Port,t.ReadMs());
    }
    else
    {
        //
        // We stole from the send queue. This *SHOULD* never happen since the send queue should always be
        // relatively short.
        //
        pRequest = (net_sendrecv_request *)g_Net->m_pqSendPending->Recv(MQ_NOBLOCK);
        ASSERT(pRequest);
        x_DebugMsg("netlib: Ran out of buffers, stole from send queue, addr=%08x,port=%d\n",pRequest->m_Header.Address,pRequest->m_Header.Port);
    }

    return pRequest;
}
//-----------------------------------------------------------------------------
//------------- Threads, just internal
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void *net_RecvCallback(u32 command,void *data,s32 size)
{
    net_sendrecv_request *pRequest;
    net_sendrecv_request *pPacket;
    monitored_port       *pPort;
    xbool                status;
    s32                  i;

    (void)command;
    (void)size;

    FlushCache(0);

    // If we issue this command, it's a interface_info packet coming in
    if (command == INEV_NET_NEWIFC_CMD)
    {
        interface_info *pInfo;

        pInfo = (interface_info *)data;
        for (i=0;i<MAX_NET_DEVICES;i++)
        {
            g_Net->m_Info[i] = pInfo[i];
        }
        return data;
    }

    ASSERT(command == INEV_NET_RECV_CMD);
    pPacket = (net_sendrecv_request *)data;

    pPort = FindPort(pPacket->m_Header.LocalPort);
    //ASSERTS(pPort,"Receive attempted on unmonitored port\n");
    if (pPort)
    {
        pRequest = AcquireBuffer();
        ASSERT(pRequest);
        x_memcpy(pRequest,pPacket,sizeof(net_sendrecv_header)+pPacket->m_Header.Length);
        status = pPort->m_pqPending->Send(pRequest,MQ_NOBLOCK);
        ASSERT(status);
    }
#ifdef bwatson
    x_memset(data,0xc9,sizeof(net_sendrecv_request));
#endif
    FlushCache(0);
    return data;

}

sceSifQueueData RecvQueueData PS2_ALIGNMENT(64);
sceSifServeData RecvServerData PS2_ALIGNMENT(64);

void net_PeriodicRecv(void)
{

    sceSifSetRpcQueue(&RecvQueueData,GetThreadId());
    sceSifRegisterRpc(&RecvServerData,INEV_NET_RECV_DEV,net_RecvCallback,&s_ReceiveBuffer,NULL,NULL,&RecvQueueData);
    sceSifRpcLoop(&RecvQueueData);
    ASSERT(FALSE);
}
f32 net_SendDelay;
sceSifQueueData s_SendQueueData PS2_ALIGNMENT(64);
sceSifClientData s_SendClientData PS2_ALIGNMENT(64);
//-----------------------------------------------------------------------------
void net_PeriodicSend(void)
{
    net_sendrecv_request *pRequest;
    xtimer t;
    s32 result;

    while (1) 
    {
	    result = sceSifBindRpc (&s_SendClientData, INEV_NET_SEND_DEV, 0);
        ASSERTS(result>=0,"error: sceSifBindRpc for PeriodicSend failed");

	    if (s_SendClientData.serve != 0) break;
    }
    x_DebugMsg("Send init complete\n");

    while (1)
    {
        pRequest = (net_sendrecv_request *)g_Net->m_pqSendPending->Recv(MQ_BLOCK);
        ASSERT(pRequest);

#ifdef X_DEBUG
        u32 *pData;

        pData = (u32 *)pRequest->m_Data;
        net_Log(xfs("Periodic Send, to %08x:%d, data=%08x:%08x",pRequest->m_Header.Address,pRequest->m_Header.Port,*pData++,*pData++));
#endif
        t.Reset();
        t.Start();
        result = sceSifCallRpc (&s_SendClientData,  
                INEV_NET_SEND_CMD,0,
                pRequest,sizeof(net_sendrecv_header)+pRequest->m_Header.Length,
                &pRequest->m_Header,sizeof(net_sendrecv_header),
                NULL,NULL);
		ASSERT(result==0);
        g_Net->m_pqAvailable->Send(pRequest,MQ_BLOCK);
        t.Stop();
        net_SendDelay = t.ReadMs();
    }
    ASSERT(FALSE);
}

s32 net_GetConfigList(const char *pPath,net_config_list *pConfigList)
{
    s32 error;

    error = 0;
    iop_SendSyncRequest(NETCMD_GETCONFIGLIST,(void *)pPath,x_strlen(pPath)+1,pConfigList,sizeof(net_config_list));
    if (pConfigList->Count < 0)
    {
        error = pConfigList->Count; 
        pConfigList->Count = 0;
    }
    return error;
}

s32 net_GetAttachStatus(s32 &InterfaceId)
{
    s32 status;

    iop_SendSyncRequest(NETCMD_GET_ATTACH_STATUS,NULL,0,&status,sizeof(status));
    InterfaceId = (status >> 24);
    return status & (0x00ffffff);
}

s32  net_SetConfiguration(const char *pPath,s32 configindex)
{
    char Buffer[128];
    s32 status;
    xtimer t;

    x_memcpy(Buffer,&configindex,4);
    x_strcpy(Buffer+4,pPath);
    iop_SendSyncRequest(NETCMD_SETCONFIGURATION,Buffer,sizeof(Buffer),&status,sizeof(status));

    t.Reset();
    t.Start();

    while(1)
    {
        iop_SendSyncRequest(NETCMD_GET_SETCONFIG_STATUS,NULL,0,&status,sizeof(status));
        if (status!=1)
            break;
        x_DelayThread(2);
        if (t.ReadMs() > 8000)
        {
            ASSERT(FALSE);
            status = -1;
            break;
        }
    }
    return status;
}

//-----------------------------------------------------------------------------
void    net_ActivateConfig(xbool on)
{
    xtimer t;
    s32 status;

    iop_SendSyncRequest(NETCMD_ACTIVATECONFIG,&on,sizeof(on),NULL,0);
    t.Reset();
    t.Start();
    while(1)
    {
        iop_SendSyncRequest(NETCMD_GET_ACTIVATE_CONFIG_STATUS,NULL,0,&status,sizeof(status));
        if (status != 1)
            break;
        x_DelayThread(2);
        if (t.ReadMs() > 10000)
        {
            ASSERT(FALSE);
            break;
        }
    }
}

s32 net_GetSystemId(void)
{
	s32 sysid;

    iop_SendSyncRequest(NETCMD_GET_SYSID,NULL,0,&sysid,sizeof(sysid));
	return sysid;
}


