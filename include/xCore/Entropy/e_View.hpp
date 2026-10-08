#ifndef HOBBIT_E_VIEW_HPP
#define HOBBIT_E_VIEW_HPP

#include <xCore/x_files/x_math.hpp>

class view {
public:
    enum system {
        WORLD,
        VIEW
    };
    view();
    view(const view& other);
    ~view();
    void SetViewport(int X0, int Y0, int X1, int Y1);
    void SetXFOV(float angle);
    void SetYFOV(float angle);
    void SetZLimits(float nearZ, float farZ);
    void SetPosition(const vector3& position);
    void SetRotation(const radian3& Rotation);
    void SetV2W(const matrix4& V2W);
    void SetupOrthoScales(float width, float height);
    void GetViewport(int& X0, int& Y0, int& X1, int& Y1) const;
    void GetViewport(rect& result) const;
    void GetPixel(float ParamX,float ParamY,int& X,int& Y)const;
    void GetZLimits(float& nearZ, float& farZ) const;
    void GetPitchYaw(float& pitch, float& yaw) const;
    int PointInView(const vector3& p, system s) const;
    int SphereInView(const vector3& p, float radius, system s) const;
    int BBoxInView(const bbox& b, system s) const;
    float GetYFOV() const;
    vector3 GetPosition() const;
    vector3 ConvertV2W(const vector3& V)const;
    vector3 ConvertW2V(const vector3& V)const;
    const matrix4& GetW2V() const;
    const matrix4& GetV2W() const;
    const matrix4& GetV2C() const;
    const matrix4& GetC2S() const;
    const matrix4& GetV2S() const;
    const matrix4& GetW2C() const;
    const matrix4& GetW2S() const;
    vector3 GetViewX() const;
    vector3 GetViewY() const;
    vector3 GetViewZ() const;
    void GetViewPlanes(plane& Top,plane& Bottom,plane& Left,plane& Right,system System)const;
    void GetViewPlanes(plane& Top,plane& Bottom,plane& Left,plane& Right,plane& Near,plane& Far,system System)const;
    void GetViewPlanes(float X0,float Y0,float X1,float Y1,plane& Top,plane& Bottom,plane& Left,plane& Right,plane& Near,plane& Far,system System) const;
    const plane* GetViewPlanes(system which) const;
    const int* GetViewPlaneMinBBoxIndices(system which) const;
    const int* GetViewPlaneMaxBBoxIndices(system which) const;
    void GetMinMaxZ(const bbox& box, float& minZ, float& maxZ) const;
    void GetProjection(float& x0, float& x1, float& y0, float& y1) const;
    void SetSubShot(int x, int y, int size);
    float GetScreenDist() const;

    void Translate(const vector3& Translation,system System=WORLD);
    void RotateX(float Angle,system System=WORLD);
    void RotateY(float Angle,system System=WORLD);
    void RotateZ(float Angle,system System=WORLD);
    void LookAtPoint(const vector3& FromPoint,const vector3& ToPoint,system System=WORLD);
    void LookAtPoint(const vector3& Point,system System=WORLD);
    void OrbitPoint(const vector3& Point,float Distance,float Pitch,float Yaw);
    int SphereInCone(const vector3& Center,float Radius)const;
    int SphereInConeAngle(const vector3& Center,float Radius,float tanAngle)const;
    vector3 PointToScreen(const vector3& Point,system System=WORLD)const;
    vector3 RayFromScreen(float ScreenX,float ScreenY,system System=WORLD)const;
    float CalcScreenSize(const vector3& Position,float WorldRadius,system System=WORLD)const;

protected:
    vector3 m_WorldPos;
    matrix4 m_WorldOrient;
    int m_ViewportX0, m_ViewportY0, m_ViewportX1, m_ViewportY1;
    // PC SetViewport stores these derived dimensions at 0x5c and 0x60.
    int m_ViewportWidth, m_ViewportHeight;
    float m_XFOV, m_ZNear, m_ZFar;
    // Descriptive PC-inferred member names; absent from both sibling layouts.
    int m_Orthographic;
    float m_OrthoWidth, m_OrthoHeight;
    int m_SubShotX, m_SubShotY, m_ShotSize;
    mutable unsigned m_Dirty;
    mutable matrix4 m_W2V, m_V2W, m_V2C, m_C2S, m_V2S, m_W2C, m_W2S;
    mutable float m_ScreenDist, m_YFOV;
    mutable plane m_WorldSpacePlane[6], m_ViewSpacePlane[6];
    mutable int m_WorldPlaneMinIndex[18], m_WorldPlaneMaxIndex[18], m_ViewPlaneMinIndex[18],
        m_ViewPlaneMaxIndex[18];
    mutable plane m_ZPlane;
    mutable int m_ZPlaneMinI[3], m_ZPlaneMaxI[3];
    mutable vector3 m_ConeAxis;
    // PC UpdatePlanes stores the derived culling radius at 0x46c.
    mutable float m_ConeSlope, m_ConeRadius;
    mutable float m_ProjectX[2], m_ProjectY[2];
    void UpdateW2V() const;
    void UpdateV2W() const;
    void UpdateV2C() const;
    void UpdateC2S() const;
    void UpdateV2S() const;
    void UpdateW2C() const;
    void UpdateW2S() const;
    void UpdateYFOV() const;
    void UpdatePlanes() const;
    void UpdateScreenDist() const;
    void UpdateProjection() const;
};
#endif
