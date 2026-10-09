#ifndef BLD_MANAGER_HPP
#define BLD_MANAGER_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_bitmap.hpp>
#include "building.hpp"

class fog;

//#define BLD_TIMERS

///////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////

class bld_manager
{
///////////////////////////////////////////////////////////////////////////
public:

#ifdef BLD_TIMERS
    struct timers
    {
        xtimer  PREP_PortalWalk;
        xtimer  PREP_CopyMatrices;
        xtimer  PREP_Init;
        xtimer  PREP_CopyDLists;

        xtimer  BLD_BuildPlanes;
        xtimer  BLD_FindZone;
        xtimer  BLD_WalkPortals;

        xtimer  BLD_PrepZone;
        xtimer  BLD_Walk;
        xtimer  BLD_PortalClipperSetup;

        s32     DListnRejected;
        s32     DListnNoCliped;
        s32     DListnCliped;


        inline void Reset( void )
        {
            PREP_PortalWalk.Reset();
            PREP_CopyMatrices.Reset();
            PREP_Init.Reset();
            PREP_CopyDLists.Reset();

            BLD_BuildPlanes.Reset();
            BLD_FindZone.Reset();
            BLD_WalkPortals.Reset();
            BLD_Walk.Reset();
            BLD_PrepZone.Reset();
            BLD_PortalClipperSetup.Reset();

            DListnRejected  = 0;
            DListnNoCliped  = 0;
            DListnCliped    = 0;
        }

        inline void Output( void )
        {
            x_DebugMsg( "PREP_PortalWalk:      %f\n", PREP_PortalWalk.GetAverageMs() );
            x_DebugMsg( "PREP_CopyMatrices:    %f\n", PREP_CopyMatrices.GetAverageMs() );
            x_DebugMsg( "PREP_Init:            %f\n", PREP_Init.GetAverageMs() );
            x_DebugMsg( "PREP_CopyDLists:      %f\n", PREP_CopyDLists.GetAverageMs() );
        }
    };
#endif

///////////////////////////////////////////////////////////////////////////
public:

    void        Initialize          ( void );
    void        SetFog              ( const fog* pFog );
    void        Kill                ( void );
    void        SetLightPos         ( const vector3& Pos );
    void        SetLightDir         ( const vector3& Dir );
    void        SetMissionDir       ( const char* );

    int         GetBitmapNames      ( char*, int);
    void        RefreshTextures     ( void );
    int         RefreshTexture      ( int  );
    void        Render              ( void );
    void        CleanHash           ( void );
    xbool       IsViewInZoneZero    ( s32 ViewPortID );
    xbool       IsZoneZeroVisible   ( s32 ViewPortID );
    rect        GetZoneZeroRect     ( s32 ViewPortID );
    const char* GetBuildingName     ( const building& Building ) const;

    const xbitmap& GetBaseBitmap    ( s32 iHash );
    const xbitmap& GetLightmap      ( s32 iHash );
    xcolor         GetAverageColor  ( s32 iHash );

///////////////////////////////////////////////////////////////////////////
public:
#ifdef BLD_TIMERS
    static timers   st;
#endif

///////////////////////////////////////////////////////////////////////////
protected:

    //---------------------------------------------------------------------
    // Data manechment section
    //---------------------------------------------------------------------

    enum
    {
        MAX_BUILDINGS           = 24,
        MAX_BITMAPS             = 144,
        MAX_RENDERTABLE_NODES   = 32,
        MAX_BUILDING_INSTANCES  = MAX_BUILDINGS * 8,
        MAX_HASH_ENTRIES        = 1024,
    };

    struct bitmap_node
    {
        char            FileName[16];           // Just the file part of the name, so we don't need much
        s32             RefCount;
//        xbitmap         Bitmap;
        xbool           BuildMips;
        s32             ClutHandle[3];
        xcolor          AverageColor;

        inline bitmap_node( void )
        {
            RefCount = 0;
            ClutHandle[0]=ClutHandle[1]=ClutHandle[2]=-1;
            BuildMips=-1;
        }
    };

    struct building_node
    {
        char            FileName[32];           // Will assert if the name space is too short
        s32             RefCount;
        s32             NumInstances;
        building*       pBuilding;
    };

    //---------------------------------------------------------------------
    // Rendering section
    //---------------------------------------------------------------------

    typedef building::dlist dlist;

    struct instance;
    struct dlist_node
    {
        dlist*          pDList;         // Display list to render
        instance*       pInstance;      // Instance information
        xbool           bClip;          // Whether we need to clip this geometry 
        dlist_node*     pNext;          // Next dlist node
    };

