//==============================================================================
//
//  Textures.cpp
//
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>
#include "textures.hpp"
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>


//==============================================================================
//  STATIC
//==============================================================================

static char SrcDir[] = "P:/Art/Delivery/Shapes/Particles/";

#ifdef TARGET_PS2
static char   XDir[] = "data/projectiles/ps2/";
#endif

#ifdef TARGET_PC
static char   XDir[] = "data/projectiles/pc/";
#endif

//==============================================================================
//  FUNCTIONS
//==============================================================================


s32 LoadProjTexture     ( xbitmap& Bmp, char* pFilename )
{
    /*
    char    tName[128];

    // try to load XBMP first
    x_sprintf( tName, "%s%s.xbmp", XDir, pFilename );

    xbool Result = auxbmp_LoadNative( Bmp, tName );

    if ( !Result )
    {
        // failed, so try the src directory
        x_sprintf( tName, "%s%s.bmp", SrcDir, pFilename );

        xbool Result = auxbmp_LoadNative( Bmp, tName );

        if ( !Result )
        {
            x_DebugMsg("Could not load projectile texture %s\n",pFilename);
            return -1;
        }
        else
        {
            // loaded OK, save the XBMP
            x_sprintf( tName, "%s%s.xbmp", XDir, pFilename );
            Bmp.Save( tName );
        }
    }

    vram_Register( Bmp );

    s32 t = Bmp.GetVRAMID();

    return t;
    */
    return( 1 );
}

