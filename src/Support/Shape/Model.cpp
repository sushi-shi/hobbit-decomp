#include <xCore/Entropy/Entropy.hpp>
#include <Support/Shape/Model.hpp>
#include <Support/Shape/Shape.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include <Support/Shape/SelectionList.hpp>
#include <Support/Shape/TextureManager.hpp>


//==============================================================================
// Defines
//==============================================================================

#define PS2_USE_VU0



//==============================================================================
// Functions
//==============================================================================

// Default
model::model()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(model) & 15) == 0) ;

    // Initialize to be valid
    x_strcpy(m_Name, "undefined") ;
    
    m_Type              = 0 ;
    m_Weight            = 1.0f ;

    m_nNodes            = 0 ;
    m_Nodes             = NULL ;

    m_nVerts            = 0 ;
    m_Verts             = NULL ;
                        
    m_nPolys            = 0 ;
    m_Polys             = NULL ;

    m_nHotPoints        = 0 ;
    m_HotPoints         = NULL ;

    m_nTextureRefs      = 0 ;
    m_TextureRefs       = NULL ;

    m_nMaterials        = 0 ;
    m_Materials         = NULL ;

    m_bReflectionMapped = FALSE ;

    m_PrecisionScale    = 1.0f ;
   
    m_WorldPixelSize    = 0.001f ;  // Default

    m_nDListVerts       = 0 ;
    m_nDListPolys       = 0 ;
}

//==============================================================================

// Destructor
model::~model()
{
    if (m_Nodes)
        delete [] m_Nodes ;

    if (m_Verts)
        delete [] m_Verts ;

    if (m_Polys)
        delete [] m_Polys ;

    if (m_HotPoints)
        delete [] m_HotPoints ;

    if (m_TextureRefs)
        delete [] m_TextureRefs ;

    if (m_Materials)
        delete [] m_Materials ;
}

//==============================================================================

void model::SetName(const char *Name)
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

// Allocate nodes
void model::SetNodeCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old nodes
    if (m_Nodes)
    {
        delete [] m_Nodes ;
        m_Nodes = NULL ;
    }

    // Allocate new verts
    if (Count)
    {
        m_Nodes = new node[Count] ;
        ASSERT(m_Nodes) ;
    }

    m_nNodes = Count ;
}

//==============================================================================

// Allocate verts
void model::SetVertCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old verts
    if (m_Verts)
    {
        delete [] m_Verts ;
        m_Verts = NULL ;
    }

    // Allocate new verts
    if (Count)
    {
        m_Verts = new vert[Count] ;
        ASSERT(m_Verts) ;
    }

    m_nVerts = Count ;
}

//==============================================================================

// Allocate polys
void model::SetPolyCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old polys
    if (m_Polys)
    {
        delete [] m_Polys ;
        m_Polys = NULL ;
    }

    // Allocate new polys
    if (Count)
    {
        m_Polys = new poly[Count] ;
        ASSERT(m_Polys) ;
    }

    m_nPolys = Count ;
}

//==============================================================================

// Allocate hot points
void model::SetHotPointCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old hot_points
    if (m_HotPoints)
    {
        delete [] m_HotPoints ;
        m_HotPoints = NULL ;
    }

    // Allocate new hot_points
    if (Count)
    {
        m_HotPoints = new hot_point[Count] ;
        ASSERT(m_HotPoints) ;
    }

    m_nHotPoints = Count ;
}

//==============================================================================

// Allocate texture references
void model::SetTextureRefCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old materials
    if (m_TextureRefs)
    {
        delete [] m_TextureRefs ;
        m_TextureRefs = NULL ;
    }

    // Allocate new materials
    if (Count)
    {
        m_TextureRefs = new shape_texture_ref[Count] ;
        ASSERT(m_TextureRefs) ;
    }

    m_nTextureRefs = Count ;
}

//==============================================================================

// Adds and returns a new texture ref
s32 model::AddTextureRef(const char *Name)
{
    // See if it's already there
    for (s32 i = 0 ; i < m_nTextureRefs ; i++)
    {
        // Names match?
        if (x_stricmp(Name, m_TextureRefs[i].GetName()) == 0)
            return i ;
    }

    // Index is current count
    s32 Index = m_nTextureRefs ;

    // Create new array of textures refs
    shape_texture_ref* NewRefs = new shape_texture_ref[m_nTextureRefs+1] ;
    ASSERT(NewRefs) ;

    // Copy and delete the old list
    if (m_TextureRefs)
    {
        // Copy old list
        x_memcpy(NewRefs, m_TextureRefs, m_nTextureRefs * sizeof(shape_texture_ref)) ;
    
        // Delete old list
        delete [] m_TextureRefs ;
    }

    // Make the new list active
    m_TextureRefs = NewRefs ;
    m_nTextureRefs++ ;   

    // Return index
    return Index ;
}

//==============================================================================

// Allocate materials
void model::SetMaterialCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old materials
    if (m_Materials)
    {
        delete [] m_Materials ;
        m_Materials = NULL ;
    }

    // Allocate new materials
    if (Count)
    {
        m_Materials = new material[Count] ;
        ASSERT(m_Materials) ;
    }

    m_nMaterials = Count ;
}

//==============================================================================

void model::GetOrderedNodes(s32 NodeIndices[], s32 ArraySize, s32 NodeIndex, s32& Index)
{
    s32 i ;

    // Array full?
    if (Index >= ArraySize)
        return ;

    // Add this node    
    NodeIndices[Index++] = NodeIndex ;

    // Recurse on children
    for (i = 0 ; i < m_nNodes ; i++)
    {
        if (m_Nodes[i].GetParent() == NodeIndex)
            GetOrderedNodes(NodeIndices, ArraySize, i, Index) ;
    }
}

//==============================================================================

void model::GetOrderedNodes(s32 NodeIndices[], s32 ArraySize)
{
    s32 i, Index = 0 ;

    // Add all root nodes
    for (i = 0 ; i < m_nNodes ; i++)
    {
        if (m_Nodes[i].GetParent() == -1)
            GetOrderedNodes(NodeIndices, ArraySize, i, Index) ;
    }
}

//==============================================================================

// Returns TRUE if node is used by geometry or hot points etc
xbool model::IsNodeUsed(s32 Index)
{
    s32 i,j ;

    // Do not delete set on node?
    if (GetNode(Index).DoNotDelete())
        return TRUE ;

    // Used by any verts?
    for (i = 0 ; i < m_nVerts ; i++)
    {
        vert &Vert = GetVert(i) ;

        // Vert attached to node?
        if (Vert.GetNode() == Index)
            return TRUE ;
    }

    // Used by any hot points?
    for (i = 0 ; i < m_nHotPoints ; i++)
    {
        hot_point &HotPoint = GetHotPoint(i) ;

        // Hot point attached to node?
        if (HotPoint.GetNode() == Index)
            return TRUE ;
    }

    // Used by any materials?
    for (i = 0 ; i < m_nMaterials ; i++)
    {
        material &Mat = GetMaterial(i) ;

        // Check all dlists
        for (j = 0 ; j < Mat.GetDListCount() ; j++)
        {
            material_dlist &DList = Mat.GetDList(j) ;

            // Display list attached to node?
            if (DList.GetNode() == Index)
                return TRUE ;
        }
    }

    // Not used!
    return FALSE ;
}

//==============================================================================

