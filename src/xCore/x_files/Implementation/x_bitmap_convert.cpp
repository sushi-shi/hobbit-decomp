// PC bitmap conversion and mip generation; see docs/x-bitmap-convert.md.

#include <rva.h>

#include <xCore/x_files/Implementation/x_bitmap_private.hpp>
#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

#define ASSERT(x) ((void)0)
#define ALIGN_32(x) (((x)+31)&~31)
RVA(0x002541e0, 0x327)
static
xcolor* xbmp_DecodeToColor( const unsigned char*           pSource,
                                  xbitmap::format SourceFormat,
                                  int             Count )
{
    xcolor* pWrite;
    xcolor* pResult;
    const xbitmap::format_info& Format = xbitmap::GetFormatInfo( SourceFormat );
    pResult = (xcolor*)x_malloc_fn( Count * sizeof( xcolor ) , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 46);
    pWrite  = pResult;
    if( Format.BPC == 32 )
    {
        unsigned int* pRead = (unsigned int*)pSource;
        while( Count > 0 )
        {
            pWrite->R  = (unsigned char)((*pRead & Format.RMask) >> Format.RShiftR);
            pWrite->G  = (unsigned char)((*pRead & Format.GMask) >> Format.GShiftR);
            pWrite->B  = (unsigned char)((*pRead & Format.BMask) >> Format.BShiftR);
            pWrite->A  = (unsigned char)((*pRead & Format.AMask) >> Format.AShiftR);
            pWrite->A |= (unsigned char)((*pRead & Format.UMask) >> Format.UShiftR);
            pRead++;
            pWrite++;
            Count--;
        }
    }
    if( Format.BPC == 24 )
    {
        unsigned char* pRead = (unsigned char*)pSource;
        if( Format.RShiftR == 16 )
        {
            while( Count > 0 )
            {
                pWrite->R = *pRead++;
                pWrite->G = *pRead++;
                pWrite->B = *pRead++;
                pWrite->A = 255;
                pWrite++;
                Count--;
            }
        }
        if( Format.RShiftR == 0 )
        {
            while( Count > 0 )
            {
                pWrite->B = *pRead++;
                pWrite->G = *pRead++;
                pWrite->R = *pRead++;
                pWrite->A = 255;
                pWrite++;
                Count--;
            }
        }
    }
    if( Format.BPC == 16 )
    {
        unsigned short* pRead;
        unsigned int  R, G, B, A;
        int  Bits;
        while( Count > 0 )
        {
            pRead = (unsigned short*)pSource;
            R   = *pRead;
            R  &= Format.RMask;
            R >>= Format.RShiftR;
            R <<= Format.RShiftL;
            R  |= (R >> Format.RBits);
            G   = *pRead;
            G  &= Format.GMask;
            G >>= Format.GShiftR;
            G <<= Format.GShiftL;
            G  |= (G >> Format.GBits);
            B   = *pRead;
            B  &= Format.BMask;
            B >>= Format.BShiftR;
            B <<= Format.BShiftL;
            B  |= (B >> Format.BBits);
            if( Format.ABits )
            {
                A   = *pRead;
                A  &= Format.AMask;
                A >>= Format.AShiftR;
                A <<= Format.AShiftL;
                Bits = Format.ABits;
                while( Bits < 8 )
                {
                    A |= (A >> Bits);
                    Bits <<= 1;
                }
            }
            else
            {
                A = 0xFF;
            }
            pWrite->Set( (unsigned char)R, (unsigned char)G, (unsigned char)B, (unsigned char)A );
            pSource += 2;
            pWrite++;
            Count--;
        }
    }
    if( Format.BPC == 8 )
    {
        unsigned char* pRead;
        unsigned int  R, G, B, A;
        int  Bits;
        while( Count > 0 )
        {
            pRead = (unsigned char*)pSource;
            R   = *pRead;
            R  &= Format.RMask;
            R >>= Format.RShiftR;
            R <<= Format.RShiftL;
            R  |= (R >> Format.RBits);
            G   = *pRead;
            G  &= Format.GMask;
            G >>= Format.GShiftR;
            G <<= Format.GShiftL;
            G  |= (G >> Format.GBits);
            B   = *pRead;
            B  &= Format.BMask;
            B >>= Format.BShiftR;
            B <<= Format.BShiftL;
            B  |= (B >> Format.BBits);
            if( Format.ABits )
            {
                A   = *pRead;
                A  &= Format.AMask;
                A >>= Format.AShiftR;
                A <<= Format.AShiftL;
                Bits = Format.ABits;
                while( Bits < 8 )
                {
                    A |= (A >> Bits);
                    Bits <<= 1;
                }
            }
            else
            {
                A = 0xFF;
            }
            pWrite->Set( (unsigned char)R, (unsigned char)G, (unsigned char)B, (unsigned char)A );
            pSource += 1;
            pWrite++;
            Count--;
        }
    }
    return( pResult );
}

