
#include "BldInstance.hpp"

///////////////////////////////////////////////////////////////////////////
// ADDITIONAL INCLUDES
///////////////////////////////////////////////////////////////////////////

#include <xCore/Entropy/e_Engine.hpp>
#include "building.hpp"
#include "aux_bitmap.hpp"
#include "..\AADS\globals.hpp"

#define DEBUG_ROTATION      0

///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////

//=========================================================================

bld_instance::bld_instance( void )
{
    x_memset( this, 0, sizeof( *this ) );
}

//=========================================================================

void bld_instance::LoadBuilding( const char* pFileName, xbool LoadAlarmMaps )
{
    ASSERT( pFileName );

    matrix4 L2W;   
    L2W.Identity();
    LoadBuilding( pFileName, L2W, LoadAlarmMaps );
}

//=========================================================================

#if DEBUG_ROTATION
matrix4 mlist[20][10];
#endif

void bld_instance::LoadBuilding( const char* pFileName, const matrix4& L2W, xbool LoadAlarmMaps )
{
    ASSERT( pFileName );

    // Load the building and register the instance with the manager
    g_BldManager.LoadBuilding( pFileName, *this, LoadAlarmMaps );

    // Now set the matrix for the building
    SetLocalMatrix( L2W );

    //
    // Allocate the display list nodes
    //
    m_nDListNodes = m_pBuilding->m_nDLists;
    m_pDListNode  = new bld_manager::dlist_node[ m_nDListNodes ];
    ASSERT( m_pDListNode );
    x_memset( m_pDListNode, 0, sizeof(bld_manager::dlist_node)*m_nDListNodes );

    #if DEBUG_ROTATION
    s32 Num = g_BldManager.GetBuildingNum( *m_pBuilding );
    mlist[ Num ][ m_ID ] = m_L2W;
    #endif
}

//=========================================================================

void bld_instance::DeleteBuilding( void )
{
    //
    // delete display list nodes
    //

    if (m_pDListNode)
    {
        delete [] m_pDListNode;
        m_pDListNode = NULL;
    }
}

//=========================================================================

void bld_instance::SetLocalMatrix( const matrix4& L2W )
{
    //
    // Recompute the matrices
    //
    {
        m_L2W = L2W;
        m_W2L = L2W;
        VERIFY( m_W2L.Invert() );
    }

    //
    // Find the bounding box in world space for this building
    //
    {
        m_BBox.Clear();
        for( s32 i=0; i<m_pBuilding->m_nDLists; i++ )
        {
            building::dlist& DList = m_pBuilding->m_pDList[i];
        
            // Get the min and max points
            if( DList.BBMin.X < m_BBox.Min.X ) m_BBox.Min.X = DList.BBMin.X;
            if( DList.BBMin.Y < m_BBox.Min.Y ) m_BBox.Min.Y = DList.BBMin.Y;
            if( DList.BBMin.Z < m_BBox.Min.Z ) m_BBox.Min.Z = DList.BBMin.Z;

            if( DList.BBMax.X > m_BBox.Max.X ) m_BBox.Max.X = DList.BBMax.X;
            if( DList.BBMax.Y > m_BBox.Max.Y ) m_BBox.Max.Y = DList.BBMax.Y;
            if( DList.BBMax.Z > m_BBox.Max.Z ) m_BBox.Max.Z = DList.BBMax.Z;
        }

        m_BBox.Transform( m_L2W );
    }
}

//=========================================================================

