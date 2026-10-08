// Reconstructed PC bitmap I/O; provenance and differences in docs/x-bitmap-io.md.

#include <rva.h>

#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_string.hpp>

struct io_buffer
{
    int DataSize, ClutSize, Width, Height, PW;
    unsigned int Flags;
    int NMips;
    xbitmap::format Format;
};

static int xbmp_Save(const xbitmap& bitmap, X_FILE* file);

RVA(0x00252630, 0x11)
int xbitmap::Save(X_FILE* file) const
{
    return xbmp_Save(*this, file);
}

RVA(0x00252650, 0xe1)
static int xbmp_Save(const xbitmap& bitmap, X_FILE* file)
{
    io_buffer header;
    header.DataSize = bitmap.GetDataSize();
    header.ClutSize = bitmap.GetClutSize();
    header.Width = bitmap.GetWidth();
    header.Height = bitmap.GetHeight();
    header.PW = bitmap.GetPWidth();
    header.Flags = bitmap.GetFlags();
    header.NMips = bitmap.GetNMips();
    header.Format = bitmap.GetFormat();
    int written = x_fwrite(&header, 1, sizeof(header), file);
    if (written != sizeof(header))
        return 0;
    int bytes = bitmap.GetDataSize();
    written = x_fwrite(bitmap.GetPixelData(-1), 1, bytes, file);
    if (written != bytes)
        return 0;
    bytes = bitmap.GetClutSize();
    if (bytes)
    {
        written = x_fwrite(bitmap.GetClutData(), 1, bytes, file);
        if (written != bytes)
            return 0;
    }
    return 1;
}

RVA(0x00252740, 0x3b)
int xbitmap::Save(const char* filename) const
{
    X_FILE* file = x_fopen(filename, "wb");
    if (!file)
        return 0;
    int result = Save(file);
    x_fclose(file);
    return result;
}