// Removes node from list and updates hot point, dlist node indices etc
void model::DeleteNode(s32 Index)
{
    s32 i,j, NodeIndex ;

    // Make sure node is not used and index is valid
    ASSERT(!IsNodeUsed(Index)) ;
    ASSERT(Index >= 0) ;
    ASSERT(Index < GetNodeCount()) ;

    // Update parents to skip deleted node
    for (i = 0 ; i < m_nNodes ; i++)
    {
        node &Node = GetNode(i) ;

        if (Node.GetParent() == Index)
            Node.SetParent(m_Nodes[Index].GetParent()) ;

        ASSERT(Node.GetParent() != Index) ;
    }

    // Update nodes
    for (i = 0 ; i < m_nNodes ; i++)
    {
        node &Node = GetNode(i) ;

        // Update parent index?
        NodeIndex = Node.GetParent() ;
        if (NodeIndex > Index)
            Node.SetParent(NodeIndex-1) ;
    }

    // Update verts
    for (i = 0 ; i < m_nVerts ; i++)
    {
        vert &Vert = GetVert(i) ;

        // Update index?
        NodeIndex = Vert.GetNode() ;
        ASSERT(NodeIndex != Index) ;
        if (NodeIndex > Index)
            Vert.SetNode(NodeIndex-1) ;
    }

    // Update hot points
    for (i = 0 ; i < m_nHotPoints ; i++)
    {
        hot_point &HotPoint = GetHotPoint(i) ;

        // Update index?
        NodeIndex = HotPoint.GetNode() ;
        ASSERT(NodeIndex != Index) ;
        if (NodeIndex > Index)
            HotPoint.SetNode(NodeIndex-1) ;
    }

    // Update materials
    for (i = 0 ; i < m_nMaterials ; i++)
    {
        material &Mat = GetMaterial(i) ;

        // Check strips
        for (j = 0 ; j < Mat.GetStripCount() ; j++)
        {
            strip& Strip = Mat.GetStrip(j) ;

            // Update index?
            NodeIndex = Strip.GetNode() ;
            ASSERT(NodeIndex != Index) ;
            if (NodeIndex > Index)
                Strip.SetNode(NodeIndex-1) ;
        }

        // Check all dlists
        for (j = 0 ; j < Mat.GetDListCount() ; j++)
        {
            material_dlist &DList = Mat.GetDList(j) ;

            // Update index?
            NodeIndex = DList.GetNode() ;
            ASSERT(NodeIndex != Index) ;
            if (NodeIndex > Index)
                DList.SetNode(NodeIndex-1) ;
        }
    }

    // Create new node list
    s32   nNewNodes = GetNodeCount()-1 ;
    node *NewNodes = new node[nNewNodes] ;

    // Copy before deletion node
    for (i = 0 ; i < Index ; i++)
        NewNodes[i] = m_Nodes[i] ;

    // Copy after deletion node
    for (i = (Index+1) ; i < m_nNodes ; i++)
        NewNodes[i-1] = m_Nodes[i] ;

    // Delete old list of nodes
    delete [] m_Nodes ;

    // Finally - use the new nodes list
    m_Nodes  = NewNodes ;
    m_nNodes = nNewNodes ;

    // Make sure they are not all deleted!
    ASSERT(m_nNodes) ;
    ASSERT(m_Nodes) ;
}

//==============================================================================

// Call after parsing an ascii file
void model::CleanupUnusedMemory()
{
    // Keep memory before
    s32 MemBefore = MemoryUsed() ;

    // Free vertices
    if (m_Verts)
    {
        delete [] m_Verts ;
        m_nVerts = 0 ;
        m_Verts  = NULL ;
    }

    // Free polygons
    if (m_Polys)
    {
        delete [] m_Polys ;
        m_nPolys = 0 ;
        m_Polys  = NULL ;
    }

    // Free material
    for (s32 i = 0 ; i < m_nMaterials ; i++)
        m_Materials[i].CleanupUnusedMemory() ;

    // Get memory before
    s32 MemAfter = MemoryUsed() ;

    // Show stats
    shape::printf("   cleaning up memory.. before %dk after %dk\n", (MemBefore+1023)/1024, (MemAfter+1023)/1024) ;
}

//==============================================================================

// Returns total of memory used
s32 model::MemoryUsed()
{
    s32 i, Size = sizeof(model) ;

    for (i = 0 ; i < m_nNodes ; i++)
        Size += m_Nodes[i].MemoryUsed() ;

    for (i = 0 ; i < m_nVerts ; i++)
        Size += m_Verts[i].MemoryUsed() ;

    for (i = 0 ; i < m_nPolys ; i++)
        Size += m_Polys[i].MemoryUsed() ;
    
    for (i = 0 ; i < m_nMaterials ; i++)
        Size += m_Materials[i].MemoryUsed() ;
  
    return Size ;
}

//==============================================================================

// Returns # of polys that would be used in collision detection
s32 model::GetCollisionPolys()
{
//  random Random;
    s32 Polys=0;

    // Loop through all materials in model
    for (s32 cMat = 0 ; cMat < GetMaterialCount() ; cMat++)
    {
        material& Mat = GetMaterial(cMat) ;

        // Skip punch through materials
        if (!(Mat.GetPunchThroughFlag()))
        {
            // Loop through all display lists in material
            for (s32 cDList = 0 ; cDList < Mat.GetDListCount() ; cDList++)
            {
                material_dlist& MatDList = Mat.GetDList(cDList) ;

                // Loop through all strips in dlist
                for (s32 cStrip = 0 ; cStrip < MatDList.GetStripCount() ; cStrip++)
                {
                    dlist_strip& DListStrip = MatDList.GetStrip(cStrip) ;

                    // Loop through all polys in strip
                    for (s32 cPoly = 0 ; cPoly < DListStrip.GetPolyCount() ; cPoly++)
                    {
                        vector3 Verts[3] ;
            
                        // Grab poly and draw poly if it kicks a draw
                        if (DListStrip.GetPoly(cPoly, Verts))
                            Polys++ ;
                    }
                }
            }
        }
    }

    return Polys ;
}

//==============================================================================

// Compares material by alpha value
s32 material_compare_fn( const void* pItem1, const void* pItem2 )
{
	material *MatA = (material *)pItem1 ;
	material *MatB = (material *)pItem2 ;

    // Sort by alpha flag
	if (MatA->GetHasAlphaFlag() && !MatB->GetHasAlphaFlag()) return 1;
	if (!MatA->GetHasAlphaFlag() && MatB->GetHasAlphaFlag()) return -1;
    return 0;
}

// Sorts materials so that alpha materials are last
void model::SortMaterials(texture_manager &TextureManager)
{
	s32 i ;

	// Setup material alpha flags
	for (i = 0 ; i < m_nMaterials ; i++)
	{
		material	&Mat	 = m_Materials[i] ;
		f32         Alpha 	 = Mat.GetColor()[3] ;			

		s32 AlphaTex   = Mat.GetTextureInfo(material::TEXTURE_TYPE_ALPHA).Index ;
		s32 DiffuseTex = Mat.GetTextureInfo(material::TEXTURE_TYPE_DIFFUSE).Index ;
		s32 ReflectTex = Mat.GetTextureInfo(material::TEXTURE_TYPE_REFLECT).Index ;

		// Using diffuse texture?
        if (DiffuseTex != -1)
		{
            // Get texture
            texture* Tex = &TextureManager.GetTexture(DiffuseTex) ;
            ASSERT(Tex) ;

            xbool HasAlpha = Tex->HasAlpha() ;
            HasAlpha |= (AlphaTex != -1) ;

			// If texture has alpha (that's not used for per-pixel reflection)
            // then force a value of 0.5 to be used for the sort
			if ((HasAlpha) && (ReflectTex == -1))
			    Alpha = 0.5f ;
		}

		// Set flag
		Mat.SetHasAlphaFlag(Alpha != 1.0f) ;
	}

	// Sort materials by alpha
    if (m_nMaterials)
    {
        x_qsort(m_Materials,			// Address of first item in array.
			    m_nMaterials,			// Number of items in array.
                sizeof(material),		// Size of one item.
			    material_compare_fn) ;	// Compare function.
    }
}

//==============================================================================

// Loads all textures
void model::LoadTextures(texture_manager&           TextureManager, 
                         const shape_load_settings& LoadSettings)
{
    // Loop through all materials in the model
    for (s32 m = 0 ; m < GetMaterialCount() ; m++)
        GetMaterial(m).LoadTextures(*this, TextureManager, LoadSettings) ;
}

//==============================================================================

