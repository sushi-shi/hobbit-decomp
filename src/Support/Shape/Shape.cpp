#include <xCore/Entropy/Entropy.hpp>
#include <Support/Shape/Shape.hpp>
#include <Support/Shape/SelectionList.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include <Support/Shape/TextureManager.hpp>

#ifdef TARGET_PS2
#include "PS2_Shape.hpp"    // For micro code
#endif


//==============================================================================
// Default draw settings
//==============================================================================

shape::draw_settings shape::s_DrawSettings =
{
    TRUE,   // Fog
    -1,     // Mips
    TRUE,   // AlphaPass
    TRUE,   // DiffusePass
    TRUE,   // LuminancePass
    TRUE,   // ReflectionPass
    TRUE,   // DetailPass
    TRUE,   // BumpPass
    TRUE,   // UseDrawList
    FALSE,  // WireFrame
} ;

//==============================================================================
// Static variables
//==============================================================================


// To make sure the shape library has been setup
xbool               shape::s_Initialized = FALSE ;

// Output path for .shp and xbmp files
char                shape::s_SHP_Path  [X_MAX_PATH] = {0} ;
char                shape::s_XBMP_Path [X_MAX_PATH] = {0} ;
                    
// Light            
vector4             shape::s_LightDirection    (0.0f, 1.0f, 0.0f, 0.0f) ALIGN_DATA(16) ;
vector4             shape::s_LightColor        (1.0f, 1.0f, 1.0f, 1.0f) ALIGN_DATA(16) ;
vector4             shape::s_LightAmbientColor (0.4f, 0.4f, 0.4f, 1.0f) ALIGN_DATA(16) ;
xcolor              shape::s_FogColor   = xcolor(0,0,0,0) ;

// Drawing
xbool               shape::s_InBeginFrame      = FALSE ;
xbool               shape::s_InBeginDraw       = FALSE ;
s32                 shape::s_FrameCount        = 0 ;
                    
// View
shape::plane_info   shape::s_ViewPlane[6] ;
shape::plane_info   shape::s_ClipPlane[6] ;


// Debugging
//shape_printf_fn* shape::s_PrintfFunc = shape::DefaultPrintf ;
shape_printf_fn* shape::s_PrintfFunc = NULL ;

// Material ID's (for debugging to see how big the draw lists should be)
s32                 shape::s_LastAssignedMaterialID         = 0 ;
s32                 shape::s_LastAssignedMaterialDrawListID = 0 ;

// PS2 specific
#ifdef TARGET_PS2
s32                 shape::s_hMicroCode = -1 ;
#endif


// D3D specific
#ifdef TARGET_PC

xbool             shape::s_UseDot3 = FALSE ;

s32               shape::s_hDifVertexShader              = 0 ;
s32               shape::s_hDifPixelShader               = 0 ;

s32               shape::s_hTexDifVertexShader           = 0 ;
s32               shape::s_hTexDifPixelShader            = 0 ;
                                                          
s32               shape::s_hTexDifRefVertexShader        = 0 ;
s32               shape::s_hTexDifRefPixelShader         = 0 ;
                                                          
s32               shape::s_hTexDifRefBumpVertexShader    = 0 ;
s32               shape::s_hTexDifRefBumpPixelShader     = 0;
                                                          
s32               shape::s_hTexDifDot3VertexShader       = 0 ;
s32               shape::s_hTexDifDot3PixelShader        = 0 ;
                                                          
s32               shape::s_hTexDifRefDot3VertexShader    = 0 ;
s32               shape::s_hTexDifRefDot3PixelShader     = 0 ;

#endif
    

//==============================================================================
//
// Class functions
//
//==============================================================================


//==============================================================================
// Functions
//==============================================================================


// Default
shape::shape()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(shape) & 15) == 0) ;

    // Init
    m_Name[0]           = 0 ;
    m_Type              = 0 ;                          

    m_TextureManager    = NULL ;
                        
    m_nModels           = 0 ;
    m_Models            = NULL ;

    m_nAnims            = 0 ;
    m_Anims             = NULL ;
                        
    m_nDListVerts       = 0 ;
    m_nDListPolys       = 0 ;

    m_BinFileData       = NULL ;
}

//==============================================================================

// Destructor
shape::~shape()
{
    // If shape came from a binary file, then all the allocated memory came from this
    // so there's no need to delete the other classes...
    if (m_BinFileData)
        delete [] m_BinFileData ;
    else
    {
        // Free all memory...
        if (m_Models)
            delete [] m_Models ;

        if (m_Anims)
            delete [] m_Anims ;
    }
}

//==============================================================================

void shape::SetName(const char *Name)
{
    char    DRIVE   [X_MAX_DRIVE];
    char    DIR     [X_MAX_DIR];
    char    FNAME   [X_MAX_FNAME];
    char    EXT     [X_MAX_EXT];
    char    PATH    [X_MAX_PATH];

    // Break it up
    x_splitpath(Name,DRIVE,DIR,FNAME,EXT);

    // Build name
    x_makepath( PATH, NULL, NULL, FNAME, NULL);

    ASSERT(x_strlen(PATH) < (s32)sizeof(m_Name));
    x_strcpy(m_Name, PATH) ;
}

//==============================================================================

// Call after parsing an ascii file
void shape::CleanupUnusedMemory()
{
    s32 i ;

    // Keep memory before
    s32 MemBefore = MemoryUsed() ;

    // Cleanup models
    for (i = 0 ; i < m_nModels ; i++)
        m_Models[i].CleanupUnusedMemory() ;

    // Get memory before
    s32 MemAfter = MemoryUsed() ;

    // Show stats
    shape::printf("   cleaning up memory.. before %dk after %dk\n", (MemBefore+1023)/1024, (MemAfter+1023)/1024) ;
}

//==============================================================================

// Returns total of memory used
s32 shape::MemoryUsed()
{
    s32 i, Size = sizeof(shape) ;

    for (i = 0 ; i < m_nModels ; i++)
        Size += m_Models[i].MemoryUsed() ;

    for (i = 0 ; i < m_nAnims ; i++)
        Size += m_Anims[i].MemoryUsed() ;
    
    return Size ;
}

//==============================================================================

// Loads all textures
void shape::LoadTextures(const shape_load_settings&   LoadSettings)
{
#if !defined(TARGET_LINUX)
    // Make sure texture manager has been assigned
    ASSERT(m_TextureManager) ;

    // Loop through all models
    for (s32 i = 0 ; i < m_nModels ; i++)
        GetModel(i).LoadTextures(*m_TextureManager, LoadSettings) ;
#endif
}

//==============================================================================

// Finds bounds
void shape::CalculateBounds()
{
    s32 m ;

    // Reset
    m_Bounds.Clear() ;

    // Loop through all models
    for (m = 0 ; m < m_nModels ; m++)
    {
        // Get model bounds
        model &M = m_Models[m] ;

        // Add to shape bounds
        m_Bounds += M.GetBounds() ;
    }
}

//==============================================================================

