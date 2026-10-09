#include <Support/Shape/Shape.hpp>
#include <Support/Shape/Util.hpp>
#include <Support/Shape/UserData.hpp>
#include <stdio.h>




//==============================================================================
// Defines
//==============================================================================

// Skel file sections
enum SkelSections
{
    NULL_SECTION,
    NODES_SECTION,
    VERTS_SECTION,
    POLYS_SECTION,
    SKIN_SECTION,
    TEXTURE_SECTION,
    MATERIAL_SECTION,
    STRIPS_SECTION,
    STRIP_VERTS_SECTION,
    ANIM_SECTION,

    USER_DATA_HOT_POINT_INFO_SECTION,
    USER_DATA_SHAPE_INFO_SECTION,
    USER_DATA_MODEL_INFO_SECTION,
    USER_DATA_NODE_INFO_SECTION,
    USER_DATA_ANIM_INFO_SECTION,
    USER_DATA_GUI_INFO_SECTION,
} ;



//==============================================================================
// Functions
//==============================================================================



//==============================================================================

// Just reads time stamp from skel file
xbool shape::ReadAsciiFileTimeStamp(const char *Filename, time_stamp &TimeStamp)
{
    TimeStamp.Clear() ; // constructs to all 0

	// Try open file
    X_FILE *File = x_fopen(Filename, "rt") ;  // read text mode
    if (File)
    {
        s32 Seconds=0, Minutes=0, Hours=0, Day=0, Month=0, Year=0 ;

        char Line[256], *CurrentVar ;

        // Read a single line
        x_fread(&Line, sizeof(char), 256, File) ;
        if (x_strncmp(Line, "[TimeStamp:",11) == 0)
        {
            // Parse
            CurrentVar = &Line[11] ;
            UtilRead(CurrentVar, &Seconds) ;
            UtilRead(CurrentVar, &Minutes) ;
            UtilRead(CurrentVar, &Hours) ;
            UtilRead(CurrentVar, &Day) ;
            UtilRead(CurrentVar, &Month) ;
            UtilRead(CurrentVar, &Year) ;
            TimeStamp.Seconds = Seconds ;
            TimeStamp.Minutes = Minutes ;
            TimeStamp.Hours   = Hours ;
            TimeStamp.Day     = Day ;
            TimeStamp.Month   = Month ;
            TimeStamp.Year    = Year ;
        }
        
        // Close the file
        x_fclose(File) ;

		// Success
		return TRUE ;
    }                

    // Fail
	return FALSE ;
}

//==============================================================================

// Straight load from a binary file
xbool shape::LoadFromBinaryFile(const char*                 ShapeFile, 
                                const shape_load_settings&  LoadSettings)
{
    // Split up filename
    char DRIVE  [X_MAX_DRIVE];
    char DIR    [X_MAX_DIR];
    char FNAME  [X_MAX_FNAME];
    char EXT    [X_MAX_EXT];
    char SHPPath[X_MAX_PATH];

    ASSERT(ShapeFile) ;

	// Grab shape file
	x_splitpath(ShapeFile, DRIVE, DIR, FNAME, EXT) ;
        
	// Build the shp path?
    if (shape::Get_SHP_Path())
        x_makepath( SHPPath, NULL, shape::Get_SHP_Path(), FNAME, ".shp");
    else
        x_makepath( SHPPath, DRIVE, DIR, FNAME, ".shp");

    // Read binary
	if (ReadBinaryFile(SHPPath, LoadSettings))
	{
        /* DEDICATED_SERVER
        // Load the textures
        LoadTextures(LoadSettings) ;
        DEDICATED_SERVER */

        return TRUE ;
    }
	
	// If you assert here - you need to rebuild the SHP file(s)

	// In your call to shape_manager::AddAndLoadShapes, make sure you use
	// shape_manager::MODE_BUILD_BINARY as the mode or use the shape builder tool

	ASSERTS(0, ".shp file is corrupt/out of date - delete it and rebuild!\n") ;

	// Fail
	return FALSE ;
}

//==============================================================================

