//==============================================================================
//
//  x_math.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_MATH_HPP
#include "..\x_math.hpp"
#endif

//==============================================================================
//  DEFINES
//==============================================================================

#define M(row,column)   m_Cell[row][column]


//==============================================================================

xbool x_isvalid( f32 a )
{
    return( ( (*((u32*)(&a))) & 0x7F800000 ) != 0x7F800000 );
/*
    if( a==0 ) 
        return TRUE;

    if( (a>=F32_MIN) && (a<=F32_MAX) ) 
        return TRUE;

    if( (a<=-F32_MIN) && (a>=-F32_MAX) ) 
        return TRUE;

    return FALSE;
*/
}

//==============================================================================
//  FUNCTIONS
//==============================================================================

quaternion BlendSlow( const quaternion& Q0, 
                  const quaternion& Q1, f32 T )
{
    quaternion Q;
    f32 Cs,Sn;
    f32 Angle,InvSn,TAngle;
    f32 C0,C1;
    f32 x0,y0,z0,w0;

    // Determine if quats are further than 90 degrees
    Cs = Q0.X*Q1.X + Q0.Y*Q1.Y + Q0.Z*Q1.Z + Q0.W*Q1.W;

    // If dot is negative flip one of the quaterions
    if( Cs < 0.0f )
    {
       x0 = -Q0.X;
       y0 = -Q0.Y;
       z0 = -Q0.Z;
       w0 = -Q0.W;
       Cs = -Cs;
    }
    else
    {
       x0 = +Q0.X;
       y0 = +Q0.Y;
       z0 = +Q0.Z;
       w0 = +Q0.W;
    }

    // Compute sine of angle between Q0,Q1
    Sn = 1.0f - Cs*Cs;
    if( Sn < 0.0f ) Sn = -Sn;
    Sn = x_sqrt( Sn );

    // Check if quaternions are very close together
    if( (Sn < 1e-3) && (Sn > -1e-3) )
    {
        return Q0;
    }

    Angle   = x_atan2( Sn, Cs );
    InvSn   = 1.0f/Sn;    
    TAngle  = T*Angle;

    C0      = x_sin( Angle - TAngle) * InvSn;
    C1      = x_sin( TAngle ) * InvSn;    

    Q.X     = C0*x0 + C1*Q1.X;
    Q.Y     = C0*y0 + C1*Q1.Y;
    Q.Z     = C0*z0 + C1*Q1.Z;
    Q.W     = C0*w0 + C1*Q1.W;

    return Q;
}

//==============================================================================

quaternion BlendToIdentitySlow( const quaternion& Q0, f32 T )
{
    quaternion Q;
    f32 Cs,Sn;
    f32 Angle,InvSn,TAngle;
    f32 C0,C1;
    f32 x0,y0,z0,w0;

    // Determine if quats are further than 90 degrees
    Cs = Q0.W;

    // If dot is negative flip one of the quaterions
    if( Cs < 0.0f )
    {
       x0 = -Q0.X;
       y0 = -Q0.Y;
       z0 = -Q0.Z;
       w0 = -Q0.W;
       Cs = -Cs;
    }
    else
    {
       x0 = +Q0.X;
       y0 = +Q0.Y;
       z0 = +Q0.Z;
       w0 = +Q0.W;
    }

    // Compute sine of angle between Q0,Q1
    Sn = 1.0f - Cs*Cs;
    if( Sn < 0.0f ) Sn = -Sn;
    Sn = x_sqrt( Sn );

    // Check if quaternions are very close together
    if( (Sn < 1e-3) && (Sn > -1e-3) )
    {
        return Q0;
    }

    Angle   = x_atan2( Sn, Cs );
    InvSn   = 1.0f/Sn;    
    TAngle  = T*Angle;

    C0      = x_sin( Angle - TAngle) * InvSn;
    C1      = x_sin( TAngle ) * InvSn;    

    Q.X     = C0*x0;
    Q.Y     = C0*y0;
    Q.Z     = C0*z0;
    Q.W     = C0*w0 + C1;

    return Q;
}

//==============================================================================

