#include <xCore/x_files/x_files.hpp>
#include <xCore/x_files/x_threads.hpp>
#include "netlib/netlib.hpp"
#include "telnetdisplay.hpp"
#include "telnetmgr.hpp"
#include "telnetclient.hpp"
#include <stdio.h>



#if defined(TARGET_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <asm/errno.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>

#elif defined(TARGET_PC)
#include <conio.h>
#else
#error This is a PC/Linux only file. Please exclude it from the build rules
#endif

//******************************************************************
void telnet_display::Init(telnet_client* pClient,const net_address &Address,const s32 Width, const s32 Height)
{
	m_pClient = pClient;
	m_Remote = Address;

	m_EditBufferIndex = 0;
	m_EditCursorIndex = 0;
	m_BufferIndex = 0;

	m_OutBufferValid = 0;

	m_BufferLength = 0;
	m_BufferIndex = 0;
	m_DisplayWidth = Width;
	m_DisplayHeight = Height;

	m_pDisplayBuffer=(telnet_char*)x_malloc(Width*Height*sizeof(telnet_char));
	m_pDisplayBufferCopy = (telnet_char*)x_malloc(Width*Height*sizeof(telnet_char));
	// Make sure the display buffer and the copy are NOT the same.
	x_memset(m_pDisplayBuffer, 0, Width*Height*sizeof(telnet_char));
	x_memset(m_pDisplayBufferCopy, 1, Width*Height*sizeof(telnet_char));
	ASSERT(m_pDisplayBuffer);
	ASSERT(m_pDisplayBufferCopy);
	m_pWindow = new window(0,0,Width,Height,WIN_FLAG_BORDER);
	m_pDefaultWindow = m_pWindow;
	Clear();
	Flush(TRUE);
}

//******************************************************************
void telnet_display::Kill(void)
{

	Flush(FALSE);
	delete m_pDefaultWindow;
	x_free(m_pDisplayBuffer);
	x_free(m_pDisplayBufferCopy);
	m_pDefaultWindow = NULL;
	m_pWindow = NULL;
}

//******************************************************************
void telnet_display::PurgeBuffer(void)
{
	if (m_Remote.Socket==BAD_SOCKET)
	{
	}
	else
	{
		if (m_OutBufferValid)
		{
			send(m_Remote.Socket,m_OutBuffer,m_OutBufferValid,0);
		}
	}
	m_OutBufferValid = 0;
}

//******************************************************************
void telnet_display::Flush(xbool ClearFirst)
{
	s32 x,y;
	s32 currx,curry;
	telnet_color fore,back;
	xbool changed;

	// Set Initial mode to no echo, clear the screen and reset mode
	if (ClearFirst)
	{
		PrintRaw("\xff\xfe\x22");			// (telnet) IAC will linemode
		PrintRaw("\xff\xfd\x22");			// (telnet) IAC Disable linemode
		PrintRaw("\xff\xfb\x01");			// (telnet) IAC will echo
		PrintRaw("\xff\xfe\x01");			// (telnet) IAC disable echo
		PrintRaw("\x1B[12h");				// ANSI code to disable echo
		PrintRaw("\x1B[=3;7;20l");				// 80x25 color
		PrintRaw("\x1B[H\x1B[J");			// ANSI code to clear screen
		x_memset(m_pDisplayBufferCopy,0xff,m_DisplayWidth*m_DisplayHeight*sizeof(telnet_char));
	}

	fore=(telnet_color)-1;
	back=(telnet_color)-1;
	currx=-1;
	curry=-1;

	changed = FALSE;

	// Go through the display buffer and sync up what is in
	// m_pDisplayBuffer and m_pDisplayBufferCopy

	for (y=0;y<m_DisplayHeight;y++)
	{
		currx=-1;
		for (x=0;x<m_DisplayWidth;x++)
		{
			s32 index;

			index = y*m_DisplayWidth + x;
			if (x_memcmp(&m_pDisplayBuffer[index],&m_pDisplayBufferCopy[index],sizeof(telnet_char))!=0)
			{
				if ( (currx != x) || (curry != y) )
				{
					if (m_Remote.Socket==BAD_SOCKET)
					{
						x_printfxy(x,y,"");
					}
					else
					{
						// Goto xpos,ypos, ansi sequence
						PrintRaw("\x1B[%d;%dH",y+1,x+1);
					}
				}

				if ( (fore != m_pDisplayBuffer[index].ForegroundColor) || 
					 (back != m_pDisplayBuffer[index].BackgroundColor) )
				{
					fore = m_pDisplayBuffer[index].ForegroundColor;
					back = m_pDisplayBuffer[index].BackgroundColor;
					ASSERT( (s32)back>=0);
					ASSERT( (s32)fore>=0);
					if (m_Remote.Socket==BAD_SOCKET)
					{
					}
					else
					{
						PrintRaw("\x1B[%d;%dm",(s32)fore+30,(s32)back+40);
					}
				}
				if (m_Remote.Socket == BAD_SOCKET)
				{
					x_printf("%c",m_pDisplayBuffer[index].Character);
				}
				else
				{
					PutChar(m_pDisplayBuffer[index].Character);
				}
				currx = x+1;
				curry = y;
				changed = TRUE;
			}
		}
	}

	// Force a reset of the cursor position to where it should be
	x = m_pWindow->m_X + m_pWindow->m_AnchorX;
	y = m_pWindow->m_Y + m_pWindow->m_AnchorY;
	if (m_Remote.Socket==BAD_SOCKET)
	{
		x_printfxy(x,y,"");
	}
	else
	{
		PrintRaw("\x1B[%d;%dH",y+1,x+1);
	}
	x_memcpy(m_pDisplayBufferCopy,m_pDisplayBuffer,m_DisplayWidth*m_DisplayHeight*sizeof(telnet_char));

	PurgeBuffer();
}

