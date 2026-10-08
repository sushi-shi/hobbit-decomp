//==============================================================================
//
//  x_math_v3_inline.hpp
//
//==============================================================================

#ifndef X_MATH_V3_INLINE_HPP
#define X_MATH_V3_INLINE_HPP
#else
#error "File " __FILE__ " has been included twice!"
#endif

#ifndef X_DEBUG_HPP
#include "../x_debug.hpp"
#endif

//==============================================================================

inline 
vector3::vector3( void )
{
}

//==============================================================================

inline 
vector3::vector3( const vector3& V )
{
    X = V.X; 
    Y = V.Y; 
    Z = V.Z;
}

//==============================================================================

inline 
vector3::vector3( f32 aX, f32 aY, f32 aZ )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
}

//==============================================================================

inline
vector3::vector3( radian Pitch, radian Yaw )
{
    Set( Pitch, Yaw );
}

//==============================================================================

inline
vector3::vector3( const radian3& R )
{
    Set( R );
}

//==============================================================================

inline
void vector3::Set( radian Pitch, radian Yaw )
{
    f32 PS, PC;
    f32 YS, YC;

    x_sincos( Pitch, PS, PC );
    x_sincos( Yaw,   YS, YC );

    X =  (YS * PC);
    Y = -(PS);
    Z =  (YC * PC);
}

//==============================================================================

inline
void vector3::Set( const radian3& R )
{
    X = 0.0f;
    Y = 0.0f;
    Z = 1.0f;
    Rotate(R);
}

//==============================================================================

inline 
void vector3::Set( f32 aX, f32 aY, f32 aZ )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
}

//==============================================================================

inline 
void vector3::operator () ( f32 aX, f32 aY, f32 aZ )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
}

//==============================================================================

inline
f32 vector3::operator [] ( s32 Index ) const
{
    ASSERT( (Index >= 0) && (Index < 3) );
    return( ((f32*)this)[Index] );
}

//==============================================================================

inline
f32& vector3::operator [] ( s32 Index )
{
    ASSERT( (Index >= 0) && (Index < 3) );
    return( ((f32*)this)[Index] );
}

//==============================================================================

inline 
void vector3::Zero( void )
{
    X = Y = Z = 0.0f;
}

//==============================================================================

inline 
void vector3::Negate( void )
{
    X = -X; 
    Y = -Y; 
    Z = -Z;
}

//==============================================================================

inline 
xbool vector3::Normalize( void )
{
    f32 N = x_1sqrt(X*X + Y*Y + Z*Z);

    if( x_isvalid(N) )
    {
        X *= N;
        Y *= N;
        Z *= N;
        return TRUE;
    }
    else
    {
        X = 0;
        Y = 0;
        Z = 0;
        return FALSE;
    }
}

//==============================================================================

inline 
xbool vector3::NormalizeAndScale( f32 Scalar )
{
    f32 N = x_1sqrt(X*X + Y*Y + Z*Z);

    if( x_isvalid(N) )
    {
        N *= Scalar;
        X *= N;
        Y *= N;
        Z *= N;
        return TRUE;
    }
    else
    {
        X = 0;
        Y = 0;
        Z = 0;
        return FALSE;
    }
}

//==============================================================================

inline 
void vector3::Scale( f32 Scalar )
{
    ASSERT( x_isvalid( Scalar ) );

    X *= Scalar; 
    Y *= Scalar; 
    Z *= Scalar;
}

//==============================================================================

inline 
f32 vector3::Length( void ) const
{
    return( x_sqrt( X*X + Y*Y + Z*Z ) );
}

//==============================================================================

inline
f32 vector3::LengthSquared( void ) const
{
    return( X*X + Y*Y + Z*Z );
}

//==============================================================================

inline 
void vector3::Rotate( const radian3& R )
{
    RotateZ( R.Roll );
    RotateX( R.Pitch );
    RotateY( R.Yaw );
}

//==============================================================================

inline 
void vector3::RotateX( radian Angle )
{
    radian S, C;
    f32    y, z;

    x_sincos( Angle, S, C );

    y = Y;
    z = Z;
    Y = (C * y) - (S * z);
    Z = (C * z) + (S * y);
}