// Removes any unused nodes from the heirarchy
// A node is considered unused if:
// 1) It has no geometry attached to it
// 2) It has no hot points attached to it
void shape::DeleteUnusedNodes()
{
    s32 i,j,k, NodeCount = -1 ;

    // Show info
    shape::printf("Removing unused nodes...\n") ;

    // Make sure all models have the same number of nodes
    for (i = 0 ; i < GetModelCount() ; i++)
    {
        model &Model = GetModel(i) ;

        if (NodeCount == -1)
            NodeCount = Model.GetNodeCount() ;
        else
        if (NodeCount != Model.GetNodeCount())
        {
            // If you got here your models do not have the same heirarchy - check the max files!
            ASSERTS(0, "Heirarchy of models do not match - check max/skel files!\n") ;
            return ;
        }
    }

    // No models were loaded...
    if (NodeCount == -1)
    {
        ASSERTS(0, "No models were loaded - a quite useless shape!\n") ;
        return ;
    }

    // Make sure all animations have the same number of key sets (1 per node)
    for (i = 0 ; i < GetAnimCount() ; i++)
    {
        anim &Anim = GetAnim(i) ;

        if (Anim.GetKeySetCount() != NodeCount)
        {
            // If you got here, the models heirarchy does not match the anim heirarchy - check the max file!
            ASSERTS(0, "Heirarchy of anim does not match heirarhcy of models - check max/skel files!\n") ;
            return ;
        }
    }

    // Okay - let's loop through the nodes, backwards - deleting the unused nodes
    for (i = (NodeCount-1) ; i >= 0 ; i--)
    {
        xbool NodeUsed = FALSE ;

        // Check all models for using this node
        for (j = 0 ; j < GetModelCount() ; j++)
        {
            model &Model = GetModel(j) ;

            NodeUsed |= Model.IsNodeUsed(i) ;
        }

        // Used first model for getting at parents etc
        ASSERT(GetModelCount()) ;
        model &Model = GetModel(0) ;

        // Can it be deleted?
        if (!NodeUsed)
        {
            // First of all - apply key sets of node to the children
            for (j = 0 ; j < Model.GetNodeCount() ; j++)
            {
                // Is this node a child?
                if (Model.GetNode(j).GetParent() == i)
                {
                    ASSERT(i != j) ;    // Can't be child of self!

                    // Okay - update all the anims
                    for (k = 0 ; k < GetAnimCount() ; k++)
                        GetAnim(k).ApplyKeySetToChildKeySet(i, j) ;
                }
            }

            // Delete node from anims
            for (j = 0 ; j < GetAnimCount() ; j++)
                GetAnim(j).DeleteKeySet(i) ;

            // Delete node from models
            for (j = 0 ; j < GetModelCount() ; j++)
                GetModel(j).DeleteNode(i) ;
        }
    }

    // Show info...
    s32 NewNodeCount = GetModel(0).GetNodeCount() ;
    shape::printf("Original node count:%d Optimized node count:%d\n", NodeCount, NewNodeCount) ;
}

//==============================================================================

// Call after loading all shape data
void shape::InitializeForDrawing(shape_bin_file_class::target Target)
{
    s32 i ;

    // Setup models
    for (i = 0 ; i < m_nModels ; i++)
        m_Models[i].InitializeForDrawing(*this, *m_TextureManager, Target) ;

    // Setup vars
    CalculateBounds() ;

    // Release unused memory
    CleanupUnusedMemory() ;
}

//==============================================================================

// Returns model
model &shape::GetModel(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nModels) ;
    return m_Models[Index] ;
}

//==============================================================================

// Returns model
model *shape::GetModelByIndex(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nModels) ;
    return &m_Models[Index] ;
}

//==============================================================================

// Returns model of given type if found, else NULL
model *shape::GetModelByType(s32 Type)
{
    s32             i ;
    selection_list  List ;

    // Add models of requested type to the selection list
    for (i = 0 ; i < m_nModels ; i++)
    {
        model &Model = m_Models[i] ;
        if (Model.GetType() == Type)
            List.AddElement(i, Model.GetWeight()) ;
    }

    // Not found?
    if (List.GetCount() == 0)
        return NULL ;

    // Choose from list
    return &m_Models[List.ChooseElement()] ;
}

//==============================================================================

// Compares anims by their types
s32 anim_type_compare_fn( const void* pItem1, const void* pItem2 )
{
	anim *pAnimA = (anim *)pItem1 ;
	anim *pAnimB = (anim *)pItem2 ;

    // Sort by type
    if (pAnimA->GetType() > pAnimB->GetType() ) return 1;
    if (pAnimA->GetType() < pAnimB->GetType() ) return -1;
    return 0;
}

// Sorts anims by type for faster lookup
void shape::SortAnimsByType(void)
{
    s32 i ;

	// Sort anims by type
    if (m_nAnims)
    {
        // Sort
        x_qsort(m_Anims,   			    // Address of first item in array.
			    m_nAnims,	            // Number of items in array.
                sizeof(anim), 		    // Size of one item.
			    anim_type_compare_fn) ;	// Compare function.

        // Update indices
        for (i = 0 ; i < m_nAnims ; i++)
            m_Anims[i].SetIndex(i) ;
    }
}

//==============================================================================

anim &shape::GetAnim(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nAnims) ;
    return m_Anims[Index] ;
}

//==============================================================================

// Returns animation
anim *shape::GetAnimByIndex(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nAnims) ;
    return &m_Anims[Index] ;
}

//==============================================================================

// Returns anim of given type if found, else NULL
anim *shape::GetAnimByType(s32 Type)
{
    // Do binary search...
    s32             Lo, Mid, Hi ;
    anim*           Anim ;
    selection_list  List ;

    // Must be some anims!
    if (!m_nAnims)
        return NULL ;

    // Start at array limits
    Lo = 0 ;
    Hi = m_nAnims-1 ;

    // Loop until we get the closest type
    while(Lo < Hi)
    {
        // Lookup mid anim
        Mid  = (Lo + Hi) >> 1 ;
        Anim = &m_Anims[Mid] ;
        ASSERT(Anim) ;

        // Chop search in half
        if (Anim->GetType() < Type)
            Lo = Mid+1 ;
        else
            Hi = Mid ;
    }

    // Found the anim?
    Anim = &m_Anims[Lo] ;
    ASSERT(Anim) ;
    if (Anim->GetType() == Type)
    {
        // There may be more than one anim assigned the same type,
        // so search backwards for the first occurance..
        while((Lo > 0) && (m_Anims[Lo-1].GetType() == Type))
            Lo-- ;

        // Get first anim of this type...
        ASSERT(Lo >= 0) ;
        Anim = &m_Anims[Lo] ;
        ASSERT(Anim->GetType() == Type) ;

        // Add anims of requested type to the selection list
        while((Lo < m_nAnims) && (Anim->GetType() == Type))
        {
            List.AddElement(Lo, Anim->GetWeight()) ;
            
            // Next anim
            Lo++ ;
            Anim++ ;
        }

        // Finally, choose from list
        ASSERT(List.GetCount()) ;
        return &m_Anims[List.ChooseElement()] ;
    }
    else
        return NULL ;
    

    /*
    s32             i ;
    selection_list  List ;

    // Add anims of requested type to the selection list
    for (i = 0 ; i < m_nAnims ; i++)
    {
        anim &Anim = m_Anims[i] ;
        if (Anim.GetType() == Type)
            List.AddElement(i, Anim.GetWeight()) ;
    }

    // Not found?
    if (List.GetCount() == 0)
        return NULL ;

    // Choose from list
    return &m_Anims[List.ChooseElement()] ;
    */
}


//==============================================================================

// Adds an animation onto end of animation arrary
void shape::AddAnim(anim &Anim)
{
    s32 i ;

    // Allocate space for new anims
    s32     nNewAnims = m_nAnims + 1 ;
    anim    *NewAnims = new anim[nNewAnims] ;
    ASSERT(NewAnims) ;

    // Copy old anims
    for (i = 0 ; i < m_nAnims ; i++)
        NewAnims[i] = m_Anims[i] ;

    // Copy new anim
    NewAnims[m_nAnims] = Anim ;

    // Delete old anim data
    if (m_Anims)
    {
        ASSERT(m_nAnims) ;
        delete [] m_Anims ;
    }

    // Use new anim data
    m_nAnims = nNewAnims ;
    m_Anims  = NewAnims ;

}

//==============================================================================

#ifdef TARGET_PS2