//******************************************************************
void telnet_display::SetColor(telnet_color foreground, telnet_color background)
{
	m_pWindow->m_CurrentFrontColor = foreground;
	m_pWindow->m_CurrentBackColor = background;
}

//******************************************************************
void telnet_display::Clear(void)
{
	ASSERT(m_pWindow);
	ClearLines(0,GetHeight());
	RenderBorder();
	GotoXY(0,0);
}

//******************************************************************
void telnet_display::ClearChars(s32 Count)
{
	s32 i;
	s32 Line,Col;


	Line = m_pWindow->m_AnchorY+m_pWindow->m_Y;
	Col  = m_pWindow->m_AnchorX+m_pWindow->m_X;

	s32 x=m_pWindow->m_X;
	for (i=0;i<Count;i++)
	{
		PrintChar(' ');
	}
	GotoXY(x,m_pWindow->m_Y);
}

//******************************************************************
void telnet_display::ClearLine(s32 Line)
{
	GotoXY(0,Line);
	ClearChars(m_pWindow->m_Width);
}

//******************************************************************
void telnet_display::ClearLines(s32 Line, s32 Count)
{
	for (s32 i=0;i<Count;i++)
	{
		ClearLine(Line+i);
	}
}

//******************************************************************

//******************************************************************
void telnet_display::SetJustify(const telnet_justify Just)
{
	ASSERT(m_pWindow);
	m_pWindow->m_Justify = Just;
}

//******************************************************************
void telnet_display::GotoXY(const s32 x,const s32 y)
{
	ASSERT(m_pWindow);
	m_pWindow->m_X = x; 
	m_pWindow->m_Y = y;
}

//******************************************************************
void telnet_display::GetXY(s32& x, s32& y)
{
	ASSERT(m_pWindow);
	x=m_pWindow->m_X;
	y=m_pWindow->m_Y;
}

