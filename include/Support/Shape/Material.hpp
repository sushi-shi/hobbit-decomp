#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__


#include <xCore/Entropy/Entropy.hpp>
#include <Support/Shape/Strip.hpp>
#include <Support/Shape/Vert.hpp>
#include <Support/Shape/Poly.hpp>
#include <Support/Shape/Node.hpp>
#include <Support/Shape/ShapeBinFile.hpp>


//==============================================================================
// Forward references
//==============================================================================
class material ;
class shape ;
struct shape_load_settings ;
class model ;
class texture ;
class texture_manager ;
class collider;
struct material_draw_info ;
struct texture_draw_info ;



//==============================================================================
// Classes
//==============================================================================

// Strip class
class dlist_strip : public shape_bin_file_class
{

friend class material_dlist ;

// Public structures
public:
    struct vert
    {
        vector3     Pos ;
        vector2     TexCoord ;
        vector3     Normal ;
    } ;

    struct ivector3
    {
        s32 X,Y,Z ;
    } ;

    struct ibbox
    {
        ivector3 Min,Max ;
    } ;


// Data
private:
    s32                     m_nVerts ;              // Total verts in strip
    void*                   m_Pos ;                 // List of positions
    void*                   m_TexCoord ;            // List of uv's
    void*                   m_Normal ;              // List of normals
    
    byte        PADDING[12] ;    // Align to 16 bytes for shape_bin_file_class

// Functions
public:

    // Constructor/destructor
    dlist_strip() ;
    ~dlist_strip() ;

    // Operators
    dlist_strip&  operator =  ( dlist_strip& Strip );

    // Initialization
    void    Setup           ( s32 nVerts, void* Pos, void* TexCoord, void* Normal) ;
    void    AddOffset       ( s32 Offset ) ;

    // Vert and poly access
    s32     GetVertCount    ( void ) ;
    void    GetVert         ( s32 VertIndex, vector3& Pos, vector3& Norm, vector2& TexCoord ) ;
    void    SetVert         ( s32 VertIndex, vector3& Pos, vector3& Norm, vector2& TexCoord ) ;
    
    s32     GetPolyCount    ( void ) ;
    xbool   GetPoly         ( s32 PolyIndex, vector3* pVert ) ;

    // Fast collision functions (they return how many polys were sent to the collider)
    s32     ApplyCollision  ( collider& Collider, const matrix4& DListL2W, const dlist_strip::ibbox& LocalBBox) ;
    s32     ApplyCollision  ( collider& Collider, const matrix4& DListL2W, const bbox& LocalBBox) ;

    // Hardcore strip access - use at your own risc!
    void*   GetPosList      ( void )    { return m_Pos ;      }
    void*   GetTexCoordList ( void )    { return m_TexCoord ; }
    void*   GetNormalList   ( void )    { return m_Normal ;   }

    // File IO
    void    ReadWrite       (shape_bin_file &File) ;

} ;

// Material dlist class
class material_dlist : public shape_bin_file_class
{
// Data
private:
    s32             m_Node ;        // Node this dlist is attached to
	s32		        m_nPolys ;		// Total polys in dlist
	s32		        m_VertSize ;	// Size of vertex structure used
    
    byte            *m_DList ;      // Display list
    s32             m_DListSize ;   // Size of display list
                    
    s32             m_nStrips ;     // Total number of strips
    dlist_strip*    m_Strips ;      // List of strips   

    #ifdef TARGET_PC
    LPDIRECT3DVERTEXBUFFER8 m_pVB ;                 // Machine specific vertex buffer etc
    #else
    void*                   m_pVB ;
    #endif

    byte        PADDING[12] ;    // Align to 16 bytes for shape_bin_file_class

// Functions
public:

    // Constructor/destructor
    material_dlist() ;
    ~material_dlist() ;

