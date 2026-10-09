#include "../AADS/Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>

static const f32 EQN_EPSILON = 1e-1f;

static f32 s_LastArcingTime = 0.0f;

//==============================================================================
// To use the f64 version, define SLOW...
//==============================================================================
#define nSLOW
//==============================================================================
// 
//==============================================================================

xbool CalculateArcingAimDirectionSlow(
      const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    ,       f32      VelInheritFactor
    ,       f32      MuzzleSpeed
    ,       f32      LifetimeS
    ,       vector3& AimDirection
    ,       vector3& AimPosition
    ,       f32      Gravity );

//==============================================================================
static inline void Swap(f32 & a, f32 & b)
{
    f32 t = b;
    b = a;
    a = t;
}

//==============================================================================
static inline void SwapSlow(f64 & a, f64 & b)
{
    f64 t = b;
    b = a;
    a = t;
}

//==============================================================================
static inline xbool IsZero(f32 Val)
{
    return((Val > -EQN_EPSILON) && (Val < EQN_EPSILON));
}

//==============================================================================
static inline xbool IsZeroSlow(f64 Val)
{
    return((Val > -EQN_EPSILON) && (Val < EQN_EPSILON));
}

//==============================================================================
static inline f32 Cbrt(f32 val)
{
    if(val < 0.0f)
        return(-x_pow(-val, f32(1.0f/3.0f)));
    else
        return(x_pow(val, f32(1.0f/3.0f)));
}

//==============================================================================
static inline f64 CbrtSlow(f64 val)
{
    if(val < 0)
        return(-pow(-val, f64(1.0/3.0)));
    else
        return(pow(val, f64(1.0/3.0)));
}

//==============================================================================
static inline u32 SolveLinear(f32 a, f32 b, f32 * x)
{
    if(IsZero(a))
        return(0);

    x[0] = -b/a;
    return(1);
}

//==============================================================================
static inline u32 SolveLinearSlow(f64 a, f64 b, f64* x)
{
    if(IsZeroSlow(a))
        return(0);

    x[0] = -b/a;
    return(1);
}

//==============================================================================
s32 SolveQuadric( f32 c[3], f32 s[2] )
{
    f32 p, q, D;

    // normal form: x^2 + px + q = 0

    p = c[1] / (2 * c[2]);
    q = c[0] / c[2];

    D = p * p - q;

    if ( IsZero( D ) )
    {
        s[0] = -p;
        return 1;
    }
    else if ( D < 0 )
    {
        return 0;
    }
    else 
    {
        ASSERT( D > 0 );
        const f32 SqrtD = x_sqrt( D );
        s[0] = SqrtD - p;
        s[1] = -SqrtD - p;
        return 2;
    }
}

//==============================================================================
s32 SolveQuadricSlow( f64 c[3], f64 s[2] )
{
    f64 p, q, D;

    // normal form: x^2 + px + q = 0

    p = c[1] / (2 * c[2]);
    q = c[0] / c[2];

    D = p * p - q;

    if ( IsZeroSlow( D ) )
    {
        s[0] = -p;
        return 1;
    }
    else if ( D < 0 )
    {
        return 0;
    }
    else 
    {
        ASSERT( D > 0 );
        const f64 SqrtD = sqrt( D );
        s[0] = SqrtD - p;
        s[1] = -SqrtD - p;
        return 2;
    }
}

