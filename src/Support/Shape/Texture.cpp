#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include <Support/Shape/Shape.hpp>
#include <Support/Shape/Texture.hpp>
#include <Support/Shape/Util.hpp>


//==============================================================================
// Defines
//==============================================================================

#define PS2_MAX_TEXTURE_SIZE   (1020*1024)         // almost 1 meg
//#define PS2_MAX_TEXTURE_SIZE   (1024*1024)         // almost 1 meg
#define PC_MAX_TEXTURE_SIZE    (1024*1024*16)      // 16 meg


//==============================================================================
// Texture reference class
//==============================================================================

shape_texture_ref::shape_texture_ref()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(shape_texture_ref) & 15) == 0) ;

    x_strcpy(m_Name, "NULL") ;
}

//==============================================================================

const char *shape_texture_ref::GetName()
{
    return m_Name ;
}

//==============================================================================

void shape_texture_ref::SetName(const char *Name)
{
    ASSERT(x_strlen(Name) < (s32)sizeof(m_Name)) ;

    x_strcpy(m_Name, Name) ;
}

//==============================================================================

void shape_texture_ref::ReadWrite(shape_bin_file &File)
{
    File.ReadWrite(m_Name) ;
}

//==============================================================================


//==============================================================================
// Texture class
//==============================================================================

// Constructor
texture::texture()
{
    // Set defaults
    m_Name[0]        = 0 ;
    m_ReferenceCount = 0 ;
}

//==============================================================================

// Destructor
texture::~texture()
{
    // Free memory used
    Kill() ;
}

//==============================================================================

// Removes texture from VRAM and frees up the memory
void texture::Kill()
{
    // Already in VRAM?
    if (IsValid())
        vram_Unregister(m_Bitmap) ;

    // Free current bitmap
    m_Bitmap.Kill() ;
}

//==============================================================================

s32 texture_log_total = 0;
xstring* x_texture_log = NULL;