RVA(0x00254510, 0x1d8)
static
void xbmp_EncodeFromColor(       unsigned char*           pDestination,
                                 xbitmap::format DestinationFormat,
                           const xcolor*         pSource,
                                 int             Count )
{
    const xcolor* pRead = pSource;
    const xbitmap::format_info& Format = xbitmap::GetFormatInfo( DestinationFormat );
    unsigned int R, G, B, A, U;
    U = Format.UMask;
    if( Format.BPC == 32 )
    {
        unsigned int* pWrite = (unsigned int*)pDestination;
        while( Count > 0 )
        {
            R = (unsigned int)pRead->R;
            G = (unsigned int)pRead->G;
            B = (unsigned int)pRead->B;
            A = (unsigned int)pRead->A;
            R >>= Format.RShiftL;
            R <<= Format.RShiftR;
            G >>= Format.GShiftL;
            G <<= Format.GShiftR;
            B >>= Format.BShiftL;
            B <<= Format.BShiftR;
            A >>= Format.AShiftL;
            A <<= Format.AShiftR;
            A  &= Format.AMask;
            *pWrite = R | G | B | A | U;
            pWrite++;
            pRead++;
            Count--;
        }
    }
    if( Format.BPC == 24 )
    {
        unsigned char* pWrite = pDestination;
        if( Format.RShiftR == 16 )
        {
            while( Count > 0 )
            {
                *pWrite++ = pRead->R;
                *pWrite++ = pRead->G;
                *pWrite++ = pRead->B;
                pRead++;
                Count--;
            }
        }
        if( Format.RShiftR == 0 )
        {
            while( Count > 0 )
            {
                *pWrite++ = pRead->B;
                *pWrite++ = pRead->G;
                *pWrite++ = pRead->R;
                pRead++;
                Count--;
            }
        }
    }
    if( Format.BPC == 16 )
    {
        unsigned int  Encoded;
        unsigned short* pWrite = (unsigned short*)pDestination;
        while( Count > 0 )
        {
            R = (unsigned int)pRead->R;
            G = (unsigned int)pRead->G;
            B = (unsigned int)pRead->B;
            A = (unsigned int)pRead->A;
            R >>= Format.RShiftL;
            R <<= Format.RShiftR;
            G >>= Format.GShiftL;
            G <<= Format.GShiftR;
            B >>= Format.BShiftL;
            B <<= Format.BShiftR;
            A >>= Format.AShiftL;
            A <<= Format.AShiftR;
            A  &= Format.AMask;
            Encoded = R | G | B | A | U;
            *pWrite = (unsigned short)Encoded;
            pWrite++;
            pRead++;
            Count--;
        }
    }
    if( Format.BPC == 8 )
    {
        unsigned int  Encoded;
        unsigned char* pWrite = (unsigned char*)pDestination;
        while( Count > 0 )
        {
            R = (unsigned int)pRead->R;
            G = (unsigned int)pRead->G;
            B = (unsigned int)pRead->B;
            A = (unsigned int)pRead->A;
            R >>= Format.RShiftL;
            R <<= Format.RShiftR;
            G >>= Format.GShiftL;
            G <<= Format.GShiftR;
            B >>= Format.BShiftL;
            B <<= Format.BShiftR;
            A >>= Format.AShiftL;
            A <<= Format.AShiftR;
            A  &= Format.AMask;
            Encoded = R | G | B | A | U;
            *pWrite = (unsigned char)Encoded;
            pWrite++;
            pRead++;
            Count--;
        }
    }
}