//======================================================================
u32 SolveCubic( f32 c[4], f32 s[3] )
{
    s32 i, num;
    f32 sub;
    f32 A, B, C;
    f32 sq_A, p, q;
    f32 cb_p, D;

    // normal form: x^3 +Ax^2 + Bx + C = 0
    A = c[2] / c[3];
    B = c[1] / c[3];
    C = c[0] / c[3];

    // substitute x = y - A/3 to eliminate quadric term:
    // x^3 + px + q = 0

    sq_A = A * A;
    p = 1.0f / 3 * (-1.0f/3 * sq_A + B );
    q = 1.0f / 2 * (2.0f/27 * A * sq_A - 1.0f/3 * A * B + C);

    // use Cardano's formula

    cb_p = p * p * p;
    D = q * q + cb_p;

    if ( IsZero( D ) )
    {
        if ( IsZero( q ) )      // one triple solution
        {
            s[0] = 0.0f;
            num = 1;
        }
        else                    // one single and one double solution
        {
            f32 u = Cbrt( -q );
            s[0] = 2 * u;
            s[1] = -u;
            num = 2;
        }
    }
    else if ( D < 0 )           // Casus irreducibilis: three real solutions
    {
        f32 phi = 1.0f / 3 * x_acos( -q / x_sqrt( -cb_p ) );
        f32 t = 2 * x_sqrt( -p );

        s[0] = t * x_cos( phi );
        s[1] = -t * x_cos( phi + PI / 3 );
        s[2] = -t * x_cos( phi - PI / 3 );
        num = 3;
    }
    else                        // one real solution
    {
        const f32 sqrt_D = x_sqrt( D );
        const f32 u = Cbrt( sqrt_D - q );
        const f32 v = -Cbrt( sqrt_D + q );

        s[0] = u + v;
        num = 1;
    }

    // resubstitute

    sub = 1.0f/3 * A;

    for ( i = 0; i < num; ++i )
    {
        s[i] -= sub;
    }

    return num;
    
}

//======================================================================
u32 SolveCubicSlow( f64 c[4], f64 s[3] )
{
    s32 i, num;
    f64 sub;
    f64 A, B, C;
    f64 sq_A, p, q;
    f64 cb_p, D;

    // normal form: x^3 +Ax^2 + Bx + C = 0
    A = c[2] / c[3];
    B = c[1] / c[3];
    C = c[0] / c[3];

    // substitute x = y - A/3 to eliminate quadric term:
    // x^3 + px + q = 0

    sq_A = A * A;
    p = 1 / 3 * (-1/3 * sq_A + B );
    q = 1 / 2 * (2/27 * A * sq_A - 1/3 * A * B + C);

    // use Cardano's formula

    cb_p = p * p * p;
    D = q * q + cb_p;

    if ( IsZeroSlow( D ) )
    {
        if ( IsZeroSlow( q ) )      // one triple solution
        {
            s[0] = 0.0;
            num = 1;
        }
        else                    // one single and one double solution
        {
            f64 u = CbrtSlow( -q );
            s[0] = 2 * u;
            s[1] = -u;
            num = 2;
        }
    }
    else if ( D < 0 )           // Casus irreducibilis: three real solutions
    {
        f64 phi = 1 / 3 * acos( -q / sqrt( -cb_p ) );
        f64 t = 2 * sqrt( -p );

        s[0] = t * cos( phi );
        s[1] = -t * cos( phi + PI / 3 );
        s[2] = -t * cos( phi - PI / 3 );
        num = 3;
    }
    else                        // one real solution
    {
        const f64 sqrt_D = sqrt( D );
        const f64 u = CbrtSlow( sqrt_D - q );
        const f64 v = -CbrtSlow( sqrt_D + q );

        s[0] = u + v;
        num = 1;
    }

    // resubstitute

    sub = 1/3 * A;

    for ( i = 0; i < num; ++i )
    {
        s[i] -= sub;
    }

    return num;
    
}

