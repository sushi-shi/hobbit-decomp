//==============================================================================
//
//  aux_Bitmap.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_string.hpp>
#include <xCore/x_files/x_math.hpp>

//#define DUMP_LOADS

#ifdef DUMP_LOADS
static s32 DumpLoadCount=0;
#endif

//==============================================================================
//  TYPES
//==============================================================================

typedef xbool load_fn(       xbitmap& Bitmap, const char* pFileName );
typedef xbool save_fn( const xbitmap& Bitmap, const char* pFileName );

struct load_entry
{
    char      Extension[8];
    load_fn*  LoadFn;
};

struct save_entry
{
    char      Extension[8];
    save_fn*  SaveFn;
};

//==============================================================================
//  FUNCTION DECLARATIONS - Instead of headers with one function declared.
//==============================================================================

       load_fn     tga_Load;
       load_fn     bmp_Load;
       load_fn     png_Load;
       load_fn     psd_Load;
static load_fn     xbmp_Load;

static save_fn     xbmp_Save;

//==============================================================================
//  LOCAL STORAGE
//==============================================================================

static load_entry LoadTable[] = 
{
    { ".TGA",       tga_Load    },
    { ".BMP",       bmp_Load    },
    { ".PNG",       png_Load    },
    { ".PSD",       psd_Load    },
    { ".XBMP",      xbmp_Load   },
};

