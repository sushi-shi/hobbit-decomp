// Provisional Tribes Entropy interface; original unlabelled API, not a PC address claim.
//==============================================================================
//  
//  e_VRAM.hpp
//
//==============================================================================

#ifndef E_VRAM_HPP
#define E_VRAM_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_bitmap.hpp>

//==============================================================================
//  FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
//  Public functions
//------------------------------------------------------------------------------

s32         vram_Register       ( const xbitmap& Bitmap  );
void        vram_Unregister     ( const xbitmap& Bitmap  );
void        vram_Unregister     (       s32      VRAM_ID );

void        vram_Activate       ( const xbitmap& Bitmap  );
void        vram_Activate       (       s32      VRAM_ID );
void        vram_Activate       ( void );

xbool       vram_IsActive       ( const xbitmap& Bitmap );

void        vram_Flush          ( void );

//------------------------------------------------------------------------------
//  Debugging functions
//------------------------------------------------------------------------------

s32         vram_GetNRegistered ( void );
s32         vram_GetRegistered  ( s32 ID );

void        vram_PrintStats     ( void );
void        vram_SanityCheck    ( void );

//------------------------------------------------------------------------------
//  Private functions
//------------------------------------------------------------------------------

void        vram_Init           ( void );
void        vram_Kill           ( void );

//------------------------------------------------------------------------------
//  PC specific functions
//------------------------------------------------------------------------------

#ifdef TARGET_PC

#include <xCore/Entropy/D3DEngine/d3deng_private.hpp>

s32                 vram_LoadTexture    ( const char*    pFileName );

IDirect3DTexture8*  vram_GetSurface     ( const xbitmap& Bitmap  );
IDirect3DTexture8*  vram_GetSurface     (       s32      VRAM_ID );

// Register bitmap as a dudv bump map (bitmap should be grey scale height)
// Until xbitmap supports such formats, the conversion happens when creating the D3D texture 
s32                 vram_RegisterDuDv   ( const xbitmap& Bitmap ) ;

#endif

//==============================================================================
#endif // E_VRAM_HPP
//==============================================================================