//=============================================================================
s32 SolveQuartic( f32 c[5], f32 s[4] )
{
    f32 coeffs[4];
    f32 z, u, v, sub;
    f32 A, B, C, D;
    f32 sq_A, p, q, r;
    s32 i, num;

    // normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0

    A = c[3] / c[4];
    B = c[2] / c[4];
    C = c[1] / c[4];
    D = c[0] / c[4];

    // substitued x = y - A/4 to eliminate cubic term:
    // x^4 + px^2 + qx + r = 0

    sq_A = A * A;
    p = (-3.0f / 8) * sq_A + B;
    q = (1.0f / 8) * sq_A * A - (1.0f / 2) * A * B + C;
    r = (-3.0f/256) * sq_A * sq_A + (1.0f/16) * sq_A * B - (1.0f/4) * A * C + D;

    if ( IsZero( r ) )
    {
        // no absolute term: y(y^3 + py + q) = 0

        coeffs[0] = q;
        coeffs[1] = p;
        coeffs[2] = 0;
        coeffs[3] = 1;

        num = SolveCubic( coeffs, s );
        s[num++] = 0;
    }
    else
    {
        // solve the resolvent cubic...

        coeffs[0] = (1.0f / 2) * r * p - 1.0f / 8 * q * q;
        coeffs[1] = -r;
        coeffs[2] = -1.0f / 2 * p;
        coeffs[3] = 1;

        SolveCubic( coeffs, s );

        // ... and take the one real solution

        z = s[0];

        // ... to build to quadric equations

        u = z * z - r;
        v = 2 * z - p;

        if ( IsZero( u ) )
        {
            u = 0;
        }
        else if ( u > 0 )
        {
            u = x_sqrt( u );
        }
        else
        {
            return 0;
        }

        if ( IsZero( v ) )
        {
            v = 0;
        }
        else if ( v > 0 )
        {
            v = x_sqrt( v );
        }
        else 
        {
            return 0;
        }
        

        coeffs[0] = z - u;
        coeffs[1] = q < 0 ? -v : v; // changed per Graphics Gems errata...
        coeffs[2] = 1;

        num = SolveQuadric( coeffs, s );

        coeffs[0] = z + u;
        coeffs[1] = q < 0 ? v : -v; // changed per Graphics Gems errata...
        coeffs[2] = 1;

        num += SolveQuadric( coeffs, s + num );
    }

    // resubstitute
    sub = 1.0f/4 * A;

    for ( i = 0; i < num; ++i )
    {
        s[i] -= sub;
    }

    return num;
}

//=============================================================================
s32 SolveQuarticSlow( f64 c[5], f64 s[4] )
{
    f64 coeffs[4];
    f64 z, u, v, sub;
    f64 A, B, C, D;
    f64 sq_A, p, q, r;
    s32 i, num;

    // normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0

    A = c[3] / c[4];
    B = c[2] / c[4];
    C = c[1] / c[4];
    D = c[0] / c[4];

    // substitued x = y - A/4 to eliminate cubic term:
    // x^4 + px^2 + qx + r = 0

    sq_A = A * A;
    p = (-3 / 8) * sq_A + B;
    q = (1 / 8) * sq_A * A - (1 / 2) * A * B + C;
    r = (-3/256) * sq_A * sq_A + (1/16) * sq_A * B - (1/4) * A * C + D;

    if ( IsZeroSlow( r ) )
    {
        // no absolute term: y(y^3 + py + q) = 0

        coeffs[0] = q;
        coeffs[1] = p;
        coeffs[2] = 0;
        coeffs[3] = 1;

        num = SolveCubicSlow( coeffs, s );
        s[num++] = 0;
    }
    else
    {
        // solve the resolvent cubic...

        coeffs[0] = (1 / 2) * r * p - 1 / 8 * q * q;
        coeffs[1] = -r;
        coeffs[2] = -1 / 2 * p;
        coeffs[3] = 1;

        SolveCubicSlow( coeffs, s );

        // ... and take the one real solution

        z = s[0];

        // ... to build to quadric equations

        u = z * z - r;
        v = 2 * z - p;

        if ( IsZeroSlow( u ) )
        {
            u = 0;
        }
        else if ( u > 0 )
        {
            u = sqrt( u );
        }
        else
        {
            return 0;
        }

        if ( IsZeroSlow( v ) )
        {
            v = 0;
        }
        else if ( v > 0 )
        {
            v = sqrt( v );
        }
        else 
        {
            return 0;
        }
        

        coeffs[0] = z - u;
        coeffs[1] = q < 0 ? -v : v; // changed per Graphics Gems errata...
        coeffs[2] = 1;

        num = SolveQuadricSlow( coeffs, s );

        coeffs[0] = z + u;
        coeffs[1] = q < 0 ? v : -v; // changed per Graphics Gems errata...
        coeffs[2] = 1;

        num += SolveQuadricSlow( coeffs, s + num );
    }

    // resubstitute
    sub = 1/4 * A;

    for ( i = 0; i < num; ++i )
    {
        s[i] -= sub;
    }

    return num;
}