// Returns array of node matrices, given the anim and frame (and optional parent transformation matrix)
matrix4* model::GetNodeMatrices(anim& Anim, f32 Frame, matrix4* ParentTM /*=NULL*/)
{
    s32         i ;
    vector3     Pos ;
    quaternion  Rot ;

    // Declare statically
    static matrix4  Matrices[128] ;
    ASSERT(m_nNodes <= 128) ;

    // Hierarchies must match!
    ASSERT(Anim.GetKeySetCount() == m_nNodes) ;

    // Setup key access vars
    s32 tA, tB ;
    f32 Frac ;
    Anim.SetupKeyAccessVars(Frame, tA, tB, Frac) ;

    // Loop through all nodes
    for (i = 0 ; i < m_nNodes ; i++)
    {
        node    &Node   = GetNode(i) ;
        key_set &KeySet = Anim.GetKeySet(i) ;

        // Get keys frames for this frame
        KeySet.GetPosKey(Pos, tA, tB, Frac) ;
        KeySet.GetRotKey(Rot, tA, tB, Frac) ;

		// Build node matrix from animation keys
        Matrices[i].Identity() ;
		Matrices[i].Setup(Rot) ;
        Matrices[i].SetTranslation(Pos) ;

        // Take parent into account if there is one
        if (Node.GetParent() != -1)
        {
            ASSERT(Node.GetParent() < i) ;
            Matrices[i] = Matrices[Node.GetParent()] * Matrices[i] ;
        }
        else
        if (ParentTM)   // Take parent TM into account if there is one
            Matrices[i] = (*ParentTM) * Matrices[i] ;
    }

    return Matrices ;
}

//==============================================================================

// Setups bounds of model (assumes node bounds have already been setup)
void model::CalculateBounds(shape &Shape, shape_bin_file_class::target Target)
{
    s32 a,f,n,v ;

    // Reset total counds
    m_Bounds.Clear() ;

    //==========================================================================
    // Pass 1 - calculate node local bounds
    //==========================================================================

    // Calculate local bounds of all nodes
    for (n = 0 ; n < m_nNodes ; n++)
        m_Nodes[n].CalculateBounds(n, m_Verts, m_nVerts) ;

    //==========================================================================
    // Pass 2 - Loop through all animations and transform local node bounds into
    //          world bounds, then add to this models bounds
    //==========================================================================

    // Loop through all animations?
    if (Shape.GetAnimCount())
    {
        // Loop through all anims
        for (a = 0 ; a < Shape.GetAnimCount() ; a++)
        {
            anim &Anim = Shape.GetAnim(a) ;

            // Hierarchies must match!
            ASSERT(Anim.GetKeySetCount() == m_nNodes) ;

            // Disregard additive anims
            if (!Anim.GetAdditiveFlag())
            {
                // Loop through all frames of animation
                for (f = 0 ; f < Anim.GetFrameCount() ; f++)
                {
                    // Setup matrices for this anim frame
                    matrix4* NodeMatrices = GetNodeMatrices(Anim, (f32)f, NULL) ;

                    // Loop through all verts - keeps it tight!
                    for (v = 0 ; v < m_nVerts ; v++)
                    {
                        vert& Vert = m_Verts[v] ;

                        m_Bounds += NodeMatrices[Vert.GetNode()] * Vert.GetPos() ;
                    }

                    // Loop through all nodes - faster but not as tight
                    //for (n = 0 ; n < m_nNodes ; n++)
                    //{
                        //node& Node = m_Nodes[n] ;

                        // Transform node bounds into world
                        //bbox WorldNodeBounds = Node.GetBounds() ;
                        //WorldNodeBounds.Transform(NodeMatrices[n]) ;

                        // Add to model bounds
                        //m_Bounds += WorldNodeBounds ;
                    //}
                }
            }
        }
    }
    else
    {
        // There are no anims so just use node bounds
        for (n = 0 ; n < m_nNodes ; n++)
            m_Bounds += m_Nodes[n].GetBounds() ;
    }

    // PS2 build?
    if (Target == shape_bin_file_class::PS2)
    {
        // Setup precision scale so verts makes best use of 16bits for the position
        f32     Radius = 0.0f ;
        for (s32 v = 0 ; v < m_nVerts ; v++)
        {
            Radius = MAX(Radius, ABS(m_Verts[v].GetPos().X)) ;
            Radius = MAX(Radius, ABS(m_Verts[v].GetPos().Y)) ;
            Radius = MAX(Radius, ABS(m_Verts[v].GetPos().Z)) ;
        }
        m_PrecisionScale = 32767.0f / Radius ;
    }
}

//==============================================================================

// Computes world pixel size to be used for mip-mapping
void model::ComputeWorldPixelSize(texture_manager &TextureManager)
{
    vector3 V1,V2 ;
    vector2 T1,T2 ;
    f32     PolyArea, TexArea ;
    s32     nPolys ;
    
    // Prepare for taking average
    m_WorldPixelSize = 0.0f ;
    nPolys           = 0 ;

    f32 PolyAreaMin  = F32_MAX ;
    f32 TexAreaMin   = F32_MAX ;
    f32 PixelSizeMin = F32_MAX ;
    f32 PixelSizeMax = -F32_MAX ;

    // Loop through all polys
    for (s32 i = 0 ; i < m_nPolys ; i++)
    {
        poly &P = m_Polys[i] ;

        // Get material poly is using
        ASSERT(P.GetMaterial() >= 0) ;
        ASSERT(P.GetMaterial() < m_nMaterials) ;
        material &M = m_Materials[P.GetMaterial()] ;

        // Try using luminance texture first since this is higher res!
        s32 TexIndex = M.GetTextureInfo(material::TEXTURE_TYPE_LUMINANCE).Index ;

        // If texture has no luminance (ie. it's not compressed) then use the diffuse map
        if (TexIndex == -1)
            TexIndex = M.GetTextureInfo(material::TEXTURE_TYPE_DIFFUSE).Index ;

        // If no diffuse texture, try the reflect texture
        //if (TexIndex == -1)
            //TexIndex = M.GetTextureInfo(material::TEXTURE_TYPE_REFLECT).Index ;
        
        // Does this material use a texture?
        if (TexIndex != -1)
        {
            // Get texture
            texture &T = TextureManager.GetTexture(TexIndex) ;
            if ( (T.GetWidth()) && (T.GetHeight()) )
            {
                // Calculate area of polygon
                V1          = m_Verts[P.GetVert(2)].GetPos() - m_Verts[P.GetVert(1)].GetPos() ;
                V2          = m_Verts[P.GetVert(0)].GetPos() - m_Verts[P.GetVert(1)].GetPos() ;
                PolyArea    = v3_Cross(V1,V2).Length() * 0.5f ;

                // Calculate area of texture space used on this poly
                T1 = m_Verts[P.GetVert(2)].GetTexCoord() - m_Verts[P.GetVert(1)].GetTexCoord() ;
                T2 = m_Verts[P.GetVert(0)].GetTexCoord() - m_Verts[P.GetVert(1)].GetTexCoord() ;
                T1.X *= (float)T.GetWidth() ;
                T2.X *= (float)T.GetWidth() ;
                T1.Y *= (float)T.GetHeight() ;
                T2.Y *= (float)T.GetHeight() ;
                TexArea = v3_Cross(vector3(T1.X, T1.Y, 0.0f), vector3(T2.X, T2.Y, 0.0f)).Length() * 0.5f ;

                // Add ratio if areas are valid
                if ((PolyArea > 0) && (TexArea > 1.0f))
                {
                    PolyAreaMin   = MIN(PolyAreaMin, PolyArea) ;
                    TexAreaMin    = MIN(TexAreaMin,  TexArea) ;

                    f32 PixelSize = x_sqrt(PolyArea) / x_sqrt(TexArea) ;
                    PixelSizeMin = MIN(PixelSizeMin, PixelSize) ;
                    PixelSizeMax = MAX(PixelSizeMax, PixelSize) ;

                    m_WorldPixelSize += PixelSize ;
                    nPolys++ ;
                }
            }
        }
    }
    
    // Take average 
    if (nPolys)
        m_WorldPixelSize /= nPolys ;
    else
        m_WorldPixelSize = 0.001f ;   // Default

    // Show info
    shape::printf("WorldPixelSize:%f\n", m_WorldPixelSize) ;

}

//==============================================================================

