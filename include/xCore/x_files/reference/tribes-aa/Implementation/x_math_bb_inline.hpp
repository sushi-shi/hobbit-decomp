//==============================================================================
//
//  x_math_bb_inline.hpp
//
//==============================================================================

#ifndef X_MATH_BB_INLINE_HPP
#define X_MATH_BB_INLINE_HPP
#else
#error "File " __FILE__ " has been included twice!"
#endif

#ifndef X_DEBUG_HPP
#include "../x_debug.hpp"
#endif

//==============================================================================
//  FUNCTIONS
//==============================================================================

inline 
bbox::bbox( void )
{
    Min(  F32_MAX,  F32_MAX,  F32_MAX );
    Max( -F32_MAX, -F32_MAX, -F32_MAX );
}

//==============================================================================

inline
void bbox::Clear( void )
{
    Min(  F32_MAX,  F32_MAX,  F32_MAX );
    Max( -F32_MAX, -F32_MAX, -F32_MAX );
}

//==============================================================================

inline 
bbox::bbox( const bbox& BBox )
{
    Min = BBox.Min;
    Max = BBox.Max;
}

//==============================================================================

inline 
bbox::bbox( const vector3& P1 )
{
    Min = Max = P1;    
}

//==============================================================================

inline 
bbox::bbox( const vector3& P1, const vector3& P2 )
{
    Min( MIN( P1.X, P2.X ), MIN( P1.Y, P2.Y ), MIN( P1.Z, P2.Z ) );
    Max( MAX( P1.X, P2.X ), MAX( P1.Y, P2.Y ), MAX( P1.Z, P2.Z ) );
}

//==============================================================================

inline 
bbox::bbox( const vector3& Center, f32 Radius )
{
    Min = Center - vector3(Radius,Radius,Radius);
    Max = Center + vector3(Radius,Radius,Radius);
}

//==============================================================================

inline 
bbox::bbox( const vector3* pVerts, s32 NVerts )
{
    ASSERT( pVerts );
    ASSERT( NVerts > 0 );

    // Start with first vertex as the bbox.
    Min = Max = *pVerts;
    pVerts++;
    NVerts--;

    // Add in all the others.
    while( NVerts > 0 )
    {
        if( pVerts->X < Min.X )     Min.X = pVerts->X;
        if( pVerts->Y < Min.Y )     Min.Y = pVerts->Y;
        if( pVerts->Z < Min.Z )     Min.Z = pVerts->Z;
        if( pVerts->X > Max.X )     Max.X = pVerts->X;
        if( pVerts->Y > Max.Y )     Max.Y = pVerts->Y;
        if( pVerts->Z > Max.Z )     Max.Z = pVerts->Z;

        pVerts++;
        NVerts--;
    }
}

//==============================================================================

inline 
void bbox::Set( const vector3& Center, f32 Radius )
{
    Min = Center - vector3(Radius,Radius,Radius);
    Max = Center + vector3(Radius,Radius,Radius);
}

//==============================================================================

inline
void bbox::Set( const vector3& P1, const vector3& P2 )
{
    Min( MIN( P1.X, P2.X ), MIN( P1.Y, P2.Y ), MIN( P1.Z, P2.Z ) );
    Max( MAX( P1.X, P2.X ), MAX( P1.Y, P2.Y ), MAX( P1.Z, P2.Z ) );
}

//==============================================================================

inline
void bbox::operator () ( const vector3& P1, const vector3& P2 )
{
    Min( MIN( P1.X, P2.X ), MIN( P1.Y, P2.Y ), MIN( P1.Z, P2.Z ) );
    Max( MAX( P1.X, P2.X ), MAX( P1.Y, P2.Y ), MAX( P1.Z, P2.Z ) );
}

//==============================================================================

inline 
vector3 bbox::GetSize( void ) const
{
    return( Max - Min );
}

//==============================================================================

inline 
vector3 bbox::GetCenter( void ) const
{
    return( (Min + Max) * 0.5f );
}

//==============================================================================

inline 
f32 bbox::GetRadius( void ) const
{
    return( (Max-Min).Length() * 0.5f );
}

//==============================================================================

inline 
f32 bbox::GetRadiusSquared( void ) const
{
    vector3 R = (Max-Min) * 0.5f;
    return( (R.X*R.X) + (R.Y*R.Y) + (R.Z*R.Z) );
}

