//=========================================================================
//
//  BITSTREAM.CPP
//
//=========================================================================
#include "bitstream.hpp"
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_string.hpp>
#include <Support/ObjectMgr/ObjectMgr.hpp>


#ifndef LITTLE_ENDIAN
#error bitstream.cpp currently relies on little endian
#endif

//=========================================================================

#define TO_U32(x)       (*((u32*)(&(x))))
#define TO_F32(x)       (*((f32*)(&(x))))
#define LOWER_BITS(x)   ((u32)(~(-1<<(x))))

#define STATIC_BUFFER_SIZE  (896)

struct chunk128
{
    byte PAD[128];
} PS2_ALIGNMENT(128);

static chunk128 StaticBitstreamBuffer[(STATIC_BUFFER_SIZE+128)/128];

//=========================================================================

inline s32 GetHighestBit( u32 N )
{
    s32 B=0;
    while( N ) { B++; N>>=1;}
    return B;
}

//=========================================================================

bitstream::bitstream( void )
{
    m_Data = NULL;
    m_DataSize = 0;
    Init( (byte*)StaticBitstreamBuffer, STATIC_BUFFER_SIZE );
}

//=========================================================================

bitstream::~bitstream( void )
{
    m_Data = NULL;
    m_DataSize = 0;
}

//=========================================================================

void    bitstream::Init            ( void* pData, s32 DataSize )
{
    m_Data = (byte*)pData;
    m_DataSize = DataSize;
    m_DataSizeInBits = m_DataSize<<3;
    m_HighestBitWritten = -1;
    m_Cursor = 0;
    m_Overwrite = FALSE;
    m_SectionCursor = -1;
}

//=========================================================================

s32     bitstream::GetNBytes       ( void ) const
{
    return m_DataSize;
}

//=========================================================================

xbool     bitstream::Overwrite       ( void ) const
{
    return m_Overwrite;
}

//=========================================================================

void     bitstream::ClearOverwrite       ( void )
{
    m_Overwrite = FALSE;
}

//=========================================================================

s32     bitstream::GetNBits        ( void ) const
{
    return m_DataSizeInBits;
}

//=========================================================================

s32     bitstream::GetNBytesUsed   ( void ) const
{
    s32 NBitsUsed = (m_HighestBitWritten+1);
    return (NBitsUsed + 7)>>3;
}

//=========================================================================

s32     bitstream::GetNBytesFree   ( void ) const
{
    return m_DataSize - GetNBytesUsed();
}

//=========================================================================

s32     bitstream::GetNBitsUsed    ( void ) const
{
    return (m_HighestBitWritten+1);
}

//=========================================================================

s32     bitstream::GetNBitsFree    ( void ) const
{
    return m_DataSizeInBits - GetNBitsUsed();
}

//=========================================================================

byte*   bitstream::GetDataPtr      ( void ) const
{
    return m_Data;
}

//=========================================================================

xbool   bitstream::IsFull          ( void ) const
{
    return (m_Cursor>=m_DataSizeInBits);
}

//=========================================================================

s32     bitstream::GetCursor       ( void ) const
{
    return m_Cursor;
}

//=========================================================================

s32     bitstream::GetCursorRemaining( void ) const
{
    return m_DataSizeInBits - m_Cursor;
}

//=========================================================================

void    bitstream::SetCursor       ( s32 BitIndex )
{
    ASSERT( BitIndex>=0 );

    // This assert was taken out for silent failure to write
    //ASSERT( BitIndex<m_DataSizeInBits );

    m_Cursor = BitIndex;
}

//=========================================================================

void    bitstream::OpenSection     ( void )
{
    ASSERT( m_SectionCursor == -1 );
    m_SectionCursor = GetCursor();
}

//=========================================================================

xbool   bitstream::CloseSection    ( void )
{
    ASSERT( m_SectionCursor >= 0 );
    if( Overwrite() )
    {
        SetCursor( m_SectionCursor );
        ClearOverwrite();
        m_SectionCursor = -1;
        return( FALSE );
    }

    m_SectionCursor = -1;
    return( TRUE );
}