// Loads micro code (call before using any draw functions)
void shape::InitMicroCode()
{
    // plane pack
    struct vu_plane
    {
        u32 Mask ;
        u32 DifJumpAddr ;
        u32 DifRefJumpAddr ;
        u32 Unused[1] ;
    } ;

    // Init pack for setting up double buffering to vu memory
    struct init_pack
    {
        dmatag      DMA ;
        u32         NOP1 ;
        u32         MSCAL ;
        u32         STCYCL ;        
        u32         UNPACK ;        
        giftag      SyncGIF ;   // QWord 1
        matrix4     W2V ;       // QWords 2,3,4,5
        matrix4     W2C ;       // QWords 6,7,8,9
        matrix4     C2S ;       // QWords 10,11,12,13
        matrix4     W2S ;       // QWords 14,15,16,17

        u32         NOP[3] ;
        u32         UNPACK2 ;
        vu_plane    VUPlane[7] ;
    } ;

    // Wait for GS to finish last draw before writing to vu memory
    vram_Sync() ;

    // Load micro code
    eng_ActivateMicrocode(s_hMicroCode) ;

    // Prepare to send data to vu data memory
    init_pack *Pack = DLStruct(init_pack) ;
    Pack->DMA.SetCont(sizeof(init_pack) - sizeof(dmatag)) ;
    Pack->DMA.PAD[0]    = SCE_VIF1_SET_NOP(0) ;
    Pack->DMA.PAD[1]    = SCE_VIF1_SET_NOP(0) ;
    Pack->NOP1          = SCE_VIF1_SET_NOP(0) ;
    Pack->MSCAL         = SCE_VIF1_SET_MSCAL(SHAPE_MICRO_CODE_INIT, 0) ;
    Pack->STCYCL        = SCE_VIF1_SET_STCYCL(4, 4, 0) ;
    Pack->UNPACK        = VIF_Unpack(SHAPE_SYNC_GIF_ADDR, 17, VIF_V4_32, FALSE, FALSE, TRUE) ;

    Pack->NOP[0]        = SCE_VIF1_SET_NOP(0) ;
    Pack->NOP[1]        = SCE_VIF1_SET_NOP(0) ;
    Pack->NOP[2]        = SCE_VIF1_SET_NOP(0) ;
    Pack->UNPACK2       = VIF_Unpack(SHAPE_CLIPPER_PLANE_TABLE, 7, VIF_V4_32, FALSE, FALSE, TRUE) ;

    // Setup data
    const view* View = eng_GetView() ;
    ASSERT(View) ;
    Pack->SyncGIF.Build2(0, 0, 0, 0) ;

     // Setup camera matrices
    Pack->W2V = View->GetW2V() ;
    Pack->W2C = View->GetW2C() ;
    Pack->C2S = View->GetC2S() ;
    Pack->W2S = View->GetW2S() ;

    // Temp to test clipping
    //f32 Scale = 0.75f ;
    //f32 InvScale = 1.0f / Scale ;
    //Pack->W2C.Scale(vector3(InvScale, InvScale, 1.0f)) ;
    //Pack->C2S.PreScale(vector3(Scale, Scale, 1.0f)) ;

    // Setup clip plane data
    vu_plane* Plane = Pack->VUPlane ;

    // BACK
    //Plane->Mask             = CLIP_POS_Z_FLAG ;
    //Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_Z ;
    //Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_Z ;
    //Plane++ ;

    // FRONT (Put first since floating inaccuracy can cause big polys)
    Plane->Mask             = CLIP_NEG_Z_FLAG ;
    Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_Z ;
    Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_Z ;
    Plane++ ;

    // RIGHT
    Plane->Mask             = CLIP_POS_X_FLAG ;
    Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_X ;
    Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_X ;
    Plane++ ;
    
    // LEFT
    Plane->Mask             = CLIP_NEG_X_FLAG ;
    Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_X ;
    Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_X ;
    Plane++ ;
    
    // BOTTOM
    Plane->Mask             = CLIP_POS_Y_FLAG ;
    Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_Y ;
    Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_Y ;
    Plane++ ;
    
    // TOP
    Plane->Mask             = CLIP_NEG_Y_FLAG ;
    Plane->DifJumpAddr      = SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_Y ;
    Plane->DifRefJumpAddr   = SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_Y ;
    Plane++ ;
    
    // TERMINATE
    Plane->Mask     = 0 ;
}

#endif  //#ifdef TARGET_PS2

//==============================================================================

// Read/write shape as binary file
void shape::ReadWrite(shape_bin_file &File)
{
    // Read/write attributes
    File.ReadWrite(m_Name) ;
    File.ReadWrite(m_Type) ;
    File.ReadWrite(m_nDListVerts) ;
    File.ReadWrite(m_nDListPolys) ;
    File.ReadWrite(m_Bounds) ;

    // Read/write models
    File.ReadWriteClassArray(m_Models, m_nModels) ;

    // Read/write anims
    File.ReadWriteClassArray(m_Anims, m_nAnims) ;
}

//==============================================================================

// Loads shape from binary file
xbool shape::ReadBinaryFile(const char *Filename, const shape_load_settings& LoadSettings)
{
    shape_bin_file  File ;
    time_stamp      TimeStamp ;
    s32             i ;

    // Try load file
    m_BinFileData = File.OpenForReading(Filename) ;
    if (!m_BinFileData)
        return FALSE ;

    // Show info
    shape::printf("Loading %s\n", Filename) ;

    // Read the file
    ReadWrite(File) ;

    // Done!
    File.Close() ;

    // Show info
    shape::printf("Finished reading %s\n", Filename) ;

    // Check the version and time stamps of all model and anim files
    xbool BinaryFileIsGood = TRUE ;

	// Open skel files and check all timestamps?
	if (LoadSettings.CheckSkelTimeStamps)
	{
		// Check models
		for (i = 0 ; i < m_nModels ; i++)
		{
			model &Model = m_Models[i] ;

			// If binary version of model is older than the .skel file, then re-load the skel file
			if (ReadAsciiFileTimeStamp(Model.GetName(), TimeStamp))
			{
				// Show info
				shape::printf("   Checking model %s time stamp\n", Model.GetName()) ;
				if (TimeStamp != Model.GetTimeStamp())
				{
					// Re-load .skel
					shape::printf("   Binary version of model %s is old. Reloading .skel...\n", Model.GetName()) ;
					BinaryFileIsGood = FALSE ;
				}
			}
		}

		// Check animations
		for (i = 0 ; i < m_nAnims ; i++)
		{
			anim &Anim = m_Anims[i] ;

			// If binary version of anim is older than the .skel file, then re-load the skel file
			if (ReadAsciiFileTimeStamp(Anim.GetName(), TimeStamp))
			{
			    // Show info
				shape::printf("   Checking anim %s time stamp\n", Anim.GetName()) ;
				if (TimeStamp != Anim.GetTimeStamp())
				{
					// Re-load .skel
					shape::printf("   Binary version of anim %s is old. Reloading .skel...\n", Anim.GetName()) ;
					BinaryFileIsGood = FALSE ;
				}
			}
		}
	}
	    
    // If binary file is not good, free the memory used
    if (!BinaryFileIsGood)
    {
        delete [] m_BinFileData ;
        m_BinFileData = NULL ;
    }

    // If binary file is out of date, return FALSE so that ASCII files are loaded again..
    return (BinaryFileIsGood) ;
}

//==============================================================================

// Count the number of anim .skel files in the binary shape
// (do this by counting the number of unique anim names - since the anim name
//  is the skel file it came from)
s32 shape::GetAnimFileCount()
{
    s32   Total = 0 ;

    // Loop through all anims
    for (s32 i = 0 ; i < m_nAnims ; i++)
    {
        // Count the # of unique anim file names
        xbool Found = FALSE ;
        for (s32 j = (i-1) ; j >= 0 ; j--)
        {
            // Do names match?
            if (x_strcmp(GetAnim(i).GetName(), GetAnim(j).GetName()) == 0)
            {
                Found = TRUE ;
                break ;
            }
        }
        
        // If file name is unique, update the total
        if (!Found)
            Total++ ;
    }

    return Total ;
}

//==============================================================================

// Count the number of model .skel files in the binary shape
// (do this by counting the number of unique anim names - since the anim name
//  is the skel file it came from)
s32 shape::GetModelFileCount()
{
    s32   Total = 0 ;

    // Loop through all models
    for (s32 i = 0 ; i < m_nModels ; i++)
    {
        // Count the # of unique model file names
        xbool Found = FALSE ;
        for (s32 j = (i-1) ; j >= 0 ; j--)
        {
            // Do names match?
            if (x_strcmp(GetModel(i).GetName(), GetModel(j).GetName()) == 0)
            {
                Found = TRUE ;
                break ;
            }
        }
        
        // If file name is unique, update the total
        if (!Found)
            Total++ ;
    }

    return Total ;
}


//==============================================================================
//
// Static global functions
//
//==============================================================================

// Set output path for .shp and .xbmp files
void shape::SetDataOutputPaths(const char *SHP_Path, const char *XBMP_Path)
{
    Set_SHP_Path(SHP_Path) ;
    Set_XBMP_Path(XBMP_Path) ;
}

//==============================================================================

void shape::Set_SHP_Path(const char *SHP_Path)
{
    // Keep .shp path
	if (SHP_Path)
		x_strcpy(s_SHP_Path,  SHP_Path) ;
	else
		s_SHP_Path[0] = 0 ;
}

