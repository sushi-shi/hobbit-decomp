// Reconstructed from the pinned PC view region; names corroborated
// by the Xbox map and sibling e_View sources. Hobbit-specific fields are
// inferred from PC accesses. See docs/view-reconstruction.md for evidence.

#include <rva.h>

#include <xCore/Entropy/e_View.hpp>
#include <xCore/x_files/x_plus.hpp>

// Naturally emitted array construction iterator; linker-folded provider.
RVA_COMPGEN(0x00001080, 0x30, ??_H@YGXPAXIHP6EX0@Z@Z)

RVA(0x00271410, 0x9e)
view::view() {
    m_WorldPos.Set(0, 0, 0);
    m_WorldOrient.Identity();
    m_XFOV = 1.0471975803375244f;
    m_ZNear = 0.1f;
    m_ZFar = 1000.0f;
    m_Orthographic = 0;
    m_OrthoWidth = 640.0f;
    m_OrthoHeight = 480.0f;
    m_SubShotX = 0;
    m_SubShotY = 0;
    m_ShotSize = 1;
    SetViewport(50, 50, 589, 429);
    m_Dirty = ~0u;
}

RVA(0x002714b0, 0xbe)
view::view(const view& a) {
    m_WorldPos = a.m_WorldPos;
    m_WorldOrient = a.m_WorldOrient;
    m_XFOV = a.m_XFOV;
    m_ZNear = a.m_ZNear;
    m_ZFar = a.m_ZFar;
    m_Orthographic = a.m_Orthographic;
    m_OrthoWidth = a.m_OrthoWidth;
    m_OrthoHeight = a.m_OrthoHeight;
    m_SubShotX = a.m_SubShotX;
    m_SubShotY = a.m_SubShotY;
    m_ShotSize = a.m_ShotSize;
    SetViewport(a.m_ViewportX0, a.m_ViewportY0, a.m_ViewportX1, a.m_ViewportY1);
    m_Dirty = ~0u;
}

RVA(0x00271570, 0x1)
view::~view() {}

RVA(0x00271580, 0x39)
void view::SetViewport(int X0, int Y0, int X1, int Y1) {
    m_ViewportX0 = X0;
    m_ViewportY0 = Y0;
    m_ViewportX1 = X1;
    m_ViewportY1 = Y1;
    m_ViewportWidth = X1 - X0 + 1;
    m_ViewportHeight = Y1 - Y0 + 1;
    m_Dirty = ~0u;
}

RVA(0x002715c0, 0x14)
void view::SetXFOV(float a) {
    m_XFOV = a;
    m_Dirty = ~0u;
}

RVA(0x002715e0, 0x6c)
void view::SetYFOV(float a) {
    float halfH = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
    float halfW = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
    m_ScreenDist = halfH / x_tan(a * 0.5f);
    m_XFOV = x_atan(halfW / m_ScreenDist) * 2.0f;
    m_YFOV = a;
    m_Dirty = ~0u & ~((1 << 11) | (1 << 7));
}

RVA(0x00271650, 0x1b)
void view::SetZLimits(float n, float f) {
    m_ZNear = n;
    m_ZFar = f;
    m_Dirty = ~0u;
}

RVA(0x00271670, 0x21)
void view::SetPosition(const vector3& a) {
    m_WorldPos = a;
    m_Dirty = ~0u;
}

RVA(0x002716a0,0x104)
void view::SetRotation(const radian3& Rotation){m_WorldOrient.Setup(Rotation);m_Dirty=~0u;}
RVA(0x002717b0,0x84)
void view::SetV2W(const matrix4& V2W){m_WorldOrient=V2W;m_WorldOrient.ZeroTranslation();m_WorldPos=V2W.GetTranslation();m_Dirty=~0u;}

RVA(0x00271930, 0x2c)
void view::SetupOrthoScales(float w, float h) {
    m_OrthoWidth = w;
    if (h != 0.0f) {
        m_OrthoHeight = h;
    }
    m_Dirty = ~0u;
}

