
#include "BldManager.hpp"

///////////////////////////////////////////////////////////////////////////
// ADDITIONAL INCLUDES
///////////////////////////////////////////////////////////////////////////

#include "AUX_Bitmap.hpp"
#include "building.hpp"
#include "BldInstance.hpp"
#include "BldRender.hpp"
#include <Support/Fog/Fog.hpp>

///////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
///////////////////////////////////////////////////////////////////////////
extern volatile f32     PS2BUILDING_GLOBALK;
bld_manager             g_BldManager;
vector3                 g_LightPos;     // position of global light source in world
vector3                 g_LightDir;     // direction of global light source in world

xbool PRINT_BUILDING_STATS = FALSE;

bld_debug g_BldDebug =
{
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
    TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,
};

///////////////////////////////////////////////////////////////////////////
// STATICS VARIABLES
///////////////////////////////////////////////////////////////////////////

static xbool            s_Init          = FALSE;

#ifdef BLD_TIMERS
bld_manager::timers     bld_manager::st;
#endif

//s32 AndyLMCount=0;

///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////

//=========================================================================

void bld_manager::SetLightPos( const vector3& Pos )
{
    g_LightPos = Pos;
}

void bld_manager::SetLightDir( const vector3& Dir )
{
    g_LightDir = Dir;
    g_LightDir.Normalize();
}

void bld_manager::SetMissionDir( const char* Path )
{
    ASSERT( x_strlen( Path ) < 64 );
    x_strcpy( m_MissionDir, Path );
}

//=========================================================================

void bld_manager::Initialize( void )
{
    x_memset( this, 0, sizeof(bld_manager) );
    s_Init = TRUE;

    //
    // Initialzile the prep info structure
    //
    for( s32 i=0; i<4; i++ )
    {
        m_PrepInfo[ i ].bViewInZoneZero = TRUE;
        m_PrepInfo[ i ].OutPortal.Clear();
        m_PrepInfo[ i ].ZonesRendered   = 0;
    }
    
    //
    // Initialize the building rendering
    //
    BLDRD_Initialize();
    m_pFog = NULL;
    m_nInstances = 0;
}

//=========================================================================

void bld_manager::SetFog( const fog* pFog )
{
    m_pFog = pFog;
}

//=========================================================================

void bld_manager::Kill( void )
{
	int i;
    
    //
    // Delete all the buildings
    //
    for ( i=0; i<m_nBuildings; i++)
    {
        DeleteBuilding( m_Building[i].FileName );
    }
    
    x_memset( this, 0, sizeof( *this ) );
}

//=========================================================================

bld_manager::building_node& bld_manager::AddBuilding( const char* pFileName )
{
    ASSERT( m_nBuildings < MAX_BUILDINGS );

    ASSERT(x_strlen(pFileName) < (s32)sizeof(m_Building[m_nBuildings].FileName));
    x_strcpy( m_Building[m_nBuildings].FileName, pFileName  );
    m_Building[ m_nBuildings ].pBuilding    = new building;
    m_Building[ m_nBuildings ].RefCount     = 0;
    ASSERT( m_Building[ m_nBuildings ].pBuilding );

    return m_Building[ m_nBuildings++ ];    
}

//=========================================================================

bld_manager::building_node* bld_manager::GetBuilding( const char* pFileName )
{
    int i;
    
    ASSERT( pFileName );

    for( i=0; i<m_nBuildings; i++ )
    {
        if( x_stricmp( m_Building[i].FileName, pFileName ) == 0 ) 
        {
            m_Building[i].RefCount++;
            return &m_Building[i];
        }
    }

    return NULL;
}

s32 bld_manager::GetBuildingNum( building& Building )
{
    for( s32 i=0; i<m_nBuildings; i++ )
    {
        if( m_Building[i].pBuilding == &Building )
        {
            return( i );
        }
    }
    
    ASSERT( 0 );
    return( 0 );
}

//=========================================================================

