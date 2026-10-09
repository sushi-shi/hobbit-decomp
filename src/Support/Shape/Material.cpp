#include <Support/Shape/Material.hpp>
#include <Support/Shape/Texture.hpp>
#include <Support/Shape/TextureManager.hpp>
#include <Support/Shape/Model.hpp>
#include <Support/Shape/Node.hpp>
#include <Support/Shape/Shape.hpp>
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include <Support/ObjectMgr/Collider.hpp>


// Need all versions of machine specific code for building display lists
#include <Support/Shape/Shaders/D3D_Shape.hpp>

//==============================================================================
// Static variables
//==============================================================================
#ifdef X_DEBUG
    s32     material::s_LoadSettings  = 0 ;
    s32     material::s_LoadColor     = 0 ;
    s32     material::s_LoadTex       = 0 ;
    s32     material::s_DListDraw     = 0 ;
#endif


//==============================================================================
// Useful macros
//==============================================================================

#define ASSERT_ALIGNED4(__addr__)    ASSERT(((s32)(__addr__) & 3) == 0) ;
#define ASSERT_ALIGNED16(__addr__)   ASSERT(((s32)(__addr__) & 15) == 0) ;


//==============================================================================
// Simple display list builder class
//==============================================================================

class dlist_builder
{
    byte* m_Addr;
    byte* m_StartAddr;
    byte* m_EndAddr;
    s32   m_Size;
  
public:
            dlist_builder   (void) ;
    void    Setup           ( byte* StartAddr, s32 Size ) ;
    void*   Alloc           ( s32 Size ) ;
    byte*   GetStartAddr    ( void ) ;
	s32     GetLength       ( void ) ;
};

//==============================================================================

dlist_builder::dlist_builder( void )
{
    m_StartAddr = 0;
    m_Size      = 0;
    m_EndAddr   = 0;
    m_Addr      = 0;
}

//==============================================================================

void dlist_builder::Setup( byte* StartAddr, s32 aSize )
{
    m_StartAddr = StartAddr;
    m_Size      = aSize;
    m_EndAddr   = StartAddr + aSize;
    m_Addr      = StartAddr;

    ASSERT(((s32)StartAddr & 0xF) == 0) ;

}

//==============================================================================

void* dlist_builder::Alloc( s32 Size )
{
    byte* AllocAddr;

    //shape::printf("dlist_builder::Alloc(%d)\n", Size) ;
    
    AllocAddr = m_Addr;
    m_Addr += Size;
    ASSERT( m_Addr < m_EndAddr );
    return AllocAddr;
}

//==============================================================================

byte* dlist_builder::GetStartAddr( void )
{
    return m_StartAddr;
}

//==============================================================================

s32 dlist_builder::GetLength( void )
{
	return (s32)(m_Addr - m_StartAddr);
}

//==============================================================================


//==============================================================================
// Strip class
//==============================================================================

// Constructor/destructor
dlist_strip::dlist_strip()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(dlist_strip) & 15) == 0) ;

    m_nVerts    = 0 ;       // Total verts in strip
    m_Pos       = NULL ;    // List of positions
    m_TexCoord  = NULL ;    // List of uv's
    m_Normal    = NULL ;    // List of normals
}

//==============================================================================

dlist_strip::~dlist_strip()
{
}

//==============================================================================

// Operators
dlist_strip&  dlist_strip::operator = ( dlist_strip& Strip )
{
    m_nVerts    = Strip.m_nVerts ;
    m_Pos       = Strip.m_Pos ;
    m_TexCoord  = Strip.m_TexCoord ;
    m_Normal    = Strip.m_Normal ;

    return *this ;
}

//==============================================================================

// Initialization
void dlist_strip::Setup( s32 nVerts, void* Pos, void* TexCoord, void* Normal)
{
    ASSERT(nVerts) ;
    m_nVerts    = nVerts ;
    m_Pos       = Pos ;
    m_TexCoord  = TexCoord ;
    m_Normal    = Normal ;
}

//==============================================================================

void dlist_strip::AddOffset( s32 Offset )
{
    m_Pos      = (void*)((s32)m_Pos      + Offset) ;    
    m_Normal   = (void*)((s32)m_Normal   + Offset) ;    

    // Tex co-ords are special case - they may not be any
    if (m_TexCoord != NULL)
        m_TexCoord = (void*)((s32)m_TexCoord + Offset) ;    
}

//==============================================================================

// Vert and poly access
s32 dlist_strip::GetVertCount( void )
{
    return m_nVerts ;
}

//==============================================================================

void dlist_strip::GetVert( s32 VertIndex, vector3& Pos, vector3& Norm, vector2& TexCoord )
{
    ASSERT(VertIndex >= 0) ;
    ASSERT(VertIndex < GetVertCount()) ;

#if defined(TARGET_PC)||defined(TARGET_LINUX)
    d3d_vert&   D3DVert = ((d3d_vert*)m_Pos)[VertIndex] ;

    Pos      = D3DVert.vPos ;
    Norm     = D3DVert.vNormal ;
    TexCoord = D3DVert.TexCoord ;
#endif
}

//==============================================================================

void dlist_strip::SetVert( s32 VertIndex, vector3& Pos, vector3& Norm, vector2& TexCoord )
{
    ASSERT(VertIndex >= 0) ;
    ASSERT(VertIndex < GetVertCount()) ;

#if defined(TARGET_PC)||defined(TARGET_LINUX)
    d3d_vert&   D3DVert = ((d3d_vert*)m_Pos)[VertIndex] ;

    D3DVert.vPos     = Pos ;
    D3DVert.vNormal  = Norm ;
    D3DVert.TexCoord = TexCoord ;
#endif
}

//==============================================================================

s32 dlist_strip::GetPolyCount( void )
{
    return m_nVerts-2 ;
}

//==============================================================================

xbool dlist_strip::GetPoly( s32 PolyIndex, vector3* pVert )
{
    ASSERT(PolyIndex >= 0) ;
    ASSERT(PolyIndex < GetPolyCount()) ;

#if defined(TARGET_PC)||defined(TARGET_LINUX)
    d3d_vert*   pD3DVert = &((d3d_vert*)m_Pos)[PolyIndex] ;

    // No kick (1st bit of y pos)?
    if ((*(u32*)&pD3DVert[2].vPos.Y) & 1)
        return FALSE ;

    // Which orientation? (1st bit of y pos)
    if ((*(u32*)&pD3DVert[2].vPos.X) & 1)
    {
        pVert[0] = pD3DVert[1].vPos;
        pVert[1] = pD3DVert[0].vPos;
        pVert[2] = pD3DVert[2].vPos;
    }
    else
    {
        pVert[0] = pD3DVert[0].vPos;
        pVert[1] = pD3DVert[1].vPos;
        pVert[2] = pD3DVert[2].vPos;
    }
#endif

    return TRUE ;
}

//==============================================================================

// Fast collision function - 
// Floating point version (currently only used on PC)
s32 dlist_strip::ApplyCollision( collider& Collider, const matrix4& DListL2W, const bbox& LocalBBox)
{
    s32     PolysSent = 0 ;

#if defined(TARGET_PC)||defined(TARGET_LINUX)
    // Locals
    vector3                 Verts[3] ;
    f32                     V ;
    
    // Get poly count and first vert
    s32                     PolyCount = GetPolyCount() ;
    d3d_vert*  pD3DVert  = (d3d_vert*)m_Pos ;
    
    // Loop over all polys
    while(PolyCount--)
    {
        // No kick (1st bit of y pos)?
        if( ((*(u32*)&pD3DVert[2].vPos.Y) & 1) ) goto SKIP_POLY;

        // Trivial rejection - check floating point bbox
        V = pD3DVert[0].vPos.X;
        if( pD3DVert[1].vPos.X < V ) V = pD3DVert[1].vPos.X;
        if( pD3DVert[2].vPos.X < V ) V = pD3DVert[2].vPos.X;
        if( LocalBBox.Max.X < V ) goto SKIP_POLY ;
        V = pD3DVert[0].vPos.X;
        if( pD3DVert[1].vPos.X > V ) V = pD3DVert[1].vPos.X;
        if( pD3DVert[2].vPos.X > V ) V = pD3DVert[2].vPos.X;
        if( LocalBBox.Min.X > V ) goto SKIP_POLY ;
        V = pD3DVert[0].vPos.Z;
        if( pD3DVert[1].vPos.Z < V ) V = pD3DVert[1].vPos.Z;
        if( pD3DVert[2].vPos.Z < V ) V = pD3DVert[2].vPos.Z;
        if( LocalBBox.Max.Z < V ) goto SKIP_POLY ;
        V = pD3DVert[0].vPos.Y;
        if( pD3DVert[1].vPos.Y > V ) V = pD3DVert[1].vPos.Y;
        if( pD3DVert[2].vPos.Y > V ) V = pD3DVert[2].vPos.Y;
        if( LocalBBox.Min.Y > V ) goto SKIP_POLY ;
        V = pD3DVert[0].vPos.Y;
        if( pD3DVert[1].vPos.Y < V ) V = pD3DVert[1].vPos.Y;
        if( pD3DVert[2].vPos.Y < V ) V = pD3DVert[2].vPos.Y;
        if( LocalBBox.Max.Y < V ) goto SKIP_POLY ;
        V = pD3DVert[0].vPos.Z;
        if( pD3DVert[1].vPos.Z > V ) V = pD3DVert[1].vPos.Z;
        if( pD3DVert[2].vPos.Z > V ) V = pD3DVert[2].vPos.Z;
        if( LocalBBox.Min.Z > V ) goto SKIP_POLY ;

        // Which orientation? (1st bit of x pos)
        if ((*(u32*)&pD3DVert[2].vPos.X) & 1)
        {
            Verts[0] = pD3DVert[1].vPos;
            Verts[1] = pD3DVert[0].vPos;
            Verts[2] = pD3DVert[2].vPos;
        }
        else
        {
            Verts[0] = pD3DVert[0].vPos;
            Verts[1] = pD3DVert[1].vPos;
            Verts[2] = pD3DVert[2].vPos;
        }

        // Transform into world space and do full collision
        PolysSent++;
        DListL2W.Transform( Verts, Verts, 3 );
        Collider.ApplyNGon( Verts, 3 ) ;

        // Early out?
        if (Collider.GetEarlyOut())
            return PolysSent ;

        // Next poly (simply goto next vert in strip)
        SKIP_POLY:
            pD3DVert++ ;
    }

#else

    // To stop warnings
    (void)Collider ;
    (void)DListL2W ;
    (void)LocalBBox ;

#endif

    // Tell caller how many polys went to the collider
    return PolysSent ;
}

//==============================================================================

// File IO
void dlist_strip::ReadWrite   (shape_bin_file &File)
{
    File.ReadWrite(m_nVerts) ;
    File.ReadWrite(m_Pos) ;
    File.ReadWrite(m_TexCoord) ;
    File.ReadWrite(m_Normal) ;
}


