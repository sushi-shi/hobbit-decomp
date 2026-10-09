// TextureManager.cpp
//
	
#include <Support/Shape/TextureManager.hpp>
#include <Support/Shape/ShapeManager.hpp>
#include <Support/Shape/Shape.hpp>
#include "Tokenizer/Tokenizer.hpp"




//==============================================================================
//
// texture_manager class - a simple class to consolidate texture loads
//
//==============================================================================


//==============================================================================
// Functions
//==============================================================================

// Constructor
texture_manager::texture_manager()
{
    // Clear texture count
    m_nTextures         = 0 ;

    // Set current path to NULL
    m_Paths[0].Name[0] = 0 ;
    m_nPaths = 1 ;

    // Clear missing texture count
    m_nMissingTextures  = 0 ;
}

//==============================================================================

// Destructor
texture_manager::~texture_manager()
{
    DeleteAllTextures() ;
}

//==============================================================================

// Add texture to manager and returns index
s32 texture_manager::AddTexture(texture* Texture)
{
    s32 Index = m_nTextures ;   // Keep index

    // Ran out of space? Then make MAX_TEXTURES bigger
    ASSERT(Index < MAX_TEXTURES) ;

    // Add to list
    m_Textures[m_nTextures++] = Texture ;

    // Return index
    return Index ;
}

//==============================================================================

// Adds and loads a single texture - returns -1 if the texture is not found
s32 texture_manager::AddAndLoadTexture(const char*                  Name,
                                       const shape_load_settings&   LoadSettings,
                                       material::texture_type       Type)
{
    s32 i ;

    // Split up filename
    char DRIVE  [X_MAX_DRIVE];
    char DIR    [X_MAX_DIR];
    char FNAME  [X_MAX_FNAME];
    char EXT    [X_MAX_EXT];
    char PATH   [X_MAX_PATH];
    x_splitpath( Name,DRIVE,DIR,FNAME,EXT);

    // Only certain extensions can be loaded otherwise the bitmap load will assert!
    if (    (x_stricmp(EXT, ".tga") != 0) && 
            (x_stricmp(EXT, ".bmp") != 0) &&
            (x_stricmp(EXT, ".psd") != 0) &&
            (x_stricmp(EXT, ".png") != 0) )
    {
        // Show error!
        shape::printf("\nUNSUPPORTED TEXTURE EXTENSION FOR %s - SKIPPING !!!!\n", Name) ;

        return -1 ;
    }

    // Split up filename
    char FName  [X_MAX_FNAME];
    x_splitpath( Name, NULL, NULL, FName, NULL) ;

    // Search to see if the texture is already there
    // (NOTE: Only filenames are compared, since all xmps goto the same directory)
    for (i = 0 ; i < m_nTextures ; i++)
    {
        // Split up filename
        char TexFName  [X_MAX_FNAME];
        x_splitpath( m_Textures[i]->GetName(), NULL, NULL, TexFName, NULL) ;

        // Textures match if just filename name matches!
        if (x_strcmp(TexFName, FName) == 0)
            return i ;
    }

    // Okay - add new entry
    ASSERT(m_nTextures < MAX_TEXTURES) ;

    // Try create a new texture
    texture *pTexture = new texture() ;
    if (!pTexture)
        return -1 ;

    // Display texture info
    shape::printf("\nTrying to load texture %s...\n", Name) ;

    // Setup texture
    pTexture->SetName(Name) ;
    
    // Try load the texture
    if (pTexture->Load(Name, LoadSettings, Type))
    {
        shape::printf("\n") ;
        return AddTexture(pTexture) ; // Success!
    }
    else
    {
        // Okay - the .skel texture was not found - let's try the search paths
        if (LoadSettings.SearchTexturePaths)
        {
            // Split up filename
            x_splitpath( Name,DRIVE,DIR,FNAME,EXT);
        
            // Try all search paths
            //for (i = -1 ; i < m_nPaths ; i++)
            for (i = 0 ; i < m_nPaths ; i++)
            {
                // Build filename
                if (i == -1)
                    x_makepath( PATH, NULL, NULL, FNAME, EXT) ; // search path of executable
                else
                    x_makepath( PATH, NULL, m_Paths[i].Name, FNAME, EXT) ;  // use search path

                // Try loading it...
                if (pTexture->Load(PATH, LoadSettings, Type))
                {
                    shape::printf("\n") ;
                    return AddTexture(pTexture) ; // Success!
                }
            }
        }
    }

    shape::printf("\n") ;

    // Destroy the texture that was created
    delete pTexture ;

    // Fail index
    return -1 ;
}

