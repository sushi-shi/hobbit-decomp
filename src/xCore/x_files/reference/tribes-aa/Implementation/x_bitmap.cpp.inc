//==============================================================================
//  
//  x_bitmap.cpp
//  
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_BITMAP_HPP
#include <xCore/x_files/x_bitmap.hpp>
#endif

#ifndef X_MEMORY_HPP
#include <xCore/x_files/x_memory.hpp>
#endif

#ifndef X_MATH_HPP
#include <xCore/x_files/x_math.hpp>
#endif

//==============================================================================
//  VARIABLES
//==============================================================================

//                 Bits    -----Bits----    ----Shift Right---    --Shift Left-
//  BITS           Used    R  G  B  A  U     R   G   B   A   U    R  G  B  A  U
//                    |    |  |  |  |  |     |   |   |   |   |    |  |  |  |  |
#define B_RGBA_8888  32,   8, 8, 8, 8, 0,   24, 16,  8,  0,  0,   0, 0, 0, 0, 0
#define B_RGBU_8888  24,   8, 8, 8, 0, 8,   24, 16,  8,  0,  0,   0, 0, 0, 0, 0
#define B_ARGB_8888  32,   8, 8, 8, 8, 0,   16,  8,  0, 24,  0,   0, 0, 0, 0, 0
#define B_URGB_8888  24,   8, 8, 8, 0, 8,   16,  8,  0,  0, 24,   0, 0, 0, 0, 0
#define B_RGB_888    24,   8, 8, 8, 0, 0,   16,  8,  0,  0,  0,   0, 0, 0, 0, 0
#define B_RGBA_4444  16,   4, 4, 4, 4, 0,   12,  8,  4,  0,  0,   4, 4, 4, 4, 0
#define B_ARGB_4444  16,   4, 4, 4, 4, 0,    8,  4,  0, 12,  0,   4, 4, 4, 4, 0
#define B_RGBA_5551  16,   5, 5, 5, 1, 0,   11,  6,  1,  0,  0,   3, 3, 3, 7, 0
#define B_RGBU_5551  16,   5, 5, 5, 0, 1,   11,  6,  1,  0,  0,   3, 3, 3, 0, 7
#define B_ARGB_1555  16,   5, 5, 5, 1, 0,   10,  5,  0, 15,  0,   3, 3, 3, 7, 0
#define B_URGB_1555  16,   5, 5, 5, 0, 1,   10,  5,  0,  0, 15,   3, 3, 3, 0, 7
#define B_RGB_565    16,   5, 6, 5, 0, 0,   11,  5,  0,  0,  0,   3, 2, 3, 0, 0
//                    |    |  |  |  |  |     |   |   |   |   |    |  |  |  |  |
#define B_BGRA_8888  32,   8, 8, 8, 8, 0,    8, 16, 24,  0,  0,   0, 0, 0, 0, 0
#define B_BGRU_8888  24,   8, 8, 8, 0, 8,    8, 16, 24,  0,  0,   0, 0, 0, 0, 0
#define B_ABGR_8888  32,   8, 8, 8, 8, 0,    0,  8, 16, 24,  0,   0, 0, 0, 0, 0
#define B_UBGR_8888  24,   8, 8, 8, 0, 8,    0,  8, 16,  0, 24,   0, 0, 0, 0, 0
#define B_BGR_888    24,   8, 8, 8, 0, 0,    0,  8, 16,  0,  0,   0, 0, 0, 0, 0
#define B_BGRA_4444  16,   4, 4, 4, 4, 0,    4,  8, 12,  0,  0,   4, 4, 4, 4, 0
#define B_ABGR_4444  16,   4, 4, 4, 4, 0,    0,  4,  8, 12,  0,   4, 4, 4, 4, 0
#define B_BGRA_5551  16,   5, 5, 5, 1, 0,    1,  6, 11,  0,  0,   3, 3, 3, 7, 0
#define B_BGRU_5551  16,   5, 5, 5, 0, 1,    1,  6, 11,  0,  0,   3, 3, 3, 0, 7
#define B_ABGR_1555  16,   5, 5, 5, 1, 0,    0,  5, 10, 15,  0,   3, 3, 3, 7, 0
#define B_UBGR_1555  16,   5, 5, 5, 0, 1,    0,  5, 10,  0, 15,   3, 3, 3, 0, 7
#define B_BGR_565    16,   5, 6, 5, 0, 0,    0,  5, 11,  0,  0,   3, 2, 3, 0, 0
//                    |    |  |  |  |  |     |   |   |   |   |    |  |  |  |  |
#define B_NULL       -1,  -1,-1,-1,-1,-1,   -1, -1, -1, -1, -1,  -1,-1,-1,-1,-1