// Loads texture
xbool texture::Load( const char*                Filename, 
                     const shape_load_settings& LoadSettings,
                     material::texture_type     Type)
{
#if !defined(TARGET_LINUX)
    char    DRIVE   [X_MAX_DRIVE];
    char    DIR     [X_MAX_DIR];
    char    FNAME   [X_MAX_FNAME];
    char    EXT     [X_MAX_EXT];
    char    XBMPPath[X_MAX_PATH];
    xbool   Success         = FALSE ;
    s32     MaxTextureSize  = 0 ;

    // Make sure file is there before trying anything!
    //if (!UtilDoesFileExist(Filename))
        //return FALSE ;

    // Remove if already there
    Kill() ;

    //-------------------------------------------------------------------------
    // Check for loading .xbm?
    //-------------------------------------------------------------------------
    if (LoadSettings.CheckForXBMP)
    {
        // Make .xbm filename
        x_splitpath( m_Name,DRIVE,DIR,FNAME,EXT);

        // If it asserts here, you must call the global function
        // "shape::SetDataOutputPaths(const char *SHP_Path, const char *XBMP_Path)"
        // when your app initializes!
        ASSERT(shape::Get_XBMP_Path()) ;

        // Build the xbmp path
        if (shape::Get_XBMP_Path())
            x_makepath( XBMPPath, NULL, shape::Get_XBMP_Path(), FNAME, ".xbmp");
        else
            x_makepath( XBMPPath, DRIVE, DIR, FNAME, ".xbmp");

        // Display texture info
        shape::printf("   checking for %s...",XBMPPath);
        
        // Try load it
        Success = m_Bitmap.Load(XBMPPath) ;
        if (Success)
            shape::printf("FOUND!\n") ;
        else
            shape::printf("not found\n") ;
    }


    //-------------------------------------------------------------------------
    // Lookup target info
    //-------------------------------------------------------------------------
    switch(LoadSettings.Target)
    {
        default:
            ASSERT(0) ; // version not supported!
            break ;

        case shape_bin_file_class::PS2:
            MaxTextureSize  = PS2_MAX_TEXTURE_SIZE ;
            break ;

        case shape_bin_file_class::PC:
            MaxTextureSize  = PC_MAX_TEXTURE_SIZE ;
            break ;
    }

    //-------------------------------------------------------------------------
    // Try regular load
    //-------------------------------------------------------------------------
    if ((!Success) && (LoadSettings.SearchTexturePaths))
    {
        // Load...
        shape::printf("   checking for %s...", Filename);
        switch(LoadSettings.Target)
        {
            default:
                ASSERT(0) ; // version not supported!
                break ;

            case shape_bin_file_class::PS2:
                Success         = auxbmp_Load(m_Bitmap, Filename) ;
                break ;

            case shape_bin_file_class::PC:
                Success         = auxbmp_Load(m_Bitmap, Filename) ;
                break ;
        }

        // Success?
        if (Success)
        {
            shape::printf("FOUND!\n") ;

            //-----------------------------------------------------------------
            // Convert grey scale textures into alpha version for alpha textures
            //-----------------------------------------------------------------
            if (Type == material::TEXTURE_TYPE_ALPHA)
            {
                // Make sure it's palettized with alpha bits
                //if (m_Bitmap.GetBPP() == 4) // bits per pixel==4?
                    //m_Bitmap.ConvertFormat(xbitmap::FMT_P4_RGBA_8888) ;
                //else
                    //m_Bitmap.ConvertFormat(xbitmap::FMT_P8_RGBA_8888) ;

                // Convert all alpha textures to 4bit!
                m_Bitmap.ConvertFormat(xbitmap::FMT_32_RGBA_8888) ;   // to kick quantizer in!!!
                m_Bitmap.ConvertFormat(xbitmap::FMT_P4_RGBA_8888) ;

                // Finally - build alpha from color
                m_Bitmap.MakeIntensityAlpha() ;
            }

            //-----------------------------------------------------------------
            // Convert reflection maps to 8bit (for PS2)
            //-----------------------------------------------------------------
            if (Type == material::TEXTURE_TYPE_REFLECT)
                m_Bitmap.ConvertFormat(xbitmap::FMT_P8_RGBU_8888) ;

            //-----------------------------------------------------------------
            // Attempt to convert diffuse 8bit palettized to 4bit palettized to save some ram...
            //-----------------------------------------------------------------
            if (        ((Type == material::TEXTURE_TYPE_DIFFUSE) || (Type == material::TEXTURE_TYPE_REFLECT))
                    &&  (m_Bitmap.IsClutBased()) && (m_Bitmap.GetBPP() == 8))
            {
                // Count unique colors
                xcolor  Palette[16] ;
                s32     NColors=0 ;
                s32     x,y,i ;

                // Need to check all pixels
                // (we can't just check the palette since palette entries maybe the same,
                //  and some palette entries may not even be used)
                for (x = 0 ; (x < m_Bitmap.GetWidth()) && (NColors <=16) ; x++)
                {
                    for (y = 0 ; (y < m_Bitmap.GetHeight()) && (NColors <=16) ; y++)
                    {
                        // Get pixel color
                        xcolor Col = m_Bitmap.GetPixelColor(x,y) ;

                        // Check to see if color has already been added to palette
                        xbool Added=FALSE ;
                        for (i = 0 ; i < NColors ; i++)
                        {
                           // Already added?
                           if (Palette[i] == Col)
                           {
                               Added = TRUE ;
                               break ;
                           }
                        }

                        // Add to palette?
                        if (!Added)
                        {
                            // Only add if still within 16 colors
                            if (NColors < 16)
                                Palette[NColors] = Col ;

                            // Increase color count
                            NColors++ ;
                            if (NColors > 16)
                                break ;
                        }
                    }
                }

                // If there are 16 colors or less, then we can convert to 16colors - yipee!
                if (NColors <= 16)
                {
                    // Do alpha version?
                    if (m_Bitmap.HasAlphaBits())
                    {
                        m_Bitmap.ConvertFormat(xbitmap::FMT_32_RGBA_8888) ;   // to kick quantizer in!!!
                        m_Bitmap.ConvertFormat(xbitmap::FMT_P4_RGBA_8888) ;
                    }
                    else
                    {
                        m_Bitmap.ConvertFormat(xbitmap::FMT_32_RGBU_8888) ;   // to kick quantizer in!!!
                        m_Bitmap.ConvertFormat(xbitmap::FMT_P4_RGBU_8888) ;
                    }
                }
            }

            // Finally, put into native format
            switch(LoadSettings.Target)
            {
                default:
                    ASSERT(0) ; // version not supported!
                    break ;

                case shape_bin_file_class::PS2:
                    auxbmp_ConvertToPS2(m_Bitmap) ;
                    break ;

                case shape_bin_file_class::PC:
                    auxbmp_ConvertToD3D(m_Bitmap) ;
                    break ;
            }
        }
        else
            shape::printf("not found\n") ;
    }

    //-------------------------------------------------------------------------
    // Finally, build mips if needed and register with vram
    //-------------------------------------------------------------------------
    if (Success)
    {
        // Make mips if we need them and there are none (only for diffuse textures!)
        if ((LoadSettings.BuildMips) && (Type == material::TEXTURE_TYPE_DIFFUSE) && (m_Bitmap.GetNMips() == 0))
        {
            shape::printf("   building mips...\n") ;
            m_Bitmap.BuildMips();
        }
   
        // Only register with VRAM on compile machine
        if (LoadSettings.Target == shape_bin_file_class::GetDefaultTarget())
            Register(Type) ;

        shape::printf("\n") ;

        // Test- Write out tga
        //if (shape::Get_XBMP_Path())
        //{
            //x_makepath( XBMPPath, NULL, shape::Get_XBMP_Path(), FNAME, ".tga");
            //m_Bitmap.SaveMipsTGA(XBMPPath) ;
        //}

        #if 0//defined(TARGET_PS2_DEV) && defined(athyssen) 
        {
            s32 W = m_Bitmap.GetWidth();
            s32 H = m_Bitmap.GetHeight();
            s32 BPP = m_Bitmap.GetBPP();
            s32 Size = m_Bitmap.GetDataSize() + m_Bitmap.GetClutSize();
            if( x_texture_log == NULL )
                x_texture_log = new xstring;
            *x_texture_log += xfs("%4d  %4d  %2d  %8d  %s\n",W,H,BPP,Size,XBMPPath);
        }
        #endif
    }

    // Too big?
    s32 DataSize = m_Bitmap.GetDataSize() ;
    if ((Success) && (DataSize > MaxTextureSize))
    {
        shape::printf("FAILURE - It's too big to fit in VRAM! - make it smaller") ;
        //Success = FALSE ;
    }

    // Return TRUE if texture was loaded...
    return Success ;
#else
	return FALSE;
#endif
}