//==============================================================================

// Returns output path to use when writing .shp files
const char *shape::Get_SHP_Path()
{
    // If path is not setup yet, return NULL so it can be checked for!
    if (!x_strlen(s_SHP_Path))
        return NULL ;

    // Return current path
    return s_SHP_Path ;
}

//==============================================================================

void shape::Set_XBMP_Path(const char *XBMP_Path)
{
    // Keep .shp path
	if (XBMP_Path)
		x_strcpy(s_XBMP_Path,  XBMP_Path) ;
	else
		s_XBMP_Path[0] = 0 ;
}

//==============================================================================

// Returns output path to use when writing .xbmp files
const char *shape::Get_XBMP_Path()
{
    // If path is not setup yet, return NULL so it can be checked for!
    if (!x_strlen(s_XBMP_Path))
        return NULL ;

    // Return current path
    return s_XBMP_Path ;
}

//==============================================================================

// Sets the light direction 
void shape::SetLightDirection( const vector3& vDir, xbool Normalize )
{   
    // Make a copy so we can normalize it if needed
    vector3 Dir = vDir ;

    // Normalize it?
    if( Normalize )
        Dir.Normalize() ;

    // Put into hardware friendly vector4 (last component must be zero)
    s_LightDirection.Set( Dir.X, Dir.Y, Dir.Z, 0.0f );
}

//==============================================================================

// Set the light color
void shape::SetLightColor(f32 R, f32 G, f32 B, f32 A)
{
    s_LightColor.X = R ;
    s_LightColor.Y = G ;
    s_LightColor.Z = B ;
    s_LightColor.W = A ;
}

void shape::SetLightColor(const vector4& Col)
{
	SetLightColor(Col.X, Col.Y, Col.Z, Col.W) ;
}

void shape::SetLightColor(const xcolor&  Col)
{
	SetLightColor((f32)Col.R / 255.0f, (f32)Col.G / 255.0f, (f32)Col.B / 255.0f, (f32)Col.A / 255.0f) ;
}

//==============================================================================

// Set the light ambience color
void shape::SetLightAmbientColor(f32 R, f32 G, f32 B, f32 A)
{
    s_LightAmbientColor.X = R ;
    s_LightAmbientColor.Y = G ;
    s_LightAmbientColor.Z = B ;
    s_LightAmbientColor.W = A ;
}

void shape::SetLightAmbientColor(const vector4& Col)
{
	SetLightAmbientColor(Col.X, Col.Y, Col.Z, Col.W) ;
}

void shape::SetLightAmbientColor(const xcolor&  Col)
{
	SetLightAmbientColor((f32)Col.R / 255.0f, (f32)Col.G / 255.0f, (f32)Col.B / 255.0f, (f32)Col.A / 255.0f) ;
}

//==============================================================================

vector3 shape::GetLightDirection()
{
    return vector3(s_LightDirection.X, s_LightDirection.Y, s_LightDirection.Z) ;
}

//==============================================================================

vector4 shape::GetLightColor()
{
    return s_LightColor ;
}

//==============================================================================

vector4 shape::GetLightAmbientColor()
{
    return s_LightAmbientColor ;
}

//==============================================================================

void shape::SetFogColor(const xcolor &Color)
{
    // Keep color
    s_FogColor = Color ;    
}

//==============================================================================

xcolor shape::GetFogColor()
{
    return s_FogColor ;
}

//==============================================================================

xbool shape::InBeginFrame()
{
    return s_InBeginFrame ;
}

//==============================================================================

// Call at beggining of rendering frame
void shape::BeginFrame()
{
    // Make sure you called shape::Init() and shape::Kill() in your app
    ASSERT(s_Initialized) ;

    // Should not already be in begin!
    ASSERT(!s_InBeginFrame) ;

    // Flag in frame
    s_InBeginFrame = TRUE ;

    // Clear stats
    #ifdef X_DEBUG
        material::ClearStats() ;
    #endif
}

//==============================================================================

// Call at end of rendering frame
void shape::EndFrame()
{
    // Should be in begin frame!
    ASSERT(s_InBeginFrame) ;

    // Flag end of frame
    s_InBeginFrame = FALSE ;

    // Next frame
    s_FrameCount++ ;

    // Print stats
    
    //#if defined(sbroumley) && defined(X_DEBUG)
        // Only show evert 63 frames
        //if ( ((s_FrameCount & 63) == 0) && (((s_FrameCount+1) & 63) == 1))
            //material::PrintStats() ;
    //#endif
}


//==============================================================================
// DRAW LIST FUNCTIONS
//==============================================================================

xbool shape::InBeginDraw()
{
    return s_InBeginDraw ;
}


//==============================================================================

void shape::BeginDrawSettings()
{
    //======================================================================
    // PS2 specific code
    //======================================================================
    #ifdef TARGET_PS2
        // Wait for GS to finish...
        vram_Sync() ;

        // Get rid of all textures and banks ready for shapes...
        vram_Flush() ;

        // Download the micro code so that we can draw shapes!
        InitMicroCode() ;

        // Set GS registers
        gsreg_Begin();

        // Turn off clamping on context0
        eng_PushGSContext(0);
        gsreg_SetClamping( FALSE );
        eng_PopGSContext();

        // Turn on clamping for context1
        eng_PushGSContext(1);
        gsreg_SetClamping( TRUE );
        eng_PopGSContext();

        // Setup stencil mode on context1 so that reflection pass is only rendered once per pixel!
        eng_PushGSContext(1);
        gsreg_SetAlphaAndZBufferTests( FALSE,                   // AlphaTest
                                       ALPHA_TEST_ALWAYS,       // AlphaTestMethod
                                       64,                      // AlphaRef
                                       ALPHA_TEST_FAIL_KEEP,    // AlphaTestFail
                                       TRUE,                    // DestAlphaTest
                                       DEST_ALPHA_TEST_0,       // DestAlphaTestMethod
                                       TRUE,                    // ZBufferTest
                                       ZBUFFER_TEST_GEQUAL ) ;  // ZBufferTestMethod
        gsreg_SetAlphaCorrection(TRUE) ;
        eng_PopGSContext();

        // Ship register settings
        gsreg_End();

    #endif

    //======================================================================
    // PC specific code
    //======================================================================
    #ifdef TARGET_PC         
        // Get current view
        const view *View = eng_GetActiveView(0) ;
		ASSERT(View) ;

        // Setup world to view and the projection matrix
        g_pd3dDevice->SetTransform(D3DTS_VIEW,       (D3DMATRIX *)&View->GetW2V()) ;
        g_pd3dDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX *)&View->GetV2C()) ;
        matrix4 mIdent ;
        mIdent.Identity() ;
        g_pd3dDevice->SetTransform(D3DTS_WORLD,      (D3DMATRIX *)&mIdent) ;

	    // Right handed mode
		//g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
	    
        // No culling just like PS2
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

        // Enable lighting
        g_pd3dDevice->LightEnable( 0, TRUE ) ;
        g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE) ;

        // Setup poly fill mode
        if (s_DrawSettings.WireFrame)
            g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) ;
        else
            g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) ;

        // Setup viewport
        eng_SetViewport( *View );
    #endif
}

//==============================================================================

