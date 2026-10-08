//==============================================================================
//
//  x_math_m4_inline.hpp
//
//==============================================================================

#ifndef X_MATH_M4_INLINE_HPP
#define X_MATH_M4_INLINE_HPP
#else
#error "File " __FILE__ " has been included twice!"
#endif

#ifndef X_DEBUG_HPP
#include "../x_debug.hpp"
#endif

#ifndef X_PLUS_HPP
#include "../x_plus.hpp"
#endif

//==============================================================================
//  DEFINES
//==============================================================================

#define M(row,column)   m_Cell[row][column]

//==============================================================================
//  FUNCTIONS
//==============================================================================

inline 
matrix4::matrix4( void )
{
}

//==============================================================================

inline 
matrix4::matrix4( const matrix4& M )
{   
    M(0,0) = M.M(0,0);
    M(0,1) = M.M(0,1);
    M(0,2) = M.M(0,2);
    M(0,3) = M.M(0,3);

    M(1,0) = M.M(1,0);
    M(1,1) = M.M(1,1);
    M(1,2) = M.M(1,2);
    M(1,3) = M.M(1,3);

    M(2,0) = M.M(2,0);
    M(2,1) = M.M(2,1);
    M(2,2) = M.M(2,2);
    M(2,3) = M.M(2,3);

    M(3,0) = M.M(3,0);
    M(3,1) = M.M(3,1);
    M(3,2) = M.M(3,2);
    M(3,3) = M.M(3,3);
}

//==============================================================================

inline 
matrix4::matrix4( const radian3& R )
{   
    Setup( R );
}

//==============================================================================

inline 
matrix4::matrix4( const quaternion& Q )
{   
    Setup( Q );
}

//==============================================================================

inline 
matrix4::matrix4( const vector3& Scale,
                  const radian3& Rotation,
                  const vector3& Translation )
{
    Setup( Scale, Rotation, Translation );
}

//==============================================================================

inline 
matrix4::matrix4( const vector3& Scale,
                  const quaternion& Rotation,
                  const vector3& Translation )
{
    Setup( Scale, Rotation, Translation );
}

//==============================================================================

inline
f32 matrix4::operator () ( s32 Column, s32 Row ) const
{
    return( m_Cell[Column][Row] );
}

//==============================================================================

inline
f32& matrix4::operator () ( s32 Column, s32 Row )
{
    return( m_Cell[Column][Row] );
}

//==============================================================================

inline 
void matrix4::Zero( void )
{
    M(0,0) = M(1,0) = M(2,0) = M(3,0) = 0.0f;
    M(0,1) = M(1,1) = M(2,1) = M(3,1) = 0.0f;
    M(0,2) = M(1,2) = M(2,2) = M(3,2) = 0.0f;
    M(0,3) = M(1,3) = M(2,3) = M(3,3) = 0.0f;
}

//==============================================================================

inline 
void matrix4::Identity( void )
{    
             M(1,0) = M(2,0) = M(3,0) = 0.0f;
    M(0,1)          = M(2,1) = M(3,1) = 0.0f;
    M(0,2) = M(1,2)          = M(3,2) = 0.0f;
    M(0,3) = M(1,3) = M(2,3)          = 0.0f;

    M(0,0) = 1.0f;
    M(1,1) = 1.0f;
    M(2,2) = 1.0f;
    M(3,3) = 1.0f;
}

//==============================================================================

inline
void matrix4::Transpose( void )
{
    f32     T;    
   
    T      = M(1,0);
    M(1,0) = M(0,1);
    M(0,1) = T;
    
    T      = M(2,0);
    M(2,0) = M(0,2);
    M(0,2) = T;
    
    T      = M(3,0);
    M(3,0) = M(0,3);
    M(0,3) = T;

    T      = M(2,1);
    M(2,1) = M(1,2);
    M(1,2) = T;
    
    T      = M(3,1);
    M(3,1) = M(1,3);
    M(1,3) = T;

    T      = M(3,2);
    M(3,2) = M(2,3);
    M(2,3) = T;
}

//==============================================================================

