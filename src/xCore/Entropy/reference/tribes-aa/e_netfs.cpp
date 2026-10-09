//==============================================================================
//
//  Network File System Implementation
//
//==============================================================================

//==============================================================================
//  CONTROL SWITCHES
//==============================================================================

//==============================================================================
//  DEFINES
//==============================================================================
#define FILESERVER_MAGIC 0x4afb0001
//==============================================================================
//  INCLUDES
//==============================================================================

#include "x_files.hpp"
#include "e_netfs.hpp"
#include "e_netfsdefs.hpp"
#include "netlib\netlib.hpp"
#include "tokenizer\tokenizer.hpp"
#include "entropy.hpp"
#include "x_threads.hpp"
#include "..\..\Demo1/SpecialVersion.hpp"

#ifdef TARGET_PS2
#include "ps2\iop\iop.hpp"
#else
#include <windows.h>
#endif

//==============================================================================
//  STRUCTURES
//==============================================================================
#define FSRV_BUFFER_SIZE    768
#define FSRV_BUFFER_THRESHOLD 256

typedef struct s_netfs_file
{
    s32 Type;
    s32 Magic;
    s32 Handle;
    s32 Position;
    s32 PositionOnHost;
    s32 Length;
    s32 BufferBase;
    s32 BufferLength;
    u8  Buffer[FSRV_BUFFER_SIZE];
} netfs_file;

//==============================================================================
//  VARIABLES
//==============================================================================

static open_fn*                 old_Open            = NULL;     // old filesystem functions
static close_fn*                old_Close           = NULL;
static read_fn*                 old_Read            = NULL;
static write_fn*                old_Write           = NULL;
static seek_fn*                 old_Seek            = NULL;
static tell_fn*                 old_Tell            = NULL;
static flush_fn*                old_Flush           = NULL;
static eof_fn*                  old_EOF             = NULL;
static length_fn*               old_Length          = NULL;

static xbool                    s_Initialized       = FALSE;    // TRUE after netfs_Init call   

static net_address              s_Server;                       // IP/Port of the server
static net_address              s_Client;                       // IP/Port of the client
static char                     s_Hostname[128];

//==============================================================================
//  HELPER FUNCTIONS
//==============================================================================
// These functions will send/recv packets with a 1/10th of a second timeout.
// Returns TRUE if the packet was processed properly, FALSE otherwise
static xbool SendPacket(fileserver_request &Request,s32 RequestLength);
static xbool GetPacket(fileserver_reply &Response,s32 ResponseLength);
static xbool SendSyncPacket(fileserver_request &Request,s32 RequestLength,fileserver_reply &Reply,s32 ReplyLength);
//==============================================================================
//  PROTOTYPES
//==============================================================================

static X_FILE* netfs_Open           ( const char* pPathName, const char* pMode );
static void    netfs_Close          ( X_FILE* pFile );
static s32     netfs_Read           ( X_FILE* pFile, byte* pBuffer, s32 Bytes );
static s32     netfs_Write          ( X_FILE* pFile, const byte* pBuffer, s32 Bytes );
static s32     netfs_Seek           ( X_FILE* pFile, s32 Offset, s32 Origin );
static s32     netfs_Tell           ( X_FILE* pFile );
static s32     netfs_Flush          ( X_FILE* pFile );
static xbool   netfs_EOF            ( X_FILE* pFile );
static s32     netfs_Length         ( X_FILE* pFile );

//==============================================================================
//  IMPLEMENTATION
//==============================================================================