u32 bld_instance::IsBuildingInView( const view& View ) const 
{
    vector3 MINP, MAXP;
    u32     SkipPlaneMask=0;
    s32     i;

    //
    // Check culling planes
    //
    for( i=0; i<6; i++ )
    {
        const plane& Plane = View.GetViewPlanes()[i];

        if( Plane.Normal.X > 0)  { MAXP.X = m_BBox.Max.X; MINP.X = m_BBox.Min.X; }
        else                     { MAXP.X = m_BBox.Min.X; MINP.X = m_BBox.Max.X; }

        if( Plane.Normal.Y > 0)  { MAXP.Y = m_BBox.Max.Y; MINP.Y = m_BBox.Min.Y; }
        else                     { MAXP.Y = m_BBox.Min.Y; MINP.Y = m_BBox.Max.Y; }

        if( Plane.Normal.Z > 0)  { MAXP.Z = m_BBox.Max.Z; MINP.Z = m_BBox.Min.Z; }
        else                     { MAXP.Z = m_BBox.Min.Z; MINP.Z = m_BBox.Max.Z; }


        // If completely outside plane, we are culled
        // don't need to draw this dlist
        f32 MaxDot = Plane.Distance( MAXP );
        if(MaxDot < 0)
            return (~0);

        // If completely inside plane we don't need to check clipping plane
        f32 MinDot = Plane.Distance( MINP );
        if(MinDot>=0)
            SkipPlaneMask |= (1<<i);
    }

    return SkipPlaneMask;
}

//=========================================================================
xbool IsOccludedByTerrain( const bbox& BBox, const vector3& EyePos );
void draw_normals( const building& Building, matrix4& L2W );

#if defined( X_DEBUG ) && defined( jcossigny )
#define MAX_DLISTS      2048
building::dlist* pDLists[ MAX_DLISTS ];
#endif

int z0=1,z1=1;

xbool ClipMat = FALSE;
f32   g_Clip  = 256.0f;

s32 bld_Pitch = 0;
s32 bld_Yaw   = 0;
s32 bld_Roll  = 0;

xbool IsFogged( const bbox& BBox, const view* pView )
{
    f32 R = BBox.GetRadius();
    f32 D = (pView->GetPosition() - BBox.GetCenter()).Length();
    f32 VD = (tgl.Fog.GetVisDistance()+20.0f);
    if( D-R > VD )
    {
        return TRUE;
    }

    return FALSE;
}