//******************************************************************
// I/O routines
//******************************************************************
const char telnet_display::GetChar(void)
{
	xtimer RefreshTime;
	// If we have no buffer data, get some (socket is
	// in blocking mode)
	Flush(FALSE);
	RefreshTime.Reset();
	RefreshTime.Start();
	while (m_BufferLength<=0)
	{
		if (m_Remote.Socket != BAD_SOCKET)
		{
			m_BufferLength = recv(	m_Remote.Socket,
									m_Buffer,
									sizeof(m_Buffer),
									0);

			// Did they close the socket on us?

			if (m_BufferLength < 0)
			{
#if defined(TARGET_PC)
                if (WSAGetLastError() != WSAEWOULDBLOCK)
					m_pClient->Stop();

#else
				extern s32 errno;
                    // **BW** Need to check this
                if (errno != EAGAIN)
				{
					m_pClient->Stop();
					return 0;
				}
#endif
			}

			if ( m_BufferLength == 0 )
			{
				m_pClient->Stop();
				return 0;
			}
		}
		else
		{
#if defined(TARGET_PC)
            if (_kbhit())
				return _getch();
#endif
		}

		if ( RefreshTime.ReadSec() > 1.0f)
		{
			return 0;
		}
		x_DelayThread(10);
		m_BufferIndex = 0;
	}

	if ( (m_BufferLength > 2) && (m_Buffer[m_BufferIndex]=='\x1b') && (m_Buffer[m_BufferIndex+1]=='[') )
	{
		if (m_Buffer[m_BufferIndex+2] == 'A')
		{
			m_BufferIndex+=3;
			m_BufferLength-=3;
			return TELNET_CHAR_UP;
		}
		if (m_Buffer[m_BufferIndex+2] == 'B')
		{
			m_BufferIndex+=3;
			m_BufferLength-=3;
			return TELNET_CHAR_DOWN;
		}
		if (m_Buffer[m_BufferIndex+2] == 'C')
		{
			m_BufferIndex+=3;
			m_BufferLength-=3;
			return TELNET_CHAR_LEFT;
		}
		if (m_Buffer[m_BufferIndex+2] == 'D')
		{
			m_BufferIndex+=3;
			m_BufferLength-=3;
			return TELNET_CHAR_RIGHT;
		}
	}

	m_BufferLength--;
	return m_Buffer[m_BufferIndex++];
}
//******************************************************************
const char* telnet_display::GetLine(xbool HideCharacters)
{
	char ch;
	s32  x, y;

	xbool Exit = FALSE;
	m_EditBufferIndex = 0;
	x_memset(m_EditBuffer,0,sizeof(m_EditBuffer));

	// Get starting position of the line we're trying to edit
	GetXY(x,y);

	while(!Exit)
	{
		ch = GetChar();

		switch((u8)ch)
		{
        case 0xff:
            switch ((u8)(ch=GetChar()))
            {
            case 0xfa:
            {
                xtimer t;
                t.Start();
                while ((u8)(ch=GetChar()) != 0xf0)
                {
                    if (t.ReadSec() > 1.0f)
                        break;
                }
            }
            continue;
            default:
                ch = GetChar();
                continue;
                break;
            }
            break;
		case 0:
			break;
		case '\n':						// Do nothing for line feed
			break;
		case '\r':
			Exit = TRUE;
			break;
		case '\x1b':
			return NULL;
			break;
		case 8:							// backspace or delete
		case 127:
			if (m_EditBufferIndex)
			{
				m_EditBufferIndex--;
				m_EditBuffer[m_EditBufferIndex]=0x0;
			}
			break;

		default:
			m_EditBuffer[m_EditBufferIndex]=ch;
			m_EditBufferIndex++;
			if (m_EditBufferIndex >= sizeof(m_EditBuffer)-1)
			{
				m_EditBufferIndex = sizeof(m_EditBuffer)-2;
			}
		}
		m_EditBuffer[m_EditBufferIndex]=0x0;

		GotoXY(x,y);
		ClearChars(GetWidth()-x);
		if (HideCharacters)
		{
			s32 i;

			GotoXY(x,y);
			for (i=0;i<m_EditBufferIndex;i++)
			{
				PrintChar('*');
			}
		}
		else
		{
			PrintXY(x,y,m_EditBuffer);
		}
	}

	// Now go through the edit buffer and replace any \x<digits> with that specific character
	char* pInBuffer;
	char* pOutBuffer;
	char* pNumber;
	s32   Length;
	pInBuffer = m_EditBuffer;
	pOutBuffer = m_EditBuffer;
	Length = x_strlen(m_EditBuffer);
	while (Length)
	{
		if ( (pInBuffer[0]=='\\') && (Length > 2) && (pInBuffer[1]=='x') )
		{
			s32 value,Skip;
			pNumber = pInBuffer+2;
			value = 0;
			Skip = 2;
			ch = x_toupper(*pNumber++);
			while( ((ch >='A') && (ch <='F')) || ((ch>='0') && (ch <='9')) )
			{
				ch -='0';
				if (ch >=10)
					ch-= 'A'-'9'-1;
				value = value*16+ch;
				ch = x_toupper(*pNumber++);
				Skip++;
			}

			if ( (value > 0) && (value < 255) )
			{
				*pOutBuffer++ = value;
				pInBuffer += Skip;
				Length -= Skip;
			}
			else
			{
				*pOutBuffer++=*pInBuffer++;
				Length++;
			}
		}
		else
		{

			*pOutBuffer++ = *pInBuffer++;
			Length--;
		}
	}
	*pOutBuffer=0x0;
	GotoXY(x,y);
	return m_EditBuffer;
}