    // Functions
    void    Setup			(s32 Node, s32 nPolys, s32 VertSize, byte *DList, s32 Size) ;
    void    SetNode			(s32 Node)		    { m_Node = Node ;           }
    s32     GetNode			(void)				{ return m_Node ;           }
    void    SetPolyCount    (s32 PolyCount)	    { m_nPolys = PolyCount ;    }
    s32     GetPolyCount    (void)				{ return m_nPolys ;         }
    void    SetVertSize     (s32 VertSize)	    { m_VertSize = VertSize ;   }
    s32     GetVertSize     (void)				{ return m_VertSize ;       }
    s32     MemoryUsed		(void) ;
    s32     GetDListSize	(void)				{ return m_DListSize ;      }

    #ifdef TARGET_PS2
        void    Draw			(const material_draw_info& DrawInfo, s32 &CurrentLoadedNode) ;
    #endif

    #ifdef TARGET_PC
        void    Draw			(const material& Material, const material_draw_info& DrawInfo, s32 &CurrentLoadedNode) ;
    #endif


    // On the PC this creates the vertex buffers
    void        InitializeForDrawing    ( void ) ;

    // File IO
    void        ReadWrite               (shape_bin_file &File) ;

    // Strip access functions
    void            AddStrip        (dlist_strip &Strip) ;
    s32             GetStripCount   ( void ) ;
    dlist_strip&    GetStrip        ( s32 StripIndex ) ;
} ;

//==============================================================================

// Material class
class material : public shape_bin_file_class
{
// Defines
public:
	// Material texture types
	enum texture_type
	{
		TEXTURE_TYPE_START=0,

		TEXTURE_TYPE_DIFFUSE = TEXTURE_TYPE_START,	// Diffuse texture (put first incase diffuse+alpha are same texture)
		TEXTURE_TYPE_LUMINANCE,                     // Luminance texture (for compression)
        TEXTURE_TYPE_ALPHA,	                        // Alpha texture
		TEXTURE_TYPE_REFLECT,                       // Reflection mapping texture
		TEXTURE_TYPE_BUMP,                          // Bump mapping texture
		
        TEXTURE_TYPE_END = TEXTURE_TYPE_BUMP,

        TEXTURE_TYPE_TOTAL
	} ;

    // Animated texture playback types
    enum playback_type
    {
        PLAYBACK_TYPE_LOOP,
        PLAYBACK_TYPE_PING_PONG,
        PLAYBACK_TYPE_ONCE_ONLY,

        PLAYBACK_TYPE_TOTAL
    } ;

    // Misc
    enum
    {
        MAX_NAME_LENGTH = 32    // Max chars in name
    } ;

// Structures
private:

	// Material flags
	struct flags
	{
		u32 UMirror:1 ;
		u32 VMirror:1 ;
		u32 UWrap:1 ;
		u32 VWrap:1 ;
		u32 SelfIllum:1 ;
		u32 HasAlpha:1 ;
        u32 Additive:1 ;
        u32 PunchThrough:1 ;
	} ;

public:

    // Material texture reference (supports animated textures)
    struct texture_info
    {
        s32     Index ;             // Texture start index in texture manager
        s32     Count ;             // Total consecutive textures
        s32     FrameBits ;         // Used in material consolidation
    } ;


// Static data (for stats)
public:

    #ifdef X_DEBUG
        static  s32         s_LoadSettings ;
        static  s32         s_LoadColor ;
        static  s32         s_LoadTex ;
        static  s32         s_DListDraw ;

        // Call before drawing
        static void ClearStats(void)
        {
            s_LoadSettings = 0 ;
            s_LoadColor    = 0 ;
            s_LoadTex      = 0 ;
            s_DListDraw    = 0 ;
        }

        // Call after drawing
        static void PrintStats(void)
        {
            x_DebugMsg("LoadSettings:%d LoadColor:%d LoadTex:%d DListDraw:%d\n",
                       s_LoadSettings, 
                       s_LoadColor,
                       s_LoadTex,
                       s_DListDraw) ;        
        }
    #endif


// Data
private:
    char                m_Name[MAX_NAME_LENGTH] ;           // Name of material