inline
xbool matrix4::InvertRT( void )
{
    matrix4 Src = *this;

    M(0,0) = Src(0,0);
    M(0,1) = Src(1,0);
    M(0,2) = Src(2,0);
    M(1,0) = Src(0,1);
    M(1,1) = Src(1,1);
    M(1,2) = Src(2,1);
    M(2,0) = Src(0,2);
    M(2,1) = Src(1,2);
    M(2,2) = Src(2,2);
    M(0,3) = 0.0f;
    M(1,3) = 0.0f;
    M(2,3) = 0.0f;
    M(3,3) = 1.0f;
    M(3,0) = -(Src(3,0)*M(0,0) + Src(3,1)*M(1,0) + Src(3,2)*M(2,0));
    M(3,1) = -(Src(3,0)*M(0,1) + Src(3,1)*M(1,1) + Src(3,2)*M(2,1));
    M(3,2) = -(Src(3,0)*M(0,2) + Src(3,1)*M(1,2) + Src(3,2)*M(2,2));

    return TRUE;
}

//==============================================================================

inline 
void matrix4::Orthogonalize( void )
{
    vector3 VX;
    vector3 VY;
    vector3 VZ;    

    VX.X = M(0,0); 
    VX.Y = M(0,1);
    VX.Z = M(0,2);
    
    VY.X = M(1,0); 
    VY.Y = M(1,1);
    VY.Z = M(1,2);

    VX.Normalize();
    VY.Normalize();

    VZ = v3_Cross( VX, VY );
    VY = v3_Cross( VZ, VX );

    SetColumns( VX, VY, VZ );
}

//==============================================================================

inline 
vector3 matrix4::GetScale( void ) const
{
    return( vector3( M(0,0), M(1,1), M(2,2) ) );
}

//==============================================================================

inline 
vector3 matrix4::GetTranslation( void ) const
{
    return( vector3( M(3,0), M(3,1), M(3,2) ) );
}

//==============================================================================

inline 
void matrix4::ZeroScale( void )
{
    M(0,0) = M(1,1) = M(2,2) = 0.0f;
}

//==============================================================================

inline 
void matrix4::ZeroRotation( void )
{                       
    M(0,0) = 1.0f;     M(1,0) = 0.0f;     M(2,0) = 0.0f;
    M(0,1) = 0.0f;     M(1,1) = 1.0f;     M(2,1) = 0.0f;
    M(0,2) = 0.0f;     M(1,2) = 0.0f;     M(2,2) = 1.0f;
}

//==============================================================================

inline 
void matrix4::ZeroTranslation( void )
{
    M(3,0) = M(3,1) = M(3,2) = 0.0f;
}

//==============================================================================

inline 
void matrix4::SetScale( f32 Scale )
{
    M(0,0) = Scale;
    M(1,1) = Scale;
    M(2,2) = Scale;
}

//==============================================================================

inline 
void matrix4::SetScale( const vector3& Scale )
{
    M(0,0) = Scale.X;
    M(1,1) = Scale.Y;
    M(2,2) = Scale.Z;
}

//==============================================================================

inline 
void matrix4::SetTranslation( const vector3& Translation )
{
    M(3,0) = Translation.X;
    M(3,1) = Translation.Y;
    M(3,2) = Translation.Z;
}

//==============================================================================

inline 
void matrix4::Scale( f32 Scale )
{
    // Scale the rotation.

    M(0,0) *= Scale;
    M(0,1) *= Scale;
    M(0,2) *= Scale;

    M(1,0) *= Scale;
    M(1,1) *= Scale;
    M(1,2) *= Scale;

    M(2,0) *= Scale;
    M(2,1) *= Scale;
    M(2,2) *= Scale;

    // Scale the translation.

    M(3,0) *= Scale;
    M(3,1) *= Scale;
    M(3,2) *= Scale;
}

//==============================================================================

inline 
void matrix4::Scale( const vector3& Scale )
{
    // Scale the rotation.

    M(0,0) *= Scale.X;
    M(0,1) *= Scale.Y;
    M(0,2) *= Scale.Z;

    M(1,0) *= Scale.X;
    M(1,1) *= Scale.Y;
    M(1,2) *= Scale.Z;

    M(2,0) *= Scale.X;
    M(2,1) *= Scale.Y;
    M(2,2) *= Scale.Z;

    // Scale the translation.

    M(3,0) *= Scale.X;
    M(3,1) *= Scale.Y;
    M(3,2) *= Scale.Z;
}

//==============================================================================

inline 
void matrix4::Rotate( const radian3& Rotation )
{
    matrix4 RM( Rotation );
    *this = RM * *this;    
}

//==============================================================================