//******************************************************************
void telnet_display::PutChar(const unsigned char ch)
{
	if (m_OutBufferValid >= sizeof(m_OutBuffer))
	{
		PurgeBuffer();
	}
	m_OutBuffer[m_OutBufferValid]=ch;

	m_OutBufferValid++;
}

//******************************************************************
void telnet_display::PrintChar(const unsigned char ch)
{
	ASSERT(m_pWindow);
	s32 Index;
	s32 x,y;

	x = m_pWindow->m_AnchorX+m_pWindow->m_X;
	y = m_pWindow->m_AnchorY+m_pWindow->m_Y;

	Index = y*m_DisplayWidth + x;


	// Update cursor position for each character but check for escape codes
	if (ch =='\n')
	{
		m_pWindow->m_X = 0;
		m_pWindow->m_Y++;
		if (m_pWindow->m_Y >= m_pWindow->m_Height)
		{
			m_pWindow->m_Y = 0;
		}
		return;
	}
	else if (ch==8)
	{
		m_pWindow->m_X-=1;
		if (m_pWindow->m_X < 0)
			m_pWindow->m_X = 0;
		return;
	}
	else if (ch<32)
	{
		return;
	}
	else
	{
		m_pWindow->m_X++;
		if (m_pWindow->m_X >= m_DisplayWidth)
		{
			m_pWindow->m_X = 0;
			m_pWindow->m_Y++;
			if (m_pWindow->m_Y >= m_DisplayHeight)
			{
				m_pWindow->m_Y = 0;
			}
		}
	}

	if ( (x >= m_DisplayWidth) || (y >= m_DisplayHeight) ||
		 (x < 0) || (y < 0) )
		return;

	m_pDisplayBuffer[Index].ForegroundColor = m_pWindow->m_CurrentFrontColor;
	m_pDisplayBuffer[Index].BackgroundColor = m_pWindow->m_CurrentBackColor;
	m_pDisplayBuffer[Index].Character		= ch;
}
//******************************************************************
void telnet_display::Print(const char* pFormat,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormat );

	char LineBuffer[TELNET_LINE_WIDTH];

	x_vsprintf(LineBuffer,pFormat,Args);
	ASSERT(x_strlen(LineBuffer)<sizeof(LineBuffer));
	
	LineBuffer[m_pWindow->m_Width]=0x0;
	PrintChars(LineBuffer);
}

//******************************************************************
void telnet_display::PrintChars(const char* pString)
{
	char ch;
	s32 fgcol,bgcol;
	telnet_color oldf,oldb;

	
	oldf = m_pWindow->m_CurrentFrontColor;
	oldb = m_pWindow->m_CurrentBackColor;
	ch = *pString++;
	while (ch)
	{
		if (ch==0x03)
		{
			if ( (*pString>='0') && (*pString<='9') )
			{
				fgcol = *pString++;
				if ( (fgcol =='0') && (*pString>='0') && (*pString<='9') )
				{
					fgcol=*pString++;
				}

				if (*pString==',')
				{
					pString++;
					bgcol = *pString++;
					if ( (bgcol =='0') && (*pString>='0') && (*pString<='9') )
					{
						bgcol=*pString++;
					}
				}
				switch (fgcol)
				{
				case '0':
					SetColor(DEFAULT,m_pWindow->m_CurrentBackColor);
					break;
				case '1':
					SetColor(WHITE,m_pWindow->m_CurrentBackColor);
					break;
				case '2':
					SetColor(BLUE,m_pWindow->m_CurrentBackColor);
					break;
				case '3':
					SetColor(CYAN,m_pWindow->m_CurrentBackColor);
					break;
				case '4':
					SetColor(RED,m_pWindow->m_CurrentBackColor);
					break;
				case '5':
					SetColor(YELLOW,m_pWindow->m_CurrentBackColor);
					break;
				case '9':
					SetColor(GREEN,m_pWindow->m_CurrentBackColor);
					break;
				default:
					SetColor(WHITE,m_pWindow->m_CurrentBackColor);
					break;
				}
			}
		}
		else
		{
			PrintChar(ch);
		}
		ch = *pString++;
	}
	SetColor(oldf,oldb);
}