//=========================================================================

void    bitstream::WriteU64        ( u64 Value, s32 NBits )
{
    u32 V0     = (u32)((Value>> 0)&0xFFFFFFFF);
    u32 V1     = (u32)((Value>>32)&0xFFFFFFFF);
    s32 NBits0 = MIN(NBits,32);
    s32 NBits1 = NBits - NBits0;

    WriteRaw32( V0, NBits0);

    if( NBits1 > 0 )
        WriteRaw32( V1, NBits1);
}

//=========================================================================

void    bitstream::WriteS32        ( s32 Value, s32 NBits )
{
    WriteRaw32(Value, NBits);
}

//=========================================================================

void    bitstream::WriteU32        ( u32 Value, s32 NBits )
{
    WriteRaw32(Value, NBits);
}

//=========================================================================

void    bitstream::WriteS16        ( s16 Value, s32 NBits )
{
    WriteRaw32(Value, NBits);
}

//=========================================================================

void    bitstream::WriteU16        ( u16 Value, s32 NBits )
{
    WriteRaw32(Value, NBits);
}

//=========================================================================

void    bitstream::WriteMarker  ( void )
{
    WriteU32( 0xDEADBEEF );
}

//=========================================================================

void    bitstream::ReadMarker  ( void ) const
{
    u32 R;
    ReadU32( R );
    ASSERT( R == 0xDEADBEEF );
}

//=========================================================================

void    bitstream::WriteRangedS32  ( s32 Value, s32 Min, s32 Max )
{
    ASSERT( (Max>Min) && (Value>=Min) && (Value<=Max) );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    WriteU32( Value - Min, NBits );
}

//=========================================================================

void    bitstream::WriteRangedU32  ( u32 Value, s32 Min, s32 Max )
{
    ASSERT( (Max>Min) && (Value>=(u32)Min) && (Value<=(u32)Max) );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    WriteU32( Value - Min, NBits );
}

//=========================================================================
/*
s32     bitstream::ReadS32         ( s32 NBits ) const
{
    u32 V = ReadRaw32( NBits );
    
    if( NBits==32 ) return (s32)V;
    
    // extend sign bit
    if( V & (1<<(NBits-1)) )
        V |= (0xFFFFFFFF & (~((1<<NBits)-1)));

    return (s32)V;
}

//=========================================================================

u32     bitstream::ReadU32         ( s32 NBits ) const
{
    u32 V = ReadRaw32( NBits );
    return V;
}

//=========================================================================

s32     bitstream::ReadRangedS32   ( s32 Min, s32 Max ) const
{
    ASSERT( Max>Min );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    s32 Value = ReadS32( NBits ) + Min;
    ASSERT( (Value>=Min) && (Value<=Max) );
    return Value;
}

//=========================================================================

u32     bitstream::ReadRangedU32   ( s32 Min, s32 Max ) const
{
    ASSERT( Max>Min );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    s32 Value = ReadU32( NBits ) + Min;
    ASSERT( (Value>=Min) && (Value<=Max) );
    return Value;
}
*/
//=========================================================================

void    bitstream::ReadU64( u64& Value, s32 NBits ) const
{
    s32 NBits0 = MIN(NBits,32);
    s32 NBits1 = NBits - NBits0;
    u32 V0;
    u32 V1=0;

    V0 = ReadRaw32( NBits0 );

    if( NBits1 > 0 )
        V1 = ReadRaw32( NBits1 );

    Value = (((u64)V1)<<32) | V0;
}

//=========================================================================

void    bitstream::ReadS32         ( s32& Value, s32 NBits ) const
{
    Value = ReadRaw32( NBits );
    
    if( NBits==32 ) return;
    
    // extend sign bit
    if( Value & (1<<(NBits-1)) )
        Value |= (0xFFFFFFFF & (~((1<<NBits)-1)));
}

//=========================================================================

void    bitstream::ReadU32         ( u32& Value, s32 NBits ) const
{
    Value = ReadRaw32( NBits );
}

//=========================================================================

