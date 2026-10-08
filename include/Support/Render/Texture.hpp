// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-render.md.
#ifndef TEXTURE_HPP
#define TEXTURE_HPP

//=========================================================================
// INCLUDES
//=========================================================================

#include <Support/ResourceMgr/ResourceMgr.hpp>
#include <xCore/x_files/x_files.hpp>
#include <xCore/x_files/x_bitmap.hpp>

#ifdef TARGET_XBOX
#include <xCore/Entropy/Xbox/TextureMgr.hpp>
#endif

//=========================================================================
// CLASS
//=========================================================================

class texture
{
public:

    texture( void );

    static void GetStats( s32* pNumTextureLoaded, s32* pTextureMemorySize );

    typedef rhandle<texture> handle;

    xbitmap m_Bitmap;

    friend struct texture_loader;
};

//=========================================================================

class cubemap
{
public:
    cubemap( void );

    enum sides
    {
        TOP = 0,
        BOTTOM,
        FRONT,
        BACK,
        LEFT,
        RIGHT
    };

    typedef rhandle<cubemap> handle;

#ifdef TARGET_XBOX
    texture_factory::cube_handle m_hTexture;
#else
    void* m_hTexture;
#endif

    xbitmap m_Bitmap[6];

    friend struct cubemap_loader;
};

//=========================================================================
// END
//=========================================================================
#endif