//==============================================================================
// Material display list class
//==============================================================================

// Constructor
material_dlist::material_dlist()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(material_dlist) & 15) == 0) ;

    m_Node      = -1 ;

    m_DList     = NULL ;
    m_DListSize = 0 ;

    m_nStrips   = 0 ;
    m_Strips    = NULL ;

    m_pVB       = NULL ;    // Vertex buffer
}

//==============================================================================

// Destructor
material_dlist::~material_dlist()
{
    if (m_DList)
        delete [] m_DList ;

    if (m_Strips)
        delete [] m_Strips ;

    if (m_pVB)
    {
        #ifdef TARGET_PC
        m_pVB->Release() ;
        #endif
    }
}

//==============================================================================

// Sets up display list
void material_dlist::Setup(s32 Node, s32 nPolys, s32 VertSize, byte *DList, s32 Size)
{
    s32 i ;

    #ifdef TARGET_PS2
        // Must be 16 byte aligned for DMA
        ASSERT((Size & 0xF) == 0) ;
    #endif

    ASSERT(nPolys) ;
	ASSERT(VertSize) ;
	ASSERT(DList) ;
    ASSERT(Size) ;

    // Create copy of dlist
    m_Node      = Node ;
	m_nPolys	= nPolys ;
	m_VertSize	= VertSize ;
    m_DList     = new byte[Size] ;
    ASSERT(m_DList) ;
    m_DListSize = Size ;
    x_memcpy(m_DList, DList, Size) ;

    // Convert dlist strip offsets into pointers
    for (i = 0 ; i < m_nStrips ; i++)
        m_Strips[i].AddOffset((s32)m_DList) ;
}

//==============================================================================

// Returns total memory used
s32 material_dlist::MemoryUsed()
{
    return sizeof(material_dlist) + m_DListSize ;
}

//==============================================================================

// On the PC this creates the vertex buffers
void material_dlist::InitializeForDrawing( void )
{
    #ifdef TARGET_PC

        // Already done?
        if (m_pVB)
            return ;

        // Locals
        dxerr   Error ;
        byte*   Buffer ;

        // Create vertex buffer
        Error = g_pd3dDevice->CreateVertexBuffer(m_DListSize,
                                                 D3DUSAGE_WRITEONLY, 
                                                 0, 
                                                 D3DPOOL_MANAGED, 
                                                 &m_pVB) ;
        ASSERT(Error == 0) ;
        ASSERT(m_pVB) ;

        // Copy all the verts (for PC they are at the m_Pos address)
        Error = m_pVB->Lock( 0, 0, &Buffer, 0);
        ASSERT(Error == 0);
        ASSERT(Buffer) ;
        ASSERT(m_DList) ;
        memcpy( Buffer, m_DList, m_DListSize ) ;
        m_pVB->Unlock();

    #endif
}

//==============================================================================

void material_dlist::ReadWrite(shape_bin_file &File)
{
    s32 i ;

    // Attributes
    File.ReadWrite(m_Node) ;
    File.ReadWrite(m_nPolys) ;
    File.ReadWrite(m_VertSize) ;

    // DList
    File.ReadWriteStructArray(m_DList, m_DListSize) ;

    // Turn pointers into offsets
    for (i = 0 ; i < m_nStrips ; i++)
        m_Strips[i].AddOffset(-(s32)m_DList) ;

    // Strips
    File.ReadWriteClassArray(m_Strips, m_nStrips) ;

    // Turn offset into pointers
    for (i = 0 ; i < m_nStrips ; i++)
        m_Strips[i].AddOffset((s32)m_DList) ;

    #ifdef TARGET_PS2
        // Must be 16 byte aligned for DMA
        ASSERT(((s32)m_DList & 0xF) == 0) ;
        ASSERT((m_DListSize & 0xF) == 0) ;
    #endif
}

//==============================================================================

// Adds a strip onto end of strip arrary
void material_dlist::AddStrip(dlist_strip &Strip)
{
    s32 i ;

    // Allocate space for new strips
    s32          nNewStrips = m_nStrips + 1 ;
    dlist_strip* NewStrips = new dlist_strip[nNewStrips] ;
    ASSERT(NewStrips) ;

    // Copy old strips
    for (i = 0 ; i < m_nStrips ; i++)
        NewStrips[i] = m_Strips[i] ;

    // Copy new strip
    NewStrips[m_nStrips] = Strip ;

    // Delete old strip data
    if (m_Strips)
    {
        ASSERT(m_nStrips) ;
        delete [] m_Strips ;
    }

    // Use new strip data
    m_nStrips = nNewStrips ;
    m_Strips  = NewStrips ;
}

//==============================================================================

s32 material_dlist::GetStripCount( void )
{
    return m_nStrips ;
}

//==============================================================================

dlist_strip& material_dlist::GetStrip( s32 StripIndex )
{
    ASSERT(StripIndex >= 0) ;
    ASSERT(StripIndex < m_nStrips ) ;

    return m_Strips[StripIndex] ;
}

//==============================================================================



//==============================================================================
// material class
//==============================================================================

// Constructor
material::material()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(material) & 15) == 0) ;

    // Initialize to be valid
    x_strcpy(m_Name, "undefined") ;

    m_nStrips   = 0 ;
    m_Strips    = NULL ;

    m_nDLists   = 0 ;
    m_DLists    = NULL ;

    m_ID        = -1 ;

    m_NodeFlags = 0 ;

    // Clear texture indices
    for (s32 i = 0 ; i < TEXTURE_TYPE_TOTAL ; i++)
    {
        SetTextureInfo((texture_type)i, -1,0) ;
        SetTextureRef((texture_type)i,-1) ;
    }

    // Init flags
    SetUMirrorFlag      (FALSE) ;
    SetUWrapFlag        (FALSE) ;
    SetVMirrorFlag      (FALSE) ;
    SetVWrapFlag        (FALSE) ;
    SetSelfIllumFlag    (FALSE) ;
    SetHasAlphaFlag		(FALSE) ;
    SetAdditiveFlag		(FALSE) ;
    SetPunchThroughFlag	(FALSE) ;
}

//==============================================================================

// Destructor
material::~material()
{
    // Cleanup
    if (m_Strips)
        delete [] m_Strips ;

    if (m_DLists)
        delete [] m_DLists ;
}


//==============================================================================

void material::SetName(const char *Name)
{
    // If string length okay, just copy it
    if (x_strlen(Name) < MAX_NAME_LENGTH)
        x_strcpy(m_Name, Name) ;
    else
    {
        // Copy and truncate string
        x_memcpy(m_Name, Name, MAX_NAME_LENGTH-1) ;
        m_Name[MAX_NAME_LENGTH-1] = 0 ; // End string
    }
}

//==============================================================================

// Returns memory used
s32 material::MemoryUsed()
{
    s32 i ;
    s32 Size = sizeof(material) ;
    
    // Strips
    for (i = 0 ; i < m_nStrips ; i++)
        Size += m_Strips[i].MemoryUsed() ;

    // DLists
    for (i = 0 ; i < m_nDLists ; i++)
        Size += m_DLists[i].MemoryUsed() ;

    return Size ;
}

//==============================================================================

// Call after parsing ascii file
void material::CleanupUnusedMemory()
{
    // Make sure DList was built
    // ASSERT(m_nDLists) ;
    // TO DO - Fix up max exporter to delete any materials that have no geometry
    // For now don't worry about this anymore - some materials may not have any geometry attached
    
    // Free strips
    if (m_Strips)
    {
        delete [] m_Strips ;
        m_nStrips = 0 ;
        m_Strips  = NULL ;
    }
}

//==============================================================================

// Allocated strips
void material::SetStripCount(s32 Count)
{
    ASSERT(Count >= 0) ;

    // Free old strips
    if (m_Strips)
    {
        delete [] m_Strips ;
        m_Strips = NULL ;
    }

    // Allocate new strips
    if (Count)
    {
        m_Strips = new strip[Count] ;
        ASSERT(m_Strips) ;
    }

    m_nStrips = Count ;
}

//==============================================================================

// Returns total size of dlists
s32 material::GetDListSize()
{
    s32 Size = 0 ;
    for (s32 i = 0 ; i < m_nDLists ; i++)
        Size += m_DLists[i].GetDListSize() ;

    return Size ;
}

//==============================================================================

// On the PC this creates the vertex buffers
void material::InitializeForDrawing( void )
{
    // Prepare all display lists
    for (s32 i = 0 ; i < m_nDLists ; i++)
        m_DLists[i].InitializeForDrawing() ;
}

//==============================================================================

// Sends and draws geometry via VIF and VU1
void material::Draw(const material_draw_info& DrawInfo, s32 &CurrentLoadedNode)
{
    #ifdef TARGET_PC
        // Draw all dlists
        for (s32 i = 0 ; i < m_nDLists ; i++)
            m_DLists[i].Draw(*this, DrawInfo, CurrentLoadedNode) ;
    #endif

}

//==============================================================================

void material::CreateNewTextureName(char* NewName, const char* String, const texture& TEX)
{
    // Split up texture name
    char Drive[X_MAX_DRIVE];  
    char Dir  [X_MAX_DIR];  
    char FName[X_MAX_FNAME];  
    char Ext  [X_MAX_EXT];  
    x_splitpath( TEX.GetName(), Drive, Dir, FName, Ext) ;

    // Create new file name portion
    char NewFName[X_MAX_FNAME] ;
    x_strcpy(NewFName, String) ;
    x_strcat(NewFName, FName) ;

    // Finally rebuild into NewName
    x_makepath(NewName, Drive, Dir, NewFName, Ext) ;

    // Check
    ASSERT(x_strlen(NewName) < TEXTURE_MAX_FNAME) ;
}

//==============================================================================

void material::CreateNewTextureName(char* NewName, const texture& TEXA, const char* String, const texture& TEXB)
{
    // Get TEXA filename
    char FNameA[X_MAX_FNAME];  
    x_splitpath( TEXA.GetName(), NULL, NULL, FNameA, NULL) ;

    // Remove digits from end of name so that animated diffuse maps work!
    s32 i = x_strlen(FNameA)-1 ;
    while((i >= 0) && (FNameA[i] >= '0') && (FNameA[i] <= '9'))
        FNameA[i--] = 'X' ;

    // Split up texture B name
    char FNameB[X_MAX_FNAME];  
    char Drive[X_MAX_DRIVE];  
    char Dir  [X_MAX_DIR];  
    char Ext  [X_MAX_EXT];  
    x_splitpath( TEXB.GetName(), Drive, Dir, FNameB, Ext) ;

    // Create new file name portion
    char NewFName[X_MAX_FNAME] ;
    x_strcpy(NewFName, FNameA) ;
    x_strcat(NewFName, String) ;
    x_strcat(NewFName, FNameB) ;

    // Finally rebuild into NewName
    x_makepath(NewName, Drive, Dir, NewFName, Ext) ;

    // Check
    ASSERT(x_strlen(NewName) < TEXTURE_MAX_FNAME) ;
}