//==============================================================================

// Writes out xbmp
void texture::SaveXBMP(void) const
{
    char    DRIVE   [X_MAX_DRIVE];
    char    DIR     [X_MAX_DIR];
    char    FNAME   [X_MAX_FNAME];
    char    EXT     [X_MAX_EXT];
    char    XBMPPath[X_MAX_PATH];

    // Make .xbm filename
    x_splitpath( m_Name,DRIVE,DIR,FNAME,EXT);

    // If it asserts here, you must call the global function
    // "shape::SetDataOutputPaths(const char *SHP_Path, const char *XBMP_Path)"
    // when your app initializes!
    ASSERT(shape::Get_XBMP_Path()) ;

    // Build the xbmp path
    if (shape::Get_XBMP_Path())
        x_makepath( XBMPPath, NULL, shape::Get_XBMP_Path(), FNAME, ".xbmp");
    else
        x_makepath( XBMPPath, DRIVE, DIR, FNAME, ".xbmp");

    // Save
    m_Bitmap.Save( XBMPPath );

    // Print info
    x_makepath( XBMPPath, NULL, NULL, FNAME, EXT);
    shape::printf("      writing XBMP from %30s (%.4d x %.4d x %.4d  %.8d Bytes  %.5dK)\n", 
                   XBMPPath, 
                   m_Bitmap.GetWidth(), 
                   m_Bitmap.GetHeight(), 
                   m_Bitmap.GetBPP(),
                   m_Bitmap.GetDataSize() + m_Bitmap.GetClutSize(),
                   (m_Bitmap.GetDataSize() + m_Bitmap.GetClutSize()+1023)/1024 ) ;


}