    s32                 m_nStrips ;                         // Number of strips in material
    strip               *m_Strips ;                         // Array of strips
                   
    s32                 m_TextureRefs[TEXTURE_TYPE_TOTAL] ; // List of texture indices into model texture references
    texture_info        m_TextureInfo[TEXTURE_TYPE_TOTAL] ; // List of texture info
    
    vector4             m_Color ;                           // Color of material
    
    flags               m_Flags ;                           // Property flags
                                                
    s32                 m_nDLists ;                         // Number of dlists
    material_dlist      *m_DLists ;                         // List of dlists (one for each node)

    s32                 m_ID ;                              // Material assigned ID 
    s32                 m_DrawListID ;                      // Draw list assigned ID

    u32                 m_NodeFlags ;                       // Nodes that material is on (1 bit per node)

    byte                PADDING[12] ;                       // Align to 16 bytes for shape_bin_file_class


// Functions
public:

    // Constructor/destructor
    material() ;
    ~material() ;

    // Functions

    // Name functions
    const char* GetName             (void)          { return m_Name ; }
    void        SetName             (const char *Name) ;     

    // Flag functions
    xbool       GetUMirrorFlag      (void) const    { return m_Flags.UMirror ;      } 
    xbool       GetUWrapFlag        (void) const    { return m_Flags.UWrap ;        } 
    xbool       GetVMirrorFlag      (void) const    { return m_Flags.VMirror ;      } 
    xbool       GetVWrapFlag        (void) const    { return m_Flags.VWrap ;        } 
    xbool       GetSelfIllumFlag    (void) const    { return m_Flags.SelfIllum ;    }
    xbool       GetHasAlphaFlag     (void) const    { return m_Flags.HasAlpha ;     } 
    xbool       GetAdditiveFlag	    (void) const    { return m_Flags.Additive ;     } 
    xbool       GetPunchThroughFlag	(void) const    { return m_Flags.PunchThrough ; } 

    void        SetUMirrorFlag      (xbool Value)   { m_Flags.UMirror       = Value ; }
    void        SetUWrapFlag        (xbool Value)   { m_Flags.UWrap         = Value ; }
    void        SetVMirrorFlag      (xbool Value)   { m_Flags.VMirror       = Value ; }
    void        SetVWrapFlag        (xbool Value)   { m_Flags.VWrap         = Value ; }
    void        SetSelfIllumFlag    (xbool Value)   { m_Flags.SelfIllum     = Value ; }
    void        SetHasAlphaFlag     (xbool Value)   { m_Flags.HasAlpha      = Value ; }
    void        SetAdditiveFlag     (xbool Value)   { m_Flags.Additive      = Value ; }
    void        SetPunchThroughFlag (xbool Value)   { m_Flags.PunchThrough  = Value ; }

    // Returns memory used
    s32         MemoryUsed          (void) ;

    void        CleanupUnusedMemory (void) ;

    // Allocated strips
    void        SetStripCount       (s32 Count) ;
           
    // Build display list functions
    s32         GetDListSize        (void) ;

    // On the PC this creates the vertex buffers
    void        InitializeForDrawing( void ) ;

    // Draw functions
    void        Draw                (const material_draw_info& DrawInfo, s32 &CurrentLoadedNode) ;

    // Inline functions

    // Returns # of material display lists
    s32 GetDListCount               (void)          { return m_nDLists ; }
    
