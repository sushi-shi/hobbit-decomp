//=========================================================================
//
// DAMAGE.CPP
//
//=========================================================================

#include "Damage.hpp"
#include <Support/Shape/Texture.hpp>
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>


//=========================================================================
// DATA
//=========================================================================

// Tweak these to darken the damage texture
static f32 s_DamageAlphaMult = 2.0f ;
static f32 s_DamageAlphaAdd  = 0.0f ;



//=========================================================================
// FUNCTIONS
//=========================================================================

damage::damage( void )
{
    s32 i ;

    // Clear texture and cluts
    m_Texture = NULL ;
    m_Clut    = NULL ;

    // Clear clut info
    for (i = 0 ; i < PALETTE_COUNT ; i++)
        m_ClutHandle[i] = -1 ;
}

//=========================================================================

damage::~damage( void )
{
    // Make sure you have called kill!
    ASSERT(m_Texture == NULL) ;
    ASSERT(m_Clut    == NULL) ;
}


//=========================================================================

// Given the color, returns the palette index
// (the damage texture is organized into 4 colors, 1 for each layer)
s32 damage::GetPaletteIndex( xcolor Color )
{
    // Red dominent color?
    if ((Color.R > Color.G) && (Color.R > Color.B))
        return 0 ;
    else
    // Green dominent color?
    if ((Color.G > Color.R) && (Color.G > Color.B))
        return 1 ;
    else
    // Blue dominent color?
    if ((Color.B > Color.R) && (Color.B > Color.G))
        return 2 ;
    else
        return 3 ;   // White
}

//=========================================================================

// Compares palette entry by count
struct pal_entry
{
    u8  Alpha ;
    s32 Count ;
} ;

static s32 pal_entry_compare_fn( const void* pItem1, const void* pItem2 )
{
	pal_entry *PalEntryA = (pal_entry *)pItem1 ;
	pal_entry *PalEntryB = (pal_entry *)pItem2 ;

    // Sort by count
	return (PalEntryA->Count < PalEntryB->Count) ;
}

// Converts the 32bit texture into a palettized 8bit textures.
// The palette is split into 4 sets - one for each damage layer
void damage::PrepareTexture( texture* DamageTexture )
{
#if !defined(TARGET_LINUX)
    s32 i,j,k,x,y ;

    // Get texture bitmap
    xbitmap& BMP = DamageTexture->GetXBitmap() ;

    // Setup new bitmap
    xbitmap NewBMP ;
    byte*   NewClut = new byte[256*4] ; // 4 bytes per color
    byte*   NewData = new byte[BMP.GetWidth() * BMP.GetHeight()] ;
    NewBMP.Setup(xbitmap::FMT_P8_ABGR_8888,          // Format
                 BMP.GetWidth(), BMP.GetHeight(),    // Width,Height
                 TRUE,                               // DataOwned
                 NewData,                            // Data
                 TRUE,                               // Clut owned,
                 NewClut) ;                          // Clut

    // Create 4 palettes
    pal_entry   NewPalette[PALETTE_COUNT][256] ;
    x_memset(NewPalette, 0, sizeof(NewPalette)) ;

    // Make sure entry 0 for all palettes is zero!
    for (i = 0 ; i < PALETTE_COUNT ; i++)
        NewPalette[i][0].Count = S32_MAX ;

    // Grab current palettes
    for (x = 0 ; x < BMP.GetWidth() ; x++)
    {
        for (y = 0 ; y < BMP.GetHeight() ; y++)
        {
            xcolor Color = BMP.GetPixelColor(x,y) ;

            // Which palette?
            s32 Palette = GetPaletteIndex(Color) ;

            // Add to palette (ignore 0 since it's already added)
            if (Color.A)
            {
                // Make from 0->1
                f32 Alpha = Color.A / 255.0f ;

                // Scale, add, and keep 0->1
                Alpha *= s_DamageAlphaMult ;
                Alpha += s_DamageAlphaAdd ;
                Alpha = MIN(1.0f, Alpha) ;

                // Convert 0->255
                Color.A = (u8)(Alpha * 255.0f) ;

                // Add to palette
                NewPalette[Palette][Color.A].Alpha = Color.A ;
                NewPalette[Palette][Color.A].Count++ ;
            }
        }
    }

    // Parepare the palettes
    u8 LookupTable[PALETTE_COUNT][256] ;
    for (i = 0 ; i < PALETTE_COUNT ; i++)
    {
	    // Sort palette entries by most used
        x_qsort(NewPalette[i],		    // Address of first item in array.
			    256,        			// Number of items in array.
                sizeof(pal_entry),		// Size of one item.
			    pal_entry_compare_fn) ;	// Compare function.

        // Grab the top most used colors from the palette
        for (j = 0 ; j < (256/PALETTE_COUNT) ; j++)
        {
            xcolor Color = XCOLOR_BLACK ;
            Color.A = (u8)NewPalette[i][j].Alpha ;
            NewBMP.SetClutColor(Color, ((i*256)/PALETTE_COUNT) + j) ;
        }

        // Create the lookup table
        for (j = 0 ; j < 256 ; j++)
        {
            // Make from 0->1
            f32 Alpha = (f32)j / 255.0f ;

            // Scale, add, and keep 0->1
            Alpha *= s_DamageAlphaMult ;
            Alpha += s_DamageAlphaAdd ;
            Alpha = MIN(1.0f, Alpha) ;

            // Convert 0->255
            u8 iAlpha = (u8)(Alpha * 255.0f) ;

            // Search for closest matching alpha...
            s32 BestDist  = S32_MAX ;
            s32 BestIndex = 0 ;
            for (k = 0 ; k < (256/PALETTE_COUNT) ; k++)
            {
                s32 Dist = ABS(NewPalette[i][k].Alpha - iAlpha) ;
                if (Dist < BestDist)
                {
                    BestDist  = Dist ;
                    BestIndex =  k ;
                }
            }

            // Plop into lookup table
            LookupTable[i][j] = BestIndex + ((i * 256)/PALETTE_COUNT) ;
        }
    }

    // Setup pixel data
    for (x = 0 ; x < BMP.GetWidth() ; x++)
    {
        for (y = 0 ; y < BMP.GetHeight() ; y++)
        {
            xcolor Color = BMP.GetPixelColor(x,y) ;

            // Which palette?
            s32 Palette = GetPaletteIndex(Color) ;

            // Set pixel
            NewBMP.SetPixelIndex(LookupTable[Palette][Color.A], x,y) ;
        }
    }

    // Okay - kick out the old texture
    DamageTexture->Kill() ;

    // Replace with new texture
    BMP = NewBMP ;

    // Convert to machine target
    auxbmp_ConvertToNative(BMP) ;

    // Finally, register with vram again
    vram_Register(BMP) ;

    // Save out xbmp so this code doesn't happen next time...
    DamageTexture->SaveXBMP() ;
#endif
}