void shape::EndDrawSettings()
{
    //======================================================================
    // PS2 specific code
    //======================================================================
    #ifdef TARGET_PS2
        // Wait for GS to finish...
        vram_Sync() ;

        // Enable z write and set z test so it clear the alpha test (punch thru)
        gsreg_Begin();

        eng_PushGSContext(0);
        gsreg_SetZBufferUpdate(TRUE);
        gsreg_SetZBufferTest(ZBUFFER_TEST_GEQUAL) ;
        gsreg_SetFBMASK(0) ;
        gsreg_SetMipEquation( TRUE, 
                              0, 
                              0, 
                              MIP_MAG_BILINEAR, 
                              MIP_MIN_INTERP_MIPMAP_BILINEAR) ;
        gsreg_SetClamping( FALSE );
        eng_PopGSContext();

        eng_PushGSContext(1);
        gsreg_SetZBufferUpdate(TRUE);
        gsreg_SetZBufferTest(ZBUFFER_TEST_GEQUAL) ;
        gsreg_SetFBMASK(0) ;
        gsreg_SetMipEquation( TRUE, 
                              0, 
                              0, 
                              MIP_MAG_BILINEAR, 
                              MIP_MIN_INTERP_MIPMAP_BILINEAR) ;
        gsreg_SetClamping( FALSE );
        eng_PopGSContext();

        gsreg_End();
    #endif

    //======================================================================
    // PC specific code
    //======================================================================
    #ifdef TARGET_PC
        // Disable lighting and blending
        g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE ) ; 
        g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE ) ;

		// Terminate texture stages
		g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE) ;
		g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE) ;

        // Turn off texturing
        g_pd3dDevice->SetTexture( 1, NULL ) ;
		g_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE ) ;

        // Turn off fog
		g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE) ;
        
        // Enable z write
        g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

	    // Enable culling
		g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );

        // Turn off the alpha test (may have been used by punchthrough)
        g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE) ;  

        // Fill polys
        g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) ;

        // Turn clipping back on
        g_pd3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE) ;

        // Use default vertex/pixel shader
        d3deng_ActivateVertexShader(NULL) ;
        d3deng_ActivatePixelShader(NULL) ;

    #endif
}


//==============================================================================

void shape::BeginDraw()
{
    // Should not already be in begin!
    ASSERT(!s_InBeginDraw) ;

    // Must be in begin frame!
    ASSERT(InBeginFrame()) ;
    
    // Flag we are here
    s_InBeginDraw = TRUE ;

    // Clear all draw list entries
    ClearDrawList() ;

    // Do settings?
    if (!s_DrawSettings.UseDrawList)
        BeginDrawSettings() ;

    // Get active view
    const view* pView = eng_GetActiveView(0);
    ASSERT(pView) ;

    // Setup view planes
    //s32 X0, Y0, X1, Y1 ;
    //pView->GetViewport(X0, Y0, X1, Y1 ) ;
    //f32 W = (X1 - X0 + 1)*0.5f;
    //f32 H = (Y1 - Y0 + 1)*0.5f;
    //pView->GetViewPlanes(-W, -H, W, H,
                         //s_ViewPlane[3].Plane,  //T
                         //s_ViewPlane[2].Plane,  //B
                         //s_ViewPlane[0].Plane,  //L
                         //s_ViewPlane[1].Plane,  //R
                         //s_ViewPlane[4].Plane,  //N
                         //s_ViewPlane[5].Plane,  //F
                         //view::WORLD) ;

    // Setup PS2 clipping planes
    //#ifdef TARGET_PS2
        // Setup planes to turn clipping on/off (lets the GS scissoring take care of most of xy)
        //pView->GetViewPlanes(-1000.0f,
                             //-1000.0f,
                              //1000.0f,
                              //1000.0f,
                              //s_ClipPlane[3].Plane,  //T
                              //s_ClipPlane[2].Plane,  //B
                              //s_ClipPlane[0].Plane,  //L
                              //s_ClipPlane[1].Plane,  //R
                              //s_ClipPlane[4].Plane,  //N
                              //s_ClipPlane[5].Plane,  //F
                              //view::WORLD) ;
    //#endif

    // Setup PC clipping planes (same as view volume)
    //#ifdef TARGET_PC         

        // Setup planes to turn clipping on/off
        //pView->GetViewPlanes( s_ClipPlane[3].Plane,  //T
                              //s_ClipPlane[2].Plane,  //B
                              //s_ClipPlane[0].Plane,  //L
                              //s_ClipPlane[1].Plane,  //R
                              //s_ClipPlane[4].Plane,  //N
                              //s_ClipPlane[5].Plane,  //F
                              //view::WORLD) ;
    //#endif

    // Just use view volume for clipping too.
    // The GS screws up close textures otherwise
    pView->GetViewPlanes(s_ViewPlane[3].Plane,  //T
                         s_ViewPlane[2].Plane,  //B
                         s_ViewPlane[0].Plane,  //L
                         s_ViewPlane[1].Plane,  //R
                         s_ViewPlane[4].Plane,  //N
                         s_ViewPlane[5].Plane,  //F
                         view::WORLD) ;

    pView->GetViewPlanes(s_ClipPlane[3].Plane,  //T
                         s_ClipPlane[2].Plane,  //B
                         s_ClipPlane[0].Plane,  //L
                         s_ClipPlane[1].Plane,  //R
                         s_ClipPlane[4].Plane,  //N
                         s_ClipPlane[5].Plane,  //F
                         view::WORLD) ;

    // Setup plane bbox indices for view/clip planes
    for (s32 i = 0 ; i < 6 ; i++)
    {
        s_ViewPlane[i].Plane.GetBBoxIndices(s_ViewPlane[i].BBoxMinIndex, s_ViewPlane[i].BBoxMaxIndex) ;
        s_ClipPlane[i].Plane.GetBBoxIndices(s_ClipPlane[i].BBoxMinIndex, s_ClipPlane[i].BBoxMaxIndex) ;
    }
}

//==============================================================================

void shape::EndDraw()
{
    // Must be in begin!
    ASSERT(s_InBeginDraw) ;

    // Must be in begin frame!
    ASSERT(InBeginFrame()) ;

    // Draw any remaining geometry
    BeginDrawSettings() ;
    FlushDrawList() ;

    // Flag done...
    s_InBeginDraw = FALSE ;

    // End settings
    EndDrawSettings() ;
}

//==============================================================================

// Tribes2 shapes totals:
#define MATERIAL_ENTRIES    350        // Must be >= total number of materials used (MaxID)
#define DRAW_ENTRIES        6000       // ID's are assigned based on tex count (MaxDrawListID)


// Assign material id's to a list of shapes, ready for drawing with the draw list
void shape::AssignMaterialIDs(shape* Shapes[], s32 nShapes, s32& ID, s32& DrawListID)
{
    // Loop through all shapes
    for (s32 s = 0 ; s < nShapes ; s++)
    {
        // Get shape
        ASSERT(Shapes[s]) ;
        shape &Shape = *Shapes[s] ;

        // Loop through all models in shape
        for (s32 m = 0 ; m < Shape.GetModelCount() ; m++)
        {
            // Get model
            model &Model = Shape.GetModel(m) ;

            // Loop through all materials in shape
            for (s32 mat = 0 ; mat < Model.GetMaterialCount() ; mat++)
            {
                // Get material
                material &Material = Model.GetMaterial(mat) ;

                // Set IDs
                Material.SetID(ID) ;
                Material.SetDrawListID(DrawListID) ;

                // For animated materials, make sure they hash to a different draw entry slot
                s32 TotalFrameBits = 0 ;
                
                // Take into account the texture frames
                for (s32 t = material::TEXTURE_TYPE_START ; t <= material::TEXTURE_TYPE_END ; t++)
                    TotalFrameBits += Material.GetTextureInfo((material::texture_type)t).FrameBits ;
   
                // Skip to next ID
                ID++ ;
                DrawListID += 1<<TotalFrameBits ;
            }
        }
    }
    
    // Record last id for debugging...
    s_LastAssignedMaterialID         = ID ;
    s_LastAssignedMaterialDrawListID = DrawListID ;

    // If this asserts, you need to increase these defines
    ASSERT(ID         < MATERIAL_ENTRIES) ;
    ASSERT(DrawListID < DRAW_ENTRIES) ;
}

//==============================================================================

void shape::AssignMaterialIDs(shape* Shapes[], s32 nShapes)
{
    s32 ID = 0 ;
    s32 DrawListID = 0 ;
    AssignMaterialIDs(Shapes, nShapes, ID, DrawListID) ;
}

//==============================================================================

// Structure used for sorting and drawing instances
struct inst_entry
{
    shape_instance*         Instance ;  // Instance to draw
    material_draw_info*     DrawInfo ;  // Draw info to use
    f32                     ViewZ ;     // Z from view (used for sorting)
} ;

#define INST_ENTRIES   100


static inst_entry  s_InstList[INST_ENTRIES] ;
static s32         s_InstEntries = 0 ;


// Sorts instances from back to front
s32 inst_entry_compare_fn( const void* pItem1, const void* pItem2 )
{
	inst_entry *EntryA = (inst_entry *)pItem1 ;
	inst_entry *EntryB = (inst_entry *)pItem2 ;

    // Sort by z
    if (EntryA->ViewZ < EntryB->ViewZ) return 1;
    if (EntryA->ViewZ > EntryB->ViewZ) return -1;
    return 0;
}