    // Returns material display list
    material_dlist  &GetDList(s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nDLists) ;
        return m_DLists[Index] ;
    }

    void            SetColor    (const vector4& Col)    { m_Color   = Col ; }
    const vector4&  GetColor    (void) const		    { return m_Color ;  }

    // Returns texture
    const texture_info &GetTextureInfo(texture_type Type) const
    {
        ASSERT(Type >= 0 ) ;
        ASSERT(Type < TEXTURE_TYPE_TOTAL) ;
        return m_TextureInfo[Type] ;
    }

    // Sets texture reference index
    void    SetTextureInfo(texture_type Type, s32 TextureIndex, s32 TextureCount)
    {
        ASSERT(Type >= 0 ) ;
        ASSERT(Type < TEXTURE_TYPE_TOTAL) ;
        m_TextureInfo[Type].Index = TextureIndex ;
        m_TextureInfo[Type].Count = TextureCount ;
        
        // Setup count bits
        m_TextureInfo[Type].FrameBits = 0 ;
        if (TextureCount)
        {
            TextureCount-- ;    // Frame can only from 0 to (TextureCount-1)
            while(TextureCount)
            {
                m_TextureInfo[Type].FrameBits++ ;
                TextureCount >>= 1 ;
            }
        }
    }
    
    // Sets texture reference index
    void    SetTextureRef(texture_type Type, s32 TextureIndex)
    {
        ASSERT(Type >= 0 ) ;
        ASSERT(Type < TEXTURE_TYPE_TOTAL) ;
        m_TextureRefs[Type] = TextureIndex ;
    }

    // Returns texture reference index
    s32     GetTextureRef(texture_type Type)
    {
        ASSERT(Type >= 0 ) ;
        ASSERT(Type < TEXTURE_TYPE_TOTAL) ;
        return m_TextureRefs[Type] ;
    }
   
    // Returns number of strips in material
    s32     GetStripCount(void)
    {
        return m_nStrips ;
    }
    
    // Returns strip
    strip   &GetStrip(s32 Index)
    {
        ASSERT(Index >= 0) ;
        ASSERT(Index < m_nStrips) ;
        return m_Strips[Index] ;
    }

    // ID used for material consolidation
    void    SetID           (s32 ID)            { m_ID = ID ;   }
    s32     GetID           (void)              { return m_ID ; }

    void    SetDrawListID   (s32 DrawListID)    { m_DrawListID = DrawListID ;   }
    s32     GetDrawListID   (void)              { return m_DrawListID ;         }

    // Get nodes that material is applied to
    u32     GetNodeFlags    (void)          { return m_NodeFlags ; }

    // File IO      
    void    ReadWrite               ( shape_bin_file &File) ;

    // Util functions to create texture names when burning textures together etc.
    void    CreateNewTextureName    (char* NewName, const char* String, const texture& TEX) ;
    void    CreateNewTextureName    (char* NewName, const texture& TEXA, const char* String, const texture& TEXB) ;

    // Checks for an alpha map with 2 values - then burns into diffuse map and uses punch through drawing
    void    CheckForPunchThrough    ( model&                        Model, 
                                      texture_manager&              TextureManager, 
                                      const shape_load_settings&    LoadSettings ) ;

    // Used for PC right now...
    void    BurnAlphaIntoDiffuse    ( model&                        Model, 
                                      texture_manager&              TextureManager, 
                                      const shape_load_settings&    LoadSettings ) ;

    // Checks for compressing diffuse texture
    void    CompressTextures        ( model&                        Model,  
                                      texture_manager&              TextureManager, 
                                      const shape_load_settings&    LoadSettings ) ;

    // Prepares textures ready for using
    void    CheckForBurningTextures ( model&                        Model, 
                                      texture_manager&              TextureManager, 
                                      const shape_load_settings&    LoadSettings ) ;


    // Loads and registers textures
    void    LoadTextures            ( model&                        Model, 
                                      texture_manager&              TextureManager, 
                                      const shape_load_settings&    LoadSettings ) ;

    // Sets up the texture draw info in preparation for rendering
    // Returned is the ID to use in the shape draw list
    s32     LookupTextures          ( const material_draw_info&     DrawInfo,
                                            texture_draw_info&      TexDrawInfo ) ;

    // Misc draw functions
    void    PrimeZBuffer            ( const material_draw_info&     DrawInfo,
                                      const texture_draw_info&      TexDrawInfo,
                                            s32&                    CurrentLoadedNode ) ;
                                                                    
    // Prepare for rendering functions
    void    Activate                ( const material_draw_info&     DrawInfo,
                                            s32&                    CurrentLoadedNode ) ;

    void    Activate                ( const material_draw_info&     DrawInfo, 
                                      const texture_draw_info&      TexDrawInfo,
                                            s32&                    CurrentLoadedNode ) ;

    void    ActivateSettings        ( const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo ) ;
    void    ActivateColor           ( const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo ) ;
    xbool   ActivateTextures        ( const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo ) ;
    
    // Create DList functions
    
    // Combines material strips attached to a node into one big strip    
    void    PC_BuildBigSingleStrip      (s32         nNode,
                                         s32         nModelVerts,
                                         strip_vert* &StripVerts,
                                         s32         &nStripVerts) ;

    void    PS2_BuildBigSingleStrip     (s32         nNode,
                                         s32         nModelVerts,
                                         strip_vert* &StripVerts,
                                         s32         &nStripVerts) ;

    // Builds a single display list
    void    PC_BuildDList               (vert       ModelVerts[], 
                                         s32        nModelVerts, 
                                         s32        Node,
                                         strip_vert StripVerts[],
                                         s32        nStripVerts,
                                         byte*      &DList,
                                         s32        &DListSize,       
                                         s32        &nDListVerts, 
                                         s32        &nDListPolys) ;

    void    PS2_BuildDList              (vert       ModelVerts[], 
                                         s32        nModelVerts, 
                                         s32        Node,
                                         strip_vert StripVerts[],
                                         s32        nStripVerts,
                                         byte*      &DList,
                                         s32        &DListSize,       
                                         s32        &nDListVerts, 
                                         s32        &nDListPolys) ;

    // Builds all material display lists
    void    BuildDLists                 (s32                            nTotalPolys, 
                                         vert                           ModelVerts[], 
                                         s32                            nModelVerts, 
                                         s32                            nNodes, 
                                         s32&                           nDListVerts, 
                                         s32&                           nDListPolys,
                                         shape_bin_file_class::target   Target) ;
} ;