void    netfs_Init(void)
{
    token_stream        Tok;
    char                iptext[64];
    fileserver_request  Request;
    fileserver_reply    Reply;
    interface_info      info;

    // Can only initialize once
    if (s_Initialized)
    {
        x_DebugMsg("netfs_Init: Attempt to initialize netfs again\n");
        return;
    }
    ASSERT(sizeof(fileserver_request)==1024);
    ASSERT(sizeof(fileserver_reply)==1024);

    while (1)
    {
        net_GetInterfaceInfo(-1,info);

        if (info.Address)
            break;
        x_printfxy(3,6,"Waiting for interface to attach...\n");
        eng_PageFlip();
    }
    net_Bind(s_Client);

    // We could not find the "serverip.txt" file so we do a broadcast to see if we
    // can find our fileserver.
    Request.Header.Type = FSRV_REQ_FIND;
    Request.Find.Address = s_Client.IP;
    Request.Find.Port    = s_Client.Port;
    s_Server.IP = (u32)-1;
    s_Server.Port = FILESERVER_PORT;
    x_printfxy(3,6,"Searching for a server.....\n");
    net_IPToStr(s_Client.IP,iptext);
    x_printfxy(3,7,"My address = %s:%d\n",iptext,s_Client.Port);
    eng_PageFlip();
    eng_PageFlip();
    SendSyncPacket(Request,sizeof(Request.Find),Reply,sizeof(Reply.Find));
    net_IPToStr(Reply.Find.Address,iptext);
    x_printfxy(1,7,"Server found at %s:%d\n",iptext,Reply.Find.Port);
    eng_PageFlip();
    eng_PageFlip();
    s_Server.IP = Reply.Find.Address;
    s_Server.Port = Reply.Find.Port;

    net_IPToStr(s_Server.IP,iptext);
    x_DebugMsg("Using host server (%s) IP address %s\n",s_Hostname,iptext);

    x_DebugMsg("Attempting to reset fileserver...\n");
    Request.Header.Type = FSRV_REQ_RESET;
    SendSyncPacket(Request,sizeof(Request.Reset),Reply,sizeof(Reply.Reset));
    x_DebugMsg("Fileserver reset complete.\n");
    // Set Initialized
    s_Initialized = TRUE;

    // Set IO hooks if we succeeded
    x_GetFileIOHooks(  old_Open,
                       old_Close,
                       old_Read,
                       old_Write,
                       old_Seek,
                       old_Tell,
                       old_Flush,
                       old_EOF,
                       old_Length );

    x_SetFileIOHooks(  netfs_Open,
                       netfs_Close,
                       netfs_Read,
                       netfs_Write,
                       netfs_Seek,
                       netfs_Tell,
                       netfs_Flush,
                       netfs_EOF,
                       netfs_Length );
}

//==============================================================================

void netfs_Kill( void )
{
    if( s_Initialized )
    {
        x_SetFileIOHooks(  old_Open,
                           old_Close,
                           old_Read,
                           old_Write,
                           old_Seek,
                           old_Tell,
                           old_Flush,
                           old_EOF,
                           old_Length );
        s_Initialized = FALSE;
    }
}

//==============================================================================
//  FILE I/O FUNCTIONS FOR NETFS
//==============================================================================

static X_FILE* netfs_Open( const char* pPathName, const char* pMode )
{
    fileserver_request  Request;
    fileserver_reply    Reply;
    netfs_file          *pOpenFile;

    Request.Header.Type = FSRV_REQ_OPEN;
    x_strcpy(Request.Open.Filename,pPathName);
    x_strcpy(Request.Open.Mode,pMode);

    SendSyncPacket(Request,sizeof(Request.Open),Reply,sizeof(Reply.Open));

    if (Reply.Open.Status == FSRV_ERR_OK)
    {
        pOpenFile = (netfs_file *)x_malloc(sizeof(netfs_file));
        ASSERT(pOpenFile);

        pOpenFile->Handle   = Reply.Open.Handle;
        pOpenFile->Length   = Reply.Open.Length;
        pOpenFile->Magic    = FILESERVER_MAGIC;
        pOpenFile->Position = 0;
        pOpenFile->PositionOnHost = 0;
        pOpenFile->Type     = FSRV_TYPE_LOCAL;
        pOpenFile->BufferBase = -1;
        pOpenFile->BufferLength = 0;

        return (X_FILE *)pOpenFile;
    }
    else if (old_Open)
    {
        X_FILE *Handle;

        Handle = old_Open(pPathName,pMode);
        if (Handle)
        {
            pOpenFile = (netfs_file *)x_malloc(sizeof(netfs_file));
            ASSERT(pOpenFile);
            pOpenFile->Handle   = (s32)Handle;
            pOpenFile->Type     = FSRV_TYPE_PASSTHRU;
            pOpenFile->Magic    = FILESERVER_MAGIC;
            pOpenFile->Position = 0;
            pOpenFile->PositionOnHost = 0;
            return (X_FILE *)pOpenFile;
        }
    }
    return NULL;
}

//==============================================================================