void    bitstream::ReadS16         ( s16& Value, s32 NBits ) const
{
    Value = ReadRaw32( NBits );
    
    if( NBits==16 ) return;
    
    // extend sign bit
    if( Value & (1<<(NBits-1)) )
        Value |= (0xFFFF & (~((1<<NBits)-1)));
}

//=========================================================================

void    bitstream::ReadU16         ( u16& Value, s32 NBits ) const
{
    Value = ReadRaw32( NBits );
}

//=========================================================================

void    bitstream::ReadRangedS32   ( s32& Value, s32 Min, s32 Max ) const
{
    ASSERT( Max>Min );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    u32 V;
    ReadU32( V, NBits );
    Value = (s32)V+Min;
    ASSERT( (Value>=Min) && (Value<=Max) );
}

//=========================================================================

void    bitstream::ReadRangedU32   ( u32& Value, s32 Min, s32 Max ) const
{
    ASSERT( Max>Min );
    s32 Range = Max - Min;
    s32 NBits = GetHighestBit(Range);
    ReadU32( Value, NBits );
    Value += Min;
    ASSERT( (Value>=(u32)Min) && (Value<=(u32)Max) );
}

//=========================================================================

void    bitstream::WriteF32        ( f32 Value )
{
    WriteRaw32( TO_U32(Value), 32 );
}

//=========================================================================

void    bitstream::WriteRangedF32  ( f32 Value, s32 NBits, f32 Min, f32 Max )
{
    if( NBits==32 )
    {
        WriteF32( Value );
        return;
    }

    ASSERT( Max > Min );
    Value -= Min;
    f32 Range = Max-Min;
    s32 Scalar = LOWER_BITS(NBits);
    u32 N = (u32)(((Value/Range) * (f32)Scalar) + 0.5f);
    ASSERT( N <= (u32)Scalar );

    WriteU32( N, NBits );
}

//=========================================================================
/*
f32     bitstream::ReadF32         ( void ) const
{
    u32 V = ReadRaw32( 32 );
    return TO_F32(V);
}

//=========================================================================

f32     bitstream::ReadRangedF32   ( s32 NBits, f32 Min, f32 Max ) const
{
    ASSERT( Max>Min );
    f32 Range = Max-Min;
    u32 N = ReadU32( NBits );
    f32 Value = (((f32)N)/(f32)LOWER_BITS(NBits))*Range + Min;
    ASSERT( (Value>=Min) && (Value<=Max) );
    return Value;
}
*/
//=========================================================================

void    bitstream::ReadF32         ( f32& Value ) const
{
    u32 UValue = ReadRaw32( 32 );
    Value = TO_F32(UValue);
}

//=========================================================================

void    bitstream::ReadRangedF32   ( f32& Value, s32 NBits, f32 Min, f32 Max ) const
{
    if( NBits==32 )
    {
        ReadF32( Value );
        return;
    }
    ASSERT( Max>Min );
    f32 Range = Max-Min;
    s32 Scalar = LOWER_BITS(NBits);
    u32 N;
    ReadU32( N, NBits );
    Value = (((f32)N)/Scalar)*Range + Min;
    ASSERT( (Value>=Min) && (Value<=Max) );
}

//=========================================================================

void bitstream::TruncateRangedF32( f32& Value, s32 NBits, f32 Min, f32 Max )
{
    if( NBits==32 ) 
        return;

    if( IN_RANGE( Min, Value, Max ) )
    {
        ASSERT( (Max>Min) && IN_RANGE( Min, Value, Max ) );
        Value -= Min;
        f32 Range = Max-Min;
        s32 Scalar = LOWER_BITS(NBits);
        u32 N = (u32)(((Value/Range)*(f32)Scalar)+0.5f);
        Value = (((f32)N)/Scalar)*Range + Min;
        ASSERT( IN_RANGE( Min, Value, Max ) );
    }
}

//=========================================================================

void bitstream::TruncateRangedVector( vector3& N, s32 NBits, f32 Min, f32 Max )
{
	TruncateRangedF32(N.X, NBits, Min, Max) ;
	TruncateRangedF32(N.Y, NBits, Min, Max) ;
	TruncateRangedF32(N.Z, NBits, Min, Max) ;
}