void bld_manager::LoadBuilding( const char* pFileName, instance& Instance, xbool LoadAlarmMaps )
{
    building_node* pBuildingNode = GetBuilding( pFileName );

    if( pBuildingNode == NULL ) 
    {
        X_FILE* Fp = NULL;

        pBuildingNode = &AddBuilding( pFileName );
        Fp = x_fopen( pFileName, "rb" );
        ASSERT( Fp );

        //x_DebugMsg( "Loading Building: %s\n", pFileName );

		pBuildingNode->NumInstances = pBuildingNode->pBuilding->ImportData( Fp, LoadAlarmMaps );
        pBuildingNode->pBuilding->Initialize();

        x_fclose( Fp );
    }

    Instance.m_pBuilding = pBuildingNode->pBuilding;
	Instance.m_ID        = pBuildingNode->RefCount;

    m_pInstance[ m_nInstances ] = &Instance;
    m_nInstances++;
    //x_DebugMsg("NInstances: %1d\n",m_nInstances);

    Instance.m_nZones      = pBuildingNode->pBuilding->m_nZones;
    Instance.m_ZoneVisible = 0;

	if ( pBuildingNode->RefCount > pBuildingNode->NumInstances-1 )
	{
        Instance.m_HashAdd = pBuildingNode->NumInstances-1;
	}
	else
	{
		Instance.m_HashAdd = pBuildingNode->RefCount;
    } 
}

//=========================================================================

void bld_manager::DeleteBuilding(const char* pFileName)
{
	building_node* bn = GetBuilding(pFileName);
	
	if (!bn) return;

	bn->pBuilding->DeleteBuilding();

	delete(bn->pBuilding);

	x_memset(bn, 0, sizeof(building_node));
	bn->RefCount = 0;
}

//=========================================================================

void bld_manager::ReleaseBuilding( building& Building )
{
    s32 i;

    for( i=0; i<m_nBuildings; i++ )
    {
        if( m_Building[i].pBuilding == &Building )
            break;
    }

    ASSERTS( i < m_nBuildings, "That is strange I could not find that building" );   
    ASSERT ( m_Building[i].RefCount > 0 );
    m_Building[i].RefCount--;
}

//=========================================================================

void bld_manager::UpdatePrepInfo( prep_info& PrepInfo,
                                  s32        ViewPortID )
{
    //
    // Update the prerender info
    //
    if( PrepInfo.bViewInZoneZero == FALSE )
    {
        // This should only happens ones per view at max
        m_PrepInfo[ ViewPortID ] = PrepInfo;

        //draw_Rect( PrepInfo.OutPortal, xcolor(255,255,255,255), TRUE );
    }
    else
    {
        if( m_PrepInfo[ ViewPortID ].bViewInZoneZero == TRUE )
        {
            m_PrepInfo[ ViewPortID ].ZonesRendered |= PrepInfo.ZonesRendered;
        }
    }
}

//=========================================================================

void bld_manager::AddDisplayListInHash( dlist_node&     Node, 
                                        s32             ViewPortID,
                                        f32             MinDist,
                                        f32             MaxDist,
										s32             Instance )
{
    ASSERT( Node.pDList );
    ASSERT( ViewPortID >= 0 );
    ASSERT( ViewPortID <  4 );

    // Add the display list into the hash node
    s32 Index  = m_HashIDToTable[ Node.pDList->iHash + Instance ];
    Node.pNext = m_RenderTable[ Index ].pDList[ ViewPortID ];
    m_RenderTable[ Index ].pDList[ ViewPortID ] = &Node;

    if( MinDist < m_RenderTable[ Index ].MinDist ) m_RenderTable[ Index ].MinDist = MinDist;
    if( MaxDist > m_RenderTable[ Index ].MaxDist ) m_RenderTable[ Index ].MaxDist = MaxDist;

    if( Node.pDList->K < m_RenderTable[ Index ].MinK ) m_RenderTable[ Index ].MinK = Node.pDList->K;
    if( Node.pDList->K > m_RenderTable[ Index ].MaxK ) m_RenderTable[ Index ].MaxK = Node.pDList->K;
}


//=========================================================================

#ifndef TARGET_PC
f32 g_MaxLod = 0.25f;
//#include "AADS/globals.hpp"
#endif

volatile xbool BLDMANAGER_Log = FALSE;
static X_FILE* LogFP = NULL;