// Builds display list ready to be referenced for drawing
void model::BuildDLists(shape_bin_file_class::target Target)
{
    s32 i ;

    shape::printf("\n\nBuilding model display lists...\n") ;

    // Make sure we haven't already built the display list
    ASSERT(m_nDListVerts == 0) ;
    ASSERT(m_nDListPolys == 0) ;

    // Scale verts to make max use of integer values
    for (i = 0 ; i < m_nVerts ; i++)
    {
        vert &V = m_Verts[i] ;
        V.SetPos(V.GetPos() * m_PrecisionScale) ;
    }
    
    // Put into node binding so that the shape is not altered
    //for (i = 0 ; i < m_nNodes ; i++)
        //m_Nodes[i].CalcBindMatrices() ;

    // Build material display lists
    s32 TotalSize = 0 ;
    for (i = 0 ; i < m_nMaterials ; i++)
    {
        material &M = m_Materials[i] ;

        // Count the total polys using this material
        s32 TotalPolys = 0 ;
        for (s32 j = 0 ; j < m_nPolys ; j++)
        {
            if (m_Polys[j].GetMaterial() == i)
                TotalPolys++ ;
        }

        // Build display list
        M.BuildDLists(TotalPolys, m_Verts, m_nVerts, m_nNodes, m_nDListVerts, m_nDListPolys, Target) ;
        TotalSize += M.GetDListSize() ;

        // If this material uses reflection mapping, then flag nodes
        if (M.GetTextureInfo(material::TEXTURE_TYPE_REFLECT).Index != -1)
        {
            // Flag model
            m_bReflectionMapped = TRUE ;

            // Loop through all strips in material
            for (s32 s = 0 ; s < M.GetStripCount() ; s++)
            {
                strip &S = M.GetStrip(s) ;
                
                // Loop through all verts in strip
                for (s32 v = 0 ; v < S.GetVertCount() ; v++)
                {
                    // Set node flag
                    s32 Node = m_Verts[S.GetVert(v).GetVertIndex()].GetNode() ;
                    ASSERT(Node >= 0) ;
                    ASSERT(Node < m_nNodes) ;
                    m_Nodes[Node].SetReflectionMapped(TRUE) ;
                }
            }
        }

        // Finally, get ready for drawing
        M.InitializeForDrawing() ;
    }

    // Show info
    shape::printf("TotalSize:%d bytes (%dk)\n\n", TotalSize, (TotalSize/1024)+1) ;
}

//==============================================================================

// Call after loading all model data
void model::InitializeForDrawing(shape&                         Shape, 
                                 texture_manager&               TextureManager, 
                                 shape_bin_file_class::target   Target)
{
    // Setup vars
    ComputeWorldPixelSize(TextureManager) ; // Before sorting materials, so we don't have to update polys
	SortMaterials(TextureManager) ;
    SortHotPointsByType() ; 
    CalculateBounds(Shape, Target) ;
    BuildDLists(Target) ;

    // Release unused memory
    CleanupUnusedMemory() ;
}

//==============================================================================

// Returns index of first node with given type if found, else -1
s32 model::GetNodeIndexByType(s32 Type)
{
    // Loop through all nodes
    for (s32 i = 0 ; i < m_nNodes ; i++)
    {
        // Does this node have a matching type?
        if (m_Nodes[i].GetType() == Type)
            return i ;
    }

    // Not found
    return -1 ;
}

//==============================================================================

// Returns list of texture indices for given node
s32 model::GetNodeTextures(s32 NodeIndex, s32 TextureIndices[], s32 ArraySize)
{
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < m_nNodes) ;
    ASSERT(ArraySize > 0) ;

    // Clear total
    s32 Total = 0 ;

    // Loop through all materials
    for (s32 i = 0 ; i < m_nMaterials ; i++)
    {
        material &Mat = m_Materials[i] ;

        // Loop through all display lists in material
        for (s32 j = 0 ; j < Mat.GetDListCount() ; j++)
        {
            material_dlist &DList = Mat.GetDList(j) ;

            // Is this material attached to the node?
            if (DList.GetNode() == NodeIndex)
            {
                // Use diffuse texture of material
                s32 TexIndex = Mat.GetTextureInfo(material::TEXTURE_TYPE_DIFFUSE).Index ;
                if (TexIndex != -1)
                {
                    // Search to see if this texture has already been added
                    xbool Found = FALSE ;
                    for (s32 k = 0 ; k < Total ; k++)
                    {
                        if (TextureIndices[k] == TexIndex)
                        {
                            Found = TRUE ;
                            break ;
                        }
                    }

                    // Add to list if it's not already there
                    if (!Found)
                    {
                        // Add to array
                        TextureIndices[Total++] = TexIndex ;

                        // Array full?
                        if (Total >= ArraySize)
                            return Total ;
                    }
                }
            }
        }
    }

    // Return total found
    return Total ;
}

//==============================================================================

// Returns number of children that a node has
s32 model::GetNodeChildCount(s32 NodeIndex)
{
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < m_nNodes) ;

    // Clear total
    s32 Total = 0 ;

    // Loop through all nodes
    for (s32 i = 0 ; i < m_nNodes ; i++)
    {
        // Is this node a child?
        if (m_Nodes[i].GetParent() == NodeIndex)
			Total++ ;
    }

    // Return total found
    return Total ;
}

//==============================================================================

// Returns given child index of node
s32 model::GetNodeChildIndex(s32 NodeIndex, s32 ChildIndex)
{
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < m_nNodes) ;

	ASSERT(ChildIndex >= 0) ;
	ASSERT(ChildIndex < GetNodeChildCount(NodeIndex)) ;

    // Loop through all nodes
    for (s32 i = 0 ; i < m_nNodes ; i++)
    {
        // Is this node a child?
        if (m_Nodes[i].GetParent() == NodeIndex)
		{
			// Reached requested child yet?
			if (--ChildIndex == -1)
				return i ;
		}
    }

	// Should never get here if node has children
	ASSERT(0) ;
	return -1 ;
}

//==============================================================================

// Returns list of node children indices for given node
s32 model::GetNodeChildren(s32 NodeIndex, s32 ChildrenIndices[], s32 ArraySize)
{
    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < m_nNodes) ;
    ASSERT(ArraySize > 0) ;

    // Get total
    s32 Count = MIN(ArraySize, GetNodeChildCount(NodeIndex)) ;

	// Fill in array
	for (s32 i = 0 ; i < Count ; i++)
		ChildrenIndices[i] = GetNodeChildIndex(NodeIndex, i) ;

    // Return total found
	ASSERT(Count <= GetNodeChildCount(NodeIndex)) ;
    return Count ;
}

//==============================================================================

// Returns number of descendent nodes (children, and their children's children etc)
s32 model::GetNodeDescendentCount(s32 NodeIndex)
{
	// Get total children for this node
	s32 Total = GetNodeChildCount(NodeIndex) ;

	// Recurse on children
	for (s32 i = 0 ; i < GetNodeChildCount(NodeIndex) ; i++)
		Total += GetNodeDescendentCount(GetNodeChildIndex(NodeIndex, i)) ;

	// Return total
	return Total ;
}

//==============================================================================

// Returns list of node children indices for given node
s32 model::GetNodeDescendents(s32 NodeIndex, s32 DescendentIndices[], s32 ArraySize, s32 &ArrayIndex)
{
	s32 Total = 0 ;

    ASSERT(NodeIndex >= 0) ;
    ASSERT(NodeIndex < m_nNodes) ;
    ASSERT(ArraySize > 0) ;

	// Get total children of this node
    s32 Count = GetNodeChildCount(NodeIndex) ;

    // Add children of current node to array
    for (s32 i = 0 ; i < Count ; i++)
	{
		// Array full?
		if (ArrayIndex == ArraySize)
			return Total ;

		// Get child
		s32 ChildNodeIndex = GetNodeChildIndex(NodeIndex, i) ;

		// Add to array
		DescendentIndices[ArrayIndex++] = ChildNodeIndex ;
		Total++ ;

		// Recursively adds children
		Total += GetNodeDescendents(ChildNodeIndex, DescendentIndices, ArraySize, ArrayIndex) ;
	}

    // Return total added
    return Total ;
}

//==============================================================================