//=========================================================================

void    bitstream::WriteColor      ( xcolor Color )
{
    WriteU32( *((u32*)&Color) );
}

//=========================================================================

void    bitstream::ReadColor       ( xcolor& Color ) const
{
    u32 C;
    ReadU32(C);
    Color = (xcolor)C;
}

//=========================================================================

void    bitstream::WriteVector         ( const vector3& N )
{
    WriteF32( N.X );
    WriteF32( N.Y );
    WriteF32( N.Z );
}

//=========================================================================

void    bitstream::WriteRangedVector   ( const vector3& N, s32 NBits, f32 Min, f32 Max )
{
    WriteRangedF32( N.X, NBits, Min, Max );
    WriteRangedF32( N.Y, NBits, Min, Max );
    WriteRangedF32( N.Z, NBits, Min, Max );
}

//=========================================================================

void    bitstream::WriteUnitVector     ( const vector3& N, s32 TotalBits )
{
#ifdef X_ASSERT
    f32 L = N.X*N.X + N.Y*N.Y + N.Z*N.Z;
    ASSERT( (L>=0.99f) && (L<=1.001f) );
#endif

    radian P,Y;
    N.GetPitchYaw(P,Y);

    while( Y<0     ) Y += R_360;
    while( Y>R_360 ) Y -= R_360;
    while( P<0     ) P += R_360;
    while( P>R_360 ) P -= R_360;
    if( P>R_180 ) P = P - R_360;
    if( P<-R_90 ) P = -R_90;
    if( P>R_90 ) P = R_90;

    WriteRangedF32( Y, (TotalBits/2),  R_0,  R_360 );
    WriteRangedF32( P, (TotalBits/2), -R_90, R_90 );
}

//=========================================================================

void    bitstream::WriteWorldPosCM     ( const vector3& Pos )
{
    xbool InRange=TRUE;
    if( (Pos.X < 0) || (Pos.X>2048.0f) ) InRange = FALSE;
    if( (Pos.Y < 0) || (Pos.Y>1024.0f) ) InRange = FALSE;
    if( (Pos.Z < 0) || (Pos.Z>2048.0f) ) InRange = FALSE;

    if( WriteFlag( InRange ) )
    {
        WriteRangedF32( Pos.X, 18, 0.0f, 2048.0f );
        WriteRangedF32( Pos.Y, 17, 0.0f, 1024.0f );
        WriteRangedF32( Pos.Z, 18, 0.0f, 2048.0f );
    }
    else
    {
        WriteVector( Pos );
    }
}

//=========================================================================

void    bitstream::ReadVector          ( vector3& N ) const
{
    ReadF32(N.X);
    ReadF32(N.Y);
    ReadF32(N.Z);
}

//=========================================================================

void    bitstream::ReadRangedVector    ( vector3& N, s32 NBits, f32 Min, f32 Max ) const
{
    ReadRangedF32( N.X, NBits, Min, Max );
    ReadRangedF32( N.Y, NBits, Min, Max );
    ReadRangedF32( N.Z, NBits, Min, Max );
}

//=========================================================================

void    bitstream::ReadUnitVector     ( vector3& N, s32 TotalBits ) const
{
    radian P,Y;
    ReadRangedF32( Y, (TotalBits/2),  R_0,  R_360 );
    ReadRangedF32( P, (TotalBits/2), -R_90, R_90 );
    N.Set( P, Y );
}

//=========================================================================

void    bitstream::ReadWorldPosCM     ( vector3& Pos ) const
{
    if( ReadFlag() )
    {
        ReadRangedF32( Pos.X, 18, 0.0f, 2048.0f );
        ReadRangedF32( Pos.Y, 17, 0.0f, 1024.0f );
        ReadRangedF32( Pos.Z, 18, 0.0f, 2048.0f );
    }
    else
    {
        ReadVector( Pos );
    }
}

//=========================================================================

void    bitstream::WriteRadian3         ( const radian3& R )
{
    WriteF32( R.Pitch );
    WriteF32( R.Yaw );
    WriteF32( R.Roll );
}