void bld_instance::PrepRender( void )
{    
    s32             nDListNodes = 0;
    s32             i;

    #if defined( X_DEBUG ) && defined( jcossigny )  // JP: for debugging strips

    ASSERT( m_pBuilding->m_nDLists < MAX_DLISTS );
    
    for( i=0; i<m_pBuilding->m_nDLists; i++)
    {
        pDLists[i] = &m_pBuilding->m_pDList[i];
    }

    draw_normals( GetBuilding(), m_L2W );
    
    #endif

    #if DEBUG_ROTATION
    {
        radian3 R;
        
        R.Pitch = DEG_TO_RAD( (f32)bld_Pitch );
        R.Yaw   = DEG_TO_RAD( (f32)bld_Yaw   );
        R.Roll  = DEG_TO_RAD( (f32)bld_Roll  );
        
        matrix4 M;
        M.Identity();
        M.Setup( R );
    
        s32 Num = g_BldManager.GetBuildingNum( *m_pBuilding );
    
        m_L2W = mlist[ Num ][ m_ID ] * M;
        
        SetLocalMatrix( m_L2W );
        
        {
            draw_BBox( m_BBox );
            draw_SetL2W( m_L2W );
            draw_Axis( 20.0f );
            draw_ClearL2W();
        }
    }
    #endif

    //
    // Set all the display list pieces into the cache
    //
    for( i=0; i<eng_GetNActiveViews(); i++ )
    {
#ifdef BLD_TIMERS
        bld_manager::st.PREP_Init.Start();
#endif

        prep_info        PrepInfo;
        f32              MaxDist, MinDist;
        building::dlist* pDList          = NULL;
        const view&      View            = *eng_GetActiveView( i );
        u32              SkipPlaneMask;      
        f32              ClipX = 256.0f;
        f32              ClipY = 256.0f;

        //
        // Clear the prep render
        //
        PrepInfo.OutPortal.Clear();
        PrepInfo.bViewInZoneZero = FALSE;
        PrepInfo.ZonesRendered   = 0;

        //
        // If the bbox is not in the view then we are done
        //
        if( 0 )
        {
            draw_BBox( m_BBox, xcolor(255,0,255,255) );
        }

        SkipPlaneMask = IsBuildingInView( View );
        if( SkipPlaneMask == (u32)(~0) ) 
        {
#ifdef BLD_TIMERS
            bld_manager::st.PREP_Init.Stop();
#endif
            continue;
        }

        if( IsOccludedByTerrain( m_BBox, View.GetPosition() ) )
        {
#ifdef BLD_TIMERS
            bld_manager::st.PREP_Init.Stop();
#endif
            continue;
        }

        if( IsFogged( m_BBox, &View ) )
        {
#ifdef BLD_TIMERS
            bld_manager::st.PREP_Init.Stop();
#endif
            continue;
        }

#ifdef BLD_TIMERS
        bld_manager::st.PREP_Init.Stop();
        bld_manager::st.PREP_CopyMatrices.Start();
#endif
        //
        // Lets fill up our matrices
        //
#ifdef TARGET_PS2
        m_RL2C[i] = View.GetW2C() * m_L2W;
        m_RC2S[i] = View.GetC2S();
        m_RL2S[i] = View.GetW2S() * m_L2W;

        //---------------------------------------------------------------------
        
        if( ClipMat )
        {
            matrix4 V2C, C2S;
            s32 X0, Y0, X1, Y1;

            View.GetViewport( X0, Y0, X1, Y1 );

            f32 Ratio = (f32)( Y1 - Y0 ) / (f32)( X1 - X0 );
        
            ClipX = g_Clip;
            ClipY = g_Clip * Ratio;
            
            x_memset( &V2C, 0, sizeof( V2C ));
            x_memset( &C2S, 0, sizeof( C2S ));
    
            //---------------------------------------------------------------------
    
            f32 D = View.GetScreenDist();        
            f32 X = x_atan( ClipX / D );
            f32 Y = x_atan( ClipY / D );
            f32 W = (f32)(1.0f / x_tan( X ));
            f32 H = (f32)(1.0f / x_tan( Y ));
            
            f32 ZNear, ZFar;
    
            View.GetZLimits( ZNear, ZFar );
            
            V2C(0,0) = -W;
            V2C(1,1) =  H;
    	    V2C(2,2) =      ( ZNear + ZFar ) / ( ZFar - ZNear );
    	    V2C(3,2) = ( -2 * ZNear * ZFar ) / ( ZFar - ZNear );
    	    V2C(2,3) = 1.0f;
    
            m_RL2C[i] = ( V2C * View.GetW2V() ) * m_L2W;
    
            //---------------------------------------------------------------------
    
            W = ClipX;
            H = ClipY;
            
            f32 SW     = ( X1 - X0 ) / 2.0f;
            f32 ZScale = (f32)((s32)1<<19);
            
            C2S(0,0) =  W;
            C2S(1,1) = -H;
            C2S(2,2) = -ZScale/2;
            C2S(3,3) = 1.0f;
            C2S(3,0) = W + X0 + (f32)( 2048 - ClipX );
            C2S(3,1) = H + Y0 + (f32)( 2048 - ClipY ) - (( 1.0f - Ratio ) * SW );
            C2S(3,2) =  ZScale/2;
    
            m_RC2S[i] = C2S;
        }

        //---------------------------------------------------------------------
#else
        m_RL2W[i] = m_L2W;
        m_RW2V[i] = View.GetW2V();
        m_RV2C[i] = View.GetV2C();
#endif

#ifdef BLD_TIMERS
        bld_manager::st.PREP_CopyMatrices.Stop();
#endif
        //
        // let the building build the display list
        //
#ifdef BLD_TIMERS
        bld_manager::st.PREP_PortalWalk.Start();
#endif
        pDList = m_pBuilding->PrepRender( m_L2W, m_W2L, View, PrepInfo, SkipPlaneMask, ClipX, ClipY );
#ifdef BLD_TIMERS
        bld_manager::st.PREP_PortalWalk.Stop();
#endif

        //
        // Let compute the min and Min distance for the mips
        //
        {
            vector3 PMin, PMax;
            plane Near;
            Near.Setup( View.GetPosition(), View.GetViewPlanes()[4].Normal );

            if( Near.Normal.X > 0 )  { PMax.X = m_BBox.Max.X;  PMin.X = m_BBox.Min.X; }
            else                     { PMax.X = m_BBox.Min.X;  PMin.X = m_BBox.Max.X; }
                                                                            
            if( Near.Normal.Y > 0 )  { PMax.Y = m_BBox.Max.Y;  PMin.Y = m_BBox.Min.Y; }
            else                     { PMax.Y = m_BBox.Min.Y;  PMin.Y = m_BBox.Max.Y; }
                                                                            
            if( Near.Normal.Z > 0 )  { PMax.Z = m_BBox.Max.Z;  PMin.Z = m_BBox.Min.Z; }
            else                     { PMax.Z = m_BBox.Min.Z;  PMin.Z = m_BBox.Max.Z; }

            MaxDist = Near.Distance( PMax );
            MinDist = Near.Distance( PMin );
            MaxDist = MAX(0,MaxDist);
            MinDist = MAX(0,MinDist);
        }

        //
        // Go throw the list and add all the nodes that need to be render in the hash table
        //
#ifdef BLD_TIMERS
        bld_manager::st.PREP_CopyDLists.Start();
#endif
        for( building::dlist* pDL = pDList; pDL ; pDL = pDL->pNext )
        {
            ASSERT( nDListNodes < m_nDListNodes );

            s32 Alarm = 0;
            
            if( m_AlarmLighting )
            {
                if( pDL->bAlarm )
                {
                    Alarm = 1;
                }
            }

            #ifdef X_DEBUG

            if( !z1 )
            {
                if( pDL->PassMask == XBIN(1011) ) continue;
            }
            
            if( !z0 )
            {
                if( pDL->PassMask == XBIN(1101) ) continue;
            }
            
            #endif

            // Add the node for the display list
            m_pDListNode[ nDListNodes ].bClip       = pDL->bClip;
            m_pDListNode[ nDListNodes ].pDList      = pDL;
            m_pDListNode[ nDListNodes ].pInstance   = this;

            s32 Add;

            if( pDL->bAlarm )
            {
                Add = ( m_HashAdd * 2 ) + Alarm;
            }
            else
            {
                Add = m_HashAdd;
            }

            // Add thre node in the cache
            g_BldManager.AddDisplayListInHash( m_pDListNode[ nDListNodes ], i, MinDist, MaxDist, Add );

            // Get ready for the next node
            nDListNodes++;
        }
        m_ZoneVisible = PrepInfo.ZonesRendered;
        g_BldManager.UpdatePrepInfo( PrepInfo, i );

#ifdef BLD_TIMERS
        bld_manager::st.PREP_CopyDLists.Stop();
#endif
    }

    if( 0 )
    {
        draw_SetL2W( m_L2W );
        for( i=0; i<m_pBuilding->m_nDLists; i++ )
        {
            building::dlist* pDL = &m_pBuilding->m_pDList[ i ];
            draw_BBox( bbox( pDL->BBMin, pDL->BBMax), xcolor(255,0,0,255)  );
        }
    }
}