//******************************************************************
void telnet_display::PrintXY(const s32 x, const s32 y,const char* pFormat,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormat );

	char LineBuffer[256];
	s32 i;

	x_vsprintf(LineBuffer,pFormat,Args);
	ASSERT(x_strlen(LineBuffer)<sizeof(LineBuffer));
	s32 jx;
	switch (m_pWindow->m_Justify)
	{
	default:
	case LEFT:
		jx = 0;
		break;
	case RIGHT:
		jx = -x_strlen(LineBuffer);
		break;
	case CENTER:
		jx = -x_strlen(LineBuffer)/2;
		break;
	}
	GotoXY(x+jx,y);
	i=0;
	PrintChars(LineBuffer);
}

//******************************************************************
void telnet_display::PrintRaw(const char* pFormat,...)
{
    x_va_list   Args;
    x_va_start( Args, pFormat );

	char LineBuffer[256];
	s32 i;

	x_vsprintf(LineBuffer,pFormat,Args);
	ASSERT(x_strlen(LineBuffer)<sizeof(LineBuffer));
	i=0;
	while (LineBuffer[i])
	{
		PutChar(LineBuffer[i]);
		i++;
	}
}

//******************************************************************
void telnet_display::SetWindow(window* pWindow)
{
	if (pWindow)
	{
		m_pWindow = pWindow;
	}
	else
	{
		m_pWindow = m_pDefaultWindow;
	}

	GotoXY(m_pWindow->m_X,m_pWindow->m_Y);
	SetColor(m_pWindow->m_CurrentFrontColor,m_pWindow->m_CurrentBackColor);
	// Reset ANSI clipping area on display
}

void telnet_display::RenderBorder(void)
{
	s32 x,y,i,j;

	ASSERT(m_pWindow);

	if (m_pWindow->m_Flags & WIN_FLAG_BORDER)
	{
		GetXY(x,y);
		// Set color
		// Go to top left of window (remember, raw codes start at 1,1 not 0,0
		// and draw top line
		GotoXY(-1,-1);
		if ( (m_pWindow->m_AnchorY > m_pDefaultWindow->m_AnchorY) && !(m_pWindow->m_Flags & WIN_FLAG_MODAL) )
			PrintChar(0xcc);										// Left T piece
		else
			PrintChar(0xc9);										// Top left corner piece

		for (j=0;j<m_pWindow->m_Width;j++)
		{
			PrintChar(0xcd);										// Top/bottom line piece
		}
		if ( (m_pWindow->m_Height < m_pDefaultWindow->m_Height)  && !(m_pWindow->m_Flags & WIN_FLAG_MODAL) )
			PrintChar(0xb9);										// Right T piece
		else
			PrintChar(0xbb);										// Top right corner piece

		for (i=0;i<m_pWindow->m_Height;i++)
		{
			// Left pos
			GotoXY(-1,i);
			
			PrintChar(0xba);										// Left/right line piece
			GotoXY(m_pWindow->m_Width,i);
			PrintChar(0xba);										// Left/right line piece
		}

		GotoXY(-1,m_pWindow->m_Height);

		if ( (m_pWindow->m_AnchorY + m_pWindow->m_Height < m_pDefaultWindow->m_Height) && !(m_pWindow->m_Flags & WIN_FLAG_MODAL) )
			PrintChar(0xcc);										// Left T piece
		else
			PrintChar(0xc8);										// Bottom left corner piece

		for (j=0;j<m_pWindow->m_Width;j++)
		{
			PrintChar(0xcd);										// Top/bottom line piece
		}

		if ( (m_pWindow->m_AnchorY + m_pWindow->m_Height < m_pDefaultWindow->m_Height) && !(m_pWindow->m_Flags & WIN_FLAG_MODAL) )
			PrintChar(0xb9);										// Right T piece
		else
			PrintChar(0xbc);										// Bottom right corner piece

		GotoXY(x,y);

	}
}

telnet_display::window::window(const s32 x, const s32 y, 
							   const s32 width, const s32 height, 
							   const s32 flags, const telnet_color foreground, 
							   const telnet_color background)
{
	m_X				= 0;
	m_Y				= 0;
	m_Flags			= flags;
	m_Foreground	= foreground;
	m_Background	= background;
	m_CurrentFrontColor = m_Foreground;
	m_CurrentBackColor = m_Background;
	m_Justify		= LEFT;

	if (flags & WIN_FLAG_BORDER)
	{
		m_AnchorX = x+1;
		m_AnchorY = y+1;
		m_Width	  = width-2;
		m_Height  = height-2;
		ASSERT( (m_Width > 0) && (m_Height > 0) );
	}
	else
	{
		m_AnchorX = x;
		m_AnchorY = y;
		m_Width	  = width;
		m_Height  = height;
	}
}

telnet_display::window::~window(void)
{
}