//=========================================================================

void    bitstream::ReadRadian3          ( radian3& R ) const
{
    ReadF32(R.Pitch);
    ReadF32(R.Yaw);
    ReadF32(R.Roll);
}

//=========================================================================

void    bitstream::WriteRangedRadian3  ( const radian3& Radian, s32 NBits )
{
    f32 X = x_fmod(Radian.Pitch,R_360)*(1/R_360)*0.5f + 0.5f;
    f32 Y = x_fmod(Radian.Yaw,R_360)*(1/R_360)*0.5f + 0.5f;
    f32 Z = x_fmod(Radian.Roll,R_360)*(1/R_360)*0.5f + 0.5f;

    f32 LB = (f32)LOWER_BITS(NBits);

    u32 NX = (u32)(X*LB);
    u32 NY = (u32)(Y*LB);
    u32 NZ = (u32)(Z*LB);

    ASSERT( NX<=LB );
    ASSERT( NY<=LB );
    ASSERT( NZ<=LB );

    WriteU32( NX, NBits );
    WriteU32( NY, NBits );
    WriteU32( NZ, NBits );
}

//=========================================================================

void    bitstream::ReadRangedRadian3   ( radian3& Radian, s32 NBits ) const
{
    u32 NX,NY,NZ;
    ReadU32( NX, NBits );
    ReadU32( NY, NBits );
    ReadU32( NZ, NBits );

    Radian.Pitch = (f32)((NX*(1/(f32)LOWER_BITS(NBits)))-0.5f)*2*R_360;
    Radian.Yaw   = (f32)((NY*(1/(f32)LOWER_BITS(NBits)))-0.5f)*2*R_360;
    Radian.Roll  = (f32)((NZ*(1/(f32)LOWER_BITS(NBits)))-0.5f)*2*R_360;
}

//=========================================================================

void    bitstream::WriteString         ( const char* pBuf )
{
    // Get len
    const char* pC = pBuf;
    while( *pC++ ) {};
    
    s32 L = pC-pBuf;
    ASSERT( L < 256 );

    WriteU32( L, 8 );
    WriteRawBits( pBuf, L*8 );
}

//=========================================================================

void    bitstream::ReadString          ( char* pBuf ) const
{
    u32 L;
    ReadU32( L, 8 );
    ReadRawBits( pBuf, L*8 );
    pBuf[L] = '\0';
}

//=========================================================================

void    bitstream::WriteWString         ( const xwchar* pBuf )
{
    // Get len
    const xwchar* pC = pBuf;
    while( *pC ) 
	{
		pC++;
	};
    
    s32 L = pC-pBuf;
    ASSERT( L < 256 );

    WriteU32( L, 8 );
    while( L > 0 )
    {
        if( WriteFlag( *pBuf < 256 ) )
        {   
            WriteRangedS32( *pBuf, 0, 255 );
        }
        else
        {
            WriteU16( *pBuf );
        }
        
        pBuf++;
        L--;
    }
}

//=========================================================================

void    bitstream::ReadWString          ( xwchar* pBuf ) const
{
    u32 L;
    s32 V;
    ReadU32( L, 8 );
    while( L > 0 )
    {
        if( ReadFlag() )
        {
            ReadRangedS32( V, 0, 255 );
            *pBuf = V;
        }
        else
        {
            ReadU16( *pBuf );
        }
        pBuf++;
        L--;
    }
    *pBuf = '\0';
}

//=========================================================================

void    bitstream::WriteMatrix4    ( const matrix4& M )
{
    f32* pF = (f32*)&M;
    for( s32 i=0; i<16; i++ )
        WriteF32( *pF++ );
}

//=========================================================================

void    bitstream::ReadMatrix4     ( matrix4& M ) const
{
    f32* pF = (f32*)&M;
    for( s32 i=0; i<16; i++ )
        ReadF32( *pF++ );
}

//=========================================================================

