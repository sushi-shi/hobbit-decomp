#ifndef HOBBIT_X_COLOR_HPP
#define HOBBIT_X_COLOR_HPP

#include <rva.h>
#include <xCore/x_files/x_types.hpp>
int x_irand(int minimum, int maximum);

// Shared Entropy color interface; byte order follows both sibling trees.
// PC random::color is validated against this layout before source admission.
struct xcolor
{
    RVA(0x0001def0, 0x3)
    xcolor() {}

    RVA(0x00025250, 0x1f)
    xcolor(const xcolor& color)
    {
        A = color.A;
        R = color.R;
        G = color.G;
        B = color.B;
    }

    RVA(0x00013480, 0x20)
    xcolor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
        { A = alpha; R = red; G = green; B = blue; }

    xcolor(unsigned int value)
        { A = value >> 24; R = value >> 16; G = value >> 8; B = value; }

    const xcolor& operator=(const xcolor& value)
        { A = value.A; R = value.R; G = value.G; B = value.B; return *this; }

    void Set(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
        { A = alpha; R = red; G = green; B = blue; }

    // Genuine sibling API additions; current four-byte PC layout retained.
    void Set(u32 value) { A = value >> 24; R = value >> 16; G = value >> 8; B = value; }
    const xcolor& operator=(u32 value) { Set(value); return *this; }
    operator const u32() const { return *((const u32*)this); }
    u32 GetRGBA() const { return (R << 24) | (G << 16) | (B << 8) | A; }
    void SetfRGBA(f32 r, f32 g, f32 b, f32 a = 1) {
        R = (u8)fMin(fMax(r * 255, 0), 255);
        G = (u8)fMin(fMax(g * 255, 0), 255);
        B = (u8)fMin(fMax(b * 255, 0), 255);
        A = (u8)fMin(fMax(a * 255, 0), 255);
    }
    void GetfRGBA(f32& r, f32& g, f32& b, f32& a) {
        f32 scale = 1.0f / 255.0f;
        r = R * scale; g = G * scale; b = B * scale; a = A * scale;
    }
    void Random(u8 alpha = 255) {
        A = alpha; B = (u8)x_irand(0, 255); G = (u8)x_irand(0, 255); R = (u8)x_irand(0, 255);
    }
    xcolor& operator+=(xcolor color) {
        R = (u8)iMin(255, R + (s32)color.R);
        G = (u8)iMin(255, G + (s32)color.G);
        B = (u8)iMin(255, B + (s32)color.B);
        return *this;
    }

    unsigned char B, G, R, A;
};

inline xcolor operator*(xcolor color, f32 intensity) {
    intensity = fMin(1, intensity);
    intensity = fMax(0, intensity);
    return xcolor((u8)(color.R * intensity), (u8)(color.G * intensity), (u8)(color.B * intensity), color.A);
}
#define ARGB(a,r,g,b) (((u32)(a)<<24)|((u32)(r)<<16)|((u32)(g)<<8)|(u32)(b))
#define XCOLOR_BLACK xcolor(0,0,0)
#define XCOLOR_WHITE xcolor(255,255,255)
#define XCOLOR_RED xcolor(255,0,0)
#define XCOLOR_GREEN xcolor(0,255,0)
#define XCOLOR_BLUE xcolor(0,0,255)
#define XCOLOR_AQUA xcolor(0,255,255)
#define XCOLOR_PURPLE xcolor(255,0,255)
#define XCOLOR_YELLOW xcolor(255,255,0)
#define XCOLOR_GREY xcolor(127,127,127)

#endif