inline 
void matrix4::Rotate( const quaternion& Q )
{
    matrix4 RM( Q );
    *this = RM * *this;    
}

//==============================================================================

inline 
void matrix4::RotateX( radian Rx )
{
    matrix4 RM;
    f32     s, c;    

    if( Rx == 0 )  return;

    x_sincos( Rx, s, c );

    RM.Identity();

    RM.M(1,1) =  c;
    RM.M(2,1) = -s;
    RM.M(1,2) =  s;
    RM.M(2,2) =  c;

    *this = RM * *this;    
}

//==============================================================================

inline 
void matrix4::RotateY( radian Ry )
{
    matrix4 RM;
    f32     s, c;    

    if( Ry == 0 )  return;

    x_sincos( Ry, s, c );

    RM.Identity();

    RM.M(0,0) =  c;
    RM.M(2,0) =  s;
    RM.M(0,2) = -s;
    RM.M(2,2) =  c;

    *this = RM * *this;    
}

//==============================================================================

inline 
void matrix4::RotateZ( radian Rz )
{
    matrix4 RM;
    f32     s, c;    

    if( Rz == 0 )  return;

    x_sincos( Rz, s, c );

    RM.Identity();

    RM.M(0,0) =  c;
    RM.M(1,0) = -s;
    RM.M(0,1) =  s;
    RM.M(1,1) =  c;
          
    *this = RM * *this;    
}

//==============================================================================

inline 
void matrix4::Translate( const vector3& Translation )
{        
    //
    // If bottom row of M is [0 0 0 1] do optimized translation.
    //

    if( ( M(0,3) == 0.0f ) && 
        ( M(1,3) == 0.0f ) &&
        ( M(2,3) == 0.0f ) && 
        ( M(3,3) == 1.0f ) )
    {
        M(3,0) += Translation.X;
        M(3,1) += Translation.Y;
        M(3,2) += Translation.Z;
    }
    else 
    {
        M(0,0) += M(0,3) * Translation.X;
        M(1,0) += M(1,3) * Translation.X;
        M(2,0) += M(2,3) * Translation.X;
        M(3,0) += M(3,3) * Translation.X;

        M(0,1) += M(0,3) * Translation.Y;
        M(1,1) += M(1,3) * Translation.Y;
        M(2,1) += M(2,3) * Translation.Y;
        M(3,1) += M(3,3) * Translation.Y;

        M(0,2) += M(0,3) * Translation.Z;
        M(1,2) += M(1,3) * Translation.Z;
        M(2,2) += M(2,3) * Translation.Z;
        M(3,2) += M(3,3) * Translation.Z;
    }
}

//==============================================================================

inline 
void matrix4::PreScale( f32 Scale )
{
    M(0,0) *= Scale;
    M(0,1) *= Scale;
    M(0,2) *= Scale;
    M(0,3) *= Scale;

    M(1,0) *= Scale;
    M(1,1) *= Scale;
    M(1,2) *= Scale;
    M(1,3) *= Scale;
    
    M(2,0) *= Scale;
    M(2,1) *= Scale;
    M(2,2) *= Scale;
    M(2,3) *= Scale;
}

//==============================================================================

inline 
void matrix4::PreScale( const vector3& Scale )
{
    M(0,0) *= Scale.X;
    M(0,1) *= Scale.X;
    M(0,2) *= Scale.X;
    M(0,3) *= Scale.X;

    M(1,0) *= Scale.Y;
    M(1,1) *= Scale.Y;
    M(1,2) *= Scale.Y;
    M(1,3) *= Scale.Y;
    
    M(2,0) *= Scale.Z;
    M(2,1) *= Scale.Z;
    M(2,2) *= Scale.Z;
    M(2,3) *= Scale.Z;
}

//==============================================================================

inline 
void matrix4::PreRotate( const radian3& Rotation )
{
    matrix4 RM( Rotation );
    *this *= RM;    
}

//==============================================================================

inline 
void matrix4::PreRotate( const quaternion& Q )
{
    matrix4 RM( Q );
    *this *= RM;    
}

//==============================================================================

inline 
void matrix4::PreRotateX( radian Rx )
{
    matrix4 RM;
    f32     s, c;    

    if( Rx == 0 )  return;

    x_sincos( Rx, s, c );

    RM.Identity();

    RM.M(1,1) =  c;
    RM.M(2,1) = -s;
    RM.M(1,2) =  s;
    RM.M(2,2) =  c;

    *this *= RM;    
}