#ifdef TARGET_PS2
extern s32 VRAM_BytesLoaded;
#endif
#ifdef TARGET_PC
#define VRAM_BytesLoaded 0
#endif

#if defined( X_DEBUG ) && defined( jcossigny )

extern building::dlist* pDLists[];
extern bld_manager g_BldManager;

s32 build=-1;
s32 ngon=0;
s32 zb=0;
s32 uuuu=0x55555555;

int strips=0;
int strip=0;
s32 jp_skip=0x55555555;
s32 jp_poly=0;
s32 jp_surf=0;
s32 jp_ngon=-1;
s32 jp_end=-1;

xbitmap* jp_base;
xbitmap* jp_lmap;
char* jp_basename;
char* jp_lmapname;

building::ngon jp_ngons[256];
s32 jp_num;

s32  ComputeNGonInfo(building::ngon* pNGons, const building::dlist& DList);

#endif

//#define VRAM_STATS

//=========================================================================

void bld_manager::PrintStats( void )
{
    x_DebugMsg( "Buildings    = %d\n", m_nBuildings );

    s32 DisplayList = 0;
    s32 Collision   = 0;
    s32 BSPData     = 0;
    s32 PlaneData   = 0;

    for( s32 i=0; i<m_nBuildings; i++ )
    {
        s32 d, c, b, p;
        
        m_Building[i].pBuilding->GetStats( d, c, b, p );
        
        DisplayList += d;
        Collision   += c;
        BSPData     += b;
        PlaneData   += p;
    }

    x_DebugMsg( "Display List = %d\n", DisplayList  );
    x_DebugMsg( "Collision    = %d\n", Collision    );
    x_DebugMsg( "BSPData      = %d\n", BSPData      );
    x_DebugMsg( "PlaneData    = %d\n", PlaneData    );
    x_DebugMsg( "-------------------------------\n" );
    x_DebugMsg( "Total Size   = %d\n", DisplayList + Collision + BSPData + PlaneData );
}

//=========================================================================

void bld_manager::CleanHash( void )
{
    s32 i;

    for( i=0; i<m_nHashEntries; i++ )
    {
        m_RenderTable[i].pDList[0] = NULL;
        m_RenderTable[i].pDList[1] = NULL;
        m_RenderTable[i].pDList[2] = NULL;
        m_RenderTable[i].pDList[3] = NULL;
        m_RenderTable[i].MinDist    = 100000.0f;
        m_RenderTable[i].MaxDist    = -m_RenderTable[i].MinDist;
        m_RenderTable[i].MinK       = 100000.0f;
        m_RenderTable[i].MaxK       = -m_RenderTable[i].MinK;
    }

    //
    // Initialzile the prep info structure
    //
    for( i=0; i<4; i++ )
    {
        m_PrepInfo[ i ].bViewInZoneZero = TRUE;
        m_PrepInfo[ i ].OutPortal.Clear();
        m_PrepInfo[ i ].ZonesRendered   = 0;
    }
}

//=========================================================================

xbool bld_manager::IsViewInZoneZero( s32 ViewPortID )
{
    ASSERT( ViewPortID >= 0 );
    ASSERT( ViewPortID <  4 );
    return m_PrepInfo[ ViewPortID ].bViewInZoneZero;
}

//=========================================================================

xbool bld_manager::IsZoneZeroVisible( s32 ViewPortID )
{
    ASSERT( ViewPortID >= 0 );
    ASSERT( ViewPortID <  4 );
    return !!(m_PrepInfo[ ViewPortID ].ZonesRendered & 1) || IsViewInZoneZero(0);
}

//=========================================================================

rect bld_manager::GetZoneZeroRect( s32 ViewPortID )
{
    ASSERT( ViewPortID >= 0 );
    ASSERT( ViewPortID <  4 );
    return m_PrepInfo[ ViewPortID ].OutPortal;
}

//=========================================================================

xcolor bld_manager::GetAverageColor( s32 iHash )
{
    return m_Bitmap[ m_RenderTable[ m_HashIDToTable[iHash] ].iBaseMap ].AverageColor;
}

