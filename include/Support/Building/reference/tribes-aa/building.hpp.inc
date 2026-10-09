#ifndef BUILDING_HPP
#define BUILDING_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////

#include "portal_clipper.hpp"
#include "BldFileSystem.hpp"
#include <xCore/Entropy/e_Engine.hpp>
#include <xCore/x_files/x_bitmap.hpp>

#define ONEPASS     1
#define PALETTED    1

#if !ONEPASS                    // 3 pass lightmaps MUST be paletted!
#undef PALETTED
#define PALETTED    1
#endif

///////////////////////////////////////////////////////////////////////////
// Building
///////////////////////////////////////////////////////////////////////////

class building
{
//=========================================================================
// PUBLIC TYPES
//=========================================================================
public:

    struct ngon
    {
        vector2 MinUV, MaxUV;
        vector2 UV[3];
        vector3 Pt[3];
        vector3 Centre;
        plane   Plane;
        s32     DP;
        xbool   BadUV;
    };

    struct material
    {
        char        FileName[256];
    };

    struct lightmap
    {
        const xbitmap*      pLightMap;
        s32*                pHandle;
        xbool               bAlarmMap;
    };

    struct dlist
    {
#if defined(TARGET_PC)
        struct pc_data
        {
            byte*                   pDList;
            IDirect3DVertexBuffer8* pVBuffer  ;
            IDirect3DIndexBuffer8*  pIBuffer  ;
            s32                     FacetCount;
        };

        struct pc_vertex
        {
            vector3     Pos;
            vector2     UV[2];
        };
#endif

#if defined(TARGET_LINUX)
        struct pc_data
        {
            byte*                   pDList;
            //IDirect3DVertexBuffer8* pVBuffer  ;
            //IDirect3DIndexBuffer8*  pIBuffer  ;
            s32                     FacetCount;
        };

        struct pc_vertex
        {
            vector3     Pos;
            vector2     UV[2];
        };
#endif
        u16         PassMask;           // Set the mask for the display list
        u16         nGons;              // Number of 'Nv'-polygons in the display list     
        u16         nVertices;          // Number of vertices in the display list
        u16         SizeData;           // Size of the data
        u16         iHash;              // Which HashID this display list belowns
        u16         bClip;              // Whether we need to clip this dlist
        s32         bAlarm;             // Whether there is an alarm lightmap
        dlist*      pNext;              // Next dlist to render

        vector3     BBMin, BBMax;       // Bounding box for the displaylist
        f32         WorldPixelSize;     // Average world pixel size.
        f32         K;                  // Mip K value.

        byte*       pData;              // Data for the display list
    };

    struct zone
    {
        u16         nPortals;
        u16         iPortal;
        u16         iDList;
        u16         nDLists;
    };

    struct raycol
    {
        vector3 Point;                  // Point of the collision
        f32     T;                      // The T of the collision
        plane   Plane;                  // Plane of the collision
    };

    struct prep_render_info
    {
        u64     ZonesRendered;          // A bit        
        xbool   bViewInZoneZero;        // Whether we are in zone zero or not
        rect    OutPortal;              // If we are not in zone zero but we can see zone zero
    };                                  // This is the viewport that we must clipwith.

//=========================================================================
// PUBLIC FUNCTIONS
//=========================================================================
public:

    void    Initialize              ( void );
    void    DeleteBuilding          ( void );

    dlist*  PrepRender              ( const matrix4&    L2W, 
                                      const matrix4&    W2L, 
                                      const view&       View, 
                                      prep_render_info& RenderInfo,
                                      u32               SkipPlaneMask,
                                      f32               ClipX,
                                      f32               ClipY );

    s32     FindZoneFromPoint       ( const vector3& Point ) const;
    xbool   IsPointInside           ( const vector3& Point ) const;
    xbool   CastRay                 ( const vector3& S, const vector3& E, f32& Dist, vector3& Point ) const;
    xbool   CastRay                 ( const vector3& S, const vector3& E, raycol& Info ) const;

    void    ExportData              ( X_FILE* Fp, material* pMaterial );
    s32     ImportData              ( X_FILE* Fp, xbool LoadAlarmMaps );