RVA(0x002720e0, 0x27)
void view::GetViewport(int& x0, int& y0, int& x1, int& y1) const {
    x0 = m_ViewportX0;
    y0 = m_ViewportY0;
    x1 = m_ViewportX1;
    y1 = m_ViewportY1;
}

RVA(0x00272110, 0x1e)
void view::GetViewport(rect& r) const {
    r.Min.X = m_ViewportX0;
    r.Min.Y = m_ViewportY0;
    r.Max.X = m_ViewportX1;
    r.Max.Y = m_ViewportY1;
}

RVA(0x00272130,0x84)
void view::GetPixel(float ParamX,float ParamY,int& X,int& Y)const {
 X=(int)(m_ViewportX0+ParamX*(m_ViewportX1-m_ViewportX0));
 Y=(int)(m_ViewportY0+ParamY*(m_ViewportY1-m_ViewportY0));
 if(m_ShotSize>1){X*=m_ShotSize;Y*=m_ShotSize;X-=m_ViewportX0;Y-=m_ViewportY1;}
}

RVA(0x002721c0, 0x15)
void view::GetZLimits(float& n, float& f) const {
    n = m_ZNear;
    f = m_ZFar;
}

RVA(0x002721e0, 0x59)
void view::GetPitchYaw(float& pitch, float& yaw) const {
    vector3 los(m_WorldOrient(2, 0), m_WorldOrient(2, 1), m_WorldOrient(2, 2));
    pitch = -x_atan2(los.Y, static_cast<float>(sqrt(los.X * los.X + los.Z * los.Z)));
    yaw = x_atan2(los.X, los.Z);
}

RVA(0x00272250, 0x10)
float view::GetYFOV() const {
    UpdateYFOV();
    return m_YFOV;
}

RVA(0x00272260, 0x17)
vector3 view::GetPosition() const {
    return m_WorldPos;
}

RVA(0x00272280, 0x10)
const matrix4& view::GetW2V() const {
    UpdateW2V();
    return m_W2V;
}

RVA(0x00272290, 0x10)
const matrix4& view::GetV2W() const {
    UpdateV2W();
    return m_V2W;
}

RVA(0x002722a0, 0x10)
const matrix4& view::GetV2C() const {
    UpdateV2C();
    return m_V2C;
}

RVA(0x002722b0, 0x10)
const matrix4& view::GetC2S() const {
    UpdateC2S();
    return m_C2S;
}

RVA(0x002722c0, 0x10)
const matrix4& view::GetV2S() const {
    UpdateV2S();
    return m_V2S;
}

RVA(0x002722d0, 0x10)
const matrix4& view::GetW2C() const {
    UpdateW2C();
    return m_W2C;
}

RVA(0x002722e0, 0x10)
const matrix4& view::GetW2S() const {
    UpdateW2S();
    return m_W2S;
}

RVA(0x002722f0, 0x18)
vector3 view::GetViewX() const {
    return vector3(m_WorldOrient(0, 0), m_WorldOrient(0, 1), m_WorldOrient(0, 2));
}

RVA(0x00272310, 0x18)
vector3 view::GetViewY() const {
    return vector3(m_WorldOrient(1, 0), m_WorldOrient(1, 1), m_WorldOrient(1, 2));
}

RVA(0x00272330, 0x18)
vector3 view::GetViewZ() const {
    return vector3(m_WorldOrient(2, 0), m_WorldOrient(2, 1), m_WorldOrient(2, 2));
}

vector3 view::ConvertW2V(const vector3& Point)const {UpdateW2V();return(m_W2V*Point);}
vector3 view::ConvertV2W(const vector3& Point)const {UpdateV2W();return(m_V2W*Point);}
RVA(0x00272830, 0x2b)
const plane* view::GetViewPlanes(system s) const {
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    return s == WORLD ? m_WorldSpacePlane : m_ViewSpacePlane;
}