// Load shape, models, and anims, from .skel files
// NOTE: The file names lists must be NULL terminated
xbool shape::LoadFromAsciiFiles(const char**	            ModelFiles, 
                                const char**	            AnimFiles, 
                                const shape_load_settings&  LoadSettings,
                                label_set*		            LabelSet,
								const char*		            ShapeFile)
{
    s32 i,j ;

    // Split up filename
    char DRIVE  [X_MAX_DRIVE];
    char DIR    [X_MAX_DIR];
    char FNAME  [X_MAX_FNAME];
    char EXT    [X_MAX_EXT];
    char SHPPath[X_MAX_PATH];
    x_splitpath(ModelFiles[0], DRIVE,DIR,FNAME,EXT);

    // Count the number of models to load
    s32 nModelFiles = 0 ;
    while(ModelFiles[nModelFiles])
        nModelFiles++ ;

    // Count the numbr of anims
    s32 nAnimFiles = 0 ;
    while(AnimFiles[nAnimFiles])
        nAnimFiles++ ;

    // Build SHP path?
    if (ShapeFile)
    {
        // If it asserts here, you must call the global function
        // "shape::SetDataOutputPaths(const char *SHP_Path, const char *XBMP_Path)"
        // when your app initializes!
        ASSERT(shape::Get_SHP_Path()) ;

		// Grab shape file
		x_splitpath(ShapeFile, NULL, NULL, FNAME, EXT) ;
        
		// Build the shp path
        if (shape::Get_SHP_Path())
            x_makepath( SHPPath, NULL, shape::Get_SHP_Path(), FNAME, ".shp");
        else
            x_makepath( SHPPath, DRIVE, DIR, FNAME, ".shp");
    }

    // Check for binary SHP file
    if (ShapeFile)
    {
        // Read binary?
        if (ReadBinaryFile(SHPPath, LoadSettings))
        {
            s32 BinAnimFileCount = GetAnimFileCount() ;
            s32 BinModelFileCount = GetModelFileCount() ;

            // Loaded all models and anims?
            if (       (m_nModels == nModelFiles)   // only 1 model per .skel file
                    && (m_nAnims  >= nAnimFiles)    // .skel file may include more than one anim!
                    && (nAnimFiles == BinAnimFileCount)
                    && (nModelFiles == BinModelFileCount) )
            {
                /* DEDICATED_SERVER
                // Load the textures
                LoadTextures(LoadSettings) ;
                DEDICATED_SERVER */
        
                return TRUE ;
            }
            else
            {
                // Show info
                shape::printf("BinShape %s doesn't contain all skel files... rebuiling...\n", SHPPath) ;
            }
        }
    }

    // Get ready for writing binary file - this has to be done so that the memory shape_bin_file
    // memory allocator keeps track of what memory is new'd/delete'd ready for reading the binary file
    shape_bin_file  BinFile ;
    xbool           BinFileOpened = FALSE ;
    if (ShapeFile)
        BinFileOpened = BinFile.OpenForWriting(SHPPath) ;

    // Allocate the models
    m_nModels = nModelFiles  ;
    if (m_nModels)
    {
        m_Models  = new model[m_nModels] ;
        ASSERT(m_Models) ;
    }

    // Allocate the anims
    m_nAnims  = nAnimFiles ;
    if (m_nAnims)
    {
        m_Anims   = new anim[m_nAnims] ;
        ASSERT(m_Anims) ;
    }
    
    // Speedup - loading from 1 file?
    if ( (nModelFiles == 1) && (nAnimFiles == 1) && (x_strcmp(ModelFiles[0], AnimFiles[0]) == 0) ) 
        ReadAsciiFile(ModelFiles[0], &m_Models[0], &m_Anims[0], LabelSet) ;
    else
    {
        // Load the models
        for (i = 0 ; i < nModelFiles ; i++)
            ReadAsciiFile(ModelFiles[i], &m_Models[i], NULL, LabelSet) ;
    
        // Load the anims
        for (i = 0 ; i < nAnimFiles ; i++)
            ReadAsciiFile(AnimFiles[i], NULL, &m_Anims[i], LabelSet) ;
    }

    // Validate anims
    for (i = 0 ; i < nAnimFiles ; i++)
    {
        // Get anim
        anim& Anim = m_Anims[i] ;

        // If no anim was read, setup a default single frame anim so we can continue...
        if (Anim.GetFrameCount() == 0)
        {
            // Set frame count
            Anim.SetFrameCount(1) ;

            // Setup key sets
            Anim.SetKeySetCount(m_Models[0].GetNodeCount()) ;                
            for (j = 0 ; j < Anim.GetKeySetCount() ; j++)
            {
                // Set single pos key
                Anim.GetKeySet(j).SetPosKeyCount(1) ;
                Anim.GetKeySet(j).SetPosKey(0, vector3(0,0,0)) ;

                // Set single rot key
                Anim.GetKeySet(j).SetRotKeyCount(1) ;
                Anim.GetKeySet(j).SetRotKey(0, quaternion(0,0,0,1)) ;

                // Set single vis key
                Anim.GetKeySet(j).SetVisKeyCount(1) ;
                Anim.GetKeySet(j).SetVisKey(0, 1.0f) ;
            }                
        }
    }

    // Load the textures...
    // This must be done here so that we can calculate the world pixel size needed for mip-maps!
    /* DEDICATED_SERVER
    LoadTextures(LoadSettings) ;
    DEDICATED_SERVER */

    // Optimize out nodes that are not used!
    DeleteUnusedNodes() ;

    //=========================================================================
    // Sort anims by type for faster lookup
    //=========================================================================
    SortAnimsByType() ;

    //=========================================================================
    // Bero bake animations
    //=========================================================================
    
    // Remove root translation and convert additive anims
    for (i = 0 ; i < GetAnimCount() ; i++)
    {
        anim &Anim = GetAnim(i) ;

        // Setup correct index
        Anim.SetIndex(i) ;

        // Bake in removal of root translation?
        if (Anim.GetRemoveRootTransFlag())
        {
            model &Model = GetModel(0) ;

            // Loop through all model nodes
            ASSERT(Model.GetNodeCount() == Anim.GetKeySetCount()) ;
            for (j = 0 ; j < Model.GetNodeCount() ; j++)
            {
                if (Model.GetNode(j).GetParent() == -1)
                    Anim.GetKeySet(j).RemoveXZTranslation() ;
            }
        }

        // Bake in additive animation?
        if (Anim.GetAdditiveFlag())
        {
            // Search for requested reference anim to use when creating additive anims
            anim* RefAnim = GetAnimByType( Anim.GetAdditiveRefType() ) ;
            
            // If not found, try using an un-assigned anim
            if (!RefAnim)
                RefAnim = GetAnimByType(0) ;

            // Last resort - use self
            if (!RefAnim)
                RefAnim = &Anim ;

            // Convert
            ASSERT(RefAnim) ;
            Anim.ConvertToAdditive(*RefAnim) ;
        }
    }

    // Collapse keys of the same value (saves memory)
    for (i = 0 ; i < GetAnimCount() ; i++)
    {
        anim &Anim = GetAnim(i) ;
        Anim.CollapseKeys() ;
    }

    // Flag anims that have visiblity animation for faster playback
    for (i = 0 ; i < GetAnimCount() ; i++)
    {
        anim &Anim = GetAnim(i) ;
        for (j = 0 ; j < Anim.GetKeySetCount() ; j++)
        {
            if (Anim.GetKeySet(j).GetVisKeyCount())
                Anim.SetHasAnimatedVisFlag(TRUE) ;
        }
    }

    //=========================================================================
    // Shape optimization:
    // If shape has just one anim and it is static (ie. contains no movement)
    // then burn into geometry and free the memory used by the anim!
    //=========================================================================
    if ( (m_nAnims == 1) && (m_Anims[0].IsStatic()) )
    {
        anim& Anim = m_Anims[0] ;

        // Loop through all models
        for (i = 0 ; i < m_nModels ; i++)
        {
            model& Model = m_Models[i] ;

            // Setup matrices for this anim frame
            matrix4* NodeMatrices = Model.GetNodeMatrices(Anim, 0, NULL) ;

            // Now transform all verts into anim world space...
            for (j = 0 ; j < Model.GetVertCount() ; j++)
            {
                // Get vert
                vert& Vert = Model.GetVert(j) ;
                ASSERT(Vert.GetNode() >= 0) ;
                ASSERT(Vert.GetNode() < Model.GetNodeCount()) ;

                // Get local space info
                vector3 LocalPos  = Vert.GetPos() ;
                vector3 LocalNorm = Vert.GetNormal() ;

                // Transform into world space
                vector3 WorldPos  = NodeMatrices[Vert.GetNode()] * LocalPos ;
                vector3 WorldNorm = NodeMatrices[Vert.GetNode()].RotateVector(LocalNorm) ;
                WorldNorm.Normalize() ;

                // Burn into vert...
                Vert.SetPos(WorldPos) ;
                Vert.SetNormal(WorldNorm) ;
            }
        }

        // Finally - delete the anim...
        delete [] m_Anims ;
        m_Anims  = NULL ;
        m_nAnims = 0 ;
    }

    //=========================================================================
    // Now all anim processing is complete, we can compress the anims
    //=========================================================================
    shape::printf("\nCompressing anims...\n") ;
    for (i = 0 ; i < GetAnimCount() ; i++)
    {
        anim& Anim = m_Anims[i] ;

        // Only compress those that want it
        if ( (!Anim.GetAdditiveFlag()) && (Anim.GetCompressFlag()) )
            Anim.Compress() ;
    }
    shape::printf("\n\n") ;

    //=========================================================================
    // Prepare for using shape...
    //=========================================================================
    shape::printf("\nInitializing for use...\n") ;
    InitializeForDrawing(LoadSettings.Target) ;
    shape::printf("\n\n") ;
   
    //=========================================================================
    // Write the binary file now that we know what memory needs to be allocated
    //=========================================================================

    if ((ShapeFile) && (BinFileOpened))
    {
        // Show info
        shape::printf("Writing %s\n", SHPPath) ;

        // Write the binary file ready for loading next time!
        ReadWrite(BinFile) ;

        // Finally, close the binary file
        BinFile.Close() ;

        // Show info
        shape::printf("Finished writing %s\n\n", SHPPath) ;
    }

    //=========================================================================
    // Show final shape info
    //=========================================================================
    shape::printf("*************************************************************\n***\n") ;
    shape::printf("*** Finished processing %s\n", m_Name) ;
    shape::printf("***   MemoryUsed:%d bytes - %dK\n", MemoryUsed(), (1023+MemoryUsed())/1024) ;

    // Show anim memory used
    s32 TotalMem = 0 ;
    for (i = 0 ; i < m_nAnims ; i++)
        TotalMem += m_Anims[i].MemoryUsed() ;
    shape::printf("***     Anims:%d  AnimMemoryUsed :%d bytes - %dK\n", m_nAnims, TotalMem, (1023+TotalMem)/1024) ;

    // Show model memory used
    TotalMem = 0 ;
    for (i = 0 ; i < m_nModels ; i++)
        TotalMem += m_Models[i].MemoryUsed() ;
    shape::printf("***     Models:%d ModelMemoryUsed:%d bytes - %dK\n", m_nModels, TotalMem, (1023+TotalMem)/1024) ;

    shape::printf("***\n*************************************************************\n\n\n") ;

    // Done!
    return TRUE ;
}


