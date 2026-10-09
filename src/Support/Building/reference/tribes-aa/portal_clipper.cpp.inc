
#include "portal_clipper.hpp"

#define MIN_PORTAL_SIZE     5.0f        // minimum dimensions of portal in pixels

///////////////////////////////////////////////////////////////////////////
// VARIABLES
///////////////////////////////////////////////////////////////////////////

portal_clipper::info portal_clipper::s_Info;

static vector3       s_PointBuffer[128];

///////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////


//=========================================================================

void portal_clipper::Setup( const view& View, const matrix4& L2W, const matrix4& W2L )
{
    //
    // Set the original view port
    //
    {
        s32 X0, Y0, X1, Y1;

        View.GetViewport( X0, Y0, X1, Y1 );
        Rect.Set( vector2((f32)X0, (f32)Y0), vector2((f32)X1, (f32)Y1) );
    }

    //
    // Compute the projection constants
    //
    {
        s_Info.L2V = View.GetW2V() * L2W;
        View.GetProjection( s_Info.XP0, s_Info.XP1, s_Info.YP0, s_Info.YP1 );
    }

    //
    // Get the viewport
    //
    View.GetViewport( s_Info.Viewport );

    //
    // Take the near plane and the eye into the local space of the building
    //

    // Make sure that the near plane is exactly where the eye is
    s_Info.Near   = View.GetViewPlanes()[4];
    s_Info.Near.ComputeD( View.GetPosition() ); 

    // Take the eye into local space
    s_Info.Near.Transform( W2L );
    s_Info.EyePos = W2L * View.GetPosition();
}

//=========================================================================

const rect& portal_clipper::GetRect( void )
{
    return Rect;
}

//=========================================================================

xbool portal_clipper::ClipWithPortal( portal_clipper& PortalClipper, s32 nPoints, const vector3* pPortal, plane& PortalPlane ) 
{
    const vector3*      pClipPoint = pPortal;
    s32                 nVerts     = nPoints;
    xbool               Done       = FALSE;

    if( x_abs( PortalPlane.Distance( s_Info.EyePos ) ) <= 2 )
    {
        bbox BBox;

        //
        // TODO: Must add whehther the eye is in the bounding box of the portal.
        //  
        BBox.Clear();
        BBox.AddVerts( pPortal, nPoints );
        BBox.Min -= vector3(2,2,2);
        BBox.Max += vector3(2,2,2);
        if( BBox.Intersect( s_Info.EyePos ) ) 
        {
            PortalClipper.Rect = s_Info.Viewport;
            Done               = TRUE;
        }
    }
    
    if( Done == FALSE )
    {
        //
        // If the portal is back face to us then we don't deal with it
        //    
        if( PortalPlane.Distance( s_Info.EyePos ) < -0.01f ) return 0;

        //
        // Set up some variables
        //
        PortalClipper.Rect.Clear();

        //
        // Create portal from view point
        //
        vector2 S;
        for( s32 i=0; i<nVerts; i++ )
        {
            vector3 P( s_Info.L2V * pClipPoint[i] );
            f32     ProjZ = P.Z;

            // Handle strange/dangerous Z's.
            if( ProjZ < 0.001f )
            {
                //
                // Clip the portal with the near plane
                //
                if( pClipPoint != s_PointBuffer )
                {
                    s_Info.Near.ClipNGon( s_PointBuffer, nVerts, pPortal, nPoints );
                    if( nVerts == 0 ) return 0;

                    // Clear the rectangle that we have so far
                    PortalClipper.Rect.Clear();

                    // This time do the cliped vertors
                    pClipPoint = s_PointBuffer;

                    // Force to redo all things again
                    i          = -1;
                    continue;
                }

                //
                // Handle strange cases
                //
                if( ProjZ > -0.001f ) ProjZ = 0.001f;
                if( ProjZ <  0.000f ) 
                    ProjZ = -ProjZ;
            }

            // Project to screen.
            S.X   = s_Info.XP0 + s_Info.XP1 * (P.X/ProjZ);
            S.Y   = s_Info.YP0 + s_Info.YP1 * (P.Y/ProjZ);
            ProjZ = P.Z;

            //
            // Add the point into ourRect
            //
            PortalClipper.Rect.AddPoint( S );
        }

        //
        // portal distance culling
        //
        
        vector2 Len = PortalClipper.Rect.GetSize();
        
        if ( Len.X < MIN_PORTAL_SIZE && Len.Y < MIN_PORTAL_SIZE )
        {
            return FALSE;
        }

        //
        // Clip our portal with the new one
        //
        if( Rect.Intersect( PortalClipper.Rect, PortalClipper.Rect ) == FALSE ) return FALSE;
    }

    //
    // Debug ouput
    //
#ifdef tarce
    if( 1 )
    {
        draw_Line( pPortal[0], pPortal[0] + PortalPlane.Normal * 10 );
        draw_Rect( PortalClipper.Rect );
    }
#endif

    return TRUE;
}

