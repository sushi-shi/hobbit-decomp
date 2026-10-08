#ifndef HOBBIT_X_MATH_HPP
#define HOBBIT_X_MATH_HPP

#include <rva.h>
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math_defs.hpp>
#include <xCore/x_files/x_debug.hpp>

#include <math.h>

// Genuine scalar PC math adapters; provisional additions carry no PC spans.
inline float x_asin(float a){return static_cast<float>(asin(static_cast<double>(a)));}
inline float x_acos(float a){return static_cast<float>(acos(static_cast<double>(a)));}
inline float x_log(float a){return static_cast<float>(log(static_cast<double>(a)));}
inline float x_log10(float a){return static_cast<float>(log10(static_cast<double>(a)));}
inline float x_exp(float a){return static_cast<float>(exp(static_cast<double>(a)));}
inline s8 x_abs(s8 a){return a<0?(s8)-a:a;}
inline s16 x_abs(s16 a){return a<0?(s16)-a:a;}
inline s32 x_abs(s32 a){return a<0?-a:a;}
inline s64 x_abs(s64 a){return a<0?-a:a;}
inline f32 x_abs(f32 a){return a<0?-a:a;}
inline f64 x_abs(f64 a){return a<0?-a:a;}

inline float x_pow(float base, float exponent)
{
    return static_cast<float>(pow(base, exponent));
}

inline float x_sqrt(float value) { return static_cast<float>(sqrt(static_cast<double>(value))); }

inline float x_tan(float a) {
    return (static_cast<float>(tan(static_cast<double>(a))));
}
inline float x_atan(float a) {
    return (static_cast<float>(atan(static_cast<double>(a))));
}

// Hobbit PC value returns at RVAs 0x247bf0/0x247c30 write these fields at
// offsets 0, 4, and (for vector3) 8. This is the Tribes-era three-float
// vector3, before Area 51's later platform-dependent union representation.
struct vector2
{
    float Dot(const vector2& V)const{return X*V.X+Y*V.Y;}
    vector2& operator+=(const vector2& V){X+=V.X;Y+=V.Y;return *this;}
    vector2& operator-=(const vector2& V){X-=V.X;Y-=V.Y;return *this;}
    vector2& operator*=(float S){X*=S;Y*=S;return *this;}
    vector2& operator/=(float S){S=1.0f/S;X*=S;Y*=S;return *this;}
    int operator==(const vector2& V)const{return X==V.X&&Y==V.Y;}
    int operator!=(const vector2& V)const{return X!=V.X||Y!=V.Y;}
    vector2 operator-()const{return vector2(-X,-Y);}
    vector2(){}
    vector2(const vector2& V):X(V.X),Y(V.Y){}
    void Set(float x,float y){X=x;Y=y;}
    void operator()(float x,float y){Set(x,y);}
    float operator[](int i)const{return (&X)[i];}
    float& operator[](int i){return (&X)[i];}
    void Zero(){X=Y=0.0f;}
    float GetX()const{return X;} float GetY()const{return Y;}
    vector2(float x, float y) : X(x), Y(y) {}
    float X, Y;
};

struct vector3
{
    RVA(0x0001def0, 0x3)
    vector3() {}
    vector3(float Pitch,float Yaw) { Set(Pitch,Yaw); }
    vector3(const vector3& value) : X(value.X), Y(value.Y), Z(value.Z) {}
    RVA(0x00001f00, 0x19)
    vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
    const vector3& operator=(const vector3& value);
    int Normalize();
    void Zero(){X=Y=Z=0.0f;}
    vector3& operator*=(float Scalar){X*=Scalar;Y*=Scalar;Z*=Scalar;return *this;}
    vector3& operator/=(float Scalar){float S=1.0f/Scalar;X*=S;Y*=S;Z*=S;return *this;}
    vector3 operator-() const;
    int IsValid()const;
    void Negate();
    int InRange(float Min,float Max)const;
    void Min(float Value); void Max(float Value);
    void Min(const vector3& V); void Max(const vector3& V);
    int SafeNormalize();
    int SafeNormalizeAndScale(float Scalar);
    vector3 GetClosestVToLSeg(const vector3& Start,const vector3& End)const;
    vector3 GetClosestPToLSeg(const vector3& Start,const vector3& End)const;
    float GetSqrtDistToLineSeg(const vector3& Start,const vector3& End)const;
    int operator==(const vector3& V)const{return X==V.X&&Y==V.Y&&Z==V.Z;}
    int operator!=(const vector3& V)const{return X!=V.X||Y!=V.Y||Z!=V.Z;}
    float LengthSquared()const;
    vector3 Cross(const vector3& V)const{return vector3(Y*V.Z-Z*V.Y,Z*V.X-X*V.Z,X*V.Y-Y*V.X);}
    float Dot(const vector3& value)const;
    void Scale(float scale);
    float Length() const { return x_sqrt(X * X + Y * Y + Z * Z); }
    float& GetX(){return X;} float& GetY(){return Y;} float& GetZ(){return Z;}
    void operator()(float x,float y,float z){Set(x,y,z);}
    int NormalizeAndScale(float Scalar);
    float GetX() const { return X; }
    float GetY() const { return Y; }
    float GetZ() const { return Z; }
    const vector3& operator+=(const vector3& V){X+=V.X;Y+=V.Y;Z+=V.Z;return *this;}
    const vector3& operator-=(const vector3& V){X-=V.X;Y-=V.Y;Z-=V.Z;return *this;}
    float& operator[](int i){return (&X)[i];}
    float operator[](int i)const{return (&X)[i];}
    void RotateX(float Angle);
    void RotateY(float Angle);
    float GetPitch() const;
    float GetYaw() const;
    void GetPitchYaw(float& pitch, float& yaw) const;
    void Set(float pitch, float yaw);
    RVA(0x00004870, 0x17)
    void Set(float x, float y, float z) { X = x; Y = y; Z = z; }
    float X, Y, Z;
};



// Packed three-float interchange type used by sibling FX source.
struct vector3p {
 float X,Y,Z;
 vector3p(){}
 vector3p(float x,float y,float z):X(x),Y(y),Z(z){}
 vector3p(const vector3& V):X(V.X),Y(V.Y),Z(V.Z){}
 void Set(float x,float y,float z){X=x;Y=y;Z=z;}
 const vector3p& operator=(const vector3& V){X=V.X;Y=V.Y;Z=V.Z;return *this;}
 operator const vector3()const{return vector3(X,Y,Z);}
};