//==============================================================================
static u32 OldSolveQuadratic(f32 a, f32 b, f32 c, f32 * x)
{
    // really linear?
    if(IsZero(a))
        return(SolveLinear(b, c, x));
  
    // get the descriminant:   (b^2 - 4ac)
    f32 desc = (b * b) - (4.0f * a * c);
   
    // solutions:
    // desc < 0:   two imaginary solutions 
    // desc > 0:   two real solutions (b +- sqrt(desc)) / 2a
    // desc = 0:   one real solution (b / 2a)
    if(IsZero(desc))
    {
        x[0] = b / (2.0f * a);
        return(1);
    }
    else if(desc > 0.0f)
    {
        const f32 sqrdesc = x_sqrt(desc);
        f32 den = (2.0f * a);
        x[0] = (b + sqrdesc) / den;
        x[1] = (b - sqrdesc) / den;

        if(x[1] < x[0])
            Swap(x[0], x[1]);

        return(2);
    }
    else 
        return(0);
}

u32 OldSolveCubic(f32 a, f32 b, f32 c, f32 d, f32 * x)
{
    if(IsZero(a))
        return(OldSolveQuadratic(b, c, d, x));

    // normal form: x^3 + Ax^2 + BX + C = 0
    f32 A = b / a;
    f32 B = c / a;
    f32 C = d / a;
   
    // substitute x = y - A/3 to eliminate quadric term and depress
    // the cubic equation to (x^3 + px + q = 0)
    f32 A2 = A * A;
    f32 A3 = A2 * A;

    f32 p = (1.0f/3.0f) * (((-1.0f/3.0f) * A2) + B);
    f32 q = (1.0f/2.0f) * (((2.0f/27.0f) * A3) - ((1.0f/3.0f) * A * B) + C);

    // use Cardano's fomula to solve the depressed cubic
    f32 p3 = p * p * p;
    f32 q2 = q * q;

    f32 D = q2 + p3;

    s32 num = 0;
   
    if(IsZero(D))          // 1 or 2 solutions
    {
        if(IsZero(q)) // 1 triple solution
        {
            x[0] = 0.0f;
            num = 1;
        }
        else // 1 single and 1 double
        {
            f32 u = Cbrt(-q);
            x[0] = 2.0f * u;
            x[1] = -u;
            num = 2;
        }
    }
    else if(D < 0.0f)        // 3 solutions: casus irreducibilis
    {
        f32 phi = (1.0f/3.0f) * x_acos(-q / x_sqrt(-p3));
        f32 t = 2.0f * x_sqrt(-p);
      
        x[0] = t * x_cos(phi);
        x[1] = -t * x_cos(phi + (PI / 3.0f));
        x[2] = -t * x_cos(phi - (PI / 3.0f));
        num = 3;
    }
    else                    // 1 solution
    {
        const f32 sqrtD = x_sqrt(D);
        f32 u = Cbrt(sqrtD - q);
        f32 v = -Cbrt(sqrtD + q);

        x[0] = u + v;
        num = 1;
    }
   
    // resubstitute
    f32 sub = (1.0f/3.0f) * A;
    s32 i;
    for(i = 0; i < num; i++)
        x[i] -= sub;

    // sort the roots
    s32 j,k;
    for(j = 0; j < (num - 1); j++)
        for(k = j + 1; k < num; k++)
            if(x[k] < x[j])
                Swap(x[k], x[j]);

    return(num);
}

