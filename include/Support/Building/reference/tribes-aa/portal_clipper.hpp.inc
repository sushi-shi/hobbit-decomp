#ifndef PORTAL_HPP
#define PORTAL_HPP

///////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////

#include <xCore/x_files/x_math.hpp>
#include <xCore/Entropy/Entropy.hpp>


class portal_clipper
{
public:

    void            Setup           ( const view& View, const matrix4& L2W, const matrix4& W2L );
    const  rect&    GetRect         ( void );
    xbool           ClipWithPortal  ( portal_clipper& PortalClipper, 
                                      s32 nPoints, 
                                      const vector3* pPortal, 
                                      plane& PortalPlane );

protected:

    struct info
    {
        vector3     EyePos;
        plane       Near;
        matrix4     L2V;
        f32         XP0;
        f32         XP1;
        f32         YP0;
        f32         YP1;

        rect        Viewport;
    };

protected:

    rect            Rect;
    static info     s_Info;

};


/*
///////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////

class portal_clipperlipper
{
///////////////////////////////////////////////////////////////////////////
public:

    enum
    {
        MAX_PLANES = 64,
        MAX_EDGES  = 64,
    };

///////////////////////////////////////////////////////////////////////////
public:

    void Setup                  ( f32 MinX, f32 MinY, f32 MaxX, f32 MaxY );

    // In TL TR BR BL Order
    void BuildFromFrustrum      ( const vector3* pNear, const vector3& Center );
    void BuildFromPlanes        ( const plane* pPlane, int nPlanes, int iNearPlane, const vector3& Center );
    s32  ClipWithPortal         ( const portal_clipperlipper& PortalClipper, s32 nPoints, 
                                  const vector3* pPortal, const vector3& Normal );

///////////////////////////////////////////////////////////////////////////
public:

    s32     m_nEdges;
    vector3 m_Edge[ MAX_EDGES ];
    vector3 m_Center;
    vector3 m_Normal;

///////////////////////////////////////////////////////////////////////////
protected:

    void ComputePlanesFormEdges ( void );
    s32  ClipEdgesWithPlane     ( vector3* pDestPoint, int nPoints, const vector3* pPoint, 
                                  const plane& Plane ) const;

///////////////////////////////////////////////////////////////////////////
public:

    s32     m_nPlanes;
    plane   m_Plane[ MAX_PLANES ];
    plane   m_Near;
};
*/

///////////////////////////////////////////////////////////////////////////
// END
///////////////////////////////////////////////////////////////////////////
#endif