RVA(0x00272860, 0x2b)
const int* view::GetViewPlaneMinBBoxIndices(system s) const {
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    return s == WORLD ? m_WorldPlaneMinIndex : m_ViewPlaneMinIndex;
}

RVA(0x00272890, 0x2b)
const int* view::GetViewPlaneMaxBBoxIndices(system s) const {
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    return s == WORLD ? m_WorldPlaneMaxIndex : m_ViewPlaneMaxIndex;
}

RVA(0x002728c0, 0x95)
void view::GetMinMaxZ(const bbox& b, float& n, float& f) const {
    // Proven contiguous bbox scalar view: Min XYZ followed by Max XYZ.
    const float* p = reinterpret_cast<const float*>(&b);
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    n = m_ZPlane.Normal.X * p[m_ZPlaneMinI[0]] + m_ZPlane.Normal.Y * p[m_ZPlaneMinI[1]]
        + m_ZPlane.Normal.Z * p[m_ZPlaneMinI[2]] + m_ZPlane.D;
    f = m_ZPlane.Normal.X * p[m_ZPlaneMaxI[0]] + m_ZPlane.Normal.Y * p[m_ZPlaneMaxI[1]]
        + m_ZPlane.Normal.Z * p[m_ZPlaneMaxI[2]] + m_ZPlane.D;
}

RVA(0x00272960,0x8b)
void view::GetViewPlanes(plane& Top,plane& Bottom,plane& Left,plane& Right,system System)const {
 UpdatePlanes(); plane* pPlane=(System==WORLD)?m_WorldSpacePlane:m_ViewSpacePlane;
 Left=pPlane[0];Right=pPlane[1];Bottom=pPlane[2];Top=pPlane[3];
}
RVA(0x002729f0,0xc1)
void view::GetViewPlanes(plane& Top,plane& Bottom,plane& Left,plane& Right,plane& Near,plane& Far,system System)const {
 UpdatePlanes(); plane* pPlane=(System==WORLD)?m_WorldSpacePlane:m_ViewSpacePlane;
 Left=pPlane[0];Right=pPlane[1];Bottom=pPlane[2];Top=pPlane[3];Near=pPlane[4];Far=pPlane[5];
}