//--------------------------------------------------------------------------
// from Graphics Gems I: pp 738-742
u32 OldSolveQuartic(f32 a, f32 b, f32 c, f32 d, f32 e, f32 * x)
{
    if( IsZero( a ) )
    {
        return( OldSolveCubic(b, c, d, e, x) );
    }

    // normal form: x^4 + ax^3 + bx^2 + cx + d = 0
    f32 A = b / a;
    f32 B = c / a;
    f32 C = d / a;
    f32 D = e / a;

    // substitue x = y - A/4 to eliminate cubic term:
    // x^4 + px^2 + qx + r = 0
    f32 A2 = A * A;
    f32 A3 = A2 * A;
    f32 A4 = A2 * A2;

    f32 p = ((-3.0f/8.0f) * A2) + B;
    f32 q = ((1.0f/8.0f) * A3) - ((1.0f/2.0f) * A * B) + C;
    f32 r = ((-3.0f/256.0f) * A4)
        + ((1.0f/16.0f) * A2 * B)
        - ((1.0f/4.0f) * A * C) + D;

    s32 num = 0;
    if( IsZero( r ) ) // no absolute term: y(y^3 + py + q) = 0
    {
        num = OldSolveCubic(1.0f, 0.0f, p, q, x);
        x[num++] = 0.0f;
    }
    else 
    {
        // solve the resolvent cubic
        f32 q2 = q * q;

        a = 1.0f;
        b = (-1.0f/2.0f) * p;
        c = -r;
        d = ((1.0f/2.0f) * r * p) - ((1.0f/8.0f) * q2);
      
        num = OldSolveCubic(a, b, c, d, x);

        f32 z = x[0];
      
        // build 2 quadratic equations from the one solution
        f32 u = (z * z) - r;
        f32 v = (2.0f * z) - p;

        if(IsZero(u))
            u = 0.0f;
        else if(u > 0.0f)
            u = x_sqrt(u);
        else
        {
            return( 0 );
        }
        
         
        if(IsZero(v))
            v = 0.0f;
        else if(v > 0.0f)
            v = x_sqrt(v);
        else
        {
            return( 0 );
        }
        
        // solve the two quadratics
        a = 1.0f;
        b = q < 0 ? -v : v;
        c = z - u;
        num = OldSolveQuadratic(a, b, c, x);
      
        a = 1.0f;
        b = q < 0 ? v : -v;
        c = z + u;
        num += OldSolveQuadratic(a, b, c, x + num);
    }
   
    // resubstitute
    f32 sub = (1.0f/4.0f) * A;
    s32 i;
    for(i = 0; i < num; i++)
        x[i] -= sub;

    // sort the roots
    s32 j, k;
    for(j = 0; j < (num - 1); j++)
        for(k = j + 1; k < num; k++)
            if(x[k] < x[j])
                Swap(x[k], x[j]);

    return(num);
}


//==============================================================================
xbool CalculateLinearAimDirection(
    const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    , f32 VelInheritFactor
    , f32 LinearSpeed
    , f32 LifetimeS
    , vector3& AimDirection
    , vector3& AimPosition 
    , xbool Verbose )
{
    const vector3 EffTargetPos = TargetPos - SourcePos;
    const vector3 EffTargetVel = TargetVel - (SourceVel * VelInheritFactor);

    AimDirection.Zero();

    // Without underwater aiming, this is a straightforward law of cosines
    //  calculation.  We're not being especially efficient here, but hey.
    //
    vector3 NormPos = EffTargetPos;
    vector3 NormVel = EffTargetVel;
    if( !NormPos.Normalize() )  return( FALSE );
    NormVel.Normalize();

    const f32 A = EffTargetVel.LengthSquared() - (LinearSpeed*LinearSpeed);
    const f32 B = 2 * EffTargetPos.Length() * EffTargetVel.Length() * NormPos.Dot(NormVel);
    const f32 C = EffTargetPos.LengthSquared();

    const f32 det = (B * B) - (4 * A * C);

    if( IN_RANGE( -0.0001f, A, 0.0001f ) )
    {
        // Value A is 0 or at least damn close to 0.  The division for Sol1 a 
        // few lines below will either die or blow up.  So, bail out 
        // meainingfully.
        AimPosition  = TargetPos;
        AimDirection = (TargetPos - SourcePos);
        return( AimDirection.Normalize() );
    }

    if (det < 0.0) {
        // No solution is possible in the real numbers
        return FALSE;
    }

    const f32 Sol1 = (-B + x_sqrt(det)) / (2 * A);
    const f32 Sol2 = (-B - x_sqrt(det)) / (2 * A);

    f32 T;

    if (Sol2 > 0.0f)    T = Sol2;
    else                T = Sol1;
    
    if (T < 0.0f) 
    {
        return FALSE;
    }

    // Once we know how long the projectile's path will take, it's
    // straightforward to find out where it should go...
    AimPosition  = TargetPos + (EffTargetVel * T);
    AimDirection = (EffTargetPos / (LinearSpeed * T)) + (EffTargetVel / LinearSpeed);

    if( Verbose )
    {
        x_DebugMsg( "From(%7.2f,%7.2f,%7.2f) - To(%7.2f,%7.2f,%7.2f) - T:%7.2f\n",
                    SourcePos.X, SourcePos.Y, SourcePos.Z,
                    AimPosition.X, AimPosition.Y, AimPosition.Z,
                    T );
    }

    return( AimDirection.Normalize() && ((T * 0.001f) <= LifetimeS) );
}