RVA(0x002546f0, 0xde)
static
xcolor* xbmp_DecodeIndexToColor( const unsigned char*   pSource,
                                 const xcolor* pPalette,
                                       int     SourceBitsPer,
                                       int     Count )
{
    xcolor*     pWrite;
    xcolor*     pResult;
    const unsigned char* pRead = pSource;
    pResult = (xcolor*)x_malloc_fn( Count * sizeof( xcolor ) , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 432);
    pWrite  = pResult;
    if( SourceBitsPer == 4 )
    {
        while( Count > 0 )
        {
            *pWrite++ = pPalette[ *pRead >> 4    ];
            *pWrite++ = pPalette[ *pRead &  0x0F ];
            pRead += 1;
            Count -= 2;
        }
    }
    if( SourceBitsPer == 8 )
    {
        while( Count > 0 )
        {
            *pWrite++ = pPalette[ *pRead++ ];
            Count--;
        }
    }
    return( pResult );
}

RVA(0x002547d0, 0x66)
static
void xbmp_EncodeColorToIndex(       unsigned char*   pDestination,
                                    int     DestinationBitsPer,
                              const xcolor* pSource,
                                    int     Count )
{
    unsigned char*         pWrite = pDestination;
    const xcolor* pRead  = pSource;
    while( Count > 0 )
    {
        int BestIndex = cmap_GetIndex( *pRead );
        if( DestinationBitsPer == 4 )
        {
            if( Count & 1 )
            {
                *pWrite |= BestIndex;
                pWrite++;
            }
            else
            {
                *pWrite = (unsigned char)(BestIndex << 4);
            }
        }
        else
        {
            *pWrite = (unsigned char)BestIndex;
            pWrite++;
        }
        pRead++;
        Count--;
    }
}

RVA(0x00254840, 0x8f)
static
void xbmp_ConvertIndexData(       unsigned char* pDestination,
                                  int   DestinationBitsPer,
                            const unsigned char* pSource,
                                  int   SourceBitsPer,
                                  int   Count )
{
    if( DestinationBitsPer == SourceBitsPer )
    {
        x_memcpy( pDestination, pSource, (Count * SourceBitsPer) >> 3 );
    }
    if( (DestinationBitsPer == 8) && (SourceBitsPer == 4) )
    {
        while( Count > 0 )
        {
            *pDestination++ = *pSource >> 4;
            *pDestination++ = *pSource &  0x0F;
            pSource++;
            Count -= 2;
        }
    }
    if( (DestinationBitsPer == 4) && (SourceBitsPer == 8) )
    {
        while( Count > 0 )
        {
            *pDestination  = *pSource++ << 4;
            *pDestination |= *pSource++ &  0x0F;
            pDestination++;
            Count -= 2;
        }
    }
}