//==============================================================================

inline 
void vector3::RotateY( radian Angle )
{
    radian S, C;
    f32    x, z;

    x_sincos( Angle, S, C );

    x = X;
    z = Z;
    X = (C * x) + (S * z);
    Z = (C * z) - (S * x);
}

//==============================================================================

inline 
void vector3::RotateZ( radian Angle )
{
    radian S, C;
    f32    x, y;

    x_sincos( Angle, S, C );

    x = X;
    y = Y;
    X = (C * x) - (S * y);
    Y = (C * y) + (S * x);
}

//==============================================================================

inline 
f32 vector3::Dot( const vector3& V ) const
{
    return( (X * V.X) + (Y * V.Y) + (Z * V.Z) );
}

//==============================================================================

inline 
vector3 vector3::Cross( const vector3& V ) const
{
    return( vector3( (Y * V.Z) - (Z * V.Y),
                     (Z * V.X) - (X * V.Z),
                     (X * V.Y) - (Y * V.X) ) );
}

//==============================================================================

inline 
f32 v3_Dot( const vector3& V1, const vector3& V2 )
{
    return( (V1.X * V2.X) + (V1.Y * V2.Y) + (V1.Z * V2.Z) );
}

//==============================================================================

inline 
vector3 v3_Cross( const vector3& V1, const vector3& V2 )
{
    return( vector3( (V1.Y * V2.Z) - (V1.Z * V2.Y),
                     (V1.Z * V2.X) - (V1.X * V2.Z),
                     (V1.X * V2.Y) - (V1.Y * V2.X) ) );
}

//==============================================================================

inline 
radian vector3::GetPitch( void ) const
{
    f32 L = (f32)x_sqrt( X*X + Z*Z );
    return( -x_atan2( Y, L ) );
}

//==============================================================================

inline 
radian vector3::GetYaw( void ) const
{
    return( x_atan2( X, Z ) );
}

//==============================================================================

inline 
void vector3::GetPitchYaw( radian& Pitch, radian& Yaw ) const
{
    Pitch = GetPitch();
    Yaw   = GetYaw();
}

