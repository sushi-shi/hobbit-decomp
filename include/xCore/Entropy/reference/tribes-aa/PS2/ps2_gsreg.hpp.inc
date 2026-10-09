//=========================================================================
//
// ps2_gsreg.hpp
//
//=========================================================================

#ifndef PS2_GSREG_HPP
#define PS2_GSREG_HPP

//=========================================================================

#include <xCore/x_files/x_types.hpp>

//=========================================================================
//  GS REGISTER SYSTEM
//=========================================================================
//
//  By wrapping REG calls with REG_Begin and REG_End, all the updates will
//  be shipped in one dma
//
//=========================================================================

void    gsreg_Begin ( void );
void    gsreg_Set   ( s32 Addr, u64 Data );
void    gsreg_End   ( void );

//=========================================================================
//  ALPHA BLENDING EQUATION Color = (A-B)*C>>7 + D
//=========================================================================

// These defines are used in the blend_mode macro
#define C_SRC       0
#define C_DST       1
#define C_ZERO      2
#define A_SRC       0
#define A_DST       1
#define A_FIX       2

#define ALPHA_BLEND_MODE(c1,c2,a,c3) (((c1)&0x3)|(((c2)&0x03)<<2)|(((a)&0x03)<<4)|(((c3)&0x03)<<6))

// Here are some predefined modes
#define ALPHA_BLEND_OFF     -1
#define ALPHA_BLEND_INTERP      ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST)   // (Src-Dst)*SrcA + Dst
#define ALPHA_BLEND_ADD         ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_FIX,C_DST)  // (Src-  0)*127  + Dst

// Here is the call to set the alpha mode
void    gsreg_SetAlphaBlend  ( s32 Mode, s32 FixedAlpha=0 );

//=========================================================================

// Used to set the frame buffer write mask
void    gsreg_SetFBMASK ( u32 Mask );

//=========================================================================

// Set mip equation
#define MIP_MAG_POINT                   0
#define MIP_MAG_BILINEAR                1
#define MIP_MIN_POINT                   0
#define MIP_MIN_BILINEAR                1
#define MIP_MIN_CLAMP_MIPMAP_POINT      2
#define MIP_MIN_CLAMP_MIPMAP_BILINEAR   3
#define MIP_MIN_INTERP_MIPMAP_POINT     4
#define MIP_MIN_INTERP_MIPMAP_BILINEAR  5

void    gsreg_SetMipEquation( xbool Manual, f32 MipK, s32 NMips, s32 MagBlend, s32 MinBlend );

//=========================================================================

#define ALPHA_TEST_NEVER            0   // Off
#define ALPHA_TEST_ALWAYS           1   // All pass
#define ALPHA_TEST_LESS             2   // A <  AlphaRef pass
#define ALPHA_TEST_LEQUAL           3   // A <= AlphaRef pass
#define ALPHA_TEST_EQUAL            4   // A == AlphaRef pass
#define ALPHA_TEST_GEQUAL           5   // A >= AlphaRef pass
#define ALPHA_TEST_GREATER          6   // A >  AlphaRef pass
#define ALPHA_TEST_NOTEQUAL         7   // A != AlphaRef pass

#define ALPHA_TEST_FAIL_KEEP        0   // Neither frame buffer nor Z buffer updated
#define ALPHA_TEST_FAIL_FB_ONLY     1   // Only frame buffer is updated
#define ALPHA_TEST_FAIL_ZB_ONLY     2   // Only z buffer is updated
#define ALPHA_TEST_FAIL_RGB_ONLY    3   // Only frame RGB buffer is updated

#define DEST_ALPHA_TEST_0           0   // Pixels, whose dest alpha==0 pass
#define DEST_ALPHA_TEST_1           1   // Pixels, whose dest alpha==1 pass
                                    
#define ZBUFFER_TEST_NEVER          0   // All pixels fail
#define ZBUFFER_TEST_ALWAYS         1   // All pixels pass
#define ZBUFFER_TEST_GEQUAL         2   // Z >= Pass
#define ZBUFFER_TEST_GREATER        3   // Z >  Pass
                                    
// Turns Z-buffering on and off
void    gsreg_SetZBuffer( xbool On );
void    gsreg_SetZBufferUpdate( xbool On );
void    gsreg_SetZBufferTest( s32 TestMethod );

// Turns alpha and z buffer testing on/off
// Can be used to setup special stencil, punch thru methods etc
void gsreg_SetAlphaAndZBufferTests ( xbool  AlphaTest,
                                     s32    AlphaTestMethod,
                                     u32    AlphaRef,
                                     s32    AlphaTestFail,
                                     xbool  DestAlphaTest,
                                     s32    DestAlphaTestMethod,
                                     xbool  ZBufferTest,
                                     s32    ZBufferTestMethod ) ;

//=========================================================================

// Sets alpha corretion (FBA) value. 
// Can be used for stencil methods together with dest alpha testing
// RGBA32 Mode: A = As | (FBA << 8) before writing to frame buffer
// RGBA16 Mode: A = As | (FBA & 0x01) before writing to frame buffer
void gsreg_SetAlphaCorrection ( xbool On ) ;

//=========================================================================

// Turns clamping on/off for both S and T (FALSE = REPEAT, TRUE = CLAMP)
void    gsreg_SetClamping( xbool On );

// Turns clamping on/off individually for S and T (FALSE = REPEAT, TRUE = CLAMP)
void    gsreg_SetClamping( xbool S, xbool T ) ;

//=========================================================================

// Sets scissor region
void    gsreg_SetScissor( s32 X0, s32 Y0, s32 X1, s32 Y1 );

//=========================================================================

// Fog settings (use gif flag GIF_FLAG_FOG to turn it on when you draw primitives)

// Set color and percentage (alpha = percentage, 0=all fog, 255=no fog)
void    gsreg_SetFogColor( u8 R, u8 G, u8 B, u8 A ) ;

//=========================================================================


#endif
//=========================================================================