struct vector4 {
 float X,Y,Z,W;
 const vector3& GetXYZ()const{return *((const vector3*)this);}
 vector3& GetXYZ(){return *((vector3*)this);}
 int GetIW()const{return ((const int*)this)[3];}
 int& GetIW(){return ((int*)this)[3];}
 vector4& operator+=(const vector4& V){X+=V.X;Y+=V.Y;Z+=V.Z;W+=V.W;return *this;}
 vector4& operator-=(const vector4& V){X-=V.X;Y-=V.Y;Z-=V.Z;W-=V.W;return *this;}
 vector4& operator/=(float S){S=1.0f/S;X*=S;Y*=S;Z*=S;W*=S;return *this;}
 int operator==(const vector4& V)const{return X==V.X&&Y==V.Y&&Z==V.Z&&W==V.W;}
 int operator!=(const vector4& V)const{return X!=V.X||Y!=V.Y||Z!=V.Z||W!=V.W;}
 vector4 operator-()const{return vector4(-X,-Y,-Z,-W);}
 vector4(const vector3& V):X(V.X),Y(V.Y),Z(V.Z),W(0.0f){}
 const vector4& operator=(const vector3& V){X=V.X;Y=V.Y;Z=V.Z;W=0.0f;return *this;}
 float& GetX(){return X;}float& GetY(){return Y;}float& GetZ(){return Z;}float& GetW(){return W;}
 vector4(){}
 vector4(float x,float y,float z,float w):X(x),Y(y),Z(z),W(w){}
 vector4(const vector4& V):X(V.X),Y(V.Y),Z(V.Z),W(V.W){}
 void Set(float x,float y,float z,float w){X=x;Y=y;Z=z;W=w;}
 void operator()(float x,float y,float z,float w){Set(x,y,z,w);}
 float operator[](int i)const{return (&X)[i];}
 float& operator[](int i){return (&X)[i];}
 float Dot(const vector4& V)const{return X*V.X+Y*V.Y+Z*V.Z+W*V.W;}
 vector4& operator*=(float S){X*=S;Y*=S;Z*=S;W*=S;return *this;}
 float GetX()const{return X;}float GetY()const{return Y;}float GetZ()const{return Z;}float GetW()const{return W;}
 void Zero(){X=Y=Z=W=0.0f;}
 const vector4& operator=(const vector4& V){X=V.X;Y=V.Y;Z=V.Z;W=V.W;return *this;}
};
struct irect {
 int l,t,r,b;
 irect(){} irect(int L,int T,int R,int B):l(L),t(T),r(R),b(B){}
 void Set(int L,int T,int R,int B){l=L;t=T;r=R;b=B;}
 void Clear(){l=t=S32_MAX;r=b=-S32_MAX;}
 void SetWidth(int W);void SetHeight(int H);void SetSize(int W,int H);
 int GetWidth()const{return r-l;}int GetHeight()const{return b-t;}
 vector2 GetSize()const{return vector2(float(r-l),float(b-t));}
 vector2 GetCenter()const{return vector2(float(r+l)/2.0f,float(b+t)/2.0f);}
 int IsEmpty()const{return l>=r||t>=b;}
 int PointInRect(int X,int Y)const{return X>=l&&X<r&&Y>=t&&Y<b;}
 void Translate(int X,int Y){l+=X;r+=X;t+=Y;b+=Y;}
 void Inflate(int X,int Y){l-=X;r+=X;t-=Y;b+=Y;}
 void Deflate(int X,int Y){l+=X;r-=X;t+=Y;b-=Y;}
};

// PC bitstream Read/WriteRadian3 use three floats at offsets 0, 4 and 8.
struct radian3
{
    int IsValid()const;
    radian3& operator+=(const radian3& R){Pitch+=R.Pitch;Yaw+=R.Yaw;Roll+=R.Roll;return *this;}
    radian3& operator-=(const radian3& R){Pitch-=R.Pitch;Yaw-=R.Yaw;Roll-=R.Roll;return *this;}
    radian3& operator*=(float S){Pitch*=S;Yaw*=S;Roll*=S;return *this;}
    radian3(){}
    radian3(float p,float y,float r):Pitch(p),Yaw(y),Roll(r){}
    void Set(float p,float y,float r){Pitch=p;Yaw=y;Roll=r;}
    void Zero(){Pitch=Yaw=Roll=0.0f;}
    void operator()(float p,float y,float r){Set(p,y,r);}
    float Pitch, Yaw, Roll;
};

// Four consecutive float components are read and returned by all four
// quaternion interpolation functions in the PC x_math region.
struct quaternion
{
    float X, Y, Z, W;
    quaternion() {}
    quaternion(const radian3& R);
    quaternion(const vector3& Axis,float Angle);
    void Setup(const radian3& R);
    void Setup(const vector3& Axis,float Angle);
    radian3 GetRotation()const;
    void RotateX(float R);void RotateY(float R);void RotateZ(float R);
    void Identity(){X=Y=Z=0.0f;W=1.0f;}
    void Invert(){W=-W;}
    quaternion& operator*=(const quaternion& R);
    vector3 operator*(const vector3& V) const;
    vector3 Rotate(const vector3& V) const;
    void Rotate(vector3* pDest,const vector3* pSource,s32 NVerts) const;
    void Normalize()
    {
        float scale = 1.0f / static_cast<float>(sqrt(X * X + Y * Y + Z * Z + W * W));
        X *= scale;
        Y *= scale;
        Z *= scale;
        W *= scale;
    }
    quaternion(const quaternion& value) : X(value.X), Y(value.Y), Z(value.Z), W(value.W) {}
    const quaternion& operator=(const quaternion& value) {
        X=value.X; Y=value.Y; Z=value.Z; W=value.W; return *this;
    }
    quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
};

// PC matrix arithmetic addresses sixteen consecutive floats. There is no
// vtable or other storage in the 64-byte value returned at RVA 0x24e040.
class matrix4 {
public:
    matrix4();
    matrix4(const radian3& R);
    void Rotate(const radian3& R);
    void Rotate(const quaternion& Q);
    void Scale(float S);
    void Scale(const vector3& S);
    void DecomposeSRT(vector3& S,quaternion& R,vector3& T);
    void DecomposeSRT(vector3& S,radian3& R,vector3& T);
    matrix4(const matrix4& other);

    matrix4(const quaternion& value);
    float operator()(int column, int row) const;
    float& operator()(int column, int row);
    void Setup(const radian3& Rotation);
    void Setup(const quaternion& Rotation);
    void Setup(const vector3& Scale,const radian3& Rotation,const vector3& Translation);
    void Setup(const vector3& Scale,const quaternion& Rotation,const vector3& Translation);
    void SetRotation(const quaternion& Rotation);
    void Translate(const vector3& Translation);
    void SetRotation(const radian3& Rotation);
    void RotateX(float Angle);
    void RotateY(float Angle);
    void RotateZ(float Angle);
    void PreRotateX(float Angle);
    void PreRotateY(float Angle);
    void PreRotateZ(float Angle);
    matrix4& operator*=(const matrix4& M);
    vector3 GetTranslation()const;
    void ZeroTranslation();
    void ClearTranslation(){m_Cell[3][0]=m_Cell[3][1]=m_Cell[3][2]=0.0f;}
    void Zero();
    void Orthogonalize();
    void ClearRotation();
    matrix4& operator+=(const matrix4& M);
    matrix4& operator-=(const matrix4& M);
    void Identity();
    void Transpose();
    radian3 GetRotation()const;
    quaternion GetQuaternion() const;
    vector3 operator*(const vector3& V)const;
    vector3 Transform(const vector3& V)const{return (*this)*V;}
    void Transform(vector3* Dest,const vector3* Source,int Count)const;

    vector3 GetScale() const;

    void SetTranslation(const vector3& a);
    void PreScale(const vector3& scale);

    void PreTranslate(const vector3& a);
    void ClearScale();

    int IsValid()const;
    int InvertRT();
    float Difference(const matrix4& M)const;
    void GetColumns(vector3& V1,vector3& V2,vector3& V3)const;
    void SetColumns(const vector3& V1,const vector3& V2,const vector3& V3);
    void SetRows(const vector3& V1,const vector3& V2,const vector3& V3);
    void SetScale(float S);
    void SetScale(const vector3& S);
    vector3 RotateVector(const vector3& V)const;
    vector3 InvRotateVector(const vector3& V)const;
    void Setup(const vector3& Axis,float Angle){Setup(quaternion(Axis,Angle));}
    void Setup(const vector3& V1,const vector3& V2,float Angle);
    int Invert();
    int InvertSRT();
    const matrix4& operator=(const matrix4& other);
    friend matrix4 operator*(const matrix4& left, const matrix4& right);

private:
    float m_Cell[4][4];
};

inline matrix4::matrix4() {}