//==============================================================================

// Check for punch through alpha texture (ie. it only has 2 values!)
void material::CheckForPunchThrough( model&                     Model, 
                                     texture_manager&           TextureManager, 
                                     const shape_load_settings& LoadSettings )
{
#if !defined(TARGET_LINUX)
    (void)Model ;
    xbool   AlphaBurned        = FALSE ;
    s32     NewDiffuseIndex = -1 ;

    // Loop through all frames!
    s32 Count = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Count ;
    for (s32 Frame = 0 ; Frame < Count ; Frame++)
    {
        // Get 1st texture
        s32 AlphaIndex   = GetTextureInfo(TEXTURE_TYPE_ALPHA).Index ;
        s32 DiffuseIndex = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index ;
        s32 NewTexIndex     = -1 ;

        // Both present and not already burned this baby?
        // And textures aren't the same ie. alpha intensity required
        if ((AlphaIndex != -1) && (DiffuseIndex != -1) && (AlphaIndex != DiffuseIndex))
        {
            // Goto correct frame
            DiffuseIndex += Frame % GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Count ;
            AlphaIndex   += Frame % GetTextureInfo(TEXTURE_TYPE_ALPHA).Count ;

            // Get textures
            texture& AlphaTEX   = TextureManager.GetTexture(AlphaIndex) ;
            texture& DiffuseTEX = TextureManager.GetTexture(DiffuseIndex) ;

            // Get bitmaps
            xbitmap& AlphaBMP    = AlphaTEX.GetXBitmap() ;
            xbitmap& DiffuseBMP  = DiffuseTEX.GetXBitmap() ;

            // Get sizes
            s32 AlphaWidth    = AlphaBMP.GetWidth() ;
            s32 AlphaHeight   = AlphaBMP.GetHeight() ;
            s32 DiffuseWidth  = DiffuseBMP.GetWidth() ;
            s32 DiffuseHeight = DiffuseBMP.GetHeight() ;

            // Must be the same sizes
            if ((AlphaWidth == DiffuseWidth) && (AlphaHeight == DiffuseHeight))
            {
                // Clear used array
                char AlphaUsed[256] ;
                s32  nAlphaUsed = 0 ;
                for (s32 i = 0 ; i < 256 ; i++)
                    AlphaUsed[i] = 0 ;

                // Count how many alpha values are used
                for (s32 x = 0 ; x < AlphaWidth ; x++)
                {
                    for (s32 y = 0 ; y < AlphaHeight ; y++)
                    {
                        xcolor AlphaColor   = AlphaBMP.GetPixelColor(x,y) ;

                        if (!AlphaUsed[AlphaColor.A])
                        {
                            AlphaUsed[AlphaColor.A] = TRUE ;
                            nAlphaUsed++ ;
                        }
                    }
                }

                // If there are only 3 values, let's burn them into the diffuse and create a punch through texture
                if (nAlphaUsed <= 3)
                {
                    AlphaBurned = TRUE ;

                    // Flag material is punch through
                    SetPunchThroughFlag(TRUE) ;

                    // Create combined name: alphanameXX_PUNCH_diffusename
                    char NewName[X_MAX_FNAME] ;
                    CreateNewTextureName(NewName, AlphaTEX, "_PUNCH_", DiffuseTEX) ;

                    // Only create the texture if it's not already there...
                    NewTexIndex = TextureManager.GetTextureIndexByName(NewName) ;
                    if (NewTexIndex == -1)
                    {
                        // Create new texture
                        texture* NewTexture = new texture() ;
                        ASSERT(NewTexture) ;
                        NewTexture->SetName(NewName) ;

                        // Add to texture manager
                        NewTexIndex = TextureManager.AddTexture(NewTexture) ;
                        ASSERT(NewTexIndex != -1) ;

                        // Setup new bitmap
                        // (Create a new 32 bit combined bitmap so the quantizer kicks in)
                        xbitmap& NewBitmap = NewTexture->GetXBitmap() ;
                        byte*    PixelData = (byte *)x_malloc(DiffuseWidth * DiffuseHeight * 4) ;
                        ASSERT(PixelData) ;
                        NewBitmap.Setup(xbitmap::FMT_32_ARGB_8888,     // Format
                                             DiffuseWidth,                  // Width
                                             DiffuseHeight,                 // Height
                                             TRUE,                          // DataOwned
                                             PixelData,                     // PixelData
                                             FALSE,                         // Clut owned
                                             NULL) ;                        // Clut data

                        // Get middle of alpha range
                        s32 MiddleAlpha = 128 ;
                        if (LoadSettings.Target == shape_bin_file_class::PS2) // PS2 is special case!
                            MiddleAlpha = 64 ;

                        // Burn alpha + diffuse into new bitmap
                        for (s32 x = 0 ; x < DiffuseWidth ; x++)
                        {
                            for (s32 y = 0 ; y < DiffuseHeight ; y++)
                            {
                                xcolor AlphaColor   = AlphaBMP.GetPixelColor(x,y) ;
                                xcolor DiffuseColor = DiffuseBMP.GetPixelColor(x,y) ;

                                // Map to either 0 or 255 for punch through!
                                if (AlphaColor.A < MiddleAlpha)
                                    AlphaColor.A = 0 ;
                                else
                                    AlphaColor.A = 255 ;

                                // Grab alpha
                                DiffuseColor.A = AlphaColor.A ;

                                // Store new color
                                NewBitmap.SetPixelColor(DiffuseColor, x,y) ;
                            }
                        }

                        // Convert to a format with alpha
                        xbitmap::format NewFormat = xbitmap::FMT_NULL;
                        if (LoadSettings.Target == shape_bin_file_class::PC)
                            NewFormat = xbitmap::FMT_32_ARGB_8888 ;
                        else
                        if (LoadSettings.Target == shape_bin_file_class::PS2)
                        {
                            // Make sure it has alpha
                            switch( DiffuseBMP.GetFormatInfo().BPP )
                            {
                                case 32:
                                case 24:    NewFormat = xbitmap::FMT_32_ABGR_8888 ; break;
                                case 16:    NewFormat = xbitmap::FMT_16_ABGR_1555 ; break;
                                case 8:     NewFormat = xbitmap::FMT_P8_ABGR_8888 ; break;
                                case 4:     NewFormat = xbitmap::FMT_P4_ABGR_8888 ; break;
                            }
                        }

                        ASSERT( NewFormat != xbitmap::FMT_NULL );
                        NewBitmap.ConvertFormat( NewFormat );

                        // Put into native format
                        switch(LoadSettings.Target)
                        {
                            default:
                                ASSERT(0) ; // version not supported!
                                break ;

                            case shape_bin_file_class::PS2:
                                auxbmp_ConvertToPS2(NewBitmap) ;
                                break ;

                            case shape_bin_file_class::PC:
                                auxbmp_ConvertToD3D(NewBitmap) ;
                                break ;
                        }

                        // Create mips?
                        if (LoadSettings.BuildMips)
                            NewBitmap.BuildMips() ;

                        // Plop new diffuse bitmap back in vram
                        if (LoadSettings.Target == shape_bin_file_class::GetDefaultTarget())
                            vram_Register(NewBitmap) ;
                    }
                }
            }
        }

        // Update?
        if (AlphaBurned)
        {
            // Update material diffuse index
            if (Frame == 0)
            {
                ASSERT(NewTexIndex != -1) ;
                NewDiffuseIndex = NewTexIndex ;
            }
            else
            {
                ASSERT(NewTexIndex == (NewDiffuseIndex + Frame)) ;
            }
        }
    }

    // Remove alpha?
    if (AlphaBurned)
    {
        // NOTE: The diffuse and alpha textures are left around incase another diffuse texture
        //       may use it again...

        // Set new diffuse texture index
        ASSERT(NewDiffuseIndex != -1) ;
        m_TextureInfo[TEXTURE_TYPE_DIFFUSE].Index = NewDiffuseIndex ;

        // Finally, remove the alpha lookup since it is not needed
        SetTextureInfo(TEXTURE_TYPE_ALPHA, -1, 0) ;
        SetTextureRef (TEXTURE_TYPE_ALPHA, -1) ;
    }
#else
ASSERT(FALSE);
#endif
}

//==============================================================================