RVA(0x00253e20, 0x3bd)
void xbitmap::ConvertFormat( xbitmap::format DestinationFormat )
{
    const format_info& OldFormat = m_FormatInfo[ m_Format ];
    const format_info& NewFormat = m_FormatInfo[ DestinationFormat ];
    ASSERT( (DestinationFormat > FMT_NULL       ) &&
            (DestinationFormat < FMT_END_OF_LIST) );
    if( DestinationFormat == m_Format )
        return;
    unsigned char*   pOldData       = 0;
    unsigned char*   pNewData       = 0;
    xcolor* pColorData     = 0;
    int     DataPixels;
    int     DataBytes;
    unsigned char*   pOldPalette    = 0;
    unsigned char*   pNewPalette    = 0;
    xcolor* pColorPalette  = 0;
    int     PaletteEntries = 0;
    int     PaletteBytes   = 0;
    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        PS2UnswizzleClut();
    if ( m_Flags & FLAG_4BIT_NIBBLES_FLIPPED )
        Unflip4BitNibbles() ;
    DataPixels = m_PW * m_Height;
    DataBytes  = (DataPixels * NewFormat.BPP) >> 3;
    pNewData   = (unsigned char*)x_malloc_fn( DataBytes , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 613);
    pOldData   = m_Data.pPixel;
    pOldData  += m_NMips ? m_Data.pMip[0].Offset : 0;
    if( OldFormat.ClutBased )
    {
        PaletteEntries = m_ClutSize / (OldFormat.BPC >> 3);
        pOldPalette    = m_pClut;
        pColorPalette  = xbmp_DecodeToColor( pOldPalette,
                                            (xbitmap::format)m_Format,
                                            PaletteEntries );
    }
    if( !(OldFormat.ClutBased) )
    {
        pColorData = xbmp_DecodeToColor( pOldData,
                                         (xbitmap::format)m_Format,
                                         DataPixels );
    }
    if( !(OldFormat.ClutBased) && (NewFormat.ClutBased) )
    {
        PaletteEntries = (NewFormat.BPP < 8) ? 16 : 256;
        pColorPalette  = (xcolor*)x_malloc_fn( sizeof(xcolor) * PaletteEntries , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 654);
        quant_Begin();
        quant_SetPixels( pColorData, DataPixels );
        quant_End( pColorPalette, PaletteEntries, (NewFormat.ABits>0) );
    }
    if( (OldFormat.ClutBased) && !(NewFormat.ClutBased) )
    {
        pColorData = xbmp_DecodeIndexToColor( pOldData,
                                              pColorPalette,
                                              OldFormat.BPP,
                                              DataPixels );
        x_free_fn( pColorPalette , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 675);
    }
    if( !(NewFormat.ClutBased) )
    {
        xbmp_EncodeFromColor( pNewData,
                              DestinationFormat,
                              pColorData,
                              DataPixels );
        x_free_fn( pColorData , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 692);
    }
    if( (OldFormat.ClutBased) && (NewFormat.ClutBased) )
    {
        xbmp_ConvertIndexData( pNewData, NewFormat.BPP,
                               pOldData, OldFormat.BPP,
                               DataPixels );
        if( (OldFormat.BPP == 8) && (NewFormat.BPP == 4) )
            PaletteEntries = 16;
    }
    if( !(OldFormat.ClutBased) && (NewFormat.ClutBased) )
    {
        PaletteBytes = (PaletteEntries * NewFormat.BPC) >> 3;
        pNewPalette  = (unsigned char*)x_malloc_fn( PaletteBytes , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 720);
        xbmp_EncodeFromColor( pNewPalette,
                              DestinationFormat,
                              pColorPalette,
                              PaletteEntries );
        cmap_Begin( pColorPalette, PaletteEntries, (NewFormat.ABits>0) );
        xbmp_EncodeColorToIndex( pNewData,
                                 NewFormat.BPP,
                                 pColorData,
                                 DataPixels );
        cmap_End();
        x_free_fn( pColorData , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 735);
        x_free_fn( pColorPalette , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 736);
    }
    if( OldFormat.ClutBased && NewFormat.ClutBased )
    {
        int NewPaletteEntries = 1 << NewFormat.BPP;
        PaletteBytes = (NewPaletteEntries * NewFormat.BPC) >> 3;
        pNewPalette  = (unsigned char*)x_malloc_fn( PaletteBytes , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 751);
        xbmp_EncodeFromColor( pNewPalette,
                              DestinationFormat,
                              pColorPalette,
                              PaletteEntries );
        x_free_fn( pColorPalette , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 758);
    }
    if( m_Flags & FLAG_DATA_OWNED )
        x_free_fn( m_Data.pPixel , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 769);
    if( (m_Flags & FLAG_CLUT_OWNED) && (m_pClut) )
        x_free_fn( m_pClut , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 773);
    m_Data.pPixel = pNewData;
    m_DataSize    = DataBytes;
    m_NMips       = 0;
    m_Flags      |= FLAG_DATA_OWNED;
    if( NewFormat.ClutBased )
    {
        m_pClut    = pNewPalette;
        m_ClutSize = PaletteBytes;
        m_Flags   |= FLAG_CLUT_OWNED;
    }
    else
    {
        m_pClut    = 0;
        m_ClutSize = 0;
        m_Flags   &= ~FLAG_CLUT_OWNED;
    }
    m_Format = (signed char)DestinationFormat;
}