//==============================================================================

// Registers texture with vram
void texture::Register( material::texture_type Type )
{
    (void)Type ;

// On the PC for now, have to call special case vram register until xbitmap can support them
#ifdef TARGET_PC
    if ((Type == material::TEXTURE_TYPE_BUMP) && (!shape::s_UseDot3))
    {
        vram_RegisterDuDv(m_Bitmap) ;
        return ;
    }
#endif        

    vram_Register(m_Bitmap) ;
}

//==============================================================================


// Loads texture into VRAM
void texture::Activate()
{
    // Must be valid
    ASSERT(IsValid()) ;

    // Activate
    vram_Activate(m_Bitmap) ;

    // There's trouble if it didn't fit into VRAM
    ASSERTS( IsActive(), xfs( "%s failed to fit in VRAM\n", m_Name ) ) ;
}

//==============================================================================

// Puts specific mip textures into VRAM
void texture::ActivateMips(f32 MinLOD, f32 MaxLOD)
{
    // Must be valid
    ASSERT(IsValid()) ;

    #ifdef TARGET_PS2

        // Activate
        vram_ActivateMips(m_Bitmap, MinLOD, MaxLOD) ;
    
    #else

        // Activate
        vram_Activate(m_Bitmap) ;

    #endif

    // There's trouble if it didn't fit into VRAM
    ASSERTS( IsActive(MinLOD, MaxLOD), xfs( "%s failed to fit in VRAM\n", m_Name ) ) ;
}

//==============================================================================

// Returns TRUE if in VRAM
xbool texture::IsActive        () 
{
    return vram_IsActive(m_Bitmap) ;    
}

//==============================================================================

// Returns TRUE if all mips are in VRAM
xbool texture::IsActive        (f32 MinLOD, f32 MaxLOD) 
{
    #ifdef TARGET_PS2
        return vram_IsActive(m_Bitmap, MinLOD, MaxLOD);
    #endif

    #ifdef TARGET_PC
        return vram_IsActive(m_Bitmap) ;
    #endif

}

//==============================================================================

// Returns TRUE if texture has been loaded successfully
s32 texture::IsValid()
{
    return (m_Bitmap.GetVRAMID() != 0) ;
}

//==============================================================================

// Draws texture on screen (used for debugging)
void texture::Draw(s32 x, s32 y)
{
    // Draw background bitmap
    if (IsValid())
    {
        // Draw quad
        f32 fw = (f32)m_Bitmap.GetWidth() ;
        f32 fh = (f32)m_Bitmap.GetHeight() ;
        f32 fx = (f32)x ;
        f32 fy = (f32)y ;

        draw_Begin(DRAW_SPRITES, DRAW_2D | DRAW_TEXTURED | DRAW_NO_ZBUFFER | DRAW_USE_ALPHA) ;

        // Load into vram
        draw_SetTexture ( m_Bitmap ) ;

        draw_SpriteUV( vector3(fx,fy,0),    // Pos
                       vector2(fw,fh),      // Size
                       vector2(0,0),        // TL uv
                       vector2(1,1),        // BR uv
                       XCOLOR_WHITE ) ;     // Color
        draw_End();
    }
}

//==============================================================================

// Bump map conversion utils
void texture::ConvertHeightMapToDuDvBumpMap( void )
{
}
    
//==============================================================================
    
void texture::ConvertHeightMapToDot3BumpMap( void )
{
}

//==============================================================================