//==============================================================================

inline 
void bbox::Transform( const matrix4& M )
{
    vector3 AMin, AMax;
    f32     a, b;
    s32     i, j;

    // Copy box A into min and max array.
    AMin = Min;
    AMax = Max;

    // Begin at T.
    Min.X = Max.X = M(3,0);
    Min.Y = Max.Y = M(3,1);
    Min.Z = Max.Z = M(3,2);

    // Find extreme points by considering product of 
    // min and max with each component of M.
    for( j=0; j<3; j++ )
    {
        for( i=0; i<3; i++ )
        {
            a = M(i,j) * AMin[i];
            b = M(i,j) * AMax[i];

            if( a < b )
            {
                Min[j] += a;
                Max[j] += b;
            }
            else
            {
                Min[j] += b;
                Max[j] += a;
            }
        }
    }
}

//==============================================================================

inline 
void bbox::Translate( const vector3& T )
{
    Min += T;
    Max += T;
}

//==============================================================================

inline 
xbool bbox::Intersect( const vector3& Point ) const
{
    return( (Point.X <= Max.X) && 
            (Point.Y <= Max.Y) && 
            (Point.Z <= Max.Z) && 
            (Point.X >= Min.X) && 
            (Point.Y >= Min.Y) && 
            (Point.Z >= Min.Z) );
}

//==============================================================================

inline 
xbool bbox::Intersect( const bbox& BBox ) const
{
    if( BBox.Min.X > Max.X ) return FALSE;
    if( BBox.Max.X < Min.X ) return FALSE;
    if( BBox.Min.Z > Max.Z ) return FALSE;
    if( BBox.Max.Z < Min.Z ) return FALSE;
    if( BBox.Min.Y > Max.Y ) return FALSE;
    if( BBox.Max.Y < Min.Y ) return FALSE;
    return TRUE;
}

//==============================================================================

inline 
xbool bbox::Intersect( const plane& Plane ) const
{
    vector3 PMin, PMax;
    f32     DMax, DMin;

    if( Plane.Normal.X > 0 )  { PMax.X = Max.X;  PMin.X = Min.X; }
    else                      { PMax.X = Min.X;  PMin.X = Max.X; }

    if( Plane.Normal.Y > 0 )  { PMax.Y = Max.Y;  PMin.Y = Min.Y; }
    else                      { PMax.Y = Min.Y;  PMin.Y = Max.Y; }

    if( Plane.Normal.Z > 0 )  { PMax.Z = Max.Z;  PMin.Z = Min.Z; }
    else                      { PMax.Z = Min.Z;  PMin.Z = Max.Z; }

    DMax = Plane.Distance( PMax );
    DMin = Plane.Distance( PMin );

    return( (DMax > 0.0f) && (DMin < 0.0f ) );
}

//==============================================================================

inline 
xbool bbox::Intersect( f32& t, const vector3& P0, const vector3& P1 ) const
{
    f32     PlaneD   [3];
    xbool   PlaneUsed[3] = { TRUE, TRUE, TRUE };
    f32     T        [3] = { -1, -1, -1 };
    vector3 Direction    = P1 - P0;
    s32     MaxPlane;
    s32     i;
    f32     Component;

    // Set a value until we have something better.
    t = 0.0f;

    // Consider relationship of each component of P0 to the box.
    for( i = 0; i < 3; i++ )
    {
        if     ( P0[i] > Max[i] )   { PlaneD[i]    = Max[i]; }
        else if( P0[i] < Min[i] )   { PlaneD[i]    = Min[i]; }
        else                        { PlaneUsed[i] = FALSE;  }
    }
    
    // Is the starting point in the box?
    if( !PlaneUsed[0] && !PlaneUsed[1] && !PlaneUsed[2] )
        return( TRUE );

    // For each plane to be used, compute the distance to the plane.
    for( i = 0; i < 3; i++ )
    {
        if( PlaneUsed[i] && (Direction[i] != 0.0f) )
            T[i] = (PlaneD[i] - P0[i]) / Direction[i];
    }

    // We need to know which plane had the largest distance.
    if( T[0] > T[1] )
    {
        MaxPlane = ((T[0] > T[2]) ? 0 : 2);
    }
    else
    {
        MaxPlane = ((T[1] > T[2]) ? 1 : 2);
    }

    // If the largest plane distance is less than zero, then there is no hit.
    if( T[MaxPlane] < 0.0f )
        return( FALSE );

    // See if the point we think is the hit point is a real hit.
    for( i = 0; i < 3; i++ )
    {
        // See if component 'i' of the hit point is on the box.
        if( i != MaxPlane )
        {
            Component = P0[i] + T[MaxPlane] * Direction[i];
            if( (Component < Min[i]) || (Component > Max[i]) )
            {
                // We missed!  Hit point was not on the box.
                return( FALSE );
            }
        }
    }

    // We have a verified hit.  Set t and we're done.
    t = T[MaxPlane];
    return( TRUE );
}

