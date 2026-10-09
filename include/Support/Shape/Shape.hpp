#ifndef __SHAPE_HPP__
#define __SHAPE_HPP__

#include <xCore/x_files/x_files.hpp>
#include <Support/Shape/Node.hpp>
#include <Support/Shape/Anim.hpp>
#include <Support/Shape/Model.hpp>
#include <Support/Shape/LabelSet.hpp>
#include <Support/Shape/ShapeBinFile.hpp>


// For display list building
#include <Support/Shape/Shaders/D3D_Shape.hpp>


//==============================================================================
// Typedefs
//==============================================================================

typedef void shape_printf_fn( const char* String ) ;


//==============================================================================
// Forward defs
//==============================================================================
class texture_manager ;


//==============================================================================
// Defines
//==============================================================================

#ifdef TARGET_PS2
    #define ALIGN_DATA(__size__)    __attribute__ ((aligned(__size__)))
#else
    #define ALIGN_DATA(__size__)
#endif


#define SHAPE_MAX_FNAME 48


//==============================================================================
// Structures
//==============================================================================

// Load settings passed to all loaders...
struct shape_load_settings
{
    // Texture settings
    xbool                           BuildMips ;             // Build mips for textures
    xbool                           CheckForXBMP ;          // Checks for loading xbmp first
    xbool                           SearchTexturePaths ;    // If texture is not found, search all texture paths
    xbool                           CheckSkelTimeStamps ;   // Checks time stamps in skel file
    xbool                           BurnTextures ;          // Burns alpha into diffuse if needed
    xbool                           CompressTextures ;      // Set to TRUE to compress diffuse texture

    // Target settings
    shape_bin_file_class::target    Target ;                // Target machine of load/build
    s32                             Mode ;                  // shape_manager::Mode of loading
} ;


//==============================================================================
// Classes
//==============================================================================


// Shape class
class shape
{
friend class material ;
friend class texture ;
friend class model ;
friend class anim ;
friend class key_set ;
friend class shape_instance ;

// Defines
public:

    // Draw flags
    enum draw_flags
    {
        DRAW_FLAG_CLIP                              = (1<<0),   // Clips polys (slower)
                                                    
        DRAW_FLAG_REF_WORLD_SPACE                   = (1<<1),   // Reflection maps in world space (instead of view space)
        DRAW_FLAG_REF_RANDOM                        = (1<<2),   // Choose random reflection matrix (use for fx)
        DRAW_FLAG_REF_SPECIFIED                     = (1<<3),   // Use specified instance reflection rotation
                                                    
        DRAW_FLAG_FOG                               = (1<<4),   // Draw with fog
        DRAW_FLAG_TURN_OFF_LIGHTING                 = (1<<5),   // Turns off lighting (it's on by default)
        DRAW_FLAG_CULL                              = (1<<6),   // Culls polys
        DRAW_FLAG_PRIME_Z_BUFFER                    = (1<<7),   // Draws object in z buffer, before drawing (can fix alpha fading)
        DRAW_FLAG_SORT                              = (1<<8),   // Draws instance in view order with other sorted instances
                                                    
        DRAW_FLAG_VISIBILITY_ANIMATED               = (1<<9),   // Internal use for drawing aniamted visibility
        DRAW_FLAG_VISIBILITY_PASS_TRANSPARENT       = (1<<10),  // Internal use for drawing transparent nodes
        DRAW_FLAG_VISIBILITY_PASS_OPAQUE            = (1<<11),  // Internal use for drawing opaque nodes
                                                    
        DRAW_FLAG_TURN_ON_ANIM_MAT_DETAIL           = (1<<12),  // Allows detail texture on animated materials (default is off)
        DRAW_FLAG_TURN_OFF_STATIC_MAT_DETAIL        = (1<<13),  // Turns off detail texture on static (non-animated) materials (default is on)
        DRAW_FLAG_TURN_OFF_SELF_ILLUM_MAT_DETAIL    = (1<<14),  // Turns off detail texture on self illuminated materials (default is on)
        
        DRAW_FLAG_TURN_OFF_ALPHA_PASS               = (1<<15),  // Turns off alpha map pass
        DRAW_FLAG_TURN_OFF_DIFFUSE_PASS             = (1<<16),  // Turns off diffuse map pass
        DRAW_FLAG_TURN_OFF_LUMINANCE_PASS           = (1<<17),  // Turns off luminance map pass
        DRAW_FLAG_TURN_OFF_REFLECT_PASS             = (1<<18),  // Turns off reflection map pass
        DRAW_FLAG_TURN_OFF_BUMP_PASS                = (1<<19),  // Turns off bump map pass
        DRAW_FLAG_TURN_OFF_DETAIL_PASS              = (1<<20),  // Turns off detail map pass
    } ;

    enum misc
    {
        MAX_DETAIL_TEXTURES=4,
    } ;

// Structures
public:
    // View plane info
    struct plane_info
    {
        plane   Plane ;             // Plane in world space
        s32     BBoxMinIndex[3] ;   // Min bbox index
        s32     BBoxMaxIndex[3] ;   // Max bbox index
    } ;

