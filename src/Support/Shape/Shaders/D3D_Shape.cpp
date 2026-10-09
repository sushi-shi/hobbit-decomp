//==============================================================================
// INCLUDES
//==============================================================================

#include <Support/Shape/Shaders/D3D_Shape.hpp>
#include "..\Shape.hpp"


//==============================================================================
// SHADERS
//==============================================================================

#include "Dif.vsa"
#include "Dif.psa"

#include "TexDif.vsa"
#include "TexDif.psa"

#include "TexDifRef.vsa"
#include "TexDifRef.psa"

#include "TexDifRefBump.vsa"
#include "TexDifRefBump.psa"

#include "TexDifDot3.vsa"
#include "TexDifDot3.psa"


//==============================================================================
// DATA
//==============================================================================

// Setup the vertex declaration
DWORD dwDecl[] =
{
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),     // v0 = Position
    D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),     // v1 = Normal
    D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),     // v2 = Texcoords
    D3DVSD_END()
};


//==============================================================================
// FUNCTIONS
//==============================================================================

void shape::Init( void )
{
    ASSERT(s_Initialized == FALSE) ;

    /* DEDICATED_SERVER

    // Register vertex and pixel shaders

    // Register dif shaders
    s_hDifVertexShader              = d3deng_RegisterVertexShader("Dif", dwDecl, dwDifVertexShader, 0, D3DFVF_VERT) ;
    s_hDifPixelShader               = d3deng_RegisterPixelShader ("Dif", dwDifPixelShader ) ;

    // Register tex-dif shaders
    s_hTexDifVertexShader           = d3deng_RegisterVertexShader("TexDif", dwDecl, dwTexDifVertexShader, 0, D3DFVF_VERT) ;
    s_hTexDifPixelShader            = d3deng_RegisterPixelShader ("TexDif", dwTexDifPixelShader ) ;

    // Register tex-dif-ref shaders
    s_hTexDifRefVertexShader        = d3deng_RegisterVertexShader("TexDifRef", dwDecl, dwTexDifRefVertexShader, 0, D3DFVF_VERT) ;
    s_hTexDifRefPixelShader         = d3deng_RegisterPixelShader ("TexDifRef", dwTexDifRefPixelShader ) ;

    // Register tex-dif-ref-bump shaders
    s_hTexDifRefBumpVertexShader    = d3deng_RegisterVertexShader("TexDifRefBump", dwDecl, dwTexDifRefBumpVertexShader, 0, D3DFVF_VERT) ;
    s_hTexDifRefBumpPixelShader     = d3deng_RegisterPixelShader ("TexDifRefBump", dwTexDifRefBumpPixelShader ) ;

    // Register tex-dif-dot3 shaders
    s_hTexDifDot3VertexShader       = d3deng_RegisterVertexShader("TexDifDot3", dwDecl, dwTexDifDot3VertexShader, 0, D3DFVF_VERT) ;
    s_hTexDifDot3PixelShader        = d3deng_RegisterPixelShader ("TexDifDot3", dwTexDifDot3PixelShader ) ;

    DEDICATED_SERVER */

    // Done!
    s_Initialized = TRUE ;
}

//==============================================================================

void shape::Kill( void )
{
    // Engine will take care of freeing shaders

    // Flag done...
    s_Initialized = FALSE;
}

//==============================================================================