//==============================================================================
// Type defs
//==============================================================================

// User function for messing with render node
typedef void render_material_fn( material&            Material, 
                                 texture_draw_info&   TexDrawInfo,
                                 material_draw_info&  DrawInfo,
                                 s32&                 CurrentLoadedNode ) ;

//==============================================================================
// Draw related structures
//==============================================================================

// Draw info passed through to all draw functions
struct material_draw_info
{
    texture_manager*    TextureManager ;        // Texture manager to use
    u32                 Flags ;                 // Draw flags
    u32                 NodeVisibleFlags ;      // Bit per node for visibility
    u32                 MaterialVisibleFlags ;  // Bit per material for visiblilty
    xcolor              FogColor ;              // Fog color (alpha is %, 0=none, 255=full)
    vector4             Color ;                 // Color multiplier
    vector4             SelfIllumColor ;        // Self illum color multiplier
    render_node*        RenderNodes ;           // Render nodes
    s32                 Frame ;                 // Texture frame
    s32                 PlaybackType ;          // Texture playback type
    f32                 MipK, MinLOD, MaxLOD ;  // Mip info
    texture*            DetailTexture ;         // Detail texture (or NULL)
    s32                 DetailClutHandle ;      // Detail clut handle (or -1)
    f32                 VertsPreScale ;         // Used for PS2
    render_material_fn* RenderMaterialFunc ;    // Special case render material function
    void*               RenderMaterialParam ;   // Param to special case render material function
} ;

// Structure that contains all the textures to use when drawing a material
struct texture_draw_info
{
    // Material textures to use
    texture*            Texture[material::TEXTURE_TYPE_TOTAL] ;
    
    // Detail texture info
    texture*            DetailTexture ;
    s32                 DetailClutHandle ;
} ;



//==============================================================================



#endif  //#ifndef __MATERIAL_HPP__
