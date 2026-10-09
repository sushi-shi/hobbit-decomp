#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/ShapeBinFile.hpp>
#include <Support/Shape/Material.hpp>


//==============================================================================
// Forward declarations
//==============================================================================

struct shape_load_settings ;

#define TEXTURE_MAX_FNAME 160
//==============================================================================
// Classes
//==============================================================================


// Texture reference class
class shape_texture_ref : public shape_bin_file_class
{
// Data
private:
    char            m_Name[TEXTURE_MAX_FNAME] ;       // Name
    byte            PADDING[12] ;               // Align to 16 bytes for shape_bin_file_class

// Functions
public:
    // Defualt contructor
    shape_texture_ref() ;
    
    // Inline functions
    const char *GetName() ;
    void        SetName(const char *Name) ;

    // File IO
    void ReadWrite(shape_bin_file &File) ;

} ;


//==============================================================================


// The main texture class
class texture
{
// Structures
private:


// Data
private:
    char        m_Name[TEXTURE_MAX_FNAME] ;   // Filename
    xbitmap     m_Bitmap ;              // Engine bitmap
    s32         m_ReferenceCount ;      // Number of times texture is used

// Functions
public:
    
    // Constructor/destructor
    texture() ;
    ~texture() ;

    // Remove from VRAM and free memory used by bitmap
    void    Kill() ;

    // Takes a 32 bit bitmap and converts it down to 8-bit, along with the alpha in the palette
    void    PalletizeAlpha  () ;

    // Loads texture
    s32     Load            (const char*                    Filename,
                             const shape_load_settings&     LoadSettings,
                             material::texture_type         Type         = material::TEXTURE_TYPE_DIFFUSE) ;

    // Saves texture as xbmp
    void    SaveXBMP        (void) const ;

    // Registers texture with vram
    void    Register        ( material::texture_type Type ) ;

    // Puts texture into VRAM
    void    Activate        ( void ) ;

    // Puts specific mip textures into VRAM
    void    ActivateMips    (f32 MinLOD, f32 MaxLOD) ;

    // Returns TRUE if in VRAM
    xbool   IsActive        () ;
    xbool   IsActive        (f32 MinLOD, f32 MaxLOD) ;

    // Returns TRUE if texture is loaded successfully
    s32     IsValid         () ;

    // Draws texture at screen co-ords
    void    Draw            (s32 x, s32 y) ;

    // Bump map conversion utils
    void    ConvertHeightMapToDuDvBumpMap ( void ) ;
    void    ConvertHeightMapToDot3BumpMap ( void ) ;

    // Inline functions

    // Returns memory used (doesn't take into account the bitmap right now...)
    s32     MemoryUsed      ()          { return sizeof(texture) ; }

    // Set texture name
    void    SetName         (const char *Name)
    {
        ASSERT(x_strlen(Name) < (s32)sizeof(m_Name));
        x_strcpy(m_Name, Name) ;
    }

    // Get texture name
    const char *GetName         ( void ) const 
    {
        return m_Name ;
    }
    
    // Returns TRUE if texture contains an alpha channel
    xbool   HasAlpha        ( void ) const
    {
		return m_Bitmap.HasAlphaBits() ;
    }

    // Returns TRUE if texture uses a palette
    xbool   HasPalette      ( void ) const
    {
        return (m_Bitmap.GetClutData() != NULL) ;
    }

    // Returns pixel width of texture
    s32     GetWidth        ( void ) const
    {
        return m_Bitmap.GetWidth() ;
    }
    
    // Returns pixel height of texture
    s32     GetHeight       ( void ) const
    {
        return m_Bitmap.GetHeight() ;
    }

    // Returns access to xbitmap
    xbitmap &GetXBitmap()
    {
        return m_Bitmap ;
    }

    // Reference count functions
    
    // Increments reference count
    void    IncReferenceCount( void )
    {
        m_ReferenceCount++ ;
    }

    // Decrease reference count
    void    DecReferenceCount( void )
    {
        m_ReferenceCount++ ;
    }

    // Returns reference count
    s32     GetReferenceCount ( void )
    {
        return m_ReferenceCount ;
    }
} ;

//==============================================================================


#endif  //#ifndef __TEXTURE_HPP__