//                                                                           
//  MASKS                 RMask       GMask       BMask       AMask       UMask
//                            |           |           |           |           |
#define M_RGBA_8888  0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
#define M_RGBU_8888  0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000, 0x000000FF
#define M_ARGB_8888  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 0x00000000
#define M_URGB_8888  0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 0xFF000000
#define M_RGB_888    0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 0x00000000
#define M_RGBA_4444  0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F, 0x00000000
#define M_ARGB_4444  0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000, 0x00000000
#define M_RGBA_5551  0x0000F800, 0x000007C0, 0x0000003E, 0x00000001, 0x00000000
#define M_RGBU_5551  0x0000F800, 0x000007C0, 0x0000003E, 0x00000000, 0x00000001
#define M_ARGB_1555  0x00007C00, 0x000003E0, 0x0000001F, 0x00008000, 0x00000000
#define M_URGB_1555  0x00007C00, 0x000003E0, 0x0000001F, 0x00000000, 0x00008000
#define M_RGB_565    0x0000F800, 0x000007E0, 0x0000001F, 0x00000000, 0x00000000
//                            |           |           |           |           |
#define M_BGRA_8888  0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF, 0x00000000
#define M_BGRU_8888  0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000, 0x000000FF
#define M_ABGR_8888  0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000
#define M_UBGR_8888  0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, 0xFF000000
#define M_BGR_888    0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, 0x00000000
#define M_BGRA_4444  0x000000F0, 0x00000F00, 0x0000F000, 0x0000000F, 0x00000000
#define M_ABGR_4444  0x0000000F, 0x000000F0, 0x00000F00, 0x0000F000, 0x00000000
#define M_BGRA_5551  0x0000003E, 0x000007C0, 0x0000F800, 0x00000001, 0x00000000
#define M_BGRU_5551  0x0000003E, 0x000007C0, 0x0000F800, 0x00000000, 0x00000001
#define M_ABGR_1555  0x0000001F, 0x000003E0, 0x00007C00, 0x00008000, 0x00000000
#define M_UBGR_1555  0x0000001F, 0x000003E0, 0x00007C00, 0x00000000, 0x00008000
#define M_BGR_565    0x0000001F, 0x000007E0, 0x0000F800, 0x00000000, 0x00000000
//                            |           |           |           |           |
#define M_NULL       0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF

//==============================================================================

