// Genuine scalar original helper closure; donors and adaptations in docs/imports/shared-math-closure.json.
#ifndef HOBBIT_X_MATH_SCALAR_CLOSURE_HPP
#define HOBBIT_X_MATH_SCALAR_CLOSURE_HPP

inline
void vector3::Negate( void )
{
    X = -X; 
    Y = -Y; 
    Z = -Z;
}

inline
xbool vector3::InRange( f32 aMin, f32 aMax ) const
{
    return( (X>=aMin) && (X<=aMax) &&
            (Y>=aMin) && (Y<=aMax) &&
            (Z>=aMin) && (Z<=aMax) );
}

inline
void vector3::Min( f32 Min )
{
    GetX() = MIN( GetX(), Min );
    GetY() = MIN( GetY(), Min );
    GetZ() = MIN( GetZ(), Min );

}

inline
void vector3::Max( f32 Max )
{
    GetX() = MAX( GetX(), Max );
    GetY() = MAX( GetY(), Max );
    GetZ() = MAX( GetZ(), Max );

}

inline
void vector3::Min( const vector3& V )
{
    GetX() = MIN( GetX(), V.GetX() );
    GetY() = MIN( GetY(), V.GetY() );
    GetZ() = MIN( GetZ(), V.GetZ() );

}

inline
void vector3::Max( const vector3& V )
{
    GetX() = MAX( GetX(), V.GetX() );
    GetY() = MAX( GetY(), V.GetY() );
    GetZ() = MAX( GetZ(), V.GetZ() );

}

inline
void matrix4::Zero( void )
{
    (*this)(0,0) = (*this)(1,0) = (*this)(2,0) = (*this)(3,0) = 0.0f;
    (*this)(0,1) = (*this)(1,1) = (*this)(2,1) = (*this)(3,1) = 0.0f;
    (*this)(0,2) = (*this)(1,2) = (*this)(2,2) = (*this)(3,2) = 0.0f;
    (*this)(0,3) = (*this)(1,3) = (*this)(2,3) = (*this)(3,3) = 0.0f;
}

inline
void matrix4::Orthogonalize( void )
{
    vector3 VX;
    vector3 VY;
    vector3 VZ;    

    VX.X = (*this)(0,0); 
    VX.Y = (*this)(0,1);
    VX.Z = (*this)(0,2);
    
    VY.X = (*this)(1,0); 
    VY.Y = (*this)(1,1);
    VY.Z = (*this)(1,2);

    VX.Normalize();
    VY.Normalize();

    VZ = v3_Cross( VX, VY );
    VY = v3_Cross( VZ, VX );

    SetColumns( VX, VY, VZ );
}

inline
matrix4& matrix4::operator += ( const matrix4& aM )
{
    (*this)(0,0) += aM(0,0);
    (*this)(0,1) += aM(0,1);
    (*this)(0,2) += aM(0,2);
    (*this)(0,3) += aM(0,3);

    (*this)(1,0) += aM(1,0);
    (*this)(1,1) += aM(1,1);
    (*this)(1,2) += aM(1,2);
    (*this)(1,3) += aM(1,3);

    (*this)(2,0) += aM(2,0);
    (*this)(2,1) += aM(2,1);
    (*this)(2,2) += aM(2,2);
    (*this)(2,3) += aM(2,3);

    (*this)(3,0) += aM(3,0);
    (*this)(3,1) += aM(3,1);
    (*this)(3,2) += aM(3,2);
    (*this)(3,3) += aM(3,3);

    return( *this );
}

inline
matrix4& matrix4::operator -= ( const matrix4& aM )
{
    (*this)(0,0) -= aM(0,0);
    (*this)(0,1) -= aM(0,1);
    (*this)(0,2) -= aM(0,2);
    (*this)(0,3) -= aM(0,3);

    (*this)(1,0) -= aM(1,0);
    (*this)(1,1) -= aM(1,1);
    (*this)(1,2) -= aM(1,2);
    (*this)(1,3) -= aM(1,3);

    (*this)(2,0) -= aM(2,0);
    (*this)(2,1) -= aM(2,1);
    (*this)(2,2) -= aM(2,2);
    (*this)(2,3) -= aM(2,3);

    (*this)(3,0) -= aM(3,0);
    (*this)(3,1) -= aM(3,1);
    (*this)(3,2) -= aM(3,2);
    (*this)(3,3) -= aM(3,3);

    return( *this );
}

inline
matrix4 operator + ( const matrix4& M1, const matrix4& M2 )
{
    matrix4 Result;

    Result(0,0) = M1(0,0) + M2(0,0);
    Result(0,1) = M1(0,1) + M2(0,1);
    Result(0,2) = M1(0,2) + M2(0,2);
    Result(0,3) = M1(0,3) + M2(0,3);
    
    Result(1,0) = M1(1,0) + M2(1,0);
    Result(1,1) = M1(1,1) + M2(1,1);
    Result(1,2) = M1(1,2) + M2(1,2);
    Result(1,3) = M1(1,3) + M2(1,3);
    
    Result(2,0) = M1(2,0) + M2(2,0);
    Result(2,1) = M1(2,1) + M2(2,1);
    Result(2,2) = M1(2,2) + M2(2,2);
    Result(2,3) = M1(2,3) + M2(2,3);
    
    Result(3,0) = M1(3,0) + M2(3,0);
    Result(3,1) = M1(3,1) + M2(3,1);
    Result(3,2) = M1(3,2) + M2(3,2);
    Result(3,3) = M1(3,3) + M2(3,3);

    return( Result );
}

