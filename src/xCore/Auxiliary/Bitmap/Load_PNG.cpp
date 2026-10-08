//==============================================================================
//
//  Load_TGA.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>

#include <xCore/Auxiliary/Bitmap/PNG/png.h>
#include <xCore/Auxiliary/Bitmap/PNG/ZLib/zlib.h>

//==============================================================================
//  FUNCTIONS
//==============================================================================

//==============================================================================
//  The PNG library uses various function pointers to get work done.  Here are
//  the required functions.  We need a file pointer for the read function.
//==============================================================================

static 
void png_FatalErrorFn( xpng_structp, xpng_const_charp )
{ 
    ASSERTS( FALSE, "Error reading PNG file." ); 
}

//------------------------------------------------------------------------------

static 
void png_WarningFn( xpng_structp, xpng_const_charp pMessage )
{ 
    (void)pMessage;
    ASSERTS( FALSE, pMessage ); 
}

//------------------------------------------------------------------------------

static 
xpng_voidp png_MallocFn( xpng_structp, xpng_size_t Size )
{ 
    return( (xpng_voidp)x_malloc( Size ) ); 
}

//------------------------------------------------------------------------------

static 
void png_FreeFn( xpng_structp, xpng_voidp pPtr )
{ 
    x_free( pPtr ); 
}

//------------------------------------------------------------------------------

static X_FILE* s_pFile;

static 
void png_ReadDataFn( xpng_structp, xpng_bytep Data, xpng_size_t Length )
{ 
    x_fread( Data, Length, 1, s_pFile ); 
}

//=========================================================================