const xbitmap::format_info xbitmap::m_FormatInfo[ xbitmap::FMT_END_OF_LIST ] =
{                       
//  Format            String          CLUT?    BPP  BPC  BITS         MASKS

{   FMT_NULL,         "NULL FORMAT",  FALSE,   -1,  -1,  B_NULL,      M_NULL       },
    
{   FMT_32_RGBA_8888, "32_RGBA_8888", FALSE,   32,  32,  B_RGBA_8888, M_RGBA_8888  },
{   FMT_32_RGBU_8888, "32_RGBU_8888", FALSE,   32,  32,  B_RGBU_8888, M_RGBU_8888  },
{   FMT_32_ARGB_8888, "32_ARGB_8888", FALSE,   32,  32,  B_ARGB_8888, M_ARGB_8888  },
{   FMT_32_URGB_8888, "32_URGB_8888", FALSE,   32,  32,  B_URGB_8888, M_URGB_8888  },
{   FMT_24_RGB_888,   "24_RGB_888",   FALSE,   24,  24,  B_RGB_888  , M_RGB_888    },
{   FMT_16_RGBA_4444, "16_RGBA_4444", FALSE,   16,  16,  B_RGBA_4444, M_RGBA_4444  },
{   FMT_16_ARGB_4444, "16_ARGB_4444", FALSE,   16,  16,  B_ARGB_4444, M_ARGB_4444  },
{   FMT_16_RGBA_5551, "16_RGBA_5551", FALSE,   16,  16,  B_RGBA_5551, M_RGBA_5551  },
{   FMT_16_RGBU_5551, "16_RGBU_5551", FALSE,   16,  16,  B_RGBU_5551, M_RGBU_5551  },
{   FMT_16_ARGB_1555, "16_ARGB_1555", FALSE,   16,  16,  B_ARGB_1555, M_ARGB_1555  },
{   FMT_16_URGB_1555, "16_URGB_1555", FALSE,   16,  16,  B_URGB_1555, M_URGB_1555  },
{   FMT_16_RGB_565,   "16_RGB_565",   FALSE,   16,  16,  B_RGB_565  , M_RGB_565    },
                                      
{   FMT_32_BGRA_8888, "32_BGRA_8888", FALSE,   32,  32,  B_BGRA_8888, M_BGRA_8888  },       
{   FMT_32_BGRU_8888, "32_BGRU_8888", FALSE,   32,  32,  B_BGRU_8888, M_BGRU_8888  },       
{   FMT_32_ABGR_8888, "32_ABGR_8888", FALSE,   32,  32,  B_ABGR_8888, M_ABGR_8888  },       
{   FMT_32_UBGR_8888, "32_UBGR_8888", FALSE,   32,  32,  B_UBGR_8888, M_UBGR_8888  },       
{   FMT_24_BGR_888,   "24_BGR_888",   FALSE,   24,  24,  B_BGR_888  , M_BGR_888    },       
{   FMT_16_BGRA_4444, "16_BGRA_4444", FALSE,   16,  16,  B_BGRA_4444, M_BGRA_4444  },  
{   FMT_16_ABGR_4444, "16_ABGR_4444", FALSE,   16,  16,  B_ABGR_4444, M_ABGR_4444  },  
{   FMT_16_BGRA_5551, "16_BGRA_5551", FALSE,   16,  16,  B_BGRA_5551, M_BGRA_5551  },       
{   FMT_16_BGRU_5551, "16_BGRU_5551", FALSE,   16,  16,  B_BGRU_5551, M_BGRU_5551  },       
{   FMT_16_ABGR_1555, "16_ABGR_1555", FALSE,   16,  16,  B_ABGR_1555, M_ABGR_1555  },       
{   FMT_16_UBGR_1555, "16_UBGR_1555", FALSE,   16,  16,  B_UBGR_1555, M_UBGR_1555  },       
{   FMT_16_BGR_565,   "16_BGR_565",   FALSE,   16,  16,  B_BGR_565  , M_BGR_565    },       
                                       
{   FMT_P8_RGBA_8888, "P8_RGBA_8888",  TRUE,    8,  32,  B_RGBA_8888, M_RGBA_8888  },         
{   FMT_P8_RGBU_8888, "P8_RGBU_8888",  TRUE,    8,  32,  B_RGBU_8888, M_RGBU_8888  },         
{   FMT_P8_ARGB_8888, "P8_ARGB_8888",  TRUE,    8,  32,  B_ARGB_8888, M_ARGB_8888  },         
{   FMT_P8_URGB_8888, "P8_URGB_8888",  TRUE,    8,  32,  B_URGB_8888, M_URGB_8888  },         
{   FMT_P8_RGB_888,   "P8_RGB_888",    TRUE,    8,  24,  B_RGB_888  , M_RGB_888    },         
{   FMT_P8_RGBA_4444, "P8_RGBA_4444",  TRUE,    8,  16,  B_RGBA_4444, M_RGBA_4444  },         
{   FMT_P8_ARGB_4444, "P8_ARGB_4444",  TRUE,    8,  16,  B_ARGB_4444, M_ARGB_4444  },         
{   FMT_P8_RGBA_5551, "P8_RGBA_5551",  TRUE,    8,  16,  B_RGBA_5551, M_RGBA_5551  },         
{   FMT_P8_RGBU_5551, "P8_RGBU_5551",  TRUE,    8,  16,  B_RGBU_5551, M_RGBU_5551  },         
{   FMT_P8_ARGB_1555, "P8_ARGB_1555",  TRUE,    8,  16,  B_ARGB_1555, M_ARGB_1555  },         
{   FMT_P8_URGB_1555, "P8_URGB_1555",  TRUE,    8,  16,  B_URGB_1555, M_URGB_1555  },         
{   FMT_P8_RGB_565,   "P8_RGB_565",    TRUE,    8,  16,  B_RGB_565  , M_RGB_565    },         
                                                                                               
{   FMT_P8_BGRA_8888, "P8_BGRA_8888",  TRUE,    8,  32,  B_BGRA_8888, M_BGRA_8888  },         
{   FMT_P8_BGRU_8888, "P8_BGRU_8888",  TRUE,    8,  32,  B_BGRU_8888, M_BGRU_8888  },         
{   FMT_P8_ABGR_8888, "P8_ABGR_8888",  TRUE,    8,  32,  B_ABGR_8888, M_ABGR_8888  },         
{   FMT_P8_UBGR_8888, "P8_UBGR_8888",  TRUE,    8,  32,  B_UBGR_8888, M_UBGR_8888  },         
{   FMT_P8_BGR_888,   "P8_BGR_888",    TRUE,    8,  24,  B_BGR_888  , M_BGR_888    },         
{   FMT_P8_BGRA_4444, "P8_BGRA_4444",  TRUE,    8,  16,  B_BGRA_4444, M_BGRA_4444  },         
{   FMT_P8_ABGR_4444, "P8_ABGR_4444",  TRUE,    8,  16,  B_ABGR_4444, M_ABGR_4444  },         
{   FMT_P8_BGRA_5551, "P8_BGRA_5551",  TRUE,    8,  16,  B_BGRA_5551, M_BGRA_5551  },         
{   FMT_P8_BGRU_5551, "P8_BGRU_5551",  TRUE,    8,  16,  B_BGRU_5551, M_BGRU_5551  },         
{   FMT_P8_ABGR_1555, "P8_ABGR_1555",  TRUE,    8,  16,  B_ABGR_1555, M_ABGR_1555  },         
{   FMT_P8_UBGR_1555, "P8_UBGR_1555",  TRUE,    8,  16,  B_UBGR_1555, M_UBGR_1555  },         
{   FMT_P8_BGR_565,   "P8_BGR_565",    TRUE,    8,  16,  B_BGR_565  , M_BGR_565    },         
                                       
{   FMT_P4_RGBA_8888, "P4_RGBA_8888",  TRUE,    4,  32,  B_RGBA_8888, M_RGBA_8888  },
{   FMT_P4_RGBU_8888, "P4_RGBU_8888",  TRUE,    4,  32,  B_RGBU_8888, M_RGBU_8888  },
{   FMT_P4_ARGB_8888, "P4_ARGB_8888",  TRUE,    4,  32,  B_ARGB_8888, M_ARGB_8888  },
{   FMT_P4_URGB_8888, "P4_URGB_8888",  TRUE,    4,  32,  B_URGB_8888, M_URGB_8888  },
{   FMT_P4_RGB_888,   "P4_RGB_888",    TRUE,    4,  24,  B_RGB_888  , M_RGB_888    },
{   FMT_P4_RGBA_4444, "P4_RGBA_4444",  TRUE,    4,  16,  B_RGBA_4444, M_RGBA_4444  },
{   FMT_P4_ARGB_4444, "P4_ARGB_4444",  TRUE,    4,  16,  B_ARGB_4444, M_ARGB_4444  },
{   FMT_P4_RGBA_5551, "P4_RGBA_5551",  TRUE,    4,  16,  B_RGBA_5551, M_RGBA_5551  },
{   FMT_P4_RGBU_5551, "P4_RGBU_5551",  TRUE,    4,  16,  B_RGBU_5551, M_RGBU_5551  },
{   FMT_P4_ARGB_1555, "P4_ARGB_1555",  TRUE,    4,  16,  B_ARGB_1555, M_ARGB_1555  },
{   FMT_P4_URGB_1555, "P4_URGB_1555",  TRUE,    4,  16,  B_URGB_1555, M_URGB_1555  },
{   FMT_P4_RGB_565,   "P4_RGB_565",    TRUE,    4,  16,  B_RGB_565  , M_RGB_565    },
                                                                                      
{   FMT_P4_BGRA_8888, "P4_BGRA_8888",  TRUE,    4,  32,  B_BGRA_8888, M_BGRA_8888  },
{   FMT_P4_BGRU_8888, "P4_BGRU_8888",  TRUE,    4,  32,  B_BGRU_8888, M_BGRU_8888  },
{   FMT_P4_ABGR_8888, "P4_ABGR_8888",  TRUE,    4,  32,  B_ABGR_8888, M_ABGR_8888  },
{   FMT_P4_UBGR_8888, "P4_UBGR_8888",  TRUE,    4,  32,  B_UBGR_8888, M_UBGR_8888  },
{   FMT_P4_BGR_888,   "P4_BGR_888",    TRUE,    4,  24,  B_BGR_888  , M_BGR_888    },
{   FMT_P4_BGRA_4444, "P4_BGRA_4444",  TRUE,    4,  16,  B_BGRA_4444, M_BGRA_4444  },
{   FMT_P4_ABGR_4444, "P4_ABGR_4444",  TRUE,    4,  16,  B_ABGR_4444, M_ABGR_4444  },
{   FMT_P4_BGRA_5551, "P4_BGRA_5551",  TRUE,    4,  16,  B_BGRA_5551, M_BGRA_5551  },
{   FMT_P4_BGRU_5551, "P4_BGRU_5551",  TRUE,    4,  16,  B_BGRU_5551, M_BGRU_5551  },
{   FMT_P4_ABGR_1555, "P4_ABGR_1555",  TRUE,    4,  16,  B_ABGR_1555, M_ABGR_1555  },
{   FMT_P4_UBGR_1555, "P4_UBGR_1555",  TRUE,    4,  16,  B_UBGR_1555, M_UBGR_1555  },
{   FMT_P4_BGR_565,   "P4_BGR_565",    TRUE,    4,  16,  B_BGR_565  , M_BGR_565    },
                                           
//  FMT_I4,
//  FMT_I8,
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

xbitmap::xbitmap( void )
{
    Init();
}

//==============================================================================

xbitmap::xbitmap( const xbitmap& Bitmap )
{
    Init();
    CopyFrom( Bitmap );
}

//==============================================================================

xbitmap::~xbitmap( void )
{
    Kill();
}

//==============================================================================

void xbitmap::CopyFrom( const xbitmap& Source )
{
    // "this" xbitmap should be ready to receive the Source xbitmap state.
    // But, do a little checking to be safe.
    ASSERT( m_Format == FMT_NULL );
    ASSERT( m_Flags  == 0        );

    // Validate the Source a little.
    ASSERT( Source.m_Flags & FLAG_VALID );

    // Clone the Source's pixel data.
    m_Data.pPixel = (byte*)x_malloc( Source.m_DataSize );
    x_memcpy( m_Data.pPixel, Source.m_Data.pPixel, Source.m_DataSize );

    // Clone the clut if present.
    if( Source.m_ClutSize )
    {
        m_pClut = (byte*)x_malloc( Source.m_ClutSize );
        x_memcpy( m_pClut, Source.m_pClut, Source.m_ClutSize );
    }
    else
    {
        m_pClut = NULL;
    }

    // Copy all of the other fields.
    m_DataSize = Source.m_DataSize;
    m_ClutSize = Source.m_ClutSize;
    m_Width    = Source.m_Width;   
    m_Height   = Source.m_Height;  
    m_PW       = Source.m_PW;      
    m_VRAMID   = 0;
    m_NMips    = Source.m_NMips;   
    m_Format   = Source.m_Format;  
    
    // Set the flags properly.
    m_Flags = (Source.m_Flags | FLAG_DATA_OWNED);
    if( m_pClut )
        m_Flags |= FLAG_CLUT_OWNED;
}

//==============================================================================

void xbitmap::Init( void )
{
    m_Data.pPixel = NULL;
    m_pClut       = NULL;
    m_DataSize    = 0;
    m_ClutSize    = 0;
    m_Width       = 0;
    m_Height      = 0;
    m_PW          = 0;
    m_Flags       = 0;
    m_NMips       = 0;
    m_VRAMID      = 0;
    m_Format      = FMT_NULL;    
}

//==============================================================================

void xbitmap::Kill( void )
{
    ASSERT( m_VRAMID == 0 );
    if( m_Flags & FLAG_DATA_OWNED )  x_free( m_Data.pPixel );
    if( m_Flags & FLAG_CLUT_OWNED )  x_free( m_pClut       );
    Init();
}

//==============================================================================

const xbitmap& xbitmap::operator = ( const xbitmap& Bitmap )
{
    Kill();
    CopyFrom( Bitmap );
    return( *this );
}

//==============================================================================

xcolor xbitmap::ReadColor( byte* pRead ) const
{
    u32 Pixel = 0;
    u32 R, G, B, A;
    s32 Bits;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    // The pointer pRead is pointing to some color data we want to read from 
    // this bitmap.  The data may be in the image or the palette.  It doesn't
    // matter.

    // Read the color into our local 32 bit Pixel.

    if( Format.BPC == 32 )
    {
        Pixel = *((u32*)(pRead));
    }
    else
    if( Format.BPC == 24 )
    {
        Pixel = ((u32)(pRead[0]) << 16) |
                ((u32)(pRead[1]) <<  8) |
                ((u32)(pRead[2]) <<  0);
    }
    else
    if( Format.BPC == 16 )
    {
        Pixel = (u32)(*((u16*)(pRead)));
    }
    else
    {
        ASSERT( FALSE );
    }

    // We have the color isolated in Pixel, but in its "native" format).  Pick 
    // out each component, and make sure we have 8 valid bits of data.

    // R

    R   = Pixel;
    R  &= Format.RMask;
    R >>= Format.RShiftR;
    R <<= Format.RShiftL;
    R  |= (R >> Format.RBits);

    // G

    G   = Pixel;
    G  &= Format.GMask;
    G >>= Format.GShiftR;
    G <<= Format.GShiftL;
    G  |= (G >> Format.GBits);

    // B

    B   = Pixel;
    B  &= Format.BMask;
    B >>= Format.BShiftR;
    B <<= Format.BShiftL;
    B  |= (B >> Format.BBits);

    // A

    if( Format.ABits )
    {
        A   = Pixel;
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

    // That's it.  We've got a full 8 bits for each component.

    return( xcolor( (u8)R, (u8)G, (u8)B, (u8)A ) );
}

//==============================================================================

void xbitmap::WriteColor( byte* pWrite, xcolor Color )
{
    u32 R, G, B, A, U;

    // SB - Validate writing address!
    byte* Start ;
    byte* End ;
    if (m_pClut)
    {
        Start = m_pClut ;
        End   = m_pClut + m_ClutSize ;
    }
    else
    {
        Start = m_Data.pPixel ;
        End   = m_Data.pPixel + m_DataSize ;
    }
    ASSERT(pWrite >= Start) ;
    ASSERT(pWrite <= (End-4)) ;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    U = Format.UMask;

    //
    // NOTE - The shift information in xbitmap::m_FormatInfo is for "reading". 
    //        Since we are "writing", we reverse the shift directions and order.
    //        As it turns out, we do not have to do any masking for the RGB 
    //        data.  The shifts will get rid of any extra bits.
    //

    //
    // Encode 32 or 16 bit color information.
    //
    if( (Format.BPC == 32) || (Format.BPC == 16) )
    {
        u32  Encoded;

        R = (u32)Color.R;
        G = (u32)Color.G;
        B = (u32)Color.B;
        A = (u32)Color.A;

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

        if( Format.BPC == 32 )
        {
            *((u32*)(pWrite)) = (u32)Encoded;
        }
        else
        {
            *((u16*)(pWrite)) = (u16)Encoded;
        }
    }

    //
    // Encode 24 bit color information.
    //
    if( Format.BPC == 24 )
    {
        // There are only two 24 bit configurations: RGB and BGR.  
        // Just write code for each case.

        // We can't just ask if "Format == FMT_24_RGB_888" because it may be
        // be paletted like FMT_P8_RGB_888.  So, we will check the right shift on
        // the red.  If it is 16, then we have a RGB format.  And if it is 0, 
        // then we have a BGR format.

        if( Format.RShiftR == 16 )
        {
            pWrite[0] = Color.R;
            pWrite[1] = Color.G;
            pWrite[2] = Color.B;
        }
        else    
        {
            pWrite[0] = Color.B;
            pWrite[1] = Color.G;
            pWrite[2] = Color.R;
        }
    }
}

//==============================================================================

xcolor xbitmap::GetBilinearColor( f32 ParamU, f32 ParamV, xbool Clamp, s32 Mip ) const
{
    ASSERT( Mip >= 0 );

    // Get width and height.
    s32 W, H;
    if( m_NMips == 0 )
    {
        ASSERT( Mip == 0 );
        W = m_Width;
        H = m_Height;
    }
    else
    {
        ASSERT( Mip <= m_NMips );
        W = m_Data.pMip[Mip].Width;
        H = m_Data.pMip[Mip].Height;
    }

    // Compute the four float UVs.
    f32 U0 = ParamU*W - 0.5f;   
    f32 V0 = ParamV*H - 0.5f;

    // Compute the integer pixel coordinates.
    s32 iU0 = (s32)x_floor(U0);
    s32 iV0 = (s32)x_floor(V0);
    s32 iU1 = iU0+1;
    s32 iV1 = iV0+1;

    // Compute the fractional values before clamping/wrapping.
    f32 UF = U0 - (f32)iU0;
    f32 VF = V0 - (f32)iV0;

    // Clamp or wrap integer pixel coordinates.
    if( Clamp )
    {
        // Peg on edges of texture.
        if( iU0 < 0   ) iU0 = 0;
        if( iU0 > W-1 ) iU0 = W-1;
        if( iU1 < 0   ) iU1 = 0;
        if( iU1 > W-1 ) iU1 = W-1;
        if( iV0 < 0   ) iV0 = 0;
        if( iV0 > H-1 ) iV0 = H-1;
        if( iV1 < 0   ) iV1 = 0;
        if( iV1 > H-1 ) iV1 = H-1;
    }
    else
    {
        // Move values into positive range and then modulate.
        iU0 = (iU0 + (W<<16)) % W;
        iU1 = (iU1 + (W<<16)) % W;
        iV0 = (iV0 + (H<<16)) % H;
        iV1 = (iV1 + (H<<16)) % H;
    }

    // Request pixels from the bitmap.
    // There is probably a faster way...
    xcolor CA = GetPixelColor( iU0, iV0, Mip );
    xcolor CB = GetPixelColor( iU1, iV0, Mip );
    xcolor CC = GetPixelColor( iU0, iV1, Mip );
    xcolor CD = GetPixelColor( iU1, iV1, Mip );

    // Compute the bilinear interpolaters.
    f32 IA = (1.0f-UF)*(1.0f-VF);
    f32 IB = (     UF)*(1.0f-VF);
    f32 IC = (1.0f-UF)*(     VF);
    f32 ID = (     UF)*(     VF);

    // Interpolate colors.
    f32 R = IA*CA.R + IB*CB.R + IC*CC.R + ID*CD.R;
    f32 G = IA*CA.G + IB*CB.G + IC*CC.G + ID*CD.G;
    f32 B = IA*CA.B + IB*CB.B + IC*CC.B + ID*CD.B;
    f32 A = IA*CA.A + IB*CB.A + IC*CC.A + ID*CD.A;

    // Build color and return.
    s32 r = (s32)(R+0.5f);
    s32 g = (s32)(G+0.5f);
    s32 b = (s32)(B+0.5f);
    s32 a = (s32)(A+0.5f);
    ASSERT( (r >= 0) && (r <= 255) );
    ASSERT( (g >= 0) && (g <= 255) );
    ASSERT( (b >= 0) && (b <= 255) );
    ASSERT( (a >= 0) && (a <= 255) );
    return( xcolor( r, g, b, a ) );
}

//==============================================================================

static s32 GetPS2SwizzledIndex( s32 I )
{
    static u8 swizzle_lut[32] = 
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    return swizzle_lut[(I) & 31] + (I & ~31);
}

//==============================================================================

xcolor xbitmap::GetPixelColor( s32 X, s32 Y, s32 Mip ) const
{
    byte* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Mip >= 0 );

    if( m_NMips == 0 )
    {
        ASSERT( Mip == 0 );
        ASSERT( X   <  m_Width  );
        ASSERT( Y   <  m_Height ); 
    }
    else
    {
        ASSERT( Mip <= m_NMips );
        ASSERT( X   <  m_Data.pMip[Mip].Width  );
        ASSERT( Y   <  m_Data.pMip[Mip].Height );
    }

    // Find the desired pixel in the data.

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += m_NMips ? (((Y * m_Data.pMip[Mip].Width) * Format.BPP) >> 3) : (((Y * m_PW) * Format.BPP) >> 3) ;
    pPixel += ((   X    ) * Format.BPP) >> 3;                   

    // If this is a palette based bitmap, we must read the index and then 
    // recompute the address to the desired color in the CLUT.

    if( Format.ClutBased )
    {
        s32 Index;

        if( Format.BPP == 4 )
        {
            // Nibbles flipped?
            if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
                Index = (X & 0x01) ? (*pPixel >> 4) : (*pPixel & 0x0F);
            else
                Index = (X & 0x01) ? (*pPixel & 0x0F) : (*pPixel >> 4);
        }
        else
        {
            Index = *pPixel;
        }


        if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
            Index = GetPS2SwizzledIndex( Index );

        // We now have the index, find the pixel's color in the palette.

        ASSERT( m_pClut );
        ASSERT( Index < (m_ClutSize / (Format.BPC >> 3)) );
        pPixel  = m_pClut;
        pPixel += (Index * Format.BPC) >> 3;
    }

    // We'll use an internal helper function for the rest.

    return( ReadColor( pPixel ) );
}

//==============================================================================

s32 xbitmap::GetPixelIndex( s32 X, s32 Y, s32 Mip ) const
{
    byte* pPixel;
    s32   Index;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Format.ClutBased );
    ASSERT( Mip >= 0 );
    
    if( m_NMips == 0 )
    {
        ASSERT( Mip == 0 );
        ASSERT( X   <  m_Width  );
        ASSERT( Y   <  m_Height ); 
    }
    else
    {
        ASSERT( Mip <= m_NMips );
        ASSERT( X   <  m_Data.pMip[Mip].Width  );
        ASSERT( Y   <  m_Data.pMip[Mip].Height );
    }

    // Find the desired pixel in the data.

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += m_NMips ? (((Y * m_Data.pMip[Mip].Width) * Format.BPP) >> 3) : (((Y * m_PW) * Format.BPP) >> 3) ;
    pPixel += ((   X    ) * Format.BPP) >> 3;                   

    // We know this is a palette based bitmap.  So the pixel is actually an 
    // index.  Read the index.

    if( Format.BPP == 4 )
    {
        // Nibbles flipped?
        if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
            Index = (X & 0x01) ? (*pPixel >> 4) : (*pPixel & 0x0F);
        else
            Index = (X & 0x01) ? (*pPixel & 0x0F) : (*pPixel >> 4);
    }
    else
    {
        Index = *pPixel;
    }

    // We're done!

    return( Index );
}