static void netfs_Close( X_FILE* pFile )
{
    netfs_file          *pOpenFile;
    fileserver_request  Request;
    fileserver_reply    Reply;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );
    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Close);
        old_Close((X_FILE *)pOpenFile->Handle);
        x_free(pOpenFile);
    }
    else
    {
        Request.Close.Handle = pOpenFile->Handle;
        Request.Header.Type = FSRV_REQ_CLOSE;
        SendSyncPacket(Request,sizeof(Request.Close),Reply,sizeof(Reply.Close));
        x_free(pOpenFile);
    }
}

//==============================================================================

typedef struct s_pending_request
{
    xbool               InUse;
    s32                 TimeoutDelay;
    fileserver_request  Request;
} pending_request;

static s32 netfs_Read( X_FILE* pFile, byte* pBuffer, s32 Bytes )
{
    netfs_file          *pOpenFile;
    s32                 BytesRead;
    s32                 OutstandingRequests;
    pending_request     *pRequestBlock;
    pending_request     *pRequest;
    s32                 BlockCount;
    s32                 Length;
    s32                 i;
    s32                 BasePosition;
    s32                 Status;
    s32                 BytesToRequest;
    fileserver_reply    Reply;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    BytesRead = 0;

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Read);
        return old_Read((X_FILE *)pOpenFile->Handle,pBuffer,Bytes);
    }
    else
    {
        xtimer timer;
        f32 time;
        f32 amount;

        if (Bytes < FSRV_BUFFER_THRESHOLD)
        {
            s32 offset;

            offset = pOpenFile->Position - pOpenFile->BufferBase;
            if ( (offset >= 0) && ( (offset + Bytes) < pOpenFile->BufferLength) )
            {
                x_memcpy(pBuffer,&pOpenFile->Buffer[pOpenFile->Position-pOpenFile->BufferBase],Bytes);
                pOpenFile->Position+=Bytes;
                return Bytes;
            }
            else
            {
                pRequest = (pending_request *)x_malloc(sizeof(pending_request));
                ASSERT(pRequest);
                pRequest->Request.Header.Type   = FSRV_REQ_READ;
                pRequest->Request.Read.Handle   = pOpenFile->Handle;
                pRequest->Request.Read.Position = pOpenFile->Position;
                pRequest->Request.Read.Length   = FSRV_BUFFER_SIZE;
                SendSyncPacket(pRequest->Request,sizeof(pRequest->Request.Read),Reply,sizeof(Reply.Read));
                pOpenFile->BufferBase = pOpenFile->Position;
                pOpenFile->BufferLength = Reply.Read.Length;
                if (Reply.Read.Length < Bytes)
                {
                    Bytes = Reply.Read.Length;
                }
                x_memcpy(pOpenFile->Buffer,Reply.Read.Data,Reply.Read.Length);
                x_memcpy(pBuffer,pOpenFile->Buffer,Bytes);
                pOpenFile->Position+=Bytes;
                x_free(pRequest);
                return Bytes;
            }
        }

        pOpenFile->BufferBase = -1;
        pOpenFile->BufferLength = 0;
        pRequestBlock = (pending_request *)x_malloc(sizeof(pending_request)*FILESERVER_CONCURRENT_REQUESTS);
        ASSERT(pRequestBlock);
        for (i=0;i<FILESERVER_CONCURRENT_REQUESTS;i++)
        {
            pRequestBlock[i].InUse = FALSE;
        }

        timer.Reset();
        timer.Start();
        BlockCount = (Bytes+FILESERVER_BLOCK_SIZE-1) / FILESERVER_BLOCK_SIZE;
        BytesToRequest = Bytes;
        OutstandingRequests = 0;
        BasePosition = pOpenFile->Position;

        while (Bytes)
        {
            //
            // Fill up the send requests and dispatch them to the iop
            //
            while (OutstandingRequests < FILESERVER_CONCURRENT_REQUESTS)
            {
                //
                // Do we have any more blocks left to be requested? Nope, just bail right now
                //
                if ( !BlockCount )
                    break;
                
                // 
                // Find a request block that's available (there should ALWAYS be one available at this point)
                //
                pRequest = NULL;
                for (i=0;i<FILESERVER_CONCURRENT_REQUESTS;i++)
                {
                    if (!pRequestBlock[i].InUse)
                    {
                        pRequest = &pRequestBlock[i];
                        break;
                    }
                }
                ASSERT(pRequest);

                Length = BytesToRequest;
                if (Length > FILESERVER_BLOCK_SIZE)
                {
                    Length = FILESERVER_BLOCK_SIZE;
                }

                pRequest->InUse                 = TRUE;
                pRequest->Request.Header.Type   = FSRV_REQ_READ;
                pRequest->Request.Read.Handle   = pOpenFile->Handle;
                pRequest->Request.Read.Position = pOpenFile->Position;
                pRequest->Request.Read.Length   = Length;
                pOpenFile->Position            += Length;
                SendPacket(pRequest->Request,sizeof(pRequest->Request.Read));
                BlockCount--;
                OutstandingRequests++;
                BytesToRequest -= Length;
            }

            if (OutstandingRequests)
            {
                Status = GetPacket(Reply,sizeof(Reply.Read));
                if (Status)
                {
                    // Find the packet request containing this sequence #
                    pRequest = NULL;
                    for (i=0;i<FILESERVER_CONCURRENT_REQUESTS;i++)
                    {
                        if ( pRequestBlock[i].InUse) 
                        {
                            if (pRequestBlock[i].Request.Header.Sequence == Reply.Header.Sequence)
                            {
                                pRequest = &pRequestBlock[i];
                                break;
                            }
                        }
                    }

                    if (pRequest)
                    {
                        x_memcpy(pBuffer+pRequest->Request.Read.Position - BasePosition,Reply.Read.Data,Reply.Read.Length);
                        pRequest->InUse  = FALSE;
                        BytesRead       += Reply.Read.Length;
                        Bytes           -= pRequest->Request.Read.Length;
                        OutstandingRequests--;
                        ASSERT(OutstandingRequests >=0 );
                        ASSERT(Bytes >= 0);
                    }
                    else
                    {
                        x_DebugMsg("netfs_Read: Reply received but discarded due to bad sequence #%08x\n",Reply.Header.Sequence);
                    }
                }
                else
                {
                    x_DebugMsg("netfs_Read: Timed out on connection to host. Resending all queued requests\n");
                    // We need to do a retry at this point since we didn't get ANY packets
                    // back on the outstanding requests. We should go through the outstanding
                    // request list and just do them again
                    for (i=0;i<FILESERVER_CONCURRENT_REQUESTS;i++)
                    {
                        if (pRequestBlock[i].InUse)
                        {
                            SendPacket(pRequestBlock[i].Request,sizeof(pRequestBlock[i].Request.Read));
                        }
                    }
                }
            }
            else
            {
                ASSERT(Bytes==0);
            }
        }

        timer.Stop();
        time = timer.ReadSec();
        amount = (f32)BytesRead / 1024.0f;
#if defined( bwatson ) || defined( RELEASE_CANDIDATE )
        x_DebugMsg("Read %d bytes in %2.4f seconds, %f Kbytes/sec\n",BytesRead,time,amount / time);
#endif
        x_free(pRequestBlock);
    }
    return BytesRead;
}