// Returns list of node children indices for given node
s32 model::GetNodeDescendents(s32 NodeIndex, s32 DescendentIndices[], s32 ArraySize)
{
	s32 ArrayIndex = 0 ;

	// Call recursive version
	return GetNodeDescendents(NodeIndex, DescendentIndices, ArraySize, ArrayIndex) ;
}

//==============================================================================

// Compares hot_points by their types
s32 hot_point_type_compare_fn( const void* pItem1, const void* pItem2 )
{
	hot_point *pHotPointA = (hot_point *)pItem1 ;
	hot_point *pHotPointB = (hot_point *)pItem2 ;

    // Sort by type
	if (pHotPointA->GetType() > pHotPointB->GetType()) return 1;
	if (pHotPointA->GetType() < pHotPointB->GetType()) return -1;
    return 0;
}

// Sorts hot_points by type for faster lookup
void model::SortHotPointsByType(void)
{
	// Sort hot_points by type
    if (m_nHotPoints)
    {
        // Sort
        x_qsort(m_HotPoints,   			    // Address of first item in array.
			    m_nHotPoints,	            // Number of items in array.
                sizeof(hot_point), 		    // Size of one item.
			    hot_point_type_compare_fn) ;	// Compare function.
    }
}

//==============================================================================

// Returns requested hot point
hot_point *model::GetHotPointByIndex(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nHotPoints) ;

    return &m_HotPoints[Index] ;
}

//==============================================================================

// Returns pointer to given hot point if found, else NULL
hot_point *model::GetHotPointByType(s32 Type)
{
    // Do binary search...
    s32             Lo, Mid, Hi ;
    hot_point*      HotPoint ;

    // Must be some hot points!
    if (!m_nHotPoints)
        return NULL ;

    // Start at array limits
    Lo = 0 ;
    Hi = m_nHotPoints-1 ;

    // Loop until we get the closest type
    while(Lo < Hi)
    {
        // Lookup mid hot point
        Mid      = (Lo + Hi) >> 1 ;
        HotPoint = &m_HotPoints[Mid] ;
        ASSERT(HotPoint) ;

        // Chop search in half
        if (HotPoint->GetType() < Type)
            Lo = Mid+1 ;
        else
            Hi = Mid ;
    }

    // Found the hot point?
    HotPoint = &m_HotPoints[Lo] ;
    ASSERT(HotPoint) ;
    if (HotPoint->GetType() == Type)
    {
        // There may be more than one hot point assigned the same type,
        // so search backwards for the first occurance..
        while((Lo > 0) && (m_HotPoints[Lo-1].GetType() == Type))
            Lo-- ;

        // Get first hot point of this type...
        ASSERT(Lo >= 0) ;
        HotPoint = &m_HotPoints[Lo] ;
        ASSERT(HotPoint->GetType() == Type) ;

        // Add hot points of requested type to the selection list
        selection_list  List ;
        while((Lo < m_nHotPoints) && (HotPoint->GetType() == Type))
        {
            List.AddElement(Lo, 1.0f) ;
            
            // Next hot point
            Lo++ ;
            HotPoint++ ;
        }

        // Finally, choose from list
        ASSERT(List.GetCount()) ;
        return &m_HotPoints[List.ChooseElement()] ;
    }
    else
        return NULL ;

    /*
    s32             i ;
    selection_list  List ;

    // Add hot points of requested type to the selection list
    for (i = 0 ; i < m_nHotPoints ; i++)
    {
        hot_point &HotPoint = m_HotPoints[i] ;
        if (HotPoint.GetType() == Type)
            List.AddElement(i, HotPoint.GetWeight()) ;
    }

    // Not found?
    if (List.GetCount() == 0)
        return NULL ;

    // Choose from list
    return &m_HotPoints[List.ChooseElement()] ;
    */
}

//==============================================================================

// Returns index of first node with given name if found, else -1
s32 model::GetMaterialIndexByName(const char *Name)
{
    // Loop through all materials
    for (s32 i = 0 ; i < m_nMaterials ; i++)
    {
        // Found?
        if (x_strcmp(m_Materials[i].GetName(), Name) == 0)
            return i ;
    }

    // Not found
    return -1 ;
}

//==============================================================================
#if 0

#ifdef TARGET_PS2