//==============================================================================

xcolor xbitmap::GetClutColor( s32 Index ) const
{
    byte* pColor;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Format.ClutBased );
    ASSERT( Index >= 0 );
    ASSERT( Index < (1 << Format.BPP) );

    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        Index = GetPS2SwizzledIndex( Index );

    // We have the index, find the color in the palette.

    ASSERT( m_pClut );
    ASSERT( Index < (m_ClutSize / (Format.BPC >> 3)) );
    pColor  = m_pClut;
    pColor += (Index * Format.BPC) >> 3;

    // We'll use an internal helper function for the rest.

    return( ReadColor( pColor ) );
}

//==============================================================================

void xbitmap::SetPixelColor( xcolor Color, s32 X, s32 Y, s32 Mip )
{
    byte* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Mip >= 0 );
    ASSERT( !Format.ClutBased );

    if( m_NMips == 0 )
    {
        ASSERT( Mip == 0 );
        ASSERT( X   <  m_Width  );
        ASSERT( Y   <  m_Height ); 
    }
    else
    {
        ASSERT( Mip <= m_NMips );
        ASSERT( X   <  m_Data.pMip[Mip].Width  );
        ASSERT( Y   <  m_Data.pMip[Mip].Height );
    }

    // Find the target pixel in the data.

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += m_NMips ? ((Y * m_Data.pMip[Mip].Width) * Format.BPP) >> 3 : ((Y * m_PW) * Format.BPP) >> 3 ;
    pPixel += ((   X    ) * Format.BPP) >> 3;                   

    // We'll use an internal helper function for the rest.

    WriteColor( pPixel, Color );
}

