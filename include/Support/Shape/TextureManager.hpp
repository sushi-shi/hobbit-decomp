#ifndef __TEXTURE_MANAGER_H__
#define __TEXTURE_MANAGER_H__

  
#include <Support/Shape/Texture.hpp>



//==============================================================================
// Forward references
//==============================================================================

struct shape_load_settings ;


//==============================================================================
// Structures
//==============================================================================

struct path
{
    char    Name[TEXTURE_MAX_FNAME] ;      // Path name
} ;


//==============================================================================
// Classes
//==============================================================================

// A simple class to consolidate texture loading
class texture_manager
{
// Defines
public:
    enum texture_manager_defines
    {
        MAX_TEXTURES     = 1024,     // Currently set in stone... could be changed
        MAX_SEARCH_PATHS = 10,       // Max # of search paths to use for textures
        MAX_MISSING_TEXTURES = 20,   // MAx # of textures that are not found
    } ;


// Data
private:
    texture     *m_Textures[MAX_TEXTURES] ; // List of textures
    s32          m_nTextures ;               // # of textures

    path        m_Paths[MAX_SEARCH_PATHS] ; // List of search paths
    s32         m_nPaths ;                  // Total search paths

    char        m_MissingTextures[MAX_MISSING_TEXTURES][TEXTURE_MAX_FNAME] ;
    s32         m_nMissingTextures ;

// Functions
public:
	texture_manager() ;
	~texture_manager() ;

    // Add texture to manager and returns index
    s32 AddTexture(texture* Texture) ;

    // Adds and loads a single texture - returns -1 if the texture is not found
    s32 AddAndLoadTexture(const char*                   Name, 
                          const shape_load_settings&    LoadSettings,
                          material::texture_type        Type = material::TEXTURE_TYPE_DIFFUSE) ;

    // Adds and loads texture(s) - returns the total number that were successfully loaded and the start index
    // (start index is set to -1 if no textures are loaded)
    // (a max count of zero means keep searching if an animated texture is found)
    s32 AddAndLoadTextures(const char*                      Name, 
                           const shape_load_settings&       LoadSettings,
                           material::texture_type           Type,
                           s32&                             StartIndex,
                           s32                              MaxCount = 0) ;

    // Returns number of textures
    s32     GetTextureCount()       { return m_nTextures ; }

    // Returns texture, given the index
    texture &GetTexture(s32 Index) ;

    // Returns texture if found, or NULL
    texture *GetTextureByName(const char* Name) ;

    // Returns texture index if found, or -1
    s32  GetTextureIndexByName(const char* Name) ;

    // Clears missing texture list
    void    ClearMissingTextures() ;

    // Adds to missing texture list
    void    AddMissingTexture(const char* Name) ;

    // Returns number of missing textures
    s32     GetMissingTextureCount()    { return m_nMissingTextures ; }

    // Returns name of missing textures
    const char* GetMissingTexture(s32 Index) ;

    // Sets current search path (defaults to null string)
    void    SetCurrentSearchPath(const char *Path) ;

    // Adds new search path to manager
    void    AddSearchPath(const char *Path) ;

    // Clears all texture paths
    void    DeleteAllSearchPaths() ;

    // Removes all texture from memory
    void    DeleteAllTextures() ;

    // Saves all textures as xbmps
    void    SaveXBMPs( void ) ;

} ;


#endif	//#define __TEXTURE_MANAGER_H__