//==============================================================================

inline 
void matrix4::PreRotateY( radian Ry )
{
    matrix4 RM;
    f32     s, c;    

    if( Ry == 0 )  return;

    x_sincos( Ry, s, c );

    RM.Identity();

    RM.M(0,0) =  c;
    RM.M(2,0) =  s;
    RM.M(0,2) = -s;
    RM.M(2,2) =  c;

    *this *= RM;    
}

//==============================================================================

inline 
void matrix4::PreRotateZ( radian Rz )
{
    matrix4 RM;
    f32     s, c;    

    if( Rz == 0 )  return;

    x_sincos( Rz, s, c );

    RM.Identity();

    RM.M(0,0) =  c;
    RM.M(1,0) = -s;
    RM.M(0,1) =  s;
    RM.M(1,1) =  c;
          
    *this *= RM;    
}

//==============================================================================

inline 
void matrix4::PreTranslate( const vector3& Translation )
{                       
    M(3,0) += (M(0,0) * Translation.X) + (M(1,0) * Translation.Y) + (M(2,0) * Translation.Z);
    M(3,1) += (M(0,1) * Translation.X) + (M(1,1) * Translation.Y) + (M(2,1) * Translation.Z);
    M(3,2) += (M(0,2) * Translation.X) + (M(1,2) * Translation.Y) + (M(2,2) * Translation.Z);
    M(3,3) += (M(0,3) * Translation.X) + (M(1,3) * Translation.Y) + (M(2,3) * Translation.Z);
}

//==============================================================================

inline 
void matrix4::GetRows( vector3& V1, vector3& V2, vector3& V3 ) const
{
    V1.X = M(0,0);     V1.Y = M(1,0);     V1.Z = M(2,0);
    V2.X = M(0,1);     V2.Y = M(1,1);     V2.Z = M(2,1);
    V3.X = M(0,2);     V3.Y = M(1,2);     V3.Z = M(2,2);
}

//==============================================================================

inline 
void matrix4::GetColumns( vector3& V1, vector3& V2, vector3& V3 ) const
{
    V1.X = M(0,0);     V2.X = M(1,0);     V3.X = M(2,0);
    V1.Y = M(0,1);     V2.Y = M(1,1);     V3.Y = M(2,1);
    V1.Z = M(0,2);     V2.Z = M(1,2);     V3.Z = M(2,2);
}

//==============================================================================

inline 
void matrix4::SetRows( const vector3& V1, 
                       const vector3& V2, 
                       const vector3& V3 )
{
    M(0,0) = V1.X;     M(1,0) = V1.Y;     M(2,0) = V1.Z;
    M(0,1) = V2.X;     M(1,1) = V2.Y;     M(2,1) = V2.Z;
    M(0,2) = V3.X;     M(1,2) = V3.Y;     M(2,2) = V3.Z;
}

//==============================================================================

inline 
void matrix4::SetColumns( const vector3& V1, 
                          const vector3& V2, 
                          const vector3& V3 )
{
    M(0,0) = V1.X;     M(1,0) = V2.X;     M(2,0) = V3.X;
    M(0,1) = V1.Y;     M(1,1) = V2.Y;     M(2,1) = V3.Y;
    M(0,2) = V1.Z;     M(1,2) = V2.Z;     M(2,2) = V3.Z;
}

//==============================================================================

inline 
void matrix4::Setup( const vector3& V1, 
                     const vector3& V2, 
                           radian   Angle )
{
    Setup( V2 - V1, Angle );

    PreTranslate( -V1 );
    Translate   (  V1 );
}

//==============================================================================

inline
void matrix4::Setup( const vector3& Axis, radian Angle )
{
    Setup( quaternion( Axis, Angle ) );
}

//==============================================================================

inline 
vector3 matrix4::operator * ( const vector3& V ) const
{
    return( vector3( (M(0,0)*V.X) + (M(1,0)*V.Y) + (M(2,0)*V.Z) + M(3,0),
                     (M(0,1)*V.X) + (M(1,1)*V.Y) + (M(2,1)*V.Z) + M(3,1),
                     (M(0,2)*V.X) + (M(1,2)*V.Y) + (M(2,2)*V.Z) + M(3,2) ) );
}

//==============================================================================