//=========================================================================

const char* bld_manager::GetBuildingName( const building& Building ) const
{
    for( s32 i=0; i<m_nBuildings; i++ )
    {
        const building_node& Node = m_Building[i];
        if( Node.pBuilding == &Building )
            return Node.FileName;
    }

    return NULL;
}

//=========================================================================

xbool bld_manager::IsZoneVisible( s32 BuildingIndex, s32 ZoneID )
{
    ASSERT( (BuildingIndex>=0) && (BuildingIndex<m_nInstances) );
    ASSERT( (ZoneID>=0) && (ZoneID<m_pInstance[BuildingIndex]->m_nZones) );
    return (m_pInstance[BuildingIndex]->m_ZoneVisible & (((u64)1)<<ZoneID)) ? (1):(0);
}

//=========================================================================

void bld_manager::RenderZoneInfo( void )
{
    for( s32 i=0; i<m_nInstances; i++ )
    {
        x_printfxy(0,2+i,"[%2d] ",i);
        for( s32 j=0; j<m_pInstance[i]->m_nZones; j++ )
            x_printfxy(6+j,2+i,"%1c",(m_pInstance[i]->m_ZoneVisible & (((u64)1)<<j)) ? 'X':'.');
    }
}

//=========================================================================

void bld_manager::ClearZoneInfo( void )
{
    s32 i;

    //
    // Initialzile the prep info structure
    //
    for( i=0; i<4; i++ )
    {
        m_PrepInfo[ i ].bViewInZoneZero = TRUE;
        m_PrepInfo[ i ].OutPortal.Clear();
        m_PrepInfo[ i ].ZonesRendered   = 0;
    }

    for( i=0; i<m_nInstances; i++ )
    {
        m_pInstance[i]->m_ZoneVisible = 0;
    }
}

//=========================================================================

void bld_manager::GetZoneSet( zone_set& ZS, const bbox& BBox, xbool JustCenter )
{
    vector3 P[9];
    P[0] = BBox.GetCenter();
    P[1].X = BBox.Min.X;    P[1].Y = BBox.Min.Y;    P[1].Z = BBox.Min.Z;
    P[2].X = BBox.Min.X;    P[2].Y = BBox.Min.Y;    P[2].Z = BBox.Max.Z;
    P[3].X = BBox.Min.X;    P[3].Y = BBox.Max.Y;    P[3].Z = BBox.Min.Z;
    P[4].X = BBox.Min.X;    P[4].Y = BBox.Max.Y;    P[4].Z = BBox.Max.Z;
    P[5].X = BBox.Max.X;    P[5].Y = BBox.Min.Y;    P[5].Z = BBox.Min.Z;
    P[6].X = BBox.Max.X;    P[6].Y = BBox.Min.Y;    P[6].Z = BBox.Max.Z;
    P[7].X = BBox.Max.X;    P[7].Y = BBox.Max.Y;    P[7].Z = BBox.Min.Z;
    P[8].X = BBox.Max.X;    P[8].Y = BBox.Max.Y;    P[8].Z = BBox.Max.Z;

    ZS.NZones = 0;
    ZS.InZoneZero = 0;
    s32 i,j,k;
    s32 NPoints = (JustCenter)?(1):(9);

    // Loop through all buildings
    for( i=0; i<m_nInstances; i++ )
    {
        // Check if bboxes overlap
        bbox BB;
        ((bld_instance*)m_pInstance[i])->GetBBox(BB);
        if( !BBox.Intersect(BB) )
            continue;

        // BBoxes overlap so check for zones
        for( j=0; j<NPoints; j++ )
        {
            s32 Z = ((bld_instance*)m_pInstance[i])->GetZoneFromPoint( P[j] );
            if( Z==0 )
            {
                ZS.InZoneZero = TRUE;
            }
            else
            {
                // Check if already in list
                for( k=0; k<ZS.NZones; k++ )
                {
                    if( (ZS.ZoneID[k] == (byte)Z) &&
                        (ZS.BuildingID[k] == (byte)i)) 
                        break;
                }

                // Add zone if unique
                if( (k==ZS.NZones) && (ZS.NZones<4) )
                {
                    ZS.ZoneID[ ZS.NZones ] = (byte)Z;
                    ZS.BuildingID[ ZS.NZones ] = (byte)i;
                    ZS.NZones++;
                }
            }
        }
    }

    if( ZS.NZones==0 )
        ZS.InZoneZero = TRUE;
}