    // Settings that control drawing 
    // (can be tweaked in the debugger)
    struct draw_settings
    {
        xbool   Fog ;               // Turns fogging on/off
        xbool   Mips ;              // Turns mips/on/off
        xbool   AlphaPass ;         // Turns alpha pass on/off
        xbool   DiffusePass ;       // Turnd diffuse pass on/off
        xbool   LuminancePass ;     // Turns luminance pass on/off
        xbool   ReflectPass ;       // Turns reflection pass on/off
        xbool   DetailPass ;        // Turns detail pass on/off
        xbool   BumpPass ;          // Turns bump pass on/off
        xbool   UseDrawList ;       // Turns use of draw list on/off
        xbool   WireFrame ;         // Turns wire frame on/off
    } ;


// Public static data
public:

    // Draw settings
    static draw_settings    s_DrawSettings ;

// Private static data
private:

    // To make sure the shape library has been setup
    static xbool            s_Initialized ;

	// Output path for .shp and xbmp files
	static char             s_SHP_Path  [X_MAX_PATH] ;
	static char             s_XBMP_Path [X_MAX_PATH] ;
                            
	// Light                
	static vector4          s_LightDirection ;
	static vector4          s_LightColor ;
	static vector4          s_LightAmbientColor ;
                            
	// Fog                  
    static xbool            s_FogEnabled ;
	static xcolor           s_FogColor ;
                            
	// Drawing              
	static xbool            s_InBeginFrame ;
	static xbool            s_InBeginDraw ;
	static s32              s_FrameCount ;

    // Detail textures
    static s32              s_nDetailTextures ;
    static texture*         s_DetailTextures[MAX_DETAIL_TEXTURES] ;
    static s32              s_DetailTextureFrameBits ;

    // View                 
    static plane_info       s_ViewPlane[6] ;
    static plane_info       s_ClipPlane[6] ;

    // Debugging
    static shape_printf_fn* s_PrintfFunc ;

    // Material ID's (for debugging to see how big the draw lists should be)
    static s32              s_LastAssignedMaterialID ;
    static s32              s_LastAssignedMaterialDrawListID ;

// PS2 specific
#ifdef TARGET_PS2
	static s32              s_hMicroCode ;
#endif

// D3D specific
#ifdef TARGET_PC
    static xbool          s_UseDot3 ;

    static s32            s_hDifVertexShader ;
    static s32            s_hDifPixelShader ;

    static s32            s_hTexDifVertexShader ;
    static s32            s_hTexDifPixelShader ;
                            
    static s32            s_hTexDifRefVertexShader ;
    static s32            s_hTexDifRefPixelShader ;
                            
    static s32            s_hTexDifRefBumpVertexShader ;
    static s32            s_hTexDifRefBumpPixelShader ;
                            
    static s32            s_hTexDifDot3VertexShader ;
    static s32            s_hTexDifDot3PixelShader ;
                            
    static s32            s_hTexDifRefDot3VertexShader ;
    static s32            s_hTexDifRefDot3PixelShader ;
#endif


// Data
private:
    char            m_Name[SHAPE_MAX_FNAME] ;       // Filename
    int             m_Type ;                        // Type of shape
    
    s32             m_nModels ;                     // Number of models
    model           *m_Models ;                     // Array of models

    texture_manager *m_TextureManager ;             // Pointer to texture manager this shape is using
                                              
    s32             m_nAnims ;                      // Number of anims
    anim            *m_Anims ;                      // Array of anims
                                              
    bbox            m_Bounds ;                      // Combined bounds of all models

    s32             m_nDListVerts, m_nDListPolys ;  // Display list stats

    byte            *m_BinFileData ;                // Big single allocation used when reading binary file


    byte            PADDING[4] ;                    // Align to 16 bytes for shape_bin_file_class


// Functions
public:

    //======================================================================
    // Class functions
    //======================================================================

    // Constructor/destructor
    shape() ;
    ~shape() ;

    // Initialization functions
    void            CleanupUnusedMemory         () ;
    s32             MemoryUsed                  () ;
    void            CalculateBounds             () ;
    void            DeleteUnusedNodes           () ;
    void            InitializeForDrawing        (shape_bin_file_class::target Target) ;

    // Inline functions
    void            SetName                     (const char *Name) ;
    const char      *GetName                    ()      { return m_Name ;           }
    s32             GetType                     ()      { return m_Type ;           }
    
    s32             GetModelCount               ()      { return m_nModels ;        }
    s32             GetAnimCount                ()      { return m_nAnims ;         }
                                                
    void            SetTextureManager           (texture_manager *TM)   { m_TextureManager = TM ;}
    texture_manager &GetTextureManager          ()      { return *m_TextureManager ;}
    bbox            GetBounds                   ()      { return m_Bounds ;         }
                                                
    s32             GetDListVertCount           ()      { return m_nDListVerts ;    }
    s32             GetDListPolyCount           ()      { return m_nDListPolys ;    }

