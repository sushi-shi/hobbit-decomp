// ShapeManager.cpp
//
	
#include <Support/Shape/ShapeManager.hpp>
#include "Tokenizer/Tokenizer.hpp"
#include <Support/Shape/LabelSet.hpp>




//==============================================================================
//
// shape_manager class - a simple class to load and track shapes
//
//==============================================================================


//==============================================================================
// Functions
//==============================================================================

// Constructor
shape_manager::shape_manager()
{
    // Init to valid state
    m_nShapes = 0 ;
}

//==============================================================================

// Destructor
shape_manager::~shape_manager()
{
    DeleteAllShapes() ;
}

//==============================================================================

// Builds binary shapes
s32 shape_manager::AddAndLoadShapes(const char*                     Filename, 
                                    shape_manager::mode             Mode,
                                    shape_bin_file_class::target    Target)
{
    // Max files
    enum max_files
    {
        MAX_MODEL_FILES = 10,
        MAX_ANIM_FILES  = 100,
        MAX_SKEL_FILES  = (MAX_MODEL_FILES + MAX_ANIM_FILES)
    } ;

    // Simple file struc
    struct skel_file
    {
        char File[TOKEN_STRING_SIZE] ;
    } ;

    // Locals
	s32				        i ;
	token_stream	        Tok ; 
    label_set               LabelSet ;
                            
    skel_file*              SkelFiles    = NULL ;
    s32                     nSkelFiles   = 0 ;
                            
	const char		        **ModelFiles = NULL	;
    s32                     nModelFiles  = 0 ;
					        
	const char	            **AnimFiles  = NULL ;
    s32                     nAnimFiles   = 0 ;
                            
    char                    Drive     [X_MAX_DRIVE];
    char                    Dir       [X_MAX_DIR];
    char                    SearchPath[X_MAX_PATH];

    // Show some info
    //x_DebugMsg("Loading shapes from %s\n", Filename) ;

	// Try open file
	if (!Tok.OpenFile(Filename))
	{
		ASSERT(0) ;
		return 0 ;
	}

    // Alloc temporary file name storage data
    SkelFiles  = (skel_file*)  x_malloc(MAX_SKEL_FILES      * sizeof(skel_file)) ;
    ASSERT(SkelFiles) ;

    ModelFiles = (const char**)x_malloc((MAX_MODEL_FILES+1) * sizeof(char*)) ;
    ASSERT(ModelFiles) ;

    AnimFiles  = (const char**)x_malloc((MAX_ANIM_FILES+1)  * sizeof(char*)) ;
    ASSERT(AnimFiles) ;        

    // Lookup target if none is given
    if (Target == shape_bin_file_class::DEFAULT)
        Target = shape_bin_file_class::GetDefaultTarget() ;

    // Setup general load settings
    shape_load_settings LoadSettings ;
    LoadSettings.BuildMips           = (Mode == MODE_BUILD_BINARY) ;
    LoadSettings.CheckForXBMP        = (Mode != MODE_LOAD_ASCII) ;
    LoadSettings.SearchTexturePaths  = (Mode == MODE_BUILD_BINARY) || (Mode == MODE_LOAD_ASCII)  ;
    LoadSettings.CheckSkelTimeStamps = (Mode == MODE_BUILD_BINARY) ;
    LoadSettings.BurnTextures        = (Mode == MODE_BUILD_BINARY) || (Mode == MODE_LOAD_ASCII) ;
    LoadSettings.CompressTextures    = FALSE ;
    LoadSettings.Target              = Target ;  
    LoadSettings.Mode                = Mode ;

    // Setup texture load settings (they don't need mips)
    shape_load_settings TexLoadSettings = LoadSettings ;
    TexLoadSettings.BuildMips      = FALSE ;

    // Lookup target strings
    const char *SHP_PathString ;
    const char *XBMP_PathString ;
    switch(Target)
    {
        default:
            ASSERT(0) ; // not supported type!

        case shape_bin_file_class::PS2:
            SHP_PathString  = "PS2_SHP_PATH" ;
            XBMP_PathString = "PS2_XBMP_PATH" ;
            break ;

        case shape_bin_file_class::PC:
            SHP_PathString  = "PC_SHP_PATH" ;
            XBMP_PathString = "PC_XBMP_PATH" ;
            break ;
    }

    // Clear shape output paths
    shape::SetDataOutputPaths(NULL, NULL) ;

	//-----------------------------------------------------------------------
	// Pass 1 - Load all label sets, texture paths, special case stuff etc
	//-----------------------------------------------------------------------
	do
	{
		// Read next token
		Tok.Read() ;

		// Symbol found?
		if (Tok.Type() == token_stream::TOKEN_SYMBOL)
		{
            switch(Mode)
            {
                case MODE_BUILD_BINARY:
                case MODE_LOAD_ASCII:

			        // Label set? (Only need label sets if reading ascii files)
			        if (x_strcmp(Tok.String(), "LoadLabelSet") == 0)
					    LabelSet.Read(Tok.ReadString()) ;
                    else
                    // Texture path?
			        if (x_strcmp(Tok.String(), "AddTexturePath") == 0)
				        m_TextureManager.AddSearchPath(Tok.ReadString()) ;

                    // Fall through...

                case MODE_LOAD_BINARY:

			        // Texture(s)?
			        if (x_strcmp(Tok.String(), "LoadTexture") == 0)
                    {
                        /* DEDICATED_SERVER

                        s32 StartIndex, Count ;
                        Count = m_TextureManager.AddAndLoadTextures(Tok.ReadString(),
                                                                    TexLoadSettings,
                                                                    material::TEXTURE_TYPE_DIFFUSE, 
                                                                    StartIndex) ;

                        // Make sure texture was found
                        // (delete it from your .txt file if it is not needed)
                        ASSERT(Count) ;

                        // Flag texture is used so the xbmp is written!
                        for (i = 0 ; i < Count ; i++)
                        {
                            texture& Tex = m_TextureManager.GetTexture(StartIndex + i) ;
                            Tex.IncReferenceCount() ;
                        }

                        DEDICATED_SERVER */
                    }
                    else
                    // SHP path?
                    if (x_strcmp(Tok.String(), SHP_PathString) == 0)
                        shape::Set_SHP_Path(Tok.ReadString()) ;
                    else
                    // XBMP path?
                    if (x_strcmp(Tok.String(), XBMP_PathString) == 0)
                        shape::Set_XBMP_Path(Tok.ReadString()) ;
                    break ;
            }
		}
	}
	while(Tok.Type() != token_stream::TOKEN_EOF) ;


	// Clear filename counts
    char  ShapeFile[TOKEN_STRING_SIZE] = {0} ;
    xbool ShapeFileBegin = FALSE ;


	//-----------------------------------------------------------------------
	// Pass 2 - Read all shapes (now that all label sets and paths are setup)
	//-----------------------------------------------------------------------
    Tok.Rewind() ;
	do
	{
		// Read next token
		Tok.Read() ;

		// Anim or model file?
		if (Tok.Type() == token_stream::TOKEN_SYMBOL)
		{
            // Check for loading model or anim file (only when building shapes)
            switch(Mode)
            {
                case MODE_BUILD_BINARY:
                case MODE_LOAD_ASCII:

			        // Model filename?
			        if (x_strcmp(Tok.String(), "ModelFile") == 0)
                    {
                        ASSERT(ShapeFileBegin == TRUE) ;
                        ASSERT(nSkelFiles  < MAX_SKEL_FILES) ;
                        ASSERT(nModelFiles < MAX_MODEL_FILES) ;

                        // Add to skel file list
				        char *NextModelFile = Tok.ReadString() ;
                        x_strcpy(SkelFiles[nSkelFiles].File, NextModelFile) ;

                        // Add to model list
                        ModelFiles[nModelFiles++] = SkelFiles[nSkelFiles++].File ;

                        // Make current texture search path be the skel files directory
                        x_splitpath(NextModelFile,Drive,Dir,NULL,NULL);
                        x_makepath(SearchPath, Drive, Dir, NULL, NULL) ;
                        m_TextureManager.SetCurrentSearchPath(SearchPath) ;
                    }
			        else
			        // Anim filename?
			        if (x_strcmp(Tok.String(), "AnimFile") == 0)
                    {
                        ASSERT(ShapeFileBegin == TRUE) ;
                        ASSERT(nSkelFiles  < MAX_SKEL_FILES) ;
                        ASSERT(nModelFiles < MAX_ANIM_FILES) ;

				        char *NextAnimFile = Tok.ReadString() ;

                        // Make sure it hasn't been added twice!
                        for (i = 0 ; i < nAnimFiles ; i++)
                        {
                            // If you assert here, it means you have added the same anim file more than once!
                            // CHECK YOUR SHAPES .TXT FILE!!
                            ASSERT(x_strcmp(NextAnimFile, AnimFiles[i]) != 0) ;
                        }

                        // Add to skel file list
                        x_strcpy(SkelFiles[nSkelFiles].File, NextAnimFile) ;

                        // Add to anim list
                        AnimFiles[nAnimFiles++] = SkelFiles[nSkelFiles++].File ;
                    }
                    break ;
            }
        }

        // All mode token process
		if (Tok.Type() == token_stream::TOKEN_SYMBOL)
        {
		    // Turn compress texture on?
		    if (x_strcmp(Tok.String(), "CompressTexturesOn") == 0)
            {
                ASSERT(ShapeFileBegin == FALSE) ;
                LoadSettings.CompressTextures = TRUE ;
            }
            else
            // Turn compress textures off?
		    if (x_strcmp(Tok.String(), "CompressTexturesOff") == 0)
            {
                ASSERT(ShapeFileBegin == FALSE) ;
                LoadSettings.CompressTextures = FALSE ;
            }
            else
		    // Turn build mips on?
		    if ((x_strcmp(Tok.String(), "BuildMipsOn") == 0) && (Mode != MODE_LOAD_BINARY))
            {
                ASSERT(ShapeFileBegin == FALSE) ;
                LoadSettings.BuildMips = TRUE ;
            }                
            else
            // Turn build mipss off?
		    if ((x_strcmp(Tok.String(), "BuildMipsOff") == 0) && (Mode != MODE_LOAD_BINARY))
            {
                ASSERT(ShapeFileBegin == FALSE) ;
                LoadSettings.BuildMips = FALSE ;
            }
            else
            // At start of new shape?
		    if (x_strcmp(Tok.String(), "ShapeFileBegin") == 0)
            {
                ASSERT(ShapeFileBegin == FALSE) ;

				// Read shape filename
				Tok.ReadString() ;
                x_strcpy(ShapeFile, Tok.String()) ;
                ASSERT(x_strlen(ShapeFile)) ;
                ShapeFileBegin = TRUE ;
            }
            else
            // At end of shape? ie. trigger loading of shape
		    if (x_strcmp(Tok.String(), "ShapeFileEnd") == 0)
            {
                ASSERT(ShapeFileBegin == TRUE) ;

			    // Create a new shape
			    shape *Shape = new shape() ;
			    ASSERT(Shape) ;

			    // Add to list
			    ASSERT(m_nShapes < MAX_SHAPES) ;
			    m_Shapes[m_nShapes++] = Shape ;

			    // Assign a texture manager
			    Shape->SetTextureManager(&m_TextureManager) ;

                switch(Mode)
                {
                    case MODE_LOAD_BINARY:

                        // Show some info
                        //x_DebugMsg("   Loading %s\n", ShapeFile) ;

                        // Show info
                        switch(Target)
                        {
                            case shape_bin_file_class::PS2:
                                shape::printf("\n\n\n*************** LOADING PS2 %s ***************\n\n", ShapeFile) ;
                                break ;

                            case shape_bin_file_class::PC:
                                shape::printf("\n\n\n*************** LOADING PC %s ***************\n\n", ShapeFile) ;
                                break ;
                        }

				        // Load it.. no questions asked
				        Shape->LoadFromBinaryFile(ShapeFile,        // ShapeFile
                                                  LoadSettings) ;   // Settings
                        break ;

                    case MODE_BUILD_BINARY:

                        // Show info
                        switch(Target)
                        {
                            case shape_bin_file_class::PS2:
                                shape::printf("\n\n\n*************** BUILDING PS2 %s ***************\n\n", ShapeFile) ;
                                break ;

                            case shape_bin_file_class::PC:
                                shape::printf("\n\n\n*************** BUILDING PC %s ***************\n\n", ShapeFile) ;
                                break ;
                        }

                    case MODE_LOAD_ASCII:

		                // There must be at least one model file!
                        ASSERT(nModelFiles) ;
		                if (nModelFiles)
		                {
                            ASSERT(nSkelFiles  <= MAX_SKEL_FILES) ;
                            ASSERT(nModelFiles <= MAX_MODEL_FILES) ;
                            ASSERT(nAnimFiles  <= MAX_ANIM_FILES) ;

                            // Terminate file lists
                            ModelFiles[nModelFiles] = NULL ;
                            AnimFiles [nAnimFiles]  = NULL ;

			                // Load the shape from the ascii files - this also builds the shp and xbmp files
			                Shape->LoadFromAsciiFiles(ModelFiles,                                       // Model files
									                  AnimFiles,                                        // Anim files
									                  LoadSettings,                                     // Settings
                                                      &LabelSet,                                        // Label set
                                                      (Mode == MODE_LOAD_ASCII) ? NULL : ShapeFile) ;   // ShapeFile to build/check
		                }
                        break ;
                }

			    // Clear file counts ready for next shape load
                nSkelFiles  = 0 ;
			    nModelFiles = 0 ;
			    nAnimFiles  = 0 ;

                // Done!
                ShapeFileBegin = FALSE ;
            }
        }
	}
	while(Tok.Type() != token_stream::TOKEN_EOF) ;

    // Make sure file was ended good!
    ASSERT(ShapeFileBegin == FALSE) ;

	// Cleanup
	Tok.CloseFile() ;

    // Assign default material ID's starting from 0
    AssignMaterialIDs() ;

    // Write out XBMP's?
    if ((Mode == MODE_BUILD_BINARY) && (LoadSettings.CheckForXBMP))
    {
        // Show info
        shape::printf("\n\nWriting XBMPs...\n") ;

        // Write out all XBMPs
        m_TextureManager.SaveXBMPs() ;

        shape::printf("\n\n") ;
    }

    // Show shape info
    if (Mode == MODE_BUILD_BINARY)
    {
        // Show info
        shape::printf("\n\n%d Shapes loaded...\n", m_nShapes) ;

        // Write out all shape info
        for (i = 0 ; i < m_nShapes ; i++)
        {
            shape* Shape = m_Shapes[i] ;
            ASSERT(Shape) ;

            // Get simple shape name
            char    DRIVE   [X_MAX_DRIVE];
            char    DIR     [X_MAX_DIR];
            char    FNAME   [X_MAX_FNAME];
            char    EXT     [X_MAX_EXT];
            char    Name    [X_MAX_PATH];
            x_splitpath( Shape->GetName(), DRIVE, DIR,  FNAME, EXT) ;
            x_makepath ( Name,             NULL,  NULL, FNAME, EXT) ;

            // Print info
            shape::printf("   Shape:%30s Nodes:%.3d Models:%.3d Anims:%.3d Size:%.8d Bytes %.5d K\n",
                           Name,
                           Shape->GetModelCount() ? Shape->GetModelByIndex(0)->GetNodeCount() : 0,
                           Shape->GetModelCount(),
                           Shape->GetAnimCount(),
                           Shape->MemoryUsed(),
                           (Shape->MemoryUsed()+1023)/1024 ) ;
        }
        
        shape::printf("\n\n") ;
    }


    // Display missing textures
    if (m_TextureManager.GetMissingTextureCount())
    {
        // Header
        shape::printf("\n") ;
        shape::printf("!!!!!***** START OF MISSING TEXTURES LIST *****!!!!!\n") ;

        // Print them all
        for (i = 0 ; i < m_TextureManager.GetMissingTextureCount() ; i++)
            shape::printf("   %s\n", m_TextureManager.GetMissingTexture(i)) ;

        // Foot
        shape::printf("\n") ;
        shape::printf("!!!!!*****  END OF MISSING TEXTURES LIST  *****!!!!!\n\n") ;
    }

    // Display 32bit textures
    if (Mode == MODE_BUILD_BINARY)
    {
        // Are there any 32bit textures?
        xbool bFound = FALSE ;
        for (i = 0 ; i < m_TextureManager.GetTextureCount() ; i++)
        {
            // Get texture and check to see if it's 32bit
            texture& Tex = m_TextureManager.GetTexture(i) ;
            if (Tex.GetXBitmap().GetBPP() == 32)
            {
                bFound = TRUE ;
                break ;
            }
        }

        // Show info?
        if (bFound)
        {
            // Header
            shape::printf("\n") ;
            shape::printf("!!!!!***** START OF 32BIT TEXTURES LIST *****!!!!!\n") ;

            // Check all textures
            for (i = 0 ; i < m_TextureManager.GetTextureCount() ; i++)
            {
                // Get texture and check to see if it's 32bit
                texture& Tex = m_TextureManager.GetTexture(i) ;
                xbitmap& BM  = Tex.GetXBitmap() ;
                if (BM.GetBPP() == 32)
                {
                    // Show texture info
                    shape::printf("\n   %s (%.4d x %.4d x %.4d  %.8d Bytes  %.5dK)\n", 
                                  Tex.GetName(),
                                  BM.GetWidth(), 
                                  BM.GetHeight(), 
                                  BM.GetBPP(),
                                  BM.GetDataSize() + BM.GetClutSize(),
                                  (BM.GetDataSize() + BM.GetClutSize()+1023)/1024 ) ;

                    // Loop through all the shapes to find source
                    for (s32 s = 0 ; s < m_nShapes ; s++)
                    {
                        // Get shape
                        shape& Shape = GetShape(s) ;

                        // Loop through all the models in the shape
                        for (s32 m = 0 ; m < Shape.GetModelCount() ; m++)
                        {
                            // Get model
                            model& Model = Shape.GetModel(m) ;

                            // Loop through all materials in model
                            for (s32 mat = 0 ; mat < Model.GetMaterialCount() ; mat++)
                            {
                                material& Mat = Model.GetMaterial(mat) ;

                                // Loop through all textures in material
                                for (s32 t = material::TEXTURE_TYPE_START ; t <= material::TEXTURE_TYPE_END ; t++)
                                {
                                    // Using this texture?
                                    s32 TexIndex = Mat.GetTextureInfo((material::texture_type)t).Index ;
                                    if (TexIndex == i)
                                    {
                                        // Show model
                                        shape::printf("      used on model:%s by ", Model.GetName()) ;

                                        // Show texture slot type
                                        switch(t)
                                        {
                                            default:    ASSERT(0) ; break ; // implement me!
                                            case material::TEXTURE_TYPE_ALPHA:   shape::printf("alpha") ;   break ;
                                            case material::TEXTURE_TYPE_DIFFUSE: shape::printf("diffuse") ; break ;
                                            case material::TEXTURE_TYPE_REFLECT: shape::printf("reflect") ; break ;
                                        }

                                        // Show material
                                        shape::printf(" map on material:%s \n", Mat.GetName()) ;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Foot
            shape::printf("\n") ;
            shape::printf("!!!!!*****  END OF 32BIT TEXTURE LIST  *****!!!!!\n\n") ;
        }
    }

    // Free temporary storage
    x_free(SkelFiles) ;
    x_free(ModelFiles) ;
    x_free(AnimFiles) ;

    // Sort shapes by type for faster lookup
    SortShapesByType() ;

	// Return # loaded
	return m_nShapes ;
}

//==============================================================================

// Returns shape, given the index
shape &shape_manager::GetShape(int Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nShapes) ;
    ASSERT(m_Shapes[Index]) ;

    return *m_Shapes[Index] ;
}
//==============================================================================

// Returns shape, given the index
shape  *shape_manager::GetShapeByIndex(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nShapes) ;
    ASSERT(m_Shapes[Index]) ;

    return m_Shapes[Index] ;
}

//==============================================================================

// Compares shapes by their types
s32 shape_type_compare_fn( const void* pItem1, const void* pItem2 )
{
	shape **ppShapeA = (shape **)pItem1 ;
	shape **ppShapeB = (shape **)pItem2 ;

    // Sort by type
	if ( (*ppShapeA)->GetType() > (*ppShapeB)->GetType() ) return 1 ;
	if ( (*ppShapeA)->GetType() < (*ppShapeB)->GetType() ) return -1 ;
    return 0;
}

// Sorts shapes by type for faster lookup
void shape_manager::SortShapesByType()
{
	// Sort shapes by type
    if (m_nShapes)
    {
        x_qsort(m_Shapes,   			    // Address of first item in array.
			    m_nShapes,	                // Number of items in array.
                sizeof(shape*), 		    // Size of one item.
			    shape_type_compare_fn) ;	// Compare function.
    }
}

//==============================================================================

// Returns shape, given the type if found, else NULL
shape  *shape_manager::GetShapeByType(s32 Type)
{
    // Do binary search...
    s32     Lo, Mid, Hi ;
    shape*  Shape ;

    // Must be some shapes!
    if (!m_nShapes)
        return NULL ;

    // Start at array limits
    Lo = 0 ;
    Hi = m_nShapes-1 ;

    // Loop until we get the closest type
    while(Lo < Hi)
    {
        // Lookup mid shape
        Mid = (Lo + Hi) >> 1 ;
        Shape = m_Shapes[Mid] ;
        ASSERT(Shape) ;

        // Chop search in half
        if (Shape->GetType() < Type)
            Lo = Mid+1 ;
        else
            Hi = Mid ;
    }

    // Found the shape?
    Shape = m_Shapes[Lo] ;
    ASSERT(Shape) ;
    if (Shape->GetType() == Type)
        return Shape ;
    else
        return NULL ;

    // Perform slow linear search for now...
    // Loop through all shapes
    //for (i = 0 ; i < m_nShapes ; i ++)
    //{
        //shape *Shape = m_Shapes[i] ;
        //ASSERT(Shape) ;

        // Found shape?
        //if (Shape->GetType() == Type)
            //return Shape ;
    //}

    // Not found
    return NULL ;
}

//==============================================================================

// Removes all shape from memory
void shape_manager::DeleteAllShapes()
{
    // Delete list of shapes
    for (s32 i = 0 ; i < m_nShapes ; i++)
        delete m_Shapes[i] ;

    // Reset total
    m_nShapes = 0 ;

    // Delete all textures
    m_TextureManager.DeleteAllTextures() ;
}

//==============================================================================

// Assigns material ID's and returns the next available ID
void shape_manager::AssignMaterialIDs(s32 ID, s32 DrawListID)
{
    // Record ID's incase another shape manager is used
    m_NextMaterialID         = ID ;
    m_NextMaterialDrawListID = DrawListID ;

    // Assign the IDs
    shape::AssignMaterialIDs(m_Shapes, m_nShapes, m_NextMaterialID, m_NextMaterialDrawListID) ;
}

//==============================================================================

// Returns the next available material ID to assign for other materials
s32 shape_manager::GetNextMaterialID( void )
{
    return m_NextMaterialID ;    
}

//==============================================================================

// Returns the next available material draw list ID to assign for other materials
s32 shape_manager::GetNextMaterialDrawListID( void )
{
    return m_NextMaterialDrawListID ;    
}

//==============================================================================