inline matrix4::matrix4(const matrix4& other) {
    m_Cell[0][0] = other.m_Cell[0][0];
    m_Cell[0][1] = other.m_Cell[0][1];
    m_Cell[0][2] = other.m_Cell[0][2];
    m_Cell[0][3] = other.m_Cell[0][3];
    m_Cell[1][0] = other.m_Cell[1][0];
    m_Cell[1][1] = other.m_Cell[1][1];
    m_Cell[1][2] = other.m_Cell[1][2];
    m_Cell[1][3] = other.m_Cell[1][3];
    m_Cell[2][0] = other.m_Cell[2][0];
    m_Cell[2][1] = other.m_Cell[2][1];
    m_Cell[2][2] = other.m_Cell[2][2];
    m_Cell[2][3] = other.m_Cell[2][3];
    m_Cell[3][0] = other.m_Cell[3][0];
    m_Cell[3][1] = other.m_Cell[3][1];
    m_Cell[3][2] = other.m_Cell[3][2];
    m_Cell[3][3] = other.m_Cell[3][3];
}

inline matrix4::matrix4(const quaternion& value) {
    float x2 = 2.0f * value.X, y2 = 2.0f * value.Y, z2 = 2.0f * value.Z;
    float xw = x2 * value.W, yw = y2 * value.W, zw = z2 * value.W;
    float xx = x2 * value.X, yx = y2 * value.X, zx = z2 * value.X;
    float yy = y2 * value.Y, zy = z2 * value.Y, zz = z2 * value.Z;
    m_Cell[0][0] = 1.0f - (yy + zz);
    m_Cell[0][1] = yx + zw;
    m_Cell[0][2] = zx - yw;
    m_Cell[1][0] = yx - zw;
    m_Cell[1][1] = 1.0f - (xx + zz);
    m_Cell[1][2] = zy + xw;
    m_Cell[2][0] = zx + yw;
    m_Cell[2][1] = zy - xw;
    m_Cell[2][2] = 1.0f - (xx + yy);
    m_Cell[0][3] = 0.0f;
    m_Cell[1][3] = 0.0f;
    m_Cell[2][3] = 0.0f;
    m_Cell[3][3] = 1.0f;
    m_Cell[3][2] = 0.0f;
    m_Cell[3][1] = 0.0f;
    m_Cell[3][0] = 0.0f;
}

inline float matrix4::operator()(int column, int row) const {
    return m_Cell[column][row];
}

inline float& matrix4::operator()(int column, int row) {
    return m_Cell[column][row];
}

inline void matrix4::Identity() {
    m_Cell[1][0] = m_Cell[2][0] = m_Cell[3][0] = 0.0f;
    m_Cell[0][1] = m_Cell[2][1] = m_Cell[3][1] = 0.0f;
    m_Cell[0][2] = m_Cell[1][2] = m_Cell[3][2] = 0.0f;
    m_Cell[0][3] = m_Cell[1][3] = m_Cell[2][3] = 0.0f;
    m_Cell[0][0] = 1.0f;
    m_Cell[1][1] = 1.0f;
    m_Cell[2][2] = 1.0f;
    m_Cell[3][3] = 1.0f;
}

inline void matrix4::Transpose() {
    float t;
    t = m_Cell[1][0];
    m_Cell[1][0] = m_Cell[0][1];
    m_Cell[0][1] = t;
    t = m_Cell[2][0];
    m_Cell[2][0] = m_Cell[0][2];
    m_Cell[0][2] = t;
    t = m_Cell[3][0];
    m_Cell[3][0] = m_Cell[0][3];
    m_Cell[0][3] = t;
    t = m_Cell[2][1];
    m_Cell[2][1] = m_Cell[1][2];
    m_Cell[1][2] = t;
    t = m_Cell[3][1];
    m_Cell[3][1] = m_Cell[1][3];
    m_Cell[1][3] = t;
    t = m_Cell[3][2];
    m_Cell[3][2] = m_Cell[2][3];
    m_Cell[2][3] = t;
}

RVA(0x000068c0, 0x73)
inline vector3 matrix4::GetScale() const {
    vector3 scale;
    vector3 axis;
    // Each PC column norm loads Z, Y, X before the squared sum.
    axis.Z = m_Cell[0][2];
    axis.Y = m_Cell[0][1];
    axis.X = m_Cell[0][0];
    scale.X = axis.Length();
    axis.Z = m_Cell[1][2];
    axis.Y = m_Cell[1][1];
    axis.X = m_Cell[1][0];
    scale.Y = axis.Length();
    axis.Z = m_Cell[2][2];
    axis.Y = m_Cell[2][1];
    axis.X = m_Cell[2][0];
    scale.Z = axis.Length();
    return scale;
}

inline void matrix4::ClearScale() {
    vector3 scale = GetScale();
    PreScale(vector3(1 / scale.GetX(), 1 / scale.GetY(), 1 / scale.GetZ()));
}

inline void matrix4::SetTranslation(const vector3& a) {
    m_Cell[3][0] = a.X;
    m_Cell[3][1] = a.Y;
    m_Cell[3][2] = a.Z;
}

RVA(0x00006940, 0x6d)
inline void matrix4::PreScale(const vector3& scale) {
    m_Cell[0][0] *= scale.X;
    m_Cell[0][1] *= scale.X;
    m_Cell[0][2] *= scale.X;
    m_Cell[0][3] *= scale.X;
    m_Cell[1][0] *= scale.Y;
    m_Cell[1][1] *= scale.Y;
    m_Cell[1][2] *= scale.Y;
    m_Cell[1][3] *= scale.Y;
    m_Cell[2][0] *= scale.Z;
    m_Cell[2][1] *= scale.Z;
    m_Cell[2][2] *= scale.Z;
    m_Cell[2][3] *= scale.Z;
}

inline void matrix4::PreTranslate(const vector3& a) {
    m_Cell[3][0] += (m_Cell[0][0] * a.X) + (m_Cell[1][0] * a.Y) + (m_Cell[2][0] * a.Z);
    m_Cell[3][1] += (m_Cell[0][1] * a.X) + (m_Cell[1][1] * a.Y) + (m_Cell[2][1] * a.Z);
    m_Cell[3][2] += (m_Cell[0][2] * a.X) + (m_Cell[1][2] * a.Y) + (m_Cell[2][2] * a.Z);
    m_Cell[3][3] += (m_Cell[0][3] * a.X) + (m_Cell[1][3] * a.Y) + (m_Cell[2][3] * a.Z);
}

inline vector3 matrix4::operator*(const vector3& V)const {
 return( vector3((m_Cell[0][0]*V.X)+(m_Cell[1][0]*V.Y)+(m_Cell[2][0]*V.Z)+m_Cell[3][0],
                 (m_Cell[0][1]*V.X)+(m_Cell[1][1]*V.Y)+(m_Cell[2][1]*V.Z)+m_Cell[3][1],
                 (m_Cell[0][2]*V.X)+(m_Cell[1][2]*V.Y)+(m_Cell[2][2]*V.Z)+m_Cell[3][2]));
}

inline const matrix4& matrix4::operator=(const matrix4& other) {
    if (this != &other) {
        const float* src = &other.m_Cell[0][0];
        const float* end = src + 16;
        float* dest = &m_Cell[0][0];
        while (src != end) {
            dest[0] = src[0];
            dest[1] = src[1];
            dest[2] = src[2];
            dest[3] = src[3];
            src += 4;
            dest += 4;
        }
    }
    return *this;
}
int x_isvalid(float value);
matrix4 operator*(const matrix4& left, const matrix4& right);
matrix4 operator*(const matrix4& matrix, const quaternion& rotation);
matrix4 operator*(const quaternion& rotation, const matrix4& matrix);

float x_floor(float value);
float x_ceil(float value);
float x_modf(float value, float* whole);
float x_frexp(float value, int* exponent);
float x_ldexp(float value, int exponent);
float x_fmod(float value, float divisor);
inline float x_lpr(float a, float b) {
    a = x_fmod(a, b);
    if (a < 0.0f) a += b;
    return a;
}