// Sets up bind info ready for micro code to use
render_node *model::CalculateRenderNodes(shape_instance&    Instance,
                                         u32                Flags,
                                         matrix4*           L2W, 
                                         radian3&           RefRot,
                                         void*              NodeFuncParam,
                                         render_node_fn*    RenderNodeFunc)
{
    ASSERT((sizeof(render_node)/16) == NODE_SIZE) ;

	// Allocate render nodes
	render_node  *RenderNodes = (render_node *)smem_BufferAlloc(sizeof(render_node) * m_nNodes) ;
	if (!RenderNodes)
    {
        x_DebugMsg("Could not allocate RenderNodes!!!\n");
		return NULL ;
    }

    // Get current view
    const view *View = eng_GetView() ;

    // Get useful matrices
    static matrix4 W2V PS2_ALIGNMENT(16);
    W2V = View->GetW2V() ;

    static matrix4 mLook PS2_ALIGNMENT(16);
    static vector4 RefLook, RefUp, RefRight ;

    // Get light info
    static vector4 LightDir PS2_ALIGNMENT(16);
    static vector4 LightCol PS2_ALIGNMENT(16);
    static vector4 LightAmb PS2_ALIGNMENT(16);

    // Get light direction
    LightDir.X = -Instance.GetLightDirection().X ;
    LightDir.Y = -Instance.GetLightDirection().Y ;
    LightDir.Z = -Instance.GetLightDirection().Z ;
    LightDir.W = 0 ;
    
    // Turn off lighting?
    if (Flags & shape::DRAW_FLAG_TURN_OFF_LIGHTING)
    {
        // Since lighting can't be turned off, use full ambient and no diffuse!
        LightCol = vector4(0,0,0,0) ;
        LightAmb = vector4(1,1,1,1) ;
    }
    else
    {
        // Use normal lighting
        LightCol = Instance.GetLightColor() ;
        LightCol.W = 0 ;

        LightAmb = Instance.GetLightAmbientColor() ;
    }

    // Map ready for vu code
    LightAmb *= 127.0f ;
    LightAmb.W = 128.0f ;

    static xbool s_Swap=FALSE ;

    // Setup env map look matrix so that it moves as the object moves
    if (m_bReflectionMapped)
    {
        // Use specified rotation?
        if (Flags & shape::DRAW_FLAG_REF_SPECIFIED)
        {
            // Setup specified rotation
            #ifdef PS2_USE_VU0
                sceVu0UnitMatrix((sceVu0FMATRIX)(&mLook)) ;
            #else
                mLook.Identity() ;
            #endif

            mLook.RotateY(RefRot.Yaw) ;
            mLook.RotateX(RefRot.Pitch) ;
            mLook.RotateZ(RefRot.Roll) ;
        }
        else
        // Use a random look?
        if (Flags & shape::DRAW_FLAG_REF_RANDOM)
        {
            // Setup random rotation
            #ifdef PS2_USE_VU0
                sceVu0UnitMatrix((sceVu0FMATRIX)(&mLook)) ;
            #else
                mLook.Identity() ;
            #endif

            mLook.SetRotation(radian3(x_frand(-R_180, R_180), x_frand(-R_180, R_180), x_frand(-R_180, R_180))) ;
        }
        else
        {
            // Get object pos in camera space
            //vector3 vObjPos = W2V * L2W[0].GetTranslation() ;
            vector3 vObjPos = W2V * Instance.GetPos() ;

            // Get view components
            vector3 LookOffset = vObjPos ;
            LookOffset.Normalize() ;

            vector3 UpOffset    = v3_Cross(LookOffset, vector3(1,0,0)) ;
            vector3 RightOffset = v3_Cross(UpOffset, LookOffset) ;

            // Setup look matrix (used to rotate co-ords so that environment map moves when object does)
            mLook(0, 0) = RightOffset.X; mLook(0, 1) = UpOffset.X;  mLook(0, 2) = LookOffset.X;  mLook(0, 3) = 0.0f;
            mLook(1, 0) = RightOffset.Y; mLook(1, 1) = UpOffset.Y;  mLook(1, 2) = LookOffset.Y;  mLook(1, 3) = 0.0f;
            mLook(2, 0) = RightOffset.Z; mLook(2, 1) = UpOffset.Z;  mLook(2, 2) = LookOffset.Z;  mLook(2, 3) = 0.0f;
            mLook(3, 0) = mLook(3, 1) = mLook(3, 2) = mLook(3, 3) = 0.0f ;

            // Reflect in view space?
            if (!(Flags & shape::DRAW_FLAG_REF_WORLD_SPACE))
                mLook = mLook * W2V ;
        }

        // Make range 0->0.5f / 127.0f (normals are *127)
        // NOTE: the - sign is so that the relfection map orientates like the map itself
        // (that way you can use a sky texture etc)
        if (s_Swap)
            mLook.PreScale(-0.5 / 127.0f) ;
    }

    // Perform world->screen
    for (s32 i = 0 ; i < m_nNodes ; i++)
    {
        node         &Node       = m_Nodes[i] ;
        render_node  &RenderNode = RenderNodes[i] ;

        static matrix4 L2V         PS2_ALIGNMENT(16);
        static matrix4 W2L         PS2_ALIGNMENT(16);
        static matrix4 mReflect    PS2_ALIGNMENT(16);
        static vector4 InvLightDir PS2_ALIGNMENT(16);

        ASSERT(ALIGN_16(&RenderNode.mL2W)) ;
        ASSERT(ALIGN_16(&W2L)) ;
        ASSERT(ALIGN_16(&mReflect)) ;
        ASSERT(ALIGN_16(&mLook)) ;
        ASSERT(ALIGN_16(&L2W[i])) ;
        ASSERT(ALIGN_16(&L2V)) ;
        ASSERT(ALIGN_16(&W2V)) ;
        ASSERT(ALIGN_16(&InvLightDir)) ;
        ASSERT(ALIGN_16(&LightDir)) ;
        ASSERT(ALIGN_16(&RefLook)) ;
        ASSERT(ALIGN_16(&RefUp)) ;
        ASSERT(ALIGN_16(&RefRight)) ;

        // Setup node L2W matrix-
        // For the PS2, the model verts are scaled up to take the full 16 bit integer range
        // (since the local x,y,z are 16bits each). We need to inverse this scale in the matrices
        #ifdef PS2_USE_VU0
            sceVu0CopyMatrix((sceVu0FMATRIX)(&RenderNode.mL2W), (sceVu0FMATRIX)(&L2W[i]) ) ;
        #else            
            RenderNode.mL2W = L2W[i] ;
        #endif

        // Setup env map matrix
        if (Node.IsReflectionMapped())
        {
            // Setup reflection map matrix
            if (Flags & shape::DRAW_FLAG_REF_SPECIFIED)
            {
                // Use specified
                #ifdef PS2_USE_VU0
                    sceVu0CopyMatrix((sceVu0FMATRIX)(&mReflect), (sceVu0FMATRIX)(&mLook)) ;
                #else
                    mReflect = mLook ;
                #endif
            }
            else
            {
                // Reflect in node world/view space
                #ifdef PS2_USE_VU0
                    sceVu0MulMatrix((sceVu0FMATRIX)(&mReflect), (sceVu0FMATRIX)(&mLook), (sceVu0FMATRIX)(&L2W[i])) ;
                #else
	                mReflect = mLook * L2W[i] ;
                #endif
            }

            if (s_Swap)
            {
                // Put into node info
                RenderNode.mRefUV(0, 0) = mReflect(0,0) ;
                RenderNode.mRefUV(1, 0) = mReflect(1,0) ;
                RenderNode.mRefUV(2, 0) = mReflect(2,0) ;
                RenderNode.mRefUV(0, 1) = mReflect(0,1) ;
                RenderNode.mRefUV(1, 1) = mReflect(1,1) ;
                RenderNode.mRefUV(2, 1) = mReflect(2,1) ;
                RenderNode.mRefUV(0, 2) = mReflect(0,2) ;
                RenderNode.mRefUV(1, 2) = mReflect(1,2) ;
                RenderNode.mRefUV(2, 2) = mReflect(2,2) ;
                RenderNode.mRefUV(0, 3) = 0 ;
                RenderNode.mRefUV(1, 3) = 0 ;
                RenderNode.mRefUV(2, 3) = 0 ;
            }
            else
            {
                RefRight.X  = mReflect(0, 0) ;
                RefRight.Y  = mReflect(1, 0) ;
                RefRight.Z  = mReflect(2, 0) ;
                RefRight.W  = 0 ;

                RefUp.X     = mReflect(0, 1) ;
                RefUp.Y     = mReflect(1, 1) ;
                RefUp.Z     = mReflect(2, 1) ;
                RefUp.W     = 0 ;

                RefLook.X   = mReflect(0, 2) ;
                RefLook.Y   = mReflect(1, 2) ;
                RefLook.Z   = mReflect(2, 2) ;
                RefLook.W   = 0 ;

                // Make range 0->0.5f / 127.0f (normals are *127)
                // NOTE: the - sign is so that the relfection map orientates like the map itself
                // (that way you can use a sky texture etc)
                #ifdef PS2_USE_VU0
                    sceVu0Normalize((sceVu0FVECTOR)(&RefRight), (sceVu0FVECTOR)(&RefRight)) ;
                    sceVu0ScaleVector((sceVu0FVECTOR)(&RefRight), (sceVu0FVECTOR)(&RefRight), -0.5f / 127.0f) ;

                    sceVu0Normalize((sceVu0FVECTOR)(&RefUp), (sceVu0FVECTOR)(&RefUp)) ;
                    sceVu0ScaleVector((sceVu0FVECTOR)(&RefUp), (sceVu0FVECTOR)(&RefUp), -0.5f / 127.0f) ;

                    sceVu0Normalize((sceVu0FVECTOR)(&RefLook), (sceVu0FVECTOR)(&RefLook)) ;
                    sceVu0ScaleVector((sceVu0FVECTOR)(&RefLook), (sceVu0FVECTOR)(&RefLook), -0.5f / 127.0f) ;
                #else
                    RefRight.NormalizeAndScale(-0.5f / 127.0f) ;
                    RefUp.NormalizeAndScale(-0.5f / 127.0f) ;
                    RefLook.NormalizeAndScale(-0.5f / 127.0f) ;
                #endif

                // Put into node info
                RenderNode.mRefUV(0, 0) = RefRight.X ;
                RenderNode.mRefUV(0, 1) = RefUp.X ;
                RenderNode.mRefUV(0, 2) = RefLook.X ;

                RenderNode.mRefUV(1, 0) = RefRight.Y ;
                RenderNode.mRefUV(1, 1) = RefUp.Y ;
                RenderNode.mRefUV(1, 2) = RefLook.Y ;
                
                RenderNode.mRefUV(2, 0) = RefRight.Z ;
                RenderNode.mRefUV(2, 1) = RefUp.Z ;
                RenderNode.mRefUV(2, 2) = RefLook.Z ;
                
                RenderNode.mRefUV(0, 3) = 0 ;
                RenderNode.mRefUV(1, 3) = 0 ;
                RenderNode.mRefUV(2, 3) = 0 ;
            }
        }

        // Setup light
        #ifdef PS2_USE_VU0
    		//sceVu0TransposeMatrix((sceVu0FMATRIX)(&W2L), (sceVu0FMATRIX)(&L2W[i])) ;
            //sceVu0ApplyMatrix((sceVu0FVECTOR)(&InvLightDir), (sceVu0FMATRIX)(&W2L), (sceVu0FVECTOR)(&LightDir)) ;
            //InvLightDir.W = 0 ;
            //sceVu0Normalize((sceVu0FVECTOR)(&InvLightDir), (sceVu0FVECTOR)(&InvLightDir)) ;
            //sceVu0ScaleVector((sceVu0FVECTOR)&RenderNode.mLight(0,0), (sceVu0FVECTOR)(&LightCol), InvLightDir.X) ;
            //sceVu0ScaleVector((sceVu0FVECTOR)&RenderNode.mLight(1,0), (sceVu0FVECTOR)(&LightCol), InvLightDir.Y) ;
            //sceVu0ScaleVector((sceVu0FVECTOR)&RenderNode.mLight(2,0), (sceVu0FVECTOR)(&LightCol), InvLightDir.Z) ;
            //sceVu0CopyVector((sceVu0FVECTOR)&RenderNode.mLight(3,0), (sceVu0FVECTOR)(&LightAmb)) ;

            RenderNode.mLight(0,0) = LightDir.X ;
            RenderNode.mLight(0,1) = LightDir.Y ;
            RenderNode.mLight(0,2) = LightDir.Z ;
            RenderNode.mLight(0,3) = 0 ;

            RenderNode.mLight(1,0) = LightCol.X ;
            RenderNode.mLight(1,1) = LightCol.Y ;
            RenderNode.mLight(1,2) = LightCol.Z ;
            RenderNode.mLight(1,3) = 0 ;

            RenderNode.mLight(2,0) = LightAmb.X ;
            RenderNode.mLight(2,1) = LightAmb.Y ;
            RenderNode.mLight(2,2) = LightAmb.Z ;
            RenderNode.mLight(2,3) = LightAmb.W ;

        #else
            InvLightDir.X  = (L2W[i](0, 0) * LightDir.X) + (L2W[i](0, 1) * LightDir.Y) + (L2W[i](0, 2) * LightDir.Z) ;
            InvLightDir.Y  = (L2W[i](1, 0) * LightDir.X) + (L2W[i](1, 1) * LightDir.Y) + (L2W[i](1, 2) * LightDir.Z) ;
            InvLightDir.Z  = (L2W[i](2, 0) * LightDir.X) + (L2W[i](2, 1) * LightDir.Y) + (L2W[i](2, 2) * LightDir.Z) ;
            InvLightDir.W  = 0.0f ;
            InvLightDir.Normalize() ;

            RenderNode.mLight(0,0) = InvLightDir.X * LightCol.X ;
            RenderNode.mLight(0,1) = InvLightDir.X * LightCol.Y ;
            RenderNode.mLight(0,2) = InvLightDir.X * LightCol.Z ;
            RenderNode.mLight(0,3) = 0 ;

            RenderNode.mLight(1,0) = InvLightDir.Y * LightCol.X ;
            RenderNode.mLight(1,1) = InvLightDir.Y * LightCol.Y ;
            RenderNode.mLight(1,2) = InvLightDir.Y * LightCol.Z ;
            RenderNode.mLight(1,3) = 0 ;

            RenderNode.mLight(2,0) = InvLightDir.Z * LightCol.X ;
            RenderNode.mLight(2,1) = InvLightDir.Z * LightCol.Y ;
            RenderNode.mLight(2,2) = InvLightDir.Z * LightCol.Z ;
            RenderNode.mLight(2,3) = 0 ;

            RenderNode.mLight(3,0) = LightAmb.X ;
            RenderNode.mLight(3,1) = LightAmb.Y ;
            RenderNode.mLight(3,2) = LightAmb.Z ;
            RenderNode.mLight(3,3) = LightAmb.W ;
        #endif

        // Call render node function?
        if (RenderNodeFunc)
            RenderNodeFunc(NodeFuncParam, Node, RenderNode) ;
    }

    // Success!
    return RenderNodes ;
}