DATA(0x003e86ec)
int g_CompileMipTest = 0;
RVA_DYNINIT(0x002548d0, 0x5, s_MipTestColor)
RVA_DYNINIT(0x002548e0, 0x9a, s_MipTestColor)
DATA(0x003e86d0)
static xcolor s_MipTestColor[7] = { xcolor(255,255,255), xcolor(255,0,0), xcolor(0,255,0), xcolor(0,0,255), xcolor(255,255,0), xcolor(255,0,255), xcolor(0,255,255) };
RVA(0x00254e30, 0x1e1)
static
xcolor* xbmp_GenerateColorMip( const xcolor* pSource, int W, int H, int Mip )
{
    int     Shift;
    int     x, y;
    int     X, Y;
    int     w, h;
    xcolor* pMipImage;
    xcolor* pWrite;
    Shift = (Mip<<1) - 1;
    w = W >> Mip;
    h = H >> Mip;
    pMipImage = (xcolor*)x_malloc_fn( w * h * sizeof(xcolor) , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 836);
    ASSERT( pMipImage );
    pWrite = pMipImage;
    for( y = 0; y < h; y++ )
    for( x = 0; x < w; x++ )
    {
        int R, G, B, A;
        R = G = B = A = 0;
        for( Y = (y << Mip); Y < ((y+1) << Mip); Y++ )
        for( X = (x << Mip); X < ((x+1) << Mip); X++ )
        {
            xcolor C = pSource[ (Y * W) + X ];
            R += C.R;
            G += C.G;
            B += C.B;
            A += C.A;
        }
        pWrite->R = ((R >> Shift) + 1) >> 1;
        pWrite->G = ((G >> Shift) + 1) >> 1;
        pWrite->B = ((B >> Shift) + 1) >> 1;
        pWrite->A = ((A >> Shift) + 1) >> 1;
        if (g_CompileMipTest) pWrite[0] = s_MipTestColor[ (Mip<6) ? Mip : 6 ];
        pWrite++;
    }
    return( pMipImage );
}

RVA(0x00255020, 0x22e)
static
xcolor* xbmp_GenerateColorMipA1( const xcolor* pSource, int W, int H, int Mip )
{
    int     x, y;
    int     X, Y;
    int     w, h;
    xcolor* pMipImage;
    xcolor* pWrite;
    w = W >> Mip;
    h = H >> Mip;
    pMipImage = (xcolor*)x_malloc_fn( w * h * sizeof(xcolor) , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 890);
    ASSERT( pMipImage );
    pWrite = pMipImage;
    for( y = 0; y < h; y++ )
    for( x = 0; x < w; x++ )
    {
        int TR, TG, TB, Transparent;
        int OR, OG, OB, Opaque;
        TR = TG = TB = Transparent = 0;
        OR = OG = OB = Opaque      = 0;
        for( Y = (y << Mip); Y < ((y+1) << Mip); Y++ )
        for( X = (x << Mip); X < ((x+1) << Mip); X++ )
        {
            xcolor C = pSource[ (Y * W) + X ];
            if( C.A >= 128 )
            {
                OR += C.R;
                OG += C.G;
                OB += C.B;
                Opaque += 1;
            }
            else
            {
                TR += C.R;
                TG += C.G;
                TB += C.B;
                Transparent += 1;
            }
        }
        if( Transparent > Opaque )
        {
            pWrite->R = TR / Transparent;
            pWrite->G = TG / Transparent;
            pWrite->B = TB / Transparent;
            pWrite->A = 0;
        }
        else
        {
            pWrite->R = OR / Opaque;
            pWrite->G = OG / Opaque;
            pWrite->B = OB / Opaque;
            pWrite->A = 255;
        }
        if (g_CompileMipTest) {
            pWrite->R = s_MipTestColor[ (Mip<6) ? Mip : 6 ].R;
            pWrite->G = s_MipTestColor[ (Mip<6) ? Mip : 6 ].G;
            pWrite->B = s_MipTestColor[ (Mip<6) ? Mip : 6 ].B;
        }
        pWrite++;
    }
    return( pMipImage );
}