//==============================================================================

void xbitmap::SetPixelIndex( s32 Index, s32 X, s32 Y, s32 Mip )
{
    byte* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Format.ClutBased );
    ASSERT( Mip >= 0 );
    
    if( m_NMips == 0 )
    {
        ASSERT( Mip == 0 );
        ASSERT( X   <  m_Width  );
        ASSERT( Y   <  m_Height ); 
    }
    else
    {
        ASSERT( Mip <= m_NMips );
        ASSERT( X   <  m_Data.pMip[Mip].Width  );
        ASSERT( Y   <  m_Data.pMip[Mip].Height );
    }

    // Find the desired pixel in the data.

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += m_NMips ? (((Y * m_Data.pMip[Mip].Width) * Format.BPP) >> 3) : (((Y * m_PW) * Format.BPP) >> 3) ;
    pPixel += ((   X    ) * Format.BPP) >> 3;                   

    // We know this is a palette based bitmap, and thie pixel is actually an 
    // index.  Go ahead and write to it.

    if( Format.BPP == 4 )
    {
        u8 Byte;

        // Nibbles flipped?
        if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
        {
            if( X & 0x01 )
                Byte = (*pPixel & 0x0F) | ((u8)Index <<   4);
            else
                Byte = (*pPixel & 0xF0) | ((u8)Index & 0x0F);
        }
        else
        {
            if( X & 0x01 )
                Byte = (*pPixel & 0xF0) | ((u8)Index & 0x0F);
            else
                Byte = (*pPixel & 0x0F) | ((u8)Index <<   4);
        }
        
        *pPixel = Byte;
    }
    else
    {
        *pPixel = (u8)Index;
    }
}