quaternion Blend( const quaternion& Q0, 
                      const quaternion& Q1, f32 T )
{
    f32 Dot;
    f32 LenSquared;
    f32 OneOverL;
    f32 x0,y0,z0,w0;

    // Determine if quats are further than 90 degrees
    Dot = Q0.X*Q1.X + Q0.Y*Q1.Y + Q0.Z*Q1.Z + Q0.W*Q1.W;

    // If dot is negative flip one of the quaterions
    if( Dot < 0.0f )
    {
       x0 = -Q0.X;
       y0 = -Q0.Y;
       z0 = -Q0.Z;
       w0 = -Q0.W;
    }
    else
    {
       x0 = +Q0.X;
       y0 = +Q0.Y;
       z0 = +Q0.Z;
       w0 = +Q0.W;
    }

    // Compute interpolated values
    x0 = x0 + T*(Q1.X - x0);
    y0 = y0 + T*(Q1.Y - y0);
    z0 = z0 + T*(Q1.Z - z0);
    w0 = w0 + T*(Q1.W - w0);

    // Get squared length of new quaternion
    LenSquared = x0*x0 + y0*y0 + z0*z0 + w0*w0;

    // Use home-baked polynomial to compute 1/sqrt(LenSquared)
    // Input range is 0.5 <-> 1.0
    // Ouput range is 1.414213 <-> 1.0

    if( LenSquared<0.857f )
        OneOverL = (((0.699368f)*LenSquared) + -1.819985f)*LenSquared + 2.126369f;    //0.0000792
    else
        OneOverL = (((0.454012f)*LenSquared) + -1.403517f)*LenSquared + 1.949542f;    //0.0000373

    // Renormalize and return quaternion
    return quaternion( x0*OneOverL, y0*OneOverL, z0*OneOverL, w0*OneOverL );
}

//==============================================================================

quaternion BlendToIdentity( const quaternion& Q0, f32 T )
{
    f32 LenSquared;
    f32 OneOverL;
    f32 x0,y0,z0,w0;

    // If dot is negative flip one of the quaterions
    if( Q0.W < 0.0f )
    {
       x0 = -Q0.X;
       y0 = -Q0.Y;
       z0 = -Q0.Z;
       w0 = -Q0.W;
    }
    else
    {
       x0 = +Q0.X;
       y0 = +Q0.Y;
       z0 = +Q0.Z;
       w0 = +Q0.W;
    }

    // Compute interpolated values
    x0 = x0 + T*(0.0f - x0);
    y0 = y0 + T*(0.0f - y0);
    z0 = z0 + T*(0.0f - z0);
    w0 = w0 + T*(1.0f - w0);

    // Get squared length of new quaternion
    LenSquared = x0*x0 + y0*y0 + z0*z0 + w0*w0;

    // Use home-baked polynomial to compute 1/sqrt(LenSquared)
    // Input range is 0.5 <-> 1.0
    // Ouput range is 1.414213 <-> 1.0

    if( LenSquared<0.857f )
        OneOverL = (((0.699368f)*LenSquared) + -1.819985f)*LenSquared + 2.126369f;    //0.0000792
    else
        OneOverL = (((0.454012f)*LenSquared) + -1.403517f)*LenSquared + 1.949542f;    //0.0000373

    // Renormalize and return quaternion
    return quaternion( x0*OneOverL, y0*OneOverL, z0*OneOverL, w0*OneOverL );
}

//==============================================================================