// Structured used for drawing section of a model
// (one per material per model)
struct geom_entry
{
    texture_draw_info   TexDrawInfo ;   // Textures to use
    material_draw_info* DrawInfo ;      // Draw info to use
    geom_entry*         Next ;          // Next in list
} ;

// Structure used in consolidating material draws.
// (it contains a list of geometries that want to draw in this material)
struct draw_entry
{
    geom_entry*         GeomHead ;      // Start of geometry linked list
    geom_entry*         GeomTail ;      // Tail of geometry linked list
    draw_entry*         Next ;          // Next in linked list (if any)
} ;

// The big draw list of materials...
struct material_entry
{
    material*           Material ;      // Material
    draw_entry*         DrawHead ;      // Head of draw list
    draw_entry*         DrawTail ;      // Tail of draw list
    material_entry*     Next ;          // Next draw entry in linked list
} ;

// The main draw list
struct draw_list
{
    // Draw entry (several per material - depending on how many different textures that material can use)
    draw_entry      DrawEntries[DRAW_ENTRIES] ;

    // Material list (one per material)
    material_entry  MatEntries[MATERIAL_ENTRIES] ;
    material_entry  *MatHead, *MatTail ;
} ;

static draw_list s_DrawList[2] ; // 0=opaque, 1 = trans
static xbool     s_DrawListEmpty = FALSE ;

//==============================================================================
//
// Here's how this draw list works:
//
// There are two lists inside of it, the DrawEntries list has an entry for every displayable combination
// of every material (ie. if the material contains 4 animating textures, then there are 4 entries).
// Each draw entry contains a list of geometry entries - all of which are using the same material
// combination.
//
// The MatEntries have an entry for every unique material. Within each mat entry is a list of all the
// draw entries that descend from the material.
//
// Calling AddMaterialToDrawList will map the material into the correct draw entry, and if needed,
// will create and add the materail entry into the main draw list.
//
// Tracing through the final draw list, the drawing will take place in material order.
// Within each material, it will be in material combination order.
//
// There are 2 draw lists - one for the opaque materials (drawn first),
// and one for the transparent material (drawn last)
//
//==============================================================================


//==============================================================================

void shape::ClearDrawList()
{
    // Only clear if we have to
    if (!s_DrawListEmpty)
    {
        // Clear instance list    
        s_InstEntries = 0 ;

        // Clear material list
        x_memset(s_DrawList, 0, sizeof(s_DrawList)) ;
        
        // Flag it's empty
        s_DrawListEmpty = TRUE ;
    }

    // Make sure the lists are empty
    ASSERT(s_DrawList[0].MatHead == NULL) ;
    ASSERT(s_DrawList[0].MatTail == NULL) ;
    ASSERT(s_DrawList[1].MatHead == NULL) ;
    ASSERT(s_DrawList[1].MatTail == NULL) ;
    ASSERT(s_InstEntries == 0 );
}

//==============================================================================

// Adds instance to the draw list (doesn't do any drawing - that happens in the flush)
void shape::AddInstanceToDrawList(shape_instance& Instance, material_draw_info& DrawInfo)
{
    // Emergency action - if draw list is full, then flush it so we can add more
    if (s_InstEntries == INST_ENTRIES)
        FlushInstanceDrawList() ;

    // Should never be full!
    ASSERT(s_InstEntries < INST_ENTRIES) ;

    // Can't be NULL!
    ASSERT(&Instance) ;
    ASSERT(&DrawInfo) ;

    // Get current view
    const view *View = eng_GetActiveView(0) ;
	ASSERT(View) ;

    // Get world to view matrix
    const matrix4& W2V = View->GetW2V() ;

    // Use middle of instance for sort pos
    vector3 Pos = Instance.GetWorldBounds().GetCenter() ;
    
    // Get next entry
    inst_entry& Entry = s_InstList[s_InstEntries++] ;

    // Setup entry
    Entry.Instance = &Instance ;
    Entry.DrawInfo = &DrawInfo ;
    Entry.ViewZ    = (W2V(0,2)*Pos.X) + (W2V(1,2)*Pos.Y) + (W2V(2,2)*Pos.Z) + W2V(3,2) ;
}

//==============================================================================

// Adds all instances to material draw list
void shape::FlushInstanceDrawList()
{
    s32 i ;

    // Any to draw?
    if (!s_InstEntries)
        return ;

	// Sort instances by view z
    x_qsort(s_InstList,			        // Address of first item in array.
			s_InstEntries,  	        // Number of items in array.
            sizeof(inst_entry),        // Size of one item.
			inst_entry_compare_fn) ;	// Compare function.

    // Draw all instances into material draw list
    for (i = 0 ; i < s_InstEntries ; i++)
    {
        // Get entry
        inst_entry&    Entry = s_InstList[i] ;

        // Get instance
        shape_instance*     Instance = Entry.Instance ;
        ASSERT(Instance) ;

        // Get draw info
        material_draw_info* DrawInfo = Entry.DrawInfo ;
        ASSERT(DrawInfo) ;

        // Lookup model
        model*              Model = Instance->GetModel() ;
        ASSERT(Model) ;

        // Draw model (which will add to material draw list)   
        Model->Draw(*DrawInfo) ;
    }

    // Clear instance draw list
    s_InstEntries = 0 ;
}

//==============================================================================

// Adds material to the draw list (doesn't do any drawing - that happens in the flush)
void shape::AddMaterialToDrawList(material& Material, material_draw_info& DrawInfo)
{
#if !defined(TARGET_LINUX)
    // Must be in begin draw and frame!
    ASSERT(InBeginDraw()) ;
    ASSERT(InBeginFrame()) ;

    // Can't be NULL!
    ASSERT(&Material) ;
    ASSERT(&DrawInfo) ;

    // Try allocate a new geometry draw entry
    geom_entry* GeomEntry = (geom_entry*)smem_BufferAlloc(sizeof(geom_entry)) ;
    if( !GeomEntry )
    {
        x_DebugMsg("Could not allocate GeomEntry!!!\n");
        return;
    }

    // Setup geometry draw entry
    s32 DrawListID = Material.LookupTextures(DrawInfo, GeomEntry->TexDrawInfo) ;
    GeomEntry->DrawInfo = &DrawInfo ;
    GeomEntry->Next     = NULL ;

    // You must assign an ID to the shape->model->materials
    // (if you use the shape_manager to load your shapes, it will assign them for you)
    ASSERT(DrawListID >= 0) ;
    ASSERT(DrawListID < DRAW_ENTRIES) ;

    ASSERT(Material.GetID() >= 0) ;
    ASSERT(Material.GetID() < MATERIAL_ENTRIES) ;

    // Choose draw list (materials that don't update z buffer should go in transparent list!)
    draw_list *DrawList ; 
    if (Material.GetPunchThroughFlag()) // Special case - it has alpha, but can use normal draw
        DrawList = &s_DrawList[0] ; // Opaque
    else
    if (Material.GetHasAlphaFlag())
        DrawList = &s_DrawList[1] ; // Transparent
    else
    if (Material.GetAdditiveFlag())
        DrawList = &s_DrawList[1] ; // Transparent
    else
    if (DrawInfo.Color[3] != 1.0f)
        DrawList = &s_DrawList[1] ; // Transparent
    else
        DrawList = &s_DrawList[0] ; // Opaque

    // Lookup entries
    material_entry* MaterialEntry = &DrawList->MatEntries[Material.GetID()] ;
    draw_entry*     DrawEntry     = &DrawList->DrawEntries[DrawListID] ;

    // If there is no draw entry for this material, create one and add it to the material list
    if (DrawEntry->GeomHead == NULL)
    {
        ASSERT(DrawEntry->GeomTail == NULL) ;

        // Setup 1st draw entry
        DrawEntry->GeomHead    = GeomEntry ;
        DrawEntry->GeomTail    = GeomEntry ;
        DrawEntry->Next        = NULL ;

        // If this material has not been used before, then create a new material entry
        // and add it to the main draw list
        if (MaterialEntry->Material == NULL)
        {
            // Add to empty list
            ASSERT(MaterialEntry->DrawHead == NULL) ;
            ASSERT(MaterialEntry->DrawTail == NULL) ;
            MaterialEntry->Material = &Material ;
            MaterialEntry->DrawHead = DrawEntry ;
            MaterialEntry->DrawTail = DrawEntry ;
            MaterialEntry->Next     = NULL ;

            // Add material into the big draw list material list
            if (DrawList->MatHead == NULL)
            {
                // Add to empty list
                ASSERT(DrawList->MatTail == NULL) ;
                DrawList->MatHead = MaterialEntry ;
                DrawList->MatTail = MaterialEntry ;
            }
            else
            {
                // Add to end of list
                ASSERT(DrawList->MatTail) ;
                DrawList->MatTail->Next = MaterialEntry ;
                DrawList->MatTail       = MaterialEntry ;
            }
        }
        else
        {
            // Add to end of material entry draw list
            ASSERT(MaterialEntry->Material == &Material) ;
            ASSERT(MaterialEntry->DrawHead) ;
            ASSERT(MaterialEntry->DrawTail) ;
            MaterialEntry->DrawTail->Next = DrawEntry ;
            MaterialEntry->DrawTail       = DrawEntry ;
        }
    }
    else
    {
        // Make sure material entry is setup!
        ASSERT(MaterialEntry->Material == &Material) ;

        // Simply add onto the end of the current geometry list
        ASSERT(DrawEntry->GeomHead) ;
        ASSERT(DrawEntry->GeomTail) ;

        DrawEntry->GeomTail->Next = GeomEntry ;
        DrawEntry->GeomTail       = GeomEntry ;
    }

    // Make sure material is valid!
    ASSERT(GeomEntry) ;
    ASSERT(GeomEntry->DrawInfo) ;
    ASSERT(DrawEntry) ;
    ASSERT(DrawEntry->GeomHead) ;
    ASSERT(DrawEntry->GeomTail) ;
    ASSERT(MaterialEntry) ;
    ASSERT(MaterialEntry->Material) ;
    ASSERT(MaterialEntry->Material == &Material) ;
    ASSERT(MaterialEntry->DrawHead) ;
    ASSERT(MaterialEntry->DrawTail) ;

    // Consolidate all mips levels into the first geom entries draw info for this material
    // since all these mips will need to be loaded anyway
    material_draw_info* HeadGeomDrawInfo = DrawEntry->GeomHead->DrawInfo ;
    ASSERT(HeadGeomDrawInfo) ;
    HeadGeomDrawInfo->MaxLOD = MAX(HeadGeomDrawInfo->MaxLOD, GeomEntry->DrawInfo->MaxLOD) ;
    HeadGeomDrawInfo->MinLOD = MIN(HeadGeomDrawInfo->MinLOD, GeomEntry->DrawInfo->MinLOD) ;
#endif
}