inline 
vector4 matrix4::operator * ( const vector4& V ) const
{
    return( vector4( (M(0,0)*V.X) + (M(1,0)*V.Y) + (M(2,0)*V.Z) + (M(3,0)*V.W),
                     (M(0,1)*V.X) + (M(1,1)*V.Y) + (M(2,1)*V.Z) + (M(3,1)*V.W),
                     (M(0,2)*V.X) + (M(1,2)*V.Y) + (M(2,2)*V.Z) + (M(3,2)*V.W),
                     (M(0,3)*V.X) + (M(1,3)*V.Y) + (M(2,3)*V.Z) + (M(3,3)*V.W)) );
}

//==============================================================================

inline
vector3 matrix4::Transform( const vector3& V ) const
{
    return( (*this) * V );
}

//==============================================================================

inline
void matrix4::Transform(       vector3* pDest, 
                         const vector3* pSource, 
                               s32      NVerts ) const
{
    ASSERT( pDest   );
    ASSERT( pSource );

    while( NVerts > 0 )
    {
        *pDest = Transform( *pSource );

        pDest++;
        pSource++;
        NVerts--;
    }
}

//==============================================================================

inline
vector4 matrix4::Transform( const vector4& V ) const
{
    return( (*this) * V );
}

//==============================================================================

inline
void matrix4::Transform(       vector4* pDest, 
                         const vector4* pSource, 
                               s32      NVerts ) const
{
    ASSERT( pDest   );
    ASSERT( pSource );

    while( NVerts > 0 )
    {
        *pDest = Transform( *pSource );

        pDest++;
        pSource++;
        NVerts--;
    }
}

//==============================================================================

// Rotates vector by matrix rotation
// (ignores matrix translation, but it will scale the vector if the matrix contains a scale)
inline
vector3 matrix4::RotateVector( const vector3& V ) const
{
    return( vector3( (M(0,0)*V.X) + (M(1,0)*V.Y) + (M(2,0)*V.Z),
                     (M(0,1)*V.X) + (M(1,1)*V.Y) + (M(2,1)*V.Z),
                     (M(0,2)*V.X) + (M(1,2)*V.Y) + (M(2,2)*V.Z) ) );
}

//==============================================================================

// Inverse rotates vector by matrix rotation
// (ignores matrix translation, but it will scale the vector if the matrix contains a scale)
inline
vector3 matrix4::InvRotateVector( const vector3& V ) const
{
    return( vector3( (M(0,0)*V.X) + (M(0,1)*V.Y) + (M(0,2)*V.Z),
                     (M(1,0)*V.X) + (M(1,1)*V.Y) + (M(1,2)*V.Z),
                     (M(2,0)*V.X) + (M(2,1)*V.Y) + (M(2,2)*V.Z) ) );
}

//==============================================================================

inline
const matrix4& matrix4::operator = ( const matrix4& aM )
{
    if( this != &aM )
    {
        const f32* pSrc = (const f32*)aM.m_Cell;
        const f32* pSrcEnd = pSrc+16;
        f32* pDst = (f32*)m_Cell;
        while( pSrc != pSrcEnd )
        {
            *pDst++ = *pSrc++;
        }
    }

    return( *this );
}

//==============================================================================

inline
matrix4& matrix4::operator += ( const matrix4& aM )
{
    M(0,0) += aM.M(0,0);
    M(0,1) += aM.M(0,1);
    M(0,2) += aM.M(0,2);
    M(0,3) += aM.M(0,3);

    M(1,0) += aM.M(1,0);
    M(1,1) += aM.M(1,1);
    M(1,2) += aM.M(1,2);
    M(1,3) += aM.M(1,3);

    M(2,0) += aM.M(2,0);
    M(2,1) += aM.M(2,1);
    M(2,2) += aM.M(2,2);
    M(2,3) += aM.M(2,3);

    M(3,0) += aM.M(3,0);
    M(3,1) += aM.M(3,1);
    M(3,2) += aM.M(3,2);
    M(3,3) += aM.M(3,3);

    return( *this );
}

//==============================================================================

