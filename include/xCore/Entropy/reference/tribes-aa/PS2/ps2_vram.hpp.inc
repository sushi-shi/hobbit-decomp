//=========================================================================
//
// PS2_VRAM.H
//
//=========================================================================

#ifndef PS2_VRAM_HPP
#define PS2_VRAM_HPP

#include <xCore/Entropy/e_View.hpp>
#include <xCore/x_files/x_bitmap.hpp>

//=========================================================================

void vram_Sync              ( void );
void vram_SetMipEquation    ( f32 MipK, xbool Manual=FALSE );

s32  vram_AllocateBank      ( s32 N8kPixelPages, s32 N8ClutPages );

void vram_Flush             ( s32 Bank );
void vram_Activate          ( s32 VRAMID, s32 Bank );
void vram_Activate          ( const xbitmap& BMP, s32 Bank );

void vram_ActivateMips      ( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD, s32 Bank=0 );
void vram_ComputeMipRange   ( f32 MinZ, f32 MaxZ, f32 MipK, f32& MinLOD, f32& MaxLOD );

void vram_ComputeMipRange   ( const bbox&   BBox, 
                              const view&   View,
                              f32           WorldPixelSize, 
                              f32&          MinLOD,
                              f32&          MaxLOD );

f32  vram_ComputeMipK       ( f32 WorldPixelSize, const view &View );
f32  vram_Log2              ( f32 x );

s32  vram_RegisterClut      ( byte* pClut, s32 NColors );
void vram_UnRegisterClut    ( s32 Handle );
void vram_ActivateClut      ( s32 Handle, s32 Bank=0 );
void vram_LoadClut          ( s32 Handle, s32 Bank=0 );
s32  vram_GetClutBaseAddr   ( s32 Handle );
s32  vram_GetClutHandle     ( const xbitmap& BMP );
xbool vram_IsClutActive     ( s32 Handle ) ;


xbool vram_IsActive         ( const xbitmap& BMP );
xbool vram_IsActive         ( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD );

s32 vram_GetUploadSize      ( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD );

//=========================================================================
// VRAM REGISTERS
//=========================================================================

struct vram_registers
{
    sceGsTex0       Tex0;
    u64             Tex0Addr;
    sceGsTex1       Tex1;
    u64             Tex1Addr;
    sceGsMiptbp1    Mip1;
    u64             Mip1Addr;
    sceGsMiptbp2    Mip2;
    u64             Mip2Addr;
};

void vram_GetRegisters( vram_registers& Reg, const xbitmap& BMP );

/*
//=========================================================================

void VRAM_Init              ( void );

void VRAM_Kill              ( void );

s32  VRAM_Register          ( x_bitmap& BMP );

void VRAM_UnRegister        ( x_bitmap& BMP );

void VRAM_UnRegister        ( s32 VRAMID );



xbool VRAM_IsActive         ( x_bitmap& BMP );

s32  VRAM_GetNextID         ( s32 VRAMID );


void VRAM_PrintStats        ( void );

void VRAM_Sync              ( void );

void VRAM_SanityCheck       ( void );
//=========================================================================
// Bank management
//=========================================================================


//=========================================================================
// Mipmap control
//=========================================================================


void VRAM_ActivateMips      ( const x_bitmap& BMP, f32 MinLOD, f32 MaxLOD, s32 Bank=0 );

void VRAM_ActivateMips      ( s32 VRAMID, f32 MinLOD, f32 MaxLOD, s32 Bank=0 );

void VRAM_ComputeMipRange   ( f32 MinZ, f32 MaxZ, f32 MipK, f32& MinLOD, f32& MaxLOD );

f32  VRAM_ComputeMipK       ( f32 WorldPixelSize );

f32  VRAM_Log2              ( f32 x );

//=========================================================================
// Additional clut management
//=========================================================================


//=========================================================================
// NOT COMPLETED
//=========================================================================


void VRAM_Display           ( void );
//=========================================================================
*/
#endif