xbool matrix4::Invert( void )
{
    f32 Scratch[4][8];
    f32 a;
    s32 i, j, k, jr, Pivot;
    s32 Row[4];    

    //
    // Initialize.
    //

    for( j = 0; j < 4; j++ )
    {
        for( k = 0; k < 4; k++ )
        {
            Scratch[j][k]   = M(j,k);
            Scratch[j][4+k] = 0.0f;
        }

        Scratch[j][4+j] = 1.0f;
        Row[j] = j;
    }

    //
    // Eliminate columns.
    //

    for( i = 0; i < 4; i++ )
    {
        // Find pivot.
        k = i;
        
        a = x_abs( Scratch[Row[k]][k] );
        
        for( j = i+1; j < 4; j++ )
        {
            jr = Row[j];

            if( a < x_abs( Scratch[jr][i] ) )
            {
                k = j;
                a = x_abs( Scratch[jr][i] );
            }
        }

        // Swap the pivot row (Row[k]) with the i'th row.
        Pivot  = Row[k];
        Row[k] = Row[i];
        Row[i] = Pivot;

        // Normalize pivot row.
        a = Scratch[Pivot][i];

        if( a == 0.0f ) 
            return( FALSE );

        Scratch[Pivot][i] = 1.0f;

        for( k = i+1; k < 8; k++ ) 
            Scratch[Pivot][k] /= a;

        // Eliminate pivot from all remaining rows.
        for( j = i+1; j < 4; j++ )
        {
            jr = Row[j];
            a  = -Scratch[jr][i];
            
            if( a == 0.0f ) 
                continue;

            Scratch[jr][i] = 0.0f;

            for( k = i+1; k < 8; k++ )
                Scratch[jr][k] += (a * Scratch[Pivot][k]);
        }
    }

    //
    // Back solve.
    //

    for( i = 3; i > 0; i-- )
    {
        Pivot = Row[i];
        for( j = i-1; j >= 0; j-- )
        {
            jr = Row[j];
            a  = Scratch[jr][i];

            for( k = i; k < 8; k++ )
                Scratch[jr][k] -= (a * Scratch[Pivot][k]);
        }
    }

    //
    // Copy inverse back into the matrix.
    //

    for( j = 0; j < 4; j++ )
    {
        jr = Row[j];
        for( k = 0; k < 4; k++ )
        {
            M(j,k) = Scratch[jr][k+4];
        }
    }

    // Success!
    return( TRUE );
}

//==============================================================================

xbool matrix4::InvertSRT( void )
{
    matrix4 Src = *this;
    f32     Determinant;

    //
    // Calculate the determinant.
    //

    Determinant = ( Src.M(0,0) * ( Src.M(1,1) * Src.M(2,2) - Src.M(1,2) * Src.M(2,1) ) -
                    Src.M(0,1) * ( Src.M(1,0) * Src.M(2,2) - Src.M(1,2) * Src.M(2,0) ) +
                    Src.M(0,2) * ( Src.M(1,0) * Src.M(2,1) - Src.M(1,1) * Src.M(2,0) ) );

    if( x_abs( Determinant ) < 0.00001f ) 
        return( FALSE );

    Determinant = 1.0f / Determinant;

    //
    // Find the inverse of the matrix.
    //

    M(0,0) =  Determinant * ( Src.M(1,1) * Src.M(2,2) - Src.M(1,2) * Src.M(2,1) );
    M(0,1) = -Determinant * ( Src.M(0,1) * Src.M(2,2) - Src.M(0,2) * Src.M(2,1) );
    M(0,2) =  Determinant * ( Src.M(0,1) * Src.M(1,2) - Src.M(0,2) * Src.M(1,1) );
    M(0,3) = 0.0f;

    M(1,0) = -Determinant * ( Src.M(1,0) * Src.M(2,2) - Src.M(1,2) * Src.M(2,0) );
    M(1,1) =  Determinant * ( Src.M(0,0) * Src.M(2,2) - Src.M(0,2) * Src.M(2,0) );
    M(1,2) = -Determinant * ( Src.M(0,0) * Src.M(1,2) - Src.M(0,2) * Src.M(1,0) );
    M(1,3) = 0.0f;

    M(2,0) =  Determinant * ( Src.M(1,0) * Src.M(2,1) - Src.M(1,1) * Src.M(2,0) );
    M(2,1) = -Determinant * ( Src.M(0,0) * Src.M(2,1) - Src.M(0,1) * Src.M(2,0) );
    M(2,2) =  Determinant * ( Src.M(0,0) * Src.M(1,1) - Src.M(0,1) * Src.M(1,0) );
    M(2,3) = 0.0f;

    M(3,0) = -( Src.M(3,0) * M(0,0) + Src.M(3,1) * M(1,0) + Src.M(3,2) * M(2,0) );
    M(3,1) = -( Src.M(3,0) * M(0,1) + Src.M(3,1) * M(1,1) + Src.M(3,2) * M(2,1) );
    M(3,2) = -( Src.M(3,0) * M(0,2) + Src.M(3,1) * M(1,2) + Src.M(3,2) * M(2,2) );
    M(3,3) = 1.0f;
              
    // Success!
    return( TRUE );
}