inline
matrix4& matrix4::operator -= ( const matrix4& aM )
{
    M(0,0) -= aM.M(0,0);
    M(0,1) -= aM.M(0,1);
    M(0,2) -= aM.M(0,2);
    M(0,3) -= aM.M(0,3);

    M(1,0) -= aM.M(1,0);
    M(1,1) -= aM.M(1,1);
    M(1,2) -= aM.M(1,2);
    M(1,3) -= aM.M(1,3);

    M(2,0) -= aM.M(2,0);
    M(2,1) -= aM.M(2,1);
    M(2,2) -= aM.M(2,2);
    M(2,3) -= aM.M(2,3);

    M(3,0) -= aM.M(3,0);
    M(3,1) -= aM.M(3,1);
    M(3,2) -= aM.M(3,2);
    M(3,3) -= aM.M(3,3);

    return( *this );
}

//==============================================================================

inline
matrix4& matrix4::operator *= ( const matrix4& aM )
{
    *this = (*this) * aM;
    return( *this );
}

//==============================================================================

inline
matrix4 operator + ( const matrix4& M1, const matrix4& M2 )
{
    matrix4 Result;

    Result.M(0,0) = M1.M(0,0) + M2.M(0,0);
    Result.M(0,1) = M1.M(0,1) + M2.M(0,1);
    Result.M(0,2) = M1.M(0,2) + M2.M(0,2);
    Result.M(0,3) = M1.M(0,3) + M2.M(0,3);
    
    Result.M(1,0) = M1.M(1,0) + M2.M(1,0);
    Result.M(1,1) = M1.M(1,1) + M2.M(1,1);
    Result.M(1,2) = M1.M(1,2) + M2.M(1,2);
    Result.M(1,3) = M1.M(1,3) + M2.M(1,3);
    
    Result.M(2,0) = M1.M(2,0) + M2.M(2,0);
    Result.M(2,1) = M1.M(2,1) + M2.M(2,1);
    Result.M(2,2) = M1.M(2,2) + M2.M(2,2);
    Result.M(2,3) = M1.M(2,3) + M2.M(2,3);
    
    Result.M(3,0) = M1.M(3,0) + M2.M(3,0);
    Result.M(3,1) = M1.M(3,1) + M2.M(3,1);
    Result.M(3,2) = M1.M(3,2) + M2.M(3,2);
    Result.M(3,3) = M1.M(3,3) + M2.M(3,3);

    return( Result );
}

//==============================================================================

inline
matrix4 operator - ( const matrix4& M1, const matrix4& M2 )
{
    matrix4 Result;

    Result.M(0,0) = M1.M(0,0) - M2.M(0,0);
    Result.M(0,1) = M1.M(0,1) - M2.M(0,1);
    Result.M(0,2) = M1.M(0,2) - M2.M(0,2);
    Result.M(0,3) = M1.M(0,3) - M2.M(0,3);
    
    Result.M(1,0) = M1.M(1,0) - M2.M(1,0);
    Result.M(1,1) = M1.M(1,1) - M2.M(1,1);
    Result.M(1,2) = M1.M(1,2) - M2.M(1,2);
    Result.M(1,3) = M1.M(1,3) - M2.M(1,3);
    
    Result.M(2,0) = M1.M(2,0) - M2.M(2,0);
    Result.M(2,1) = M1.M(2,1) - M2.M(2,1);
    Result.M(2,2) = M1.M(2,2) - M2.M(2,2);
    Result.M(2,3) = M1.M(2,3) - M2.M(2,3);
    
    Result.M(3,0) = M1.M(3,0) - M2.M(3,0);
    Result.M(3,1) = M1.M(3,1) - M2.M(3,1);
    Result.M(3,2) = M1.M(3,2) - M2.M(3,2);
    Result.M(3,3) = M1.M(3,3) - M2.M(3,3);

    return( Result );
}

//==============================================================================

inline
f32 matrix4::Difference( const matrix4& M ) const
{
    f32 Diff=0.0f;
    for( s32 i=0; i<16; i++ )
        Diff += (((f32*)this)[i] - ((f32*)(&M))[i])*
                (((f32*)this)[i] - ((f32*)(&M))[i]);
    return Diff;
}

//==============================================================================

inline
xbool matrix4::InRange( f32 Min, f32 Max ) const
{
    for( s32 i=0; i<16; i++ )
    {
        if( (((f32*)this)[i] < Min) || (((f32*)this)[i] > Max) )
            return FALSE;
    }

    return TRUE;
}

//==============================================================================

inline
xbool matrix4::IsValid( void ) const
{
    for( s32 i=0; i<16; i++ )
    {
        if( !x_isvalid( ((f32*)this)[i] ) )
            return FALSE;
    }

    return TRUE;
}