RVA(0x00252780, 0xf0)
int xbitmap::Load(X_FILE* file)
{
    int result = 0;
    io_buffer header;
    Kill();
    if (x_fread(&header, 1, sizeof(header), file) != sizeof(header))
        goto done;
    m_DataSize = header.DataSize;
    m_ClutSize = header.ClutSize;
    m_Width = static_cast<short>(header.Width);
    m_Height = static_cast<short>(header.Height);
    m_PW = static_cast<short>(header.PW);
    m_Flags = static_cast<unsigned short>(header.Flags);
    m_NMips = static_cast<signed char>(header.NMips);
    m_Format = static_cast<signed char>(header.Format);
    m_Data.pPixel = static_cast<unsigned char*>(x_malloc_fn(m_DataSize, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_io.cpp", 203));
    m_Flags |= FLAG_DATA_OWNED;
    if (x_fread(m_Data.pPixel, 1, m_DataSize, file) != m_DataSize)
        goto fail;
    if (m_ClutSize)
    {
        m_pClut = static_cast<unsigned char*>(x_malloc_fn(m_ClutSize, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_io.cpp", 214));
        m_Flags |= FLAG_CLUT_OWNED;
        if (x_fread(m_pClut, 1, m_ClutSize, file) != m_ClutSize)
            goto fail;
    }
    result = 1;
    goto done;
fail:
    Kill();
done:
    return result;
}

RVA(0x00252870, 0x3b)
int xbitmap::Load(const char* filename)
{
    X_FILE* file = x_fopen(filename, "rb");
    if (!file)
        return 0;
    int result = Load(file);
    x_fclose(file);
    return result;
}

// Reconstructed names for the PC-only two-bit-group and DXT1 endian helpers.
RVA(0x002528b0, 0x16)
unsigned char Swap2BitPairs(unsigned char value)
{
    return ((value >> 2) & 0x33) | ((value & 0x33) << 2);
}

RVA(0x00252b30, 0xa3)
static void ToggleEndianDXT(void* input, int count)
{
    unsigned char* data = static_cast<unsigned char*>(input);
    for (int i = 0; i < count; i += 8)
    {
        *reinterpret_cast<unsigned short*>(data) = ENDIAN_SWAP_16(*reinterpret_cast<unsigned short*>(data));
        data += 2;
        *reinterpret_cast<unsigned short*>(data) = ENDIAN_SWAP_16(*reinterpret_cast<unsigned short*>(data));
        data += 2;
        data[0] = Swap2BitPairs((data[0] >> 4) | (data[0] << 4));
        data[1] = Swap2BitPairs((data[1] >> 4) | (data[1] << 4));
        data[2] = Swap2BitPairs((data[2] >> 4) | (data[2] << 4));
        data[3] = Swap2BitPairs((data[3] >> 4) | (data[3] << 4));
        data += 4;
    }
}
RVA(0x00252ac0, 0x23)
static void ToggleEndian16( unsigned short* pData, int Count )
{
    while( Count )
    {
        *pData = ENDIAN_SWAP_16( *pData );
        pData++;
        Count--;
    }
}

RVA(0x00252af0, 0x3e)
static void ToggleEndian32( unsigned int* pData, int Count )
{
    while( Count )
    {
        *pData = ENDIAN_SWAP_32( *pData );
        pData++;
        Count--;
    }
}

RVA(0x002528d0, 0x1eb)
void xbitmap::ToggleEndian( void )
{

    if( (m_FormatInfo[ m_Format ].ClutBased) &&
        (m_FormatInfo[ m_Format ].BPC == 16) )
    {

        ToggleEndian16( (unsigned short*)m_pClut, m_ClutSize >> 1 );
    }

    if( (m_FormatInfo[ m_Format ].ClutBased) &&
        (m_FormatInfo[ m_Format ].BPC == 32) )
    {
        ToggleEndian32( (unsigned int*)m_pClut, m_ClutSize >> 2 );
    }

    if( m_NMips == 0 )
    {

        if (IsCompressed())
        {
            ToggleEndianDXT(m_Data.pPixel, m_DataSize);
            return;
        }
        if (m_Flags & FLAG_GCN_DATA_SWIZZLED) return;
        if( m_FormatInfo[ m_Format ].BPP == 16 )
            ToggleEndian16( (unsigned short*)m_Data.pPixel, m_PW * m_Height );

        if( m_FormatInfo[ m_Format ].BPP == 32 )
            ToggleEndian32( (unsigned int*)m_Data.pPixel, m_PW * m_Height );
    }
    else
    {
        int i;

        for( i = 0; i < m_NMips+1; i++ )
        {
            m_Data.pMip[i].Offset = ENDIAN_SWAP_32( m_Data.pMip[i].Offset );
            m_Data.pMip[i].Width  = ENDIAN_SWAP_16( m_Data.pMip[i].Width  );
            m_Data.pMip[i].Height = ENDIAN_SWAP_16( m_Data.pMip[i].Height );
            if (IsCompressed())
                ToggleEndianDXT(m_Data.pPixel + m_Data.pMip[i].Offset, (m_Data.pMip[i].Width * m_Data.pMip[i].Height) / 2);
            else if (!(m_Flags & FLAG_GCN_DATA_SWIZZLED))
            {

            if( m_FormatInfo[ m_Format ].BPP == 16 )
                ToggleEndian16( (unsigned short*)(m_Data.pPixel + m_Data.pMip[i].Offset),
                                m_Data.pMip[i].Width * m_Data.pMip[i].Height );

            if( m_FormatInfo[ m_Format ].BPP == 32 )
                ToggleEndian32( (unsigned int*)(m_Data.pPixel + m_Data.pMip[i].Offset),
                                m_Data.pMip[i].Width * m_Data.pMip[i].Height );

            }
        }
    }
}

RVA(0x00252ed0, 0x69)
static void xbmp_Dump32( X_FILE* pFile, unsigned int* pData, int PerRow, int Rows )
{
    int i;
    while( Rows-- )
    {
        x_fprintf( pFile, "   " );
        for( i = 0; i < PerRow; i++ )
        {
            x_fprintf( pFile, " 0x%08X,", *pData );
            pData++;
        }
        x_fprintf( pFile, "\n" );
    }
}

RVA(0x00252f40, 0x74)
static void xbmp_Dump24( X_FILE* pFile, unsigned char* pData, int PerRow, int Rows )
{
    int i;
    while( Rows-- )
    {
        x_fprintf( pFile, "  " );
        for( i = 0; i < PerRow; i++ )
        {
            x_fprintf( pFile, "  0x%02X,0x%02X,0x%02X,",
                              pData[0], pData[1], pData[2] );
            pData += 3;
        }
        x_fprintf( pFile, "\n" );
    }
}

RVA(0x00252fc0, 0x6a)
static void xbmp_Dump16( X_FILE* pFile, unsigned short* pData, int PerRow, int Rows )
{
    int i;
    while( Rows-- )
    {
        x_fprintf( pFile, "   " );
        for( i = 0; i < PerRow; i++ )
        {
            x_fprintf( pFile, " 0x%04X,", *pData );
            pData++;
        }
        x_fprintf( pFile, "\n" );
    }
}

RVA(0x00253030, 0x68)
static void xbmp_Dump8( X_FILE* pFile, unsigned char* pData, int PerRow, int Rows )
{
    int i;
    while( Rows-- )
    {
        x_fprintf( pFile, "    " );
        for( i = 0; i < PerRow; i++ )
        {
            x_fprintf( pFile, "0x%02X,", *pData );
            pData++;
        }
        x_fprintf( pFile, "\n" );
    }
}

RVA(0x002530a0, 0x6c)
static void xbmp_Dump4( X_FILE* pFile, unsigned char* pData, int PerRow, int Rows )
{
    int i;
    while( Rows-- )
    {
        x_fprintf( pFile, "    " );
        for( i = 0; i < PerRow; i += 2 )
        {
            x_fprintf( pFile, "0x%02X,", *pData );
            pData++;
        }
        x_fprintf( pFile, "\n" );
    }
}

RVA(0x00252be0, 0x273)
int xbitmap::DumpSourceCode( const char* pFileName ) const
{
    X_FILE* pFile;
    int     DataSize;
    unsigned char*   pData;

    pFile = x_fopen( pFileName, "wt" );
    if( !pFile )
        return( 0 );

    DataSize = (m_PW * m_Height * m_FormatInfo[m_Format].BPP) >> 3;
    pData    = m_NMips ?
               m_Data.pPixel + m_Data.pMip[0].Offset :
               m_Data.pPixel;

    x_fprintf( pFile, "// Format   : %6d (%s)\n",
                      (int)m_Format,
                      m_FormatInfo[m_Format].String );
    x_fprintf( pFile, "// Width    : %6d pixels\n", (int)m_Width    );
    x_fprintf( pFile, "// Height   : %6d pixels\n", (int)m_Height   );
    x_fprintf( pFile, "// PW       : %6d pixels\n", (int)m_PW       );
    x_fprintf( pFile, "// DataSize : %6d bytes\n",       DataSize   );
    x_fprintf( pFile, "// ClutSize : %6d bytes\n",       m_ClutSize );
    x_fprintf( pFile, "// Flags    : 0x%04X\n",          m_Flags    );
    x_fprintf( pFile, "\n" );

    if( m_ClutSize )
    {
        int Rows;

        if( m_FormatInfo[ m_Format ].BPP == 4 )   Rows = 1;
        else                                      Rows = 16;

        switch( m_FormatInfo[ m_Format ].BPC )
        {
        case 32:    x_fprintf( pFile, "u32  Clut[] =\n{\n" );  break;
        case 24:    x_fprintf( pFile, "byte Clut[] =\n{\n" );  break;
        case 16:    x_fprintf( pFile, "u16  Clut[] =\n{\n" );  break;
        default: break;
        }

        switch( m_FormatInfo[ m_Format ].BPC )
        {
        case 32:    xbmp_Dump32( pFile, (unsigned int*)m_pClut, 16, Rows );  break;
        case 24:    xbmp_Dump24( pFile,       m_pClut, 16, Rows );  break;
        case 16:    xbmp_Dump16( pFile, (unsigned short*)m_pClut, 16, Rows );  break;
        default: break;
        }

        x_fprintf( pFile, "};\n\n" );
    }

    switch( m_FormatInfo[ m_Format ].BPP )
    {
    case 32:    x_fprintf( pFile, "u32  Data[] =\n{\n" );  break;
    case 24:    x_fprintf( pFile, "byte Data[] =\n{\n" );  break;
    case 16:    x_fprintf( pFile, "u16  Data[] =\n{\n" );  break;
    case  8:    x_fprintf( pFile, "byte Data[] =\n{\n" );  break;
    case  4:    x_fprintf( pFile, "byte Data[] =\n{\n" );  break;
    default: break;
    }

    switch( m_FormatInfo[ m_Format ].BPP )
    {
    case 32:    xbmp_Dump32( pFile, (unsigned int*)pData, m_PW, m_Height );  break;
    case 24:    xbmp_Dump24( pFile,       pData, m_PW, m_Height );  break;
    case 16:    xbmp_Dump16( pFile, (unsigned short*)pData, m_PW, m_Height );  break;
    case  8:    xbmp_Dump8 ( pFile,       pData, m_PW, m_Height );  break;
    case  4:    xbmp_Dump4 ( pFile,       pData, m_PW, m_Height );  break;
    default: break;
    }

    x_fprintf( pFile, "};\n\n" );
    x_fclose ( pFile );
    return( 1 );
}

RVA(0x00253110, 0x130)
int xbitmap::SavePaletteTGA( const char* pFileName ) const
{

    if( GetBPP() > 8 )
        return 0;

    int W = (1 << GetBPP());
    int H = 32;

    xcolor* pData = static_cast<xcolor*>(x_malloc_fn(sizeof(xcolor)*W*H, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_io.cpp", 628));

    xbitmap BMP;
    BMP.Setup( xbitmap::FMT_XCOLOR, W, H, 0, reinterpret_cast<unsigned char*>(pData), 0, 0, -1, 0 );

    for( int x=0; x<W; x++ )
    {
        xcolor C = GetClutColor(x);
        for( int y=0; y<H; y++ )
            pData[ x+y*W ] = C;
    }

    int Result = BMP.SaveTGA( pFileName );

    x_free_fn(pData, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_io.cpp", 643);
    return Result;
}

RVA(0x00253240, 0x121)
int xbitmap::SaveTGA( const char* pFileName ) const
{
    X_FILE* pFile;
    format  Format;
    unsigned char    Header[18];
    xbitmap Clone( *this );
    Clone.m_VRAMID = 0;

    Format = FMT_32_ARGB_8888;

    Clone.BuildMips( 0, 0 );
    Clone.ConvertFormat( Format );

    x_memset( Header, 0, 18 );
    Header[ 2] = 2;
    Header[12] = (Clone.m_PW     >> 0) & 0xFF;
    Header[13] = (Clone.m_PW     >> 8) & 0xFF;
    Header[14] = (Clone.m_Height >> 0) & 0xFF;
    Header[15] = (Clone.m_Height >> 8) & 0xFF;
    Header[16] = 32;
    Header[17] = 32;

    pFile = x_fopen( pFileName, "wb" );
    if( !pFile )
        return( 0 );

    x_fwrite( Header, 1, 18, pFile );
    x_fwrite( Clone.m_Data.pPixel, 1, Clone.m_PW * Clone.m_Height * 4, pFile );
    x_fclose( pFile );

    return( 1 );
}

RVA(0x00253370, 0x19d)
int xbitmap::SaveMipsTGA( const char* pFileName ) const
{
    char FileName[ X_MAX_PATH  ];
    char Drive   [ X_MAX_DRIVE ];
    char Dir     [ X_MAX_DIR   ];
    char FName   [ X_MAX_FNAME ];
    char Ext     [ X_MAX_EXT   ];
    int  i;

    if( m_NMips == 0 )
    {
        return( SaveTGA( pFileName ) );
    }

    x_splitpath( pFileName, Drive, Dir, FName, Ext );

    for( i = 0; i <= m_NMips; i++ )
    {
        xbitmap Temp;
        int   Result;

        Temp.Setup( GetFormat(),
                    GetWidth(i),
                    GetHeight(i),
                    0,
                    const_cast<unsigned char*>(GetPixelData(i)),
                    0,
                    const_cast<unsigned char*>(GetClutData()), -1, 0 );

        x_makepath( FileName, Drive, Dir, xfs( "%s%02d", FName, i ), Ext );
        Result = Temp.SaveTGA( FileName );

        if( !Result )
            return( 0 );
    }

    return( 1 );
}



// Genuine sibling metadata reader; PC address not yet established.
int xbitmap::Info( const char* pFileName, info& BitmapInfo )
{
    X_FILE*     pFile;
    int       Success = 0;
    io_buffer   Buffer;
    int         BytesRead;



    pFile = x_fopen( pFileName, "rb" );
    if( pFile )
    {
        // Read in our simple io_buffer for the header data.
        BytesRead = x_fread( &Buffer, 1, sizeof(io_buffer), pFile );
        if( BytesRead == sizeof(io_buffer) )
        {
            // Make sure we have correctly read Little Endian data.
            #ifdef BIG_ENDIAN
            Buffer.DataSize = ENDIAN_SWAP_32( Buffer.DataSize );
            Buffer.ClutSize = ENDIAN_SWAP_32( Buffer.ClutSize );
            Buffer.Width    = ENDIAN_SWAP_32( Buffer.Width    );
            Buffer.Height   = ENDIAN_SWAP_32( Buffer.Height   );
            Buffer.PW       = ENDIAN_SWAP_32( Buffer.PW       );
            Buffer.Flags    = ENDIAN_SWAP_32( Buffer.Flags    );
            Buffer.NMips    = ENDIAN_SWAP_32( Buffer.NMips    );
            Buffer.Format   = (xbitmap::format)ENDIAN_SWAP_32( Buffer.Format   );
            #endif

            // Close the file
            x_fclose( pFile );

            // Fill in the info structure
            BitmapInfo.W      = Buffer.Width;
            BitmapInfo.H      = Buffer.Height;
            BitmapInfo.nMips  = Buffer.NMips;
            BitmapInfo.Format = Buffer.Format;
            Success = 1;
        }
    }

    return( Success );
}

