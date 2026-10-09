#ifndef BLD_RENDER_HPP
#define BLD_RENDER_HPP

//=========================================================================
// INCLUDES
//=========================================================================

#include "building.hpp"
#include <xCore/x_files/x_bitmap.hpp>

//=========================================================================
// FUNCTIONS
//=========================================================================

void BLDRD_Initialize       ( void );

void BLDRD_Begin            ( void );
void BLDRD_End              ( void );
void BLDRD_Delay            ( void );

void BLDRD_RenderDList      ( const building::dlist& DList, xbool DoClip );

void BLDRD_UpdaloadMatrices (const matrix4& L2C, 
                             const matrix4& C2S, 
                             const matrix4& L2S,
                             const matrix4& L2W,
                             const vector3& WorldEyePos,
                             const vector3& LocalEyePos,
                             const vector4& FogMul,
                             const vector4& FogAdd );

void BLDRD_UpdaloadBase     ( const xbitmap& Bitmap, f32 MinLOD, f32 MaxLOD );
void BLDRD_UpdaloadLightMap ( const xbitmap& Bitmap, s32* pClutHandle );
void BLDRD_UpdaloadFog      ( const xbitmap& Bitmap );
void BLDRD_UpdaloadPassMask ( u16 PassMask );

//=========================================================================
// END
//=========================================================================
#endif