static save_entry SaveTable[] = 
{
    { ".XBMP",      xbmp_Save   },
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

xbool auxbmp_Load( xbitmap& Bitmap, const char* pFileName )
{           
    char    Ext[ X_MAX_EXT ];
    xbool   Result  = FALSE;
    s32     Loaders = sizeof(LoadTable) / sizeof(load_entry);
    s32     i;

    ASSERT( pFileName );

    // Break up the filename.
    x_splitpath( pFileName, NULL, NULL, NULL, Ext );

    // Upper case the extension.
    x_strtoupper( Ext );

    // Search for a loader.
    for( i = 0; i < Loaders; i++ )
    {
        if( x_strcmp( Ext, LoadTable[i].Extension ) == 0 )
        {
            Result = LoadTable[i].LoadFn( Bitmap, pFileName );
            break;
        }
    }

    // If we did not successfully load, then setup the "default" bitmap.  Leave
    // the Result with a FALSE value, though, to inform the caller of the 
    // situation.

    if( Result == FALSE )
    {
        auxbmp_SetupDefault( Bitmap );
    }

    //
    // We are using a system where we can place "formatting commands" within a 
    // bitmap's file name between square braces.
    //

    char* pCmd = x_strstr( pFileName, "[" );

    if( pCmd )
    {
        // Parse the command codes.
        while( (*pCmd != ']' ) && ( *pCmd != 0 ) )
        {
            char Cmd = x_toupper( *pCmd );

            switch( Cmd )
            {
                // Convert to intensity alpha
                case 'A':
                    Bitmap.MakeIntensityAlpha();
                    break;

                // Convert to a palette
                case 'P':

                    // Start with converting to 32bits so that the quantizer kicks in
                    // when converting between palettized version!
                    if (Bitmap.HasAlphaBits())
                        Bitmap.ConvertFormat(xbitmap::FMT_32_ARGB_8888) ;
                    else
                        Bitmap.ConvertFormat(xbitmap::FMT_32_URGB_8888) ;

                    // Get bits per pixel (8 or 4)
                    pCmd++ ; // Skip P
                    Cmd = *pCmd ;
                    switch(Cmd)
                    {
                        case '4':
                            if (Bitmap.HasAlphaBits())
                                Bitmap.ConvertFormat(xbitmap::FMT_P4_ABGR_8888) ;
                            else
                                Bitmap.ConvertFormat(xbitmap::FMT_P4_UBGR_8888) ;
                            break ;

                        case '8':
                            if (Bitmap.HasAlphaBits())
                                Bitmap.ConvertFormat(xbitmap::FMT_P8_ABGR_8888) ;
                            else
                                Bitmap.ConvertFormat(xbitmap::FMT_P8_UBGR_8888) ;
                            break ;
                    }
                    break ;

                default:
                    break;
            }
            pCmd++;
        } 
    }

#ifdef DUMP_LOADS
    Bitmap.SaveTGA(xfs("auxdump%04d.tga",DumpLoadCount++));
#endif

    return( Result );
}

//==============================================================================

xbool auxbmp_Save( const xbitmap& Bitmap, const char* pFileName )
{           
    char    Ext[ X_MAX_EXT ];
    xbool   Result = FALSE;
    s32     Savers = sizeof(SaveTable) / sizeof(save_entry);
    s32     i;

    ASSERT( pFileName );

    // Break up the filename.
    x_splitpath( pFileName, NULL, NULL, NULL, Ext );

    // Upper case the extension.
    x_strtoupper( Ext );

    // Search for a saver.
    for( i = 0; i < Savers; i++ )
    {
        if( x_strcmp( Ext, SaveTable[i].Extension ) == 0 )
        {
            Result = SaveTable[i].SaveFn( Bitmap, pFileName );
            break;
        }
    }

    // And we are outta here.
    return( Result );
}

//==============================================================================

static
xbool xbmp_Load( xbitmap& Bitmap, const char* pFileName )
{
    // This is easy!  "XBMP" is "xbitmap", and they load themselves!
    return( Bitmap.Load( pFileName ) );
}

//==============================================================================

static
xbool xbmp_Save( const xbitmap& Bitmap, const char* pFileName )
{
    // This is easy!  "XBMP" is "xbitmap", and they save themselves!
    return( Bitmap.Save( pFileName ) );
}

//==============================================================================

void auxbmp_ConvertToD3D( xbitmap& Bitmap )
{
    xbitmap::format OldFormat;
    xbitmap::format NewFormat = xbitmap::FMT_NULL;

    // See if we got lucky and have a compatable format.
    OldFormat = Bitmap.GetFormat();
    if( (OldFormat == xbitmap::FMT_32_ARGB_8888 ) ||
        (OldFormat == xbitmap::FMT_32_URGB_8888 ) ||
        (OldFormat == xbitmap::FMT_16_ARGB_1555 ) ||
        (OldFormat == xbitmap::FMT_16_URGB_1555 ) ||
        (OldFormat == xbitmap::FMT_16_RGB_565   ) )
        return;        
      
    //
    // Oh well, we need to convert.
    //
    // Since we don't support any palette based formats in DirectX, we only 
    // worry about "bits per color" rather than "bits per pixel".  Thus, a
    // palette based bitmap with 32 bits per color will be converted to the
    // same destination format as a bitmap with 32 bits per pixel.
    // 

    switch( Bitmap.GetFormatInfo().BPC )
    {
    case 32:
        // If there is alpha, use FMT_32_ARGB_8888, otherwise FMT_32_URGB_8888.
        if( Bitmap.GetFormatInfo().ABits > 0 )
            NewFormat = xbitmap::FMT_32_ARGB_8888;
        else
            NewFormat = xbitmap::FMT_32_URGB_8888;
        break;

    case 24:
        NewFormat = xbitmap::FMT_32_URGB_8888;
        break;

    case 16:
        // If there is 4 bits of alpha, then go to FMT_32_ARGB_8888.  Otherwise,
        // use closest match of FMT_16_ARGB_1555, FMT_16_URGB_1555, or
        // FMT_16_RGB_565 as appropriate.
        if( Bitmap.GetFormatInfo().ABits == 4 )
            NewFormat = xbitmap::FMT_32_ARGB_8888;
        else
        if( Bitmap.GetFormatInfo().GBits == 6 )
            NewFormat = xbitmap::FMT_16_RGB_565;
        else
        if( Bitmap.GetFormatInfo().ABits == 1 )
            NewFormat = xbitmap::FMT_16_ARGB_1555;
        else
            NewFormat = xbitmap::FMT_16_URGB_1555;
    }

    ASSERT( NewFormat != xbitmap::FMT_NULL );

    Bitmap.ConvertFormat( NewFormat );
}

//==============================================================================

xbool auxbmp_LoadD3D( xbitmap& Bitmap, const char* pFileName )
{
    xbool Result;
    Result = auxbmp_Load( Bitmap, pFileName );
    auxbmp_ConvertToD3D( Bitmap );
    return Result;
}

//==============================================================================

void auxbmp_ConvertToPS2( xbitmap& Bitmap )
{
    xbitmap::format OldFormat;
    xbitmap::format NewFormat = xbitmap::FMT_NULL;

    // See if we got lucky and have a compatable format.
    OldFormat = Bitmap.GetFormat();
    if( (OldFormat != xbitmap::FMT_P4_ABGR_8888 ) &&
        (OldFormat != xbitmap::FMT_P8_ABGR_8888 ) &&
        (OldFormat != xbitmap::FMT_P4_UBGR_8888 ) &&
        (OldFormat != xbitmap::FMT_P8_UBGR_8888 ) &&
        (OldFormat != xbitmap::FMT_16_ABGR_1555 ) &&
        (OldFormat != xbitmap::FMT_16_UBGR_1555 ) &&
        (OldFormat != xbitmap::FMT_32_ABGR_8888 ) &&
        (OldFormat != xbitmap::FMT_32_UBGR_8888 ) )
    {
        //
        // Oh well, we need to convert.
        //

        xbool HasAlpha = Bitmap.HasAlphaBits();
        switch( Bitmap.GetFormatInfo().BPP )
        {
            case 32:
            case 24:
                NewFormat = (HasAlpha)?(xbitmap::FMT_32_ABGR_8888):(xbitmap::FMT_32_UBGR_8888);
                break;
            case 16:
                NewFormat = (HasAlpha)?(xbitmap::FMT_16_ABGR_1555):(xbitmap::FMT_16_UBGR_1555);
                break;
            case 8:
                NewFormat = (HasAlpha)?(xbitmap::FMT_P8_ABGR_8888):(xbitmap::FMT_P8_UBGR_8888);
                break;
            case 4:
                NewFormat = (HasAlpha)?(xbitmap::FMT_P4_ABGR_8888):(xbitmap::FMT_P4_UBGR_8888);
                break;
        }

        ASSERT( NewFormat != xbitmap::FMT_NULL );

        Bitmap.ConvertFormat( NewFormat );
    }

    // Shift alphas down
    // (even when not present since when drawing with alpha blending eg. fading player, it will get used!!)

	// Clut based?
    if (Bitmap.GetFormatInfo().ClutBased)
    {
        s32    NColors = 0;
        byte*  pC      = NULL;

        switch( Bitmap.GetFormatInfo().BPP )
        {
            case 8:    
                NColors = 256;
                pC      = (byte*)Bitmap.GetClutData();
                break;
            case 4:    
                NColors = 16;
                pC      = (byte*)Bitmap.GetClutData();
                break;

            default:
                ASSERTS(0, "Unsupported clut format!") ;
                break ;
        }

        while( NColors-- )
        {
            pC[3] >>= 1;
            pC     += 4;
        }
    }
    else
    {
        // Non-clut based

        // Loop through all mip levels
        for (s32 Mip = 0 ; Mip <= Bitmap.GetNMips() ; Mip++)
        {
            s32    NColors = 0;
            byte*  pC      = NULL;

            switch( Bitmap.GetFormatInfo().BPP )
            {
                case 32:    
                    NColors = Bitmap.GetWidth(Mip) * Bitmap.GetHeight(Mip);
                    pC      = (byte*)Bitmap.GetPixelData(Mip) ;
                    break;
            }

            while( NColors-- )
            {
                pC[3] >>= 1;
                pC     += 4;
            }
        }
    }

    // Be sure the good stuff is swizzled.
    Bitmap.PS2SwizzleClut();

    // Make sure 4 bits per pixel data nibbles are flipped
    Bitmap.Flip4BitNibbles();
}

//==============================================================================

xbool auxbmp_LoadPS2( xbitmap& Bitmap, const char* pFileName )
{
    xbool Result;
    Result = auxbmp_Load( Bitmap, pFileName );

#ifdef DUMP_LOADS
    Bitmap.SaveTGA( xfs("LoadPS2A%04d.tga",DumpCount) );
    Bitmap.SavePaletteTGA(xfs("LoadPS2PALA%04d.tga",DumpLoadCount) );
#endif

    auxbmp_ConvertToPS2( Bitmap );

#ifdef DUMP_LOADS
    Bitmap.SaveTGA( xfs("LoadPS2B%04d.tga",DumpCount) );
    Bitmap.SavePaletteTGA(xfs("LoadPS2PALB%04d.tga",DumpLoadCount) );
    DumpLoadCount++;
#endif

    return( Result );
}

//==============================================================================

void auxbmp_ConvertToNative( xbitmap& Bitmap )
{
    #ifdef TARGET_PC
    auxbmp_ConvertToD3D( Bitmap );
    return;
    #endif

    #ifdef TARGET_PS2
    auxbmp_ConvertToPS2( Bitmap );
    return;
    #endif  

    ASSERT( FALSE );
}

//==============================================================================

xbool auxbmp_LoadNative( xbitmap& Bitmap, const char* pFileName )
{
    xbool Result;
    Result = auxbmp_Load( Bitmap, pFileName );
    auxbmp_ConvertToNative( Bitmap );
#ifdef DUMP_LOADS
    Bitmap.SaveTGA(xfs("auxdumpN%04d.tga",DumpLoadCount++));
#endif
    return Result;
}

//==============================================================================

// Quantizer utils...
void quant_Begin            ( void );
void quant_SetPixels        ( const xcolor* pColor, s32 NColors );
void quant_End              ( xcolor* pPalette, s32 NColors, xbool UseAlpha );

// Color map utils...
void cmap_Begin     ( const xcolor* pColor, s32 NColors, xbool UseAlpha );
s32  cmap_GetIndex  ( xcolor Color );
void cmap_End       ( void );

// Converts to new format with all the generated mips intact...
void auxbmp_ConvertFormat( xbitmap& BMP, xbitmap::format Format )
{
    s32 i, x,y,mip, NColors ;

    // Lookup useful info
    ASSERT( (Format > xbitmap::FMT_NULL       ) && 
            (Format < xbitmap::FMT_END_OF_LIST) );
    const xbitmap::format_info& NewFormat = xbitmap::GetFormatInfo( Format ) ;

    // Create a copy
    xbitmap SrcBMP = BMP ;

    // Let xbitmap class do all the work to setup a bitmap in the new format with space for mips...
    BMP.BuildMips(0) ;
    BMP.ConvertFormat(Format) ;
    BMP.BuildMips(SrcBMP.GetNMips()) ;

    // Converting to a clut format?
    if (NewFormat.ClutBased)
    {
        // O-oh - time for the quantizer to do it's stuff....

        // Create space for new palette
        s32     NPaletteColors = 1 << NewFormat.BPP ;
        xcolor* Palette = new xcolor[NPaletteColors] ;
        ASSERT(Palette) ;

        // For the quantizer we need to build a single list of colors which includes all mips!
        NColors = 0 ;
        for (mip = 0 ; mip <= SrcBMP.GetNMips() ; mip++)
            NColors += SrcBMP.GetWidth(mip) * SrcBMP.GetHeight(mip) ;

        // Allocate space for all the colors, and set them up
        xcolor* Colors = new xcolor[NColors] ;
        ASSERT(Colors) ;
        i = 0 ;
        for (mip = 0 ; mip <= SrcBMP.GetNMips() ; mip++)
        for (y = 0 ; y < SrcBMP.GetHeight(mip) ; y++)
        for (x = 0 ; x < SrcBMP.GetWidth(mip) ;  x++)
            Colors[i++] = SrcBMP.GetPixelColor(x,y, mip) ;
        ASSERT(i == NColors) ;

        // Let the quantizer do it's thing
        quant_Begin() ;
        quant_SetPixels(Colors, NColors) ;
        quant_End(Palette, NPaletteColors, (NewFormat.ABits > 0)) ;

        // Copy the new generated palette
        for (i = 0 ; i < NPaletteColors ; i++)
            BMP.SetClutColor(Palette[i], i) ;

        // Now setup the pixels (yuk this is slow!) letting xbitmap do all the format conversion work...
        cmap_Begin( Palette, NPaletteColors, (NewFormat.ABits > 0)) ;
        for (mip = 0 ; mip <= BMP.GetNMips() ; mip++)
        for (y = 0 ; y < BMP.GetHeight(mip) ; y++)
        for (x = 0 ; x < BMP.GetWidth(mip) ;  x++)
            BMP.SetPixelIndex(cmap_GetIndex(SrcBMP.GetPixelColor(x,y, mip)), x,y, mip) ;
        cmap_End() ;

        // Free allocated memory
        delete [] Colors ;
        delete [] Palette ;
    }
    else
    {
        // Easy - convert to a non-clut format

        // Now setup all the mips letting xbitmap do all the format conversion work...
        for (mip = 0 ; mip <= BMP.GetNMips() ; mip++)
        for (y = 0 ; y < BMP.GetHeight(mip) ; y++)
        for (x = 0 ; x < BMP.GetWidth(mip) ;  x++)
            BMP.SetPixelColor(SrcBMP.GetPixelColor(x,y, mip), x,y, mip) ;
    }
}


// Compresses bitmap by splitting it up into 2 bitmaps:
// A base bitmap (a quarter of the size of the original) and a 
// luminance bitmap (same size as original, but only 4 bits per pixel)
void auxbmp_CompressPS2( const xbitmap& SrcBMP, xbitmap& BaseBMP, xbitmap& LumBMP, xbool Subtractive )
{
    s32 i,x,y,bx,by, mip, MipWidth, MipHeight, BaseMip ;
    s32 BlockSize, BBP ;

    // Setup compression type
    // BlockSize=4, BBP=8,  Ratio=0.56635:1
    // BlockSize=4, BBP=16, Ratio=0.625:1
    // BlockSize=2, BBP=8,  Ratio=0.75:1
    BlockSize=4 ; BBP=8 ;   // best compression

    // Lookup source size
    s32 SrcWidth  = SrcBMP.GetWidth() ;
    s32 SrcHeight = SrcBMP.GetHeight() ;

    // Create a 32bit base bitmap
    s32     BaseWidth  = SrcWidth  / BlockSize ;
    s32     BaseHeight = SrcHeight / BlockSize ;
    byte*   BaseData = (byte *)x_malloc(BaseWidth * BaseHeight * 4) ; // 32bit
    ASSERT(BaseData) ;
    BaseBMP.Setup(SrcBMP.HasAlphaBits() ? xbitmap::FMT_32_ARGB_8888 : xbitmap::FMT_32_URGB_8888,  // Format
                 BaseWidth,                   // Width
                 BaseHeight,                  // Height
                 TRUE,                        // DataOwned
                 BaseData,                    // PixelData
                 FALSE,                       // Clut owned
                 NULL) ;                      // Clut data

    // Create space for mips?
    if (SrcBMP.GetNMips())
        BaseBMP.BuildMips() ;

    // Loop through and generate all mips
    // NOTE - Mips can't be generated with the normal xbitmap stuff since we need the best
    //        pixel match for the compression algorithm - not the blur xbitmap mip generating algorithm!
    //        (If you use the regular xbitmap build bitmaps, then the mips will incorrectly get darker
    //         for subtractive compression)
    for (mip = 0 ; mip <= BaseBMP.GetNMips() ; mip++)
    {
        // Lookup base size
        MipWidth  = BaseBMP.GetWidth(mip) ;
        MipHeight = BaseBMP.GetHeight(mip) ;

        // Get block size to average
        // (use corresponding mip of source texture since that's what we want to look like!)
        BlockSize = SrcBMP.GetWidth(mip) / MipWidth ;

        // Create 32bit base colour map
        for( by=0; by < MipHeight ; by++ )
        for( bx=0; bx < MipWidth  ; bx++ )
        {
            // Calculate average color for block
            s32    Max[4]={0};
            s32    Min[4]={255,255,255,255};
            xcolor BaseCol;

            if (Subtractive)
            {
                // For subtractive, take the brightest color
                s32 L=-1 ;
                for( y=0; y<BlockSize; y++ )
                for( x=0; x<BlockSize; x++ )
                {
                    // Keep brightest color (always use top level mip for best color!)
                    xcolor PC = SrcBMP.GetPixelColor( (bx*BlockSize)+x, (by*BlockSize)+y, mip );
                    s32 B = SQR(PC.R) + SQR(PC.G) + SQR(PC.B) ;
                    if (B > L)
                    {
                        L = B ;
                        BaseCol = PC ;
                    }
                }
            }
            else
            {
                // For multiplicative, take the middle of the min/max of each component
                for( y=0; y<BlockSize; y++ )
                for( x=0; x<BlockSize; x++ )
                {
                    // Always use top level mip for best color!
                    xcolor PC = SrcBMP.GetPixelColor( (bx*BlockSize)+x, (by*BlockSize)+y, mip );
                    Max[0] = MAX(Max[0],PC.R);
                    Max[1] = MAX(Max[1],PC.G);
                    Max[2] = MAX(Max[2],PC.B);
                    Max[3] = MAX(Max[3],PC.A);

                    Min[0] = MIN(Min[0],PC.R);
                    Min[1] = MIN(Min[1],PC.G);
                    Min[2] = MIN(Min[2],PC.B);
                    Min[3] = MIN(Min[3],PC.A);
                }

                BaseCol.R = (Max[0] + Min[0]) >> 1;
                BaseCol.G = (Max[1] + Min[1]) >> 1;
                BaseCol.B = (Max[2] + Min[2]) >> 1;
                BaseCol.A = (Max[3] + Min[3]) >> 1;
            }

            // Set base color
            BaseBMP.SetPixelColor(BaseCol, bx, by, mip) ;
        }
    }

    // Convert back to original format keeping the mips as they have been calculated!
    auxbmp_ConvertFormat( BaseBMP, SrcBMP.GetFormat() ) ;

    // Use even spread of colors/luminances for the 16 palette entries
    s32 CL[16] ;
    for (i = 0 ; i < 16 ; i++)
        CL[i] = (s32)((f32)i * (255.0f/15.0f)) ;

    // Create a palettized lum bitmap same size as source, but only 4bits per pixel
    s32     LumWidth  = SrcWidth  ;
    s32     LumHeight = SrcHeight ;
    byte*   LumData   = (byte *)x_malloc((LumWidth * LumHeight * 4) / 8) ;  // 4bits per poixel
    byte*   LumClut   = (byte *)x_malloc(16*4) ;                            // 16 clut entries
    ASSERT(LumData) ;
    ASSERT(LumClut) ;
    LumBMP.Setup(xbitmap::FMT_P4_ABGR_8888,  // Format (PS2)
                 LumWidth,                   // Width
                 LumHeight,                  // Height
                 TRUE,                       // DataOwned
                 LumData,                    // PixelData
                 TRUE,                       // Clut owned
                 LumClut) ;                  // Clut data

    // Create space for mips?
    if (SrcBMP.GetNMips())
        LumBMP.BuildMips() ;

    // Setup lum bitmap palette
    for (i = 0 ; i < 16 ; i++)
    {
        u8 C = (u8)CL[i] ;
        if (Subtractive)
            LumBMP.SetClutColor(xcolor(C, C, C, 128), i) ;  // Make SrcA represent one for GS alpha blend mode
        else
            LumBMP.SetClutColor(xcolor(C, C, C, C), i) ;    // RGB doesn't matter...
    }

    // Create luminance mip maps
    for( mip=0; mip <= LumBMP.GetNMips() ; mip++)
    {
        // Lookup base mip to use
        BaseMip    = MIN(BaseBMP.GetNMips(), mip) ;

        // Lookup lum map size
        LumWidth   = LumBMP.GetWidth(mip) ;
        LumHeight  = LumBMP.GetHeight(mip) ;

        // Setup all luminance pixels in this mip
        for( y=0; y < LumHeight ; y++ )
        for( x=0; x < LumWidth ; x++ )
        {
            // Get base map color
            // NOTE: Read with bi-linear just like the hardware will render!
            f32 u = (f32)x / (f32)LumWidth ;
            f32 v = (f32)y / (f32)LumHeight ;
            xcolor BaseCol = BaseBMP.GetBilinearColor(u,v,TRUE, BaseMip) ;

            // Lookup the real actual color that we are trying to look like!
            xcolor SrcCol = SrcBMP.GetPixelColor(x, y, mip) ;

            // Find best matching luminance from palette
            s32 BI=0;
            s32 BD=S32_MAX ;
            for( i=0; i<16; i++ )
            {
                s32 C[3];
                s32 L = CL[i] ;
                if (Subtractive)
                {
                    // Get color that GS will work out
                    C[0] = BaseCol.R - L ;
                    C[1] = BaseCol.G - L ;
                    C[2] = BaseCol.B - L ;
                
                    // Since the GS can clamp to zero, we can do that here
                    for (s32 k = 0 ; k < 3 ; k++)
                    {
                        if (C[k] < 0)
                            C[k] = 0 ;
                    }
                }
                else
                {
                    // Get color that GS will work out
                    C[0] = ((s32)BaseCol.R * L) >> 7;
                    C[1] = ((s32)BaseCol.G * L) >> 7;
                    C[2] = ((s32)BaseCol.B * L) >> 7;
                }

                // Valid color?
                if ( (C[0] >= 0)   && (C[1] >= 0)   && (C[2] >= 0) &&
                     (C[0] <= 255) && (C[1] <= 255) && (C[2] <= 255) )
                {
                    // If closest to output color, then keep it!
                    s32 D = SQR(C[0]-(s32)SrcCol.R) +
                            SQR(C[1]-(s32)SrcCol.G) +
                            SQR(C[2]-(s32)SrcCol.B) ;
                    if( D < BD )
                    {
                        //BI = (((i * SrcCol.A)+64) >> 7) ; // TAKE SRC ALPHA INTO ACCOUNT!
                        BI = i ;
                        BD = D ;
                    }
                }
            }

            // Set luminance color
            LumBMP.SetPixelIndex(BI, x,y, mip) ;
        }
    }

    // Put base map into ps2 format
    auxbmp_ConvertToPS2( BaseBMP ) ;

    // Put luminance map into PS2 format
    // (NOTE: Can't call auxbmp_ConvertToPS2 because it halfs all the alphas,
    //        and besides, there's no need since we are already almost have a ps2 format!)
    LumBMP.PS2SwizzleClut();
    LumBMP.Flip4BitNibbles() ;
}

//==============================================================================

// Util function for getting height
s32 auxbmp_GetHeight(const xbitmap& BMP, s32 x, s32 y)
{
    xcolor Col = BMP.GetPixelColor(x,y) ;
    s32    H   = (Col.R + Col.G + Col.B) / 3 ;
    return H ;
}

// Creates a dot3 map from the source grey scale height map
void auxbmp_ConvertToDot3(const xbitmap& SrcBMP, xbitmap& DstBMP)
{
    // Keep original format
    xbitmap::format DstFormat = DstBMP.GetFormat() ;

    // Convert to 32bit if it's a clut format
    if (DstBMP.IsClutBased())
        DstBMP.ConvertFormat(xbitmap::FMT_32_ARGB_8888) ;

    // Get sizes
    s32 BumpWidth  = SrcBMP.GetWidth() ;
    s32 BumpHeight = SrcBMP.GetHeight() ;

    // Create dot 3 map
    for( s32 y=0; y< BumpHeight; y++ )
    {
        for( s32 x=0; x< BumpWidth; x++ )
        {
            s32 H00 = auxbmp_GetHeight(SrcBMP, x,y) ;                       // Get the current pixel
            s32 H10 = auxbmp_GetHeight(SrcBMP, x, MIN(y+1, BumpHeight-1));  // and the pixel one line below.
            s32 H01 = auxbmp_GetHeight(SrcBMP, MIN(x+1, BumpWidth-1), y) ;  // and the pixel to the right

            vector3 vPoint00( (f32)x+0.0f, (f32)y+0.0f, (f32)H00 );
            vector3 vPoint10( (f32)x+1.0f, (f32)y+0.0f, (f32)H10 );
            vector3 vPoint01( (f32)x+0.0f, (f32)y+1.0f, (f32)H01 );
            vector3 v10 = vPoint10 - vPoint00;
            vector3 v01 = vPoint01 - vPoint00;

            vector3 vNormal;
            vNormal = v3_Cross( v10, v01 );
            vNormal.Normalize() ;

            // Scale -1 -> 1 to 0->255
            vNormal += vector3(1,1,1) ;
            vNormal *= 0.5f * 255.0f ;

            // Set final color
            xcolor Col ;
            Col.R = (u8)vNormal.X ;
            Col.G = (u8)vNormal.Y ;
            Col.B = (u8)vNormal.Z ;
            Col.A = (u8)H00 ;
            DstBMP.SetPixelColor(Col, x,y) ;
        }
    }

    // Convert back to original format
    DstBMP.ConvertFormat(DstFormat) ;
}

//==============================================================================