//==============================================================================
xbool CalculateArcingAimDirection(
    const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    , f32 VelInheritFactor
    , f32 MuzzleSpeed
    , f32 LifetimeS
    , vector3& AimDirection
    , vector3& AimPosition
    , f32 Gravity /* = 25.0f */ )
{
    // Ok, this is a quartic calculation to determine the time to target.  TODO:
    //  get the gravity from the environment.  TODO: handle water
    //
    vector3 p = TargetPos - SourcePos;
    vector3 v = TargetVel - (SourceVel * VelInheritFactor);

    const f32 g = Gravity;

    f32 c[5];
    c[4] = 0.25f * (g * g);
    c[3] = v.Y * g;
    c[2] = p.Y * g + v.LengthSquared() - (MuzzleSpeed * MuzzleSpeed);
    c[1] = 2 * p.Dot(v);
    c[0] = p.LengthSquared();

    f32 x[4];
    u32 NumRealSolutions = SolveQuartic( c, x);
    //u32 NumRealSolutions = OldSolveQuartic( c[4], c[3], c[2], c[1], c[0], x);

    if (NumRealSolutions == 0) {
        // No Solution is possible in the real numbers
#ifdef SLOW
        return CalculateArcingAimDirectionSlow(
            TargetPos
            , TargetVel
            , SourcePos
            , SourceVel
            , VelInheritFactor
            , MuzzleSpeed
            , LifetimeS
            , AimDirection
            , AimPosition
            , Gravity );
#else
        return FALSE;
#endif
    }

    f32 MinPos[2] = { F32_MAX, F32_MAX };

    u32 i;
    for ( i = 0; i < NumRealSolutions; i++) {
        if (x[i] > 0.0f) {
            if (x[i] < MinPos[0]) {
                MinPos[1] = MinPos[0];
                MinPos[0] = x[i];
            } else if (x[i] < MinPos[1]) {
                MinPos[1] = x[i];
            }
        }
    }
    if (MinPos[0] == F32_MAX) {
#ifdef SLOW
        return CalculateArcingAimDirectionSlow(
            TargetPos
            , TargetVel
            , SourcePos
            , SourceVel
            , VelInheritFactor
            , MuzzleSpeed
            , LifetimeS
            , AimDirection
            , AimPosition
            , Gravity );
#else
        return FALSE;
#endif
    }

    if (MinPos[1] == F32_MAX)
    {
        MinPos[1] = MinPos[0];
    }
   
    ASSERT(MinPos[0]  // bad calculation of minimum positive times!
        != F32_MAX && MinPos[1]
        != F32_MAX && MinPos[0]
        > 0.0f && MinPos[1] > 0.0f ); 


    // NOTE: (MAR
    // The original code returned the two minimum time solutions, where I
    // am just returning the single minimum, for simplicity. No harm if we
    // later wish to return two like the original code (see #if 0 below...)
    //
    // Ok, we have Solutions!

    AimPosition   = TargetPos;
    AimPosition  += v * MinPos[0];
    AimPosition  -= vector3(0, -g, 0) * 0.5f * MinPos[0] * MinPos[0];

    AimDirection  = AimPosition - SourcePos;
    AimDirection /= MinPos[0];
    AimDirection -= (SourceVel * VelInheritFactor);
    AimDirection /= MuzzleSpeed;
    AimDirection.Normalize();

    // Record the travel time in case the aller wants it in a moment.
    s_LastArcingTime = MinPos[0];

    return (MinPos[0] * 0.001f) < LifetimeS;

#if 0
    // Ok, we have Solutions!
    OutputVectorMin  = TargetPos;
    OutputVectorMin += TargetVel * MinPos[0];
    OutputVectorMin -= vector3(0, -g, 0) * 0.5f * MinPos[0] * MinPos[0];
    OutputVectorMin -= SourcePos;
    OutputVectorMin /= MinPos[0];
    OutputVectorMin -= (SourceVel * VelInheritFactor);
    OutputVectorMin /= MuzzleSpeed;
    OutputVectorMin.Normalize();

    OutputVectorMax  = TargetPos;
    OutputVectorMax += TargetVel * MinPos[1];
    OutputVectorMax -= vector3(0, -g, 0) * 0.5f * MinPos[1] * MinPos[1];
    OutputVectorMax -= SourcePos;
    OutputVectorMax /= MinPos[1];
    OutputVectorMax -= (SourceVel * VelInheritFactor);
    OutputVectorMax /= MuzzleSpeed;
    OutputVectorMax.Normalize();
    

    OutputMinTime = MinPos[0];
    OutputMaxTime = MinPos[1];
    return (OutputMinTime * 1000.0f) < LifetimeS;
#endif

}

