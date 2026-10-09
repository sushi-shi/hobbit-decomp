//=============================================================================
//
//  BITSTREAM.HPP
//
//=============================================================================
#ifndef BYTESTREAM_HPP
#define BYTESTREAM_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
//=============================================================================

class bytestream
{

private:

    byte*   m_Data;
    s32     m_DataSize;
    s32     m_Cursor;
	xbool	m_Error;

public:

            bytestream      ( void );
           ~bytestream      ( void );

    void    Init            ( void );
    void    Init            ( byte* pData, s32 DataSize );

    s32     GetLong         ( void );
    s16     GetWord         ( void );
    byte    GetByte         ( void );
    s16     GetNetWord      ( void );

    void    GetLongs        ( s32* pBuffer, s32 Count);
    void    GetWords        ( s16* pBuffer, s32 Count);
    void    GetBytes        ( byte* pBuffer, s32 Count);
    void    GetString       ( char* pBuffer);
    void    GetWideString   ( xwchar* pBuffer);

    void    PutLong         ( const s32 value );
    void    PutWord         ( const s16 value );
    void    PutByte         ( const byte value );
    void    PutBytes        ( const byte* pBuffer,s32 Count);
    void    PutWords        ( const s16* pBuffer, s32 Count);
    void    PutString       ( const char* pBuffer);
    void    PutWideString   ( const xwchar* pBuffer);
    void    PutNetWord      ( const s16 value );

    byte*   GetDataPtr      ( void );
    s32     GetBytesUsed    ( void );
	xbool	WasExhauted		( void ) {return (m_Error);};
};

//=============================================================================
#endif
//=============================================================================