    struct render_table
    {
        u16             iLightMap;      // Light map material
        u16             iBaseMap;       // Base map material
        f32             MinDist;        // Minimun Distance of the object
        f32             MaxDist;        // Maxumin Distance of the object
        f32             MinK;           // Maximun K 
        f32             MaxK;           // Minimun K
        dlist_node*     pDList[4];      // A pointer to each of the display list to render
    };

    struct instance
    {
        s32             m_nDListNodes;  // Number of display list for this instance
        dlist_node*     m_pDListNode;   // A pointer to the display lists
        
#ifdef TARGET_PS2
        matrix4         m_RL2C[4];      // Local to clip render matrix
        matrix4         m_RC2S[4];      // Clip to screen render matrix
        matrix4         m_RL2S[4];      // Local to screen render matrix
#else
        matrix4         m_RL2W[4];      // Local to world
        matrix4         m_RW2V[4];      // World to view
        matrix4         m_RV2C[4];      // View to Screen
#endif
        bbox            m_BBox;         // Bounding box in world space

        matrix4         m_L2W;          // Use to set the building in the world
        matrix4         m_W2L;          // Use to bring things into the building space

        building*       m_pBuilding;    // Building which we are an instance of
    
        int             m_ID;           // which instance of a building this is
        int             m_HashAdd;      // value to add to hash to get correct lightmap for this instance
        s32             m_nZones;       // Num zones this instance has
        u64             m_ZoneVisible;  // Bit=1 if zone is visible
        
        xbool           m_AlarmLighting;// when this is set to TRUE, alarm lighting will be used
    };

    typedef building::prep_render_info prep_info;

///////////////////////////////////////////////////////////////////////////
protected:

    building_node&  AddBuilding         ( const char* pFileName );
    building_node*  GetBuilding         ( const char* pFileName );
    bitmap_node&    AddBitmap           ( const char* pFileName );
    bitmap_node*    GetBitmap           ( const char* pFileName );
    s16             GetBitmapID         ( const char* pFileName );
    s16             GetHashEntryID      ( const char* pBase, const char* pLM, int instance );

    void            AddDisplayListInHash( dlist_node&   Node, 
                                          s32           ViewPortID,
                                          f32           MinDist,
                                          f32           MaxDist,
                                          s32           Instance );

    void            UpdatePrepInfo      ( prep_info&    PrepInfo,
                                          s32           ViewPortID );

    void            LoadBuilding        ( const char* pFileName, instance& Instance, xbool LoadAlarmMaps );
    void            DeleteBuilding      ( const char* pFileName );
    const xbitmap&  LoadBitmap          ( const char* pFileName,  s32** pClutHandle, xbool LoadBitmap = TRUE );
    void            ReleaseBuilding     ( building& Building );
    void            CleanUp             ( void );

///////////////////////////////////////////////////////////////////////////
protected:

    s32             m_nBitmaps;
    bitmap_node     m_Bitmap        [ MAX_BITMAPS   ];

    s32             m_nBuildings;
    building_node   m_Building      [ MAX_BUILDINGS ];

    s32             m_nRTNodes;
    render_table    m_pRTNode       [ MAX_RENDERTABLE_NODES ];

    s32             m_nInstances;
    instance*       m_pInstance     [ MAX_BUILDING_INSTANCES ];

    s32             m_nHashEntries;
    render_table    m_RenderTable   [ MAX_HASH_ENTRIES ];    
    s16             m_HashIDToTable [ MAX_HASH_ENTRIES ];    

    prep_info       m_PrepInfo      [4];

    const fog*      m_pFog;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

    friend class  bld_instance;
    friend class  building;

public:

    char           m_MissionDir[64];
    
    bitmap_node*   GetBitmapNode    ( xbitmap& Bitmap );
    void           PrintStats       ( void );
    void           RenderNGon       ( s32 iBuilding, s32 iNGon );
    
    s32            GetBuildingNum   ( building& Building );

    xbool           IsZoneVisible   ( s32 BuildingIndex, s32 ZoneID );
    void            ClearZoneInfo   ( void );
    void            RenderZoneInfo  ( void );


struct zone_set
{
    byte    BuildingID[4];
    byte    ZoneID[4];
    s32     NZones;
    xbool   InZoneZero;
};

    void            GetZoneSet          ( zone_set& ZS, const bbox& BBox, xbool JustCenter=TRUE );
    xbool           IsZoneSetVisible    ( zone_set& ZS );
};

///////////////////////////////////////////////////////////////////////////
// GLOVAL VARIABLES
///////////////////////////////////////////////////////////////////////////

extern bld_manager g_BldManager;
extern vector3     g_LightPos;
extern vector3     g_LightDir;

///////////////////////////////////////////////////////////////////////////
// END
///////////////////////////////////////////////////////////////////////////
#endif