void material::BurnAlphaIntoDiffuse( model&                     Model, 
                                     texture_manager&           TextureManager, 
                                     const shape_load_settings& LoadSettings )
{
#if !defined(TARGET_LINUX)
    (void)Model ;

    xbool   AlphaBurned = FALSE ;
    s32     NewDiffuseIndex = -1 ;

    // Loop through all frames!
    s32 Count = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Count ;
    for (s32 Frame = 0 ; Frame < Count ; Frame++)
    {
        // Get 1st texture
        s32 AlphaIndex      = GetTextureInfo(TEXTURE_TYPE_ALPHA).Index ;
        s32 DiffuseIndex    = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index ;
        s32 NewTexIndex        = -1 ;

        // Both present and not already burned this baby?
        if ((AlphaIndex != -1) && (DiffuseIndex != -1))
        {
            // Goto correct frame
            DiffuseIndex += Frame % GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Count ;
            AlphaIndex   += Frame % GetTextureInfo(TEXTURE_TYPE_ALPHA).Count ;

            // Get textures
            texture& AlphaTEX   = TextureManager.GetTexture(AlphaIndex) ;
            texture& DiffuseTEX = TextureManager.GetTexture(DiffuseIndex) ;

            // Get bitmaps
            xbitmap& AlphaBMP    = AlphaTEX.GetXBitmap() ;
            xbitmap& DiffuseBMP  = DiffuseTEX.GetXBitmap() ;

            // Simple case if diffuse and alpha are the same texture!
            if (DiffuseIndex == AlphaIndex)
            {
                // Setup name of new texture...
                AlphaBurned = TRUE ;

                // Create name "AI_diffusename"
                char NewName[X_MAX_FNAME] ;
                CreateNewTextureName(NewName, "AI_", DiffuseTEX) ;
                ASSERT(x_strlen(NewName) < TEXTURE_MAX_FNAME) ;

                // Only create the texture if it's not already there...
                NewTexIndex = TextureManager.GetTextureIndexByName(NewName) ;
                if (NewTexIndex == -1)
                {
                    // Create new texture
                    texture* NewTexture = new texture() ;
                    ASSERT(NewTexture) ;
                    NewTexture->SetName(NewName) ;

                    // Add to texture manager
                    NewTexIndex = TextureManager.AddTexture(NewTexture) ;
                    ASSERT(NewTexIndex != -1) ;

                    // Setup new bitmap
                    xbitmap& NewBitmap = NewTexture->GetXBitmap() ;
                    NewBitmap = DiffuseBMP ;
                    NewBitmap.BuildMips(0) ;
                    NewBitmap.MakeIntensityAlpha() ;

                    // Put into native format
                    switch(LoadSettings.Target)
                    {
                        default:
                            ASSERT(0) ; // version not supported!
                            break ;

                        case shape_bin_file_class::PS2:
                            auxbmp_ConvertToPS2(NewBitmap) ;
                            break ;

                        case shape_bin_file_class::PC:
                            auxbmp_ConvertToD3D(NewBitmap) ;
                            break ;
                    }

                    // Create mips?
                    if (LoadSettings.BuildMips)
                        NewBitmap.BuildMips() ;

                    // Put into vram
                    if (LoadSettings.Target == shape_bin_file_class::GetDefaultTarget())
                        vram_Register(NewBitmap) ;
                }
            }
            else
            if (LoadSettings.Target == shape_bin_file_class::PC) // Only burn in on the PC
            {
                // Setup name of new texture...
                AlphaBurned = TRUE ;

                // Create combined name: "alphanameXX_BURNT_diffusename"
                char NewName[X_MAX_FNAME] ;
                CreateNewTextureName(NewName, AlphaTEX, "_BURNT_", DiffuseTEX) ;

                // Only create the texture if it's not already there...
                NewTexIndex = TextureManager.GetTextureIndexByName(NewName) ;
                if (NewTexIndex == -1)
                {
                    // Create new texture
                    texture* NewTexture = new texture() ;
                    ASSERT(NewTexture) ;
                    NewTexture->SetName(NewName) ;

                    // Add to texture manager
                    NewTexIndex = TextureManager.AddTexture(NewTexture) ;
                    ASSERT(NewTexIndex != -1) ;

                    // Get sizes
                    s32 AlphaWidth    = AlphaBMP.GetWidth() ;
                    s32 AlphaHeight   = AlphaBMP.GetHeight() ;
                    s32 DiffuseWidth  = DiffuseBMP.GetWidth() ;
                    s32 DiffuseHeight = DiffuseBMP.GetHeight() ;

                    // Setup new bitmap
                    xbitmap& NewBitmap = NewTexture->GetXBitmap() ;
                    byte*    PixelData = (byte *)x_malloc(DiffuseWidth * DiffuseHeight * 4) ;
                    ASSERT(PixelData) ;
                    NewBitmap.Setup(xbitmap::FMT_32_ARGB_8888,     // Format
                                    DiffuseWidth,                  // Width
                                    DiffuseHeight,                 // Height
                                    TRUE,                          // DataOwned
                                    PixelData,                     // PixelData
                                    FALSE,                         // Clut owned
                                    NULL) ;                        // Clut data

                    // Okay - we need to go through pixel by pixel combing the diffuse color, and alpha

                    // Quick version
                    if (        (AlphaWidth  == DiffuseWidth)
                            &&  (AlphaHeight == DiffuseHeight) )
                    {
                        // Now setup alpha
                        for (s32 x = 0 ; x < DiffuseWidth ; x++)
                        {
                            for (s32 y = 0 ; y < DiffuseHeight ; y++)
                            {
                                xcolor AlphaColor   = AlphaBMP.GetPixelColor(x,y) ;
                                xcolor DiffuseColor = DiffuseBMP.GetPixelColor(x,y) ;
                                DiffuseColor.A      = AlphaColor.A ;
                                NewBitmap.SetPixelColor(DiffuseColor, x,y) ;
                            }
                        }
                    }
                    else
                    {
                        // Slow version...
                        for (s32 x = 0 ; x < DiffuseWidth ; x++)
                        {
                            for (s32 y = 0 ; y < DiffuseHeight ; y++)
                            {
                                xcolor AlphaColor   = AlphaBMP.GetBilinearColor( (f32)x/DiffuseWidth, (f32)y/DiffuseHeight, TRUE) ;
                                xcolor DiffuseColor = DiffuseBMP.GetPixelColor(x,y) ;
                                DiffuseColor.A      = AlphaColor.A ;
                                NewBitmap.SetPixelColor(DiffuseColor, x,y) ;
                            }
                        }
                    }

                    // Put into native format
                    switch(LoadSettings.Target)
                    {
                        default:
                            ASSERT(0) ; // version not supported!
                            break ;

                        case shape_bin_file_class::PS2:
                            auxbmp_ConvertToPS2(NewBitmap) ;
                            break ;

                        case shape_bin_file_class::PC:
                            auxbmp_ConvertToD3D(NewBitmap) ;
                            break ;
                    }

                    // Create mips?
                    if (LoadSettings.BuildMips)
                        NewBitmap.BuildMips() ;

                    // Put into vram
                    if (LoadSettings.Target == shape_bin_file_class::GetDefaultTarget())
                        vram_Register(NewBitmap) ;
                }
            }
        }

        // Update?
        if (AlphaBurned)
        {
            // Update material diffuse index
            if (Frame == 0)
            {
                ASSERT(NewTexIndex != -1) ;
                NewDiffuseIndex = NewTexIndex ;
            }
            else
            {
                ASSERT(NewTexIndex == (NewDiffuseIndex + Frame)) ;
            }
        }
    }

    // Remove alpha?
    if (AlphaBurned)
    {
        // NOTE: The diffuse and alpha textures are left around incase another diffuse texture
        //       may use it again...

        // Set new diffuse texture index
        ASSERT(NewDiffuseIndex != -1) ;
        m_TextureInfo[TEXTURE_TYPE_DIFFUSE].Index = NewDiffuseIndex ;

        // Finally, remove the alpha lookup since it is not needed
        SetTextureInfo(TEXTURE_TYPE_ALPHA, -1, 0) ;
        SetTextureRef (TEXTURE_TYPE_ALPHA, -1) ;
    }
#endif
}

//==============================================================================

// Prepares textures ready for using
void material::CompressTextures( model&                     Model, 
                                 texture_manager&           TextureManager, 
                                 const shape_load_settings& LoadSettings )
{
#if !defined(TARGET_LINUX)
    // Only compress on the PS2
    if (LoadSettings.Target != shape_bin_file_class::PS2)
        return ;

    // Any diffuse texture(s)?
    s32 Count = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Count ;
    if (!Count)
        return ;

    // Skip punch through materials since they eat a lot of gs time due to the huge
    // leaf polys that are in trees
    if (m_Flags.PunchThrough)
        return ;

    // Check to see if the compressed textures have already been created
    {
        // Lookup 1st diffuse texture info
        s32      DiffuseIndex = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index ;
        texture& DiffuseTEX   = TextureManager.GetTexture(DiffuseIndex) ;
        xbitmap& DiffuseBMP   = DiffuseTEX.GetXBitmap() ;

        // Can texture be compressed?
        // (smallest width/height is 8 pixels for ps2 and base map will be 4* smaller)
        if ( (DiffuseBMP.GetWidth() < (8*4)) || (DiffuseBMP.GetHeight() < (8*4)) )
            return ;

        // Skip alpha textures because they look like shit
        if (DiffuseBMP.HasAlphaBits())
            return ;

        // Skip 4bits per pixel formats since they will not compress!
        if (DiffuseBMP.GetBPP() <= 4)
            return ;

        // Already compressed?
        char FName[X_MAX_FNAME];  
        x_splitpath( DiffuseTEX.GetName(), NULL, NULL, FName, NULL) ;
        if (x_strncmp("BASE_", FName, 5) == 0)
            return ;

        // Create base name: "BASE_diffusename"
        char BaseName[X_MAX_FNAME] ;
        CreateNewTextureName(BaseName, "BASE_", DiffuseTEX) ;

        // Create lum name: "LUM_diffusename"
        char LumName[X_MAX_FNAME] ;
        CreateNewTextureName(LumName, "LUM_", DiffuseTEX) ;

        // Make material reference it (this will add it to the model only if it's not already there)
        SetTextureRef(TEXTURE_TYPE_LUMINANCE, Model.AddTextureRef(LumName)) ;

        // Already converted?
        s32 BaseTexIndex = TextureManager.GetTextureIndexByName(BaseName) ;
        s32 LumTexIndex  = TextureManager.GetTextureIndexByName(LumName) ;
        if ((BaseTexIndex != -1) && (LumTexIndex != -1))
        {
            // Just set indices to textures and we are done
            SetTextureInfo(TEXTURE_TYPE_DIFFUSE,   BaseTexIndex, Count) ;
            SetTextureInfo(TEXTURE_TYPE_LUMINANCE, LumTexIndex,  Count) ;
            return ;
        }
    }

    // Create the compressed textures

    // These will be the new texture indices once we are done
    s32 BaseTexIndex = TextureManager.GetTextureCount() ;
    s32 LumTexIndex  = TextureManager.GetTextureCount() + Count ;

    // Create the base textures
    s32 Frame ;
    for (Frame = 0 ; Frame < Count ; Frame++)
    {
        // Lookup diffuse texture info
        s32      DiffuseIndex = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index + Frame ;
        texture& DiffuseTEX   = TextureManager.GetTexture(DiffuseIndex) ;

        // Create base name: "BASE_diffusename"
        char BaseName[X_MAX_FNAME] ;
        CreateNewTextureName(BaseName, "BASE_", DiffuseTEX) ;

        // Create the texture and add to texture manager
        texture* BaseTEX = new texture() ;
        ASSERT(BaseTEX) ;
        BaseTEX->SetName(BaseName) ;

        // Add to texture manager
        TextureManager.AddTexture(BaseTEX) ;
    }

    // Create the lum textures
    for (Frame = 0 ; Frame < Count ; Frame++)
    {
        // Lookup diffuse texture info
        s32      DiffuseIndex = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index + Frame ;
        texture& DiffuseTEX   = TextureManager.GetTexture(DiffuseIndex) ;

        // Create lum name: "LUM_diffusename"
        char LumName[X_MAX_FNAME] ;
        CreateNewTextureName(LumName, "LUM_", DiffuseTEX) ;

        // Create the texture and add to texture manager
        texture* LumTEX = new texture() ;
        ASSERT(LumTEX) ;
        LumTEX->SetName(LumName) ;

        // Add to texture manager
        TextureManager.AddTexture(LumTEX) ;
    }

    // Compress the textures
    for (Frame = 0 ; Frame < Count ; Frame++)
    {
        // Lookup diffuse texture info
        s32      DiffuseIndex = GetTextureInfo(TEXTURE_TYPE_DIFFUSE).Index + Frame ;
        texture& DiffuseTEX   = TextureManager.GetTexture(DiffuseIndex) ;
        xbitmap& DiffuseBMP   = DiffuseTEX.GetXBitmap() ;
        
        // Lookup base texture info
        texture& BaseTEX      = TextureManager.GetTexture(BaseTexIndex + Frame) ;
        xbitmap& BaseBMP      = BaseTEX.GetXBitmap() ;
        
        // Lookup lum texture info
        texture& LumTEX       = TextureManager.GetTexture(LumTexIndex + Frame) ;
        xbitmap& LumBMP       = LumTEX.GetXBitmap() ;
        
        // Show info
        shape::printf("compressing texture: %s\n", DiffuseTEX.GetName()) ;

        // Subtractive compression! (Also builds mips!)
        auxbmp_CompressPS2( DiffuseBMP, BaseBMP, LumBMP, TRUE ) ;

        // Just incase we are doing a pc test
        if (LoadSettings.Target == shape_bin_file_class::PC)
        {
            auxbmp_ConvertToD3D (BaseBMP) ;
            auxbmp_ConvertToD3D (LumBMP) ;
        }

        // Register with vram
        if (LoadSettings.Target == shape_bin_file_class::GetDefaultTarget())
        {
            BaseTEX.Register(TEXTURE_TYPE_DIFFUSE) ;
            LumTEX.Register(TEXTURE_TYPE_LUMINANCE) ;
        }

        // TEST - BUILD AND SAVE OUT DECOMPRESSED VERSIONS
        #if 0
        {
            xbitmap OutBMP ;

            // Create a 32 bit bitmap the same size of the source
            s32     OutWidth  = LumBMP.GetWidth() ;
            s32     OutHeight = LumBMP.GetHeight() ;
            byte*   OutData   = (byte *)x_malloc(OutWidth * OutHeight * 4) ;
            ASSERT(OutData) ;
            OutBMP.Setup(xbitmap::FMT_32_ARGB_8888,  // Format
                         OutWidth,                   // Width
                         OutHeight,                  // Height
                         TRUE,                       // DataOwned
                         OutData,                    // PixelData
                         FALSE,                      // Clut owned
                         NULL) ;                     // Clut data
            s32 BlockSize = OutWidth / DiffuseBMP.GetWidth() ;

            // Create output
            for( s32 by=0; by<OutHeight/BlockSize; by++ )
            for( s32 bx=0; bx<OutWidth/BlockSize; bx++ )
            {
                // Setup block
                for( s32 y=0; y<BlockSize; y++ )
                for( s32 x=0; x<BlockSize; x++ )
                {
                    // Get base map color
                    f32 u = (f32)((bx*BlockSize)+x) / (f32)OutWidth ;
                    f32 v = (f32)((by*BlockSize)+y) / (f32)OutHeight ;
                    xcolor BaseCol = BaseBMP.GetBilinearColor(u,v,TRUE) ;

                    // Lookup actual color
                    xcolor LumCol = LumBMP.GetPixelColor(bx*BlockSize+x, by*BlockSize+y) ;
                    #if 0
                        // Multiplicative
                        s32 R = (((s32)BaseCol.R * (s32)LumCol.R) >> 7) ;
                        s32 G = (((s32)BaseCol.G * (s32)LumCol.G) >> 7) ;
                        s32 B = (((s32)BaseCol.B * (s32)LumCol.B) >> 7) ;
                        s32 A = (((s32)BaseCol.A * (s32)LumCol.A) >> 7) ;
                    #else
                        // Subtractive
                        s32 R = ((s32)BaseCol.R - (s32)LumCol.R) ;
                        s32 G = ((s32)BaseCol.G - (s32)LumCol.G) ;
                        s32 B = ((s32)BaseCol.B - (s32)LumCol.B) ;
                        s32 A = ((s32)BaseCol.A - (s32)LumCol.A) ;
                    #endif

                    xcolor OutCol ;
                    OutCol.R = (u8)MAX(0, MIN(R,255)) ;
                    OutCol.G = (u8)MAX(0, MIN(G,255)) ;
                    OutCol.B = (u8)MAX(0, MIN(B,255)) ;
                    OutCol.A = (u8)MAX(0, MIN(A,255)) ;

                    // Set it!
                    OutBMP.SetPixelColor(OutCol,bx*BlockSize+x, by*BlockSize+y) ;
                }
            }

            // Save
            char    DRIVE   [X_MAX_DRIVE];
            char    DIR     [X_MAX_DIR];
            char    FNAME   [X_MAX_FNAME];
            char    EXT     [X_MAX_EXT];
            char    OutName [X_MAX_PATH];
            x_splitpath( DiffuseTEX.GetName(), DRIVE, DIR, FNAME, EXT) ;
            x_makepath ( OutName, NULL, NULL, FNAME, NULL) ;
            OutBMP.SaveTGA(xfs("%s_COMP.tga",OutName)) ;

            DiffuseBMP.SaveTGA(xfs("%s_ORIG.tga",OutName)) ;
            BaseBMP.SaveTGA(xfs("%s_P1_BASE.tga",OutName)) ;
            LumBMP.SaveTGA(xfs("%s_P1_LUM.tga",OutName)) ;
        }
        #endif
    }

    // Finally, make the material use the new textures
    SetTextureInfo(TEXTURE_TYPE_DIFFUSE,   BaseTexIndex, Count) ;
    SetTextureInfo(TEXTURE_TYPE_LUMINANCE, LumTexIndex,  Count) ;
#endif
}

