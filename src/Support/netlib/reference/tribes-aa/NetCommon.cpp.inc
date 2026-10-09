//=============================================================================
//
//  NETLIB.CPP
//
//=============================================================================
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_time.hpp>
#include "NetLib.hpp"
#include <xCore/x_files/x_memory.hpp>

#if defined(TARGET_PC)
#include <winsock.h>
#endif

#if defined(TARGET_LINUX)
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#endif

static random  Random;

//=============================================================================
//=============================================================================
//=============================================================================
//  INTERNET EMULATION
//=============================================================================
//=============================================================================
//=============================================================================

#if defined(athyssen)
//#define SHOW_ISETTINGS_ACTIONS
#endif

struct internet_settings
{
    s32     LatencyMs;
    s32     PercSendsLost;
    s32     PercReceivesLost;
    xbool   BlockSends;
    xbool   BlockReceives;
    xtimer  ReceiveDelay;
    xtimer  SendDelay;

    s32     PercPacketsSwapped;
    byte*   pPacketSwapBuffer;
    net_address PacketSwapCaller;
    net_address PacketSwapDest;
    s32         PacketSwapSize;
};

internet_settings ISettings;

struct net_buffer
{
    net_address     Caller;
    net_address     Dest;
    byte            Buffer[MAX_PACKET_SIZE];
    s32             BufferSize;
    xtimer          SendTimer;
};

#define MAX_BUFFERED_SENDS  10

net_buffer      SendBuffers[MAX_BUFFERED_SENDS];
s32             SendBufferTail;
s32             SendBufferHead;
s32             SendBufferValid;
/*
static stream_chunk* s_pBuffer = NULL;
static s32           s_FirstSendBuffer = -1;
static s32           s_LastSendBuffer = -1;
static s32           s_FirstRecvBuffer = -1;
static s32           s_LastRecvBuffer = -1;
static s32           s_FirstFreeBuffer = -1;
*/

#ifdef TARGET_PS2_DEV
s32 s_ReceiveSeconds[32];
s32 s_TransmitSeconds[32];
s32 s_ReceiveAverage;
s32 s_TransmitAverage;

s32 s_ReceiveData[32];
s32 s_TransmitData[32];
#endif
//=============================================================================

void    net_PS2Send     ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*         pBuffer, 
                          s32                 BufferSize );

void    net_PCSend      ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize );

void    net_LinuxSend   ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize );

xbool   net_PS2Receive  ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize );

xbool   net_PCReceive   ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize );

xbool   net_LinuxReceive( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize );

void    net_PCInit      ( void );
void    net_PCKill      ( void );
void    net_LinuxInit   ( void );
void    net_LinuxKill   ( void );
void    net_PS2Init     ( void );
void    net_PS2Kill     ( void );

xtimer  NET_SendTime;
xtimer  NET_ReceiveTime;
s32     NET_NSendPackets;
s32     NET_NRecvPackets;
s32     NET_NSendBytes;
s32     NET_NRecvBytes;

u16  net_Checksum(byte *pBuffer,s32 length)
{
    s32 i;
    u16 checksum;

    checksum = 0;
    for (i=0;i<length;i++)
    {
        checksum ^= (u8)*pBuffer++;
    }
    return checksum;
}

//=============================================================================

void net_Init(void)
{
    ISettings.BlockReceives = FALSE;
    ISettings.BlockSends = FALSE;
    ISettings.LatencyMs = 0;
    ISettings.PercReceivesLost = 0;
    ISettings.PercSendsLost = 0;

    SendBufferHead = 0;
    SendBufferTail = 0;
    SendBufferValid = 0;
    #ifdef TARGET_PS2
    net_PS2Init();
    #endif

    #ifdef TARGET_PC
    net_PCInit();
    #endif

    #ifdef TARGET_LINUX
    net_LinuxInit();
    #endif

#if defined(athyssen)
    //ISettings.PercReceivesLost = 10;
    //ISettings.PercSendsLost = 10;
#endif
}

//=============================================================================

void net_Kill(void)
{
    #ifdef TARGET_PS2
    net_PS2Kill();
    #endif

    #ifdef TARGET_PC
    net_PCKill();
    #endif

    #ifdef TARGET_LINUX
    net_LinuxKill();
    #endif
}

