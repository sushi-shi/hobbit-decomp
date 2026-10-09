#include <xCore/Entropy/Entropy.hpp>
#include "chatclient.hpp"
#include "gamemgr/gamemgr.hpp"
#include "telnetmgr.hpp"
#include "fe_globals.hpp"
#include "gameserver.hpp"

#if defined(TARGET_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/errno.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

chat_client g_ChatManager;

static char* s_GameTypes[]=
{
    "None",
    "CTF",
    "CnH",
    "TDM",
    "DM",
    "HUNT",
    "SOLO",
};

//-----------------------------------------------------------------------------
chat_client::chat_client(void)
{
	m_Local.Clear();
	m_Channel.Clear();
	m_Nick.Clear();
	m_Host.Clear();
	m_WriteIndex = 0;
	m_ReadIndex = 0;
	m_GameScoreTimer = 0.0f;
	m_GameStateTimer = 0.0f;
	m_ChatState = CHAT_IDLE;
	m_PostFix = ' ';
	m_InBufferSize =0;
	m_InBufferIndex=0;
}

//-----------------------------------------------------------------------------
void chat_client::Init(void)
{
	m_ChatState = CHAT_IDLE;
}

//-----------------------------------------------------------------------------
void chat_client::Kill(void)
{
	CloseSession();
}

//-----------------------------------------------------------------------------
void chat_client::Update(const f32 DeltaTime)
{
	const char* pLine;

	if (m_ChatState == CHAT_IDLE)
	{
		m_PostFix = ' ';
		return;
	}
	m_StateTimer += DeltaTime;

	pLine = GetLine();

	if (pLine)
	{
//		x_printf("%s\n",pLine);
		// Deal with any generic messages that may be sent, such as PING and LOGIN failed,
		// disconnected etc.
		if (x_strncmp("PING",pLine,4)==0)
		{
			PrintBare("PONG %s\n",pLine+5);
		}
		// Any 'user in use' or 'nick in use' message kicks us in to nick conflict resolution.
		// and trying all over again. We will use the # of retries to append a number to the
		// new nick
		if (x_strstr(pLine,"have not registered"))
		{
			m_ChatState = CHAT_BEGIN_LOGIN;
			m_PostFix++;
			if (m_PostFix<'0')				// Silently convert the ' ' to a '0'
				m_PostFix='0';
			if (m_PostFix>'9')				// Too many retries (10), just give up.
			{
				SetState(CHAT_DISCONNECT);
			}
		}
	}

	switch(m_ChatState)
	{
	case CHAT_IDLE:
		break;

	case CHAT_BEGIN_LOGIN:
		// This state will just consume any early chat messages that appear.
		// It will still respond to PINGs.
		if (pLine)
			m_StateTimer = 0.5f;
		if (m_StateTimer > 1.0f)
		{
			SetState(CHAT_SEND_NICK);
			PrintBare("USER %s%c localhost localhost :Dedicated Server\n",(const char*)m_Nick,m_PostFix);
		}
		break;

	case CHAT_SEND_NICK:
		if (pLine)
			m_StateTimer = 0.5f;
		if (m_StateTimer > 1.0f)
		{
			PrintBare("NICK %s\n",(const char*)m_Nick);
			SetState(CHAT_SEND_JOIN);
		}
		break;
	case CHAT_SEND_JOIN:
		if (pLine)
			m_StateTimer = 0.5f;
		if (m_StateTimer > 1.0f)
		{
			PrintBare("JOIN #%s\n",(const char*)m_Channel);
			SetState(CHAT_WAIT_JOIN);
		}
		break;
	case CHAT_WAIT_JOIN:
		if (pLine)
			m_StateTimer = 0.5f;
		if (m_StateTimer > 2.0f)
		{
			SetState(CHAT_ACTIVE);
		}
		break;
	
	case CHAT_ACTIVE:
		m_SendInterval += DeltaTime;
		if ( (m_SendInterval > 0.075f) && (m_WriteIndex != m_ReadIndex) )
		{
			m_SendInterval = 0.0f;
//			x_printf("%s",m_OutBuffer[m_ReadIndex]);
			send(m_Local.Socket,m_OutBuffer[m_ReadIndex],x_strlen(m_OutBuffer[m_ReadIndex]),0);
			m_ReadIndex++;
			if (m_ReadIndex>=CHAT_BUFFER_HISTORY)
				m_ReadIndex=0;

		}
		break;
	case CHAT_DISCONNECT:
		if (m_StateTimer > 1.0f)
		{
			if (m_StateTimer > 2.0f)
			{
				SetState(CHAT_IDLE);
			}
			PrintBare("QUIT\n");
		}
		SetState(CHAT_IDLE);
		break;
	}
}

