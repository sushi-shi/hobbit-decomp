#include "BldRender.hpp"

//=========================================================================

void DebugRender( building& Render )
{
}

//=========================================================================

void BLDRD_Initialize( void )
{

}

//=========================================================================

void BLDRD_Begin( void )
{
}

//=========================================================================

void BLDRD_End( void )
{
}

//=========================================================================

void BLDRD_Delay(void)
{
}

//=========================================================================

void BLDRD_RenderDList( const building::dlist& DList, xbool DoClip )
{
}

//=========================================================================

//void BLDRD_UpdaloadMatrices ( const matrix4& L2W, const matrix4& W2V, const matrix4& V2C )
void BLDRD_UpdaloadMatrices( const matrix4& L2W, const matrix4& W2V, const matrix4& V2C,
                             const matrix4& L2W2,
                             const vector3& WorldEyePos,
                             const vector3& LocalEyePos,
                             const vector4& FogMul,
                             const vector4& FogAdd)

{
}

//=========================================================================

void BLDRD_UpdaloadBase( const xbitmap& Bitmap, f32 MinLOD, f32 MaxLOD )
{
}

//=========================================================================

void BLDRD_UpdaloadLightMap ( const xbitmap& Bitmap, s32* pClutHandle )
{
}

//=========================================================================

void BLDRD_UpdaloadFog( const xbitmap& Bitmap )
{
}