//==============================================================================

// Prepares textures ready for using
void material::CheckForBurningTextures( model&                      Model, 
                                        texture_manager&            TextureManager, 
                                        const shape_load_settings&  LoadSettings )
{
    // Punch through special case
    CheckForPunchThrough(Model, TextureManager, LoadSettings) ;

    // Burn alpha into diffuse texture special cases
    BurnAlphaIntoDiffuse(Model, TextureManager, LoadSettings) ;

    // Compress the textures for PS2 target
    // (do after punch through since they are skipped)
    if (LoadSettings.CompressTextures)
        CompressTextures(Model, TextureManager, LoadSettings) ;
}

//==============================================================================

// Loads all textures
void material::LoadTextures( model&                     Model, 
                             texture_manager&           TextureManager, 
                             const shape_load_settings& LoadSettings )
{
    s32 Type ;

    // PS doesn't support bump mapping
    if (LoadSettings.Target == shape_bin_file_class::PS2)
    {
        // Clear all references to the bumpmap
        SetTextureInfo(TEXTURE_TYPE_BUMP,-1,0) ;
        SetTextureRef(TEXTURE_TYPE_BUMP,-1) ;
    }

    // Loop through all textures in material
    for (Type = 0 ; Type < TEXTURE_TYPE_TOTAL ; Type++)
    {
        // Is a texture in this slot?
        s32 TexRefIndex = GetTextureRef((texture_type)Type) ;
        if (TexRefIndex != -1)
        {
            // Lookup texture name
            const char* TextureName = Model.GetTextureRef(TexRefIndex).GetName() ;

            // Remap texture index into texture manager index
            s32 Index = -1 ;
            s32 Count = TextureManager.AddAndLoadTextures(TextureName,
                                                          LoadSettings,
                                                          (texture_type)Type, 
                                                          Index,
                                                          GetTextureInfo((texture_type)Type).Count) ;

            // Texture is not found, so remove reference from shape
            // (so that .shp will not try to load it again...)
            if (Count == 0)
                SetTextureRef((texture_type)Type, -1) ;

            // Update material to use texture manager index
            SetTextureInfo((texture_type)Type, Index, Count) ;
        }
    }

    // Burn textures (for when reading from ascii)
    if (LoadSettings.BurnTextures)
        CheckForBurningTextures(Model, TextureManager, LoadSettings) ;

    // Increase the reference counts of textures that are used ready for saving xbmps
    for (Type = 0 ; Type < TEXTURE_TYPE_TOTAL ; Type++)
    {
        // Any textures of this type?
        s32 Count = GetTextureInfo((material::texture_type)Type).Count ;
        s32 Index = GetTextureInfo((material::texture_type)Type).Index ;
        if (Count)
        {
            // Get texture
            ASSERT(Index != -1) ;
            texture& Tex = TextureManager.GetTexture(Index) ;

            // Get model texture ref
            s32 TexRefIndex = GetTextureRef((texture_type)Type) ;
            ASSERT(TexRefIndex != -1) ;
            shape_texture_ref&  TexRef = Model.GetTextureRef(TexRefIndex) ;

            // Update reference to use texture name so that no searching is performed when loading .shp file
            TexRef.SetName(Tex.GetName()) ;

            // Loop through all frames this type
            for (s32 Frame = 0 ; Frame < Count ; Frame++)
            {
                // Get 1st texture index
                s32 TexIndex   = GetTextureInfo((material::texture_type)Type).Index ;

                // Goto correct frame
                TexIndex += Frame % GetTextureInfo((material::texture_type)Type).Count ;

                // Flag it's referenced
                texture& Texture = TextureManager.GetTexture(TexIndex) ;
                Texture.IncReferenceCount() ;

                // Show debug info
                char    DRIVE   [X_MAX_DRIVE];
                char    DIR     [X_MAX_DIR];
                char    FNAME   [X_MAX_FNAME];
                char    EXT     [X_MAX_EXT];
                char    Name    [X_MAX_PATH];
                x_splitpath( Model.GetName(), DRIVE, DIR,  FNAME, EXT) ;
                x_makepath ( Name,            NULL,  NULL, FNAME, EXT) ;
                shape::printf("   Model:%s ", Name) ;

                x_splitpath( Texture.GetName(), DRIVE, DIR,  FNAME, EXT) ;
                x_makepath ( Name,              NULL,  NULL, FNAME, EXT) ;
                shape::printf(" references texture %s\n", Name) ;
            }
        }
    }
}

//==============================================================================