//==============================================================================

static s32 netfs_Write( X_FILE* pFile, const byte* pBuffer, s32 Bytes )
{
    netfs_file          *pOpenFile;
    fileserver_request  Request;
    fileserver_reply    Reply;
    s32                 BytesWritten;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );
    BytesWritten = 0;

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Write);
        return old_Write((X_FILE *)pOpenFile->Handle,pBuffer,Bytes);
    }
    else
    {
        while (Bytes)
        {
            s32 Length;

            Length = Bytes;
            if (Length > FILESERVER_BLOCK_SIZE)
            {
                Length = FILESERVER_BLOCK_SIZE;
            }
            Request.Header.Type      = FSRV_REQ_WRITE;
            Request.Write.Handle     = pOpenFile->Handle;
            Request.Write.Position   = pOpenFile->Position;
            Request.Write.Length     = Length;
            x_memcpy(Request.Write.Data,pBuffer,Length);
            SendSyncPacket(Request,sizeof(Request.Write),Reply,sizeof(Reply.Write));
            if (Reply.Write.Status != FSRV_ERR_OK)
                break;

            pBuffer                 += Length;
            Bytes                   -= Length;
            pOpenFile->Position     += Length;
            BytesWritten            += Length;
            if (pOpenFile->Position > pOpenFile->Length)
            {
                pOpenFile->Length = pOpenFile->Position;
            }
        }
    }
    return BytesWritten;
}

//==============================================================================