//==============================================================================



// Load shape from ascii .SKEL file
xbool shape::ReadAsciiFile(const char *Filename, model *pModel, anim *pAnim, label_set *LabelSet)
{
#define MAX_NODES   1024
    u8          NodeHasScaleKeys[MAX_NODES] ;
    u8          NodeHasRotKeys  [MAX_NODES] ;
    u8          NodeHasPosKeys  [MAX_NODES] ;
    char        *FileBuffer ;
    s32         FileBufferPos, FileBufferLen ;
    char        *Line, *CurrentVar ;
    time_stamp  TimeStamp ;
    s32         AnimInfoCount = 0 ;
    anim        OriginalAnim ;

    // Get the time stamp of the file
    ReadAsciiFileTimeStamp(Filename, TimeStamp) ;

    // Keep file info for binary file writing/reading
    if (x_strlen(m_Name) == 0)
        SetName(Filename) ;

    if (pModel)
    {
        pModel->SetTimeStamp(TimeStamp) ;
        pModel->SetName(Filename) ;
    }

    if (pAnim)
    {
        pAnim->SetTimeStamp(TimeStamp) ;
        pAnim->SetName(Filename) ;
    }
   
	// Try open file
    X_FILE *File = x_fopen(Filename, "rt") ;  // read text mode

    // Fail?
	if (!File)
    {
        // Show error!
        shape::printf("Failed to open:\n%s\n\n", Filename);
        ASSERTS(0, "skel file does not exist!\n") ;
		return FALSE ;
    }

    // Show info
    shape::printf("Parsing %s\n", Filename) ;

    // Allocate buffer to read file in one go
    x_fseek( File, 0, X_SEEK_END );
    FileBufferLen = x_ftell(File);
    x_fseek( File, 0, X_SEEK_SET );
    FileBuffer = new char[FileBufferLen+1] ; // +1 for string terminator!
    if (!FileBuffer)
    {
        // Close file
        x_fclose(File) ;

        // Show error!
        shape::printf("Failed to load:\n%s - Not enough memory\n\n", Filename);
		return FALSE ;
    }

    // Read file in one go...
    FileBufferPos = 0 ;
    FileBufferLen = x_fread((void *)&FileBuffer[0], 1, FileBufferLen, File) ;

    // Make sure end of file is terminated since scanf on the pc does a strlen!
    FileBuffer[FileBufferLen+1] = 0 ;

	// Scan file line by line
	s32 Count, Section = NULL_SECTION ;
    s32 SkipLine = FALSE ;
    s32 LineNumber = 0 ;    // for debugging!
    while(FileBufferPos < FileBufferLen)
	{
        // Read line
        Line        = &FileBuffer[FileBufferPos] ;
        CurrentVar  = Line ;

        // Find end of line ready 
        while((FileBufferPos < FileBufferLen) && (FileBuffer[FileBufferPos] != '\n'))
            FileBufferPos++ ;
        
        // Skip line returns
        while(  (FileBufferPos < FileBufferLen) && 
                ((FileBuffer[FileBufferPos] == '\n') || (FileBuffer[FileBufferPos] == '\r')) )
            FileBufferPos++ ;
		
        // Make sure end of line is terminated since scanf on the pc does a strlen!
        if (FileBufferPos)
            FileBuffer[FileBufferPos-1] = 0 ;

        // Skip this line?
        if (SkipLine)
            SkipLine = FALSE ;
        else
        // End of section of file?
        if (Line[0] == ' ')
            Section = NULL_SECTION ;
        else        
        // Start of new section?
        if (Line[0] == '[')
        {
            // Incase it's not found
            Section  = NULL_SECTION ;

		    // Nodes?
		    if ( ((pModel) || (pAnim)) && (sscanf(Line,"[Hierarchy:%d]", &Count) == 1))
		    {
			    // Create new nodes
                shape::printf("   parsing nodes...\n") ;
			    Section	= NODES_SECTION ;
			    
                // Setup model?
                if (pModel)
                {
                    pModel->SetNodeCount(Count) ;
                    ASSERT(pModel->GetNodeCount() == Count) ;
                }
		    }

            // Vertices?
		    if ((pModel) && (sscanf(Line,"[Vertices:%d]", &Count) == 1))
		    {
			    // Create new verts
                shape::printf("   parsing verts...\n") ;
			    Section	= VERTS_SECTION ;

                pModel->SetVertCount(Count) ;
                ASSERT(pModel->GetVertCount() == Count) ;
		    }
	    
		    // Polygons?
		    if ((pModel) && (sscanf(Line,"[Polygons:%d]", &Count) == 1))
		    {
                shape::printf("   parsing polys...\n") ;
			    Section	= POLYS_SECTION ;

                pModel->SetPolyCount(Count) ;
                ASSERT(pModel->GetPolyCount() == Count) ;
		    }

            // Skin?
		    if ((pModel) && (sscanf(Line,"[Skin:%d]", &Count) == 1))
		    {
                shape::printf("   parsing skin...\n") ;
			    Section	= SKIN_SECTION ;
		    }

            // Textures?
            if ((pModel) && (sscanf(Line,"[Textures:%d]", &Count) == 1))
            {
                shape::printf("   parsing textures...\n") ;
                Section = TEXTURE_SECTION ;

                pModel->SetTextureRefCount(Count) ;
                ASSERT(pModel->GetTextureRefCount() == Count) ;
            }

            // Materials?
            if ((pModel) && (sscanf(Line,"[Materials:%d]", &Count) == 1))
            {
                shape::printf("   parsing materials...\n") ;
                Section = MATERIAL_SECTION ;

                pModel->SetMaterialCount(Count) ;
                ASSERT(pModel->GetMaterialCount() == Count) ;
            }

            // Strips?
            if ((pModel) && (sscanf(Line,"[Strips:%d]", &Count) == 1))
            {
                shape::printf("   parsing strips...\n") ;
                Section = STRIPS_SECTION ;
            }

            // Strip verts?
            if ((pModel) && (sscanf(Line,"[StripVertices:%d]", &Count) == 1))
            {
                shape::printf("   parsing strip verts...\n") ;
                Section = STRIP_VERTS_SECTION ;
            }

            // Anim?
            if ((pAnim) && (sscanf(Line,"[Animation:%d]", &Count) == 1))
            {
                shape::printf("   parsing anim...\n") ;
                Section = ANIM_SECTION ;

                // Set anim frame total
                pAnim->SetFrameCount(Count) ;
                ASSERT(Count > 0) ;

                // Allocate key sets (there is one key set per node)
                pAnim->SetKeySetCount(GetModel(0).GetNodeCount()) ;

                // Allocate key frames for all key sets
                for (s32 k = 0 ; k < pAnim->GetKeySetCount() ; k++)
                {
                    key_set &KeySet = pAnim->GetKeySet(k) ;

                    // Alloc pos keys
                    if (NodeHasPosKeys[k])
                        KeySet.SetPosKeyCount(Count) ;
                    else
                        KeySet.SetPosKeyCount(1) ;

                    // Alloc rot keys
                    if (NodeHasRotKeys[k])
                        KeySet.SetRotKeyCount(Count) ;
                    else
                        KeySet.SetRotKeyCount(1) ;

                    // Alloc vis keys
                    KeySet.SetVisKeyCount(Count) ;
                }
            }
            
            //-----------------------------------------------------------------------------
            // User data
            //-----------------------------------------------------------------------------

            // HotPointInfo?
            if ((pModel) && (sscanf(Line,"[HotPointInfo:%d]", &Count) == 1))
            {
                Section = USER_DATA_HOT_POINT_INFO_SECTION ;

                // Allocate hot points
                pModel->SetHotPointCount(Count) ;
                ASSERT(pModel->GetHotPointCount() == Count) ;
            }

            // ShapeInfo?
            if (sscanf(Line,"[ShapeInfo:%d]", &Count) == 1)
                Section = USER_DATA_SHAPE_INFO_SECTION ;

            // ModelInfo?
            if ((pModel) && (sscanf(Line,"[ModelInfo:%d]", &Count) == 1))
                Section = USER_DATA_MODEL_INFO_SECTION ;

            // NodeInfo?
            if ( ((pModel) || (pAnim)) && (sscanf(Line,"[NodeInfo:%d]", &Count) == 1))
                Section = USER_DATA_NODE_INFO_SECTION ;

            // AnimInfo?
            if ((pAnim) && (sscanf(Line,"[AnimInfo:%d]", &Count) == 1))
            {
                AnimInfoCount = Count ;
                ASSERT(Count > 0) ;
                Section       = USER_DATA_ANIM_INFO_SECTION ;

                // Create a copy of the original anim, ready to make sub-copies of it
                // (see code for USER_DATA_ANIM_INFO_SECTION)
                OriginalAnim = *pAnim ;
            }

            // GUIInfo?
            if ((pModel) && (sscanf(Line,"[GUIInfo:%d]", &Count) == 1))
                Section = USER_DATA_GUI_INFO_SECTION ;

            // Skip next line if section is found
            SkipLine = (Section != NULL_SECTION) ;
        }
        else
		// Process section data
		switch(Section)
		{
			case NODES_SECTION:
                if ( (pModel) || (pAnim) )
                {
                    s32         i=-1,c,p ;
                    char        name[256] ;
                    vector3     Scale=vector3(0,0,0), Pos=vector3(0,0,0) ;
                    quaternion  Rot=quaternion(0,0,0,1) ;
                    s32         HasScaleKeys=1, HasRotKeys=1, HasPosKeys=1 ;

                    // Read Index Name #Children Parent ScaleXYZ RotXYZT PosXYZ HasScaleKeys HasRotKeys HasPosKeys
                    UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &name[0]) ;
                    UtilRead(CurrentVar, &c) ;
                    UtilRead(CurrentVar, &p) ;
                    UtilRead(CurrentVar, &Scale.X) ; UtilRead(CurrentVar, &Scale.Y) ; UtilRead(CurrentVar, &Scale.Z) ;
                    UtilRead(CurrentVar, &Rot.X) ;   UtilRead(CurrentVar, &Rot.Y) ;   UtilRead(CurrentVar, &Rot.Z) ; UtilRead(CurrentVar, &Rot.W) ;
                    UtilRead(CurrentVar, &Pos.X) ;   UtilRead(CurrentVar, &Pos.Y) ;   UtilRead(CurrentVar, &Pos.Z) ;
                    UtilRead(CurrentVar, &HasScaleKeys) ; UtilRead(CurrentVar, &HasRotKeys) ; UtilRead(CurrentVar, &HasPosKeys) ;

                    // Setup model?
                    if (pModel)
                    {
                        ASSERT( (i >= 0) && (i < pModel->GetNodeCount()) ) ;
                        ASSERT(i < MAX_NODES) ;
                        ASSERT(c >= 0) ;
                        ASSERT(p < pModel->GetNodeCount()) ;
                        ASSERT(p < i) ;

                        node &Node = pModel->GetNode(i) ;
                        Node.SetParent(p) ;
                        //Node.SetBindScale(Scale) ;
                        //Node.SetBindRot(Rot) ;
                        //Node.SetBindPos(Pos) ;

                        // Special case tribes turret fix - do not delete nodes with "bonedum" in their name!
                        x_strtolower(name) ;
                        if (x_strstr(name, "bonedum"))
                            Node.SetDoNotDelete(TRUE) ;
                    }

                    // Keep copy of key flags for setting up anim
                    NodeHasScaleKeys[i] = HasScaleKeys ;
                    NodeHasRotKeys[i]   = HasRotKeys ;
                    NodeHasPosKeys[i]   = HasPosKeys ;
                }
                break ;

			case VERTS_SECTION:

				if ( (pModel) && (pModel->GetVertCount()) )
				{
					s32     i=-1,w=0;
					vector3 Pos=vector3(0,0,0), Normal=vector3(0,0,0) ;
                    f32     u=0,v=0 ;

                    // Read Index x y z nx ny nz u v weights
					UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &Pos.X) ;
                    UtilRead(CurrentVar, &Pos.Y) ;
                    UtilRead(CurrentVar, &Pos.Z) ;
                    UtilRead(CurrentVar, &Normal.X) ;
                    UtilRead(CurrentVar, &Normal.Y) ;
                    UtilRead(CurrentVar, &Normal.Z) ;
                    UtilRead(CurrentVar, &u) ;
                    UtilRead(CurrentVar, &v) ;
                    UtilRead(CurrentVar, &w) ;

                    // Write to vert
                    ASSERT( (i >= 0) && (i < pModel->GetVertCount()) ) ;

                    vert &Vert = pModel->GetVert(i) ;
					Vert.SetPos(Pos) ;
					Vert.SetNode(0) ;
                    Vert.SetNormal(Normal) ;
                    Vert.SetTexCoord(vector2(u, v)) ;
				}
				break ;

			case POLYS_SECTION:

				if ( (pModel) && (pModel->GetPolyCount()) )
				{
					s32 i=-1, a=-1,b=-1,c=-1, mat=-1 ;
					vector3 Normal=vector3(0,0,0) ;

					// Read Index A B C nx ny nz Material
					UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &a) ;
                    UtilRead(CurrentVar, &b) ;
                    UtilRead(CurrentVar, &c) ;
                    UtilRead(CurrentVar, &Normal.X) ;
                    UtilRead(CurrentVar, &Normal.Y) ;
                    UtilRead(CurrentVar, &Normal.Z) ;
                    UtilRead(CurrentVar, &mat) ;

                    // Write to poly
                    ASSERT( (i >= 0) && (i < pModel->GetPolyCount()) ) ;
                    ASSERT( (a >= 0) && (a < pModel->GetVertCount()) ) ;
                    ASSERT( (b >= 0) && (b < pModel->GetVertCount()) ) ;
                    ASSERT( (c >= 0) && (c < pModel->GetVertCount()) ) ;
                    ASSERT(mat >= 0) ;

                    poly &Poly = pModel->GetPoly(i) ;
                    Poly.SetVert(0, a) ;
                    Poly.SetVert(1, b) ;
                    Poly.SetVert(2, c) ;
                    Poly.SetMaterial(mat) ;
				}
				break ;
            
            case SKIN_SECTION:
				if ( (pModel) && (pModel->GetVertCount()) )
				{
					s32 v=-1,i=-1,b=-1 ;
					f32 w=0 ;

					// Read Vertex Index Node Weight
					UtilRead(CurrentVar, &v) ;
                    UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &b) ;
                    UtilRead(CurrentVar, &w) ;

                    // Write
					ASSERT( (v >= 0) && (v < pModel->GetVertCount()) ) ;
					//ASSERT((i >= 0) && (i < Verts[v].m_nWeights)) ;
					ASSERT( (b >=0 ) && (b < pModel->GetNodeCount()) ) ;

                    // Rigid skinning for now...
                    vert &Vert = pModel->GetVert(v) ;
					Vert.SetNode(b) ;
				}
				break ;

            case TEXTURE_SECTION:
                if ( (pModel) && (pModel->GetTextureRefCount()) )
                {
                    s32 i=-1;
                    char name[X_MAX_FNAME] ;

                    // Read Index Name
					UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &name[0]) ;

                    // Write
					ASSERT( (i >= 0) && (i < pModel->GetTextureRefCount()) ) ;

                    // Convert question marks to spaces
                    for (int j = 0 ; j < x_strlen(name) ; j++)
                    {
                        if (name[j] == '?')
                            name[j] = ' ' ;
                    }

                    // Setup texture reference
                    shape_texture_ref &TexRef = pModel->GetTextureRef(i) ;
                    TexRef.SetName(name) ;
                }
                break ;

            case MATERIAL_SECTION:

				if ( (pModel) && (pModel->GetMaterialCount()) )
				{
					s32 i=-1 ;
					char Name[256] ;
					s32 Strips=-1, DiffuseTex=-1, ReflectTex=-1, AlphaTex=-1, SpecTex=-1, BumpTex=-1, DispTex=-1 ;
                    f32 r=0, g=0,b=0, a=0 ;
					s32 UMirror=0, VMirror=0, UWrap=0, VWrap=0, SelfIllum=0, DoubleSided=0, Additive=0 ;

                    // Read Index Mat Strips DiffuseTex ReflectTex AlphaTex SpecTex BumpTex DispTex r g b a UMirror VMirror UWrap VWrap SelfIllum DoubleSided Additive
                    UtilRead(CurrentVar, &i) ; 
                    UtilRead(CurrentVar, &Name[0]) ;
                    UtilRead(CurrentVar, &Strips) ;
                    UtilRead(CurrentVar, &DiffuseTex) ;
                    UtilRead(CurrentVar, &ReflectTex) ;
                    UtilRead(CurrentVar, &AlphaTex) ;
                    UtilRead(CurrentVar, &SpecTex) ;
                    UtilRead(CurrentVar, &BumpTex) ;
                    UtilRead(CurrentVar, &DispTex) ;
                    UtilRead(CurrentVar, &r) ;
                    UtilRead(CurrentVar, &g) ;
                    UtilRead(CurrentVar, &b) ;
                    UtilRead(CurrentVar, &a) ;
                    UtilRead(CurrentVar, &UMirror) ;
                    UtilRead(CurrentVar, &VMirror) ;
                    UtilRead(CurrentVar, &UWrap) ;
                    UtilRead(CurrentVar, &VWrap) ;
                    UtilRead(CurrentVar, &SelfIllum) ;
                    UtilRead(CurrentVar, &DoubleSided) ;
                    UtilRead(CurrentVar, &Additive) ;

                    // Check values
                    ASSERT( (i >= 0) && (i < pModel->GetMaterialCount()) ) ;
                    ASSERT( Strips != -1 ) ;
                    ASSERT( (DiffuseTex >= -1) && (DiffuseTex < pModel->GetTextureRefCount()) ) ;
                    ASSERT( (ReflectTex >= -1) && (ReflectTex < pModel->GetTextureRefCount()) ) ;
                    ASSERT( (AlphaTex   >= -1) && (AlphaTex   < pModel->GetTextureRefCount()) ) ;
                    ASSERT( (SpecTex    >= -1) && (SpecTex    < pModel->GetTextureRefCount()) ) ;
                    ASSERT( (BumpTex    >= -1) && (BumpTex    < pModel->GetTextureRefCount()) ) ;
                    ASSERT( (DispTex    >= -1) && (DispTex    < pModel->GetTextureRefCount()) ) ;
                   
                    // Write to material
                    material &Material = pModel->GetMaterial(i) ;
                    Material.SetName(Name) ;
                    Material.SetStripCount(Strips) ;
                    Material.SetTextureRef(material::TEXTURE_TYPE_BUMP,    BumpTex) ;
                    Material.SetTextureRef(material::TEXTURE_TYPE_DIFFUSE, DiffuseTex) ;
                    Material.SetTextureRef(material::TEXTURE_TYPE_REFLECT, ReflectTex) ;
                    Material.SetTextureRef(material::TEXTURE_TYPE_ALPHA,   AlphaTex) ;
                    Material.SetColor(vector4(r,g,b,a)) ;
                    Material.SetUMirrorFlag(UMirror) ;
                    Material.SetVMirrorFlag(VMirror) ;
                    Material.SetUWrapFlag(UWrap) ;
                    Material.SetVWrapFlag(VWrap) ;
                    Material.SetSelfIllumFlag(SelfIllum) ;
                    Material.SetAdditiveFlag(Additive) ;
                }
				break ;

            case STRIPS_SECTION:

                if ( (pModel) && (pModel->GetMaterialCount()) )
                {
                    s32 MatIndex=-1, StripIndex=-1, nVerts=0, NodeIndex=-1, Orient=0 ;

                    // MaterialIndex StripIndex #Verts Node Orient
                    UtilRead(CurrentVar, &MatIndex) ;
                    UtilRead(CurrentVar, &StripIndex) ;
                    UtilRead(CurrentVar, &nVerts) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &Orient) ;

                    // Write
                    ASSERT( (MatIndex >= 0) && (MatIndex < pModel->GetMaterialCount()) ) ;
                    ASSERT( (StripIndex >= 0) && (StripIndex < pModel->GetMaterial(MatIndex).GetStripCount())) ;

                    // Setup strip
                    strip &Strip = pModel->GetMaterial(MatIndex).GetStrip(StripIndex) ;

                    Strip.SetVertCount(nVerts) ;
                    Strip.SetOrient(Orient) ;
                    Strip.SetNode(NodeIndex) ;
                }
                break ;

            case STRIP_VERTS_SECTION:

                if ( (pModel) && (pModel->GetMaterialCount()) )
                {
                    s32 MatIndex=-1, StripIndex=-1, StripVertIndex=-1, VertIndex=-1, NoKick=0, Orient=0 ;

                    // Read MaterialIndex StripIndex VertIndex Vert
                    UtilRead(CurrentVar, &MatIndex) ;
                    UtilRead(CurrentVar, &StripIndex) ;
                    UtilRead(CurrentVar, &StripVertIndex) ;
                    UtilRead(CurrentVar, &VertIndex) ;

                    // Get strip
                    strip& Strip = pModel->GetMaterial(MatIndex).GetStrip(StripIndex) ;

                    // Default NoKick incase it's not in the ascii file
                    NoKick = 0 ;
                    UtilRead(CurrentVar, &NoKick) ;

                    // All verts in start of strip are no kick
                    NoKick |= (StripVertIndex < 2) ;

                    // Default orient to strips incase it's not in the ascii file
                    Orient = Strip.GetOrient() ^ (StripVertIndex & 1) ;
                    UtilRead(CurrentVar, &Orient) ;

                    // Write
                    ASSERT( (MatIndex >= 0)          && (MatIndex        < pModel->GetMaterialCount()) ) ;
                    ASSERT( (StripIndex >= 0)        && (StripIndex      < pModel->GetMaterial(MatIndex).GetStripCount()) ) ;
                    ASSERT( (StripVertIndex >= 0)    && (StripVertIndex  < pModel->GetMaterial(MatIndex).GetStrip(StripIndex).GetVertCount()) ) ;
                    ASSERT( (VertIndex >= 0)         && (VertIndex       < pModel->GetVertCount()) ) ;

                    // If strip has no node, assign it one from the first vert (old version support)
                    if (Strip.GetNode() == -1)
                        Strip.SetNode(pModel->GetVert(VertIndex).GetNode()) ;

                    // Setup strip vert
                    Strip.SetVert(StripVertIndex, VertIndex, NoKick, Orient) ;
                }
                break ;

            case ANIM_SECTION:
				if ( (pAnim) &&  (GetModel(0).GetNodeCount()) )
				{
                    // If this assert, the hierarchy of your max anim and max model files do not
                    // match - fix it!!!
                    ASSERTS(pAnim->GetKeySetCount() == GetModel(0).GetNodeCount(), "Heirarchy of anim file does not match that of model file!") ;

					s32 f=-1;
					char name[256] ;
					s32 i=-1 ;
					vector3 Scale=vector3(0,0,0), Pos=vector3(0,0,0) ;
                    quaternion Rot=quaternion(0,0,0,1) ;
                    f32 Vis=1.0f ;

                    // Read Frame NodeName NodeIndex ScaleXYZ RotXYZA PosXYZ Visiblity
                    UtilRead(CurrentVar, &f) ;
                    UtilRead(CurrentVar, &name[0]) ;
                    UtilRead(CurrentVar, &i) ;
                    UtilRead(CurrentVar, &Scale.X) ; UtilRead(CurrentVar, &Scale.Y) ; UtilRead(CurrentVar, &Scale.Z) ;
                    UtilRead(CurrentVar, &Rot.X) ;   UtilRead(CurrentVar, &Rot.Y) ;   UtilRead(CurrentVar, &Rot.Z) ; UtilRead(CurrentVar, &Rot.W) ;
                    UtilRead(CurrentVar, &Pos.X) ;   UtilRead(CurrentVar, &Pos.Y) ;   UtilRead(CurrentVar, &Pos.Z) ;
                    UtilRead(CurrentVar, &Vis) ;
                   
                    // Write
					ASSERT( (f >= 0) && (f < pAnim->GetFrameCount()) ) ;
					ASSERT( (i >= 0) && (i < GetModel(0).GetNodeCount()) ) ;

					// Store anim key frame
                    key_set &KeySet = pAnim->GetKeySet(i) ;

                    // Set pos key
                    if (f < KeySet.GetPosKeyCount()) 
                        KeySet.SetPosKey(f, Pos) ;
                    
                    // Set rot key
                    if (f < KeySet.GetRotKeyCount()) 
                        KeySet.SetRotKey(f, Rot) ;

                    // Set vis key
                    if (f < KeySet.GetVisKeyCount()) 
                        KeySet.SetVisKey(f, Vis) ;
				}
				break ;

            case USER_DATA_HOT_POINT_INFO_SECTION:
                if (pModel)
                {
                    s32         Index=-1, NodeIndex=-1 ;
                    vector3     Pos=vector3(0,0,0) ;
                    quaternion  Rot=quaternion(0,0,0,1) ;
                    char        TypeLabel[256] ;

                    // Read Index NodeIndex x y z rx ry rz rw Type
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &Pos.X) ; UtilRead(CurrentVar, &Pos.Y) ;  UtilRead(CurrentVar, &Pos.Z) ;
                    UtilRead(CurrentVar, &Rot.X) ; UtilRead(CurrentVar, &Rot.Y) ;  UtilRead(CurrentVar, &Rot.Z) ;  UtilRead(CurrentVar, &Rot.W) ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;
        
                    // Setup hot point
                    ASSERT( (Index >= -1) && (Index < pModel->GetHotPointCount()) ) ;
                    ASSERT( (NodeIndex >= -1) && (NodeIndex < pModel->GetNodeCount()) ) ;
                    
                    if (LabelSet)
                    {
                        hot_point &HotPoint = pModel->GetHotPoint(Index) ;
                        
                        HotPoint.SetNode(NodeIndex) ;
                        HotPoint.SetPos(Pos) ;
                        HotPoint.SetDir(Rot) ;
                        HotPoint.SetType(LabelSet->FindLabelValue(TypeLabel)) ;
                    }
                }
                break ;

            case USER_DATA_SHAPE_INFO_SECTION:
                {
                    s32 Index=-1, NodeIndex=-1 ;
                    char TypeLabel[256] ;

                    // Read Index NodeIndex Type
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;

                    if (LabelSet)
                        m_Type = LabelSet->FindLabelValue(TypeLabel) ;
                }
                break ;

            case USER_DATA_MODEL_INFO_SECTION:
                if (pModel)
                {
                    s32 Index=-1, NodeIndex=-1 ;
                    char TypeLabel[256] ;
                    f32 Weight=0 ;

                    // Read Index NodeIndex Type Weight
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;
                    UtilRead(CurrentVar, &Weight) ;

                    ASSERT( (NodeIndex >= -1) && (NodeIndex < pModel->GetNodeCount()) ) ;
                    
                    if (LabelSet)
                    {
                        pModel->SetType(LabelSet->FindLabelValue(TypeLabel)) ;
                        pModel->SetWeight(Weight) ;
                    }
                }
                break ;

            case USER_DATA_NODE_INFO_SECTION:
                if ((pModel) || (pAnim))
                {
                    s32 Index=-1, NodeIndex=-1, NeverAnimate=FALSE, DoNotDelete=FALSE ;
					f32 AnimWeight = 1.0f ;

					char TypeLabel[256] ;
					
                    // Read Index NodeIndex Type
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;
					UtilRead(CurrentVar, &NeverAnimate) ;
					UtilRead(CurrentVar, &AnimWeight) ;
					UtilRead(CurrentVar, &DoNotDelete) ;

                    // Only process if node index is valid
                    if (NodeIndex != -1)
                    {
					    // Assign model node?
					    if (pModel)
					    {
	                        ASSERT( (NodeIndex >= 0) && (NodeIndex < pModel->GetNodeCount()) ) ;

						    if (LabelSet)
						    {
	                            node &Node = pModel->GetNode(NodeIndex) ;
							    Node.SetType(LabelSet->FindLabelValue(TypeLabel)) ;
                                if (DoNotDelete)
                                    Node.SetDoNotDelete(DoNotDelete) ;
						    }
					    }

					    // Assign animation properties
					    if (pAnim)
					    {
						    ASSERT(GetAnimCount() >= 1) ;
						    ASSERT(GetModelCount()) ;
	                        ASSERT( (NodeIndex >= 0) && (NodeIndex < pAnim->GetKeySetCount()) ) ;

						    // Get key sets
						    key_set &RootKeySet	= GetAnim(0).GetKeySet(NodeIndex) ;
						    key_set &KeySet = pAnim->GetKeySet(NodeIndex) ;

						    // Turn off animation for this node?
						    if (NeverAnimate)
						    {
							    // Replace keys with root key set
							    KeySet = RootKeySet ;

							    // Set never animate flag
							    KeySet.SetNeverAnimateFlag(TRUE) ;
						    }

						    // Set anim weight (used in additive and masked anim playback)
						    KeySet.SetAnimWeight(AnimWeight) ;
					    }
                    }
                }
                break ;


            case USER_DATA_ANIM_INFO_SECTION:

                if ( (pAnim) && (OriginalAnim.GetFrameCount()) )
                {
                    s32 Index=-1, NodeIndex=-1 ;
                    char TypeLabel[256] ;
                    user_data_anim_info AnimInfo ;
                    char AddRefTypeLabel[256] ;

                    // Setup defaults
                    AnimInfo.Type = 0 ;                             // Animation type - see AnimTypes.hpp
                    AnimInfo.Weight = 1.0f ;                        // Weight used when selecting anim
                    AnimInfo.Fps = 30.0f ;                          // Playback rate in frames per second
                    AnimInfo.StartFrame = 0 ;                       // Start frame of animation
                    AnimInfo.EndFrame = pAnim->GetFrameCount() -1 ; // End frame of animation
                    AnimInfo.LoopToFrame = 0 ;                      // Frame to jump to after playing
                    AnimInfo.RemoveRootTrans = 0 ;                  // Remove root translation flag
                    AnimInfo.Additive = 0;                          // Additive anim flag
                    AnimInfo.Compress = 1;                          // Compress flag
                    AnimInfo.AdditiveRefType = 0 ;                  // Reference animation type for additive anims

                    // Index NoneIndex Type Weight Fps StartFrame EndFrame LoopToFrame RemoveRoot Additive Compress AddRefType
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;

                    TypeLabel[0] = 0 ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;
                    
                    UtilRead(CurrentVar, &AnimInfo.Weight) ;
                    UtilRead(CurrentVar, &AnimInfo.Fps) ;
                    UtilRead(CurrentVar, &AnimInfo.StartFrame) ;
                    UtilRead(CurrentVar, &AnimInfo.EndFrame) ;
                    UtilRead(CurrentVar, &AnimInfo.LoopToFrame) ;
                    UtilRead(CurrentVar, &AnimInfo.RemoveRootTrans) ;
                    UtilRead(CurrentVar, &AnimInfo.Additive) ;
                    UtilRead(CurrentVar, &AnimInfo.Compress) ;

                    AddRefTypeLabel[0] = 0 ;
                    UtilRead(CurrentVar, &AddRefTypeLabel[0]) ;

                    // Keep frame indices within range incase of wrong user data
                    ASSERTS(OriginalAnim.GetFrameCount(), "Animation is missing from max file - re-export!") ;

                    AnimInfo.StartFrame  = MAX(AnimInfo.StartFrame, 0) ;
                    AnimInfo.StartFrame  = MIN(AnimInfo.StartFrame, OriginalAnim.GetFrameCount()-1) ;

                    AnimInfo.EndFrame    = MAX(AnimInfo.EndFrame, 0) ;
                    AnimInfo.EndFrame    = MIN(AnimInfo.EndFrame, OriginalAnim.GetFrameCount()-1) ;

                    AnimInfo.LoopToFrame = MAX(AnimInfo.LoopToFrame, MIN(AnimInfo.StartFrame, AnimInfo.EndFrame)) ;
                    AnimInfo.LoopToFrame = MIN(AnimInfo.LoopToFrame, MAX(AnimInfo.StartFrame, AnimInfo.EndFrame)) ;

                    // Convert label to type
                    if (LabelSet)
                    {
                        s32 LabelIndex ;

                        // Lookup type label
                        if (TypeLabel[0])
                        {
                            LabelIndex = LabelSet->FindLabel(TypeLabel) ;
                            if (LabelIndex != -1)
                            {
                                label &Label = LabelSet->GetLabel(LabelIndex) ;
                                AnimInfo.Type = Label.GetValue() ;
                            }
                        }

                        // Lookup additive reference type label
                        if (AddRefTypeLabel[0])
                        {
                            LabelIndex = LabelSet->FindLabel(AddRefTypeLabel) ;
                            if (LabelIndex != -1)
                            {
                                label &Label = LabelSet->GetLabel(LabelIndex) ;
                                AnimInfo.AdditiveRefType = Label.GetValue() ;
                            }
                        }
                    }
                    
                    // Create a new anim which is a portion of the original anim
                    anim SubAnim(OriginalAnim, AnimInfo) ;

                    // Use original anim slot for the first sub-anim
                    if (Index == 0)
                    {
                        ASSERT(pAnim != &OriginalAnim) ;
                        *pAnim = SubAnim ;
                    }
                    else
                    {
                        // Add to end of list
                        AddAnim(SubAnim) ;
					}

	                // Since add anim re-allocates the anim array, pAnim will become invalid,
		            // so make it use the original anim (which is always valid)
					pAnim = &OriginalAnim ;
                }
                break ;

            case USER_DATA_GUI_INFO_SECTION:
                if (pModel)
                {
                    s32 Index=-1, NodeIndex=-1 ;
                    char TypeLabel[256], HitLeftLabel[256], HitRightLabel[256], HitUpLabel[256], HitDownLabel[256] ;
                    
                    // Read Index NodeIndex Type HitLeft HitRight HitUp HitDown
                    UtilRead(CurrentVar, &Index) ;
                    UtilRead(CurrentVar, &NodeIndex) ;
                    UtilRead(CurrentVar, &TypeLabel[0]) ;
                    UtilRead(CurrentVar, &HitLeftLabel[0]) ;
                    UtilRead(CurrentVar, &HitRightLabel[0]) ;
                    UtilRead(CurrentVar, &HitUpLabel[0]) ;
                    UtilRead(CurrentVar, &HitDownLabel[0]) ;

                    ASSERT( (NodeIndex >= -1) && (NodeIndex < pModel->GetNodeCount()) ) ;

                    if (LabelSet)
                    {
                        // Grab user data
                        user_data_gui_info  GUIInfo ;
                        GUIInfo.Type     = LabelSet->FindLabelValue(TypeLabel) ;
                        GUIInfo.HitLeft  = LabelSet->FindLabelValue(HitLeftLabel) ;
                        GUIInfo.HitRight = LabelSet->FindLabelValue(HitRightLabel) ;
                        GUIInfo.HitUp    = LabelSet->FindLabelValue(HitUpLabel) ;
                        GUIInfo.HitDown  = LabelSet->FindLabelValue(HitDownLabel) ;

                        // Attach to node
                        node &Node = pModel->GetNode(NodeIndex) ;
                        Node.SetupUserData(&GUIInfo, sizeof(GUIInfo)) ;
                    }
                }
                break ;
        }

        LineNumber++ ;
	}

    // Loaded an anim?
    //if (pAnim)
    //{
        //ASSERTS(pAnim->GetFrameCount(), ".skel file does not contain animation!! re-export!") ;
    //}


	// Close file
    x_fclose(File) ;

    // Free file buffer
    delete [] FileBuffer ;

    // Show info
    shape::printf("Model %s finished loading and initializing\n\n", Filename) ;

    // Success
    return TRUE ;
}