//==============================================================================
xbool CalculateArcingAimDirectionSlow(
    const vector3& TargetPos
    , const vector3& TargetVel
    , const vector3& SourcePos
    , const vector3& SourceVel
    , f32 VelInheritFactor
    , f32 MuzzleSpeed
    , f32 LifetimeS
    , vector3& AimDirection
    , vector3& AimPosition
    , f32 Gravity )
{
    // Ok, this is a quartic calculation to determine the time to target.  TODO:
    //  get the gravity from the environment.  TODO: handle water
    //
    vector3 p = TargetPos - SourcePos;
    vector3 v = TargetVel - (SourceVel * VelInheritFactor);

    const f64 g = (f64)Gravity;

    f64 c[5];
    c[4] = 0.25 * (g * g);
    c[3] = v.Y * g;
    c[2] = p.Y * g + v.LengthSquared() - (MuzzleSpeed * MuzzleSpeed);
    c[1] = 2 * p.Dot(v);
    c[0] = p.LengthSquared();

    f64 x[4];
    u32 NumRealSolutions = SolveQuarticSlow( c, x);

    if (NumRealSolutions == 0) {
        // No Solution is possible in the real numbers
        AimDirection.Set(0.0f, 0.0f, 0.0f);
        return FALSE;
    }

    f64 MinPos[2] = { F64_MAX, F64_MAX };

    u32 i;
    for ( i = 0; i < NumRealSolutions; i++) {
        if (x[i] > 0.0f) {
            if (x[i] < MinPos[0]) {
                MinPos[1] = MinPos[0];
                MinPos[0] = x[i];
            } else if (x[i] < MinPos[1]) {
                MinPos[1] = x[i];
            }
        }
    }
    if (MinPos[0] == F64_MAX) {
        AimDirection.Set(0.0, 0.0, 0.0);
        return FALSE;
    }

    if (MinPos[1] == F64_MAX)
    {
        MinPos[1] = MinPos[0];
    }
   
    ASSERT(MinPos[0]  // bad calculation of minimum positive times!
        != F64_MAX && MinPos[1]
        != F64_MAX && MinPos[0]
        > 0 && MinPos[1] > 0 ); 


    // NOTE: (MAR
    // The original code returned the two minimum time solutions, where I
    // am just returning the single minimum, for simplicity. No harm if we
    // later wish to return two like the original code (see #if 0 below...)
    //
    // Ok, we have Solutions!

    AimPosition   = TargetPos;
    AimPosition  += v * (f32)MinPos[0];
    AimPosition  -= vector3(0, -(f32)g, 0) * 0.5 * (f32)MinPos[0] * (f32)MinPos[0];

    AimDirection  = AimPosition - SourcePos;
    AimDirection /= (f32)MinPos[0];
    AimDirection -= (SourceVel * VelInheritFactor);
    AimDirection /= MuzzleSpeed;
    AimDirection.Normalize();

    // Record the travel time in case the aller wants it in a moment.
    s_LastArcingTime = (f32)MinPos[0];

    return (MinPos[0] * 0.001) < LifetimeS;
}