    void    IOFields                ( bld_fileio& FileIO );
    void    ExportData              ( X_FILE* Fp, bld_fileio& FileIO, material* pMaterialList );
    s32     ImportData              ( X_FILE* Fp, bld_fileio& FileIO, xbool LoadAlarmMaps );

    void    GetStats                ( s32& DList, s32& Collision, s32& BSPData, s32& PlaneData );

//=========================================================================
// PUBLIC VARIABLES
//=========================================================================
public:

    s32                 m_nDLists;
    dlist*              m_pDList;

    s32                 m_nZones;
    zone*               m_pZone;

    s32                 m_nLightMaps;
    lightmap*           m_pLightMap;
    
//=========================================================================
// PROTECTED TYPES
//=========================================================================
protected:

    //--------------------------------------------------------------------
    // Render types
    //--------------------------------------------------------------------
    struct uv
    {
        s16         U, V;
    };

    struct portal
    {
        u16         nPoints;
        u16         iPoint;
        u16         iZone;
        u16         iZoneGoingTo;
        plane       Plane;
    };

    struct hash_material
    {
        u16         iMaterialBase;
        u16         iMaterialLightMap;
    };

    struct zone_walk
    {
        u32                 SkipPlaneMask;
        u32                 RenderZoneMask[16];
        u32                 VisitedZoneMask[16];
        s32                 MinIndex[6*3*2];
        s32                 MaxIndex[6*3*2];
        u64                 ZonesRendered;
        dlist*              pList;
        plane               Plane[6*2];
        const view*         pView;
        prep_render_info*   pRenderInfo;

        s32                 nRejected;
        s32                 nNoCliped;
        s32                 nCliped;
    };

    //--------------------------------------------------------------------
    // Collision types
    //--------------------------------------------------------------------

    struct bspnode
    {
        u16         iPlane;             // Index to the plane
        u16         iFront;             // Index to another bspnode
        u16         iBack;              // index to another bspnode

        u16         iTerminalZone;      // if high bit set, then the lower 15 bits are the zone
                                        // of any of the subsidiary nodes.  Note that this is
                                        // going to overestimate some, since an object could be
                                        // completely contained in solid, but it's probably
                                        // going to turn out alright.
    };

#define BUILDING_GRID_CELL_SIZE 4.0f
public:
    struct building_grid
    {
        struct ngon
        {
            plane       Plane;
            bbox        BBox;
            vector4*    pVert;
            s32         nVerts;
            s32         Seq;
        };

        struct cell
        {
            u16         nNGons;
            u16         RefIndex;
        };

        bbox    m_BBox;

        s32     m_nNGons;
        ngon*   m_pNGon;

        s32     m_GridSizeX;
        s32     m_GridSizeY;
        s32     m_GridSizeZ;
        s32     m_YOffset;
        s32     m_ZOffset;
        s32     m_nGridCells;
        cell*   m_pCell;

        s32     m_nNGonRefs;
        u16*    m_pNGonRef;

        mutable s32     m_Sequence;

        s32     m_MemSize;
        s32     m_nEmptyCells;
        s32     m_MaxNGonsPerCell;
    };

    xcolor  LookupLighting          ( const building_grid::ngon& NGon, 
                                      const vector3& Point,
                                      s32   HashAdd,
                                      xbool UseAlarm ) const;
                                      
    void    LightBuilding           ( s32 Instance, const matrix4& L2W );
    s32     LightPixel              ( const vector3& Point, f32 DP );

    void    DrawPortal              ( const portal&  Portal, const vector3* pPortalPoint, xcolor Color );
    void    RenderPortals           ( const matrix4& L2W );

//=========================================================================
// PROTECTED FUNCTIONS
//=========================================================================
protected:

    //--------------------------------------------------------------------
    // Render functions
    //--------------------------------------------------------------------
    void    PrepRenderZone          ( s32 iZone, u32 SkipPlaneMask );
    void    PrepRenderFromZone      ( portal_clipper& PortalClipper, s32 iZone, u32 SkipPlaneMask );
    s32     IsBoxInView             ( const bbox& BBox, const plane* const pClipPlane, u32 SkipPlaneMask ) const;
    void    CompilePCData           ( dlist& pDList );