static s32 netfs_Seek( X_FILE* pFile, s32 Offset, s32 Origin )
{
    netfs_file *pOpenFile;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Seek);
        return old_Seek((X_FILE *)pOpenFile->Handle,Offset,Origin);
    }

    // Set new position
    switch( Origin )
    {
    case X_SEEK_SET:
        pOpenFile->Position = Offset;
        break;
    case X_SEEK_CUR:
        pOpenFile->Position += Offset;
        break;
    case X_SEEK_END:
        pOpenFile->Position = pOpenFile->Length - Offset;
        break;
    }

    // Limit position
    pOpenFile->Position = MAX( 0, pOpenFile->Position );
    pOpenFile->Position = MIN( pOpenFile->Position, pOpenFile->Length );

    // Success
    return 0;
}

//==============================================================================

static s32 netfs_Tell( X_FILE* pFile )
{
    netfs_file *pOpenFile;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Tell);
        return old_Tell((X_FILE *)pOpenFile->Handle);
    }

    return pOpenFile->Position;
}

//==============================================================================

static s32 netfs_Flush( X_FILE* pFile )
{
    netfs_file *pOpenFile;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Flush);
        return old_Flush((X_FILE *)pOpenFile->Handle);
    }

    ASSERTS( FALSE, "netfs_Flush: Unsupported operation on CD" );
    return TRUE;
}

//==============================================================================

static xbool netfs_EOF( X_FILE* pFile )
{
    netfs_file *pOpenFile;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_EOF);
        return old_EOF((X_FILE *)pOpenFile->Handle);
    }


    return pOpenFile->Position >= pOpenFile->Length;
}

//==============================================================================

static s32 netfs_Length( X_FILE* pFile )
{
    netfs_file *pOpenFile;

    pOpenFile = (netfs_file *)pFile;
    ASSERT(pOpenFile);
    ASSERT(pOpenFile->Magic == FILESERVER_MAGIC);
    ASSERT( s_Initialized );

    if (pOpenFile->Type == FSRV_TYPE_PASSTHRU)
    {
        ASSERT(old_Length);
        return old_Length((X_FILE *)pOpenFile->Handle);
    }
    return pOpenFile->Length;
}

//==============================================================================
//  HELPER FUNCTION BODIES
//==============================================================================
// This function will send a request to the server and wait for a response. There
// will, by default, be 3 retries and a 20ms wait for a response.
//
void net_DumpLog(void);
static xbool SendSyncPacket(fileserver_request &Request,s32 RequestLength,fileserver_reply &Reply,s32 ReplyLength)
{
    while(1)
    {
        xbool   status;
        s32     nRetries;
    
        SendPacket(Request,RequestLength);
        nRetries = FILESERVER_RETRIES;
        while (nRetries>0)
        {
            status = GetPacket(Reply,ReplyLength);

            if (status)
            {
                if (Reply.Header.Sequence == Request.Header.Sequence)
                {
                    return TRUE;
                }
                else
                {
                    x_DebugMsg("Packet received with bad sequence, got %04x, expected %04x\n",Reply.Header.Sequence,Request.Header.Sequence);
                }
            }
            else
            {
                x_DebugMsg("Server timed out. Sequence id %04x\n",Request.Header.Sequence);
                nRetries = 0;
            }
            nRetries--;
        }
#ifdef TARGET_PS2
#ifdef  X_DEBUG
        net_DumpLog();
#endif
#endif
    }
    return FALSE;
}

static xbool SendPacket(fileserver_request &Request,s32 RequestLength)
{
    s32     nRetries;
    static s32     TopSequence=0;

    nRetries = 0;
    // Have to include the size of the packet header. At the moment, this is
    // only 4 bytes but it will be bigger
    Request.Header.Sequence = TopSequence++;x_rand();
    net_Send(s_Client,s_Server,(u8 *)&Request,RequestLength+sizeof(Request.Header));
    return TRUE;
}

static xbool GetPacket(fileserver_reply &Response,s32 ResponseLength)
{
    s32     Timeout;
    s32     Length;
    xbool   Status;
        ;

    Timeout = FILESERVER_TIMEOUT;
    while (Timeout)
    {
        Length = ResponseLength+sizeof(Response.Header);
        Status = net_Receive(s_Client,s_Server,(u8 *)&Response,Length);
        Length -= sizeof(Response.Header);
        
        if ( Status )
        {
            return TRUE;
        }
        x_DelayThread(1);
        Timeout--;
    }
    return FALSE;
}