float x_sin(float angle);
float x_cos(float angle);
void x_sincos(float angle, float& sine, float& cosine);

inline vector3 matrix4::GetTranslation()const{return vector3(m_Cell[3][0],m_Cell[3][1],m_Cell[3][2]);}
inline void matrix4::ZeroTranslation(){m_Cell[3][0]=m_Cell[3][1]=m_Cell[3][2]=0.0f;}
inline void matrix4::SetRotation(const radian3& Rotation){
 float sx,cx,sy,cy,sz,cz,sxsz,sxcz;
 x_sincos(Rotation.Pitch,sx,cx);x_sincos(Rotation.Yaw,sy,cy);x_sincos(Rotation.Roll,sz,cz);
 sxsz=sx*sz;sxcz=sx*cz;
 m_Cell[0][0]=cy*cz+sy*sxsz;m_Cell[0][1]=cx*sz;m_Cell[0][2]=cy*sxsz-sy*cz;
 m_Cell[1][0]=sy*sxcz-sz*cy;m_Cell[1][1]=cx*cz;m_Cell[1][2]=sy*sz+sxcz*cy;
 m_Cell[2][0]=cx*sy;m_Cell[2][1]=-sx;m_Cell[2][2]=cx*cy;
}
inline void matrix4::Setup(const radian3& Rotation){
 SetRotation(Rotation);m_Cell[0][3]=0;m_Cell[1][3]=0;m_Cell[2][3]=0;m_Cell[3][3]=1;
 m_Cell[3][2]=0;m_Cell[3][1]=0;m_Cell[3][0]=0;
}

float x_atan2(float y, float x);
quaternion BlendSlow(const quaternion& first, const quaternion& second, float factor);
quaternion BlendToIdentitySlow(const quaternion& first, float factor);
quaternion Blend(const quaternion& first, const quaternion& second, float factor);
quaternion BlendToIdentity(const quaternion& first, float factor);


inline float vector3::GetPitch() const
{
    float L = x_sqrt(X * X + Z * Z);
    float P = -x_atan2(Y, L);
    return P;
}
inline float vector3::GetYaw() const { return x_atan2(X,Z); }

inline void vector3::GetPitchYaw(float& Pitch, float& Yaw) const
{
    Pitch = GetPitch();
    Yaw = GetYaw();
}

inline void vector3::Set(float Pitch, float Yaw)
{
    float PS, PC;
    float YS, YC;
    x_sincos(Pitch, PS, PC);
    x_sincos(Yaw, YS, YC);
    Set(YS * PC, -PS, YC * PC);
}


RVA(0x000357e0, 0x19)
inline const vector3& vector3::operator=(const vector3& value){X=value.X;Y=value.Y;Z=value.Z;return *this;}
RVA(0x00002000, 0x1f)
inline float vector3::LengthSquared()const{return X*X+Y*Y+Z*Z;}
RVA(0x00002020, 0x1b)
inline float vector3::Dot(const vector3& value)const{return X*value.X+Y*value.Y+Z*value.Z;}
RVA(0x00048290, 0x1f)
inline void vector3::Scale(float scale){X=X*scale;Y=Y*scale;Z=Z*scale;}

inline float x_1sqrt(float value){return 1.0f/x_sqrt(value);}
inline vector3 vector3::operator-() const { return(vector3(-X,-Y,-Z)); }
inline vector3 v3_Cross(const vector3& A,const vector3& B) {
 return vector3((A.Y*B.Z)-(A.Z*B.Y),(A.Z*B.X)-(A.X*B.Z),(A.X*B.Y)-(A.Y*B.X));
}
inline int vector3::Normalize() {
 float n=x_1sqrt(X*X+Y*Y+Z*Z);
 if(x_isvalid(n)){X*=n;Y*=n;Z*=n;return 1;}
 X=0;Y=0;Z=0;return 0;
}
struct plane
{
    plane(const vector3& P1,const vector3& P2,const vector3& P3);
    void GetOrthoVectors(vector3& AxisA,vector3& AxisB)const;
    vector3 Normal;
    float D;
    void Setup(float A,float B,float C,float aD){Normal.X=A;Normal.Y=B;Normal.Z=C;D=aD;float Len=Normal.LengthSquared();if(!(0.9999f<=Len&&Len<=1.0001f)){float Factor=x_1sqrt(Len);Normal.Scale(Factor);D*=Factor;}}
    void Setup(const vector3& aNormal,float aDistance){Normal=aNormal;D=aDistance;float Len=Normal.LengthSquared();if(!(0.9999f<=Len&&Len<=1.0001f)){float Factor=x_1sqrt(Len);Normal.Scale(Factor);D*=Factor;}}
    plane(){}
    void Setup(const vector3& P1,const vector3& P2,const vector3& P3);
    float Dot(const vector3& P)const;
    void Negate();

    void ComputeD(const vector3& P);
    void GetBBoxIndices(int* MinI,int* MaxI) const;

    void Setup(const vector3& point,const vector3& normal)
    {
        Normal=normal;
        float length=Normal.LengthSquared();
        if(!(0.9999f<=length && length<=1.0001f))
            Normal.Scale(x_1sqrt(length));
        D=-Normal.Dot(point);
    }
    int InFront(const vector3& P)const{return P.X*Normal.X+P.Y*Normal.Y+P.Z*Normal.Z+D>=0.0f;}
    int InBack(const vector3& P)const{return P.X*Normal.X+P.Y*Normal.Y+P.Z*Normal.Z+D<0.0f;}
    int Intersect(float& t,const vector3& P0,const vector3& P1)const;
    void Transform(const matrix4& M);
    float Distance(const vector3& value)const { return Dot(value)+D; }
    int ClipNGon(vector3* output,int& outputCount,const vector3* input,int inputCount)const;
};
inline void plane::ComputeD(const vector3& P) { D = -((Normal.X*P.X)+(Normal.Y*P.Y)+(Normal.Z*P.Z)); }
inline void plane::GetBBoxIndices(int* MinI,int* MaxI) const {
        if(MinI) {
            if(Normal.X>=0) MinI[0]=0; else MinI[0]=3;
            if(Normal.Y>=0) MinI[1]=1; else MinI[1]=4;
            if(Normal.Z>=0) MinI[2]=2; else MinI[2]=5;
        }
        if(MaxI) {
            if(Normal.X>=0) MaxI[0]=3; else MaxI[0]=0;
            if(Normal.Y>=0) MaxI[1]=4; else MaxI[1]=1;
            if(Normal.Z>=0) MaxI[2]=5; else MaxI[2]=2;
        }
    }
inline vector3 operator+(const vector3& a,const vector3& b);
inline vector3 operator-(const vector3& a,const vector3& b);
inline vector3 operator*(const vector3& a,float s);

