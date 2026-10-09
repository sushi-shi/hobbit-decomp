//=============================================================================
//
//  BITSTREAM.HPP
//
//=============================================================================
#ifndef BITSTREAM_HPP
#define BITSTREAM_HPP

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#include <Support/ObjectMgr/Object.hpp>
#include "netlib/netlib.hpp"

//=============================================================================


//=============================================================================
// BITSTREAM CLASS
//=============================================================================

class bitstream
{

private:

    byte*   m_Data;
    s32     m_DataSize;
    s32     m_DataSizeInBits;
    s32     m_HighestBitWritten;
    xbool   m_Overwrite;
    s32     m_SectionCursor;

    mutable s32     m_Cursor;
    

    void    WriteRawBits       ( const void* pData, s32 NBits );
    void    ReadRawBits        ( void* pData, s32 NBits ) const;
    void    WriteRaw32         ( u32 Value, s32 NBits );
    u32     ReadRaw32          ( s32 NBits ) const;

public:

    bitstream   ( void );
    ~bitstream  ( void );

    void    Init            ( void* pData, s32 DataSize );

    s32     GetNBytes       ( void ) const;
    s32     GetNBits        ( void ) const;
    s32     GetNBytesUsed   ( void ) const;
    s32     GetNBytesFree   ( void ) const;
    s32     GetNBitsUsed    ( void ) const;
    s32     GetNBitsFree    ( void ) const;
    byte*   GetDataPtr      ( void ) const;
    xbool   IsFull          ( void ) const;
    void    Clear           ( void );

    // Overwrite control
    xbool   Overwrite       ( void ) const;
    void    ClearOverwrite  ( void );

    s32     GetCursor       ( void ) const;
    void    SetCursor       ( s32 BitIndex );
    s32     GetCursorRemaining( void ) const;

    void    OpenSection     ( void );
    xbool   CloseSection    ( void );

    // Integer Helpers
    void    WriteU64        ( u64 Value, s32 NBits=64 );
    void    WriteS32        ( s32 Value, s32 NBits=32 );
    void    WriteU32        ( u32 Value, s32 NBits=32 );
    void    WriteS16        ( s16 Value, s32 NBits=16 );
    void    WriteU16        ( u16 Value, s32 NBits=16 );
    void    WriteRangedS32  ( s32 Value, s32 Min, s32 Max );
    void    WriteRangedU32  ( u32 Value, s32 Min, s32 Max );

    void    ReadU64         ( u64& Value, s32 NBits=64 ) const;
    void    ReadS32         ( s32& Value, s32 NBits=32 ) const;
    void    ReadU32         ( u32& Value, s32 NBits=32 ) const;
    void    ReadS16         ( s16& Value, s32 NBits=16 ) const;
    void    ReadU16         ( u16& Value, s32 NBits=16 ) const;
    void    ReadRangedS32   ( s32& Value, s32 Min, s32 Max ) const;
    void    ReadRangedU32   ( u32& Value, s32 Min, s32 Max ) const;

    // Float helpers
    void    WriteF32        ( f32 Value );
    void    WriteRangedF32  ( f32 Value, s32 NBits, f32 Min, f32 Max );

    void    ReadF32         ( f32& Value ) const;
    void    ReadRangedF32   ( f32& Value, s32 NBits, f32 Min, f32 Max ) const;

    static void    TruncateRangedF32   ( f32& Value, s32 NBits, f32 Min, f32 Max );
    static void    TruncateRangedVector( vector3& N, s32 NBits, f32 Min, f32 Max );

    // Vectors
    void    WriteVector         ( const vector3& N );
    void    WriteRangedVector   ( const vector3& N, s32 NBits, f32 Min, f32 Max );
    void    WriteUnitVector     ( const vector3& N, s32 TotalBits );
    void    WriteWorldPosCM     ( const vector3& Pos );

    void    ReadVector          ( vector3& N ) const;
    void    ReadRangedVector    ( vector3& N, s32 NBits, f32 Min, f32 Max ) const;
    void    ReadUnitVector      ( vector3& N, s32 TotalBits ) const;
    void    ReadWorldPosCM      ( vector3& Pos ) const;

    // Radian3
    void    WriteRadian3        ( const radian3& Radian );
    void    WriteRangedRadian3  ( const radian3& Radian, s32 NBits );

    void    ReadRadian3         ( radian3& Radian ) const;
    void    ReadRangedRadian3   ( radian3& Radian, s32 NBits ) const;

    // Color
    void    WriteColor          ( xcolor Color );
    void    ReadColor           ( xcolor& Color ) const;

    // String
    void    WriteString         ( const char* pBuf );
    void    ReadString          ( char* pBuf ) const;

    // Wide string
    void    WriteWString        ( const xwchar* pBuf );
    void    ReadWString         ( xwchar* pBuf ) const;

    // Handles a full matrix... full precision
    void    WriteMatrix4        ( const matrix4& M );
    void    ReadMatrix4         ( matrix4& M ) const;
   
    // ObjectID
    void    WriteObjectID       ( const object::id& ObjectID );
    void    ReadObjectID        ( object::id& ObjectID ) const;

    // TeamBits
    void    WriteTeamBits       ( u32 TeamBits );
    void    ReadTeamBits        ( u32& TeamBits ) const;

    // Raw bits
    void    WriteBits       ( const void* pData, s32 NBits );
    void    ReadBits        ( void* pData, s32 NBits ) const;
    xbool   WriteFlag       ( xbool Value );
    xbool   ReadFlag        ( void ) const;

    // Marker support
    void    WriteMarker     ( void );
    void    ReadMarker      ( void ) const;

    void    Encrypt         ( s32 EncryptionKey);
    xbool   Decrypt         ( s32 EncryptionKey);
    void    Send            ( const net_address &CallerAddress,
                              const net_address &SenderAddress);

    xbool   Receive         ( net_address &CallerAddress,
                              net_address &SenderAddress );

};

//=============================================================================
#endif
//=============================================================================