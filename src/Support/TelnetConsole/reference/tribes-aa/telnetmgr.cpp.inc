#include <xCore/x_files/x_files.hpp>
#include "telnetmgr.hpp"
#include "telnetclient.hpp"
#include "specialversion.hpp"
#include "fe_globals.hpp"
#include "gameserver.hpp"
#include "chatclient.hpp"


telnet_manager g_TelnetManager;
// We only want to do this on the PC since we'll be using some additional windows specific calls to
// deal with a tcp port connection
#if !defined(TARGET_PC)  && !defined(TARGET_LINUX)

#error This is a PC only file. Please exclude it from the build rules

#endif

#if defined(TARGET_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/errno.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <unistd.h>

#elif defined(TARGET_PC)
#else

#error This is a PC only file. Please exclude it from the build rules
#endif

//-----------------------------------------------------------------------------
telnet_manager::telnet_manager(void)
{
	x_memset(this,0,sizeof(this));
	m_TelnetPort.Clear();
}
extern int errno;
xbool OpenTcpPort(net_address& Address, const net_address& Host)
{
	// Open the telnet port. TCP packet type
    struct sockaddr_in addr;
#if defined(TARGET_PC)
    SOCKET sd_dg;
#else
    u32    sd_dg;
#endif
	Address.IP		= Host.IP;
	Address.Port	= Host.Port;

    // create a socket
    sd_dg = socket( PF_INET, SOCK_STREAM, 0 );

    // create an address and bind it
    x_memset(&addr,0,sizeof(struct sockaddr_in));



    // attempt to bind to the port
    s32 error;
    while ( 1 )
    {
		addr.sin_family = PF_INET;
		if (Address.IP != INADDR_ANY)
		{
			addr.sin_port = htons(0);
			addr.sin_addr.s_addr = htonl(0);
		}
		else
		{
			addr.sin_port = htons(Address.Port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
		}

		error = bind( sd_dg, (struct sockaddr *)&addr, sizeof(addr) );

		if (error >= 0)
			break;
        // increment port if that port is assigned
#if defined(TARGET_PC)
        if ( WSAGetLastError() == WSAEADDRINUSE )
#else
        if ( errno == EADDRINUSE )
#endif
        {
            Address.Port++;
            addr.sin_port = htons( Address.Port );
        }
        // if some other error, nothing we can do...abort
        else
        {
            return FALSE;
        }
    }

	if (Address.IP != INADDR_ANY)
	{
		addr.sin_port = htons(Address.Port);
		addr.sin_addr.s_addr = Address.IP;
		error = connect( sd_dg, (struct sockaddr *)&addr, sizeof(addr) );
		if (error < 0)
		{
#if defined(TARGET_PC)
			closesocket(sd_dg);
#else
			close(sd_dg);
#endif
			return FALSE;
		}
	}
    // set this socket to non-blocking, so we can poll it
    u_long  dwNoBlock = TRUE;
#if defined(TARGET_PC)
    ioctlsocket( sd_dg, FIONBIO, &dwNoBlock );
#else
    ioctl( sd_dg, FIONBIO, &dwNoBlock );
#endif
    // fill out the slot structure
    // first, determine local IP address
    char tbuf[128];
    hostent* pHe;
    struct in_addr in_IP;

    gethostname( tbuf, 127 );
    pHe = gethostbyname( tbuf );

    in_IP = *(struct in_addr *)(pHe->h_addr);

    Address.IP = in_IP.s_addr;
    Address.Socket = (u32)sd_dg;
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void telnet_manager::Init(void)
{
	OpenTcpPort(m_TelnetPort,net_address(INADDR_ANY,TELNET_LISTEN_PORT));
	listen(m_TelnetPort.Socket,TELNET_MAX_CLIENTS);

#ifdef ENABLE_KEYING
	if (!ValidateAccess())
	{
		x_printf("FATAL ERROR: This is an unauthorized or very out of date\n"
				 "version of Tribes:AADS. If you wish to run a dedicated\n"
				 "server for Tribes:AA please email aads@tribesaa.com.\n"
				 "You will need a static IP address and a fast internet\n"
				 "connection (T1 or greater) that is not behind a firewall.\n");
		exit(-1);
	}
#endif

#if 0
	m_Clients[0]=new telnet_client;

	net_address Dummy;

	m_Clients[0]->Init(Dummy);
#endif
	char ipstr[64];
	net_IPToStr(m_TelnetPort.IP,ipstr);
	x_printf("Telnet Control Console initialized at address %s:%d\n",ipstr,m_TelnetPort.Port);

	x_printf("Version number %s\n",VERSION_NUMBER);

	g_ChatManager.Init();
	// If we have no default channel name for IRC, then we use the server name but we
	// remove all the wide character stuff to make it work with IRC
	char ServerName[64];
	MakeNarrowString(ServerName,fegl.ServerSettings.ServerName);

	if (x_strlen(g_ChatManager.GetChannel())==0)
	{
		g_ChatManager.SetChannel(ServerName);
	}

	if (x_strlen(g_ChatManager.GetNick())==0)
	{
		g_ChatManager.SetNick(g_ChatManager.GetChannel());
	}
}

//-----------------------------------------------------------------------------
void telnet_manager::Kill(void)
{
	s32 i;

	for (i=0;i<TELNET_MAX_CLIENTS;i++)
	{
		if (m_Clients[i])
		{
			m_Clients[i]->Kill();
			delete m_Clients[i];
			m_Clients[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
void telnet_manager::Update(f32 DeltaTime)
{
	s32 i;
    struct sockaddr addr;
	s32 size;
	net_address incoming;


	// Has a new connection been established?
	size = sizeof(addr);
#if defined(TARGET_LINUX)
    incoming.Socket = accept(m_TelnetPort.Socket,&addr,(u32*)&size);
#else
    incoming.Socket = accept(m_TelnetPort.Socket,&addr,&size);
#endif
	if (incoming.Socket != BAD_SOCKET)
	{
	#if 1
		// set this socket to non-blocking, so we can poll it
		u_long  dwNoBlock = TRUE;
	#if defined(TARGET_PC)
		ioctlsocket( incoming.Socket, FIONBIO, &dwNoBlock );
	#else
		ioctl( incoming.Socket, FIONBIO, &dwNoBlock );
	#endif
	#endif
		incoming.IP = BAD_SOCKET;
		incoming.Port = BAD_SOCKET;

#ifdef ENABLE_KEYING
		if (!ValidateAccess())
		{
			x_printf("FATAL ERROR: This is an unauthorized or very out of date\n"
					 "version of Tribes:AADS. If you wish to run a dedicated\n"
					 "server for Tribes:AA please email aads@tribesaa.com.\n"
					 "You will need a static IP address and a fast internet\n"
					 "connection (T1 or greater) that is not behind a firewall.\n");
			exit(-1);
		}
#endif

		for (i=0;i<TELNET_MAX_CLIENTS;i++)
		{
			if (!m_Clients[i])
			{
				m_Clients[i] = new telnet_client;
				m_Clients[i]->Init(incoming);
				break;
			}
		}
		if (i==TELNET_MAX_CLIENTS)
		{
#if defined(TARGET_PC)
            closesocket(incoming.Socket);
#else
            close(incoming.Socket);
#endif
		}
	}

	for (i=0;i<TELNET_MAX_CLIENTS;i++)
	{
		if (m_Clients[i])
		{
			if (m_Clients[i]->Update(DeltaTime))
			{
				m_Clients[i]->Kill();
				delete m_Clients[i];
				m_Clients[i] = NULL;
			}
		}
	}

	g_ChatManager.Update(DeltaTime);
}

//-----------------------------------------------------------------------------
void telnet_manager::TTYMessage( const char* pMessage )
{
}

//-----------------------------------------------------------------------------
void telnet_manager::AddDebugText(const char* pFormat,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormat );

    x_vsprintf( m_LogText[m_LogTextIndex], pFormat, Args );

	m_LogText[m_LogTextIndex][79]=0x0;
    /*
    if ( g_ChatManager.IsConnected() )
    {
        g_ChatManager.Print("%s",m_LogText[m_LogTextIndex]);
    }
    */

    m_LogTextTime[m_LogTextIndex]=(f32)x_GetTimeSec();

	m_LogTextIndex++;
	if (m_LogTextIndex >= TELNET_LOG_HISTORY)
	{
		m_LogTextIndex=0;
	}
}

//-----------------------------------------------------------------------------
static s32 rgb_to_index(s32 r,s32 g,s32 b)
{
	// We have our r,g,b values, now pick a dest color based on them.
	// These are based on the default colors for mIrc

	if ( r == 0xff )
	{
		if (g==0xff)	return '1';	// opposite of background
		else if (g>192)	return '4';	// Very red
		else			return '5';	// Red
	}
	else
	{
		if (b==0xff)	return '2';	// Neutral (blue)
		else if (b>128)	return '9';	// Very green
		else			return '3';	// Green 
	}
	return '0';
}

void telnet_manager::AddDebugText(const xwchar* pString)
{
	s32 i;
	char Buffer[128];
	char*pBuffer;
	xwchar Char;

	// Convert a wide string to narrow and exclude special characters
	pBuffer = Buffer;

	i=0;
	Char = *pString++;
	while (Char && (i<sizeof(Buffer)) )
	{
		if ( Char<128 )
		{
			*pBuffer++=(char)Char;
			i++;
		}

		// Do we have a color sequence?
		if ( (Char & 0xff00) == 0xff00)
		{
			s32 fr,fg,fb;

			char fcol;

			fr = Char & 0x00ff;
			fg = (*pString  & 0xff00) >> 8;
			fb = (*pString++  & 0xff);

			fcol = rgb_to_index(fr,fg,fb);

			// Color control sequence
			*pBuffer++='\x03';
			i++;

			// Foreground color
			*pBuffer++=fcol;
			i++;
			*pBuffer++=',';
			i++;
			*pBuffer++='0';
			i++;
			if ( (*pString>='0') && (*pString <='9') )
			{
				*pBuffer++=' ';
				i++;
			}
		}

		Char = *pString++;
	}
	*pBuffer=0x0;
	AddDebugText(Buffer);
}

//-----------------------------------------------------------------------------
typedef struct rc4_key
{      
   unsigned char state[256];       
   unsigned char x;        
   unsigned char y;
} rc4_key;

#define swap_byte(x,y) t = *(x); *(x) = *(y); *(y) = t



void prepare_key(unsigned char *key_data_ptr, int key_data_len, rc4_key *key)
{
  unsigned char t;
  unsigned char index1;
  unsigned char index2;
  unsigned char* state;
  short counter;

  state = &key->state[0];
  for(counter = 0; counter < 256; counter++)
  state[counter] = (unsigned char)counter;
  key->x = 0;
  key->y = 0;
  index1 = 0;
  index2 = 0;
  for(counter = 0; counter < 256; counter++)
  {
    index2 = (key_data_ptr[index1] + state[counter] + index2) % 256;
    swap_byte(&state[counter], &state[index2]);
    index1 = (index1 + 1) % key_data_len;
  }
}

void rc4(unsigned char *buffer_ptr, int buffer_len, rc4_key *key)
{
  unsigned char t;
  unsigned char x;
  unsigned char y;
  unsigned char* state;
  unsigned char xorIndex;
  short counter;

  x = key->x;
  y = key->y;
  state = &key->state[0];
  for(counter = 0; counter < buffer_len; counter++)
  {
    x = (x + 1) % 256;
    y = (state[x] + y) % 256;
    swap_byte(&state[x], &state[y]);
    xorIndex = (state[x] + state[y]) % 256;
    buffer_ptr[counter] ^= state[xorIndex];
  }
  key->x = x;
  key->y = y;
}

//-----------------------------------------------------------------------------
xbool telnet_manager::ValidateAccess(void)
{
	rc4_key key;
	unsigned char buffer[16];

	x_memcpy(buffer,m_AccessKey,sizeof(m_AccessKey));
	// We decode the contents of m_AccessKey and save in pResult.
	prepare_key((u8*)VERSION_KEY,x_strlen(VERSION_KEY),&key);
	rc4(buffer, sizeof(buffer), &key);
	u32* p32 = (u32*)buffer;

	return ( (p32[1]==m_TelnetPort.IP) && (p32[2] == (p32[0]^p32[1])) );
}

//-----------------------------------------------------------------------------
void telnet_manager::SetAccessKey(const char* pKey)
{
	s32 i;
	char c1,c2;

	for (i=0;i<sizeof(m_AccessKey);i++)
	{
		c1 = *pKey++ - '0';
		c2 = *pKey++ - '0';
		if (c1 > 9)
		{
			c1 = c1 - 0x27;
		}

		if (c2 > 9)
		{
			c2 = c2 - 0x27;
		}
		m_AccessKey[i]=c1<<4|c2;
	}
}

//-----------------------------------------------------------------------------
void telnet_manager::GetAccessKey(char* pKey)
{
	static char* HexString="0123456789abcdef";
	s32 i;

	for (i=0;i<sizeof(m_AccessKey);i++)
	{
		*pKey++ = HexString[(m_AccessKey[i] >> 4) & 0x0f];
		*pKey++ = HexString[(m_AccessKey[i] & 0x0f)];
	}
	*pKey = 0x0;
}