//==============================================================================

inline
radian3 matrix4::GetRotation( void ) const
{
    radian  Pitch, Yaw, Roll;
    f32     s;

    s = M(2,1);

    if( s >  1.0f ) s =  1.0f;
    if( s < -1.0f ) s = -1.0f;

    Pitch = x_asin( -s );

    Yaw  = x_atan2( M(2,0), M(2,2) );
    Roll = x_atan2( M(0,1), M(1,1) );

    if(( Yaw == 0.0f ) && ( Roll == 0.0f ))
    {
        Roll = -s * x_atan2( M(0,2), M(0,0) );
    }
  
    return( radian3( Pitch, Yaw, Roll ) );
}

//==============================================================================

inline
void matrix4::SetRotation( const radian3& Rotation )
{
    f32 sx, cx;
    f32 sy, cy;
    f32 sz, cz;
    f32 sxsz;
    f32 sxcz;

    x_sincos( Rotation.Pitch, sx, cx );
    x_sincos( Rotation.Yaw,   sy, cy );
    x_sincos( Rotation.Roll,  sz, cz );

    sxsz = sx * sz;
    sxcz = sx * cz;

    // Fill out 3x3 rotations.

    M(0,0) = cy*cz + sy*sxsz;   M(0,1) = cx*sz;   M(0,2) = cy*sxsz - sy*cz;
    M(1,0) = sy*sxcz - sz*cy;   M(1,1) = cx*cz;   M(1,2) = sy*sz + sxcz*cy;
    M(2,0) = cx*sy;             M(2,1) = -sx;     M(2,2) = cx*cy;
}

//==============================================================================

inline
void matrix4::SetRotation( const quaternion& Q )
{
    f32 tx  = 2.0f * Q.X;   // 2x
    f32 ty  = 2.0f * Q.Y;   // 2y
    f32 tz  = 2.0f * Q.Z;   // 2z
    f32 txw =   tx * Q.W;   // 2x * w
    f32 tyw =   ty * Q.W;   // 2y * w
    f32 tzw =   tz * Q.W;   // 2z * w
    f32 txx =   tx * Q.X;   // 2x * x
    f32 tyx =   ty * Q.X;   // 2y * x
    f32 tzx =   tz * Q.X;   // 2z * x
    f32 tyy =   ty * Q.Y;   // 2y * y
    f32 tzy =   tz * Q.Y;   // 2z * y
    f32 tzz =   tz * Q.Z;   // 2z * z
                                
    // Fill out 3x3 rotations.

    M(0,0) = 1.0f-(tyy+tzz); M(0,1) = tyx + tzw;      M(0,2) = tzx - tyw;           
    M(1,0) = tyx - tzw;      M(1,1) = 1.0f-(txx+tzz); M(1,2) = tzy + txw;           
    M(2,0) = tzx + tyw;      M(2,1) = tzy - txw;      M(2,2) = 1.0f-(txx+tyy);    
}

//==============================================================================

inline
void matrix4::Setup( const radian3& Rotation )
{                           
    SetRotation( Rotation );

    M(0,3) = 0.0f;
    M(1,3) = 0.0f;
    M(2,3) = 0.0f;
    M(3,3) = 1.0f;
    M(3,2) = 0.0f;
    M(3,1) = 0.0f;
    M(3,0) = 0.0f;
}

//==============================================================================

inline
void matrix4::Setup( const quaternion& Q )
{                           
    SetRotation( Q );

    M(0,3) = 0.0f;
    M(1,3) = 0.0f;
    M(2,3) = 0.0f;
    M(3,3) = 1.0f;
    M(3,2) = 0.0f;
    M(3,1) = 0.0f;
    M(3,0) = 0.0f;
}

//==============================================================================

inline
void matrix4::Setup( const vector3& Scale, 
                     const radian3& Rotation, 
                     const vector3& Translation )
{
    Identity   ( );
    SetRotation( Rotation );
    PreScale   ( Scale );
    Translate  ( Translation );
}

//==============================================================================

inline
void matrix4::Setup( const vector3& Scale, 
                     const quaternion& Rotation,
                     const vector3& Translation )
{
    Identity   ( );
    SetRotation( Rotation );
    PreScale   ( Scale );
    Translate  ( Translation );
}

//==============================================================================
//  CLEAR DEFINES
//==============================================================================

#undef M

//==============================================================================