//=============================================================================
typedef struct s_receive_delay_buffer
{
    struct s_receive_delay_buffer *pNext;
    net_address         Sender;
    u32                 DestPort;
    s32                 Length;
    byte                Buffer[1024];
    xtimer              DelayTime;
    
} receive_delay_buffer;

static receive_delay_buffer *s_DelayBuffer=NULL;

xbool   net_Receive     ( net_address&  CallerAddress,
                          net_address&  SenderAddress,
                          void*         pBuffer,
                          s32&          BufferSize )
{
    receive_delay_buffer *pDelayBuffer,*pPrevDelayBuffer,*pOldBuffer;
    f32 time;

    ASSERT(CallerAddress.Socket);
    ASSERT(CallerAddress.Port);
    ASSERT(CallerAddress.IP);
    NET_ReceiveTime.Start();

    xbool Return;

    if (ISettings.LatencyMs)
    {
        // First check to see if we have anything in the delay buffer that has time expired
        pDelayBuffer = s_DelayBuffer;
        pPrevDelayBuffer = NULL;

        while (pDelayBuffer)
        {
            time = pDelayBuffer->DelayTime.ReadMs();
            if ( (pDelayBuffer->DestPort == CallerAddress.Port) && (time > ISettings.LatencyMs) )
            {
                if (pPrevDelayBuffer)
                {
                    pPrevDelayBuffer->pNext = pDelayBuffer->pNext;
                }
                else
                {
                    s_DelayBuffer = pDelayBuffer->pNext;
                }
                break;
            }
            else
            {
                // If something has been in the buffer for WAY too long, let's just discard it (10 seconds)
                if (time > 10000.0f)
                {
                    if (pPrevDelayBuffer)
                    {
                        pPrevDelayBuffer->pNext = pDelayBuffer->pNext;
                    }
                    else
                    {
                        s_DelayBuffer = pDelayBuffer->pNext;
                    }
                    pOldBuffer = pDelayBuffer;
                    pDelayBuffer = pDelayBuffer->pNext;
                    x_free(pOldBuffer);
                }
                else
                {
                    pPrevDelayBuffer = pDelayBuffer;
                    pDelayBuffer = pDelayBuffer->pNext;
                }
            }
        }
        //
        // If we have latency emulation going, let's put any new packets in the pending
        // buffer and leave them there.
        //
        #ifdef TARGET_PS2
        Return =  net_PS2Receive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        #ifdef TARGET_PC
        Return =  net_PCReceive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        #ifdef TARGET_LINUX
        Return =  net_LinuxReceive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        if (Return)
        {
            receive_delay_buffer *pDelay;

            pDelay = (receive_delay_buffer *)x_malloc(sizeof(receive_delay_buffer));
            ASSERTS(pDelay,"Too many requests got queued for too long");
            pDelay->Sender  = SenderAddress;
            pDelay->DestPort= CallerAddress.Port;
            pDelay->Length  = BufferSize;
            pDelay->pNext   = NULL;
            pDelay->DelayTime.Reset();
            pDelay->DelayTime.Start();
            x_memcpy(pDelay->Buffer,pBuffer,BufferSize);
            //
            // Insert this buffer in to the delay list at the end
            //
            if (s_DelayBuffer)
            {
                pPrevDelayBuffer = s_DelayBuffer;
                while (pPrevDelayBuffer->pNext)
                {
                    pPrevDelayBuffer = pPrevDelayBuffer->pNext;
                }
                pPrevDelayBuffer->pNext = pDelay;
            }
            else
            {
                s_DelayBuffer = pDelay;
            }
        }
        // If we had something in the delay buffer, we just return that for now.
        if (pDelayBuffer)
        {
//            x_DebugMsg("netlib: Delayed a packet for a total of %2.2fms\n",pDelayBuffer->DelayTime.ReadMs());
            SenderAddress = pDelayBuffer->Sender;
            x_memcpy(pBuffer,pDelayBuffer->Buffer,pDelayBuffer->Length);
            BufferSize = pDelayBuffer->Length;
            x_free(pDelayBuffer);
            Return = TRUE;
        }
        else
        {
            Return = FALSE;
        }
    }
    else
    {
        #ifdef TARGET_PS2
        Return =  net_PS2Receive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        #ifdef TARGET_PC
        Return =  net_PCReceive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        #ifdef TARGET_LINUX
        Return =  net_LinuxReceive( CallerAddress, SenderAddress, pBuffer, BufferSize );
        #endif

        if (!Return)
        {
            BufferSize = 0;
        }
    }

#ifdef TARGET_PS2_DEV
	s32 seconds;

	seconds = (s32)x_GetTimeSec();
	if (s_ReceiveSeconds[seconds%32] == seconds)
	{
		s_ReceiveData[seconds%32]+=BufferSize;
	}
	else
	{
		s_ReceiveData[seconds%32]=BufferSize;
		s_ReceiveSeconds[seconds%32]=seconds;
		s32 avg;
		avg=0;
		for (s32 i=0;i<32;i++)
		{
			avg += s_ReceiveData[i];
		}
		s_ReceiveAverage = avg / 32;
	}
#endif
    NET_ReceiveTime.Stop();

    if( ISettings.BlockReceives || (Random.irand(0,100) < ISettings.PercReceivesLost) )
    {
        #ifdef SHOW_ISETTINGS_ACTIONS
        x_DebugMsg("NetCommon: RECEIVE DROPPED\n");
        #endif
        return FALSE;
    }

    NET_NRecvBytes += BufferSize;
    if (Return)
        NET_NRecvPackets++;

    ASSERT( BufferSize <= MAX_PACKET_SIZE );
    //if( BufferSize > 0 )
        //x_DebugMsg("net::RECV %1d\n",BufferSize);
    return Return;
}