    // Loads all textures
    void            LoadTextures                (const shape_load_settings&   LoadSettings) ;
                                                
    // Model functions
    model           &GetModel                   (s32 Index) ;
    model           *GetModelByIndex            (s32 Index) ;
    model           *GetModelByType             (s32 Type) ;
                                                
    // Animation functions
    void            SortAnimsByType             (void) ;
    anim            &GetAnim                    (s32 Index) ;
    anim            *GetAnimByIndex             (s32 Index) ;
    anim            *GetAnimByType              (s32 Type) ;
    void            AddAnim                     (anim &Anim) ;

    // File IO
    void            ReadWrite                   (shape_bin_file &File) ;
    xbool           ReadBinaryFile              (const char *Filename, const shape_load_settings& LoadSettings) ;

    // Misc
    s32             GetAnimFileCount			() ;
    s32             GetModelFileCount			() ;


    //======================================================================
    // Functions found in "ShapeReadAsc.cpp"
    //======================================================================

    // Ascii file functions
	xbool ReadAsciiFileTimeStamp(const char *Filename, time_stamp &TimeStamp) ;

	// Load straight from a binary
	xbool LoadFromBinaryFile(const char*                    ShapeFile, 
                             const shape_load_settings&     LoadSettings) ;

    // Loads shape, models, and animations
	xbool LoadFromAsciiFiles(const char**	                ModelFiles, 
                             const char**	                AnimFiles, 
                             const shape_load_settings&     LoadSettings,
                             label_set*		                LabelSet=NULL,
							 const char*	                ShapeFile=NULL) ;

    // Loads ascii file and optionally extracts model or animation
    xbool ReadAsciiFile(const char *Filename, model *pModel, anim *pAnim, label_set *LabelSet = NULL) ;

    //======================================================================
    // Static global functions
    //======================================================================

    // Init/kill functions (need to be called to setup vertex/pixel shaders etc)
    static void         Init                ( void ) ;
    static void         Kill                ( void ) ;

    // Data output path functions
    static void         SetDataOutputPaths  (const char *SHP_Path, const char *XBMP_Path) ;

    static void         Set_SHP_Path        (const char *SHP_Path) ;
    static const char   *Get_SHP_Path       () ;
    
    static void         Set_XBMP_Path       (const char *XBMP_Path) ;
    static const char   *Get_XBMP_Path      () ;

    // Light functions
    static void     SetLightDirection       (const vector3& vDir, xbool Normalize) ;
    
	static void     SetLightColor           (f32 R, f32 G, f32 B, f32 A) ;
    static void     SetLightColor           (const vector4& Col) ;
    static void     SetLightColor           (const xcolor&  Col) ;

    static void     SetLightAmbientColor    (f32 R, f32 G, f32 B, f32 A) ;
    static void     SetLightAmbientColor    (const vector4& Col) ;
    static void     SetLightAmbientColor    (const xcolor&  Col) ;

    static vector3  GetLightDirection       () ;
    static vector4  GetLightColor           () ;
    static vector4  GetLightAmbientColor    () ;

    // Fog functions
    static void     SetFogEnabled           ( xbool Enabled ) ;
    static xbool    GetFogEnabled           () ;
    
    static void     SetFogColor             ( const xcolor &Color ) ;
    static xcolor   GetFogColor             () ;

    // PS2 functions
    #ifdef TARGET_PS2
        static void InitMicroCode           () ;
    #endif    

    // Draw functions
    // (you must call "shape::BeginDraw()" and "shape::EndDraw()"
    //  around your shape instance render code)
    static xbool    InBeginDraw             () ;
    static void     BeginDrawSettings       () ;
    static void     EndDrawSettings         () ;
    static void     BeginDraw               () ;
    static void     EndDraw                 () ;

    // Draw list functions
    static void     AssignMaterialIDs       (shape* Shapes[], s32 nShapes, s32& ID, s32& DrawListID) ;
    static void     AssignMaterialIDs       (shape* Shapes[], s32 nShapes) ;

    static void     ClearDrawList           () ;
    static void     AddInstanceToDrawList   (shape_instance& Instance, material_draw_info& DrawInfo) ;
    static void     FlushInstanceDrawList   () ;
    static void     AddMaterialToDrawList   (material& Material, material_draw_info& DrawInfo) ;
    static void     FlushDrawList           () ;

    static xbool    InBeginFrame            () ;
    static void     BeginFrame              () ;    // Must call at beggining of rendering frame
    static void     EndFrame                () ;    // Must call at end of rendering frame
    static s32      GetFrameCount           () ;
   
    // View function
    static s32      BBoxInViewVolume        ( const bbox& BBox ) ;
    static s32      BBoxInClipVolume        ( const bbox& BBox ) ;

    // Debugging
    static void     DefaultPrintf           ( const char* String ) ;
    static void     SetPrintfFunc           ( shape_printf_fn* fnPrintf ) ;
    static void     printf                  ( const char* pFormatStr, ... ) ;


} ;

//==============================================================================




#endif  //#ifndef __SHAPE_HPP__