//==============================================================================

// Adds and loads texture(s) - returns the total number that were successfully loaded
// (start index is set to -1 if no textures are loaded)
s32 texture_manager::AddAndLoadTextures(const char*                     Name, 
                                        const shape_load_settings&      LoadSettings,
                                        material::texture_type          Type,
                                        s32                             &StartIndex,
                                        s32                             MaxCount /*=0*/)
{
    // Split up filename
    char Drive  [X_MAX_DRIVE];
    char Dir    [X_MAX_DIR];
    char FName  [X_MAX_FNAME];
    char Ext    [X_MAX_EXT];
    char Path   [X_MAX_PATH];
    x_splitpath( Name, Drive, Dir, FName, Ext) ;

    // Incase of fail
    StartIndex = -1 ;

    // Loading a max .ifl?
    if (x_stricmp(Ext, ".ifl") == 0)
    {
    	token_stream	Tok ; 
        
	    // Try open file
	    if (!Tok.OpenFile(Name))
        {
            AddMissingTexture(Name) ;
		    return 0 ;
        }

        // Get first file in .ifl text file (assumes all textures are in incremental order)
        const char* FirstTexName = Tok.ReadLine() ;

        // Show info
        shape::printf("Loading %s, from %s  ...\n", Name, FirstTexName) ;

        // Tokenizer no longer needed
        Tok.CloseFile() ;

        // Reset count
        s32 Count = 0 ;

        // Texture there?
        if (FirstTexName)
        {
            // Build complete path to first texture
            x_makepath( Path, Drive, Dir, FirstTexName, NULL) ;

            // Load textures
            Count = AddAndLoadTextures(Path, LoadSettings, Type, StartIndex) ;

            // Missing?
            if (Count == 0)
                AddMissingTexture(Path) ;
        }

        // Return count
        return Count ;
    }

    // Try load the first texture
    StartIndex = AddAndLoadTexture(Name, LoadSettings, Type) ;

    // Fail?
    if (StartIndex == -1)
    {
        AddMissingTexture(Name) ;

        // Texture files should never be missing from a binary load!
        if (LoadSettings.Mode == shape_manager::MODE_LOAD_BINARY)
        {
            // Brian - make sure this missing file is in the data/shapes/pc & ps2 directories
            //         If it's not - come see me to make sure I've put it in source safe!
            ASSERTS(0, xfs("Texture: %s.xbmp is missing from the cd build / source safe!!\nMake sure it is in data/shapes/pc & ps2 directories.\nAnd make sure it's in source safe!", FName)) ;
        }

        return 0 ;
    }        

    // If the last 2 digits of the filename are digits, then we have an animated texture!
    xbool Animated = FALSE ;
    s32 FNameLen = x_strlen(FName) ;
    if (        (FNameLen > 2)
            &&  (FName[FNameLen-1] >= '0') && (FName[FNameLen-1] <= '9')
            &&  (FName[FNameLen-2] >= '0') && (FName[FNameLen-2] <= '9') )
    {
        Animated = TRUE ;
    }

    // If not animated, then there is only one texture
    if (!Animated)
        return 1 ;

    // See if there are any more...
    s32 Count    = 1 ;
    s32 TexIndex = StartIndex ;
    while(( TexIndex != -1) && ((MaxCount == 0) || (Count < MaxCount)) )    // load speedup - check for max count
    {
        // Increment filename to next texture number
        char* Digit = &FName[FNameLen-1] ;
        xbool Carry ;
        do
        {
            Carry = FALSE ;

            // On digit?
            if ( ((*Digit) >= '0') && ((*Digit) <= '9') )
            {
                // Carry?
                if ((*Digit) == '9')
                {
                    (*Digit) = '0' ;
                    Carry = TRUE ;
                    Digit-- ;
                }
                else
                    (*Digit) += 1 ;
            }
        }
        while(Carry) ;

        // Build complete path
        x_makepath( Path, Drive, Dir, FName, Ext) ;

        // Try load the texture
        TexIndex = AddAndLoadTexture(Path, LoadSettings, Type) ;

        // If not the next in order, then we must stop
        // eg. hilites03 -> 05 are loaded, then hilites00 -> etc
        if (TexIndex != (StartIndex+Count))
            return Count ;

        // Success?
        if (TexIndex != -1)
            Count++ ; // Increase count
    }

    // Return total loaded
    return Count ;
}

//==============================================================================