//=========================================================================

// Creates 4 palettes from the damage texture. Each palette contains
// one more layer of the original palette for turning on more damage.
void damage::Init( texture* DamageTexture )
{
    ASSERT(DamageTexture) ;
    ASSERT(m_Texture == NULL) ;
    ASSERT(m_Clut == NULL) ;

    #ifdef TARGET_PS2

        // Get texture bitmap
        xbitmap& BMP = DamageTexture->GetXBitmap() ;
    
        // Get the texture ready for grabbing palettes?
        if (!BMP.IsClutBased())
            PrepareTexture(DamageTexture) ;
    
        // Point to texture
        m_Texture = DamageTexture ;

        // Allocate cluts
        m_Clut = new byte[256*4*PALETTE_COUNT] ;

        s32 i,j ;

        // Lookup bitmap and make sure it's palettized with 4 bytes per color!
        ASSERT(BMP.IsClutBased()) ;
        ASSERT((BMP.GetClutSize() / (BMP.GetBPC()>>3)) == 256) ;

        // Setup cluts
        for (i = 0 ; i < PALETTE_COUNT ; i++)
        {
            // Copy the original bitmap
            xbitmap BMPCopy = BMP ;

            // Get the clut
            byte* Clut = &m_Clut[i*256*4] ;

            // For now, just copy the palettes alpha!
            for (j = 0 ; j < 256 ; j++)
            {
                // Get original clut color
                xcolor Color = BMP.GetClutColor(j) ;

                // Copy this color?
                s32 MaxColor = (i * 256) / PALETTE_COUNT ;
                if (j < MaxColor)
                    Color.A = 0 ;   // Leave this pixel out by setting the alpha to 0

                // Set new color
                BMPCopy.SetClutColor(Color, j) ;
            }

            // Grab the bitmap clut
            x_memcpy(Clut, BMPCopy.GetClutData(), 256*4) ;

            // Register with vram
            m_ClutHandle[i] = vram_RegisterClut(Clut, 256) ;
        }

    #endif
}

//=========================================================================

void damage::Kill( void )
{
#ifdef TARGET_PS2
    s32 i ;

    // Remove cluts from vram
    for (i = 0 ; i < PALETTE_COUNT ; i++)
    {
        if (m_ClutHandle[i] != -1)
        {
            vram_UnRegisterClut( m_ClutHandle[i] ) ;
            m_ClutHandle[i] = -1 ;
        }
    }

    // Free cluts
    if (m_Clut)
        delete [] m_Clut ;
#endif

    // Clear texture reference
    m_Texture = NULL ;
    m_Clut    = NULL ;
}

//=========================================================================

void damage::GetTexture( f32 Health, texture*& Texture, s32& ClutHandle )
{
#ifdef TARGET_PS2

    // Is texture loaded?
    if (m_Texture == NULL)
    {
        // Don't use damage texture
        Texture    = NULL ;
        ClutHandle = -1 ;
        return ;
    }

    // Range check health
    if (Health < 0)
        Health = 0 ;
    else
    if (Health > 1)
        Health = 1 ;

    // Only show damage if less than 75% of health
    if (Health < 0.75f)
    {
        Texture    = m_Texture ;
        ClutHandle = m_ClutHandle[(s32)((Health / 0.75f) * 3.9f)] ;
    }
    else
    {
        // Don't use damage texture
        Texture    = NULL ;
        ClutHandle = -1 ;
    }

#else

    // Don't use damage texture
    Texture    = NULL ;
    ClutHandle = -1 ;

#endif
}

//=========================================================================