struct bbox
{
    void operator()(const vector3& P1,const vector3& P2);
    vector3 Min,Max;
    bbox(){Min.Set(3.402823466e38f,3.402823466e38f,3.402823466e38f);Max.Set(-3.402823466e38f,-3.402823466e38f,-3.402823466e38f);}
    void Clear(){Min.Set(3.402823466e38f,3.402823466e38f,3.402823466e38f);Max.Set(-3.402823466e38f,-3.402823466e38f,-3.402823466e38f);}
    bbox(const vector3& P1,const vector3& P2,const vector3& P3);
    bbox(const vector3& P1){Min=Max=P1;}
    bbox(const vector3& P1,const vector3& P2){Set(P1,P2);}
    bbox(const vector3& Center,float Radius){Set(Center,Radius);}
    void Set(const vector3& P1,const vector3& P2){
     Min.Set(P1.X<P2.X?P1.X:P2.X,P1.Y<P2.Y?P1.Y:P2.Y,P1.Z<P2.Z?P1.Z:P2.Z);
     Max.Set(P1.X>P2.X?P1.X:P2.X,P1.Y>P2.Y?P1.Y:P2.Y,P1.Z>P2.Z?P1.Z:P2.Z);
    }
    void Set(const vector3& Center,float Radius){Min=Center-vector3(Radius,Radius,Radius);Max=Center+vector3(Radius,Radius,Radius);}
    vector3 GetSize()const{return Max-Min;}
    vector3 GetCenter()const{return (Min+Max)*0.5f;}
    float GetRadius()const{return (Max-Min).Length()*0.5f;}
    float GetRadiusSquared()const{vector3 R=(Max-Min)*0.5f;return R.X*R.X+R.Y*R.Y+R.Z*R.Z;}
    void Inflate(float X,float Y,float Z){Min.X-=X;Max.X+=X;Min.Y-=Y;Max.Y+=Y;Min.Z-=Z;Max.Z+=Z;}
    void Translate(const vector3& T){Min+=T;Max+=T;}
    void Translate(float X,float Y,float Z){Min.X+=X;Max.X+=X;Min.Y+=Y;Max.Y+=Y;Min.Z+=Z;Max.Z+=Z;}
    void Transform(const matrix4& M);
    bbox& operator+=(const bbox& B){
     if(B.Min.X<Min.X)Min.X=B.Min.X;if(B.Min.Y<Min.Y)Min.Y=B.Min.Y;if(B.Min.Z<Min.Z)Min.Z=B.Min.Z;
     if(B.Max.X>Max.X)Max.X=B.Max.X;if(B.Max.Y>Max.Y)Max.Y=B.Max.Y;if(B.Max.Z>Max.Z)Max.Z=B.Max.Z;
     return *this;
    }
    bbox& operator+=(const vector3& P){
     if(P.X<Min.X)Min.X=P.X;if(P.Y<Min.Y)Min.Y=P.Y;if(P.Z<Min.Z)Min.Z=P.Z;
     if(P.X>Max.X)Max.X=P.X;if(P.Y>Max.Y)Max.Y=P.Y;if(P.Z>Max.Z)Max.Z=P.Z;
     return *this;
    }
    bbox& AddVerts(const vector3* vertices,int count);
    int Intersect(float& t,const vector3& P0,const vector3& P1)const;
    int IntersectTriBBox(const vector3& P0,const vector3& P1,const vector3& P2)const;
    int Intersect(const vector3& Center,float Radius)const;
    int Intersect(const vector3& P)const{return P.GetX()<=Max.GetX()&&P.GetY()<=Max.GetY()&&P.GetZ()<=Max.GetZ()&&P.GetX()>=Min.GetX()&&P.GetY()>=Min.GetY()&&P.GetZ()>=Min.GetZ();}
    int Intersect(const bbox& other)const;
    int Intersect(const vector3* vertices,int count)const;
};

// PC view::GetViewport(rect&) writes these four floats in Min/Max order.
// The two-vector grouping is corroborated by both sibling x_math.hpp files.
struct rect {
    float GetWidth()const{return Max.X-Min.X;}
    float GetHeight()const{return Max.Y-Min.Y;}
    vector2 Min;
    vector2 Max;
};


inline vector3 operator+(const vector3& a,const vector3& b){return vector3(a.X+b.X,a.Y+b.Y,a.Z+b.Z);}
inline vector3 operator-(const vector3& a,const vector3& b){return vector3(a.X-b.X,a.Y-b.Y,a.Z-b.Z);}
inline vector3 operator*(float s,const vector3& a){return vector3(a.X*s,a.Y*s,a.Z*s);}
RVA(0x000484a0, 0x88)
inline bbox& bbox::AddVerts(const vector3* pVerts,int NVerts)
{
    while(NVerts>0){
        if(pVerts->X<Min.X)Min.X=pVerts->X;
        if(pVerts->Y<Min.Y)Min.Y=pVerts->Y;
        if(pVerts->Z<Min.Z)Min.Z=pVerts->Z;
        if(pVerts->X>Max.X)Max.X=pVerts->X;
        if(pVerts->Y>Max.Y)Max.Y=pVerts->Y;
        if(pVerts->Z>Max.Z)Max.Z=pVerts->Z;
        pVerts++;NVerts--;
    }
    return *this;
}
RVA(0x000322c0, 0x76)
inline int bbox::Intersect(const bbox& BBox)const
{
    if(BBox.Min.X>Max.X)return 0;
    if(BBox.Max.X<Min.X)return 0;
    if(BBox.Min.Z>Max.Z)return 0;
    if(BBox.Max.Z<Min.Z)return 0;
    if(BBox.Min.Y>Max.Y)return 0;
    if(BBox.Max.Y<Min.Y)return 0;
    return 1;
}
RVA(0x000482e0, 0x1b5)
inline int plane::ClipNGon(vector3* pDst,int& NDstVerts,const vector3* pSrc,int NSrcVerts)const
{
    float D0,D1;
    int P0,P1;
    int Clipped=0;
    NDstVerts=0;
    P1=NSrcVerts-1;
    D1=Distance(pSrc[P1]);
    for(int i=0;i<NSrcVerts;i++){
        P0=P1;D0=D1;P1=i;D1=Distance(pSrc[P1]);
        if(D0>=0)pDst[NDstVerts++]=pSrc[P0];
        if(((D0>=0)&&(D1<0))||((D0<0)&&(D1>=0))){
            float d=(D1-D0);
            if((d<0?-d:d)<0.00001f)d=0.00001f;
            float t=(0-D0)/d;
            pDst[NDstVerts++]=pSrc[P0]+t*(pSrc[P1]-pSrc[P0]);
            Clipped=1;
        }
    }
    return Clipped;
}






inline void plane::Setup(const vector3& P1,const vector3& P2,const vector3& P3) {
 Normal=v3_Cross(P2-P1,P3-P1); Normal.Normalize(); D=-Normal.Dot(P1);
}
inline float plane::Dot(const vector3& P)const {return(P.X*Normal.X+P.Y*Normal.Y+P.Z*Normal.Z);}
inline void plane::Negate(){Normal=-Normal;D=-D;}
plane operator*(const matrix4& M,const plane& P);
inline vector3 operator*(const vector3& V,float S){return vector3(V.X*S,V.Y*S,V.Z*S);}

inline
plane operator * ( const matrix4& M, const plane& Plane )
{
    matrix4     Adjoint;
    plane       NewPlane;
    vector3     V;

    // Transform a point in the plane by M
    V = M * ( Plane.Normal * -Plane.D );

    // Compute the Transpouse of the Inverse of the Matrix
    Adjoint(0,0) = M(1,1) * M(2,2) - M(1,2) * M(2,1) ;
    Adjoint(0,1) = M(1,2) * M(2,0) - M(1,0) * M(2,2) ;
    Adjoint(0,2) = M(1,0) * M(2,1) - M(1,1) * M(2,0) ;
    Adjoint(1,0) = M(2,1) * M(0,2) - M(2,2) * M(0,1) ;
    Adjoint(1,1) = M(2,2) * M(0,0) - M(2,0) * M(0,2) ;
    Adjoint(1,2) = M(2,0) * M(0,1) - M(2,1) * M(0,0) ;
    Adjoint(2,0) = M(0,1) * M(1,2) - M(0,2) * M(1,1) ;
    Adjoint(2,1) = M(0,2) * M(1,0) - M(0,0) * M(1,2) ;
    Adjoint(2,2) = M(0,0) * M(1,1) - M(0,1) * M(1,0) ;

    // Transform the normal
    NewPlane.Normal.X = Adjoint(0,0) * Plane.Normal.X + 
                        Adjoint(1,0) * Plane.Normal.Y + 
                        Adjoint(2,0) * Plane.Normal.Z;

    NewPlane.Normal.Y = Adjoint(0,1) * Plane.Normal.X + 
                        Adjoint(1,1) * Plane.Normal.Y + 
                        Adjoint(2,1) * Plane.Normal.Z;

    NewPlane.Normal.Z = Adjoint(0,2) * Plane.Normal.X + 
                        Adjoint(1,2) * Plane.Normal.Y + 
                        Adjoint(2,2) * Plane.Normal.Z;

    // Renormalize
    NewPlane.Normal.Normalize();

    // Recompute D by in the transform point
    NewPlane.ComputeD( V );

    return NewPlane;
}