//==============================================================================
//           * Start
//           |
//           <--------------(* this )
//           | Return Vector
//           |
//           |
//           * End
//
// Such: 
//
// this.GetClosestVToLSeg(a,b).LengthSquared(); // Is the length square to the line segment
// this.GetClosestVToLSeg(a,b) + this;          // Is the closest point in to the line segment
//
//==============================================================================
inline 
vector3 vector3::GetClosestVToLSeg( const vector3& Start, const vector3& End ) const
{
    vector3 Diff = *this - Start;
    vector3 Dir  = End   - Start;
    f32     T    = Diff.Dot( Dir );

    if( T > 0.0 )
    {
        f32 SqrLen = Dir.Dot( Dir );

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

//==============================================================================

inline
f32 vector3::GetSqrtDistToLineSeg( const vector3& Start, const vector3& End ) const
{
    return GetClosestVToLSeg(Start,End).LengthSquared();
}

//==============================================================================

inline
vector3 vector3::GetClosestPToLSeg( const vector3& Start, const vector3& End ) const
{
    return GetClosestVToLSeg(Start,End) + *this;
}

//==============================================================================

inline
vector3::operator const f32* ( void ) const
{
    return( &X );
}

//==============================================================================

inline 
vector3 vector3::operator - ( void ) const
{
    return( vector3( -X, -Y, -Z ) );
}

//==============================================================================

inline 
const vector3& vector3::operator = ( const vector3& V )
{
    X = V.X; 
    Y = V.Y; 
    Z = V.Z;
    return( *this );
}

//==============================================================================

inline 
vector3& vector3::operator += ( const vector3& V )
{
    X += V.X; 
    Y += V.Y; 
    Z += V.Z;
    return( *this );
}

//==============================================================================

inline 
vector3& vector3::operator -= ( const vector3& V )
{
    X -= V.X; 
    Y -= V.Y; 
    Z -= V.Z;
    return( *this );
}

//==============================================================================

inline 
vector3& vector3::operator *= ( const f32 Scalar )
{
    X *= Scalar; 
    Y *= Scalar; 
    Z *= Scalar;
    return( *this );
}

//==============================================================================

inline 
vector3& vector3::operator /= ( const f32 Scalar )
{
    ASSERT( Scalar != 0.0f );
    f32 d = 1.0f / Scalar;
    X *= d; 
    Y *= d; 
    Z *= d;
    return( *this );
}

//==============================================================================

inline 
xbool vector3::operator == ( const vector3& V ) const
{
    return( (X == V.X) && (Y == V.Y) && (Z == V.Z) );
}

//==============================================================================

inline 
xbool vector3::operator != ( const vector3& V ) const
{
    return( (X != V.X) || (Y != V.Y) || (Z != V.Z) );
}

//==============================================================================

inline 
vector3 operator + ( const vector3& V1, const vector3& V2 )
{
    return( vector3( V1.X + V2.X, 
                     V1.Y + V2.Y, 
                     V1.Z + V2.Z ) );
}

//==============================================================================

inline 
vector3 operator - ( const vector3& V1, const vector3& V2 )
{
    return( vector3( V1.X - V2.X, 
                     V1.Y - V2.Y, 
                     V1.Z - V2.Z ) );
}

//==============================================================================

inline 
vector3 operator / ( const vector3& V, f32 S )
{
    ASSERT( S != 0.0f );
    S = 1.0f / S;
    return( vector3( V.X * S, 
                     V.Y * S, 
                     V.Z * S ) );
}

//==============================================================================

inline 
vector3 operator * ( const vector3& V, f32 S )
{
    return( vector3( V.X * S, 
                     V.Y * S, 
                     V.Z * S ) );
}

//==============================================================================

inline 
vector3 operator * ( f32 S, const vector3& V )
{
    return( vector3( V.X * S, 
                     V.Y * S, 
                     V.Z * S ) );
}

//==============================================================================

inline
radian v3_AngleBetween ( const vector3& V1, const vector3& V2 )
{
    f32 D, Cos;
    
    D = V1.Length() * V2.Length();
    
    if( D == 0.0f )  return( R_0 );
    
    Cos = v3_Dot( V1, V2 ) / D;
    
    if     ( Cos >  1.0f )  Cos =  1.0f;
    else if( Cos < -1.0f )  Cos = -1.0f;
    
    return( x_acos( Cos ) );
}

//==============================================================================

inline
void  vector3::GetRotationTowards( const vector3& DestV,
                                         vector3& RotAxis, 
                                         radian&  RotAngle ) const
{
    // Get lengths of vectors
    f32 D = this->Length() * DestV.Length();
    
    // vectors are zero length
    if( D == 0.0f )  
    {
        RotAxis = vector3(1,0,0);
        RotAngle = 0;
        return;
    }
    
    // Get unit dot product of the vectors
    f32 Dot = this->Dot(DestV) / D;
    
    if     ( Dot >  1.0f )  Dot =  1.0f;
    else if( Dot < -1.0f )  Dot = -1.0f;
    
    // Get axis to rotate about
    RotAxis = v3_Cross( *this, DestV );

    if( RotAxis.Normalize() )
    {
        RotAngle = x_acos(Dot);
    }
    else
    {
        RotAxis = vector3(1,0,0);

        if( Dot > 0 )
            RotAngle = 0;       // Facing same direction
        else
            RotAngle = R_180;   // Facing opposite directions
    }
}

//==============================================================================

inline
f32 vector3::Difference( const vector3& V ) const
{
    return (*this - V).LengthSquared();
}

//==============================================================================

inline
xbool vector3::InRange( f32 aMin, f32 aMax ) const
{
    return( (X>=aMin) && (X<=aMax) &&
            (Y>=aMin) && (Y<=aMax) &&
            (Z>=aMin) && (Z<=aMax) );
}

//==============================================================================

inline
xbool vector3::IsValid( void ) const
{
    return( x_isvalid(X) && x_isvalid(Y) && x_isvalid(Z) );
}

//==============================================================================
