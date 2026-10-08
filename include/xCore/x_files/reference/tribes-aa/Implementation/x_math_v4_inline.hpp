//==============================================================================
//
//  x_math_v4_inline.hpp
//
//==============================================================================

#ifndef X_MATH_V4_INLINE_HPP
#define X_MATH_V4_INLINE_HPP
#else
#error "File " __FILE__ " has been included twice!"
#endif

#ifndef X_DEBUG_HPP
#include "../x_debug.hpp"
#endif

//==============================================================================

inline 
vector4::vector4( void )
{
}

//==============================================================================

inline 
vector4::vector4( const vector4& V )
{
    X = V.X; 
    Y = V.Y; 
    Z = V.Z;
    W = V.W;
}

//==============================================================================

inline 
vector4::vector4( f32 aX, f32 aY, f32 aZ, f32 aW )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
    W = aW;
}

//==============================================================================

inline 
void vector4::Set( f32 aX, f32 aY, f32 aZ, f32 aW )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
    W = aW;
}

//==============================================================================

inline 
void vector4::operator () ( f32 aX, f32 aY, f32 aZ, f32 aW )
{
    X = aX; 
    Y = aY; 
    Z = aZ;
    W = aW;
}

//==============================================================================

inline
f32 vector4::operator [] ( s32 Index ) const
{
    ASSERT( (Index >= 0) && (Index < 4) );
    return( ((f32*)this)[Index] );
}

//==============================================================================

inline
f32& vector4::operator [] ( s32 Index )
{
    ASSERT( (Index >= 0) && (Index < 4) );
    return( ((f32*)this)[Index] );
}

//==============================================================================

inline 
void vector4::Zero( void )
{
    X = Y = Z = W = 0.0f;
}

//==============================================================================

inline 
void vector4::Negate( void )
{
    X = -X; 
    Y = -Y; 
    Z = -Z;
    W = -W;
}

//==============================================================================

inline 
xbool vector4::Normalize( void )
{
    f32 N = x_1sqrt(X*X + Y*Y + Z*Z + W*W);

    if( x_isvalid(N) )
    {
        X *= N;
        Y *= N;
        Z *= N;
        W *= N;
        return TRUE;
    }
    else
    {
        X = 0;
        Y = 0;
        Z = 0;
        W = 0;
        return FALSE;
    }
}

//==============================================================================

inline 
xbool vector4::NormalizeAndScale( f32 Scalar )
{
    f32 N = x_1sqrt(X*X + Y*Y + Z*Z + W*W);

    if( x_isvalid(N) )
    {
        N *= Scalar;
        X *= N;
        Y *= N;
        Z *= N;
        W *= N;
        return TRUE;
    }
    else
    {
        X = 0;
        Y = 0;
        Z = 0;
        W = 0;
        return FALSE;
    }
}

//==============================================================================

inline 
void vector4::Scale( f32 Scalar )
{
    X *= Scalar; 
    Y *= Scalar; 
    Z *= Scalar;
    W *= Scalar;
}

//==============================================================================

inline 
f32 vector4::Length( void ) const
{
    return( x_sqrt( X*X + Y*Y + Z*Z + W*W ) );
}

//==============================================================================

inline
f32 vector4::LengthSquared( void ) const
{
    return( X*X + Y*Y + Z*Z + W*W );
}

//==============================================================================

inline
vector4::operator const f32* ( void ) const
{
    return( &X );
}

//==============================================================================

inline 
vector4 vector4::operator - ( void ) const
{
    return( vector4( -X, -Y, -Z, -W ) );
}

//==============================================================================

inline 
const vector4& vector4::operator = ( const vector4& V )
{
    X = V.X; 
    Y = V.Y; 
    Z = V.Z;
    W = V.W;
    return( *this );
}

//==============================================================================

inline 
vector4& vector4::operator += ( const vector4& V )
{
    X += V.X; 
    Y += V.Y; 
    Z += V.Z;
    W += V.W;
    return( *this );
}

//==============================================================================

inline 
vector4& vector4::operator -= ( const vector4& V )
{
    X -= V.X; 
    Y -= V.Y; 
    Z -= V.Z;
    W -= V.W;
    return( *this );
}

//==============================================================================

inline 
vector4& vector4::operator *= ( const f32 Scalar )
{
    X *= Scalar; 
    Y *= Scalar; 
    Z *= Scalar;
    W *= Scalar;
    return( *this );
}

//==============================================================================

inline 
vector4& vector4::operator /= ( const f32 Scalar )
{
    ASSERT( Scalar != 0.0f );
    f32 d = 1.0f / Scalar;
    X *= d; 
    Y *= d; 
    Z *= d;
    W *= d;
    return( *this );
}

//==============================================================================

inline 
xbool vector4::operator == ( const vector4& V ) const
{
    return( (X == V.X) && (Y == V.Y) && (Z == V.Z) && (W == V.W) );
}

//==============================================================================

inline 
xbool vector4::operator != ( const vector4& V ) const
{
    return( (X != V.X) || (Y != V.Y) || (Z != V.Z) || (W != V.W) );
}

//==============================================================================

inline 
vector4 operator + ( const vector4& V1, const vector4& V2 )
{
    return( vector4( V1.X + V2.X, 
                     V1.Y + V2.Y, 
                     V1.Z + V2.Z, 
                     V1.W + V2.W ) );
}

//==============================================================================

inline 
vector4 operator - ( const vector4& V1, const vector4& V2 )
{
    return( vector4( V1.X - V2.X, 
                     V1.Y - V2.Y, 
                     V1.Z - V2.Z, 
                     V1.W - V2.W ) );
}

//==============================================================================

inline 
vector4 operator / ( const vector4& V, f32 S )
{
    ASSERT( S != 0.0f );
    S = 1.0f / S;
    return( vector4( V.X * S, 
                     V.Y * S, 
                     V.Z * S, 
                     V.W * S ) );
}

//==============================================================================

inline 
vector4 operator * ( const vector4& V, f32 S )
{
    return( vector4( V.X * S, 
                     V.Y * S, 
                     V.Z * S, 
                     V.W * S ) );
}

//==============================================================================

inline 
vector4 operator * ( f32 S, const vector4& V )
{
    return( vector4( V.X * S, 
                     V.Y * S, 
                     V.Z * S, 
                     V.W * S ) );
}

//==============================================================================

inline
f32 vector4::Difference( const vector4& V ) const
{
    return (*this - V).LengthSquared();
}

//==============================================================================

inline
xbool vector4::InRange( f32 aMin, f32 aMax ) const
{
    return( (X>=aMin) && (X<=aMax) &&
            (Y>=aMin) && (Y<=aMax) &&
            (Z>=aMin) && (Z<=aMax) &&
            (W>=aMin) && (W<=aMax) );
}

//==============================================================================

inline
xbool vector4::IsValid( void ) const
{
    return( x_isvalid(X) && x_isvalid(Y) && x_isvalid(Z) && x_isvalid(W) );
}

//==============================================================================