//==============================================================================

void xbitmap::SetClutColor( xcolor Color, s32 Index )
{
    byte* pColor;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    ASSERT( Format.ClutBased );
    ASSERT( Index >= 0 );
    ASSERT( Index < (1 << Format.BPP) );

    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        Index = GetPS2SwizzledIndex( Index );

    // We have the index, find the color in the palette.

    ASSERT( m_pClut );
    ASSERT( Index < (m_ClutSize / (Format.BPC >> 3)) );
    pColor  = m_pClut;
    pColor += (Index * Format.BPC) >> 3;

    // We'll use an internal helper function for the rest.

    WriteColor( pColor, Color );
}

//==============================================================================

void xbitmap::Blit( s32 DestX, s32 DestY,
                    s32 SrcX,  s32 SrcY,
                    s32 Width, s32 Height,
                    const xbitmap& SourceBitmap )
{
    const xbitmap::format_info& SFormat = SourceBitmap.GetFormatInfo();
    const xbitmap::format_info& DFormat =              GetFormatInfo();

    // Make sure the formats are the same!
    ASSERT(&SFormat == &DFormat) ;

    s32 BytesPerBlitRow;
    s32 BytesPerReadRow;
    s32 BytesPerWriteRow;
    byte* pRead;
    byte* pWrite;

    // Validate at least minimal compatibility between the two xbitmaps.
    ASSERT( SourceBitmap.m_Flags & FLAG_VALID );
    ASSERT( SFormat.BPP == DFormat.BPP );

    // Cannot blit into self.
    ASSERT( this != &SourceBitmap );

    //
    // Trim the values as needed.
    //

    if( DestX < 0 )     { Width  += DestX;  SrcX -= DestX;  DestX = 0; }
    if( DestY < 0 )     { Height += DestY;  SrcY -= DestY;  DestY = 0; }

    if( DestX+Width  > m_Width  )       { Width  = m_Width  - DestX; }
    if( DestY+Height > m_Height )       { Height = m_Height - DestY; }

    if( Width  <= 0 )   return;
    if( Height <= 0 )   return;

    //
    // Get locals ready for action.
    //

    BytesPerBlitRow  = (SFormat.BPP * Width) >> 3;
    BytesPerReadRow  = (SFormat.BPP * SourceBitmap.m_PW) >> 3;
    BytesPerWriteRow = (DFormat.BPP *              m_PW) >> 3;

    pRead   = (byte*)SourceBitmap.GetPixelData();
    pRead  +=   SrcY * BytesPerReadRow;
    pRead  += ((SrcX * SFormat.BPP) >> 3);

    pWrite  = (byte*)GetPixelData();
    pWrite +=   DestY * BytesPerWriteRow;
    pWrite += ((DestX * DFormat.BPP) >> 3);

    // And write the rows.
    while( Height > 0 )
    {
        x_memcpy( pWrite, pRead, BytesPerBlitRow );
        pRead  += BytesPerReadRow;
        pWrite += BytesPerWriteRow;
        Height -= 1;
    }
}

