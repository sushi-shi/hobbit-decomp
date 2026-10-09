//=========================================================================
//
//  bytestream.CPP
//
//=========================================================================
#include "bytestream.hpp"
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_string.hpp>

#define STATIC_BUFFER_SIZE 512

#define LITTLE_ENDIAN

u8 StaticbytestreamBuffer[STATIC_BUFFER_SIZE];

//=========================================================================


bytestream::bytestream( void )
{
    m_Data = NULL;
    m_DataSize = 0;
    Init( (byte*)StaticbytestreamBuffer, STATIC_BUFFER_SIZE );
}

//=========================================================================

bytestream::~bytestream( void )
{
    m_Data = NULL;
    m_DataSize = 0;
}

//=========================================================================

void    bytestream::Init            ( byte* pData, s32 DataSize )
{
    m_Data = pData;
    m_DataSize = DataSize;
    m_Cursor = 0;
	m_Error = FALSE;

}

//=========================================================================

void    bytestream::Init            ( void )
{
    Init (StaticbytestreamBuffer,STATIC_BUFFER_SIZE);
}

//=========================================================================
s32     bytestream::GetLong         ( void )
{
    u16 w1,w2;

    w1 = GetWord();
    w2 = GetWord();

    
#ifdef LITTLE_ENDIAN
    return w1|(w2<<16);
#else
    return (w1<<16)|w2;
#endif
}

//=========================================================================
s16     bytestream::GetWord          ( void )
{
    s16 ret;
    u8 b1,b2;
    b1 = GetByte();
    b2 = GetByte();
#ifdef LITTLE_ENDIAN
    ret = b1|(b2<<8);
#else
    ret = (b1<<8)|b2;
#endif
    return ret;
}

//=========================================================================
// This is always big endian
s16     bytestream::GetNetWord(void)
{
    s16 ret;
    u8 b1,b2;
    b1 = GetByte();
    b2 = GetByte();
    ret = (b1<<8)|b2;
    return ret;
}

//=========================================================================
void     bytestream::GetWords         ( s16* pBuffer, s32 Count )
{
    
    while (Count)
    {
        *pBuffer++=GetWord();
        Count--;
    }
}

//=========================================================================
byte     bytestream::GetByte         ( void )
{
	if (m_Cursor >= m_DataSize)
	{
		m_Error = TRUE;
		return 0;
	}
    return m_Data[m_Cursor++];
}

//=========================================================================
void     bytestream::GetBytes         ( byte* pBuffer, s32 Count )
{
    
    while (Count)
    {
        *pBuffer++=GetByte();
        Count--;
    }
}
//=========================================================================
void    bytestream::PutLong          ( const s32 value )
{
#ifdef LITTLE_ENDIAN
    PutWord(value & 0xffff);
    PutWord(value >> 16 & 0xffff);
#else
    PutWord(value >> 16 & 0xffff);
    PutWord(value & 0xffff);
#endif
}

//=========================================================================
void    bytestream::PutWord         ( const s16 value )
{
#ifdef LITTLE_ENDIAN
    PutByte(value & 0xff);
    PutByte((value>>8) & 0xff);
#else
    PutByte((value>>8) & 0xff);
    PutByte(value & 0xff);
#endif
}

//=========================================================================
void    bytestream::PutNetWord         ( const s16 value )
{
    PutByte((value>>8) & 0xff);
    PutByte(value & 0xff);
}

//=========================================================================
void     bytestream::PutWords         ( const s16* pBuffer, s32 Count )
{
    
    while (Count)
    {
        PutWord(*pBuffer++);
        Count--;
    }
}

//=========================================================================
void    bytestream::PutByte         ( const byte value )
{
	if (m_Cursor >= m_DataSize)
		return;
    m_Data[m_Cursor++] = (value) & 0xff;
}

//=========================================================================
void    bytestream::PutBytes(const byte *pBuffer, s32 Count)
{
    while (Count)
    {
        PutByte(*pBuffer++);
        Count--;
    }
}

//=========================================================================
byte*   bytestream::GetDataPtr      ( void )
{
    return m_Data;
}

//=========================================================================
s32     bytestream::GetBytesUsed    ( void )
{
    return m_Cursor;
}

//=========================================================================
void    bytestream::GetString   (char* pBuffer)
{
    s32 Length;
    Length = GetWord();
    GetBytes((byte*)pBuffer,Length);
    pBuffer[Length]=0x0;
}

//=========================================================================
void    bytestream::GetWideString   (xwchar* pBuffer)
{
    s32 Length;
    Length = GetWord();
    GetWords((s16 *)pBuffer,Length);
    pBuffer[Length]=0x0;
}

//=========================================================================
/*
void    bytestream::GetWideString   (xwchar* pBuffer)
{
    xwchar WideBuffer[128];

    s32 Length;
    Length = GetWord();
    ASSERT(Length < 128);
    GetWords((s16 *)WideBuffer,Length);
    WideBuffer[Length]=0x0;
    x_strcpy(pBuffer,(const char *)xstring(WideBuffer));
}
*/
//=========================================================================
void bytestream::PutString      (const char* pBuffer)
{
    PutWord(x_strlen(pBuffer));
    PutBytes((byte*)pBuffer,x_strlen(pBuffer));
}

//=========================================================================
void bytestream::PutWideString      (const xwchar* pBuffer)
{
    PutWord(x_wstrlen(pBuffer));
    PutWords((s16 *)pBuffer,x_wstrlen(pBuffer));
}

//=========================================================================
/*
void bytestream::PutWideString      (const xwchar* pBuffer)
{
    xwchar  WideBuffer[128];

    ASSERT(x_strlen(pBuffer) < 128);
    x_wstrcpy(WideBuffer,xwstring(pBuffer));

    PutWord(x_strlen(pBuffer));
    PutWords((s16 *)WideBuffer,x_strlen(pBuffer));
}
*/