RVA(0x00254980, 0x4a7)
void xbitmap::BuildMips( int Mips, int bForcePunchthrough )
{
    int i;
    int PaletteEntries;
    int MipDataSize[16];
    mip Mip        [16];
    xcolor* pColorMip      = 0;
    xcolor* pColorData     = 0;
    xcolor* pColorPalette  = 0;
    int     NewDataSize;
    unsigned char*   pNewData       = 0;
    unsigned char*   pOldData       = (unsigned char*)GetPixelData( 0 );
    int   PunchThruAlpha;
    const format_info& Format = m_FormatInfo[ m_Format ];
    ASSERT( (Mips == 0) || (((m_Width  - 1) & (m_Width )) == 0) );
    ASSERT( (Mips == 0) || (((m_Height - 1) & (m_Height)) == 0) );
    ASSERT( Mips >= 0 );
    if( (m_Width <= 8) || (m_Height <= 8) )
    {
        ASSERT( m_NMips == 0 );
        return;
    }
    Mip[0].Width   = m_Width;
    Mip[0].Height  = m_Height;
    MipDataSize[0] = (Mip[0].Width * Mip[0].Height * Format.BPP) >> 3;
    for( i = 1; i <= Mips; i++ )
    {
        Mip[i].Width   = Mip[i-1].Width   >> 1;
        Mip[i].Height  = Mip[i-1].Height  >> 1;
        MipDataSize[i] = MipDataSize[i-1] >> 2;
        if( (Mip[i].Width <= 8) || (Mip[i].Height <= 8) )
            Mips = i-1;
    }
    if( Mips == 0 )
    {
        if( m_NMips > 0 )
        {
            NewDataSize = (m_Width * m_Height * Format.BPP) >> 3;
            pNewData    = (unsigned char*)x_malloc_fn( NewDataSize , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1022);
            x_memcpy( pNewData, pOldData, NewDataSize );
            if( m_Flags & FLAG_DATA_OWNED )
                x_free_fn( m_Data.pPixel , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1026);
            m_Data.pPixel = pNewData;
            m_DataSize    = NewDataSize;
            m_Flags      |= FLAG_DATA_OWNED;
            m_NMips       = 0;
        }
        return;
    }
    Mip[0].Offset = ALIGN_32( sizeof( mip ) * (Mips+1) );
    for( i = 1; i <= Mips; i++ )
    {
        Mip[i].Offset = ALIGN_32( Mip[i-1].Offset + MipDataSize[i-1] );
    }
    NewDataSize = Mip[Mips].Offset + MipDataSize[Mips];
    pNewData    = (unsigned char*)x_malloc_fn( NewDataSize , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1046);
    ASSERT( pNewData );
    x_memset(pNewData, 0, NewDataSize);
    int DoPS2SwizzleClut = 0;
    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
    {
        DoPS2SwizzleClut = 1;
        PS2UnswizzleClut();
    }
    if( Format.ClutBased )
    {
        PaletteEntries = m_ClutSize / (Format.BPC >> 3);
        pColorPalette  = xbmp_DecodeToColor( m_pClut, (format)m_Format, PaletteEntries );
        pColorData     = xbmp_DecodeIndexToColor( pOldData,
                                                  pColorPalette,
                                                  Format.BPP,
                                                  m_Width * m_Height );
    }
    else
    {
        pColorData = xbmp_DecodeToColor( pOldData,
                                         (format)m_Format,
                                         m_Width * m_Height );
    }
    {
        int NonOpaquePixels = 0;
        PunchThruAlpha = 1;
        for( i = 0; i < m_Width * m_Height; i++ )
        {
            if( pColorData[i].A != 255 )
                NonOpaquePixels++;
            if( (pColorData[i].A != 0) && (pColorData[i].A != 255) )
            {
                PunchThruAlpha = 0;
                break;
            }
        }
        if( NonOpaquePixels == 0 )
            PunchThruAlpha = 0;
    }
    x_memcpy( pNewData + Mip[0].Offset, pOldData, GetMipDataSize(0) );
    if( Format.ClutBased )
        cmap_Begin( pColorPalette, (1<<Format.BPP), (Format.ABits>0) );
    for( i = 1; i <= Mips; i++ )
    {
        if( PunchThruAlpha || bForcePunchthrough )
        {
            pColorMip = xbmp_GenerateColorMipA1( pColorData, m_Width, m_Height, i );
        }
        else
        {
            pColorMip = xbmp_GenerateColorMip( pColorData, m_Width, m_Height, i );
        }
        if( Format.ClutBased )
        {
            xbmp_EncodeColorToIndex( pNewData + Mip[i].Offset,
                                     Format.BPP,
                                     pColorMip,
                                     (m_Width >> (short)i) * (m_Height >> (short)i) );
        }
        else
        {
            xbmp_EncodeFromColor( pNewData + Mip[i].Offset,
                                  (format)m_Format,
                                  pColorMip,
                                  (m_Width >> (short)i) * (m_Height >> (short)i) );
        }
        x_free_fn( pColorMip , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1147);
    }
    if( Format.ClutBased )
        cmap_End();
    if( m_Flags & FLAG_DATA_OWNED )
        x_free_fn( m_Data.pPixel , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1160);
    m_Data.pPixel = pNewData;
    m_DataSize    = NewDataSize;
    m_Flags      |= FLAG_DATA_OWNED;
    for( i = 0; i <= Mips; i++ )
    {
        m_Data.pMip[i].Width  = Mip[i].Width;
        m_Data.pMip[i].Height = Mip[i].Height;
        m_Data.pMip[i].Offset = Mip[i].Offset;
    }
    x_free_fn( pColorData , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1174);
    if( pColorPalette )
        x_free_fn( pColorPalette , "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap_convert.cpp", 1176);
    if( DoPS2SwizzleClut )
        PS2SwizzleClut();
    m_NMips = Mips;
}