//==============================================================================

void xbitmap::Setup( format    Format,              
                     s32       Width,               
                     s32       Height,              
                     xbool     DataOwned,           
                     byte*     pPixelData,          
                     xbool     ClutOwned,  
                     byte*     pClutData,
                     s32       PhysicalWidth )
{
    ASSERT( pPixelData );
    ASSERT( Width  > 0 );
    ASSERT( Height > 0 );
    ASSERT( (Format > FMT_NULL) && (Format < FMT_END_OF_LIST) );
    ASSERT( (pClutData != NULL) == (m_FormatInfo[Format].ClutBased) );
    ASSERT( (u32)pClutData >= (u32)ClutOwned ); // Owned but no clut?

    if( PhysicalWidth == -1 )
        PhysicalWidth = Width;

    Kill();

    m_Data.pPixel = pPixelData;
    m_pClut       = pClutData;
    m_DataSize    = (Width * Height * m_FormatInfo[Format].BPP) >> 3;
    m_ClutSize    = 0;
    m_Width       = Width;
    m_Height      = Height;
    m_PW          = PhysicalWidth;
    m_VRAMID      = 0;
    m_Flags       = FLAG_VALID;
    m_NMips       = 0;
    m_Format      = Format;

    if( DataOwned )
        m_Flags |= FLAG_DATA_OWNED;

    if( pClutData )
    {
        s32 ClutEntries = (1 << m_FormatInfo[Format].BPP);
        m_ClutSize = (ClutEntries * m_FormatInfo[Format].BPC) >> 3;
        if( ClutOwned )
            m_Flags |= FLAG_CLUT_OWNED;
    }
}