// Descriptive name pending separate PC/Xbox identity evidence.
extern float GetViewFarDistance();
RVA(0x00272ac0, 0xa5d)
void view::GetViewPlanes( float X0, float Y0, float X1, float Y1,
                          plane& Top,
                          plane& Bottom,
                          plane& Left,
                          plane& Right,
                          plane& Near,
                          plane& Far,
                          system System ) const
{
    if(!m_Orthographic) {
    // Imagine sitting at the origin and looking down Z+.  Build the points on 
    // the frustum and transform into correct system.  Then build the planes 
    // the points.

    // Compute distance to screen in camera space.
    UpdateScreenDist();

    if(m_ShotSize!=1) {
      const float W=(m_ViewportX1-m_ViewportX0+1)*0.5f;
      const float H=(m_ViewportY1-m_ViewportY0+1)*0.5f;
      X0=-W;X1=W;Y0=-H;Y1=H;
    }
    // Flip signs to go from screen coordinates to view space.
    X0 = -X0;
    X1 = -X1;
    Y0 = -Y0;
    Y1 = -Y1;

    // Build camera space coordinates.
    vector3 PTL( X0, Y0,  m_ScreenDist );
    vector3 PTR( X1, Y0,  m_ScreenDist );
    vector3 PBL( X0, Y1,  m_ScreenDist );
    vector3 PBR( X1, Y1,  m_ScreenDist );
    vector3 PEYE(  0,  0, 0 );
    vector3 PNR(  0,  0, m_ZNear );
    vector3 PFR(  0,  0, m_ZFar );
    PFR.Z=GetViewFarDistance()*1.25f;
    vector3 PUP(  0,  1,  0 );
    vector3 PLFT(  1,  0,  0 );

    // Transform into correct system.
    if( System == WORLD )
    {
        PTL  = ConvertV2W(PTL );
        PTR  = ConvertV2W(PTR );
        PBL  = ConvertV2W(PBL );
        PBR  = ConvertV2W(PBR );
        PEYE = ConvertV2W(PEYE);
        PNR  = ConvertV2W(PNR );
        PFR  = ConvertV2W(PFR );
        PUP  = ConvertV2W(PUP );
        PLFT = ConvertV2W(PLFT);
    }

    // Construct side planes.
    Top.Setup   ( PEYE, PTL, PTR );
    Bottom.Setup( PEYE, PBR, PBL );
    Left.Setup  ( PEYE, PBL, PTL );
    Right.Setup ( PEYE, PTR, PBR );

    // Construct LOS normal for near and far.
    Near.Setup( PEYE, PLFT, PUP );
    Near.D = -Near.Dot(PNR);
    Far = Near;
    Far.Negate();
    Far.D  = -Far.Dot(PFR);
    } else {
        vector3 Up(GetViewY());
        vector3 RightAxis(GetViewX());
        vector3 Eye(m_WorldPos);
        float XS=m_OrthoWidth/m_ViewportWidth;
        float YS=m_OrthoHeight/m_ViewportHeight;
        vector3 P0(Eye-(X0*RightAxis)*XS-(Y0*Up)*YS);
        vector3 P1(Eye-(X1*RightAxis)*XS-(Y1*Up)*YS);
        vector3 PN(Eye+GetViewZ()*m_ZNear);
        vector3 PF(Eye+GetViewZ()*m_ZFar);
        Near.Setup(PN,GetViewZ());
        Far.Setup(PF,-GetViewZ());
        Top.Setup(P0,-Up);
        Bottom.Setup(P1,Up);
        Left.Setup(P0,-RightAxis);
        Right.Setup(P1,RightAxis);
        if(System==VIEW) {
            UpdateW2V();
            Near=m_W2V*Near;
            Far=m_W2V*Far;
            Top=m_W2V*Top;
            Bottom=m_W2V*Bottom;
            Left=m_W2V*Left;
            Right=m_W2V*Right;
        }
    }
}