// Scalar sibling engine rotations; no additional layout storage.
inline void matrix4::RotateX( float Rx )
{
    matrix4 RM;
    float     s, c;    

    if( Rx == 0 )  return;

    x_sincos( Rx, s, c );

    RM.Identity();

    RM(1,1) =  c;
    RM(2,1) = -s;
    RM(1,2) =  s;
    RM(2,2) =  c;

    *this = RM * *this;    
}

inline void matrix4::RotateY( float Ry )
{
    matrix4 RM;
    float     s, c;    

    if( Ry == 0 )  return;

    x_sincos( Ry, s, c );

    RM.Identity();

    RM(0,0) =  c;
    RM(2,0) =  s;
    RM(0,2) = -s;
    RM(2,2) =  c;

    *this = RM * *this;    
}

inline void matrix4::RotateZ( float Rz )
{
    matrix4 RM;
    float     s, c;    

    if( Rz == 0 )  return;

    x_sincos( Rz, s, c );

    RM.Identity();

    RM(0,0) =  c;
    RM(1,0) = -s;
    RM(0,1) =  s;
    RM(1,1) =  c;
          
    *this = RM * *this;    
}

inline void matrix4::PreRotateX( float Rx )
{
    matrix4 RM;
    float     s, c;    

    if( Rx == 0 )  return;

    x_sincos( Rx, s, c );

    RM.Identity();

    RM(1,1) =  c;
    RM(2,1) = -s;
    RM(1,2) =  s;
    RM(2,2) =  c;

    *this *= RM;    
}

inline void matrix4::PreRotateY( float Ry )
{
    matrix4 RM;
    float     s, c;    

    if( Ry == 0 )  return;

    x_sincos( Ry, s, c );

    RM.Identity();

    RM(0,0) =  c;
    RM(2,0) =  s;
    RM(0,2) = -s;
    RM(2,2) =  c;

    *this *= RM;    
}

inline void matrix4::PreRotateZ( float Rz )
{
    matrix4 RM;
    float     s, c;    

    if( Rz == 0 )  return;

    x_sincos( Rz, s, c );

    RM.Identity();

    RM(0,0) =  c;
    RM(1,0) = -s;
    RM(0,1) =  s;
    RM(1,1) =  c;
          
    *this *= RM;    
}

inline matrix4& matrix4::operator*=(const matrix4& M){*this=(*this)*M;return *this;}

inline void vector3::RotateX(float Angle){float S,C;x_sincos(Angle,S,C);float y=Y,z=Z;Y=C*y-S*z;Z=C*z+S*y;}

inline void vector3::RotateY(float Angle){float S,C;x_sincos(Angle,S,C);float x=X,z=Z;X=C*x+S*z;Z=C*z-S*x;}
// Real scalar SRT and bounds helpers needed by engine FX/Animation source.
inline void matrix4::SetRotation( const quaternion& Q )
{
    float tx  = 2.0f * Q.X;   
    float ty  = 2.0f * Q.Y;   
    float tz  = 2.0f * Q.Z;   
    float txw =   tx * Q.W;   
    float tyw =   ty * Q.W;   
    float tzw =   tz * Q.W;   
    float txx =   tx * Q.X;   
    float tyx =   ty * Q.X;   
    float tzx =   tz * Q.X;   
    float tyy =   ty * Q.Y;   
    float tzy =   tz * Q.Y;   
    float tzz =   tz * Q.Z;   
                                
    

    m_Cell[0][0] = 1.0f-(tyy+tzz); m_Cell[0][1] = tyx + tzw;      m_Cell[0][2] = tzx - tyw;           
    m_Cell[1][0] = tyx - tzw;      m_Cell[1][1] = 1.0f-(txx+tzz); m_Cell[1][2] = tzy + txw;           
    m_Cell[2][0] = tzx + tyw;      m_Cell[2][1] = tzy - txw;      m_Cell[2][2] = 1.0f-(txx+tyy);    
}
inline void matrix4::Setup(const quaternion& Q){SetRotation(Q);m_Cell[0][3]=0;m_Cell[1][3]=0;m_Cell[2][3]=0;m_Cell[3][3]=1;m_Cell[3][2]=0;m_Cell[3][1]=0;m_Cell[3][0]=0;}
inline void matrix4::Translate(const vector3& T){m_Cell[3][0]+=T.X;m_Cell[3][1]+=T.Y;m_Cell[3][2]+=T.Z;}
inline void matrix4::Setup(const vector3& S,const radian3& R,const vector3& T){Identity();SetRotation(R);PreScale(S);Translate(T);}
inline void matrix4::Setup(const vector3& S,const quaternion& R,const vector3& T){Identity();SetRotation(R);PreScale(S);Translate(T);}
inline void bbox::Transform(const matrix4& M){
 vector3 AMin=Min,AMax=Max;float a,b;int i,j;
 Min.X=Max.X=M(3,0);Min.Y=Max.Y=M(3,1);Min.Z=Max.Z=M(3,2);
 for(j=0;j<3;j++)for(i=0;i<3;i++){
  a=M(i,j)*AMin[i];b=M(i,j)*AMax[i];
  if(a<b){Min[j]+=a;Max[j]+=b;}else{Min[j]+=b;Max[j]+=a;}
 }
}

inline int vector3::IsValid()const{return x_isvalid(X)&&x_isvalid(Y)&&x_isvalid(Z);}
inline int radian3::IsValid()const{return x_isvalid(Pitch)&&x_isvalid(Yaw)&&x_isvalid(Roll);}
inline radian3 matrix4::GetRotation()const{
 float s=m_Cell[2][1];if(s>1.0f)s=1.0f;if(s< -1.0f)s=-1.0f;
 float Pitch=x_asin(-s),Yaw=x_atan2(m_Cell[2][0],m_Cell[2][2]),Roll=x_atan2(m_Cell[0][1],m_Cell[1][1]);
 if(Yaw==0.0f&&Roll==0.0f)Roll=-s*x_atan2(m_Cell[0][2],m_Cell[0][0]);
 return radian3(Pitch,Yaw,Roll);
}
inline quaternion operator*(const quaternion& L,const quaternion& R){
 quaternion Q;
 Q.X=L.W*R.X+R.W*L.X+L.Y*R.Z-L.Z*R.Y;
 Q.Y=L.W*R.Y+R.W*L.Y+L.Z*R.X-L.X*R.Z;
 Q.Z=L.W*R.Z+R.W*L.Z+L.X*R.Y-L.Y*R.X;
 Q.W=L.W*R.W-R.X*L.X-L.Y*R.Y-L.Z*R.Z;
 return Q;
}
inline quaternion& quaternion::operator*=(const quaternion& R){*this=(*this)*R;return *this;}
inline int matrix4::IsValid()const{for(int i=0;i<16;i++)if(!x_isvalid(((const float*)this)[i]))return 0;return 1;}

inline void quaternion::Setup( const radian3& R )
{
    Identity();
    RotateZ( R.Roll  );
    RotateX( R.Pitch );
    RotateY( R.Yaw   );
}