//-----------------------------------------------------------------------------
xbool chat_client::OpenSession(void)
{
	if (IsConnected())
	{
		CloseSession();
	}

	if (!OpenTcpPort(m_Local,m_Host))
	{
		char ipstr[64];
		net_IPToStr(m_Host.IP,ipstr);

		Print("Unable to open connection to %s:%d\n",ipstr,m_Host.Port);
		return FALSE;
	}

	m_ChatState = CHAT_BEGIN_LOGIN;
	return TRUE;
}

//-----------------------------------------------------------------------------
void chat_client::CloseSession(void)
{
	if (IsConnected())
	{
		m_ChatState = CHAT_IDLE;
#if defined(TARGET_PC)
		closesocket(m_Local.Socket);
#else
		close(m_Local.Socket);
#endif
		m_Local.Clear();
	}
}

//-----------------------------------------------------------------------------
void chat_client::Message( const char* pMessage )
{
	x_sprintf( m_OutBuffer[m_WriteIndex], "privmsg #%s :", g_ChatManager.GetChannel() );
    x_strcat ( m_OutBuffer[m_WriteIndex], pMessage );
    ASSERT( x_strlen(m_OutBuffer[m_WriteIndex]) < CHAT_LINE_WIDTH );
	m_WriteIndex++;
	if( m_WriteIndex >= CHAT_BUFFER_HISTORY )
		m_WriteIndex = 0;
//  x_printf( "%s\n", pMessage );
}

//-----------------------------------------------------------------------------
void chat_client::Print(const char* pFormatStr,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormatStr );

	s32 m,s;

	s = (s32)GameMgr.GetTimeRemaining();
	m = s / 60; s = s % 60;

	x_sprintf(m_OutBuffer[m_WriteIndex],"privmsg #%s : %02d:%02d - ",g_ChatManager.GetChannel(),m,s);
    x_vsprintf( m_OutBuffer[m_WriteIndex]+x_strlen(m_OutBuffer[m_WriteIndex]), pFormatStr, Args );
	m_WriteIndex++;
	if (m_WriteIndex >= CHAT_BUFFER_HISTORY)
		m_WriteIndex = 0;
}

//-----------------------------------------------------------------------------

void chat_client::PrintBare(const char* pFormatStr,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormatStr );

	// We're not storing the output string in the buffer since we want to send any
	// control strings immediately
    x_vsprintf( m_OutBuffer[m_WriteIndex], pFormatStr, Args );
// 	x_printf("CONTROL: %s",m_OutBuffer[m_WriteIndex]);
	send(m_Local.Socket,m_OutBuffer[m_WriteIndex],x_strlen(m_OutBuffer[m_WriteIndex]),0);
}

//-----------------------------------------------------------------------------

const char* chat_client::GetLine(void)
{
	char ch;
	s32 Length;

	if (m_InBufferSize == m_InBufferIndex)
	{
		Length = recv(m_Local.Socket,m_InBuffer,sizeof(m_InBuffer),0);
		if (Length > 0)
		{
			m_InBufferSize = Length;
			m_InBufferIndex = 0;
		}
	}

	while (m_InBufferIndex < m_InBufferSize)
	{
		ch = m_InBuffer[m_InBufferIndex++];

		if (ch == '\n')
		{
			m_LineBuffer[m_LineIndex]=0x0;
			m_LineIndex = 0;
			return m_LineBuffer;
		}

		m_LineBuffer[m_LineIndex++]=ch;
		if (m_LineIndex >= sizeof(m_LineBuffer) )
			m_LineIndex=0;
	}
	return NULL;
}

//-----------------------------------------------------------------------------

void chat_client::MakeSafe(xstring& String)
{
	s32 i;

	for (i=0;i<String.GetLength();i++)
	{
		if ( String[i]<=' ' )
		{
			String[i]='_';
		}
	}
}

//-----------------------------------------------------------------------------