// Returns the ID to use for the shape material list and sets up the textures that are going to be used
s32 material::LookupTextures( const material_draw_info&     DrawInfo,
                                    texture_draw_info&      TexDrawInfo )
{
    // Start with material base ID
    s32 DrawListID = m_DrawListID ;
    s32 FrameBits  = 0 ;

    // Setup texture lookup infos (one for each texture type)
    for (s32 Type = 0 ; Type < TEXTURE_TYPE_TOTAL ; Type++)
    {
        // Lookup texture and frame
        s32         Count = m_TextureInfo[Type].Count ;
        s32         Frame ;
        texture*    Texture ;

        // No textures?
        if (Count == 0)
        {
            // Quick case - no texture
            Frame   = 0 ;
            Texture = NULL ;
        }
        else
        {
            // Get start index
            s32 Index = m_TextureInfo[Type].Index ;
            ASSERT(Index != -1) ;

            // Single texture?
            if (Count == 1)
            {
                Frame   = 0 ;
                Texture = &(DrawInfo.TextureManager->GetTexture(Index)) ;
            }
            else
            {
                // Take frame into account
                switch(DrawInfo.PlaybackType)
                {
                    // Repeat sequence
                    case PLAYBACK_TYPE_LOOP:
                        Frame = DrawInfo.Frame % Count ;
                        break ;

                    // Ping pong sequence
                    case PLAYBACK_TYPE_PING_PONG:
                        {
                            s32 TotalFrames = Count<<1 ;
                            s32 ModFrame = DrawInfo.Frame % TotalFrames ;
                            if (ModFrame < Count)
                                Frame = ModFrame ;
                            else
                                Frame = TotalFrames - ModFrame - 1 ;
                        }
                        break ;

                    // Play sequence just once
                    case PLAYBACK_TYPE_ONCE_ONLY:
                        Frame = MIN(DrawInfo.Frame, Count-1) ;
                        break ;

                    default:
                        // Not supported!
                        ASSERT(0) ;
                        Frame = 0 ;
                        break ;
                }

                // Make sure code works!
                ASSERT(Frame >= 0) ;
                ASSERT(Frame < Count) ;

                // Lookup texture from frame
                Texture = &(DrawInfo.TextureManager->GetTexture(Index + Frame)) ;
            }
        }

        // Keep texture pointer
        TexDrawInfo.Texture[Type] = Texture ;

        // Adjust draw list ID so material lands in correct place in draw list
        DrawListID += Frame << FrameBits ;
        FrameBits  += m_TextureInfo[Type].FrameBits ;
    }

    // Setup detail texture lookup
    TexDrawInfo.DetailTexture    = DrawInfo.DetailTexture ;
    TexDrawInfo.DetailClutHandle = DrawInfo.DetailClutHandle ;

    // Turn off passes?
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_ALPHA_PASS)
        TexDrawInfo.Texture[TEXTURE_TYPE_ALPHA] = NULL ;
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_DIFFUSE_PASS)
        TexDrawInfo.Texture[TEXTURE_TYPE_DIFFUSE] = NULL ;
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_LUMINANCE_PASS)
        TexDrawInfo.Texture[TEXTURE_TYPE_LUMINANCE] = NULL ;
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_REFLECT_PASS)
        TexDrawInfo.Texture[TEXTURE_TYPE_REFLECT] = NULL ;
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_BUMP_PASS)
        TexDrawInfo.Texture[TEXTURE_TYPE_BUMP] = NULL ;
    if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_DETAIL_PASS)
        TexDrawInfo.DetailTexture = NULL ;

    // Turn off detail on self illum materials?
    if ((m_Flags.SelfIllum) && (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_SELF_ILLUM_MAT_DETAIL))
        TexDrawInfo.DetailTexture = NULL ;

    // Is this an animated texture?
    if (m_TextureInfo[TEXTURE_TYPE_DIFFUSE].Count > 1)
    {
        // Turn off detail texture for animated material?
        if (!(DrawInfo.Flags & shape::DRAW_FLAG_TURN_ON_ANIM_MAT_DETAIL))
            TexDrawInfo.DetailTexture    = NULL ;
    }
    else
    {
        // Turn off detail texture for static material?
        if (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_STATIC_MAT_DETAIL)
            TexDrawInfo.DetailTexture = NULL ;
    }

    // Return the draw list ID to use in the shape material list
    return DrawListID ;
}


//==============================================================================
//
// Shared code
//
//==============================================================================

// Prepares for rendering with material
void material::Activate ( const material_draw_info& DrawInfo,
                                s32&                CurrentLoadedNode )
{
    // Lookup textures to draw with
    texture_draw_info TexDrawInfo ;
    LookupTextures(DrawInfo, TexDrawInfo) ;

    // Activate
    Activate(DrawInfo, TexDrawInfo, CurrentLoadedNode) ;
}

//==============================================================================

// Prepares for rendering with material and textures
void material::Activate( const material_draw_info& DrawInfo, 
                         const texture_draw_info&  TexDrawInfo,
                               s32&                CurrentLoadedNode )
{
    // Activate common data
    ActivateSettings(DrawInfo, TexDrawInfo) ;

    // Activate textures
    ActivateTextures(DrawInfo, TexDrawInfo) ;

    // Write material to z buffer only?
    if (DrawInfo.Flags & shape::DRAW_FLAG_PRIME_Z_BUFFER)
        PrimeZBuffer(DrawInfo, TexDrawInfo, CurrentLoadedNode) ;

    // Actiavte material
    ActivateColor(DrawInfo, TexDrawInfo) ;
}

//==============================================================================


//==============================================================================
//
// PS2 Specific code
//
//==============================================================================

xbool material::ActivateTextures(const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo)
{
    return TRUE ;
}

//==============================================================================

// Builds material display list so that it can be referenced by cpu

//==============================================================================




//==============================================================================
//
// PC Specific code
//
//==============================================================================

#ifdef TARGET_PC

//==============================================================================
// material display list class
//==============================================================================


// Draws the material
void material_dlist::Draw(const material& Material, const material_draw_info& DrawInfo, s32 &CurrentLoadedNode)
{
    // Skip if node is not visible
    if ( (m_Node < 32) && (!(DrawInfo.NodeVisibleFlags & (1<<m_Node))) )
        return ;

    // Performing animated visibility draw?
    if (DrawInfo.Flags & shape::DRAW_FLAG_VISIBILITY_ANIMATED)
    {
        // If drawing transparent visibility pass, then skip opaque nodes
        if (DrawInfo.Flags & shape::DRAW_FLAG_VISIBILITY_PASS_TRANSPARENT)
        {
            // If this node is opaque, then skip
            if (DrawInfo.RenderNodes[m_Node].Light.Amb.W == 1)
                return ;

            // Totally transparent?
            if (DrawInfo.RenderNodes[m_Node].Light.Amb.W == 0)
                return ;
        }

        // If drawing opqaue visibility pass, then skip transparent nodes
        if (DrawInfo.Flags & shape::DRAW_FLAG_VISIBILITY_PASS_OPAQUE)
        {
            // If this node is transparent, then skip
            if (DrawInfo.RenderNodes[m_Node].Light.Amb.W != 1)
                return ;
        }
    }

    // Load a new node?
    if (CurrentLoadedNode != m_Node)
    {
        render_node &RenderNode = DrawInfo.RenderNodes[m_Node] ;

        // Flag this node is loaded
        CurrentLoadedNode = m_Node ;

        // Must have already called BuildDList!
        ASSERT(m_Node != -1) ;
		ASSERT(m_nPolys) ;
		ASSERT(m_VertSize) ;
        ASSERT(m_DList) ;
        ASSERT(m_DListSize) ;

        // Get current view
        const view* View = eng_GetView() ;
        ASSERT(View) ;

        // Setup the shader constants
        d3d_vs_consts   VSConsts ;

        // Setup useful constants
        VSConsts.Consts[0] = 0.0f ;
        VSConsts.Consts[1] = 1.0f ;
        VSConsts.Consts[2] = -1.0f ;
        VSConsts.Consts[3] = 2.0f ;
        
        // Setup local to clip space matrix
        //VSConsts.L2C = View->GetW2C() * RenderNode.L2W ; // THERES SOMETHING WRONG IN THE XFILES!!!
        VSConsts.L2C = View->GetV2C() * View->GetW2V() * RenderNode.L2W ;
        VSConsts.L2C.Transpose() ;

        // Put light into object space
        vector3 LocalLight(-RenderNode.Light.Dir.X, -RenderNode.Light.Dir.Y, -RenderNode.Light.Dir.Z) ;
        LocalLight = RenderNode.L2W.InvRotateVector(LocalLight) ;
        LocalLight.Normalize() ;

        // Get material color
        const vector4& MatColor = Material.GetColor() ;

	    // Setup color
        vector4 Col ;
        if (Material.GetTextureInfo(material::TEXTURE_TYPE_DIFFUSE).Count)
        {
            // Use texture and color multiplier
            Col.X = DrawInfo.Color[0] ;
            Col.Y = DrawInfo.Color[1] ;
            Col.Z = DrawInfo.Color[2] ;
            Col.W = MatColor.W * DrawInfo.Color[3] ; 
        }
        else
        {
            // User material color and color multiplier
            Col.X = MatColor.X * DrawInfo.Color[0] ; 
            Col.Y = MatColor.Y * DrawInfo.Color[1] ; 
            Col.Z = MatColor.Z * DrawInfo.Color[2] ; 
            Col.W = MatColor.W * DrawInfo.Color[3] ; 
        }

        // Setup lighting
        if ((Material.GetSelfIllumFlag()) || (DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_LIGHTING))
        {
			// Take self illum color also?
			Col.X *= DrawInfo.SelfIllumColor[0] ;
			Col.Y *= DrawInfo.SelfIllumColor[1] ;
			Col.Z *= DrawInfo.SelfIllumColor[2] ;
			Col.W *= DrawInfo.SelfIllumColor[3] ;

            // Trick vertex shader into using no diffuse, but full ambient
            VSConsts.LightDir.Zero() ;
            VSConsts.LightDiffuseCol.Zero() ;
            VSConsts.LightAmbientCol = Col ;

            VSConsts.LightAmbientCol.W *= RenderNode.Light.Amb.W ; // vertex shaders use ambient w as vertex alpha!
        }
        else
        {
            // Use regular lighting
            VSConsts.LightDir.Set(LocalLight.X, LocalLight.Y, LocalLight.Z, 0) ;
            VSConsts.LightDiffuseCol = RenderNode.Light.Col ;
            VSConsts.LightAmbientCol = RenderNode.Light.Amb ; // W component is node visiblity! 

            // Mult by material color so we don't have to do it in the vertex shader...
            VSConsts.LightDiffuseCol.X *= Col.X ;
            VSConsts.LightDiffuseCol.Y *= Col.Y ;
            VSConsts.LightDiffuseCol.Z *= Col.Z ;

            VSConsts.LightAmbientCol.X *= Col.X ;
            VSConsts.LightAmbientCol.Y *= Col.Y ;
            VSConsts.LightAmbientCol.Z *= Col.Z ;
            VSConsts.LightAmbientCol.W *= Col.W ;    // vertex shaders use ambient w as vertex alpha!
        }

        // Setup fog
        VSConsts.FogCol.X = (f32)DrawInfo.FogColor.R * (1.0f / 255.0f) ;
        VSConsts.FogCol.Y = (f32)DrawInfo.FogColor.G * (1.0f / 255.0f) ;
        VSConsts.FogCol.Z = (f32)DrawInfo.FogColor.B * (1.0f / 255.0f) ;
        VSConsts.FogCol.W = 1.0f - ((f32)DrawInfo.FogColor.A * (1.0f / 255.0f)) ;

		// Load the reflection matrix for stage 1?
        if (        (Material.GetTextureInfo(material::TEXTURE_TYPE_REFLECT).Count)
                &&  (!(DrawInfo.Flags & shape::DRAW_FLAG_TURN_OFF_REFLECT_PASS)) )
        {
            matrix4 Reflect ;
            if (DrawInfo.Flags & shape::DRAW_FLAG_REF_RANDOM)
            {
                // Random reflection
                Reflect.Identity() ;
                Reflect.SetRotation(radian3(x_frand(-R_180, R_180), x_frand(-R_180, R_180), x_frand(-R_180, R_180))) ;
            }
            else
            {
                // Get look direction in view space
                vector3 LookOffset  = View->GetW2V() * RenderNode.L2W.GetTranslation() ;
                LookOffset.Normalize() ;
                vector3 UpOffset    = v3_Cross(LookOffset, vector3(1,0,0)) ;
                vector3 RightOffset = v3_Cross(UpOffset, LookOffset) ;

                // Setup look matrix (used to rotate co-ords so that environment map moves when object does)
                matrix4 Look ;
                Look(0, 0) = RightOffset.X; Look(0, 1) = UpOffset.X;  Look(0, 2) = LookOffset.X;  Look(0, 3) = 0.0f;
                Look(1, 0) = RightOffset.Y; Look(1, 1) = UpOffset.Y;  Look(1, 2) = LookOffset.Y;  Look(1, 3) = 0.0f;
                Look(2, 0) = RightOffset.Z; Look(2, 1) = UpOffset.Z;  Look(2, 2) = LookOffset.Z;  Look(2, 3) = 0.0f;
                Look(3, 0) = Look(3, 1) = Look(3, 2) = Look(3, 3) = 0.0f ;

                // World space?
                if (DrawInfo.Flags & shape::DRAW_FLAG_REF_WORLD_SPACE)
                {
                    // World space reflection
		            Reflect = Look * RenderNode.L2W ;
                }
                else
                {
                    // View space reflection
                    Reflect = Look * View->GetW2V() * RenderNode.L2W ;
                }

                // Make it so uv's go from 0->1
                // NOTE: the - sign is so that the relfection map orientates like the map itself
                // (that way you can use a sky texture etc)
                Reflect.Orthogonalize() ;
                Reflect.Scale(-0.5f) ;
                Reflect.SetTranslation(vector3(0.5f, 0.5f, 0.5f)) ;
            }

            Reflect.Transpose() ;
            VSConsts.Ref = Reflect ;
        }

        // Setup pixel shader constants
        d3d_ps_consts   PSConsts ;

        // Get light dir - need to bias and scale, because pixel shader will clamp these values
        PSConsts.LightDir        = VSConsts.LightDir ;
        PSConsts.LightDiffuseCol = VSConsts.LightDiffuseCol ;
        PSConsts.LightAmbientCol = VSConsts.LightAmbientCol ;

        // Setup H vector = 0.5 * (L + V)
        vector3 ViewDir  = -View->GetViewZ() ;
        vector3 LightDir(-RenderNode.Light.Dir.X, -RenderNode.Light.Dir.Y, -RenderNode.Light.Dir.Z) ;
        vector3 H = LightDir + ViewDir ;
        H = RenderNode.L2W.InvRotateVector(H) ;
        H.Normalize() ;
        //H *= 0.0f ; // half as bright!
        PSConsts.SpecDir.Set(H.X, H.Y, H.Z, 1) ;
        PSConsts.SpecCol.Set(1.0f, 1.0f, 1.0f, 0.0f) ;

        // Set vertex shader constants into hardware
        g_pd3dDevice->SetVertexShaderConstant(0, &VSConsts, sizeof(VSConsts)/16 ) ;

        // Set pixel shader constants into hardware
        g_pd3dDevice->SetPixelShaderConstant(0, &PSConsts, sizeof(PSConsts)/16 ) ;
    }

    // Use vertex buffers?
    if (m_pVB)
    {
        // Draw from vertex buffer
        g_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(d3d_vert) );

        // Without this check, D3D can crash when the device is lost (due to alt+tab)
        if (g_pd3dDevice->TestCooperativeLevel() == D3D_OK)
            g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,    // PrimitiveType,
                                        0,                      // Start vertex
                                        m_nPolys) ;             // PrimitiveCount
    }
    else
    {
        // Draw strip from main memory

        // Without this check, D3D can crash when the device is lost (due to alt+tab)
        if (g_pd3dDevice->TestCooperativeLevel() == D3D_OK)
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,  // PrimitiveType,
                                          m_nPolys,             // PrimitiveCount,
                                          (void *)m_DList,      // verts
                                          m_VertSize) ;			// vert stride
    }

    #ifdef X_DEBUG
        material::s_DListDraw++ ;
    #endif
}