#endif  //#ifdef TARGET_PS2

//==============================================================================

#if defined(TARGET_PC)||defined(TARGET_LINUX)

render_node *model::CalculateRenderNodes(shape_instance&    Instance,
                                         u32                Flags,
                                         matrix4*           L2W, 
                                         radian3&           RefRot,
                                         void*              NodeFuncParam,
                                         render_node_fn*    RenderNodeFunc)
{
	// Allocate render nodes
	render_node  *RenderNodes = (render_node *)smem_BufferAlloc(sizeof(render_node) * m_nNodes) ;
	if (!RenderNodes)
    {
        x_DebugMsg("Could not allocate RenderNodes!!!\n");
		return NULL ;
    }

    // Get current view
    const view *View = eng_GetView() ;

    // Get useful matrices
    matrix4 W2S, W2V ;
    W2S = View->GetW2S() ;
    W2V = View->GetW2V() ;

    matrix4 mLook ;// __attribute__ ((aligned(16))) ;
    vector3 vLook, vUp, vRight ;

    // Setup env map look matrix so that it moves as the object moves
    if (m_bReflectionMapped)
    {
        // Use specified rotation?
        if (Flags & shape::DRAW_FLAG_REF_SPECIFIED)
        {
            // Setup specified rotation
            mLook.Identity() ;
            mLook.RotateY(RefRot.Yaw) ;
            mLook.RotateX(RefRot.Pitch) ;
            mLook.RotateZ(RefRot.Roll) ;
        }
        else
        // Use a random look?
        if (Flags & shape::DRAW_FLAG_REF_RANDOM)
        {
            // Setup random rotation
            mLook.Identity() ;
            mLook.SetRotation(radian3(x_frand(-R_180, R_180), x_frand(-R_180, R_180), x_frand(-R_180, R_180))) ;
        }
        else
        {
            // Get object pos in camera space
            //vector3 vObjPos = W2V * L2W[0].GetTranslation() ;
            vector3 vObjPos = W2V * Instance.GetPos() ;

            // Get view components
            vLook  = vObjPos ;
            vLook.Normalize() ;
            vUp    = v3_Cross(vLook, vector3(1,0,0)) ;
            vRight = v3_Cross(vUp, vLook) ;

            // Setup look matrix (used to rotate co-ords so that environment map moves when object does)
            mLook(0, 0) = vRight.X; mLook(0, 1) = vUp.X;  mLook(0, 2) = vLook.X;  mLook(0, 3) = 0.0f;
            mLook(1, 0) = vRight.Y; mLook(1, 1) = vUp.Y;  mLook(1, 2) = vLook.Y;  mLook(1, 3) = 0.0f;
            mLook(2, 0) = vRight.Z; mLook(2, 1) = vUp.Z;  mLook(2, 2) = vLook.Z;  mLook(2, 3) = 0.0f;
            mLook(3, 0) = mLook(3, 1) = mLook(3, 2) = mLook(3, 3) = 0.0f ;
        }
    }

    // Perform world->screen
    for (s32 i = 0 ; i < m_nNodes ; i++)
    {
        node        &Node       = m_Nodes[i] ;
        render_node &RenderNode = RenderNodes[i] ;

        matrix4 L2V ;
        matrix4 W2L ;
        matrix4 mReflect ;

        // Setup local->view
        RenderNode.mL2W = L2W[i] ;

        // Setup env map matrix
        if (Node.IsReflectionMapped())
        {
            // Setup reflection map matrix
            if (Flags & shape::DRAW_FLAG_REF_SPECIFIED)
            {
                // Use specified
                mReflect = mLook ;
            }
            else
            if (Flags & shape::DRAW_FLAG_REF_WORLD_SPACE)
            {
                // Reflect in node world space
                mReflect = mLook * L2W[i] ;
            }
            else
            {
                // Reflect in view space
                L2V = W2V * L2W[i] ;
                mReflect = mLook * L2V ;
            }

            vRight.X = mReflect(0, 0) ;
            vRight.Y = mReflect(1, 0) ;
            vRight.Z = mReflect(2, 0) ;

            vUp.X = mReflect(0, 1) ;
            vUp.Y = mReflect(1, 1) ;
            vUp.Z = mReflect(2, 1) ;

            vLook.X = mReflect(0, 2) ;
            vLook.Y = mReflect(1, 2) ;
            vLook.Z = mReflect(2, 2) ;

            // Make range 0->0.5f
            // NOTE: the - sign is so that the relfection map orientates like the map itself
            // (that way you can use a sky texture etc)
            vRight.NormalizeAndScale(-0.5f) ;
            vUp.NormalizeAndScale(-0.5f) ;
            vLook.NormalizeAndScale(-0.5f) ;

            // Put into node info
            RenderNode.mRefUV(0, 0) = vRight.X ;
            RenderNode.mRefUV(1, 0) = vRight.Y ;
            RenderNode.mRefUV(2, 0) = vRight.Z ;
            
			RenderNode.mRefUV(0, 1) = vUp.X ;
            RenderNode.mRefUV(1, 1) = vUp.Y ;
            RenderNode.mRefUV(2, 1) = vUp.Z ;
            
			RenderNode.mRefUV(0, 2) = vLook.X ;
            RenderNode.mRefUV(1, 2) = vLook.Y ;
            RenderNode.mRefUV(2, 2) = vLook.Z ;

			RenderNode.mRefUV(3, 0) = 0.5f ;	// tx (makes final u 0->1)
			RenderNode.mRefUV(3, 1) = 0.5f ;	// ty (makes final v 0->1)
			RenderNode.mRefUV(3, 2) = 0.0f ;
			RenderNode.mRefUV(3, 3) = 1.0f ;

			RenderNode.mRefUV(0, 3) = 0.0f ;
			RenderNode.mRefUV(1, 3) = 0.0f ;
			RenderNode.mRefUV(2, 3) = 0.0f ;
        }

        // Get light info
        vector3 LightDir = Instance.GetLightDirection() ;
        vector4 LightCol = Instance.GetLightColor() ;
        vector4 LightAmb = Instance.GetLightAmbientColor() ;

        // Turn off lighting?
        if (Flags & shape::DRAW_FLAG_TURN_OFF_LIGHTING)
        {
            // Since lighting can't be turned off, use full ambient and no diffuse!
            LightCol = vector4(0,0,0,1) ;
            LightAmb = vector4(1,1,1,1) ;
        }
        
        // Plop into light matrix (no need to inverse rotate the light for the PC)
        RenderNode.mLight(0,0) = LightDir.X ;
        RenderNode.mLight(0,1) = LightDir.Y ;
        RenderNode.mLight(0,2) = LightDir.Z ;
        RenderNode.mLight(0,3) = 0 ;

        RenderNode.mLight(1,0) = LightCol.X ;
        RenderNode.mLight(1,1) = LightCol.Y ;
        RenderNode.mLight(1,2) = LightCol.Z ;
        RenderNode.mLight(1,3) = LightCol.W ;

        RenderNode.mLight(2,0) = LightAmb.X ;
        RenderNode.mLight(2,1) = LightAmb.Y ;
        RenderNode.mLight(2,2) = LightAmb.Z ;
        RenderNode.mLight(2,3) = LightAmb.Z ;

        // Call render node function?
        if (RenderNodeFunc)
            RenderNodeFunc(NodeFuncParam, Node, RenderNode) ;
    }

    // Success!
    return RenderNodes ;
}