//==============================================================================

// Loops through all the draw entries and renders the materials
// (It only sets up data that has changed.)
void shape::FlushDrawList()
{
#if !defined(TARGET_LINUX)
    // Lookup vars
    draw_list*                      DrawList ;
    material_entry*                 MatEntry ;
    material*                       Material ;
    draw_entry*                     DrawEntry ;
    geom_entry*                     GeomEntry ;
    material_draw_info*             DrawInfo ;
    material_draw_info*             HeadDrawInfo ;
    texture_draw_info*              TexDrawInfo ;

    // Caching vars
    s32                             Pass ;
    render_node*                    CurrentRenderNodes = NULL ;
    s32                             CurrentLoadedNode  = -1 ;
    f32                             CurrentMipK ;
    u32                             CurrentFlags ;
    xbool                           ActivateColor ;
    vector4                         CurrentColor ;
    xcolor                          CurrentFogColor ;
    texture*                        DiffuseTexture ;
    texture*                        DetailTexture ;
    s32                             DetailClutHandle ;

    // Must be in begin draw and frame!
    ASSERT(InBeginDraw()) ;
    ASSERT(InBeginFrame()) ;

    // Sort and add "sorted" instances to the material list
    FlushInstanceDrawList() ;

    //======================================================================
    // Pass 0 = opaque materials, Pass 1 - transparent materials
    //======================================================================
    for (Pass = 0 ; Pass < 2 ; Pass++)
    {
        // Choose draw list
        DrawList = &s_DrawList[Pass] ;

        // Loop through all material entries
        MatEntry = DrawList->MatHead ;
        while(MatEntry)
        {
            // Lookup material to use
            Material = MatEntry->Material ;
            ASSERT(Material) ;

            // Lookup 1st draw entry
            DrawEntry = MatEntry->DrawHead ;
            ASSERT(DrawEntry) ;

            // Lookup 1st geometry entry
            GeomEntry = DrawEntry->GeomHead ;
            ASSERT(GeomEntry) ;

            // Lookup 1st draw info
            DrawInfo = GeomEntry->DrawInfo ;
            ASSERT(DrawInfo) ;

            // Lookup 1st texture draw info
            TexDrawInfo = &GeomEntry->TexDrawInfo ;
            ASSERT(TexDrawInfo) ;

            // Activate the common settings for this material
            // (will setup vram_SetMipEquation)
            Material->ActivateSettings(*DrawInfo, *TexDrawInfo) ;

            // On the PC, the load node, actually sets up the material self illum stuff
            #ifdef TARGET_PC
                CurrentLoadedNode = -1 ;
            #endif

            // Loop through all draw entries
            while(DrawEntry)
            {
                // Lookup 1st geometry entry
                GeomEntry = DrawEntry->GeomHead ;
                ASSERT(GeomEntry) ;

                // Lookup 1st draw info
                DrawInfo = GeomEntry->DrawInfo ;
                ASSERT(DrawInfo) ;

                // Keep head draw info since it contains all the mip levels
                HeadDrawInfo = DrawInfo ;
              
                // Lookup 1st texture draw info
                TexDrawInfo = &GeomEntry->TexDrawInfo ;
                ASSERT(TexDrawInfo) ;

                // Activate all the textures needed for these geometry entries
                // (will setup vram_SetMipEquation)
                Material->ActivateTextures(*DrawInfo, *TexDrawInfo) ;

                // Setup cache vars
                ActivateColor    = TRUE ;
                CurrentColor     = DrawInfo->Color ;
                CurrentFogColor  = DrawInfo->FogColor ;
                CurrentFlags     = DrawInfo->Flags ;
                CurrentMipK      = DrawInfo->MipK ;
                DiffuseTexture   = TexDrawInfo->Texture[material::TEXTURE_TYPE_DIFFUSE] ;
                DetailTexture    = TexDrawInfo->DetailTexture ;
                DetailClutHandle = TexDrawInfo->DetailClutHandle ;

                // Loop through all the geometry entries for this material combination
                // (only the render nodes, colors, mipk, and detail texture can be different between geom entries)
                while(GeomEntry)
                {
                    // Lookup draw info
                    DrawInfo = GeomEntry->DrawInfo ;
                    ASSERT(DrawInfo) ;

                    // SB: TEMP UNTIL I GET THIS SORTED!!!
                    DrawInfo->MinLOD = HeadDrawInfo->MinLOD ;
                    DrawInfo->MaxLOD = HeadDrawInfo->MaxLOD ;

                    // Corruption check - make sure the mip consolidation worked!
                    ASSERT(DrawInfo->MinLOD >= HeadDrawInfo->MinLOD) ;
                    ASSERT(DrawInfo->MaxLOD <= HeadDrawInfo->MaxLOD) ;

                    // Load new texture mip k?
                    if (CurrentMipK != DrawInfo->MipK)
                    {
                        CurrentMipK = DrawInfo->MipK ;

                        #ifdef TARGET_PS2

                            // Tell GS incase activate texture/color activate does not happen
                            if (DiffuseTexture)
                            {
                                eng_PushGSContext(0);
                                gsreg_Begin();
                                gsreg_SetMipEquation( FALSE, 
                                                      DrawInfo->MipK, 
                                                      DiffuseTexture->GetXBitmap().GetNMips(), 
                                                      MIP_MAG_BILINEAR, 
                                                      MIP_MIN_INTERP_MIPMAP_BILINEAR) ;
                                gsreg_End();
                                eng_PopGSContext();
                            }

                            // Mip map corruption fix - tell vram incase activate textures/color happends
                            // (since the vram_GetRegisters will return this in tex1!)
                            eng_PushGSContext(0);
                            vram_SetMipEquation(DrawInfo->MipK) ;
                            eng_PopGSContext();
                            eng_PushGSContext(1);
                            vram_SetMipEquation(DrawInfo->MipK) ;
                            eng_PopGSContext();

                            // We must download the colors again since this sets up all the gif tags
                            // for the texture registers (which contains the mipk info)
                            ActivateColor = TRUE ;

                        #endif
                    }

                    // Lookup texture draw info
                    TexDrawInfo = &GeomEntry->TexDrawInfo ;
                    ASSERT(TexDrawInfo) ;

                    // Reset current render node so that matrices are definitely sent?
                    if (CurrentRenderNodes != DrawInfo->RenderNodes)
                    {
                        CurrentRenderNodes  = DrawInfo->RenderNodes ;
                        CurrentLoadedNode   = -1 ;
                    }

                    // Special case render?
                    if (DrawInfo->RenderMaterialFunc)
                    {
                        // Call render function instead
                        DrawInfo->RenderMaterialFunc( *Material,
                                                      *TexDrawInfo,
                                                      *DrawInfo,
                                                      CurrentLoadedNode ) ;
                    }
                    else
                    {
                        // Re-load textures? (only the detail textures can be different)
                        if (        (DetailTexture    != TexDrawInfo->DetailTexture)
                                ||  (DetailClutHandle != TexDrawInfo->DetailClutHandle) )
                        {
                            DetailTexture    = TexDrawInfo->DetailTexture ;
                            DetailClutHandle = TexDrawInfo->DetailClutHandle ;

                            // If any textures are different, we need to reactivate them all
                            // (even if it's only the detail texture since it shares context0 with
                            //  the base diffuse - and for non-detail texture draws, context0 would otherwise
                            //  be left pointing at the detail texture - and show all black!)
                            Material->ActivateTextures(*DrawInfo, *TexDrawInfo) ;

                            // Force loading of color since PS2 will need to setup new material properties
                            ActivateColor = TRUE ;
                        }

                        // Prime z buffer?
                        if (DrawInfo->Flags & shape::DRAW_FLAG_PRIME_Z_BUFFER)
                        {
                            // Write material to z buffer only
                            Material->PrimeZBuffer(*DrawInfo, *TexDrawInfo, CurrentLoadedNode) ;

                            // Force loading of color since prime z buffer will have messed with them..
                            ActivateColor = TRUE ;
                        }

                        // Load new instance color/fog color?
                        if (        (ActivateColor) 
                                ||  (CurrentColor    != DrawInfo->Color)
                                ||  (CurrentFogColor != DrawInfo->FogColor)
                                ||  (CurrentFlags    != DrawInfo->Flags) )
                        {
                            ActivateColor   = FALSE ;
                            CurrentColor    = DrawInfo->Color ;
                            CurrentFogColor = DrawInfo->FogColor ;
                            CurrentFlags    = DrawInfo->Flags ;

                            Material->ActivateColor(*DrawInfo, *TexDrawInfo) ;
                        }

                        // Finally, draw the material!
                        Material->Draw(*DrawInfo, CurrentLoadedNode) ;
                    }

                    // Next geom entry
                    GeomEntry = GeomEntry->Next ;
                }

                // Clear draw entry
                DrawEntry->GeomHead = NULL ;
                DrawEntry->GeomTail = NULL ;

                // Next draw entry
                DrawEntry = DrawEntry->Next ;
            }
            
            // Clear mat entry
            MatEntry->Material = NULL ;
            MatEntry->DrawHead = NULL ;
            MatEntry->DrawTail = NULL ;

            // Next draw entry
            MatEntry = MatEntry->Next ;
        }

        // Clear draw list
        DrawList->MatHead = NULL ;
        DrawList->MatTail = NULL ;

        // Next pass...
    }
#endif
}