//==============================================================================
// material class
//==============================================================================

// Draws the material into the z buffer only (frame buffer is untouched)
void material::PrimeZBuffer(const material_draw_info&   DrawInfo,
                            const texture_draw_info&    TexDrawInfo,
                                  s32&                  CurrentLoadedNode)
{
    // Turn off updating the frame buffer
    //g_pd3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0) ;

    // Use diffuse shader
    d3deng_ActivateVertexShader( shape::s_hDifVertexShader );
    d3deng_ActivatePixelShader ( shape::s_hDifPixelShader  ) ;

    // Trick card into not changing the frame buffer, but update the z buffer
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE) ; 
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    // Misc settings
    g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE) ;
	g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE) ;
    g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE) ;

    // Setup clipping
    if (DrawInfo.Flags & shape::DRAW_FLAG_CLIP)
        g_pd3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE) ;
    else
        g_pd3dDevice->SetRenderState( D3DRS_CLIPPING, FALSE) ;

    // Draw material
    Draw(DrawInfo, CurrentLoadedNode) ;

    // Restore settings
    ActivateSettings(DrawInfo, TexDrawInfo) ;
    ActivateTextures(DrawInfo, TexDrawInfo) ;

    //g_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA |
                                                          //D3DCOLORWRITEENABLE_BLUE  |
                                                          //D3DCOLORWRITEENABLE_GREEN |
                                                          //D3DCOLORWRITEENABLE_RED );

    // Force new node load since reflection matrix is not setup and might be needed
    CurrentLoadedNode = -1 ;
}

//==============================================================================

// Converts a FLOAT to a DWORD for use in SetRenderState() calls
DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }

// Activate settings ready for drawing
void material::ActivateSettings(const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo)
{
    // Lookup textures
    texture* AlphaTEX   = TexDrawInfo.Texture[TEXTURE_TYPE_ALPHA] ;
    texture* DiffuseTEX = TexDrawInfo.Texture[TEXTURE_TYPE_DIFFUSE] ;
    texture* ReflectTEX = TexDrawInfo.Texture[TEXTURE_TYPE_REFLECT] ;
    texture* BumpTexture    = TexDrawInfo.Texture[TEXTURE_TYPE_BUMP] ;

    // Setup reflection texture stage settings
	if (!ReflectTEX)
	{		
        // Activate textures
        g_pd3dDevice->SetTexture( 1, NULL ) ;

        // Use textured diffuse shaders
        if (DiffuseTEX)
        {
            d3deng_ActivateVertexShader( shape::s_hTexDifVertexShader );
            d3deng_ActivatePixelShader ( shape::s_hTexDifPixelShader  ) ;
        }
        else
        {
            d3deng_ActivateVertexShader( shape::s_hDifVertexShader );
            d3deng_ActivatePixelShader ( shape::s_hDifPixelShader  ) ;
        }
	}
	else
	{
        if (BumpTexture)
        {
            // Stage0=Diffuse, Stage1=Bump, Stage2=Reflect, Stage3=Detail

            // Use diffuse, reflection, bump shaders
            if (shape::s_UseDot3)
            {
                d3deng_ActivateVertexShader( shape::s_hTexDifDot3VertexShader );
                d3deng_ActivatePixelShader ( shape::s_hTexDifDot3PixelShader  ) ;
            }
            else
            {
                d3deng_ActivateVertexShader( shape::s_hTexDifRefBumpVertexShader );
                d3deng_ActivatePixelShader ( shape::s_hTexDifRefBumpPixelShader  ) ;
            }
            
            // Setup bump texture stage
            g_pd3dDevice->SetTexture( 1, vram_GetSurface(BumpTexture->GetXBitmap()) ) ;
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MIPFILTER, D3DTEXF_NONE ) ;
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

            static f32 mat00=0.5f ;
            static f32 mat01=0.0f ;
            static f32 mat10=0.0f ;
            static f32 mat11=0.5f ;
            static f32 scale=1.0f ;
            static f32 offset=0.0f ;

            //static f32 t=0 ;

            //mat00 = 0.5f + (0.5f*x_sin(t)) ;
            //mat11 = 0.5f + (0.5f*x_sin(t)) ;

            //t += R_1*0.1f ;

            //x_printfxy(1,1,"%f", mat00) ;

            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVMAT00, F2DW(mat00) );
            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVMAT01, F2DW(mat01) );
            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVMAT10, F2DW(mat10) );
            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVMAT11, F2DW(mat11) );
            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVLSCALE, F2DW(scale) );
            g_pd3dDevice->SetTextureStageState( 2, D3DTSS_BUMPENVLOFFSET, F2DW(offset) );

            // Setup reflect texture stage
            g_pd3dDevice->SetTexture( 2, vram_GetSurface(ReflectTEX->GetXBitmap()) ) ;
		    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
		    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

		    // Terminate texture stages
            g_pd3dDevice->SetTexture( 3, NULL ) ;
        }
        else
        {

            // Stage0=Diffuse, Stage1=Reflect, Stage2=Detail

            // Use diffuse, reflection shaders
            d3deng_ActivateVertexShader( shape::s_hTexDifRefVertexShader );
            d3deng_ActivatePixelShader ( shape::s_hTexDifRefPixelShader  ) ;

            // Setup reflection stage
            g_pd3dDevice->SetTexture( 1, vram_GetSurface(ReflectTEX->GetXBitmap()) ) ;
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP );
		    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP );

		    // Terminate texture stages
            g_pd3dDevice->SetTexture( 2, NULL ) ;
	    }
    }

    // Setup diffuse texture settings
    if (DiffuseTEX)
    {
        // Activate textures
        g_pd3dDevice->SetTexture( 0, vram_GetSurface(DiffuseTEX->GetXBitmap()) ) ;

	    // Set bilinear mode
	    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
	    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );

	    // Turn on the trilinear blending
	    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );

	    // Set the wraping behavier
	    if (m_Flags.UMirror)
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_MIRROR );
	    else
	    if (m_Flags.UWrap)
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
	    else
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );

	    if (m_Flags.VMirror)
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_MIRROR );
	    else
	    if (m_Flags.VWrap)
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
	    else
		    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    }
    else
    {
        // Turn off texturing
        g_pd3dDevice->SetTexture( 0, NULL ) ;
    }

    // Setup final alpha blending

    // Set additive alpha blend?
    if (m_Flags.Additive)
    {
        g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
    }
    else
    {
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) ;
        g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) ;
    }

    // turn on z buffer
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE) ; 

    #ifdef X_DEBUG
        s_LoadSettings++ ;
    #endif
}

//==============================================================================

