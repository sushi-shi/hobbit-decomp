#ifndef HOBBIT_X_BITMAP_HPP
#define HOBBIT_X_BITMAP_HPP

#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_stdio.hpp>

// Initial PC layout is established by CopyFrom/Init (RVA 0x250290/0x250350).
// The final two bytes are natural alignment, not reconstructed storage fields.
class xbitmap
{
public:
    enum format
    {
        FMT_NULL = 0,

        FMT_32_RGBA_8888 =  1,          FMT_32_BGRA_8888 = 13,
        FMT_32_RGBU_8888 =  2,          FMT_32_BGRU_8888 = 14,
        FMT_32_ARGB_8888 =  3,          FMT_32_ABGR_8888 = 15,
        FMT_32_URGB_8888 =  4,          FMT_32_UBGR_8888 = 16,
        FMT_24_RGB_888   =  5,          FMT_24_BGR_888   = 17,
        FMT_16_RGBA_4444 =  6,          FMT_16_BGRA_4444 = 18,
        FMT_16_ARGB_4444 =  7,          FMT_16_ABGR_4444 = 19,
        FMT_16_RGBA_5551 =  8,          FMT_16_BGRA_5551 = 20,
        FMT_16_RGBU_5551 =  9,          FMT_16_BGRU_5551 = 21,
        FMT_16_ARGB_1555 = 10,          FMT_16_ABGR_1555 = 22,
        FMT_16_URGB_1555 = 11,          FMT_16_UBGR_1555 = 23,
        FMT_16_RGB_565   = 12,          FMT_16_BGR_565   = 24,

        FMT_P8_RGBA_8888 = 25,          FMT_P8_BGRA_8888 = 37,
        FMT_P8_RGBU_8888 = 26,          FMT_P8_BGRU_8888 = 38,
        FMT_P8_ARGB_8888 = 27,          FMT_P8_ABGR_8888 = 39,
        FMT_P8_URGB_8888 = 28,          FMT_P8_UBGR_8888 = 40,
        FMT_P8_RGB_888   = 29,          FMT_P8_BGR_888   = 41,
        FMT_P8_RGBA_4444 = 30,          FMT_P8_BGRA_4444 = 42,
        FMT_P8_ARGB_4444 = 31,          FMT_P8_ABGR_4444 = 43,
        FMT_P8_RGBA_5551 = 32,          FMT_P8_BGRA_5551 = 44,
        FMT_P8_RGBU_5551 = 33,          FMT_P8_BGRU_5551 = 45,
        FMT_P8_ARGB_1555 = 34,          FMT_P8_ABGR_1555 = 46,
        FMT_P8_URGB_1555 = 35,          FMT_P8_UBGR_1555 = 47,
        FMT_P8_RGB_565   = 36,          FMT_P8_BGR_565   = 48,

        FMT_P4_RGBA_8888 = 49,          FMT_P4_BGRA_8888 = 61,
        FMT_P4_RGBU_8888 = 50,          FMT_P4_BGRU_8888 = 62,
        FMT_P4_ARGB_8888 = 51,          FMT_P4_ABGR_8888 = 63,
        FMT_P4_URGB_8888 = 52,          FMT_P4_UBGR_8888 = 64,
        FMT_P4_RGB_888   = 53,          FMT_P4_BGR_888   = 65,
        FMT_P4_RGBA_4444 = 54,          FMT_P4_BGRA_4444 = 66,
        FMT_P4_ARGB_4444 = 55,          FMT_P4_ABGR_4444 = 67,
        FMT_P4_RGBA_5551 = 56,          FMT_P4_BGRA_5551 = 68,
        FMT_P4_RGBU_5551 = 57,          FMT_P4_BGRU_5551 = 69,
        FMT_P4_ARGB_1555 = 58,          FMT_P4_ABGR_1555 = 70,
        FMT_P4_URGB_1555 = 59,          FMT_P4_UBGR_1555 = 71,
        FMT_P4_RGB_565   = 60,          FMT_P4_BGR_565   = 72,

        FMT_DXT1 = 73,
        FMT_I4 = 74,
        FMT_I8 = 75,
        FMT_IA4 = 76,
        FMT_IA8 = 77,
        FMT_DXT3 = 78,
        FMT_RGB5A3 = 80,
        FMT_END_OF_LIST = 81,
        FMT_XCOLOR = FMT_32_ARGB_8888
    };

    struct format_info
    {
        format  Format;
        char    String[16];
        int   ClutBased;
        int     BPP;
        int     BPC;
        int     BitsUsed;
        int     RBits;
        int     GBits;
        int     BBits;
        int     ABits;
        int     UBits;
        int     RShiftR;
        int     GShiftR;
        int     BShiftR;
        int     AShiftR;
        int     UShiftR;
        int     RShiftL;
        int     GShiftL;
        int     BShiftL;
        int     AShiftL;
        int     UShiftL;
        unsigned int     RMask;
        unsigned int     GMask;
        unsigned int     BMask;
        unsigned int     AMask;
        unsigned int     UMask;
        // Additional PC words absent from the earlier Tribes record.
        int PlatformSpecific;
        int DecodedBPP;
    };
    // Original sibling metadata API, no additional object fields.
    struct info { int W; int H; int nMips; format Format; };
    static int Info(const char* filename, info& bitmapInfo);
    static const format_info m_FormatInfo[FMT_END_OF_LIST];