void bitstream::WriteObjectID( const object::id& ObjectID )
{
    // Size of data written:
    // PlayerID  = 6  bits
    // NullID    = 2  bits
    // GenericID = 12 bits (when MAX_SERVER_OBJECTS=1<<10)

    // Most object ID's to send are players (slots 0-31) so there's a special case here
    if (WriteFlag((ObjectID.Slot >= 0) && (ObjectID.Slot <= 31)))
        WriteRangedU32((u32)ObjectID.Slot, 0, 31) ;
    else
    {
        // Non-null slot?
        if (WriteFlag(ObjectID.Slot != obj_mgr::NullID.Slot))
            WriteRangedU32((u32)ObjectID.Slot, 0, obj_mgr::MAX_SERVER_OBJECTS-1) ;
    }
}

//==============================================================================

void bitstream::ReadObjectID( object::id& ObjectID ) const
{
    u32 TempU32 ;

    // Default to null id
    ObjectID = obj_mgr::NullID ;

    // Reading a player ID (slots 0-31) ?
    if (ReadFlag())
    {
        ReadRangedU32(TempU32, 0, 31) ;
        ObjectID.Slot = (s16)TempU32 ;
    }
    else
    {
        // Read non-null slot?
        if (ReadFlag())
        {
            ReadRangedU32(TempU32, 0, obj_mgr::MAX_SERVER_OBJECTS-1) ;
            ObjectID.Slot = (s16)TempU32 ;
        }
    }
}

//=========================================================================

void bitstream::WriteTeamBits( u32 TeamBits )
{
    s32 ID=-1;

    if( TeamBits < 3 )
    {
        ID = TeamBits;
        WriteFlag(TRUE);
        WriteU32(ID,2);
        return;
    }
    else
    if( TeamBits == 0xFFFFFFFF )
    {
        ID = 3;
        WriteFlag(TRUE);
        WriteU32(ID,2);
        return;
    }

    WriteFlag(FALSE);

    // Find Bit
    for( s32 i=0; i<32; i++ )
    if( TeamBits & (1<<i) )
    {
        WriteU32( (i-2), 5 );
        return;
    }
}

//=========================================================================

void bitstream::ReadTeamBits( u32& TeamBits ) const
{
    u32 ID;
    if( ReadFlag() )
    {
        ReadU32( ID, 2 );
        if( ID == 3 )
            TeamBits = 0xFFFFFFFF;
        else
            TeamBits = ID;
    }
    else
    {
        ReadU32( ID, 5 );
        TeamBits = (1<<(ID+2));
    }
}

//=========================================================================

void    bitstream::ReadBits        ( void* pData, s32 NBits ) const
{
    ReadRawBits( pData, NBits );
}

//=========================================================================

void    bitstream::WriteBits        ( const void* pData, s32 NBits )
{
    WriteRawBits( pData, NBits );
}

//=========================================================================
//=========================================================================
//=========================================================================
// THESE ROUTINES ACTUALLY READ/WRITE DATA
//=========================================================================
//=========================================================================
//=========================================================================


//=========================================================================

void    bitstream::Clear            ( void )
{
    x_memset( m_Data, 0, m_DataSize );
    m_Cursor = 0;
    m_HighestBitWritten = -1;
}

//=========================================================================

xbool   bitstream::WriteFlag       ( xbool Value )
{
    if( m_Cursor >= m_DataSizeInBits )
    {
        // Special case:  Only trigger overwrite when writing TRUE.
        if( Value )
            m_Overwrite = TRUE;
        return Value;
    }

    if( Value )
        *(m_Data + (m_Cursor>>3)) |=  (1<<(7-(m_Cursor & 0x07)));
    else
        *(m_Data + (m_Cursor>>3)) &= ~(1<<(7-(m_Cursor & 0x07)));
    
    m_Cursor++;
    m_HighestBitWritten = MAX( m_Cursor-1, m_HighestBitWritten );

    return Value;
}

//=========================================================================

xbool   bitstream::ReadFlag        ( void ) const
{
//  ASSERT( m_Cursor < m_DataSizeInBits );
    if( m_Cursor >= m_DataSizeInBits )
        return( FALSE );
    xbool B = (*(m_Data + (m_Cursor >> 3)) & (1 << (7-(m_Cursor & 0x7)))) != 0; 
    m_Cursor++;
    return B;
}