RVA(0x00273520, 0x6c)
int view::PointInView(const vector3& p, system s) const {
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    plane* planes = s == WORLD ? m_WorldSpacePlane : m_ViewSpacePlane;
    for (int i = 0; i < 6; i++) {
        // The explicit float expression preserves the observed VC6 operand order.
        float dist = p.Dot(planes[i].Normal) + static_cast<float>(planes[i].D);
        if (dist < 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00273590, 0x8b)
int view::SphereInView(const vector3& p, float radius, system s) const {
    int result = 1;
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    plane* planes = s == WORLD ? m_WorldSpacePlane : m_ViewSpacePlane;
    for (int i = 0; i < 6; i++) {
        // The explicit float expression preserves the observed VC6 operand order.
        float dist = p.Dot(planes[i].Normal) + static_cast<float>(planes[i].D);
        if (dist < -radius) {
            return 0;
        }
        if (dist < radius) {
            result = 2;
        }
    }
    return result;
}

RVA(0x00273620, 0xd0)
int view::BBoxInView(const bbox& b, system s) const {
    int result = 1;
    if (m_Dirty & (1 << 8)) {
        UpdatePlanes();
    }
    plane* planes = s == WORLD ? m_WorldSpacePlane : m_ViewSpacePlane;
    int* minI = s == WORLD ? m_WorldPlaneMinIndex : m_ViewPlaneMinIndex;
    int* maxI = s == WORLD ? m_WorldPlaneMaxIndex : m_ViewPlaneMaxIndex;
    // Proven contiguous bbox scalar view: Min XYZ followed by Max XYZ.
    const float* p = reinterpret_cast<const float*>(&b);
    for (int i = 0; i < 6; i++) {
        float mn = planes->Normal.X * p[minI[0]] + planes->Normal.Y * p[minI[1]]
                   + planes->Normal.Z * p[minI[2]] + planes->D;
        float mx = planes->Normal.X * p[maxI[0]] + planes->Normal.Y * p[maxI[1]]
                   + planes->Normal.Z * p[maxI[2]] + planes->D;
        if (mx < 0) {
            return 0;
        }
        if (mn < 0) {
            result = 2;
        }
        minI += 3;
        maxI += 3;
        planes++;
    }
    return result;
}

RVA(0x002737e0, 0x3c)
void view::GetProjection(float& x0, float& x1, float& y0, float& y1) const {
    UpdateProjection();
    x0 = m_ProjectX[0];
    x1 = m_ProjectX[1];
    y0 = m_ProjectY[0];
    y1 = m_ProjectY[1];
}

RVA(0x00273be0, 0x41)
void view::SetSubShot(int x, int y, int s) {
    if (m_SubShotX != x || m_SubShotY != y || m_ShotSize != s) {
        m_SubShotX = x;
        m_SubShotY = y;
        m_ShotSize = s;
        m_Dirty = ~0u;
    }
}

RVA(0x00273c30, 0x119)
void view::UpdateW2V() const {
    if (m_Dirty & 1) {
        m_W2V = m_WorldOrient;
        m_W2V.Transpose();
        m_W2V.PreTranslate(-m_WorldPos);
        m_Dirty &= ~1u;
    }
}

RVA(0x00273d50, 0x6d)
void view::UpdateV2W() const {
    if (m_Dirty & 2) {
        m_V2W = m_WorldOrient;
        m_V2W.SetTranslation(m_WorldPos);
        m_Dirty &= ~2u;
    }
}

RVA(0x00273dc0, 0xdc)
void view::UpdateV2C() const {
    if (m_Dirty & 4) {
        m_Dirty &= ~4u;
        x_memset(&m_V2C, 0, sizeof(matrix4));
        UpdateYFOV();
        if (!m_Orthographic) {
            float w = 1.0f / x_tan(m_XFOV * 0.5f);
            float h = 1.0f / x_tan(m_YFOV * 0.5f);
            float q = m_ZFar / (m_ZFar - m_ZNear);
            m_V2C(0, 0) = -w;
            m_V2C(1, 1) = h;
            m_V2C(2, 2) = q;
            m_V2C(3, 2) = -q * m_ZNear;
            m_V2C(2, 3) = 1.0f;
        } else {
            float q = 1.0f / (m_ZFar - m_ZNear);
            m_V2C(0, 0) = -2.0f / m_OrthoWidth;
            m_V2C(1, 1) = 2.0f / m_OrthoHeight;
            m_V2C(2, 2) = q;
            m_V2C(3, 2) = -q * m_ZNear;
            m_V2C(3, 3) = 1.0f;
        }
    }
}

RVA(0x00273ea0, 0xcb)
void view::UpdateC2S() const {
    if (m_Dirty & 8) {
        m_Dirty &= ~8u;
        x_memset(&m_C2S, 0, sizeof(matrix4));
        if (!m_Orthographic) {
            float w = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
            float h = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
            m_C2S(0, 0) = w;
            m_C2S(1, 1) = -h;
            m_C2S(2, 2) = 1;
            m_C2S(3, 3) = 1;
            m_C2S(3, 0) = w + m_ViewportX0;
            m_C2S(3, 1) = h + m_ViewportY0;
        } else {
            float w = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
            float h = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
            m_C2S(0, 0) = w;
            m_C2S(1, 1) = -h;
            m_C2S(2, 2) = 1;
            m_C2S(3, 3) = 1;
            m_C2S(3, 0) = w + m_ViewportX0;
            m_C2S(3, 1) = h + m_ViewportY0;
        }
    }
}

RVA(0x00273f70, 0x86)
void view::UpdateV2S() const {
    if (m_Dirty & (1 << 4)) {
        m_Dirty &= ~(1 << 4);
        UpdateV2C();
        UpdateC2S();
        m_V2S = m_C2S * m_V2C;
    }
}

RVA(0x00274000, 0x7d)
void view::UpdateW2C() const {
    if (m_Dirty & (1 << 5)) {
        m_Dirty &= ~(1 << 5);
        UpdateW2V();
        UpdateV2C();
        m_W2C = m_V2C * m_W2V;
    }
}

RVA(0x00274080, 0x86)
void view::UpdateW2S() const {
    if (m_Dirty & (1 << 6)) {
        m_Dirty &= ~(1 << 6);
        UpdateW2C();
        UpdateC2S();
        m_W2S = m_C2S * m_W2C;
    }
}

RVA(0x00274110, 0x66)
void view::UpdateYFOV() const {
    if (m_Dirty & (1 << 7)) {
        float distance;
        float halfH = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
        float halfW = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
        distance = halfW / x_tan(m_XFOV * 0.5f);
        m_YFOV = x_atan(halfH / distance) * 2.0f;
        m_Dirty &= ~(1 << 7);
        m_Dirty |= 1 << 2;
    }
}

RVA(0x00274180, 0x4d1)
void view::UpdatePlanes( void ) const
{
    if( m_Dirty & (1 << 8) )
    {
        m_Dirty &= ~(1 << 8);

        // Get full frustrum
        float W = (m_ViewportX1 - m_ViewportX0 + 1)*0.5f;
        float H = (m_ViewportY1 - m_ViewportY0 + 1)*0.5f;

        GetViewPlanes( -W, -H, W, H,
                       m_WorldSpacePlane[3], 
                       m_WorldSpacePlane[2], 
                       m_WorldSpacePlane[0], 
                       m_WorldSpacePlane[1], 
                       m_WorldSpacePlane[4], 
                       m_WorldSpacePlane[5], 
                       WORLD );

        GetViewPlanes( -W, -H, W, H,
                       m_ViewSpacePlane[3], 
                       m_ViewSpacePlane[2], 
                       m_ViewSpacePlane[0], 
                       m_ViewSpacePlane[1], 
                       m_ViewSpacePlane[4], 
                       m_ViewSpacePlane[5], 
                       VIEW );

        for( int i=0; i<6; i++ )
        {
            m_WorldSpacePlane[i].GetBBoxIndices( &m_WorldPlaneMinIndex[i*3], &m_WorldPlaneMaxIndex[i*3] );
            m_ViewSpacePlane[i].GetBBoxIndices ( &m_ViewPlaneMinIndex[i*3],  &m_ViewPlaneMaxIndex[i*3] );
        }

        // Build ZPlane and bbox indices
        m_ZPlane = m_WorldSpacePlane[4];
        m_ZPlane.ComputeD( m_WorldPos );
        m_ZPlane.GetBBoxIndices( m_ZPlaneMinI, m_ZPlaneMaxI );

        // Build cone values
        m_ConeAxis = GetViewZ();

        vector3 Ray;
        Ray.X = -(m_ViewportX0 - ((m_ViewportX0 + m_ViewportX1) * 0.5f));
        Ray.Y = -(m_ViewportY0 - ((m_ViewportY0 + m_ViewportY1) * 0.5f));
        Ray.Z = m_ScreenDist;
        Ray.X *= m_ZFar / m_ScreenDist;
        Ray.Y *= m_ZFar / m_ScreenDist;
        m_ConeSlope = x_sqrt(Ray.X*Ray.X + Ray.Y*Ray.Y) / m_ZFar;
        if(m_Orthographic)
            m_ConeRadius=(x_sqrt(m_OrthoWidth*m_OrthoWidth*0.25f+m_OrthoHeight*m_OrthoHeight*0.25f));
        else
            m_ConeRadius=m_ConeSlope*m_ZFar;
    }
}


RVA(0x00274670, 0x21)
float view::GetScreenDist() const {
    if (m_Dirty & (1 << 11)) {
        UpdateScreenDist();
    }
    return m_ScreenDist * m_ShotSize;
}

RVA(0x002746a0, 0x49)
void view::UpdateScreenDist() const {
    if (m_Dirty & (1 << 11)) {
        m_Dirty &= ~(1 << 11);
        float halfW = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
        m_ScreenDist = halfW / x_tan(m_XFOV * 0.5f);
    }
}

RVA(0x002746f0, 0xd2)
void view::UpdateProjection() const {
    if (m_Dirty & (1 << 10)) {
        m_Dirty &= ~(1 << 10);
        if (!m_Orthographic) {
            m_ProjectX[0] = (m_ViewportX0 + m_ViewportX1) * 0.5f;
            m_ProjectY[0] = (m_ViewportY0 + m_ViewportY1) * 0.5f;
            UpdateScreenDist();
            m_ProjectX[1] = -m_ScreenDist;
            m_ProjectY[1] = -m_ScreenDist;
        } else {
            m_ProjectX[0] = (m_ViewportX0 + m_ViewportX1) * 0.5f;
            m_ProjectY[0] = (m_ViewportY0 + m_ViewportY1) * 0.5f;
            m_ProjectX[1] = (m_ViewportX0 - m_ViewportX1) / m_OrthoWidth;
            m_ProjectY[1] = m_ProjectX[1];
        }
    }
}

// Provisional sibling-correlated engine source; PC body spans await mapping.
void view::Translate( const vector3& Translation, system System )
{   
    switch( System )
    {
        case WORLD: m_WorldPos += Translation;                      break;
        case VIEW:  m_WorldPos += (m_WorldOrient * Translation);    break;
        default:    break;
    }

    m_Dirty = ~0u;
}

void view::RotateX( float Angle, system System )
{
    switch( System )
    {
        case WORLD: m_WorldOrient.RotateX( Angle );         break;                        
        case VIEW:  m_WorldOrient.PreRotateX( Angle );      break;
        default:    break;
    }

    m_Dirty = ~0u;
}

void view::RotateY( float Angle, system System )
{
    switch( System )
    {
        case WORLD: m_WorldOrient.RotateY( Angle );         break;                        
        case VIEW:  m_WorldOrient.PreRotateY( Angle );      break;
        default:    break;
    }

    m_Dirty = ~0u;
}

void view::RotateZ( float Angle, system System )
{
    switch( System )
    {
        case WORLD: m_WorldOrient.RotateZ( Angle );         break;                        
        case VIEW:  m_WorldOrient.PreRotateZ( Angle );      break;                        
        default:    break;
    }

    m_Dirty = ~0u;
}

void view::LookAtPoint( const vector3& FromPoint, 
                        const vector3& ToPoint,
                        system System )
{
    vector3 WorldFrom;

    

    switch( System )
    {
        case WORLD: WorldFrom = FromPoint; break;
        case VIEW:  WorldFrom = ConvertV2W( FromPoint ); break;
        default:    break;
    }

    

    SetPosition( WorldFrom );
    LookAtPoint( ToPoint, System );

    m_Dirty = ~0u;
}

void view::LookAtPoint( const vector3& Point, system System )
{
    vector3 Target;

    

    switch( System )
    {
        case WORLD: Target = Point; break;
        case VIEW:  Target = ConvertV2W( Point ); break;
        default:    break;
    }

    

    Target -= m_WorldPos;

    m_WorldOrient.Identity();
    m_WorldOrient.RotateX( Target.GetPitch() );
    m_WorldOrient.RotateY( Target.GetYaw()   );

    m_Dirty = ~0u;
}

void view::OrbitPoint( const vector3& Point, 
                             float      Distance,
                             float   Pitch,
                             float   Yaw )
{
    m_WorldPos.Set( 0, 0, Distance );
    m_WorldPos.RotateX( Pitch );
    m_WorldPos.RotateY( Yaw   );
    m_WorldPos += Point;
    LookAtPoint( Point );
}

vector3 view::PointToScreen( const vector3& Point,
                                   system   System ) const
{
    
    vector3 P = Point;
    if( System==WORLD )
        P = ConvertW2V(P);

    
    UpdateProjection();

    
    float ProjZ = P.Z;
    if( ProjZ < 0.001f )
    {
        if( ProjZ > -0.001f ) ProjZ = 0.001f;
        if( ProjZ <  0.000f ) ProjZ = -ProjZ;
    }

    
    vector3 S;
    S.X = m_ProjectX[0] + m_ProjectX[1] * (P.X/ProjZ);
    S.Y = m_ProjectY[0] + m_ProjectY[1] * (P.Y/ProjZ);
    S.Z = P.Z;

    return( S );
}

vector3 view::RayFromScreen( float    ScreenX,
                             float    ScreenY,
                             system System ) const
{
    if( m_Dirty & (1u << 11) )
        UpdateScreenDist();

    
    vector3 Ray;
    Ray.X = -(ScreenX - (m_ViewportX0 + m_ViewportX1) * 0.5f);
    Ray.Y = -(ScreenY - (m_ViewportY0 + m_ViewportY1) * 0.5f);
    Ray.Z = m_ScreenDist;

    
    if( System == WORLD )
    {
        Ray  = ConvertV2W( Ray );
        Ray -= m_WorldPos;
    }

    Ray.Normalize();
    return( Ray );
}


int view::SphereInCone    ( const vector3& Center,
                                    float      Radius ) const
{
    float Radius2 = Radius*Radius;

    
    if( m_Dirty & (1u << 8) )
        UpdatePlanes();

    
    vector3 Delta = Center - m_WorldPos;
    float     DeltaLen2 = Delta.X*Delta.X + Delta.Y*Delta.Y + Delta.Z*Delta.Z;
    float     ZDist = m_ConeAxis.Dot(Delta);

    
    if( DeltaLen2 < Radius2 )
        return 1;

    
    if( (ZDist<m_ZNear) || (ZDist>m_ZFar) )
        return 0;

    
    float ConeRadius = m_ConeSlope * ZDist;
    float PerpDist2 = DeltaLen2 - (ZDist*ZDist);

    
    if( PerpDist2 > (ConeRadius+Radius)*(ConeRadius+Radius) )
        return 0;

    return 1;
}


int view::SphereInConeAngle   ( const vector3& Center,
                                        float      Radius,
                                        float      tanAngle ) const
{
    float Radius2 = Radius*Radius;

    
    if( m_Dirty & (1u << 8) )
        UpdatePlanes();

    
    vector3 Delta = Center - m_WorldPos;
    float     DeltaLen2 = Delta.X*Delta.X + Delta.Y*Delta.Y + Delta.Z*Delta.Z;
    float     ZDist = m_ConeAxis.Dot(Delta);

    
    if( DeltaLen2 < Radius2 )
        return 1;

    
    if( (ZDist<m_ZNear) || (ZDist>m_ZFar) )
        return 0;

    
    float ConeRadius = tanAngle * ZDist;
    float PerpDist2 = DeltaLen2 - (ZDist*ZDist);

    
    if( PerpDist2 > (ConeRadius+Radius)*(ConeRadius+Radius) )
        return 0;

    return 1;
}


float view::CalcScreenSize  ( const vector3& Position,
                                  float      WorldRadius,
                                  system   System  ) const
{
    float ZDist;

    if( m_Dirty & (1u << 11) )
        UpdateScreenDist();

    
    if( System == WORLD )
    {
        ZDist = m_WorldOrient(2,0) * (Position.X - m_WorldPos.X) + 
                m_WorldOrient(2,1) * (Position.Y - m_WorldPos.Y) + 
                m_WorldOrient(2,2) * (Position.Z - m_WorldPos.Z);
    }
    else
    {
        ZDist = Position.Z;
    }

    
    if( ZDist < -WorldRadius )
        return 0;

    if( ZDist < WorldRadius )
        ZDist = WorldRadius;

    
    

    
    return ((float)m_ShotSize * WorldRadius * 2 * m_ScreenDist)/ZDist;
}