//=============================================================================
#ifdef athyssen
//xtimer SendTimer;
#endif

void    net_Send        ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress, 
                          const void*   pBuffer, 
                          s32           BufferSize )
{
    ASSERT( BufferSize <= MAX_PACKET_SIZE );
    ASSERT(CallerAddress.Socket);
    ASSERT(CallerAddress.IP);
    ASSERT(CallerAddress.Port);
    ASSERT(DestAddress.IP);
    ASSERT(DestAddress.Port);

    if( ISettings.BlockSends || (Random.irand(0,100) < ISettings.PercSendsLost) )
    {
        #ifdef SHOW_ISETTINGS_ACTIONS
        x_DebugMsg("NetCommon: SEND DROPPED\n");
        #endif
        return;
    }

    if( (ISettings.pPacketSwapBuffer==NULL) && (Random.irand(0,100) < ISettings.PercPacketsSwapped) )
    {
        ISettings.pPacketSwapBuffer = (byte*)x_malloc(BufferSize);
        x_memcpy(ISettings.pPacketSwapBuffer,pBuffer,BufferSize);

        ISettings.PacketSwapCaller = CallerAddress;
        ISettings.PacketSwapDest = DestAddress;
        ISettings.PacketSwapSize = BufferSize;
        return;
    }

#ifdef TARGET_PS2_DEV
	s32 seconds;

	seconds = (s32)x_GetTimeSec();
	if (s_TransmitSeconds[seconds%32] == seconds)
	{
		s_TransmitData[seconds%32]+=BufferSize;
	}
	else
	{
		s_TransmitData[seconds%32]=BufferSize;
		s_TransmitSeconds[seconds%32]=seconds;
		s32 avg;
		avg=0;
		for (s32 i=0;i<32;i++)
		{
			avg += s_TransmitData[i];
		}
		s_TransmitAverage = avg / 32;
	}
#endif


    NET_SendTime.Start();
    #ifdef TARGET_PS2
    net_PS2Send( CallerAddress, DestAddress, pBuffer, BufferSize );
    #endif
    #ifdef TARGET_PC
    net_PCSend( CallerAddress, DestAddress, pBuffer, BufferSize );
    #endif

    #ifdef TARGET_LINUX
    net_LinuxSend( CallerAddress, DestAddress, pBuffer, BufferSize );
    #endif
    NET_SendTime.Stop();
    NET_NSendBytes += BufferSize;
    NET_NSendPackets++;


    if( ISettings.pPacketSwapBuffer != NULL )
    {
        x_DebugMsg("!!!Sending Swapped Buffer!!!\n");
        NET_SendTime.Start();
        #ifdef TARGET_PS2
        net_PS2Send( ISettings.PacketSwapCaller, ISettings.PacketSwapDest, ISettings.pPacketSwapBuffer, ISettings.PacketSwapSize );
        #endif
        #ifdef TARGET_PC
        net_PCSend( ISettings.PacketSwapCaller, ISettings.PacketSwapDest, ISettings.pPacketSwapBuffer, ISettings.PacketSwapSize );
        #endif
        #ifdef TARGET_LINUX
        net_LinuxSend( CallerAddress, DestAddress, pBuffer, BufferSize );
        #endif
        NET_SendTime.Stop();
        NET_NSendBytes += BufferSize;
        NET_NSendPackets++;

        x_free(ISettings.pPacketSwapBuffer);
        ISettings.pPacketSwapBuffer = NULL;
    }
#ifdef athyssen
    //SendTimer.Start();
    //x_DebugMsg("net::SEND %04d %8.3f\n",NET_NSendPackets,SendTimer.ReadSec());
#endif
}