//=========================================================================

void    bitstream::WriteRawBits       ( const void* pData, s32 NBits )
{
    // Check that we actually have bits to write
    ASSERT(NBits>0);

    // Check if writing out of bounds
    if( (NBits+m_Cursor) >= m_DataSizeInBits )
    {
        m_Overwrite = TRUE;
        return;
    }

    s32     NBitsRemaining = NBits;
    s32     NBitsInBuffer = 0;
    u32     BitBuffer=0;
    byte*   pSrc = (byte*)pData;
    byte*   pDst = m_Data + (m_Cursor>>3);
    s32     DstOffset = (m_Cursor & 0x7);

    while( NBitsRemaining )
    {
        // Determine how many bits we can write to dst
        s32 NBitsToWrite = MIN(NBitsRemaining,8-DstOffset);

        // Get at least that many bits in buffer
        while( NBitsInBuffer < NBitsToWrite )
        {
            BitBuffer |= ((u32)(*pSrc)) << NBitsInBuffer;
            pSrc++;
            NBitsInBuffer+=8;
        }

        // Write out bits to dest
        
        // Get mask highlighting dst bits to be overwritten
        u8 Mask = (0xFF>>DstOffset) & (0xFF<<(8-(DstOffset+NBitsToWrite)));
        u8 Byte = (BitBuffer&(0xFF>>(8-NBitsToWrite))) << ((8-DstOffset)-NBitsToWrite);

        Byte = ((*pDst) & (~Mask)) | (Byte & Mask);

        *pDst++ = Byte;
        DstOffset = 0;
        BitBuffer >>= NBitsToWrite;
        NBitsInBuffer -= NBitsToWrite;
        NBitsRemaining -= NBitsToWrite;
    }

    m_Cursor += NBits;
    m_HighestBitWritten = MAX( m_Cursor-1, m_HighestBitWritten );
}

//=========================================================================

void    bitstream::ReadRawBits        ( void* pData, s32 NBits ) const
{
    // Check that we actually have bits to read
    if( NBits <= 0 )
        return;

    // Check if reading out of bounds
    ASSERTS( (NBits+m_Cursor) <= m_DataSizeInBits, 
             (const char*)xfs( "NBits:%d  m_Cursor:%d  m_DataSizeInBits:%d", 
                               NBits, m_Cursor, m_DataSizeInBits ) );

    s32     NBitsRemaining = NBits;
    s32     NBitsInBuffer = 0;
    u32     BitBuffer=0;
    s32     SrcOffset = (m_Cursor & 0x7);
    byte*   pSrc = m_Data + (m_Cursor>>3);
    byte*   pDst = (byte*)pData;

    while( NBitsRemaining )
    {
        // Determine how many bits we can read from source
        s32 NBitsToRead = MIN(NBitsRemaining,8-SrcOffset);

        // Read bits into the buffer
        byte Byte = (*pSrc) >> ((8-SrcOffset)-NBitsToRead);
        BitBuffer |= (Byte & (~(-1<<NBitsToRead))) << NBitsInBuffer;

        pSrc++;
        SrcOffset=0;
        NBitsInBuffer += NBitsToRead;

        // Empty buffer if we've got a byte's worth
        while( NBitsInBuffer >= 8 )
        {
            *pDst++ = (BitBuffer&0xFF);
            BitBuffer>>=8;
            NBitsInBuffer -= 8;
        }

        NBitsRemaining -= NBitsToRead;
    }

    // Empty buffer if we've got a byte's worth
    if( NBitsInBuffer )
    {
        *pDst++ = BitBuffer;
    }

    m_Cursor += NBits;
}