//==============================================================================

matrix4 operator * ( const matrix4& L, const matrix4& R )
{
    matrix4 Result;
    for( s32 i=0; i<4; i++ )
    {
        Result.M(i,0) = (L.M(0,0)*R.M(i,0)) + (L.M(1,0)*R.M(i,1)) + (L.M(2,0)*R.M(i,2)) + (L.M(3,0)*R.M(i,3));
        Result.M(i,1) = (L.M(0,1)*R.M(i,0)) + (L.M(1,1)*R.M(i,1)) + (L.M(2,1)*R.M(i,2)) + (L.M(3,1)*R.M(i,3));
        Result.M(i,2) = (L.M(0,2)*R.M(i,0)) + (L.M(1,2)*R.M(i,1)) + (L.M(2,2)*R.M(i,2)) + (L.M(3,2)*R.M(i,3));
        Result.M(i,3) = (L.M(0,3)*R.M(i,0)) + (L.M(1,3)*R.M(i,1)) + (L.M(2,3)*R.M(i,2)) + (L.M(3,3)*R.M(i,3));
    }
    return( Result );

/*
    matrix4 Result;

    // If the bottom row of both L and R are [0 0 0 1], then we can do a
    // streamlined matrix multiplication.  Otherwise, we must do a full force
    // multiplication.

    if( (L.M(0,3) == 0.0f) && (R.M(0,3) == 0.0f) &&
        (L.M(1,3) == 0.0f) && (R.M(1,3) == 0.0f) && 
        (L.M(2,3) == 0.0f) && (R.M(2,3) == 0.0f) &&
        (L.M(3,3) == 1.0f) && (R.M(3,3) == 1.0f) )
    {
        Result.M(0,0) = (L.M(0,0)*R.M(0,0)) + (L.M(1,0)*R.M(0,1)) + (L.M(2,0)*R.M(0,2));
        Result.M(1,0) = (L.M(0,0)*R.M(1,0)) + (L.M(1,0)*R.M(1,1)) + (L.M(2,0)*R.M(1,2));
        Result.M(2,0) = (L.M(0,0)*R.M(2,0)) + (L.M(1,0)*R.M(2,1)) + (L.M(2,0)*R.M(2,2));
        Result.M(3,0) = (L.M(0,0)*R.M(3,0)) + (L.M(1,0)*R.M(3,1)) + (L.M(2,0)*R.M(3,2)) + L.M(3,0);

        Result.M(0,1) = (L.M(0,1)*R.M(0,0)) + (L.M(1,1)*R.M(0,1)) + (L.M(2,1)*R.M(0,2));
        Result.M(1,1) = (L.M(0,1)*R.M(1,0)) + (L.M(1,1)*R.M(1,1)) + (L.M(2,1)*R.M(1,2));
        Result.M(2,1) = (L.M(0,1)*R.M(2,0)) + (L.M(1,1)*R.M(2,1)) + (L.M(2,1)*R.M(2,2));
        Result.M(3,1) = (L.M(0,1)*R.M(3,0)) + (L.M(1,1)*R.M(3,1)) + (L.M(2,1)*R.M(3,2)) + L.M(3,1);

        Result.M(0,2) = (L.M(0,2)*R.M(0,0)) + (L.M(1,2)*R.M(0,1)) + (L.M(2,2)*R.M(0,2));
        Result.M(1,2) = (L.M(0,2)*R.M(1,0)) + (L.M(1,2)*R.M(1,1)) + (L.M(2,2)*R.M(1,2));
        Result.M(2,2) = (L.M(0,2)*R.M(2,0)) + (L.M(1,2)*R.M(2,1)) + (L.M(2,2)*R.M(2,2));
        Result.M(3,2) = (L.M(0,2)*R.M(3,0)) + (L.M(1,2)*R.M(3,1)) + (L.M(2,2)*R.M(3,2)) + L.M(3,2);

        Result.M(0,3) = 0.0f;
        Result.M(1,3) = 0.0f;
        Result.M(2,3) = 0.0f;
        Result.M(3,3) = 1.0f;
    }
    else
    {
        Result.M(0,0) = (L.M(0,0)*R.M(0,0)) + (L.M(1,0)*R.M(0,1)) + (L.M(2,0)*R.M(0,2)) + (L.M(3,0)*R.M(0,3));
        Result.M(1,0) = (L.M(0,0)*R.M(1,0)) + (L.M(1,0)*R.M(1,1)) + (L.M(2,0)*R.M(1,2)) + (L.M(3,0)*R.M(1,3));
        Result.M(2,0) = (L.M(0,0)*R.M(2,0)) + (L.M(1,0)*R.M(2,1)) + (L.M(2,0)*R.M(2,2)) + (L.M(3,0)*R.M(2,3));
        Result.M(3,0) = (L.M(0,0)*R.M(3,0)) + (L.M(1,0)*R.M(3,1)) + (L.M(2,0)*R.M(3,2)) + (L.M(3,0)*R.M(3,3));

        Result.M(0,1) = (L.M(0,1)*R.M(0,0)) + (L.M(1,1)*R.M(0,1)) + (L.M(2,1)*R.M(0,2)) + (L.M(3,1)*R.M(0,3));
        Result.M(1,1) = (L.M(0,1)*R.M(1,0)) + (L.M(1,1)*R.M(1,1)) + (L.M(2,1)*R.M(1,2)) + (L.M(3,1)*R.M(1,3));
        Result.M(2,1) = (L.M(0,1)*R.M(2,0)) + (L.M(1,1)*R.M(2,1)) + (L.M(2,1)*R.M(2,2)) + (L.M(3,1)*R.M(2,3));
        Result.M(3,1) = (L.M(0,1)*R.M(3,0)) + (L.M(1,1)*R.M(3,1)) + (L.M(2,1)*R.M(3,2)) + (L.M(3,1)*R.M(3,3));

        Result.M(0,2) = (L.M(0,2)*R.M(0,0)) + (L.M(1,2)*R.M(0,1)) + (L.M(2,2)*R.M(0,2)) + (L.M(3,2)*R.M(0,3));
        Result.M(1,2) = (L.M(0,2)*R.M(1,0)) + (L.M(1,2)*R.M(1,1)) + (L.M(2,2)*R.M(1,2)) + (L.M(3,2)*R.M(1,3));
        Result.M(2,2) = (L.M(0,2)*R.M(2,0)) + (L.M(1,2)*R.M(2,1)) + (L.M(2,2)*R.M(2,2)) + (L.M(3,2)*R.M(2,3));
        Result.M(3,2) = (L.M(0,2)*R.M(3,0)) + (L.M(1,2)*R.M(3,1)) + (L.M(2,2)*R.M(3,2)) + (L.M(3,2)*R.M(3,3));

        Result.M(0,3) = (L.M(0,3)*R.M(0,0)) + (L.M(1,3)*R.M(0,1)) + (L.M(2,3)*R.M(0,2)) + (L.M(3,3)*R.M(0,3));
        Result.M(1,3) = (L.M(0,3)*R.M(1,0)) + (L.M(1,3)*R.M(1,1)) + (L.M(2,3)*R.M(1,2)) + (L.M(3,3)*R.M(1,3));
        Result.M(2,3) = (L.M(0,3)*R.M(2,0)) + (L.M(1,3)*R.M(2,1)) + (L.M(2,3)*R.M(2,2)) + (L.M(3,3)*R.M(2,3));
        Result.M(3,3) = (L.M(0,3)*R.M(3,0)) + (L.M(1,3)*R.M(3,1)) + (L.M(2,3)*R.M(3,2)) + (L.M(3,3)*R.M(3,3));
    }

    return( Result );
*/
}

//==============================================================================
//  CLEAR DEFINES
//==============================================================================

#undef M

//==============================================================================

