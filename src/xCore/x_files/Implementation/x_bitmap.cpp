// Entropy sibling reconstruction, validated against the PC image.
// Source and target-specific differences: docs/x-bitmap-reconstruction.md.

#include <rva.h>

#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

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

#define B_NULL       -1,  -1,-1,-1,-1,-1,   -1, -1, -1, -1, -1,  -1,-1,-1,-1,-1

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

#define M_NULL       0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF

DATA(0x002f6cd8)
const xbitmap::format_info xbitmap::m_FormatInfo[ xbitmap::FMT_END_OF_LIST ] =
{

{   FMT_NULL,         "NULL FORMAT",  0,   -1,  -1,  B_NULL,      M_NULL       , 0, -1 },

{   FMT_32_RGBA_8888, "32_RGBA_8888", 0,   32,  32,  B_RGBA_8888, M_RGBA_8888  , 0, 32 },
{   FMT_32_RGBU_8888, "32_RGBU_8888", 0,   32,  32,  B_RGBU_8888, M_RGBU_8888  , 0, 32 },
{   FMT_32_ARGB_8888, "32_ARGB_8888", 0,   32,  32,  B_ARGB_8888, M_ARGB_8888  , 0, 32 },
{   FMT_32_URGB_8888, "32_URGB_8888", 0,   32,  32,  B_URGB_8888, M_URGB_8888  , 0, 32 },
{   FMT_24_RGB_888,   "24_RGB_888",   0,   24,  24,  B_RGB_888  , M_RGB_888    , 0, 24 },
{   FMT_16_RGBA_4444, "16_RGBA_4444", 0,   16,  16,  B_RGBA_4444, M_RGBA_4444  , 0, 16 },
{   FMT_16_ARGB_4444, "16_ARGB_4444", 0,   16,  16,  B_ARGB_4444, M_ARGB_4444  , 0, 16 },
{   FMT_16_RGBA_5551, "16_RGBA_5551", 0,   16,  16,  B_RGBA_5551, M_RGBA_5551  , 0, 16 },
{   FMT_16_RGBU_5551, "16_RGBU_5551", 0,   16,  16,  B_RGBU_5551, M_RGBU_5551  , 0, 16 },
{   FMT_16_ARGB_1555, "16_ARGB_1555", 0,   16,  16,  B_ARGB_1555, M_ARGB_1555  , 0, 16 },
{   FMT_16_URGB_1555, "16_URGB_1555", 0,   16,  16,  B_URGB_1555, M_URGB_1555  , 0, 16 },
{   FMT_16_RGB_565,   "16_RGB_565",   0,   16,  16,  B_RGB_565  , M_RGB_565    , 0, 16 },

{   FMT_32_BGRA_8888, "32_BGRA_8888", 0,   32,  32,  B_BGRA_8888, M_BGRA_8888  , 0, 32 },
{   FMT_32_BGRU_8888, "32_BGRU_8888", 0,   32,  32,  B_BGRU_8888, M_BGRU_8888  , 0, 32 },
{   FMT_32_ABGR_8888, "32_ABGR_8888", 0,   32,  32,  B_ABGR_8888, M_ABGR_8888  , 0, 32 },
{   FMT_32_UBGR_8888, "32_UBGR_8888", 0,   32,  32,  B_UBGR_8888, M_UBGR_8888  , 0, 32 },
{   FMT_24_BGR_888,   "24_BGR_888",   0,   24,  24,  B_BGR_888  , M_BGR_888    , 0, 24 },
{   FMT_16_BGRA_4444, "16_BGRA_4444", 0,   16,  16,  B_BGRA_4444, M_BGRA_4444  , 0, 16 },
{   FMT_16_ABGR_4444, "16_ABGR_4444", 0,   16,  16,  B_ABGR_4444, M_ABGR_4444  , 0, 16 },
{   FMT_16_BGRA_5551, "16_BGRA_5551", 0,   16,  16,  B_BGRA_5551, M_BGRA_5551  , 0, 16 },
{   FMT_16_BGRU_5551, "16_BGRU_5551", 0,   16,  16,  B_BGRU_5551, M_BGRU_5551  , 0, 16 },
{   FMT_16_ABGR_1555, "16_ABGR_1555", 0,   16,  16,  B_ABGR_1555, M_ABGR_1555  , 0, 16 },
{   FMT_16_UBGR_1555, "16_UBGR_1555", 0,   16,  16,  B_UBGR_1555, M_UBGR_1555  , 0, 16 },
{   FMT_16_BGR_565,   "16_BGR_565",   0,   16,  16,  B_BGR_565  , M_BGR_565    , 0, 16 },

{   FMT_P8_RGBA_8888, "P8_RGBA_8888",  1,    8,  32,  B_RGBA_8888, M_RGBA_8888  , 0, 8 },
{   FMT_P8_RGBU_8888, "P8_RGBU_8888",  1,    8,  32,  B_RGBU_8888, M_RGBU_8888  , 0, 8 },
{   FMT_P8_ARGB_8888, "P8_ARGB_8888",  1,    8,  32,  B_ARGB_8888, M_ARGB_8888  , 0, 8 },
{   FMT_P8_URGB_8888, "P8_URGB_8888",  1,    8,  32,  B_URGB_8888, M_URGB_8888  , 0, 8 },
{   FMT_P8_RGB_888,   "P8_RGB_888",    1,    8,  24,  B_RGB_888  , M_RGB_888    , 0, 8 },
{   FMT_P8_RGBA_4444, "P8_RGBA_4444",  1,    8,  16,  B_RGBA_4444, M_RGBA_4444  , 0, 8 },
{   FMT_P8_ARGB_4444, "P8_ARGB_4444",  1,    8,  16,  B_ARGB_4444, M_ARGB_4444  , 0, 8 },
{   FMT_P8_RGBA_5551, "P8_RGBA_5551",  1,    8,  16,  B_RGBA_5551, M_RGBA_5551  , 0, 8 },
{   FMT_P8_RGBU_5551, "P8_RGBU_5551",  1,    8,  16,  B_RGBU_5551, M_RGBU_5551  , 0, 8 },
{   FMT_P8_ARGB_1555, "P8_ARGB_1555",  1,    8,  16,  B_ARGB_1555, M_ARGB_1555  , 0, 8 },
{   FMT_P8_URGB_1555, "P8_URGB_1555",  1,    8,  16,  B_URGB_1555, M_URGB_1555  , 0, 8 },
{   FMT_P8_RGB_565,   "P8_RGB_565",    1,    8,  16,  B_RGB_565  , M_RGB_565    , 0, 8 },

{   FMT_P8_BGRA_8888, "P8_BGRA_8888",  1,    8,  32,  B_BGRA_8888, M_BGRA_8888  , 0, 8 },
{   FMT_P8_BGRU_8888, "P8_BGRU_8888",  1,    8,  32,  B_BGRU_8888, M_BGRU_8888  , 0, 8 },
{   FMT_P8_ABGR_8888, "P8_ABGR_8888",  1,    8,  32,  B_ABGR_8888, M_ABGR_8888  , 0, 8 },
{   FMT_P8_UBGR_8888, "P8_UBGR_8888",  1,    8,  32,  B_UBGR_8888, M_UBGR_8888  , 0, 8 },
{   FMT_P8_BGR_888,   "P8_BGR_888",    1,    8,  24,  B_BGR_888  , M_BGR_888    , 0, 8 },
{   FMT_P8_BGRA_4444, "P8_BGRA_4444",  1,    8,  16,  B_BGRA_4444, M_BGRA_4444  , 0, 8 },
{   FMT_P8_ABGR_4444, "P8_ABGR_4444",  1,    8,  16,  B_ABGR_4444, M_ABGR_4444  , 0, 8 },
{   FMT_P8_BGRA_5551, "P8_BGRA_5551",  1,    8,  16,  B_BGRA_5551, M_BGRA_5551  , 0, 8 },
{   FMT_P8_BGRU_5551, "P8_BGRU_5551",  1,    8,  16,  B_BGRU_5551, M_BGRU_5551  , 0, 8 },
{   FMT_P8_ABGR_1555, "P8_ABGR_1555",  1,    8,  16,  B_ABGR_1555, M_ABGR_1555  , 0, 8 },
{   FMT_P8_UBGR_1555, "P8_UBGR_1555",  1,    8,  16,  B_UBGR_1555, M_UBGR_1555  , 0, 8 },
{   FMT_P8_BGR_565,   "P8_BGR_565",    1,    8,  16,  B_BGR_565  , M_BGR_565    , 0, 8 },

{   FMT_P4_RGBA_8888, "P4_RGBA_8888",  1,    4,  32,  B_RGBA_8888, M_RGBA_8888  , 0, 4 },
{   FMT_P4_RGBU_8888, "P4_RGBU_8888",  1,    4,  32,  B_RGBU_8888, M_RGBU_8888  , 0, 4 },
{   FMT_P4_ARGB_8888, "P4_ARGB_8888",  1,    4,  32,  B_ARGB_8888, M_ARGB_8888  , 0, 4 },
{   FMT_P4_URGB_8888, "P4_URGB_8888",  1,    4,  32,  B_URGB_8888, M_URGB_8888  , 0, 4 },
{   FMT_P4_RGB_888,   "P4_RGB_888",    1,    4,  24,  B_RGB_888  , M_RGB_888    , 0, 4 },
{   FMT_P4_RGBA_4444, "P4_RGBA_4444",  1,    4,  16,  B_RGBA_4444, M_RGBA_4444  , 0, 4 },
{   FMT_P4_ARGB_4444, "P4_ARGB_4444",  1,    4,  16,  B_ARGB_4444, M_ARGB_4444  , 0, 4 },
{   FMT_P4_RGBA_5551, "P4_RGBA_5551",  1,    4,  16,  B_RGBA_5551, M_RGBA_5551  , 0, 4 },
{   FMT_P4_RGBU_5551, "P4_RGBU_5551",  1,    4,  16,  B_RGBU_5551, M_RGBU_5551  , 0, 4 },
{   FMT_P4_ARGB_1555, "P4_ARGB_1555",  1,    4,  16,  B_ARGB_1555, M_ARGB_1555  , 0, 4 },
{   FMT_P4_URGB_1555, "P4_URGB_1555",  1,    4,  16,  B_URGB_1555, M_URGB_1555  , 0, 4 },
{   FMT_P4_RGB_565,   "P4_RGB_565",    1,    4,  16,  B_RGB_565  , M_RGB_565    , 0, 4 },

{   FMT_P4_BGRA_8888, "P4_BGRA_8888",  1,    4,  32,  B_BGRA_8888, M_BGRA_8888  , 0, 4 },
{   FMT_P4_BGRU_8888, "P4_BGRU_8888",  1,    4,  32,  B_BGRU_8888, M_BGRU_8888  , 0, 4 },
{   FMT_P4_ABGR_8888, "P4_ABGR_8888",  1,    4,  32,  B_ABGR_8888, M_ABGR_8888  , 0, 4 },
{   FMT_P4_UBGR_8888, "P4_UBGR_8888",  1,    4,  32,  B_UBGR_8888, M_UBGR_8888  , 0, 4 },
{   FMT_P4_BGR_888,   "P4_BGR_888",    1,    4,  24,  B_BGR_888  , M_BGR_888    , 0, 4 },
{   FMT_P4_BGRA_4444, "P4_BGRA_4444",  1,    4,  16,  B_BGRA_4444, M_BGRA_4444  , 0, 4 },
{   FMT_P4_ABGR_4444, "P4_ABGR_4444",  1,    4,  16,  B_ABGR_4444, M_ABGR_4444  , 0, 4 },
{   FMT_P4_BGRA_5551, "P4_BGRA_5551",  1,    4,  16,  B_BGRA_5551, M_BGRA_5551  , 0, 4 },
{   FMT_P4_BGRU_5551, "P4_BGRU_5551",  1,    4,  16,  B_BGRU_5551, M_BGRU_5551  , 0, 4 },
{   FMT_P4_ABGR_1555, "P4_ABGR_1555",  1,    4,  16,  B_ABGR_1555, M_ABGR_1555  , 0, 4 },
{   FMT_P4_UBGR_1555, "P4_UBGR_1555",  1,    4,  16,  B_UBGR_1555, M_UBGR_1555  , 0, 4 },
{   FMT_P4_BGR_565,   "P4_BGR_565",    1,    4,  16,  B_BGR_565  , M_BGR_565    , 0, 4 },

    { FMT_DXT1, "DXT1", 0, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 16 },
    { FMT_I4, "I4", 0, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 4 },
    { FMT_I8, "I8", 0, 8, 8, 8, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000000, 0x00000000, 0x00000000, 0x000000ff, 0x00000000, 0, 8 },
    { FMT_IA4, "IA4", 0, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 8 },
    { FMT_IA8, "IA8", 0, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 16 },
    { FMT_DXT3, "DXT3", 0, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 32 },
    { FMT_RGB5A3, "RGB5A3", 0, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 1, 16 },
};