// Returns texture, given the index
texture &texture_manager::GetTexture(int Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nTextures) ;
    ASSERT(m_Textures[Index]) ;

    return *m_Textures[Index] ;
}

//==============================================================================

// Returns texture if found, or NULL
texture *texture_manager::GetTextureByName(const char* Name)
{
    s32 Index = GetTextureIndexByName(Name) ;

    // Found?
    if (Index != -1)
    {
        // Return address of texture
        texture& Texture = GetTexture(Index) ;
        return &Texture ;
    }
    else
        return NULL ;
}

//==============================================================================

// Returns texture index if found, or -1
s32  texture_manager::GetTextureIndexByName(const char* Name)
{
    // Split up filename
    char FName  [X_MAX_FNAME], SearchFName[X_MAX_FNAME] ;
    x_splitpath( Name, NULL, NULL, FName, NULL) ;

    // Search all textures
    for (s32 i = 0 ; i < m_nTextures ; i++)
    {
        texture& Texture = GetTexture(i) ;

        // Split up filename
        x_splitpath( Texture.GetName(), NULL, NULL, SearchFName, NULL) ;

        // Match?
        if (x_stricmp(SearchFName, FName) == 0)
            return i ;
    }

    // Not found
    return -1 ;
}

//==============================================================================

// Sets current search path (defaults to null string)
void texture_manager::SetCurrentSearchPath(const char *Path)
{
    // Path 0 is always the current search path
    x_strcpy(m_Paths[0].Name, Path) ;
}

//==============================================================================

// Adds new search path to manager
void texture_manager::AddSearchPath(const char *Path)
{
    // Max paths reached?
    if (m_nPaths == MAX_SEARCH_PATHS)
    {
        shape::printf("\nWARNING: MAX TEXTURE SEARCH PATHS REACHED!!!\n") ;
        return ;
    }

    ASSERT(Path) ;
    ASSERT(m_nPaths < MAX_SEARCH_PATHS) ;

	// If search path is already there, then don't add it
	for (s32 i = 0 ; i < m_nPaths ; i++)
	{
		// Found?
		if (x_strcmp(m_Paths[i].Name, Path) == 0)
			return ;
	}

    ASSERT(x_strlen(Path) < (s32)sizeof(m_Paths[m_nPaths].Name) );
    // Setup path structure
    x_strcpy(m_Paths[m_nPaths].Name, Path) ;
    m_nPaths++ ;
}

//==============================================================================

// Clears all texture paths
void texture_manager::DeleteAllSearchPaths()
{
    // Set current path to NULL
    m_Paths[0].Name[0] = 0 ;
    m_nPaths = 1 ;
}

//==============================================================================

// Clears missing texture list
void texture_manager::ClearMissingTextures()
{
    m_nMissingTextures = 0 ;
}

//==============================================================================

// Adds to missing texture list
void texture_manager::AddMissingTexture(const char* Name)
{
    ASSERT(x_strlen(Name) < (s32)sizeof(m_MissingTextures[m_nMissingTextures]));
    // Add if list is not full
    if (m_nMissingTextures < MAX_MISSING_TEXTURES)
        x_strcpy(m_MissingTextures[m_nMissingTextures++], Name) ;
}

//==============================================================================

// Returns name of missing textures
const char* texture_manager::GetMissingTexture(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nMissingTextures) ;

    return m_MissingTextures[Index] ;
}

//==============================================================================

// Removes all texture from memory
void texture_manager::DeleteAllTextures()
{
    // Delete list of textures
    for (s32 i = 0 ; i < m_nTextures ; i++)
        delete m_Textures[i] ;

    // Init to valid state
    m_nTextures         = 0 ;
    m_nMissingTextures  = 0 ;
}

//==============================================================================

// Writes out all xbmps
void texture_manager::SaveXBMPs( void )
{
    s32 Count, i ;

    // Count number of textures
    Count=0 ;
    for (i = 0 ; i < m_nTextures ; i++)
    {
        // Get texture
        texture* pTexture = m_Textures[i] ;
        ASSERT(pTexture) ;

        // Used?
        if (pTexture->GetReferenceCount() > 0)
            Count++ ;
    }

    // Show info
    shape::printf("Total:%d\n", Count) ;

    // Loop through all textures
    for (i = 0 ; i < m_nTextures ; i++)
    {
        // Get texture
        texture* pTexture = m_Textures[i] ;
        ASSERT(pTexture) ;

        // Only write out if it's referenced
        if (pTexture->GetReferenceCount() > 0)
            pTexture->SaveXBMP() ;
    }
}

//==============================================================================