//=========================================================================
#if defined(TARGET_LINUX)
#define LONG_MASK 0xFFFFFFFFFFLL
#else
#define LONG_MASK 0xFFFFFFFFFF
#endif
void    bitstream::WriteRaw32         ( u32 Value, s32 NBits )
{
    // Check that we actually have bits to write
    ASSERT((NBits>0) && (NBits<=32));

    // Check if writing out of bounds
    if( (NBits+m_Cursor) >= m_DataSizeInBits )
    {
        m_Overwrite = TRUE;
        return;
    }

    byte*   pDst        = m_Data + (m_Cursor>>3);
    byte*   pEnd        = m_Data + ((m_Cursor+NBits-1)>>3) + 1;
    s32     LeftOffset  = (m_Cursor & 0x7);
    s32     RightOffset = (40-LeftOffset-NBits);

    // Move cursor
    m_Cursor += NBits;
    m_HighestBitWritten = MAX( m_Cursor-1, m_HighestBitWritten );

    // Get mask highlighting bits that will be overwritten
    u64 WriteMask = (LONG_MASK >> LeftOffset) & 
                    (LONG_MASK << RightOffset);

    // Get data to align with mask
    u64 DataMask  = (((u64)Value) << RightOffset) & WriteMask;

    WriteMask = ~WriteMask;

    // Write out bytes
    s32 Shift = 32;
    while( pDst != pEnd )
    {
        *pDst = (byte)((DataMask>>Shift) | ((*pDst)&(WriteMask>>Shift)));
        pDst++;
        Shift-=8;
    }
}

//=========================================================================

u32    bitstream::ReadRaw32          ( s32 NBits ) const
{
    // Check that we actually have bits to read
    ASSERT((NBits>0) && (NBits<=32));

    // Check if reading out of bounds
    ASSERTS( (NBits+m_Cursor) <= m_DataSizeInBits, 
             (const char*)xfs( "NBits:%d  m_Cursor:%d  m_DataSizeInBits:%d", 
                               NBits, m_Cursor, m_DataSizeInBits ) );

    s32     LeftOffset  = (m_Cursor & 0x7);
    s32     RightOffset = (40-LeftOffset-NBits);
    byte*   pSrc        = m_Data + (m_Cursor>>3);
    byte*   pEnd        = m_Data + ((m_Cursor+NBits-1)>>3) + 1;

    // Move cursor
    m_Cursor += NBits;

    // Get mask highlighting bits that will be read
    u64 ReadMask = (LONG_MASK >> LeftOffset) & 
                   (LONG_MASK << RightOffset);

    // Clear data
    u64 DataMask = 0;

    // Read data
    s32 Shift = 32;
    while( pSrc != pEnd )
    {
        DataMask |= ((u64)(*pSrc++))<<Shift;
        Shift-=8;
    }

    u32 Value = (u32)((DataMask & ReadMask) >> RightOffset);

    return Value;
}

//=============================================================================
// Bitstream network helper functions
//=============================================================================
xbool   bitstream::Receive(net_address&  CallerAddress,
                          net_address&  SenderAddress)
{
    s32 NBytes = GetNBytes();
	u8* pData;
	xbool Received;
	u16 checksum;

	pData = GetDataPtr();
	Received = net_Receive( CallerAddress, SenderAddress,
                        pData,
                        NBytes );
	if ( (!Received) || (NBytes < 2) )
	{
		return FALSE;
	}

	NBytes-=1;

	checksum = net_Checksum(pData,NBytes);

	if ((u8)checksum != (u8)pData[NBytes])
	{
		x_DebugMsg("bitstream::Receive - Checksum failure on a packet\n");
		return FALSE;
	}
	return TRUE;
}

//=============================================================================
void    bitstream::Send        ( const net_address&  CallerAddress, 
                          const net_address&  DestAddress)
{
	u8* pData;
	s32 length;

	pData = GetDataPtr();
	length = GetNBytesUsed();

	pData[length] = net_Checksum(pData,length);
	length++;
    net_Send( CallerAddress, DestAddress, GetDataPtr(), length );
}

//=============================================================================
void    bitstream::Encrypt(s32 EncryptionKey)
{
    (void)EncryptionKey;
}

//=============================================================================
xbool   bitstream::Decrypt(s32 EncryptionKey)
{
    (void)EncryptionKey;
    return TRUE;
}