//=============================================================================

void net_address::Clear( void )
{
    IP      = 0;
    Port    = 0; 
    Socket  = BAD_SOCKET;
    Flags   = 0;
}

//=============================================================================

xbool net_address::IsEmpty( void )
{
    if( (IP==0) && (Port==0) )
        return TRUE;
    return FALSE;
}

//=============================================================================

net_address::net_address( void )
{
    Clear();
}

//=============================================================================

net_address::net_address( u32 aIP, s32 aPort, u32 aSocket )
{
    Clear();
    IP = aIP;
    Port = aPort;
    Socket = aSocket;
    Flags = 0;
}

//=============================================================================

net_address::net_address( char* aIP, s32 aPort, u32 aSocket )
{
    Clear();
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

void net_address::SetBlocking(xbool Block)
{
    if (Block)
        Flags |= NET_FLAGS_BLOCKING;
    else
        Flags &= ~NET_FLAGS_BLOCKING;
#if defined(TARGET_PC)
    u_long  dwNoBlock = !Block;
    ioctlsocket( Socket, FIONBIO, &dwNoBlock );
#endif

#if defined(TARGET_LINUX)
    u_long  dwNoBlock = !Block;
    ioctl( Socket, FIONBIO, &dwNoBlock );
#endif

}

xbool net_address::GetBlocking(void)
{
    return (Flags & NET_FLAGS_BLOCKING) != 0;
}

//=============================================================================

u32 net_StrToIP( const char* pStr )
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

void net_StrToAddress( const char* pStr, u32& IP, u32& Port )
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
                      *pStr++ = '0'+((Port%10)/1);
    *pStr = 0;
}
//=============================================================================

void net_AddressToStr( u32 IP, char* pStr )
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

    *pStr = 0;
}


//=============================================================================

int operator == (const net_address& A1, const net_address& A2)
{
    return ((A1.IP==A2.IP) && (A1.Port==A2.Port))?(1):(0);
}

//=============================================================================
// Packets are encrypted (or checksummed) as follows:
// The first 4 bytes are ignored. These are used as an identifier to see whether
// or not the packet is for the game manager or for the master server manager. The
// remaining data will be encrypted (or checksummed). The decryption routine assumes
// the same. Any data passed in of 4 bytes or less is assumed not to be encryptable
// or decryptable.
void    net_Encrypt         (byte *pBuffer,s32 &Length,s32 Seed)
{
    ASSERT(Length>=4);
    pBuffer[Length] = net_Checksum(pBuffer+4,Length-4)^Seed;
    Length++;
}


//=============================================================================
xbool   net_Decrypt         (byte *pBuffer,s32 &Length,s32 Seed)
{
    (void)pBuffer;
    (void)Length;
    (void)Seed;
/*
    xbool ok;

    if( Length > 800 )
    {
        s32 L = Length;
        x_DebugMsg("*****************************\n");
        for( s32 i=0; i<=32; i++ )
            x_DebugMsg("%02X ",(u32)((pBuffer)[L-32+i]));
        x_DebugMsg("\n");
    }

    if (Length < 5)
        return FALSE;

//    ok = (u8)pBuffer[Length-1] == (u8)(CalculateChecksum(pBuffer+4,Length-5)^Seed);
    ok = (u8)pBuffer[Length-1]==(u8)0xAA;
    if (ok)
    {
        Length--;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
*/
    return TRUE;
}