#endif

//==============================================================================

// Given all the node Local->World matrices, this function ships of the data needed by VU1
// to transform, light, and draw the shape
render_node *model::LoadRenderNodes(shape_instance&     Instance,
                                    u32                 Flags,
                                    matrix4*            L2W,
                                    radian3&            RefRot,
                                    void*               NodeFuncParam,
                                    render_node_fn*     RenderNodeFunc)
{
	// Setup node info
	render_node  *RenderNodes = CalculateRenderNodes(Instance, Flags, L2W, RefRot, NodeFuncParam, RenderNodeFunc) ;
	if (!RenderNodes)
		return NULL ;

#ifdef TARGET_PS2
#if 0
    struct init_pack
    {
        dmatag      DMA;
    } ;

    struct transfer_pack
    {
        dmatag      DMA;
    } ;

    // Prepare for transfering all data
    init_pack *InitPack  = DLStruct(init_pack) ;
    InitPack->DMA.SetCont(sizeof(init_pack) - sizeof(dmatag)) ;
    InitPack->DMA.PAD[0] = SCE_VIF1_SET_FLUSH(0) ;
    InitPack->DMA.PAD[1] = SCE_VIF1_SET_STCYCL(4, 4, 0) ;

    // Transfer all node data to vu
    s32 TotalVectors = m_nNodes * NODE_SIZE ;
    s32 Source       = (s32)RenderNodes ;
    s32 Dest         = SHAPE_NODE_ADDR ;
    s32 nVectors ;
    transfer_pack *Pack ;
    while(TotalVectors)
    {
        // Can only transfer a max of 255 vectors at a time with vif unpack
        nVectors = MIN(255, TotalVectors) ;
        Pack = DLStruct(transfer_pack) ;
        Pack->DMA.SetRef((nVectors*16), Source) ;
        Pack->DMA.PAD[0] = SCE_VIF1_SET_FLUSH(0) ;
        Pack->DMA.PAD[1] = VIF_Unpack(Dest, nVectors, VIF_V4_32, FALSE, FALSE, TRUE) ;
        ASSERT((Source & 0xF) == 0) ;

        // Transfer next...
        TotalVectors -= nVectors ;
        Source       += nVectors*16 ;
        Dest         += nVectors ;
    }
#endif

#endif // #ifdef TARGET_PS2

    return RenderNodes ;
}

#endif


//==============================================================================

// Draws shape with given fog value
void model::Draw(material_draw_info &DrawInfo)
{
    s32 i ;

    // Use draw list?
    if (shape::s_DrawSettings.UseDrawList)
    {
        // Loop through all materials
        for (i = 0 ; i < m_nMaterials ; i++)
        {
            material &M = m_Materials[i] ;

            // Is material not visible?
            if ( (i < 32) && (!(DrawInfo.MaterialVisibleFlags & (1<<i))) )
                continue ;
                
            // Is node not visible?
            if (!(M.GetNodeFlags() & DrawInfo.NodeVisibleFlags))
                continue ;

            // Add material to draw list
            shape::AddMaterialToDrawList(M, DrawInfo) ;
        }
    }
    else
    {
        // Flag no node is loaded yet
        s32         CurrentLoadedNode  = -1 ;

        // Loop through all materials
        for (i = 0 ; i < m_nMaterials ; i++)
        {
            material &M = m_Materials[i] ;

            // Is material not visible?
            if ( (i < 32) && (!(DrawInfo.MaterialVisibleFlags & (1<<i))) )
                continue ;
                
            // Is node not visible?
            if (!(M.GetNodeFlags() & DrawInfo.NodeVisibleFlags))
                continue ;

            // Lookup textures to draw with
            texture_draw_info TexDrawInfo ;
            M.LookupTextures(DrawInfo, TexDrawInfo) ;

            // Special case render?
            if (DrawInfo.RenderMaterialFunc)
            {
                // Call render function instead
                DrawInfo.RenderMaterialFunc( M,
                                             TexDrawInfo,
                                             DrawInfo,
                                             CurrentLoadedNode ) ;
            }
            else
            {
                // Regular draw...

                // Prepare to draw
                M.Activate( DrawInfo, TexDrawInfo, CurrentLoadedNode ) ;

                // Draw it
                M.Draw(DrawInfo, CurrentLoadedNode) ;
            }
        }

        #ifdef TARGET_PS2
            // Wait for GS to finish incase something wants to download to vu memory
            vram_Sync() ;
        #endif  // #ifdef TARGET_PS2
    }
}

//==============================================================================

// Outputs model as binary file
void model::ReadWrite(shape_bin_file &File)
{
    // Read/Write attributes
    File.ReadWrite(m_Name) ;
    File.ReadWrite(m_TimeStamp) ;
    File.ReadWrite(m_Type) ;
    File.ReadWrite(m_Weight) ;
    File.ReadWrite(m_bReflectionMapped) ;
    File.ReadWrite(m_PrecisionScale) ;
    File.ReadWrite(m_WorldPixelSize) ;
    File.ReadWrite(m_nDListVerts) ;
    File.ReadWrite(m_nDListPolys) ;
    File.ReadWrite(m_Bounds) ;

    // Read/Write nodes
    File.ReadWriteClassArray(m_Nodes, m_nNodes) ;

    // Read/Write texture references
    File.ReadWriteClassArray(m_TextureRefs, m_nTextureRefs) ;

    // Read/Write materials
    File.ReadWriteClassArray(m_Materials, m_nMaterials) ;

    // Write hot points
    File.ReadWriteClassArray(m_HotPoints, m_nHotPoints) ;
}

//==============================================================================