// Activates color etc for material, given new draw info
void material::ActivateColor(const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo)
{ 
    //======================================================================
    // Setup fog
    //======================================================================
    
    // Setup fog color
    if (DrawInfo.FogColor.A != 0)
    {
    	g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE) ;
        g_pd3dDevice->SetRenderState(D3DRS_FOGCOLOR, DrawInfo.FogColor) ;
    }
    else
    {
    	g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE) ;
    }


    //======================================================================
    // Setup D3D material
    //======================================================================
	// Setup alpha
    f32 Alpha ;
    if (TexDrawInfo.Texture[TEXTURE_TYPE_DIFFUSE])
        Alpha = DrawInfo.Color[3] ; 
    else
        Alpha = m_Color[3]  * DrawInfo.Color[3] ; 

    // Use alpha blend?
    xbool UseAlphaBlend ;
    if (m_Flags.PunchThrough)
        UseAlphaBlend = FALSE ;
    else
    if (m_Flags.Additive)
        UseAlphaBlend = TRUE ;
    else
    if ((m_Flags.HasAlpha) || (Alpha != 1.0f))   // texture need alpha blending?
        UseAlphaBlend = TRUE ;
    else
        UseAlphaBlend = FALSE ;

    // Setup alpha compare test
    if (m_Flags.PunchThrough)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE) ;           
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) ;
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 128) ;
    }
    else
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE) ;           
    }

    // Setup alpha enable and z write enable
    if (UseAlphaBlend)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE) ; 
        g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     FALSE) ;
    }
    else
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE) ; 
        g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,     TRUE) ;
    }

    // Setup clipping
    if (DrawInfo.Flags & shape::DRAW_FLAG_CLIP)
        g_pd3dDevice->SetRenderState( D3DRS_CLIPPING, TRUE) ;
    else
        g_pd3dDevice->SetRenderState( D3DRS_CLIPPING, FALSE) ;

    #ifdef X_DEBUG
        s_LoadColor++ ;
    #endif
}

#endif  //#ifdef TARGET_PC

//==============================================================================

// Builds one single strip with all the verts in it
void material::PC_BuildBigSingleStrip(s32 nNode, 
                                      s32 nModelVerts, 
                                      strip_vert* &StripVerts, 
                                      s32 &nStripVerts)
{
    s32 Pass, s,i,v ;

    (void)nModelVerts;    // Surpress a warning.

    // Incase of fail
    StripVerts  = NULL ;
    nStripVerts = 0 ;

    // 1st pass counts total verts, 2nd fills in
    for (Pass = 0 ; Pass < 2 ; Pass++)
    {
        // Count verts in section   
	    s32 nVerts = 0 ;

        // Process all strips
        for (s = 0 ; s < m_nStrips ; s++)
        {
            strip &Strip = m_Strips[s] ;
            if (Strip.GetNode() == nNode)
            {
                // Loop through all verts in this strip
                for (v = 0 ; v < Strip.GetVertCount() ; v++)
                {
                    // Grab vert
                    strip_vert &Vert = Strip.GetVert(v) ;
                    s32 VertIndex    = Vert.GetVertIndex() ;
                    s32 VertNoKick   = Vert.GetNoKick() ;
                    s32 VertOrient   = Vert.GetOrient() ;

                    // If no kick, copy the last vert again
                    if (VertNoKick)
                    {
                        if (Pass)
                        {
                            s32 LastVertIndex ;
                            if (nVerts)
                                LastVertIndex = StripVerts[nVerts-1].GetVertIndex() ;
                            else
                                LastVertIndex = Strip.GetVert(0).GetVertIndex() ;

                            // Setup big strip vert
                            ASSERT(nVerts < nStripVerts) ;
                            StripVerts[nVerts].SetVertIndex(LastVertIndex) ;
                            StripVerts[nVerts].SetOrient(VertOrient) ;
                            StripVerts[nVerts].SetNoKick(TRUE) ;
                            nVerts++ ;
                        }
                        else
                            nVerts++ ;
                    }

                    // Copy vert
                    if (Pass)
                    {
                        ASSERT(nVerts < nStripVerts) ;
                        StripVerts[nVerts].SetVertIndex(VertIndex) ;
                        StripVerts[nVerts].SetOrient(VertOrient) ;
                        StripVerts[nVerts].SetNoKick(VertNoKick) ;
                        nVerts++ ;
                    }
                    else
                        nVerts++ ;
                }
            }
        }

        // Allocate strip verts?
        if (!Pass)
        {
            StripVerts  = new strip_vert[nVerts] ;
            nStripVerts = nVerts ;
            for (i = 0 ; i < nVerts ; i++)
                StripVerts[i].SetVertIndex(-1) ;
        }
        else
        {
            // Check strips!
            ASSERT(nVerts == nStripVerts) ;
            for (i = 0 ; i < nVerts ; i++)
            {
                ASSERT(StripVerts[i].GetVertIndex() >= 0) ;
                ASSERT(StripVerts[i].GetVertIndex() < nModelVerts) ;
            }
        }
    }

    // Show info
    shape::printf("   finished building PC combined strip with %d verts\n", nStripVerts) ;
}

//==============================================================================

// Builds material display list so that it can be referenced by cpu
void material::PC_BuildDList(vert       ModelVerts[], 
                             s32        nModelVerts, 
                             s32        Node,
                             strip_vert StripVerts[],
                             s32        nStripVerts,
                             byte*      &DList,
                             s32        &DListSize,       
                             s32        &nDListVerts, 
                             s32        &nDListPolys)
{
    (void)Node ;
    (void)nModelVerts;

    // Update stats
    nDListVerts += nStripVerts ;
    nDListPolys += nStripVerts-2 ;

	// Create new display list
	DListSize   = sizeof(d3d_vert) * nStripVerts ;
	DList       = new byte[DListSize] ;

	// Create and setup a new list of d3d verts
	d3d_vert *D3DVerts = (d3d_vert *)DList ;
	ASSERT(D3DVerts) ;
	for (s32 v = 0 ; v < nStripVerts ; v++)
	{
		s32 VertIndex = StripVerts[v].GetVertIndex() ;
		ASSERT(VertIndex >= 0) ;
		ASSERT(VertIndex < nModelVerts) ;

		vert&   Vert = ModelVerts[VertIndex] ;
		D3DVerts[v].vPos        = Vert.GetPos() ;
		D3DVerts[v].vNormal     = Vert.GetNormal() ;
		D3DVerts[v].TexCoord    = Vert.GetTexCoord() ;
        
        // Lowest bit of pos X co-ord is the cull direction!
        u32* Orient = (u32*)(&D3DVerts[v].vPos.X) ;
        if (StripVerts[v].GetOrient())
            *Orient |= 1 ;
        else
            *Orient &= ~1 ;

        // Lowest bit of pos Y co-ord is the no kick flag
        u32* NoKick = (u32*)(&D3DVerts[v].vPos.Y) ;
        if (StripVerts[v].GetNoKick())
            *NoKick |= 1 ;
        else
            *NoKick &= ~1 ;
	}

    // Add strip to current dlist
    dlist_strip DListStrip ;
    DListStrip.Setup(nStripVerts, NULL, NULL, NULL) ; // All relative
    m_DLists[m_nDLists].AddStrip(DListStrip) ;
}

//==============================================================================

// Builds material display list so that it can be referenced by cpu
void material::BuildDLists(s32                          nTotalPolys, 
                           vert                         ModelVerts[], 
                           s32                          nModelVerts, 
                           s32                          nNodes, 
                           s32&                         nDListVerts, 
                           s32&                         nDListPolys,
                           shape_bin_file_class::target Target)
{
    s32 i,j ;

    // Stop warning
    (void)nTotalPolys ;

    // Clear node flags
    m_NodeFlags = 0 ;

    // Calculate how many nodes have this material - we need a dlist for each node
    s32 nDLists = 0 ;
    for (i = 0 ; i < nNodes ; i++)
    {
        // Loop through strips to see if any are attached to this node
        for (j = 0 ; j < m_nStrips ; j++)
        {
            // Node have a vert attached?
            if (m_Strips[j].GetNode() == i)
            {
                // Increment the dlists needed and goto next node
                nDLists++ ;
                break ;
            }
        }
    }

    // Allocate number of dlists needed
    m_DLists    = new material_dlist[nDLists] ;
    m_nDLists   = 0 ;
    ASSERT(m_DLists) ;

    // Loop through all the nodes
    for (i = 0 ; i < nNodes ; i++)
    {
        strip_vert *StripVerts ;
        s32         nStripVerts ;

        // Build a single strip for this node
        switch(Target)
        {
            case shape_bin_file_class::PC:
                PC_BuildBigSingleStrip(i, nModelVerts, StripVerts, nStripVerts) ;
                break ;

            default:
                ASSERTS(0, "Target not supported!") ;
                break ;
        }

        // Any geometry in this material attached to this node?
        if (nStripVerts)
        {
            byte *DList    = NULL;
            s32  DListSize = 0 ;
			s32	 nVerts    = 0 ;
			s32	 nPolys    = 0 ;

            // Flags material is on this node
            if (i < 32)     // Only 32 nodes supported currently!!!
                m_NodeFlags |= 1 << i ;
            else
                m_NodeFlags = 0xFFFFFFFF ; // Currently no control over materials on nodes >= 32

            // Build display list for material and node
            switch(Target)
            {
                case shape_bin_file_class::PC:
                    PC_BuildDList(ModelVerts, 
                                  nModelVerts, 
                                  i,
                                  StripVerts,
                                  nStripVerts,
                                  DList,
                                  DListSize,
                                  nVerts, 
                                  nPolys) ;
                    break ;

                default:
                    ASSERTS(0, "Target not supported!") ;
                    break ;
            }

            // Update stats
			nDListVerts += nVerts ;
			nDListPolys += nPolys ;

            ASSERT(DList) ;
            ASSERT(DListSize) ;

            // Setup the next available dlist
            m_DLists[m_nDLists++].Setup(i, nPolys, DListSize / nVerts, DList, DListSize) ;

            // Free memory used to build display list
            delete [] DList ;
            delete [] StripVerts ;
        }
    }


    // Make sure we set all the dlists up!
    ASSERT(m_nDLists == nDLists) ;
}
        
//==============================================================================

// Read/Write to binary file
void material::ReadWrite(shape_bin_file &File)
{
    // Write attributes
    File.ReadWrite(m_Name) ;
    File.ReadWrite(m_Color) ;
    File.ReadWrite(m_Flags) ;
    File.ReadWrite(m_NodeFlags) ;

    // Write texture ref + info indices
    File.ReadWrite(m_TextureRefs) ;
    File.ReadWrite(m_TextureInfo) ;

    // DLists
    File.ReadWriteClassArray(m_DLists, m_nDLists) ;
}

//==============================================================================

#ifdef TARGET_LINUX

//==============================================================================
// material display list class
//==============================================================================


//==============================================================================
// material class
//==============================================================================

// Draws the material into the z buffer only (frame buffer is untouched)
void material::PrimeZBuffer(const material_draw_info&   DrawInfo,
                            const texture_draw_info&    TexDrawInfo,
                                  s32&                  CurrentLoadedNode)
{
}

//==============================================================================

// Activate settings ready for drawing
void material::ActivateSettings(const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo)
{
}

//==============================================================================

// Activates color etc for material, given new draw info
void material::ActivateColor(const material_draw_info& DrawInfo, const texture_draw_info& TexDrawInfo)
{ 
}

#endif  //#ifdef TARGET_LINUX
