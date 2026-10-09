#ifndef __TELNET_DISPLAY_HPP
#define __TELNET_DISPLAY_HPP

#include <xCore/x_files/x_types.hpp>
#include "netlib/netlib.hpp"
#define TELNET_LINE_LENGTH 64

#define TELNET_CHAR_UP	0x10
#define TELNET_CHAR_DOWN 0x11
#define TELNET_CHAR_LEFT 0x12
#define TELNET_CHAR_RIGHT 0x13
enum telnet_color
{
	BLACK=0,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	DEFAULT,
};

enum telnet_justify
{
	LEFT,
	CENTER,
	RIGHT,
};

enum
{
	B_WIN_FLAG_BORDER=0,
	B_WIN_FLAG_RENDERED,
	B_WIN_FLAG_MODAL,
};

#define WIN_FLAG_MODAL (1<<B_WIN_FLAG_MODAL)
#define WIN_FLAG_BORDER (1<<B_WIN_FLAG_BORDER)
#define WIN_FLAG_RENDERED (1<<B_WIN_FLAG_RENDERED)

struct telnet_char
{
	char			Character;
	telnet_color	ForegroundColor;
	telnet_color	BackgroundColor;
};

class telnet_client;


class telnet_display
{
public:
	class window
	{
	public:
					window(const s32 x, const s32 y, 
						   const s32 width, const s32 height, 
						   const s32 flags=0, 
						   const telnet_color foreground=WHITE, 
						   const telnet_color background=BLACK);
					~window();
		s32			m_AnchorX;
		s32			m_AnchorY;
		s32			m_Width;
		s32			m_Height;
		telnet_justify m_Justify;

		telnet_color m_Foreground;
		telnet_color m_Background;
		telnet_color m_CurrentFrontColor;
		telnet_color m_CurrentBackColor;

		s32			m_X;
		s32			m_Y;
		s32			m_Flags;
	};
private:

	s32				m_DisplayWidth;
	s32				m_DisplayHeight;
	telnet_client*	m_pClient;
	net_address		m_Remote;
	window*			m_pDefaultWindow;
	window*			m_pWindow;
	char			m_OutBuffer[512];
	s32				m_OutBufferValid;
	// Input buffer
	char			m_Buffer[TELNET_LINE_LENGTH];
	s32				m_BufferLength;
	s32				m_BufferIndex;
	// Client attributes
	// Output buffer
	// Edit buffer
	char			m_EditBuffer[TELNET_LINE_LENGTH];
	s32				m_EditBufferIndex;
	s32				m_EditCursorIndex;
	telnet_char*	m_pDisplayBuffer;
	telnet_char*	m_pDisplayBufferCopy;

public:
	void			Init			(telnet_client* pClient, const net_address& Local,const s32 Width, const s32 Height);
	void			Kill			(void);

	// Gets a character, returns 0 if none available
	const char		GetChar			(void);
	void			PutChar			(const unsigned char ch);
	// Gets a line of text, returns null if the line is not yet complete

	const char*		GetLine			(xbool HideCharacters=FALSE);
	// Puts some bytes in to the output buffer, if it gets full,
	// it will flush it

	void			PutBytes		(const char* pData, s32 Length);
	// Gets raw bytes from the input stream, returns false if the number of
	// bytes requested are not available

	s8*				GetBytes		(s32 Length);

	void			PrintChar		(const unsigned char ch);
	void			PrintChars		(const char* pString);
	// Print formatted text
	void			Print			(const char* pFormat,...);


	// Print formatted text without cursor updates
	void			PrintRaw		(const char* pFormat,...);

	// Print formatted text at a specific screen position
	void			SetJustify		(const telnet_justify Just=LEFT);
	void			PrintXY			(const s32 x, const s32 y, const char* pFormat,...);

	// Set the cursor position
	void			GotoXY			(const s32 x, const s32 y);
	void			GetXY			(s32& x, s32& y);

	s32				GetHeight		( void ) { return m_pWindow->m_Height;};
	s32				GetWidth		( void ) { return m_pWindow->m_Width;};
	// Set ANSI colors
	void			SetColor		(telnet_color foreground=WHITE, telnet_color background=BLACK);
	// Purge output buffers
	void			Flush			(xbool ClearFirst);
	void			PurgeBuffer		(void);
	// Clear the display
	void			Clear			(void);
	void			ClearChars		(s32 Count);
	void			ClearLine		(s32 Line);
	void			ClearLines		(s32 Line,s32 Count);

	void			SetWindow		(window* pWindow);
	window*			GetWindow		(void) { return m_pWindow; };
	void			RenderBorder	(void);
friend			window;
};

#endif