inline void quaternion::Setup( const vector3& Axis, float Angle )
{
    float Sine, Cosine;

    x_sincos( Angle * 0.5f, Sine, Cosine );

    W = Cosine;           
    X = Sine * Axis.X;    
    Y = Sine * Axis.Y;    
    Z = Sine * Axis.Z;    
}

inline radian3 quaternion::GetRotation( void ) const
{
    float tx  = 2.0f * X;   
    float ty  = 2.0f * Y;   
    float tz  = 2.0f * Z;   
    float txw =   tx * W;   
    float tyw =   ty * W;   
    float tzw =   tz * W;   
    float txx =   tx * X;   
    float tyx =   ty * X;   
    float tzx =   tz * X;   
    float tyy =   ty * Y;   
    float tzy =   tz * Y;   
    float tzz =   tz * Z;   
                                
    float  Pitch, Yaw, Roll;
    float     s;

    
    
    s = tzy - txw;    
    if( s >  1.0f )  s =  1.0f;
    if( s < -1.0f )  s = -1.0f;
    Pitch = x_asin( -s );

    
    
    if( (Pitch > -R_89) || (Pitch < R_89) )
    {
        Yaw  = x_atan2( tzx + tyw, 1.0f-(txx+tyy) );
        Roll = x_atan2( tyx + tzw, 1.0f-(txx+tzz) );
    }
    else
    {
        Yaw  = 0.0f;
        Roll = x_atan2( tzx - tyw, 1.0f-(tyy+tzz) );
    }

    return( radian3( Pitch, Yaw, Roll ) );
}

inline void quaternion::RotateX( float Rx )
{
    float s, c;
    x_sincos( Rx/2, s, c );
    quaternion Q( s, 0, 0, c );

    *this = Q * *this;
}

inline void quaternion::RotateY( float Ry )
{
    float s, c;
    x_sincos( Ry/2, s, c );
    quaternion Q( 0, s, 0, c );

    *this = Q * *this;
}

inline void quaternion::RotateZ( float Rz )
{
    float s, c;
    x_sincos( Rz/2, s, c );
    quaternion Q( 0, 0, s, c );

    *this = Q * *this;
}

inline quaternion::quaternion(const radian3& R){Setup(R);}

inline quaternion::quaternion(const vector3& Axis,float Angle){Setup(Axis,Angle);}

inline matrix4::matrix4(const radian3& R){Setup(R);}

inline void matrix4::Rotate(const radian3& R){matrix4 RM(R);*this=RM*(*this);}

inline void matrix4::Rotate(const quaternion& Q){matrix4 RM(Q);*this=RM*(*this);}

inline void matrix4::Scale(float S){for(int c=0;c<4;c++){m_Cell[c][0]*=S;m_Cell[c][1]*=S;m_Cell[c][2]*=S;}}

inline void matrix4::Scale(const vector3& S){for(int c=0;c<4;c++){m_Cell[c][0]*=S.X;m_Cell[c][1]*=S.Y;m_Cell[c][2]*=S.Z;}}

inline void matrix4::DecomposeSRT(vector3& S,quaternion& R,vector3& T){S=GetScale();R=GetQuaternion();T=GetTranslation();}

inline void matrix4::DecomposeSRT(vector3& S,radian3& R,vector3& T){S=GetScale();R=GetRotation();T=GetTranslation();}

inline int vector3::NormalizeAndScale(float Scalar){float N=x_1sqrt(X*X+Y*Y+Z*Z);if(x_isvalid(N)){N*=Scalar;X*=N;Y*=N;Z*=N;return 1;}X=Y=Z=0;return 0;}
inline vector4 operator*(const vector4& V,float S){return vector4(V.X*S,V.Y*S,V.Z*S,V.W*S);}
inline vector4 operator*(float S,const vector4& V){return vector4(V.X*S,V.Y*S,V.Z*S,V.W*S);}
inline vector3 operator/(const vector3& V,float S){float R=1.0f/S;return vector3(V.X*R,V.Y*R,V.Z*R);}
inline void matrix4::Transform(vector3* Dest,const vector3* Source,int Count)const{while(Count>0){*Dest=Transform(*Source);Dest++;Source++;Count--;}}
#include <xCore/x_files/x_math_misc.hpp>
// Additional authentic scalar engine APIs; no unverified PC spans.
inline int matrix4::InvertRT( void )
{
    matrix4 Src = *this;

    m_Cell[0][0] = Src(0,0);
    m_Cell[0][1] = Src(1,0);
    m_Cell[0][2] = Src(2,0);
    m_Cell[1][0] = Src(0,1);
    m_Cell[1][1] = Src(1,1);
    m_Cell[1][2] = Src(2,1);
    m_Cell[2][0] = Src(0,2);
    m_Cell[2][1] = Src(1,2);
    m_Cell[2][2] = Src(2,2);
    m_Cell[0][3] = 0.0f;
    m_Cell[1][3] = 0.0f;
    m_Cell[2][3] = 0.0f;
    m_Cell[3][3] = 1.0f;
    m_Cell[3][0] = -(Src(3,0)*m_Cell[0][0] + Src(3,1)*m_Cell[1][0] + Src(3,2)*m_Cell[2][0]);
    m_Cell[3][1] = -(Src(3,0)*m_Cell[0][1] + Src(3,1)*m_Cell[1][1] + Src(3,2)*m_Cell[2][1]);
    m_Cell[3][2] = -(Src(3,0)*m_Cell[0][2] + Src(3,1)*m_Cell[1][2] + Src(3,2)*m_Cell[2][2]);

    return 1;
}

inline float matrix4::Difference( const matrix4& M ) const
{
    float Diff=0.0f;
    for( int i=0; i<16; i++ )
        Diff += (((float*)this)[i] - ((float*)(&M))[i])*
                (((float*)this)[i] - ((float*)(&M))[i]);
    return Diff;
}

inline void matrix4::GetColumns( vector3& V1, vector3& V2, vector3& V3 ) const
{
    V1.X = m_Cell[0][0];     V2.X = m_Cell[1][0];     V3.X = m_Cell[2][0];
    V1.Y = m_Cell[0][1];     V2.Y = m_Cell[1][1];     V3.Y = m_Cell[2][1];
    V1.Z = m_Cell[0][2];     V2.Z = m_Cell[1][2];     V3.Z = m_Cell[2][2];
}

inline void matrix4::SetColumns( const vector3& V1, 
                          const vector3& V2, 
                          const vector3& V3 )
{
    m_Cell[0][0] = V1.X;     m_Cell[1][0] = V2.X;     m_Cell[2][0] = V3.X;
    m_Cell[0][1] = V1.Y;     m_Cell[1][1] = V2.Y;     m_Cell[2][1] = V3.Y;
    m_Cell[0][2] = V1.Z;     m_Cell[1][2] = V2.Z;     m_Cell[2][2] = V3.Z;
}

inline void matrix4::SetRows( const vector3& V1, 
                       const vector3& V2, 
                       const vector3& V3 )
{
    m_Cell[0][0] = V1.X;     m_Cell[1][0] = V1.Y;     m_Cell[2][0] = V1.Z;
    m_Cell[0][1] = V2.X;     m_Cell[1][1] = V2.Y;     m_Cell[2][1] = V2.Z;
    m_Cell[0][2] = V3.X;     m_Cell[1][2] = V3.Y;     m_Cell[2][2] = V3.Z;
}

inline void matrix4::SetScale( float Scale )
{
    m_Cell[0][0] = Scale;
    m_Cell[1][1] = Scale;
    m_Cell[2][2] = Scale;
}