RVA(0x00250250, 0xc)
xbitmap::xbitmap( void )
{
    Init();
}

RVA(0x00250260, 0x1a)
xbitmap::xbitmap( const xbitmap& Bitmap )
{
    Init();
    CopyFrom( Bitmap );
}

RVA(0x00250280, 0x5)
xbitmap::~xbitmap( void )
{
    Kill();
}

RVA(0x00250290, 0xb7)
void xbitmap::CopyFrom( const xbitmap& Source )
{

    m_Data.pPixel = static_cast<unsigned char*>(x_malloc_fn(Source.m_DataSize, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 238));
    x_memcpy( m_Data.pPixel, Source.m_Data.pPixel, Source.m_DataSize );

    if( Source.m_ClutSize )
    {
        m_pClut = static_cast<unsigned char*>(x_malloc_fn(Source.m_ClutSize, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 244));
        x_memcpy( m_pClut, Source.m_pClut, Source.m_ClutSize );
    }
    else
    {
        m_pClut = 0;
    }

    m_DataSize = Source.m_DataSize;
    m_ClutSize = Source.m_ClutSize;
    m_Width    = Source.m_Width;
    m_Height   = Source.m_Height;
    m_PW       = Source.m_PW;
    m_VRAMID   = 0;
    m_NMips    = Source.m_NMips;
    m_Format   = Source.m_Format;

    m_Flags = (Source.m_Flags | FLAG_DATA_OWNED);
    if( m_pClut )
        m_Flags |= FLAG_CLUT_OWNED;
}