//=========================================================================

bld_instance::~bld_instance()
{
    DeleteBuilding();
}

//=========================================================================

s32 bld_instance::GetZoneFromPoint( const vector3& Point ) const
{
    vector3 V = m_W2L * Point;

    // quick reject
    //if( m_BBox.Intersect( V ) == FALSE ) return 0;

    // Check the bsp
    return m_pBuilding->FindZoneFromPoint( V );
}

//=========================================================================

xbool bld_instance::CastRay( const vector3& S, const vector3& E, raycol& Info ) const
{
    vector3  Ps, Pe;

    Ps = m_W2L * S;
    Pe = m_W2L * E;

    if( m_pBuilding->CastRay( Ps, Pe, Info ) == FALSE ) 
        return FALSE;

    //
    // Make sure to transform the info back to world space
    //
    Info.Plane.Transform( m_L2W );
    Info.Point  = m_L2W * Info.Point;
    
    return TRUE;
}

//=========================================================================

void bld_instance::GetBBox( bbox& BBox )
{
    BBox = m_BBox;
}

//=========================================================================

const building& bld_instance::GetBuilding( void )
{
    return *m_pBuilding;

}

//=========================================================================
const matrix4& bld_instance::GetL2W( void )
{
    return m_L2W;
}

//=========================================================================
const matrix4& bld_instance::GetW2L( void )
{
    return m_W2L;
}

//=========================================================================
void bld_instance::SetAlarmLighting( xbool State )
{
    m_AlarmLighting = State;
}

//==============================================================================