inline void matrix4::SetScale( const vector3& Scale )
{
    m_Cell[0][0] = Scale.X;
    m_Cell[1][1] = Scale.Y;
    m_Cell[2][2] = Scale.Z;
}

inline vector3 matrix4::RotateVector( const vector3& V ) const
{
    return( vector3( (m_Cell[0][0]*V.X) + (m_Cell[1][0]*V.Y) + (m_Cell[2][0]*V.Z),
                     (m_Cell[0][1]*V.X) + (m_Cell[1][1]*V.Y) + (m_Cell[2][1]*V.Z),
                     (m_Cell[0][2]*V.X) + (m_Cell[1][2]*V.Y) + (m_Cell[2][2]*V.Z) ) );
}

inline vector3 matrix4::InvRotateVector( const vector3& V ) const
{
    return( vector3( (m_Cell[0][0]*V.X) + (m_Cell[0][1]*V.Y) + (m_Cell[0][2]*V.Z),
                     (m_Cell[1][0]*V.X) + (m_Cell[1][1]*V.Y) + (m_Cell[1][2]*V.Z),
                     (m_Cell[2][0]*V.X) + (m_Cell[2][1]*V.Y) + (m_Cell[2][2]*V.Z) ) );
}

inline vector3 vector3::GetClosestVToLSeg( const vector3& Start, const vector3& End ) const
{
    vector3 Diff = *this - Start;
    vector3 Dir  = End   - Start;
    float     T    = Diff.Dot( Dir );

    if( T > 0.0 )
    {
        float SqrLen = Dir.Dot( Dir );

        if ( T >= SqrLen )
        {
            Diff -= Dir;
        }
        else
        {
            T    /= SqrLen;
            Diff -= T * Dir;
        }
    }

    return -Diff;
}

inline vector3 vector3::GetClosestPToLSeg( const vector3& Start, const vector3& End ) const
{
    return GetClosestVToLSeg(Start,End) + *this;
}

inline float vector3::GetSqrtDistToLineSeg( const vector3& Start, const vector3& End ) const
{
    return GetClosestVToLSeg(Start,End).LengthSquared();
}

inline int plane::Intersect( float& t, const vector3& P0, const vector3& P1 ) const
{
    t = (P1 - P0).Dot( Normal );

    if( t == 0.0f ) 
        return( 0 );
    else    
    {
        t = -Distance( P0 ) / t;
        return( 1 );
    }
}

inline float x_sqr(float a){return a*a;}
inline float x_ModAngle(float a){if(a>RADIAN(1440)||a<RADIAN(-1440))a=x_fmod(a,R_360);while(a>=R_360)a-=R_360;while(a<R_0)a+=R_360;return a;}
inline float x_ModAngle2(float a){a+=R_180;a=x_ModAngle(a);a-=R_180;return a;}
inline float x_MinAngleDiff(float a,float b){return x_ModAngle2(a-b);}
inline float v3_Dot(const vector3& A,const vector3& B){return A.X*B.X+A.Y*B.Y+A.Z*B.Z;}
inline int vector3::SafeNormalize(){float N=x_1sqrt(GetX()*GetX()+GetY()*GetY()+GetZ()*GetZ());if(x_isvalid(N)){GetX()*=N;GetY()*=N;GetZ()*=N;return 1;}GetX()=GetY()=GetZ()=0.0f;return 0;}
inline int vector3::SafeNormalizeAndScale(float Scalar){float N=x_1sqrt(GetX()*GetX()+GetY()*GetY()+GetZ()*GetZ());if(x_isvalid(N)){N*=Scalar;GetX()*=N;GetY()*=N;GetZ()*=N;return 1;}GetX()=GetY()=GetZ()=0.0f;return 0;}
inline void matrix4::Setup(const vector3& V1,const vector3& V2,float Angle){Setup(V2-V1,Angle);PreTranslate(-V1);Translate(V1);}
inline void plane::Transform(const matrix4& M){*this=M*(*this);}
inline vector2 operator+(const vector2& A,const vector2& B){return vector2(A.X+B.X,A.Y+B.Y);}
inline vector2 operator-(const vector2& A,const vector2& B){return vector2(A.X-B.X,A.Y-B.Y);}
inline vector2 operator*(const vector2& V,float S){return vector2(V.X*S,V.Y*S);}
inline vector2 operator*(float S,const vector2& V){return vector2(V.X*S,V.Y*S);}
inline vector2 operator/(const vector2& V,float S){S=1.0f/S;return vector2(V.X*S,V.Y*S);}
inline vector4 operator+(const vector4& A,const vector4& B){return vector4(A.X+B.X,A.Y+B.Y,A.Z+B.Z,A.W+B.W);}
inline vector4 operator-(const vector4& A,const vector4& B){return vector4(A.X-B.X,A.Y-B.Y,A.Z-B.Z,A.W-B.W);}
inline vector4 operator/(const vector4& V,float S){S=1.0f/S;return vector4(V.X*S,V.Y*S,V.Z*S,V.W*S);}
inline vector4 operator*(const vector4& A,const vector4& B){return vector4(A.GetX()*B.GetX(),A.GetY()*B.GetY(),A.GetZ()*B.GetZ(),A.GetW()*B.GetW());}
inline radian3 operator+(const radian3& A,const radian3& B){return radian3(A.Pitch+B.Pitch,A.Yaw+B.Yaw,A.Roll+B.Roll);}
inline radian3 operator-(const radian3& A,const radian3& B){return radian3(A.Pitch-B.Pitch,A.Yaw-B.Yaw,A.Roll-B.Roll);}
inline radian3 operator*(const radian3& A,float S){return radian3(A.Pitch*S,A.Yaw*S,A.Roll*S);}

inline int bbox::Intersect(const vector3& Center,float Radius)const{
 float d,dmin=0;
 if(Center.GetX()<Min.GetX()){d=Center.GetX()-Min.GetX();dmin+=d*d;}else if(Center.GetX()>Max.GetX()){d=Center.GetX()-Max.GetX();dmin+=d*d;}
 if(Center.GetY()<Min.GetY()){d=Center.GetY()-Min.GetY();dmin+=d*d;}else if(Center.GetY()>Max.GetY()){d=Center.GetY()-Max.GetY();dmin+=d*d;}
 if(Center.GetZ()<Min.GetZ()){d=Center.GetZ()-Min.GetZ();dmin+=d*d;}else if(Center.GetZ()>Max.GetZ()){d=Center.GetZ()-Max.GetZ();dmin+=d*d;}
 if(dmin<=Radius*Radius)return 1;return 0;
}

inline
vector3 quaternion::operator * ( const vector3& V ) const
{
    vector3 Result;
    vector3 v1;
    vector3 v2;

    Result.Set( X, Y, Z );
    v1     = v3_Cross( Result, V  );
    v2     = v3_Cross( Result, v1 );
    v1    *= 2.0f * W;
    v2    *= 2.0f;
    Result = V + v1 + v2;

    return( Result );
}

//==============================================================================

inline
vector3 quaternion::Rotate( const vector3& V ) const
{
    return( *this * V );
}

//==============================================================================

inline
void quaternion::Rotate(       vector3* pDest, 
                         const vector3* pSource, 
                               s32      NVerts ) const
{
    s32 i;

    for( i = 0; i < NVerts; i++ )
    {
        *pDest = *this * *pSource;
        pDest++;
        pSource++;
    }
}


inline void bbox::operator()(const vector3& P1,const vector3& P2)
{
    Min( MIN(P1.X,P2.X), MIN(P1.Y,P2.Y), MIN(P1.Z,P2.Z) );
    Max( MAX(P1.X,P2.X), MAX(P1.Y,P2.Y), MAX(P1.Z,P2.Z) );
}

#include <xCore/x_files/Implementation/x_math_scalar_closure.hpp>

#endif