//==============================================================================
// DRAW FUNCTIONS
//==============================================================================

//==============================================================================

s32 shape::GetFrameCount()
{
    return s_FrameCount ;    
}

//==============================================================================

//==============================================================================
// VIEW FUNCTIONS
//==============================================================================

s32 shape::BBoxInViewVolume( const bbox& BBox )
{
    // Default to fully visible
    s32 ReturnValue = view::VISIBLE_FULL ;

    // Decide which planes to use.
    f32* pF = (f32*)&BBox;

    // Loop through planes looking for a trivial reject.
    for( s32 i=0; i<6; i++ )
    {
        // Get view plane
        shape::plane_info& PlaneInfo = s_ViewPlane[i] ;

        // Compute min and max dist along normal
        f32 MinDist = (PlaneInfo.Plane.Normal.X * pF[PlaneInfo.BBoxMinIndex[0]]) +
                      (PlaneInfo.Plane.Normal.Y * pF[PlaneInfo.BBoxMinIndex[1]]) +
                      (PlaneInfo.Plane.Normal.Z * pF[PlaneInfo.BBoxMinIndex[2]]) +
                      PlaneInfo.Plane.D;

        f32 MaxDist = (PlaneInfo.Plane.Normal.X * pF[PlaneInfo.BBoxMaxIndex[0]]) +
                      (PlaneInfo.Plane.Normal.Y * pF[PlaneInfo.BBoxMaxIndex[1]]) +
                      (PlaneInfo.Plane.Normal.Z * pF[PlaneInfo.BBoxMaxIndex[2]]) +
                      PlaneInfo.Plane.D;

        // If outside plane, we are culled.
        if( MaxDist < 0 )
            return( view::VISIBLE_NONE );

        // If partially out, remember.
        if( MinDist < 0 )
            ReturnValue = view::VISIBLE_PARTIAL ;
    }

    return( ReturnValue );
}

//==============================================================================

s32 shape::BBoxInClipVolume( const bbox& BBox )
{
    // Default to fully visible
    s32 ReturnValue = view::VISIBLE_FULL ;

    // Decide which planes to use.
    f32* pF = (f32*)&BBox;

    // Loop through planes looking for a trivial reject.
    for( s32 i=0; i<6; i++ )
    {
        // Get clip plane
        shape::plane_info& PlaneInfo = s_ClipPlane[i] ;

        // Compute min and max dist along normal
        f32 MinDist = (PlaneInfo.Plane.Normal.X * pF[PlaneInfo.BBoxMinIndex[0]]) +
                      (PlaneInfo.Plane.Normal.Y * pF[PlaneInfo.BBoxMinIndex[1]]) +
                      (PlaneInfo.Plane.Normal.Z * pF[PlaneInfo.BBoxMinIndex[2]]) +
                      PlaneInfo.Plane.D;

        f32 MaxDist = (PlaneInfo.Plane.Normal.X * pF[PlaneInfo.BBoxMaxIndex[0]]) +
                      (PlaneInfo.Plane.Normal.Y * pF[PlaneInfo.BBoxMaxIndex[1]]) +
                      (PlaneInfo.Plane.Normal.Z * pF[PlaneInfo.BBoxMaxIndex[2]]) +
                      PlaneInfo.Plane.D;

        // If outside plane, we are culled.
        if( MaxDist < 0 )
            return( view::VISIBLE_NONE );

        // If partially out, remember.
        if( MinDist < 0 )
            ReturnValue = view::VISIBLE_PARTIAL ;
    }

    return( ReturnValue );
}


//==============================================================================
// DEBUGGING FUNCTIONS
//==============================================================================

void shape::DefaultPrintf( const char* String )
{
    x_DebugMsg(String) ;
}

//==============================================================================

void shape::SetPrintfFunc(shape_printf_fn* fnPrintf)
{
    ASSERT(fnPrintf) ;
    s_PrintfFunc = fnPrintf ;
}

//==============================================================================

void shape::printf( const char* pFormatStr, ... )
{
    // Print?
    if (s_PrintfFunc)
    {
        x_va_list   Args;
        x_va_start( Args, pFormatStr );

        xvfs XVFS( pFormatStr, Args );

        s_PrintfFunc(XVFS) ;
    }
}

//==============================================================================

#if defined(TARGET_LINUX)
void shape::Init( void )
{
    ASSERT(s_Initialized == FALSE) ;
    s_Initialized = TRUE ;
}

//==============================================================================

void shape::Kill( void )
{
    s_Initialized = FALSE;
}

//==============================================================================
#endif