    xbitmap();
    xbitmap(const xbitmap& bitmap);
    ~xbitmap();
    const xbitmap& operator=(const xbitmap& bitmap);
    void Kill();
    int Save(X_FILE* file) const;
    int Save(const char* filename) const;
    int Load(X_FILE* file);
    int Load(const char* filename);
    void ToggleEndian();
    void ConvertFormat(format format);
    void MakeIntensityAlpha();
    void BuildMips(int mipCount = 15, int forcePunchthrough = 0);
    int DumpSourceCode(const char* filename) const;
    int SavePaletteTGA(const char* filename) const;
    int SaveTGA(const char* filename) const;
    int SaveMipsTGA(const char* filename) const;
    int GetDataSize() const { return m_DataSize; }
    int GetClutSize() const { return m_ClutSize; }
    unsigned int GetFlags() const { return m_Flags; }
    format GetFormat() const { return static_cast<format>(m_Format); }
    const unsigned char* GetClutData() const { return m_pClut; }
    int GetWidth(int mip = 0) const
        { if (m_NMips == 0) return m_Width; else return m_Data.pMip[mip].Width; }
    int GetHeight(int mip = 0) const
        { if (m_NMips == 0) return m_Height; else return m_Data.pMip[mip].Height; }
    int GetPWidth(int mip = 0) const
        { if (m_NMips == 0) return m_PW; else return m_Data.pMip[mip].Width; }
    xcolor GetBilinearColor(float u, float v, int clamp = 0, int mip = 0) const;
    xcolor GetPixelColor(int x, int y, int mip = 0) const;
    int GetPixelIndex(int x, int y, int mip = 0) const;
    xcolor GetClutColor(int index) const;
    void SetPixelColor(xcolor color, int x, int y, int mip = 0);
    void SetPixelIndex(int index, int x, int y, int mip = 0);
    void SetClutColor(xcolor color, int index);
    void Blit(int destX, int destY, int srcX, int srcY, int width, int height, const xbitmap& source);
    void Setup(format format, int width, int height, int dataOwned, unsigned char* data, int clutOwned = 0, unsigned char* clut = 0, int physicalWidth = -1, int mipCount = 0);
    int HasAlphaBits() const;
    int IsClutBased() const;
    int IsCompressed() const;
    void PS2SwizzleClut();
    void PS2UnswizzleClut();
    void Flip4BitNibbles();
    void Unflip4BitNibbles();
    const format_info& GetFormatInfo() const { return m_FormatInfo[m_Format]; }
    static const format_info& GetFormatInfo(format value) { return m_FormatInfo[value]; }
    int GetBPP() const { return m_FormatInfo[m_Format].BPP; }
    int GetNMips() const { return m_NMips; }
    const unsigned char* GetPixelData(int mip = 0) const
    {
        if (mip == -1) return m_Data.pPixel;
        else if (m_NMips == 0) return m_Data.pPixel;
        else return m_Data.pPixel + m_Data.pMip[mip].Offset;
    }
    int GetMipDataSize(int mip = 0) const
    {
        if (m_NMips == 0) return m_DataSize;
        else return (m_Data.pMip[mip].Width * m_Data.pMip[mip].Height * m_FormatInfo[m_Format].BPP) >> 3;
    }

    // Genuine sibling methods use existing fields; no new layout storage.
    int GetBPC() const { return m_FormatInfo[m_Format].BPC; }
    int GetVRAMID() const { return m_VRAMID; }
    void SetVRAMID(int id) const { m_VRAMID = id; }
    void Resize(int width, int height, int punchthrough = 0);
    void Crop(int x, int y, int width, int height);

    void GCNSwizzleData  ( void );
    void GCNUnswizzleData( void );
    int ReplaceAlphaWithRed ( void );



protected:
    void GCNPackTileRGBA8( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip );
    void GCNPackTileRGB565( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip );
    void GCNPackTile_C8( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip );
    void GCNPackTile_C4( unsigned int x, unsigned int y, unsigned char* dstPtr, int Mip );
    unsigned char* GCNSwizzleRGBA8 ( unsigned char* pDestBuffer );
    unsigned char* GCNSwizzleRGB565( unsigned char* pDestBuffer );
    unsigned char* GCNSwizzleRGBC8 ( unsigned char* pDestBuffer );
    unsigned char* GCNSwizzleRGBC4( unsigned char* pDestBuffer );
    void GCNSwizzleDXT1  ( void );

    struct mip
    {
        int Offset;
        short Width;
        short Height;
    };
    enum flags
    {
        FLAG_VALID = 1,
        FLAG_DATA_OWNED = 2,
        FLAG_CLUT_OWNED = 4,
        FLAG_PS2_CLUT_SWIZZLED = 0x100,
        FLAG_4BIT_NIBBLES_FLIPPED = 0x200,
        FLAG_GCN_DATA_SWIZZLED = 0x800
    };
    void CopyFrom(const xbitmap& source);
    void Init();
    xcolor ReadColor(unsigned char* data) const;
    void WriteColor(unsigned char* data, xcolor color);
    union
    {
        unsigned char* pPixel;
        mip* pMip;
    } m_Data;
    unsigned char* m_pClut;
    int m_DataSize;
    int m_ClutSize;
    mutable int m_VRAMID;
    short m_Width;
    short m_Height;
    short m_PW;
    unsigned short m_Flags;
    signed char m_NMips;
    signed char m_Format;
};

#endif