RVA(0x00255250, 0x1dc)
void xbitmap::MakeIntensityAlpha( void )
{
    int   Count;
    unsigned char* pStart;
    int   R, G, B, A;
    if( (m_FormatInfo[ m_Format ].ABits == 0) &&
        (m_FormatInfo[ m_Format ].UBits == 0) )
        return;
    if( m_FormatInfo[ m_Format ].UBits != 0 )
        m_Format--;
    const format_info& Format = m_FormatInfo[ m_Format ];
    if( Format.ClutBased )
    {
        Count  = m_ClutSize / (Format.BPC >> 3);
        pStart = m_pClut;
    }
    else
    {
        Count  = m_PW * m_Height;
        pStart = m_Data.pPixel + (m_NMips ? m_Data.pMip[0].Offset : 0);
    }
    if( Format.BPC == 32 )
    {
        unsigned int* pAlter = (unsigned int*)pStart;
        unsigned int  Mask   = Format.RMask | Format.GMask | Format.BMask;
        while( Count > 0 )
        {
            R = (*pAlter & Format.RMask) >> Format.RShiftR;
            G = (*pAlter & Format.GMask) >> Format.GShiftR;
            B = (*pAlter & Format.BMask) >> Format.BShiftR;
            A = (R + G + B + 1) / 3;
            *pAlter = (*pAlter & Mask) | (A << Format.AShiftR);
            pAlter++;
            Count--;
        }
    }
    if( Format.BPC == 16 )
    {
        unsigned short* pAlter = (unsigned short*)pStart;
        unsigned short  Mask   = (unsigned short)(Format.RMask | Format.GMask | Format.BMask);
        while( Count > 0 )
        {
            R   = *pAlter;
            R  &= Format.RMask;
            R >>= Format.RShiftR;
            R <<= Format.RShiftL;
            R  |= (R >> Format.RBits);
            G   = *pAlter;
            G  &= Format.GMask;
            G >>= Format.GShiftR;
            G <<= Format.GShiftL;
            G  |= (G >> Format.GBits);
            B   = *pAlter;
            B  &= Format.BMask;
            B >>= Format.BShiftR;
            B <<= Format.BShiftL;
            B  |= (B >> Format.BBits);
            A   = (R + G + B + 1) / 3;
            A >>= Format.AShiftL;
            *pAlter = (*pAlter & Mask) | (A << Format.AShiftR);
            pAlter++;
            Count--;
        }
    }
}