//=========================================================================

xbool bld_manager::IsZoneSetVisible( zone_set& ZS )
{
    if( ZS.InZoneZero && IsZoneZeroVisible(0) )
        return TRUE;

    // Check individual zones
    for( s32 i=0; i<ZS.NZones; i++ )
    {
        if( m_pInstance[ZS.BuildingID[i]]->m_ZoneVisible & (((u64)1)<<ZS.ZoneID[i]) )
            return TRUE;
    }

    return FALSE;
}

//=========================================================================

#if defined( X_DEBUG ) && defined( jcossigny )

struct dlist_uv
{
    s16 TU,TV;
    u16 LU,LV;
};

xcolor ngon_cols[8] =
{
    xcolor( 128, 128, 128 ),
    xcolor( 128,   0,   0 ),
    xcolor(   0, 128,   0 ),
    xcolor(   0,   0, 128 ),
    xcolor( 128,   0, 128 ),
    xcolor( 128, 128,   0 ),
    xcolor(   0, 128, 128 ),
    xcolor(  64,  64,  64 ),
};

vector3 ngon_pts[64];
s32 ngon_npts;
s32 ngon_edge=0;
f32 ngon_area;
f32 ngon_ang;
f32 ngon_dot;

s32 StripToNGonWithUVs( vector3* pDstVert, vector4*  pSrcVert, 
                        vector2* pDstUV,   dlist_uv* pSrcUV,
                        s32 MaxVerts );

void bld_manager::RenderNGon( s32 iBuilding, s32 iNGon )
{
    building* pBuilding = m_Building[ iBuilding ].pBuilding;

    if( !pBuilding ) return;    

    if( iNGon >= pBuilding->m_CGrid.m_nNGons ) return;
    
    building::building_grid::ngon* pNGon = &pBuilding->m_CGrid.m_pNGon[ iNGon ];

    vector2  uvs[32];
    dlist_uv lst[32];
    
    ngon_npts = StripToNGonWithUVs( ngon_pts, pNGon->pVert, uvs, lst, pNGon->nVerts );
    ngon_area = 0;

    draw_Begin( DRAW_TRIANGLES, zb ? 0 : DRAW_NO_ZBUFFER );

    for( s32 i=1; i<pNGon->nVerts-1; i++ )
    {
        draw_Color( ngon_cols[i & 0x7] );
        
        draw_Vertex( ngon_pts[ 0 ] );
        draw_Vertex( ngon_pts[ i ] );
        draw_Vertex( ngon_pts[i+1] );
        
        vector3 V0 = ngon_pts[ i ] - ngon_pts[0];
        vector3 V1 = ngon_pts[i+1] - ngon_pts[0];
        
        vector3 V  = V0.Cross( V1 );
        ngon_area += V.Length() / 2.0f;
    }

    draw_End();

    {
        draw_Begin( DRAW_LINES, DRAW_NO_ZBUFFER );

        for( s32 i=0; i<pNGon->nVerts; i++ )
        {
            s32 A = i;
            s32 B = ( i + 1) % pNGon->nVerts;
            s32 C = ( i + 2) % pNGon->nVerts;
        
            vector3 V0 = ngon_pts[A] - ngon_pts[B];
            vector3 V1 = ngon_pts[C] - ngon_pts[B];

            V0.Normalize();
            V1.Normalize();

            if( ngon_edge == i )
            {
                ngon_dot = V0.Dot( V1 );
                ngon_ang = RAD_TO_DEG( x_acos( ngon_ang ));
                
                draw_Color( XCOLOR_WHITE );
            }
            else
            {
                draw_Color( XCOLOR_RED );
            }
        
            draw_Vertex( ngon_pts[A] );
            draw_Vertex( ngon_pts[B] );
        }
        
        draw_End();
    }
}

#endif