void CalculateArcingImpactPos(
      const vector3&    SourcePos
    , const vector3&    SourceVel
    ,       vector3&    Impact
    ,       object::id& ObjectHitID
    ,       void*       pMovingObj
    ,       s32         ExcludeObjID
    ,       f32         Gravity /* = 25 */ )
{
    // TODO: Handle these with bomber code...
    // simulate dropping the bomb right this instant
    //vector3 Offset = s_BomberBombingViewOffset ;
    //vector3 Pos = Offset + m_GeomInstance.GetPos() ;

#define nRECORD_POINTS

#ifdef RECORD_POINTS
    x_DebugMsg( "CalculateArcingImpactPos()\n" );
#endif
    
    static const f32        SIM_TIME_INC = 1.0f;
    const f32               GravConst = Gravity * 0.5f; // for "1/2 a t^2"  eq.
    s32                     i;
    const vector3           DeltaPos = SIM_TIME_INC * SourceVel;
    vector3                 OldPos, NewPos, ActualPos;
    collider                Ray;
    collider::collision     Collision; 

    OldPos = SourcePos;
    NewPos = SourcePos;

    // infinite i loop (start with 1 because time zero is accounted for already)
    for ( i = 1; ; i++ )
    {
#ifdef RECORD_POINTS
        x_DebugMsg( "vector3( %6.6ff, %6.6ff, %6.6ff )\n"
            , OldPos.X, OldPos.Y, OldPos.Z );
#endif
        f32 TotalY = GravConst * ((i * SIM_TIME_INC) * (i * SIM_TIME_INC));

        NewPos += DeltaPos;
        ActualPos = NewPos;
        ActualPos.Y = NewPos.Y - TotalY;

        // setup a collision ray for this segment
        Ray.RaySetup    ( pMovingObj, OldPos, ActualPos, ExcludeObjID );
        ObjMgr.Collide  ( object::ATTR_SOLID, Ray );
        
        if( Ray.HasCollided() )
        {
            Ray.GetCollision( Collision );
            Impact = Collision.Point;
            ObjectHitID = ((object*)(Collision.pObjectHit))->GetObjectID();
            return;
        }

        // Check to see if we've fallen through the ground
        if ( (ActualPos.Y < -1000.0f) && (OldPos.Y > ActualPos.Y) )
        {
#if defined(mreed)
            ////////////////////////////////////////////////////
            // Please try to find out what weapon the bot is
            // using. This is happening because the weapon's
            // hot point is embedded in a wall, so the fired
            // projectile would continue forever...
            x_DebugMsg( "Aimer: Endless arcing test error\n" );
            x_DebugMsg( "SourcePos: (%5.2f, %5.2f, %5.2f)\n", SourcePos.X, SourcePos.Y, SourcePos.Z );
            x_DebugMsg( "SourceVel: (%5.2f, %5.2f, %5.2f)\n", SourceVel.X, SourceVel.Y, SourceVel.Z );
            x_DebugMsg( "Gravity:   %5.2f\n", Gravity );
            ASSERT( 0 );
            ////////////////////////////////////////////////////
#endif
            
            Impact = ActualPos;
            ObjectHitID = obj_mgr::NullID;
            return;
        }

        // continue
        OldPos = ActualPos;

        // Safety
        ASSERT( i < (100 / SIM_TIME_INC) );
    }

    // We should always be able to hit something
    ASSERT( FALSE );
}

//==============================================================================

f32 GetLastArcingTime( void )
{
    return( s_LastArcingTime );
}

//==============================================================================