RVA(0x00250350, 0x27)
void xbitmap::Init( void )
{
    m_Data.pPixel = 0;
    m_pClut       = 0;
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

RVA(0x00250380, 0x42)
void xbitmap::Kill( void )
{

    if( m_Flags & FLAG_DATA_OWNED )  x_free_fn(m_Data.pPixel, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 291);
    if( m_Flags & FLAG_CLUT_OWNED )  x_free_fn(m_pClut, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 292);
    Init();
}

RVA(0x002503d0, 0x1a)
const xbitmap& xbitmap::operator = ( const xbitmap& Bitmap )
{
    Kill();
    CopyFrom( Bitmap );
    return( *this );
}

RVA(0x00250400, 0xea)
xcolor xbitmap::ReadColor( unsigned char* pRead ) const
{
    unsigned int Pixel = 0;
    unsigned int R, G, B, A;
    int Bits;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    if( Format.BPC == 32 )
    {
        // Byte-evidenced 32-bit packed-pixel load; the buffer stores multiple formats.
        Pixel = *reinterpret_cast<unsigned int*>(pRead);
    }
    else
    if( Format.BPC == 24 )
    {
        Pixel = (static_cast<unsigned int>(pRead[0]) << 16) |
                (static_cast<unsigned int>(pRead[1]) <<  8) |
                (static_cast<unsigned int>(pRead[2]) <<  0);
    }
    else
    if( Format.BPC == 16 )
    {
        // Byte-evidenced 16-bit packed-pixel load, zero-extended before unpacking.
        Pixel = static_cast<unsigned int>(*reinterpret_cast<unsigned short*>(pRead));
    }
    else
    {

    }

    R   = Pixel;
    R  &= Format.RMask;
    R >>= Format.RShiftR;
    R <<= Format.RShiftL;
    R  |= (R >> Format.RBits);

    G   = Pixel;
    G  &= Format.GMask;
    G >>= Format.GShiftR;
    G <<= Format.GShiftL;
    G  |= (G >> Format.GBits);

    B   = Pixel;
    B  &= Format.BMask;
    B >>= Format.BShiftR;
    B <<= Format.BShiftL;
    B  |= (B >> Format.BBits);

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

    return( xcolor( static_cast<unsigned char>(R), static_cast<unsigned char>(G), static_cast<unsigned char>(B), static_cast<unsigned char>(A) ) );
}

RVA(0x002504f0, 0xa5)
void xbitmap::WriteColor( unsigned char* pWrite, xcolor Color )
{
    unsigned int R, G, B, A, U;

    unsigned char* Start ;
    unsigned char* End ;
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

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    U = Format.UMask;

    if( (Format.BPC == 32) || (Format.BPC == 16) )
    {
        unsigned int  Encoded;

        R = static_cast<unsigned int>(Color.R);
        G = static_cast<unsigned int>(Color.G);
        B = static_cast<unsigned int>(Color.B);
        A = static_cast<unsigned int>(Color.A);

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
            // Byte-evidenced packed 32-bit store into the format-dependent buffer.
            *reinterpret_cast<unsigned int*>(pWrite) = static_cast<unsigned int>(Encoded);
        }
        else
        {
            // Byte-evidenced packed 16-bit store into the format-dependent buffer.
            *reinterpret_cast<unsigned short*>(pWrite) = static_cast<unsigned short>(Encoded);
        }
    }

    if( Format.BPC == 24 )
    {

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

RVA(0x002505a0, 0x30f)
xcolor xbitmap::GetBilinearColor( float ParamU, float ParamV, int Clamp, int Mip ) const
{

    int W, H;
    if( m_NMips == 0 )
    {

        W = m_Width;
        H = m_Height;
    }
    else
    {

        W = m_Data.pMip[Mip].Width;
        H = m_Data.pMip[Mip].Height;
    }

    float U0 = ParamU*W - 0.5f;
    float V0 = ParamV*H - 0.5f;

    int iU0 = static_cast<int>(x_floor(U0));
    int iV0 = static_cast<int>(x_floor(V0));
    int iU1 = iU0+1;
    int iV1 = iV0+1;

    float UF = U0 - static_cast<float>(iU0);
    float VF = V0 - static_cast<float>(iV0);

    if( Clamp )
    {

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

        iU0 = (iU0 + (W<<16)) % W;
        iU1 = (iU1 + (W<<16)) % W;
        iV0 = (iV0 + (H<<16)) % H;
        iV1 = (iV1 + (H<<16)) % H;
    }

    xcolor CA = GetPixelColor( iU0, iV0, Mip );
    xcolor CB = GetPixelColor( iU1, iV0, Mip );
    xcolor CC = GetPixelColor( iU0, iV1, Mip );
    xcolor CD = GetPixelColor( iU1, iV1, Mip );

    float IA = (1.0f-UF)*(1.0f-VF);
    float IB = (     UF)*(1.0f-VF);
    float IC = (1.0f-UF)*(     VF);
    float ID = (     UF)*(     VF);

    float R = IA*CA.R + IB*CB.R + IC*CC.R + ID*CD.R;
    float G = IA*CA.G + IB*CB.G + IC*CC.G + ID*CD.G;
    float B = IA*CA.B + IB*CB.B + IC*CC.B + ID*CD.B;
    float A = IA*CA.A + IB*CB.A + IC*CC.A + ID*CD.A;

    int r = static_cast<int>(R);
    int g = static_cast<int>(G);
    int b = static_cast<int>(B);
    int a = static_cast<int>(A);

    return( xcolor( r, g, b, a ) );
}

RVA(0x00250a90, 0x16)
static int GetPS2SwizzledIndex( int I )
{
    DATA(0x0034cffc)
    static unsigned char swizzle_lut[32] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    return swizzle_lut[(I) & 31] + (I & ~31);
}

RVA(0x002508b0, 0x1de)
xcolor xbitmap::GetPixelColor( int X, int Y, int Mip ) const
{
    unsigned char* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    int Width;
    if( m_NMips == 0 )
    {
        Width = m_Width;

    }
    else
    {
        Width = m_Data.pMip[Mip].Width;

    }

    if ((m_Flags & FLAG_GCN_DATA_SWIZZLED) && GetBPP() == 32)
    {
        int pixel = (X % 4) + (Y % 4) * 4;
        int tile = ((X / 4) + (Y / 4) * (Width / 4)) * 64;
        pPixel = m_Data.pPixel;
        pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
        xcolor C;
        C.A = (pPixel + tile + 2 * pixel)[0];
        C.R = (pPixel + tile + 2 * pixel)[1];
        C.G = (pPixel + tile + 32 + 2 * pixel)[0];
        C.B = (pPixel + tile + 32 + 2 * pixel)[1];
        return C;
    }
    else if ((m_Flags & FLAG_GCN_DATA_SWIZZLED) && GetBPP() == 24)
        return xcolor(0, 0, 0, 255);
    else if ((m_Flags & FLAG_GCN_DATA_SWIZZLED) && GetBPP() == 16)
        return xcolor(0, 0, 0, 255);
    else if ((m_Flags & FLAG_GCN_DATA_SWIZZLED) && GetBPP() == 8)
        return xcolor(0, 0, 0, 255);
    else if ((m_Flags & FLAG_GCN_DATA_SWIZZLED) && GetBPP() == 4)
        return xcolor(0, 0, 0, 255);

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += ((Y * Width) * Format.BPP) >> 3;
    pPixel += ((   X    ) * Format.BPP) >> 3;

    if( Format.ClutBased )
    {
        int Index;

        if( Format.BPP == 4 )
        {

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

        pPixel  = m_pClut;
        pPixel += (Index * Format.BPC) >> 3;
    }

    return( ReadColor( pPixel ) );
}

RVA(0x00250ab0, 0x93)
int xbitmap::GetPixelIndex( int X, int Y, int Mip ) const
{
    unsigned char* pPixel;
    int   Index;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    int Width;
    if( m_NMips == 0 )
    {
        Width = m_Width;

    }
    else
    {
        Width = m_Data.pMip[Mip].Width;

    }

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += ((Y * Width) * Format.BPP) >> 3;
    pPixel += ((   X    ) * Format.BPP) >> 3;

    if( Format.BPP == 4 )
    {

        if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
            Index = (X & 0x01) ? (*pPixel >> 4) : (*pPixel & 0x0F);
        else
            Index = (X & 0x01) ? (*pPixel & 0x0F) : (*pPixel >> 4);
    }
    else
    {
        Index = *pPixel;
    }

    return( Index );
}

RVA(0x00250b50, 0x44)
xcolor xbitmap::GetClutColor( int Index ) const
{
    unsigned char* pColor;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        Index = GetPS2SwizzledIndex( Index );

    pColor  = m_pClut;
    pColor += (Index * Format.BPC) >> 3;

    return( ReadColor( pColor ) );
}

RVA(0x00250ba0, 0x75)
void xbitmap::SetPixelColor( xcolor Color, int X, int Y, int Mip )
{
    unsigned char* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    int Width;
    if( m_NMips == 0 )
    {
        Width = m_Width;

    }
    else
    {
        Width = m_Data.pMip[Mip].Width;

    }

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += ((Y * Width) * Format.BPP) >> 3;
    pPixel += ((   X    ) * Format.BPP) >> 3;

    WriteColor( pPixel, Color );
}

RVA(0x00250c20, 0xbc)
void xbitmap::SetPixelIndex( int ParamIndex, int X, int Y, int Mip )
{
    unsigned char Index = static_cast<unsigned char>(ParamIndex);
    unsigned char* pPixel;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    int Width;
    if( m_NMips == 0 )
    {
        Width = m_Width;

    }
    else
    {
        Width = m_Data.pMip[Mip].Width;

    }

    pPixel  = m_Data.pPixel;
    pPixel += m_NMips ? m_Data.pMip[Mip].Offset : 0;
    pPixel += ((Y * Width) * Format.BPP) >> 3;
    pPixel += ((   X    ) * Format.BPP) >> 3;

    if( Format.BPP == 4 )
    {
        unsigned char Byte;

        if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
        {
            if( X & 0x01 )
                Byte = (*pPixel & 0x0F) | (Index <<   4);
            else
                Byte = (*pPixel & 0xF0) | (static_cast<unsigned char>(Index) & 0x0F);
        }
        else
        {
            if( X & 0x01 )
                Byte = (*pPixel & 0xF0) | (static_cast<unsigned char>(Index) & 0x0F);
            else
                Byte = (*pPixel & 0x0F) | (Index <<   4);
        }

        *pPixel = Byte;
    }
    else
    {
        *pPixel = static_cast<unsigned char>(Index);
    }
}

RVA(0x00250ce0, 0x54)
void xbitmap::SetClutColor( xcolor Color, int Index )
{
    unsigned char* pColor;

    const xbitmap::format_info& Format = m_FormatInfo[m_Format];

    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        Index = GetPS2SwizzledIndex( Index );

    pColor  = m_pClut;
    pColor += (Index * Format.BPC) >> 3;

    WriteColor( pColor, Color );
}

RVA(0x00250d40, 0x150)
void xbitmap::Blit( int DestX, int DestY,
                    int SrcX,  int SrcY,
                    int Width, int Height,
                    const xbitmap& SourceBitmap )
{
    const xbitmap::format_info& SFormat = SourceBitmap.GetFormatInfo();
    const xbitmap::format_info& DFormat =              GetFormatInfo();

    int BytesPerBlitRow;
    int BytesPerReadRow;
    int BytesPerWriteRow;
    const unsigned char* pRead;
    unsigned char* pWrite;

    if( DestX < 0 )     { Width  += DestX;  SrcX -= DestX;  DestX = 0; }
    if( DestY < 0 )     { Height += DestY;  SrcY -= DestY;  DestY = 0; }

    if( DestX+Width  > m_Width  )       { Width  = m_Width  - DestX; }
    if( DestY+Height > m_Height )       { Height = m_Height - DestY; }

    if( Width  <= 0 )   return;
    if( Height <= 0 )   return;

    BytesPerBlitRow  = (SFormat.BPP * Width) >> 3;
    BytesPerReadRow  = (SFormat.BPP * SourceBitmap.m_PW) >> 3;
    BytesPerWriteRow = (DFormat.BPP *              m_PW) >> 3;

    pRead   = SourceBitmap.GetPixelData();
    pRead  +=   SrcY * BytesPerReadRow;
    pRead  += ((SrcX * SFormat.BPP) >> 3);

    pWrite  = const_cast<unsigned char*>(GetPixelData());
    pWrite +=   DestY * BytesPerWriteRow;
    pWrite += ((DestX * DFormat.BPP) >> 3);

    while( Height > 0 )
    {
        x_memcpy( pWrite, pRead, BytesPerBlitRow );
        pRead  += BytesPerReadRow;
        pWrite += BytesPerWriteRow;
        Height -= 1;
    }
}

RVA(0x00250e90, 0x11e)
void xbitmap::Setup( format    Format,
                     int       Width,
                     int       Height,
                     int     DataOwned,
                     unsigned char*     pPixelData,
                     int     ClutOwned,
                     unsigned char*     pClutData,
                     int       PhysicalWidth, int NMips )
{

    if( PhysicalWidth == -1 )
        PhysicalWidth = Width;

    Kill();

    m_Data.pPixel = pPixelData;
    m_pClut       = pClutData;
    if (NMips == 0)
        m_DataSize = (Width * Height * m_FormatInfo[Format].BPP) >> 3;
    else
    {
        int size = ((NMips + 1) * sizeof(mip) + 31) & ~31;
        int W = Width;
        int H = Height;
        for (int i = 0; i <= NMips; ++i)
        {
            size += (W * H * m_FormatInfo[Format].BPP) >> 3;
            W /= 2;
            H /= 2;
        }
        m_DataSize = size;
    }
    m_ClutSize    = 0;
    m_Width       = Width;
    m_Height      = Height;
    m_PW          = PhysicalWidth;
    m_VRAMID      = 0;
    m_Flags       = FLAG_VALID;
    m_NMips       = NMips;
    m_Format      = Format;

    if( DataOwned )
        m_Flags |= FLAG_DATA_OWNED;

    if( pClutData )
    {
        int ClutEntries = (1 << m_FormatInfo[Format].BPP);
        m_ClutSize = (ClutEntries * m_FormatInfo[Format].BPC) >> 3;
        if( ClutOwned )
            m_Flags |= FLAG_CLUT_OWNED;
    }
}

RVA(0x00250fb0, 0x17)
int xbitmap::HasAlphaBits( void ) const
{
    return( m_FormatInfo[m_Format].ABits > 0 );
}

RVA(0x00250fd0, 0xe)
int xbitmap::IsClutBased( void ) const
{
    return( m_FormatInfo[m_Format].ClutBased );
}

RVA(0x00250fe0, 0xc)
int xbitmap::IsCompressed() const { return m_Format == FMT_DXT1; }

RVA(0x00250ff0, 0xe0)
void xbitmap::PS2SwizzleClut  ( void )
{
    if( m_Flags & FLAG_PS2_CLUT_SWIZZLED )
        return;

    if( GetBPP() == 8 )
    {
        int     i, j, idx;
        unsigned int*    C;
        unsigned int     Clut8[256];

        // Byte-evidenced 256-entry palette permutation copies complete 32-bit colors.
        C = reinterpret_cast<unsigned int*>(m_pClut);

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

RVA(0x002510d0, 0xe0)
void xbitmap::PS2UnswizzleClut( void )
{
    if( (m_Flags & FLAG_PS2_CLUT_SWIZZLED) == 0 )
        return;

    if( GetBPP() == 8 )
    {
        int     i, j, idx;
        unsigned int*    C;
        unsigned int     Clut8[256];

        // Byte-evidenced 256-entry palette permutation copies complete 32-bit colors.
        C = reinterpret_cast<unsigned int*>(m_pClut);

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

RVA(0x002511b0, 0xaa)
void xbitmap::Flip4BitNibbles( void )
{

    if( GetBPP() != 4 )
        return;

    if (m_Flags & FLAG_4BIT_NIBBLES_FLIPPED)
        return ;

    for( int M=0; M<GetNMips()+1; M++ )
    {

        unsigned char* pC = const_cast<unsigned char*>(GetPixelData(M));
        int   NBytes = GetMipDataSize(M);

        for( int i=0; i<NBytes; i++ )
        {
            int N0 = (pC[i]>>4) & 0x0F;
            int N1 = (pC[i]>>0) & 0x0F;
            pC[i] = (N1<<4) | (N0<<0);
        }
    }

    m_Flags |= FLAG_4BIT_NIBBLES_FLIPPED ;
}

RVA(0x00251260, 0xaa)
void xbitmap::Unflip4BitNibbles( void )
{

    if( GetBPP() != 4 )
        return;

    if (!(m_Flags & FLAG_4BIT_NIBBLES_FLIPPED))
        return ;

    for( int M=0; M<GetNMips()+1; M++ )
    {

        unsigned char* pC = const_cast<unsigned char*>(GetPixelData(M));
        int   NBytes = GetMipDataSize(M);

        for( int i=0; i<NBytes; i++ )
        {
            int N0 = (pC[i]>>4) & 0x0F;
            int N1 = (pC[i]>>0) & 0x0F;
            pC[i] = (N1<<4) | (N0<<0);
        }
    }

    m_Flags &= ~FLAG_4BIT_NIBBLES_FLIPPED ;
}


#define ALIGN_32(x) (((x)+31)&~31)

RVA(0x00251310, 0xdb)
void xbitmap::GCNPackTileRGBA8( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip )
{
    unsigned int row, col;
    unsigned int realRows, realCols;
    unsigned char* arPtr, *gbPtr;

    realRows = GetHeight(Mip) - y;
    realCols = GetWidth(Mip)  - x;

    if( realRows > 4)
        realRows = 4;

    if(realCols > 4)
        realCols = 4;

    for(row=0; row<realRows; row++)
    {

        arPtr = dstPtr  +      (row * 8);
        gbPtr = dstPtr  + 32 + (row * 8);

        for(col=0; col<realCols; col++)
        {
            xcolor C = GetPixelColor( (x+col), (y+row), Mip );

            *arPtr       = C.A;
            *(arPtr + 1) = C.R;

            *gbPtr       = C.G;
            *(gbPtr + 1) = C.B;

            arPtr += 2;
            gbPtr += 2;

        }
    }
}

RVA(0x002513f0, 0xed)
void xbitmap::GCNPackTileRGB565( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip )
{
    unsigned int row, col;
    unsigned int realRows, realCols;
    unsigned char* pData;

    realRows = GetHeight(Mip) - y;
    realCols = GetWidth(Mip)  - x;

    if( realRows > 4)
        realRows = 4;

    if(realCols > 4)
        realCols = 4;

    for(row=0; row<realRows; row++)
    {

        pData = dstPtr  + (row * 8);

        for(col=0; col<realCols; col++)
        {
            xcolor C = GetPixelColor( (x+col), (y+row), Mip );

            *pData       = ( ( C.R & 0xF8)       | ((C.G & 0xE0) >> 5) );
			*(pData + 1) = ( ((C.G & 0x1C) << 3) | ( C.B >> 3)         );

			pData += 2;

        }
    }

}

RVA(0x002514e0, 0xd8)
void xbitmap::GCNPackTile_C8( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip )
{
    unsigned int     row, col;
    unsigned int     realRows, realCols;
    unsigned char*     pData;

    realRows = GetHeight(Mip) - y;
    realCols = GetWidth(Mip)  - x;

    if( realRows > 4)
        realRows = 4;

    if(realCols > 8)
        realCols = 8;

    for(row=0; row<realRows; row++)
    {
        pData = dstPtr + (row * 8);

        for(col=0; col<realCols; col++)
        {

            *pData++ = GetPixelIndex( x+col, y+row, Mip );

        }
    }
}

RVA(0x002515c0, 0xde)
void xbitmap::GCNPackTile_C4( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip )
{
    unsigned int     row, col;
    unsigned int     realRows, realCols;
    unsigned char*     pData;

    realRows = GetHeight(Mip) - y;
    realCols = GetWidth(Mip)  - x;

    if( realRows > 8)
        realRows = 8;

    if(realCols > 8)
        realCols = 8;

    for(row=0; row<realRows; row++)
    {
        pData = dstPtr + (row * 4);

        for(col=0; col<realCols; col++)
        {
            unsigned char  Idx = GetPixelIndex( x+col, y+row, Mip );

            if( col %2 == 0 )
			{
				*pData = ((Idx & 0x0F) << 4);
			}
			else
			{
				*pData |= (Idx & 0x0F);
				pData++;
			}
        }
    }
}

RVA(0x002516a0, 0xcd)
unsigned char* xbitmap::GCNSwizzleRGBA8 ( unsigned char* pDestBuffer )
{
    unsigned int     NTileRows, CurRow;
    unsigned int     NTileCols, CurCol;
    unsigned char*     pDest;
    unsigned int     W, H;
    int     NMips;
    unsigned char*   pOrigDest = pDestBuffer;

    NMips = GetNMips();

    int     i;
    for (i=0;i<=NMips;i++)
    {
        W = GetWidth(i);
        H = GetHeight(i);

        NTileCols = ((W + 3) >> 2);
        NTileRows = ((H + 3) >> 2);

        pDest = pDestBuffer;

        for( CurRow=0; CurRow<NTileRows; CurRow++ )
        {
            for(CurCol=0; CurCol<NTileCols; CurCol++)
            {
                GCNPackTileRGBA8( (CurCol * 4), (CurRow * 4), pDest, i);
                pDest += 64;
            }
        }

        pDestBuffer = pOrigDest + ALIGN_32(reinterpret_cast<int>(pDest) - reinterpret_cast<int>(pOrigDest));

        W /= 2;
        H /= 2;
    }

    return pDestBuffer;
}

RVA(0x00251770, 0xcd)
unsigned char* xbitmap::GCNSwizzleRGB565( unsigned char* pDestBuffer )
{
    unsigned int     NTileRows, CurRow;
    unsigned int     NTileCols, CurCol;
    unsigned char*     pDest;
    unsigned int     W, H;
    int     NMips;
    unsigned char*   pOrigDest = pDestBuffer;

    NMips = GetNMips();

    int     i;
    for (i=0;i<=NMips;i++)
    {
        W = GetWidth(i);
        H = GetHeight(i);

        NTileCols = ((W + 3) >> 2);
        NTileRows = ((H + 3) >> 2);

        pDest = pDestBuffer;

        for( CurRow=0; CurRow<NTileRows; CurRow++ )
        {
            for(CurCol=0; CurCol<NTileCols; CurCol++)
            {
                GCNPackTileRGB565( (CurCol * 4), (CurRow * 4), pDest, i);
                pDest += 32;
            }
        }

        pDestBuffer = pOrigDest + ALIGN_32(reinterpret_cast<int>(pDest) - reinterpret_cast<int>(pOrigDest));

        W /= 2;
        H /= 2;
    }

    return pDestBuffer;
}

RVA(0x00251840, 0xec)
unsigned char* xbitmap::GCNSwizzleRGBC8 ( unsigned char* pDestBuffer )
{
    unsigned int     NTileRows, CurRow;
    unsigned int     NTileCols, CurCol;
    unsigned char*     pDest;
    unsigned int     W, H;
    int     NMips;
    unsigned char*   pOrigDest = pDestBuffer;

    NMips = GetNMips();

    W = GetWidth(0);
    H = GetHeight(0);

    int     i;
    for (i=0;i<=NMips;i++)
    {

        NTileCols = ((W + 7) >> 3);
        NTileRows = ((H + 3) >> 2);

        pDest = pDestBuffer;

        for( CurRow=0; CurRow<NTileRows; CurRow++ )
        {
            for(CurCol=0; CurCol<NTileCols; CurCol++)
            {
                GCNPackTile_C8( (CurCol * 8), (CurRow * 4), pDest, i);
                pDest += 32;
            }
        }

        pDestBuffer = pOrigDest + ALIGN_32(reinterpret_cast<int>(pDest) - reinterpret_cast<int>(pOrigDest));

        W /= 2;
        H /= 2;
    }

    return pDestBuffer;
}

RVA(0x00251930, 0xec)
unsigned char* xbitmap::GCNSwizzleRGBC4( unsigned char* pDestBuffer )
{
    unsigned int     NTileRows, CurRow;
    unsigned int     NTileCols, CurCol;
    unsigned char*     pDest;
    unsigned int     W, H;
    int     NMips;
    unsigned char*   pOrigDest = pDestBuffer;

    NMips = GetNMips();

    W = GetWidth(0);
    H = GetWidth(0);

    int     i;
    for (i=0;i<=NMips;i++)
    {

        NTileCols = ((W + 7) >> 3);
        NTileRows = ((H + 7) >> 3);

        pDest = pDestBuffer;

        for( CurRow=0; CurRow<NTileRows; CurRow++ )
        {
            for(CurCol=0; CurCol<NTileCols; CurCol++)
            {
                GCNPackTile_C4( (CurCol * 8), (CurRow * 8), pDest, i);
                pDest += 32;
            }
        }

        pDestBuffer = pOrigDest + ALIGN_32(reinterpret_cast<int>(pDest) - reinterpret_cast<int>(pOrigDest));

        W /= 2;
        H /= 2;
    }

    return pDestBuffer;
}

RVA(0x00251a20, 0x17c)
void xbitmap::GCNSwizzleData  ( void )
{

    if (m_Flags & FLAG_GCN_DATA_SWIZZLED)
        return;

    if (GetFormat() == xbitmap::FMT_DXT1)
    {
        GCNSwizzleDXT1();
        return;
    }

    unsigned char*   pSwizzledData;
    int     SwizzledSize = 0;
    int     BPP = 0;

    switch( GetBPP() )
    {
    case 4:
        BPP = 4;
        break;
    case 8:
        BPP = 8;
        break;
    case 16:
        BPP = 16;
        break;
    case 24:
        BPP = 32;
        break;
    case 32:
        BPP = 32;
        break;
    default:
        break;

    }

    int     NMips = GetNMips();
    int     W = m_Width;
    int     H = m_Height;
    int     i;

    for (i=0;i<=NMips;i++)
    {

        SwizzledSize += ALIGN_32( W * H * BPP / 8 );
        W /= 2;
        H /= 2;
    }

    int Offset = 0;

    if (NMips > 0)
    {

        SwizzledSize += ALIGN_32(m_Data.pMip[0].Offset);
        Offset = ALIGN_32(m_Data.pMip[0].Offset);
    }

    pSwizzledData = static_cast<unsigned char*>(x_malloc_fn(SwizzledSize, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 1721));

    x_memset(pSwizzledData,0,SwizzledSize);

    unsigned char* Ret;
    switch( GetBPP() )
    {
        case 4:
            Ret = GCNSwizzleRGBC4( pSwizzledData+Offset );

            break;
        case 8:
            Ret = GCNSwizzleRGBC8( pSwizzledData+Offset );

            break;
        case 16:
            Ret = GCNSwizzleRGB565( pSwizzledData+Offset );

            break;
        case 24:

            break;
        case 32:
            Ret = GCNSwizzleRGBA8( pSwizzledData+Offset );

            break;
    }

    if (NMips > 0)
    {

        x_memcpy( pSwizzledData, m_Data.pPixel, Offset );
    }

    if (m_Flags & FLAG_DATA_OWNED)
        x_free_fn(m_Data.pPixel, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 1756);

    m_Data.pPixel = pSwizzledData;

    m_Flags |= FLAG_DATA_OWNED | FLAG_GCN_DATA_SWIZZLED;

}

RVA(0x00251c10, 0x1)
void xbitmap::GCNUnswizzleData( void )
{
    switch( GetBPP() )
    {
        case 4:
            break;
        case 8:
            break;
        case 16:
            break;
        case 24:
            break;
        case 32:
            break;
    }
}

RVA(0x00251c20, 0x46)
int xbitmap::ReplaceAlphaWithRed ( void )
{

    if(!( m_Format == FMT_32_ARGB_8888 || m_Format == FMT_32_URGB_8888))
    {

        return 0;
    }

    unsigned char *pData = m_Data.pPixel;
    for(int i = 0; i < m_Width * m_Height; i++)
    {
        pData[i * 4 + 3] = pData[i * 4 + 2];
    }

    return 1;
}

RVA(0x00251c70, 0x182)
void xbitmap::GCNSwizzleDXT1  ( void )
{

    unsigned int     NTileRows, CurRow;
    unsigned int     NTileCols, CurCol;
    unsigned int     W, H;
    int     NMips;
    unsigned char*   pDestBuffer = static_cast<unsigned char*>(x_malloc_fn(GetDataSize(), "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 1824));
    unsigned char*   pDest;
    unsigned char*   pOrigDest = pDestBuffer;

    NMips = GetNMips();

    W = GetWidth(0);
    H = GetHeight(0);

    int     i;

    if (NMips > 0)
        pDestBuffer += m_Data.pMip[0].Offset;

    for (i=0;i<=NMips;i++)
    {
        NTileCols = ((W + 7) >> 3);
        NTileRows = ((H + 7) >> 3);

        const unsigned char*   pSrc = GetPixelData( i );

        pDest = pDestBuffer;

        for( CurRow=0; CurRow<NTileRows; CurRow++ )
        {
            for(CurCol=0; CurCol<NTileCols; CurCol++)
            {

                int     sY = CurRow * NTileCols * 4;
                int     sX = CurCol * 2;

                const unsigned char*   pTileSrcRow0 = &(pSrc[ (sY + sX) * 8 ]);
                const unsigned char*   pTileSrcRow1 = pTileSrcRow0 + NTileCols*2*8;

                x_memcpy( pDest+ 0, pTileSrcRow0, 16 );
                x_memcpy( pDest+16, pTileSrcRow1, 16 );

                pDest += 32;
            }
        }

        pDestBuffer = pDest;

        W /= 2;
        H /= 2;
    }

    if (NMips > 0)
    {

        x_memcpy( pOrigDest, m_Data.pPixel, m_Data.pMip[0].Offset );
    }

    if (m_Flags & FLAG_DATA_OWNED)
        x_free_fn(m_Data.pPixel, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bitmap.cpp", 1884);

    m_Data.pPixel = pOrigDest;

    m_Flags |= FLAG_DATA_OWNED | FLAG_GCN_DATA_SWIZZLED;

}

#undef ALIGN_32