//==============================================================================

xbool xbitmap::HasAlphaBits( void ) const
{
    return( m_FormatInfo[m_Format].ABits > 0 );
}

//==============================================================================

xbool xbitmap::IsClutBased( void ) const
{
    return( m_FormatInfo[m_Format].ClutBased );
}

//==============================================================================

void xbitmap::PS2SwizzleClut  ( void )
{
    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        return;

    if( GetBPP() == 8 )
    {
        s32     i, j, idx;
        u32*    C;
        u32     Clut8[256];

        C = (u32*)m_pClut;   

        idx = 0;
        for( i = 0; i < 256; i+=32 ) 
        {
            for( j = i;    j < i+8;    j++ )  Clut8[idx++] = C[j];
            for( j = i+16; j < i+16+8; j++ )  Clut8[idx++] = C[j];
            for( j = i+8;  j < i+8+8;  j++ )  Clut8[idx++] = C[j];
            for( j = i+24; j < i+24+8; j++ )  Clut8[idx++] = C[j];
        }

        for( i=0; i<256; i++ )
            C[i] = Clut8[i];

        m_Flags |= FLAG_PS2_CLUT_SWIZZLED;
    }
}

//==============================================================================

void xbitmap::PS2UnswizzleClut( void )
{
    if( (m_Flags & FLAG_PS2_CLUT_SWIZZLED) == 0 )
        return;

    // Attempt reversal of swizzling.
    if( GetBPP() == 8 )
    {
        s32     i, j, idx;
        u32*    C;
        u32     Clut8[256];

        C = (u32*)m_pClut;   

        idx = 0;
        for( i = 0; i < 256; i+=32 ) 
        {
            for( j = i;    j < i+8;    j++ )  Clut8[idx++] = C[j];
            for( j = i+16; j < i+16+8; j++ )  Clut8[idx++] = C[j];
            for( j = i+8;  j < i+8+8;  j++ )  Clut8[idx++] = C[j];
            for( j = i+24; j < i+24+8; j++ )  Clut8[idx++] = C[j];
        }

        for( i=0; i<256; i++ )
            C[i] = Clut8[i];

        m_Flags &= ~FLAG_PS2_CLUT_SWIZZLED;
    }
}

//==============================================================================

void xbitmap::Flip4BitNibbles( void )
{
    // Must be 4 bits per pixel!
    if( GetBPP() != 4 )
        return;

    // Already flipped?
    if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
        return ;

    // Flip those nibbles
    for( s32 M=0; M<GetNMips()+1; M++ )
    {

        byte* pC = (byte*)GetPixelData(M);
        s32   NBytes = GetMipDataSize(M);

        for( s32 i=0; i<NBytes; i++ )
        {
            s32 N0 = (pC[i]>>4) & 0x0F;
            s32 N1 = (pC[i]>>0) & 0x0F;
            pC[i] = (N1<<4) | (N0<<0);
        }
    }

    // Flag they are flipped
    m_Flags |= FLAG_4BIT_NIBBLES_FLIPPED ;
}

//==============================================================================

void xbitmap::Unflip4BitNibbles( void )
{
    // Must be 4 bits per pixel!
    if( GetBPP() != 4 )
        return;

    // Must be flipped already
    if (!(m_Flags & FLAG_4BIT_NIBBLES_FLIPPED))
        return ;

    // Flip those nibbles
    for( s32 M=0; M<GetNMips()+1; M++ )
    {

        byte* pC = (byte*)GetPixelData(M);
        s32   NBytes = GetMipDataSize(M);

        for( s32 i=0; i<NBytes; i++ )
        {
            s32 N0 = (pC[i]>>4) & 0x0F;
            s32 N1 = (pC[i]>>0) & 0x0F;
            pC[i] = (N1<<4) | (N0<<0);
        }
    }

    // Flag they are not flipped
    m_Flags &= ~FLAG_4BIT_NIBBLES_FLIPPED ;
}

//==============================================================================