xbool png_Read( xbitmap& Bitmap, X_FILE* pFile )
{
    static const s32 HeaderBytesChecked = 8;
    byte             Header[ HeaderBytesChecked ];

    // Set up the global static file pointer.
    s_pFile = pFile;

    // Read the header information.
    x_fread( Header, 1, HeaderBytesChecked, pFile );

    // Sanity check.
    if( xpng_check_sig( Header, HeaderBytesChecked ) == 0 )
        return( FALSE );

    // Try to "create a read struct".
    xpng_structp pPng = xpng_create_read_struct_2( PNG_LIBPNG_VER_STRING,
                                                   NULL,
                                                   png_FatalErrorFn,
                                                   png_WarningFn,
                                                   NULL,
                                                   png_MallocFn,
                                                   png_FreeFn );
    if( !pPng )
        return( FALSE );

    // Try to "create an info struct".
    xpng_infop pInfo = xpng_create_info_struct( pPng );   
    if( !pInfo ) 
    {
        xpng_destroy_read_struct( &pPng,
                                  (xpng_infopp)NULL,
                                  (xpng_infopp)NULL );
        return( FALSE );
    }

    // And again...
    xpng_infop pEndInfo = xpng_create_info_struct( pPng );
    if( !pEndInfo ) 
    {
        xpng_destroy_read_struct( &pPng,
                                  &pInfo,
                                  (xpng_infopp)NULL );
        return( FALSE );
    }

    // Prepare to read.
    xpng_set_read_fn( pPng, NULL, png_ReadDataFn );

    // Read off the info on the image.
    xpng_set_sig_bytes( pPng, HeaderBytesChecked );
    xpng_read_info    ( pPng, pInfo );

    // If we've made it to here, then we can get ready to start loading the 
    // image.

    s32     Width;
    s32     Height;
    s32     BitsPerPixel;
    s32     ColorType;
    byte*   pData          = NULL;
    byte*   pPalette       = NULL;
    s32     PaletteEntries = 0;
   
    xpng_get_IHDR( pPng, pInfo,
                   (xpng_uint_32*)(&Width), 
                   (xpng_uint_32*)(&Height),
                   &BitsPerPixel, &ColorType,   
                   NULL,                        // interlace
                   NULL,                        // compression_type
                   NULL );                      // filter_type

    // First, handle the color transformations.  We need this to read in the
    // data as RGB or RGBA, _always_, with a maximum channel width of 8 bits.
    xbool TransAlpha = FALSE;

    // Strip off any 16 bit info.
    if( BitsPerPixel == 16 ) 
    {
        xpng_set_strip_16( pPng );
    }

    // Expand a transparency channel into a full alpha channel.
    if( (BitsPerPixel >= 24) && (xpng_get_valid( pPng, pInfo, PNG_INFO_tRNS )) )
    {
        xpng_set_expand( pPng );
        TransAlpha = TRUE;
    }

    // Update the info pointer with the result of the transformations above.
    xpng_read_update_info( pPng, pInfo );

    xpng_uint_32 RowBytes      = xpng_get_rowbytes( pPng, pInfo );
    s32          BytesPerPixel = RowBytes / Width;
    
    // Update again.
    BitsPerPixel = BytesPerPixel * 8;

    // Allocate the pixel data space.
    pData = (byte*)x_malloc( RowBytes * Height );
    ASSERT( pData );

    // Set up the row pointers.
    xpng_bytep* pRowPointers = new xpng_bytep[ Height ];
    ASSERT( pRowPointers );

    byte* pBase = pData;
    for( s32 i = 0; i < Height; i++ )
        pRowPointers[i] = pBase + (i * RowBytes);

    // Finally!  Read the image!
    xpng_read_image( pPng, pRowPointers );

    // What type of png file?
    xbool Success = FALSE;
    switch( ColorType )
    {
        case PNG_COLOR_TYPE_PALETTE:
            {
                xpng_colorp  Palette;
                s32          NumPalette;

                // 8 bit data?
                if( BitsPerPixel == 8 )
                {
                    // Grab palette.
                    if( xpng_get_PLTE( pPng, pInfo, &Palette, &NumPalette ) )
                    {
                        // Create new palette.
                        ASSERT(NumPalette <= 256) ;
                        pPalette = (byte*)x_malloc( 256 * 3 );  // NOTE - Pad out to 256 colors!!
                        x_memset(pPalette, 0, 256*3) ;          // Clear all entries
                        PaletteEntries = NumPalette;
                        for( s32 i = 0; i < NumPalette; i++ )
                        {
                            pPalette[(i*3)+0] = Palette[i].red;
                            pPalette[(i*3)+1] = Palette[i].green;
                            pPalette[(i*3)+2] = Palette[i].blue;
                        }
                    }         

                    Bitmap.Setup( xbitmap::FMT_P8_RGB_888,
                                  Width,
                                  Height,
                                  TRUE,
                                  pData,
                                  TRUE,
                                  pPalette );
                    Success = TRUE;
                }
            }
            break;

        case PNG_COLOR_TYPE_GRAY:
            break;
        
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            break;

        case PNG_COLOR_TYPE_RGB:
            switch( BytesPerPixel )
            {
                case 2:
                    Bitmap.Setup( xbitmap::FMT_16_RGB_565,
                                  Width, 
                                  Height,
                                  TRUE,
                                  pData );
                    Success = TRUE;
                    break;

                case 3:
                    Bitmap.Setup( xbitmap::FMT_24_RGB_888,
                                  Width, 
                                  Height,
                                  TRUE,
                                  pData );
                    Success = TRUE;
                    break;
            }
            break;

        case PNG_COLOR_TYPE_RGB_ALPHA:
            switch( BytesPerPixel )
            {
                case 2:
                    Bitmap.Setup( xbitmap::FMT_16_RGBA_5551,
                                  Width, 
                                  Height,
                                  TRUE,
                                  pData );
                    Success = TRUE;
                    break;

                case 4:
                    Bitmap.Setup( xbitmap::FMT_32_ABGR_8888,
                                  Width, 
                                  Height,
                                  TRUE,
                                  pData );
                    Success = TRUE;
                    break;
            }
            break;
    }

    // We're outta here.  Destroy the png structs, and release the lock as 
    // quickly as possible.
    
    // Crashes on 8bit and I don't think we need it (AndyT)
    //xpng_read_end( pPng, NULL );

    xpng_destroy_read_struct( &pPng, &pInfo, &pEndInfo );

    // Cleanup.
    delete [] pRowPointers;
    s_pFile = NULL;

    // Cleanup.
    if( !Success )
    {
        if( pData )     x_free( pData );
        if( pPalette )  x_free( pPalette );
    }

    return( Success );
}

//==============================================================================

xbool png_Load( xbitmap& Bitmap, const char* pFileName )
{
    X_FILE* pFile = x_fopen( pFileName, "rb" );

    if( !pFile ) 
        return( FALSE );

    xbool Result = png_Read( Bitmap, pFile );

    x_fclose( pFile );

    return( Result );
}

//==============================================================================