/*
//=========================================================================

void portal_clipper::BuildFromFrustrum( const vector3* pNear, const vector3& Center )
{
    m_nEdges = 4;

    m_Edge[0] = pNear[3];
    m_Edge[1] = pNear[2];
    m_Edge[2] = pNear[1];
    m_Edge[3] = pNear[0];

    m_Center = Center;

    m_Normal = (m_Edge[1] - m_Edge[0]).Cross( m_Edge[2] - m_Edge[0] );
    ASSERT( m_Normal.Length() >= 0.001f );

    m_Normal.Normalize();
    m_Near.Normal = m_Normal;
    m_Near.D      = -m_Normal.Dot( m_Edge[0] );       
    
    ComputePlanesFormEdges();
}

//=========================================================================

void portal_clipper::BuildFromPlanes( const plane* pPlane, s32 nPlanes, s32 iNearPlane, const vector3& Center )
{
    m_Center = Center;
    m_nEdges = m_nPlanes = nPlanes;

    for( int i=0; i<nPlanes; i++ )
    {
        m_Plane[i] = pPlane[i];
        m_Plane[i].Negate();
    }
}

//=========================================================================

 void portal_clipper::ComputePlanesFormEdges( void )
{
    vector3 Normal;
    int i;

    for( i=0; i<m_nEdges; i++ )
    {
        int     j;

        j = i+1;
        if( j == m_nEdges ) j = 0;
            
        Normal = (m_Edge[i] - m_Center).Cross( m_Edge[j] - m_Center );

        m_Plane[i].Normal = Normal;
        m_Plane[i].D      = -m_Plane[i].Normal.Dot( m_Center );       
    }
    
    Normal = (m_Edge[2] - m_Edge[1]).Cross( m_Edge[0] - m_Edge[1] );

    m_Plane[i].Normal = Normal;
    m_Plane[i].D      = -Normal.Dot( m_Edge[1] );  

    m_nPlanes = m_nEdges+1;
}


//=========================================================================
int portal_clipper::ClipEdgesWithPlane( 
    vector3*        pDestPoint, 
    int             nPoints, 
    const vector3*  pPoint, 
    const plane&    Plane ) const
{
    s32 Side     [ MAX_EDGES ];
    f32 Distance [ MAX_EDGES ];
    s32 i,j;
    f32 dot;
    s32 nDestPoints=0;

    //
    // Compute all the points distances
    //
    {
        xbool   DoFrontTest;
        xbool   DoBackTest;

        DoBackTest      = FALSE;
        DoFrontTest     = FALSE;

        for( i=0; i<nPoints; i++ ) 
        {
            dot     = Plane.Normal.Dot( pPoint[i] );
            dot    += Plane.D;

            Distance[i] = dot;

            if( dot > 0.01 )
            {
                DoFrontTest = TRUE;
                Side[i] = 1;
            }
            else 
            if( dot < -0.01 ) 
            {
                DoBackTest = TRUE;
                Side[i] = -1;
            }
            else
            {
                Side[i] = 0;
            }
        }

        // If all the points are in one side then determine which side is in
        if ( DoFrontTest ^ DoBackTest )
        {
            // All points are out of this facet
            if( DoBackTest == FALSE ) return 0;

            // All the points are in
            for( i=0; i<nPoints; i++)
            {
                pDestPoint[i] = pPoint[i];
            }

            return nPoints;
        }

        // So we can only get here when DoFrontTest == DoBackTest
        // So when we have both saying false we must be coplanar 
        // to the plane. And when that happens we don't longer care about this portal
        if ( DoFrontTest == FALSE )
        {
            return 0;
        }
    }

    //---------------------------------------------------------------------
    // Start general plane intersection
    //---------------------------------------------------------------------
    for( i=0; i<nPoints; i++ )
    {
        const vector3& p = pPoint[i];

        if( i == nPoints-1) j = 0;
        else                j = i + 1;

        // Point is on the plane
        if( Side[i] == 0 ) 
        {
            pDestPoint[ nDestPoints++ ] = p;
            continue;
        }
    
        // Point is in the front side
        if( Side[i] == -1 ) 
        {
            pDestPoint[ nDestPoints++ ] = p;
        }

        // If we are going to be in the same side of the plane next time
        // then we don't need to clip.
        if( (Side[j] == Side[i]) || (Side[j] == 0) ) continue;

        //-----------------------------------------------------------------
        // generate a split point
        //-----------------------------------------------------------------
        dot = Distance[i] / ( Distance[i] - Distance[j] );

        pDestPoint[ nDestPoints++ ] = p + dot*( pPoint[j] - p );
    }

    return nDestPoints;
}

//=========================================================================

s32 portal_clipper::ClipWithPortal( const portal_clipper& PortalClipper, s32 nPoints, const vector3* pPortal, const vector3& Normal )
{
    vector3 Curent[ MAX_EDGES * 2 ];
    s32     nEdges;
    s32     Taggle = 0;
    s32     i;

    //
    // If the portal is back face to us then we don't deal with it
    //    
    if( Normal.Dot( pPortal[0] - PortalClipper.m_Center ) < 0 ) return 0;

    //
    // Do the first plane
    //
    nEdges = ClipEdgesWithPlane( &Curent[ Taggle ], nPoints, pPortal, PortalClipper.m_Plane[0] );
    if( nEdges == 0 ) return 0;
    Taggle = MAX_EDGES - Taggle;

    //
    // Do the rest of the planes
    //
    for( i=1; i<PortalClipper.m_nPlanes; i++ )
    {
        // Clip portal with facet
        nEdges = ClipEdgesWithPlane( &Curent[ Taggle ], nEdges, &Curent[ MAX_EDGES - Taggle ], PortalClipper.m_Plane[i] );

        // If we don't have any points then we must be done.
        if( nEdges == 0 )
            return 0;

        // Taggle betwwen the two buffers
        Taggle = MAX_EDGES - Taggle;
    }

    //
    // Build our new portal
    //
    Taggle   = MAX_EDGES - Taggle;

    m_Center = PortalClipper.m_Center;
    m_Normal = PortalClipper.m_Normal;
    m_Near   = PortalClipper.m_Near;
    m_nEdges = nEdges;

    for( i=0; i<nEdges; i++)
    {
        m_Edge[i] = Curent[ i + Taggle ];
    }

    ComputePlanesFormEdges();

    return m_nEdges;
}
*/