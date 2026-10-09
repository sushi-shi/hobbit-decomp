//==============================================================================
//
//  aux_Bitmap.hpp
//
//==============================================================================

#ifndef AUX_BITMAP_HPP
#define AUX_BITMAP_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_bitmap.hpp>

//==============================================================================
//  FUNCTIONS
//==============================================================================

xbool   auxbmp_Load         (       xbitmap& Bitmap, const char* pFileName );
xbool   auxbmp_Save         ( const xbitmap& Bitmap, const char* pFileName );

void    auxbmp_ConvertToD3D (       xbitmap& Bitmap );
void    auxbmp_ConvertToPS2 (       xbitmap& Bitmap );
void    auxbmp_ConvertToNative(     xbitmap& Bitmap );

xbool   auxbmp_LoadD3D      (       xbitmap& Bitmap, const char* pFileName );
xbool   auxbmp_LoadPS2      (       xbitmap& Bitmap, const char* pFileName );
xbool   auxbmp_LoadNative   (       xbitmap& Bitmap, const char* pFileName );

void    auxbmp_SetupDefault (       xbitmap& Bitmap );

// Compresses bitmap by splitting it up into 2 bitmaps:
// A base bitmap (a quarter of the size of the original) and a 
// luminance bitmap (same size as original, but only 4 bits per pixel)
// NOTE: The format of the base bitmap is the same as the source
// The compression ratio is approx 5.3 for 32bit textures, and 1.7 for 8bit textures
void auxbmp_CompressPS2( const xbitmap& SrcBMP, xbitmap& BaseBMP, xbitmap& LumBMP, xbool Subtractive ) ;

// Creates a dot3 map from the source grey scale height map
void auxbmp_ConvertToDot3(const xbitmap& SrcBMP, xbitmap& DstBMP ) ;

//==============================================================================
#endif // BITMAP_HPP
//==============================================================================