    //--------------------------------------------------------------------
    // Collision functions
    //--------------------------------------------------------------------
    xbool                   CastRay             ( const u16 iNode, const u16 iPlane,const vector3& S, 
                                                  const vector3& E, raycol& Info ) const;
    inline xbool            isBSPLeafIndex      ( u16 Index ) const;
    inline xbool            isBSPSolidLeaf      ( u16 Index ) const;
    inline s32              getBSPZone          ( u16 Index ) const;
    inline const plane&     getBSPPlane         ( u16 Index ) const;
    inline xbool            IsBSPPlaneFlipped   ( u16 Index ) const;
    inline void             getBSPFormatedPlane ( u16 Index, plane& Plane ) const;
    inline void             BSPFormatPlane      ( u16 Index, plane& Plane ) const;


//=========================================================================
// PROTECTED VARIABLES
//=========================================================================
protected:

    //--------------------------------------------------------------------
    // Render variables
    //--------------------------------------------------------------------

    s32                 m_nBytesData;
    char*               m_pDListData;

    s32                 m_nPortals;
    portal*             m_pPortal;
    s32                 m_nPortalPoints;
    vector3*            m_pPortalPoint;

    s32                 m_nMaterials;
    const xbitmap**     m_pMaterialHandle;

    s32                 m_nHashs;
    u16*                m_pHash;
    hash_material*      m_pHashMat;

    static zone_walk    s_ZoneWalk;

    //--------------------------------------------------------------------
    // Collision variables
    //--------------------------------------------------------------------

    s32                 m_nBSPNodes;
    bspnode*            m_pBSPNode;

    s32                 m_nPlanes;
    plane*              m_pPlane;        

public:
    building_grid       m_CGrid;

//=========================================================================
//=========================================================================

    friend class invbuilding;
};

//=========================================================================

struct bld_debug
{
    xbool DoubleBuffer;
    xbool ExecVU;
    xbool LoadVerts;
    xbool LoadMatrices;
    xbool LoadBase;
    xbool LoadLightmap;
    xbool BasePass;
    xbool LMapPass;
    xbool MonoPass;
    xbool FogPass;
    xbool BaseTextured;
    xbool LMapTextured;
    xbool FogTextured;
    xbool AlphaBlending;
    xbool Bilinear;
    xbool FlushVRAM;
    xbool UseVU;
    xbool ShowOverdraw;
    xbool ShowPortals;
    xbool ShowLookupColors;
    xbool ShowCellNGons;
    xbool ShowGrid;
    xbool ShowRayCast;
};

extern bld_debug g_BldDebug;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// INLINE
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//=========================================================================

inline xbool building::isBSPLeafIndex( u16 Index ) const
{ 
    return (Index & 0x8000) != 0; 
}

//=========================================================================

inline xbool building::isBSPSolidLeaf( u16 Index ) const
{ 
    ASSERT( isBSPLeafIndex(Index) );
    return (Index & 0x4000) != 0;        
}

//=========================================================================

inline s32 building::getBSPZone( u16 Index ) const
{ 
    ASSERT( isBSPSolidLeaf(Index) == FALSE );
    return (Index & ~0xC000); 
}

//=========================================================================

inline const plane&  building::getBSPPlane( u16 Index ) const
{ 
    return m_pPlane[ Index & ~0x8000 ]; 
}

//=========================================================================

inline xbool building::IsBSPPlaneFlipped( u16 Index ) const
{
    return Index >> 15;
}

//=========================================================================

inline void building::BSPFormatPlane( u16 Index, plane& Plane ) const
{ 
    if( IsBSPPlaneFlipped( Index ) ) Plane.Negate(); 
}

//=========================================================================

inline void building::getBSPFormatedPlane( u16 Index, plane& Plane ) const
{ 
    Plane = getBSPPlane( Index ); 
    BSPFormatPlane( Index, Plane ); 
}

//=========================================================================

xbool SkipBuilding          ( const char *pFileName );
void  RenderLookupColors    ( void );

///////////////////////////////////////////////////////////////////////////
// END
///////////////////////////////////////////////////////////////////////////
#endif