inline
matrix4 operator - ( const matrix4& M1, const matrix4& M2 )
{
    matrix4 Result;

    Result(0,0) = M1(0,0) - M2(0,0);
    Result(0,1) = M1(0,1) - M2(0,1);
    Result(0,2) = M1(0,2) - M2(0,2);
    Result(0,3) = M1(0,3) - M2(0,3);
    
    Result(1,0) = M1(1,0) - M2(1,0);
    Result(1,1) = M1(1,1) - M2(1,1);
    Result(1,2) = M1(1,2) - M2(1,2);
    Result(1,3) = M1(1,3) - M2(1,3);
    
    Result(2,0) = M1(2,0) - M2(2,0);
    Result(2,1) = M1(2,1) - M2(2,1);
    Result(2,2) = M1(2,2) - M2(2,2);
    Result(2,3) = M1(2,3) - M2(2,3);
    
    Result(3,0) = M1(3,0) - M2(3,0);
    Result(3,1) = M1(3,1) - M2(3,1);
    Result(3,2) = M1(3,2) - M2(3,2);
    Result(3,3) = M1(3,3) - M2(3,3);

    return( Result );
}

inline
void matrix4::ClearRotation( void )
{

    vector3 S = GetScale();
    vector3 T = GetTranslation();
    Identity();
    SetScale( S );
    SetTranslation( T );
}

inline
matrix4         m4_Transpose        ( const matrix4& M )
{

    matrix4 Mout = M;
    Mout.Transpose();
    return Mout;
}

inline
void plane::GetOrthoVectors ( vector3& AxisA,
                              vector3& AxisB ) const
{
    f32     AbsA, AbsB, AbsC;
    vector3 Dir;

    // Get a non-parallel axis to normal.
    AbsA = x_abs( Normal.X );
    AbsB = x_abs( Normal.Y );
    AbsC = x_abs( Normal.Z );
    if( (AbsA<=AbsB) && (AbsA<=AbsC) ) Dir = vector3(1,0,0);
    else
    if( (AbsB<=AbsA) && (AbsB<=AbsC) ) Dir = vector3(0,1,0);
    else                               Dir = vector3(0,0,1);

    AxisA = Normal.Cross(Dir);
    AxisB = Normal.Cross(AxisA);
    AxisA.Normalize();
    AxisB.Normalize();
}

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

inline
bbox::bbox( const vector3& P1, const vector3& P2, const vector3& P3 )
{

    Min = Max = P1;
    Min.Min( P2 );
    Max.Max( P2 );
    Min.Min( P3 );
    Max.Max( P3 );
}

inline
xbool bbox::IntersectTriBBox( const vector3& P0,
                              const vector3& P1,
                              const vector3& P2 )  const
{

    // Compute triangle bbox
    bbox TriBBox( P0, P1, P2 );
    
    // Call bbox intersect function
    return Intersect( TriBBox );
}

inline
void irect::SetWidth( s32 W )
{
    r = l + W;
}

inline
void irect::SetHeight( s32 H )
{
    b = t + H;
}

inline
void irect::SetSize( s32 W, s32 H )
{
    r = l + W;
    b = t + H;
}

inline
f32 x_round( f32 a, f32 b )
{
    f32 Quotient;

    ASSERT( !IN_RANGE( -0.00001f, b, 0.00001f ) );

    Quotient = a / b;

    if( Quotient < 0.0f )   return( x_ceil ( Quotient - 0.5f ) * b );
    else                    return( x_floor( Quotient + 0.5f ) * b );
}

inline f32 x_log2  ( f32 a )        { return( 1.442695041f * (f32)(log  ( (f64)a ) ) ); }

inline
plane::plane( const vector3& P1, const vector3& P2, const vector3& P3 )
{
    Setup( P1, P2, P3 );
}

inline
matrix4 m4_InvertRT( const matrix4& Src )
{

    matrix4 Dest;

    Dest(0,0) = Src(0,0);
    Dest(0,1) = Src(1,0);
    Dest(0,2) = Src(2,0);
    Dest(1,0) = Src(0,1);
    Dest(1,1) = Src(1,1);
    Dest(1,2) = Src(2,1);
    Dest(2,0) = Src(0,2);
    Dest(2,1) = Src(1,2);
    Dest(2,2) = Src(2,2);
    Dest(0,3) = 0.0f;
    Dest(1,3) = 0.0f;
    Dest(2,3) = 0.0f;
    Dest(3,3) = 1.0f;
    Dest(3,0) = -(Src(3,0)*Dest(0,0) + Src(3,1)*Dest(1,0) + Src(3,2)*Dest(2,0));
    Dest(3,1) = -(Src(3,0)*Dest(0,1) + Src(3,1)*Dest(1,1) + Src(3,2)*Dest(2,1));
    Dest(3,2) = -(Src(3,0)*Dest(0,2) + Src(3,1)*Dest(1,2) + Src(3,2)*Dest(2,2));


    return Dest;
}

#endif