//==============================================================================

inline 
const bbox& bbox::operator = ( const bbox& BBox )
{
    Min = BBox.Min;
    Max = BBox.Max;

    return( *this );
}

//==============================================================================

inline 
bbox& bbox::operator += ( const bbox& BBox )
{
    if( BBox.Min.X < Min.X )     Min.X = BBox.Min.X;
    if( BBox.Min.Y < Min.Y )     Min.Y = BBox.Min.Y;
    if( BBox.Min.Z < Min.Z )     Min.Z = BBox.Min.Z;
    if( BBox.Max.X > Max.X )     Max.X = BBox.Max.X;
    if( BBox.Max.Y > Max.Y )     Max.Y = BBox.Max.Y;
    if( BBox.Max.Z > Max.Z )     Max.Z = BBox.Max.Z;

    return( *this );
}

//==============================================================================

inline 
bbox& bbox::operator += ( const vector3& Point )
{
    if( Point.X < Min.X )     Min.X = Point.X;
    if( Point.Y < Min.Y )     Min.Y = Point.Y;
    if( Point.Z < Min.Z )     Min.Z = Point.Z;
    if( Point.X > Max.X )     Max.X = Point.X;
    if( Point.Y > Max.Y )     Max.Y = Point.Y;
    if( Point.Z > Max.Z )     Max.Z = Point.Z;

    return( *this );
}

//==============================================================================

inline 
bbox& bbox::AddVerts( const vector3* pVerts, s32 NVerts )
{
    ASSERT( pVerts );
    ASSERT( NVerts > 0 );

    while( NVerts > 0 )
    {
        if( pVerts->X < Min.X )     Min.X = pVerts->X;
        if( pVerts->Y < Min.Y )     Min.Y = pVerts->Y;
        if( pVerts->Z < Min.Z )     Min.Z = pVerts->Z;
        if( pVerts->X > Max.X )     Max.X = pVerts->X;
        if( pVerts->Y > Max.Y )     Max.Y = pVerts->Y;
        if( pVerts->Z > Max.Z )     Max.Z = pVerts->Z;

        pVerts++;
        NVerts--;
    }

    return( *this );
}

//==============================================================================

inline 
bbox operator + ( const bbox& BBox1, const bbox& BBox2 )
{
    bbox Result( BBox1 );
    return( Result += BBox2 );
}

//==============================================================================

inline 
bbox operator + ( const bbox& BBox, const vector3& Point )
{
    bbox Result( BBox );
    return( Result += Point );
}

//==============================================================================

inline 
bbox operator + ( const vector3& Point, const bbox& BBox )
{
    bbox Result( BBox );
    return( Result += Point );
}

//==============================================================================

inline
f32 bbox::Difference( const bbox& B ) const
{
    return Min.Difference(B.Min) + Max.Difference(B.Max);
}

//==============================================================================

inline
xbool bbox::InRange( f32 aMin, f32 aMax ) const
{
    return( (Min.X>=aMin) && (Min.X<=aMax) &&
            (Min.Y>=aMin) && (Min.Y<=aMax) &&
            (Min.Z>=aMin) && (Min.Z<=aMax) &&
            (Max.X>=aMin) && (Max.X<=aMax) &&
            (Max.Y>=aMin) && (Max.Y<=aMax) &&
            (Max.Z>=aMin) && (Max.Z<=aMax) );
}

//==============================================================================

inline
xbool bbox::IsValid( void ) const
{
    return( Min.IsValid() && Max.IsValid() );
}

//==============================================================================


