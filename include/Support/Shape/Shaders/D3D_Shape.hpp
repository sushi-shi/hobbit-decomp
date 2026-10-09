#ifndef __D3D_SHAPE_HPP__
#define __D3D_SHAPE_HPP__


//==============================================================================
// Includes
//==============================================================================


#ifdef __cplusplus

#include <xCore/Entropy/Entropy.hpp>

//==============================================================================
// C++ STRUCTURES
//==============================================================================

// Vertex description
#define D3DFVF_VERT (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0))

struct d3d_vert
{
    vector3     vPos ;			// Local pos
    vector3     vNormal ;		// Local normal
    vector2     TexCoord ;		// Diffuse/bump/damage UV's
} ;

// Vertex shader constants
struct d3d_vs_consts
{
    f32     Consts[4] ;         // c0
    matrix4 L2C ;               // c1,c2,c3,c4
    matrix4 Ref ;               // c5,c6,c7,c8
    vector4 LightDir ;          // c9
    vector4 LightDiffuseCol ;   // c10
    vector4 LightAmbientCol ;   // c11
    vector4 FogCol ;            // c12
} ;

// Pixel shader constants
struct d3d_ps_consts
{
    vector4 LightDir ;          // c0
    vector4 LightDiffuseCol ;   // c1
    vector4 LightAmbientCol ;   // c2
    vector4 SpecDir ;           // c3
    vector4 SpecCol ;           // c4
} ;

#endif

//==============================================================================
// SHADER CONSTANTS
//==============================================================================

#ifndef __cplusplus

// Vert inputs
#define VS_VERT_POS            v0
#define VS_VERT_NORMAL         v1
#define VS_VERT_UV             v2

// Vertex shader constants (match above structure)
#define VS_ZERO                c0.x
#define VS_ONE                 c0.y
#define VS_MINUS_ONE           c0.z
#define VS_TWO                 c0.w

#define VS_L2C_MATRIX          c1
#define VS_REF_MATRIX          c5
#define VS_LIGHT_DIR           c9
#define VS_LIGHT_DIFFUSE_COL   c10
#define VS_LIGHT_AMBIENT_COL   c11
#define VS_FOG_COL             c12

// Pixel shader constants (match above structure)
#define PS_LIGHT_DIR           c0
#define PS_LIGHT_DIFFUSE_COL   c1
#define PS_LIGHT_AMBIENT_COL   c2
#define PS_SPEC_DIR            c3
#define PS_SPEC_COL            c4


#endif



#endif  // #define __D3D_SHAPE_HPP__

