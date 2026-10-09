#include <Support/Shape/Anim.hpp>
#include <Support/Shape/Shape.hpp>



//==============================================================================
// Structures
//==============================================================================

struct comp_pos_key_hdr
{
    vector4 Min, Scale ;
} ;

struct comp_pos_key
{
    u8  X,Y,Z ;

    inline u8& operator [] ( s32 Index )
    {
        ASSERT( (Index >= 0) && (Index < 3) );

        return( ((u8*)this)[Index] );
    }
} ;

struct comp_rot_key_hdr
{
    vector4 Min, Scale ;
} ;

struct comp_rot_key
{
    u8 X,Y,Z,W ;

    inline u8& operator [] ( s32 Index )
    {
        ASSERT( (Index >= 0) && (Index < 4) );

        return( ((u8*)this)[Index] );
    }
} ;


//==============================================================================
// Util functions
//==============================================================================

#define PS2_USE_VU0


void BlendLinear( const vector3& vA, const vector3& vB, f32 T, vector3& vOut)
{
    vOut.X = vA.X + ( (vB.X - vA.X) * T ) ;
    vOut.Y = vA.Y + ( (vB.Y - vA.Y) * T ) ;
    vOut.Z = vA.Z + ( (vB.Z - vA.Z) * T ) ;
}

// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
// NOTE: Leaves vOut.W untouched
void BlendLinear( const vector4& vA, const vector4& vB, f32 T, vector4& vOut)
{
#if defined(TARGET_PS2) && defined(PS2_USE_VU0)

    // void sceVu0InterVectorXYZ(v0, v1, v2, t) ;   v0 = v1*t + v2*(1-t)
    //sceVu0InterVectorXYZ((sceVu0FVECTOR)&vOut, (sceVu0FVECTOR)&vB, (sceVu0FVECTOR)&vA, T) ;

    asm __volatile__(
    "
    lqc2		vf01,0x0(%0)        # vf01   = vA
	lqc2		vf02,0x0(%1)        # vf02   = vB
    mfc1		$8,%2               # vf03   = T
    qmtc2		$8,vf03
   
    vsub		vf04,vf02,vf01		# vf04 = vB - vA
    vmulaw.xyz  acc, vf01, vf00w    # acc  = vA
    vmaddx.xyz  vf01, vf04, vf03x   # vf04 = vA + ((vB - vA) * T)

	sqc2		vf01,0x0(%3)        # vOut = Result
	"
	:
	:
	"r" (&vA),      // %0
	"r" (&vB),	    // %1
	"f" (T),        // %2
	"r" (&vOut)     // %3
	:
	"$8"
	);

#else
    vOut.X = vA.X + ( (vB.X - vA.X) * T ) ;
    vOut.Y = vA.Y + ( (vB.Y - vA.Y) * T ) ;
    vOut.Z = vA.Z + ( (vB.Z - vA.Z) * T ) ;
#endif  //#ifdef TARGET_PS2
}


// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
void BlendFast( const quaternion& Qa, const quaternion& Qb, f32 T, quaternion& qOut )
{
#if defined(TARGET_PS2) && defined(PS2_USE_VU0)
    register f32     Temp;

    // Determine if quats are further than 90 degrees
    // Temp = Qa.X*Qb.X + Qa.Y*Qb.Y + Qa.Z*Qb.Z + Qa.W*Qb.W;
    // vf01 = Qa
    // vf02 = Qb
    // vf04 = Zero
    asm __volatile__(
    "
    lqc2		vf01,0x0(%1)        # vf01 = Qa
	lqc2		vf02,0x0(%2)        # vf02 = Qb

	vmul.xyzw	vf03,vf01,vf02
	vaddy.x		vf03,vf03,vf03
	vaddz.x		vf03,vf03,vf03
	vaddw.x		vf03,vf03,vf03

    vsub        vf04, vf04, vf04    # vf04 = zero

	qmfc2		$2,vf03
	mtc1		$2,%0

	"
	:
	"=f" (Temp)    // %0
	:
	"r"  (&Qa),    // %1
	"r"  (&Qb)     // %2
	:
	"$2"
	);

    // If dot is negative flip one of the quaterions
    if( Temp < 0.0f )
    {
        // qOut = -Qa
        // from above, vf04 = zero
        asm __volatile__(
        "
        vsub.xyzw vf01, vf04, vf01
	    "
	    );
    }

    // Compute interpolated values
    //qOut.X = qOut.X + T*(Qb.X - qOut.X);
    //qOut.Y = qOut.Y + T*(Qb.Y - qOut.Y);
    //qOut.Z = qOut.Z + T*(Qb.Z - qOut.Z);
    //qOut.W = qOut.W + T*(Qb.W - qOut.W);

    // from above:
	// vf01   = qOut
	// vf02   = Qb
    asm __volatile__(
    "
    mfc1		$8,%0               # vf03   = T
    qmtc2		$8,vf03

    vsub		vf04,vf02,vf01		# vf04 = Qb - qOut
    vmulaw.xyzw acc, vf01, vf00w    # acc  = qOut
    vmaddx.xyzw vf01, vf04, vf03x   # vf01 = qOut + ((Qb - qOut) * T)
	"
	:
	:
	"f" (T)         // %0
	:
	"$8"
	);

    // Get squared distance of new quaternion
    // Temp = qOut.X*qOut.X + qOut.Y*qOut.Y + qOut.Z*qOut.Z + qOut.W*qOut.W;
    // from above vf01 = qOut
    asm __volatile__(
    "
	vmul.xyzw	vf02,vf01,vf01
	vaddy.x		vf02,vf02,vf02
	vaddz.x		vf02,vf02,vf02
	vaddw.x		vf02,vf02,vf02

	qmfc2		$2,vf02
	mtc1		$2,%0

	"
	:
	"=f" (Temp)      // %0
	:
	:
	"$2"
	);

    // Use home-baked polynomial to compute 1/sqrt(Dist2)
    // since we know the range is 0.707 >= Dist2 <= 1.0
    // we'll split in half.
    if( Temp<0.857f )
        Temp = (((0.699368f)*Temp) + -1.819985f)*Temp + 2.126369f;    //0.0000792
    else
        Temp = (((0.454012f)*Temp) + -1.403517f)*Temp + 1.949542f;    //0.0000373

    // Renormalize and put into out quaternion
    // qOut *= Temp
    // from above vf01   = qOut
    asm __volatile__(
    "
    mfc1		$8,%1               # vf02   = Temp
    qmtc2		$8,vf02

	vmulx.xyzw  vf01,vf01,vf02      # vf01 = qOut * Temp

	sqc2		vf01,0x0(%0)        # qOut = Result
	"
	:
	:
	"r" (&qOut),     // %0
	"f" (Temp)   // %1
	:
	"$8"
	);

#else

    f32     Dot;
    f32     Dist2;
    f32     OneOverL;

    // Determine if quats are further than 90 degrees
    Dot = Qa.X*Qb.X + Qa.Y*Qb.Y + Qa.Z*Qb.Z + Qa.W*Qb.W;

    // If dot is negative flip one of the quaterions
    if( Dot < 0.0f )
    {
       qOut.X = -Qa.X;
       qOut.Y = -Qa.Y;
       qOut.Z = -Qa.Z;
       qOut.W = -Qa.W;
    }
    else
    {
       qOut.X = +Qa.X;
       qOut.Y = +Qa.Y;
       qOut.Z = +Qa.Z;
       qOut.W = +Qa.W;
    }

    // Compute interpolated values
    qOut.X = qOut.X + T*(Qb.X - qOut.X);
    qOut.Y = qOut.Y + T*(Qb.Y - qOut.Y);
    qOut.Z = qOut.Z + T*(Qb.Z - qOut.Z);
    qOut.W = qOut.W + T*(Qb.W - qOut.W);

    // Get squared distance of new quaternion
    Dist2 = qOut.X*qOut.X + qOut.Y*qOut.Y + qOut.Z*qOut.Z + qOut.W*qOut.W;

    // Use home-baked polynomial to compute 1/sqrt(Dist2)
    // since we know the range is 0.707 >= Dist2 <= 1.0
    // we'll split in half.
    if( Dist2<0.857f )
        OneOverL = (((0.699368f)*Dist2) + -1.819985f)*Dist2 + 2.126369f;    //0.0000792
    else
        OneOverL = (((0.454012f)*Dist2) + -1.403517f)*Dist2 + 1.949542f;    //0.0000373

    // Renormalize and put into out quaternion
    qOut.X = qOut.X*OneOverL ;
    qOut.Y = qOut.Y*OneOverL ;
    qOut.Z = qOut.Z*OneOverL ;
    qOut.W = qOut.W*OneOverL ;
#endif  //#ifdef TARGET_PS2

}

// PS2 vu0 version used by "matrix4* shape_instance::CalculateNodeMatrices()"
void BlendIdentityFast( const quaternion& Qa, f32 T, quaternion& qOut )
{
#if defined(TARGET_PS2) && defined(PS2_USE_VU0)

    register f32     Temp;

    // vf01 = Qa
    asm __volatile__(
    "
    lqc2		vf01,0x0(%0)        # vf01 = Qa
    vsub        vf04,vf04,vf04      # vf04 = zero
	"
	:
	:
	"r"  (&Qa)    // %0
	);

    // If dot is negative flip one of the quaterions
    if( Qa.W < 0.0f )
    {
        // qOut = -Qa
        // from above, vf04 = zero
        asm __volatile__(
        "
        vsub.xyzw vf01, vf04, vf01
	    "
	    );
    }

    // Compute interpolated values
    //qOut.X = qOut.X + T*(vf00.X - qOut.X);
    //qOut.Y = qOut.Y + T*(vf00.Y - qOut.Y);
    //qOut.Z = qOut.Z + T*(vf00.Z - qOut.Z);
    //qOut.W = qOut.W + T*(vf00.W - qOut.W);

    // from above:
	// vf01   = qOut
	// vf00   = Identity quaternion (how convenient)
    asm __volatile__(
    "
    mfc1		$8,%0               # vf03 = T
    qmtc2		$8,vf03

    vsub		vf04,vf00,vf01		# vf04 = vf00 - qOut
    vmulaw.xyzw acc, vf01, vf00w    # acc  = qOut
    vmaddx.xyzw vf01, vf04, vf03x   # vf01 = qOut + ((vf00 - qOut) * T)
	"
	:
	:
	"f" (T)         // %0
	:
	"$8"
	);

    // Get squared distance of new quaternion
    // Temp = qOut.X*qOut.X + qOut.Y*qOut.Y + qOut.Z*qOut.Z + qOut.W*qOut.W;
    // from above vf01 = qOut
    asm __volatile__(
    "
	vmul.xyzw	vf02,vf01,vf01
	vaddy.x		vf02,vf02,vf02
	vaddz.x		vf02,vf02,vf02
	vaddw.x		vf02,vf02,vf02

	qmfc2		$2,vf02
	mtc1		$2,%0

	"
	:
	"=f" (Temp)      // %0
	:
	:
	"$2"
	);

    // Use home-baked polynomial to compute 1/sqrt(Dist2)
    // since we know the range is 0.707 >= Dist2 <= 1.0
    // we'll split in half.
    if( Temp<0.857f )
        Temp = (((0.699368f)*Temp) + -1.819985f)*Temp + 2.126369f;    //0.0000792
    else
        Temp = (((0.454012f)*Temp) + -1.403517f)*Temp + 1.949542f;    //0.0000373

    // Renormalize and put into out quaternion
    // qOut *= Temp
    // from above vf01   = qOut
    asm __volatile__(
    "
    mfc1		$8,%1               # vf02   = Temp
    qmtc2		$8,vf02

	vmulx.xyzw  vf01,vf01,vf02      # vf01 = qOut * Temp

	sqc2		vf01,0x0(%0)        # qOut = Result
	"
	:
	:
	"r" (&qOut),     // %0
	"f" (Temp)   // %1
	:
	"$8"
	);



#else
    f32     Dist2;
    f32     OneOverL;

    // Determine if quats are further than 90 degrees -
    // If dot is negative flip one of the quaterions
    if( Qa.W < 0.0f )
    {
       qOut.X = -Qa.X;
       qOut.Y = -Qa.Y;
       qOut.Z = -Qa.Z;
       qOut.W = -Qa.W;
    }
    else
    {
       qOut.X = +Qa.X;
       qOut.Y = +Qa.Y;
       qOut.Z = +Qa.Z;
       qOut.W = +Qa.W;
    }

    // Compute interpolated values
    qOut.X = qOut.X + T*(0 - qOut.X);
    qOut.Y = qOut.Y + T*(0 - qOut.Y);
    qOut.Z = qOut.Z + T*(0 - qOut.Z);
    qOut.W = qOut.W + T*(1 - qOut.W);

    // Get squared distance of new quaternion
    Dist2 = qOut.X*qOut.X + qOut.Y*qOut.Y + qOut.Z*qOut.Z + qOut.W*qOut.W;

    // Use home-baked polynomial to compute 1/sqrt(Dist2)
    // since we know the range is 0.707 >= Dist2 <= 1.0
    // we'll split in half.
    if( Dist2<0.857f )
        OneOverL = (((0.699368f)*Dist2) + -1.819985f)*Dist2 + 2.126369f;    //0.0000792
    else
        OneOverL = (((0.454012f)*Dist2) + -1.403517f)*Dist2 + 1.949542f;    //0.0000373

    // Renormalize and put into out quaternion
    qOut.X *= OneOverL ;
    qOut.Y *= OneOverL ;
    qOut.Z *= OneOverL ;
    qOut.W *= OneOverL ;

#endif  //#ifdef TARGET_PS2
}


quaternion BlendIdentityFast( const quaternion& Qa, f32 T )
{
   f32 Dot;
   f32 Dist2;
   f32 OneOverL;
   f32 x1,y1,z1,w1;
   
   // Determine if quats are further than 90 degrees
   Dot = Qa.W ;

   // If dot is negative flip one of the quaterions
   if( Dot < 0.0f )
   {
       x1 = -Qa.X;
       y1 = -Qa.Y;
       z1 = -Qa.Z;
       w1 = -Qa.W;
   }
   else
   {
       x1 = +Qa.X;
       y1 = +Qa.Y;
       z1 = +Qa.Z;
       w1 = +Qa.W;
   }

   // Compute interpolated values
   x1 = x1 + T*(0.f - x1);
   y1 = y1 + T*(0.f - y1);
   z1 = z1 + T*(0.f - z1);
   w1 = w1 + T*(1.0f - w1);

   // Get squared distance of new quaternion
   Dist2 = x1*x1 + y1*y1 + z1*z1 + w1*w1;

   // Use home-baked polynomial to compute 1/sqrt(Dist2)
   // since we know the range is 0.707 >= Dist2 <= 1.0
   // we'll split in half.

   if( Dist2<0.857f )
      OneOverL = (((0.699368f)*Dist2) + -1.819985f)*Dist2 + 2.126369f;    //0.0000792
   else
      OneOverL = (((0.454012f)*Dist2) + -1.403517f)*Dist2 + 1.949542f;    //0.0000373

   // Renormalize 
   return quaternion( x1*OneOverL, y1*OneOverL, z1*OneOverL, w1*OneOverL );
}








//==============================================================================
// KeySet
//==============================================================================

// Constructor
key_set::key_set()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(key_set) & 15) == 0) ;

	// Flags
	m_Flags.NeverAnimate        = FALSE ;

	// Misc
	m_AnimWeight	            = 1.0f ;

	// Clear lists
    m_Flags.PosKeysCompressed   = FALSE ;
	m_PosKeys		            = NULL ;
	m_nPosKeys	                = 0 ;

    m_Flags.RotKeysCompressed   = FALSE ;
	m_RotKeys		            = NULL ;
	m_nRotKeys	                = 0 ;

    m_VisKeys                   = NULL ;
    m_nVisKeys                  = 0 ;
}

//==============================================================================

// Destructor
key_set::~key_set()
{
	// Cleanup
	if (m_PosKeys)
		delete [] m_PosKeys ;

	if (m_RotKeys)
		delete [] m_RotKeys ;

    if (m_VisKeys)
        delete [] m_VisKeys ;
}

//==============================================================================

// Copy a key set
key_set&  key_set::operator =  ( key_set& KeySet )
{
    s32 i ;

    // Must not have gone through compression yet...
    ASSERT(!KeySet.m_Flags.PosKeysCompressed) ;
    ASSERT(!KeySet.m_Flags.RotKeysCompressed) ;

    // Can't copy self!
    ASSERT(this != &KeySet) ;

	// Copy attributes
	m_Flags		 = KeySet.m_Flags ;
	m_AnimWeight = KeySet.m_AnimWeight ;

    // Create and setup new pos keys
    SetPosKeyCount(KeySet.GetPosKeyCount()) ;
    for (i = 0 ; i < KeySet.GetPosKeyCount() ; i++)
        SetPosKey(i, KeySet.GetPosKey(i)) ;

    // Create and setup new rot keys
    SetRotKeyCount(KeySet.GetRotKeyCount()) ;
    for (i = 0 ; i < KeySet.GetRotKeyCount() ; i++)
        SetRotKey(i, KeySet.GetRotKey(i)) ;

    // Create and setup new vis keys
    SetVisKeyCount(KeySet.GetVisKeyCount()) ;
    for (i = 0 ; i < KeySet.GetVisKeyCount() ; i++)
        SetVisKey(i, KeySet.GetVisKey(i)) ;

    return *this ;
}

//==============================================================================

// Returns amount of bytes used
s32 key_set::MemoryUsed()
{
    s32 Total = sizeof(key_set) ;

    // Pos key memory
    if (m_Flags.PosKeysCompressed)
        Total += sizeof(comp_pos_key_hdr) + (m_nPosKeys * sizeof(comp_pos_key)) ; // Info header + array of 8bit components
    else
        Total += (m_nPosKeys   * sizeof(vector3)) ;     // Array of vectors

    // Rot key memory
    if (m_Flags.RotKeysCompressed)
        Total += sizeof(comp_rot_key_hdr) + (m_nRotKeys * sizeof(comp_rot_key)) ; // Info header + array of 8bit components
    else
        Total += (m_nRotKeys   * sizeof(quaternion)) ;  // Array of quaternions

    // Vis key memory
    Total += m_nVisKeys * sizeof(byte) ;

    return Total ;
}

//==============================================================================

// Initialization functions
void key_set::SetPosKeyCount(s32 Count)
{
    ASSERT(!m_Flags.PosKeysCompressed) ;

    // Delete old list
    if (m_PosKeys)
    {
        delete [] m_PosKeys ;
        m_PosKeys = NULL ;
    }

    // Allocate new list
    if (Count)
    {
        m_PosKeys = (byte*)new vector3[Count] ;
        ASSERT(m_PosKeys) ;
    }

    // Set new size
    m_nPosKeys = Count ;
}

//==============================================================================

void key_set::SetRotKeyCount(s32 Count)
{
    ASSERT(!m_Flags.RotKeysCompressed) ;

    // Delete old list
    if (m_RotKeys)
    {
        delete [] m_RotKeys ;
        m_RotKeys = NULL ;
    }

    // Allocate new list
    if (Count)
    {
        m_RotKeys = (byte*)new quaternion[Count] ;
        ASSERT(m_RotKeys) ;
    }

    // Set new size
    m_nRotKeys = Count ;
}

//==============================================================================

void key_set::SetVisKeyCount(s32 Count)
{
    // Delete old list
    if (m_VisKeys)
    {
        delete [] m_VisKeys ;
        m_VisKeys = NULL ;
    }

    // Allocate new list
    if (Count)
    {
        m_VisKeys = new byte[Count] ;
        ASSERT(m_VisKeys) ;
    }

    // Set new size
    m_nVisKeys = Count ;
}

//==============================================================================

vector3& key_set::GetPosKey(s32 Index)
{
    ASSERT(!m_Flags.PosKeysCompressed) ;

    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nPosKeys) ;

    return ((vector3 *)m_PosKeys)[Index] ;
}

//==============================================================================

void key_set::SetPosKey(s32 Index, vector3 vPos)
{
    ASSERT(!m_Flags.PosKeysCompressed) ;

    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nPosKeys) ;
    
    ((vector3*)m_PosKeys)[Index] = vPos ;
}

//==============================================================================

quaternion& key_set::GetRotKey(s32 Index)
{
    ASSERT(!m_Flags.RotKeysCompressed) ;

    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nRotKeys) ;
    
    return ((quaternion *)m_RotKeys)[Index] ;
}

//==============================================================================

void key_set::SetRotKey(s32 Index, quaternion qRot)
{
    ASSERT(!m_Flags.RotKeysCompressed) ;

    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nRotKeys) ;
    
    ((quaternion*)m_RotKeys)[Index] = qRot ;
}

//==============================================================================

void key_set::SetVisKey(s32 Index, f32 F)
{
    if (F < 0)
        F = 0 ;
    else
    if (F > 1)
        F = 1 ;

    ASSERT(F >= 0) ;
    ASSERT(F <= 1) ;
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nVisKeys) ;
    
    // Map into a byte...
    byte B = (byte)(F * 255.0f) ;
    m_VisKeys[Index] = B ;
}

//==============================================================================

f32 key_set::GetVisKey(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nVisKeys) ;
    
    // Map from a byte to 0->1
    byte B = m_VisKeys[Index] ;
    f32  F = (f32)B * (1.0f / 255.0f) ;
    return F ;
}

//==============================================================================

// Remove x,z translation from the root nodes
void key_set::RemoveXZTranslation()
{
    s32 i ;

    // Loop through all pos key frames
    for (i = 0 ; i < m_nPosKeys ; i++)
    {
        // Get key
        vector3 Pos = GetPosKey(i) ;
        
        // Remove XZ
        Pos.X = 0.0f ;
        Pos.Z = 0.0f ;

        // Set key
        SetPosKey(i, Pos) ;
    }
}

//==============================================================================

// Optimize memory usage by collapsing a bunch of same keys into one
void key_set::CollapseKeys()
{
#define ROT_EQUAL_EPSILON   (R_1 / 10.0f) 
#define POS_EQUAL_EPSILON   (0.0001f)

    s32   i ;

    // Keep info
    #ifdef X_ASSERT
    s32 MemBefore = MemoryUsed();
    #endif

    // Check to see if all position keys are the same or are identity
    if (m_nPosKeys)
    {
        vector3 Pos = GetPosKey(0) ;
        vector3 AveragePos = Pos ;
        xbool   OptimizePos = TRUE ;
        for (i = 1 ; i < m_nPosKeys ; i++)
        {
            AveragePos += GetPosKey(i) ;

            // Different?
            if ((Pos - GetPosKey(i)).LengthSquared() > SQR(POS_EQUAL_EPSILON))
            {
                OptimizePos = FALSE ;
                break ;
            }
        }

        // User average of all positions if collapsing
        AveragePos /= (f32)m_nPosKeys ;

        // Convert to just one pos key?
        if (OptimizePos)
        {
            // Identity?
            if (AveragePos.LengthSquared() <= SQR(POS_EQUAL_EPSILON))
                SetPosKeyCount(0) ;
            else
            {
                // Store single frame
                SetPosKeyCount(1) ;
                SetPosKey(0, AveragePos) ;
            }
        }
    }

    // Check to see if all rotation keys are the same or are identity
    if (m_nRotKeys)
    {
        quaternion Rot        = GetRotKey(0) ;
        quaternion AverageRot = Rot ;
        xbool      OptimizeRot = TRUE ;
        for (i = 1 ; i < m_nRotKeys ; i++)
        {
            // Compute average
            AverageRot.X += GetRotKey(i).X ;
            AverageRot.Y += GetRotKey(i).Y ;
            AverageRot.Z += GetRotKey(i).Z ;
            AverageRot.W += GetRotKey(i).W ;

            // Different?
            f32 Dot =   (Rot.X * GetRotKey(i).X)
                      + (Rot.Y * GetRotKey(i).Y)
                      + (Rot.Z * GetRotKey(i).Z)
                      + (Rot.W * GetRotKey(i).W) ;
            f32 Angle = x_acos(Dot) ;
            if (ABS(Angle) > ROT_EQUAL_EPSILON)
            {
                OptimizeRot = FALSE ;
                break ;
            }
        }

        // Compute average
        AverageRot.Normalize() ;

        // Convert to just one rot key?
        if (OptimizeRot)
        {
            // Identity? (ie. dot product with 0,0,0,1)
            f32 Angle = x_acos(AverageRot.W) ;
            if (Angle <= ROT_EQUAL_EPSILON)
                SetRotKeyCount(0) ;
            else
            {
                // Store single frame
                SetRotKeyCount(1) ;
                SetRotKey(0, AverageRot) ;
            }
        }
    }

    // Check to see if all visiblity keys are the same or 1 (opaque)
    if (m_nVisKeys)
    {
        f32 Vis = GetVisKey(0) ;
        f32 AverageVis = Vis ;
        xbool OptimizeVis = TRUE ;
        for (i = 1 ; i < m_nVisKeys ; i++)
        {
            // Compute average
            AverageVis += GetVisKey(i) ;

            // Different?
            if (Vis != GetVisKey(i))
            {
                OptimizeVis = FALSE ;
                break ;
            }
        }

        // Compute average
        AverageVis /= m_nVisKeys ;

        // Convert to just one vis key?
        if (OptimizeVis)
        {
            // 1?
            if (AverageVis == 1)
                SetVisKeyCount(0) ;
            else
            {
                // Store single frame
                SetVisKeyCount(1) ;
                SetVisKey(0, AverageVis) ;
            }
        }
    }

    // Make sure there isn't a logic error...
    ASSERT( MemoryUsed() <= MemBefore );
}

//==============================================================================

// Puts keys into compressed 8 bit per component format
void key_set::Compress()
{
    s32     c,f ;
            
    s32     Size ;
    byte*   Data ;
            
    f32     Min[4], Max[4], Range[4], Scale[4], fValue, Error, fOrigValue ;
    s32     iValue ;

    // Keep info
    #ifdef X_ASSERT
    s32 MemBefore = MemoryUsed();
    #endif

    //==========================================================================
    // Compress positions
    //==========================================================================

    // Only compress if compressed size is smaller than uncompressed size!
    Size = sizeof(comp_pos_key_hdr) + (sizeof(comp_pos_key)*m_nPosKeys) ;
    if (Size < (m_nPosKeys * (s32)sizeof(vector3)))
    {
        // Calculate min,max and range for all components
        for (c = 0 ; c < 3 ; c++)
        {
            Min[c] = Max[c] = *(&(GetPosKey(0).X) + c) ;
            for (f = 0 ; f < m_nPosKeys ; f++)
            {
                Min[c] = MIN(Min[c], *(&(GetPosKey(f).X) + c)) ;
                Max[c] = MAX(Max[c], *(&(GetPosKey(f).X) + c)) ;
            }

            Range[c] = Max[c] - Min[c] ;
            if (Range[c] != 0)
                Scale[c] = Range[c] / 255.0f ;
            else
                Scale[c] = 0 ;
        }

        // Total up error of compressing
        Error = 0 ;
        for (f = 0 ; f < m_nPosKeys ; f++)
        {
            for (c = 0 ; c < 3 ; c++)
            {
                // When range is 0, there is no error!
                if (Range[c] != 0)
                {
                    // Get original value
                    fOrigValue = *(&(GetPosKey(f).X) + c) ;

                    // Compress component
                    fValue = fOrigValue ;
                    fValue -= Min[c] ;  // -Min
                    fValue /= Scale[c]; // /Scale ;
                    iValue = (u32)(fValue+0.5f) ;
                    ASSERT(iValue <= 255) ;

                    // Decompress component
                    fValue = (f32)iValue ;
                    fValue *= Scale[c] ;
                    fValue += Min[c] ;

                    // Calc error as % of range
                    Error += ABS(fOrigValue - fValue) ;/// Range[c] ;
                }
            }
        }
        Error /= m_nPosKeys ;   // Take average of error        
        //shape::printf("     PosKeyError:%f\n", Error) ;

        // Only compress if error is small enough...
        if (Error < 0.1f)
        {
            // Allocate memory
            Data = new byte[Size] ;

            comp_pos_key_hdr* Hdr  = (comp_pos_key_hdr*)Data ;
            comp_pos_key*     Keys = (comp_pos_key*)(Data + sizeof(comp_pos_key_hdr)) ;

            // Store info in data so it can be decompressed
            for (c = 0 ; c < 3 ; c++)
            {
                Hdr->Min[c]   = Min[c] ;
                Hdr->Scale[c] = Scale[c] ;
            }

            // Compress components
            for (f = 0 ; f < m_nPosKeys ; f++)
            {
                for (c = 0 ; c < 3 ; c++)
                {
                    // Compress component
                    fValue = *(&(GetPosKey(f).X) + c) ;
                    fValue -= Min[c] ;   // -Min
                    fValue /= Scale[c] ; // /Scale ;
                    iValue = (u32)(fValue+0.5f) ;
                    ASSERT(iValue <= 255) ;

                    // Store in data
                    Keys[f][c] = (u8)iValue ;
                }
            }

            // Alrighty - replace current data with the compressed data
            delete [] m_PosKeys ;
            m_PosKeys = Data ;

            // Flag pos keys are compressed!
            m_Flags.PosKeysCompressed = TRUE ;
        }
    }

    //==========================================================================
    // Compress rotations
    //==========================================================================

    // Only compress if compressed size is smaller than uncompressed size!
    Size = sizeof(comp_rot_key_hdr) + (sizeof(comp_rot_key)*m_nRotKeys) ;
    if (Size < (m_nRotKeys * (s32)sizeof(quaternion)))
    {
        // Calculate min,max and range for all components
        for (c = 0 ; c < 4 ; c++)
        {
            Min[c] = Max[c] = *(&(GetRotKey(0).X) + c) ;
            for (f = 0 ; f < m_nRotKeys ; f++)
            {
                Min[c] = MIN(Min[c], *(&(GetRotKey(f).X) + c)) ;
                Max[c] = MAX(Max[c], *(&(GetRotKey(f).X) + c)) ;
            }

            Range[c] = Max[c] - Min[c] ;
            if (Range[c] != 0)
                Scale[c] = Range[c] / 255.0f ;
            else
                Scale[c] = 0 ;
        }

        // Total up error of compressing
        Error = 0 ;
        for (f = 0 ; f < m_nRotKeys ; f++)
        {
            for (c = 0 ; c < 4 ; c++)
            {
                // When range is 0, there is no error!
                if (Range[c] != 0)
                {
                    // Get original value
                    fOrigValue = *(&(GetRotKey(f).X) + c) ;

                    // Compress component
                    fValue = fOrigValue ;
                    fValue -= Min[c] ;  // -Min
                    fValue /= Scale[c]; // /Scale ;
                    iValue = (u32)(fValue+0.5f) ;
                    ASSERT(iValue <= 255) ;

                    // Decompress component
                    fValue = (f32)iValue ;
                    fValue *= Scale[c] ;
                    fValue += Min[c] ;

                    // Calc error as % of range
                    Error += ABS(fOrigValue - fValue) ;/// Range[c] ;
                }
            }
        }
        Error /= m_nRotKeys ;   // Take average of error        
        //shape::printf("     RotKeyError:%f\n", Error) ;

        // Only compress if error is small enough...
        if (Error < 0.1f)
        {
            // Allocate memory
            Data = new byte[Size] ;

            comp_rot_key_hdr* Hdr  = (comp_rot_key_hdr*)Data ;
            comp_rot_key*     Keys = (comp_rot_key*)(Data + sizeof(comp_rot_key_hdr)) ;

            // Store in data so it can be decompressed
            for (c = 0 ; c < 4 ; c++)
            {
                Hdr->Min[c]   = Min[c] ;
                Hdr->Scale[c] = Scale[c] ;
            }

            // Compress components
            for (f = 0 ; f < m_nRotKeys ; f++)
            {
                for (c = 0 ; c < 4 ; c++)
                {
                    // Compress component
                    fValue = *(&(GetRotKey(f).X) + c) ;
                    fValue -= Min[c] ;
                    fValue /= Scale[c] ;
                    iValue = (u32)(fValue+0.5f) ;
                    ASSERT(iValue <= 255) ;

                    // Store in data
                    Keys[f][c] = (u8)iValue ;
                }
            }

            // Alrighty - replace current data with the compressed data
            delete [] m_RotKeys ;
            m_RotKeys = Data ;

            // Flag rot keys are compressed!
            m_Flags.RotKeysCompressed = TRUE ;
        }
    }

    // Validate all the quaternions
    for (f = 0 ; f < m_nRotKeys ; f++)
    {
        quaternion RotKey ;
        GetRotKey(f, RotKey) ;
        quaternion ValidQ(RotKey) ;
        (void)ValidQ ;
    }

    // Make sure there isn't a logic error...
    ASSERT( MemoryUsed() <= MemBefore );
}

//==============================================================================
// Indexed key access functions
//==============================================================================

// Returns position key
void key_set::GetPosKey(s32 Index, vector3& PosKey)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nPosKeys) ;

    if (m_Flags.PosKeysCompressed)
    {
        // Decompress on the fly...
        comp_pos_key_hdr&   Hdr = *(comp_pos_key_hdr*)m_PosKeys ;
        comp_pos_key&       Key = ((comp_pos_key*)(m_PosKeys + sizeof(comp_pos_key_hdr)))[Index] ;
        PosKey.X = Hdr.Min.X + (Hdr.Scale.X * Key.X) ;
        PosKey.Y = Hdr.Min.Y + (Hdr.Scale.Y * Key.Y) ;
        PosKey.Z = Hdr.Min.Z + (Hdr.Scale.Z * Key.Z) ;
    }
    else
        PosKey = ((vector3 *)m_PosKeys)[Index] ;
}

//==============================================================================

// Use for PS2 optimization (leaves PosKey.W untouched)
void key_set::GetPosKey(s32 Index, vector4& PosKey)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nPosKeys) ;

    if (m_Flags.PosKeysCompressed)
    {
        #if defined(TARGET_PS2) && defined(PS2_USE_VU0)

            // Lookup scale and min
            asm __volatile__(
            "
	        lqc2		vf02,0x10(%0)       # vf02 = Scale
	        lqc2		vf03,0x0(%0)        # vf03 = Min
            vmulaw.xyz  acc, vf03, vf00w    # acc  = Min
	        "
	        :
	        :
	        "r" (m_PosKeys)     // %0
	        );

            // Lookup compressed key frame
            comp_pos_key& Key = ((comp_pos_key*)(m_PosKeys + sizeof(comp_pos_key_hdr)))[Index] ;
            PosKey.X = (f32)Key.X ;
            PosKey.Y = (f32)Key.Y ;
            PosKey.Z = (f32)Key.Z ;

            // Scale up and add base
            asm __volatile__(
            "
            lqc2		vf01,0x0(%0)        # vf01 = PosKey
       
            vmadd.xyzw  vf01, vf01, vf02    # vf01 = Min + (PosKey * Scale)
       
            sqc2		vf01,0x0(%0)        # Store result
	        "
	        :
	        :
	        "r" (&PosKey)      // %0
	        );

        #else

            // Decompress on the fly...
            comp_pos_key_hdr&   Hdr = *(comp_pos_key_hdr*)m_PosKeys ;
            comp_pos_key&       Key = ((comp_pos_key*)(m_PosKeys + sizeof(comp_pos_key_hdr)))[Index] ;
            PosKey.X = Hdr.Min.X + (Hdr.Scale.X * Key.X) ;
            PosKey.Y = Hdr.Min.Y + (Hdr.Scale.Y * Key.Y) ;
            PosKey.Z = Hdr.Min.Z + (Hdr.Scale.Z * Key.Z) ;

        #endif
    }
    else
    {
        PosKey.X = ((vector3 *)m_PosKeys)[Index].X ;
        PosKey.Y = ((vector3 *)m_PosKeys)[Index].Y ;
        PosKey.Z = ((vector3 *)m_PosKeys)[Index].Z ;
    }
}

//==============================================================================

// Returns quaternion key
void key_set::GetRotKey(s32 Index, quaternion& RotKey)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nRotKeys) ;

    if (m_Flags.RotKeysCompressed)
    {
        #if defined(TARGET_PS2) && defined(PS2_USE_VU0)

            // Lookup scale and min
            asm __volatile__(
            "
	        lqc2		vf02,0x10(%0)       # vf02 = Scale
	        lqc2		vf03,0x0(%0)        # vf03 = Min
            vmulaw.xyzw acc, vf03, vf00w    # acc  = Min
	        "
	        :
	        :
	        "r" (m_RotKeys)     // %0
	        );

            // Lookup compressed key frame
            comp_rot_key& Key = ((comp_rot_key*)(m_RotKeys + sizeof(comp_rot_key_hdr)))[Index] ;
            RotKey.X = (f32)Key.X ;
            RotKey.Y = (f32)Key.Y ;
            RotKey.Z = (f32)Key.Z ;
            RotKey.W = (f32)Key.W ;

            // Scale up and add base
            asm __volatile__(
            "
            lqc2		vf01,0x0(%0)        # vf01 = RotKey
            vmadd.xyzw  vf01, vf01, vf02    # vf01 = Min + (RotKey * Scale)
        
            sqc2		vf01,0x0(%0)        # Store result
	        "
	        :
	        :
	        "r" (&RotKey)      // %0
	        );

        #else

            // Decompress on the fly...
            comp_rot_key_hdr&   Hdr = *(comp_rot_key_hdr*)m_RotKeys ;
            comp_rot_key&       Key = ((comp_rot_key*)(m_RotKeys + sizeof(comp_rot_key_hdr)))[Index] ;
            RotKey.X = Hdr.Min.X + (Hdr.Scale.X * (f32)Key.X) ;
            RotKey.Y = Hdr.Min.Y + (Hdr.Scale.Y * (f32)Key.Y) ;
            RotKey.Z = Hdr.Min.Z + (Hdr.Scale.Z * (f32)Key.Z) ;
            RotKey.W = Hdr.Min.W + (Hdr.Scale.W * (f32)Key.W) ;

        #endif
    }
    else
        RotKey = ((quaternion *)m_RotKeys)[Index] ;
}

//==============================================================================

// Returns visiblity key
void key_set::GetVisKey(s32 Index, f32& VisKey)
{
    VisKey = GetVisKey(Index) ;
}

//==============================================================================

//==============================================================================
// Animation key access functions

// Given time A, time B, and the fraction between them (from 0->1, where 0 = time A, 1 = time B)
// these functions return the interpolated key frame
//==============================================================================

void key_set::GetPosKey(vector3 &vPos, s32 tA, s32 tB, f32 Frac)
{
    vector3 PosA ;
    vector3 PosB ;

    // Multiple keys?
    if (m_nPosKeys > 1)
    {
        // Lookup and blend between two closest keys
        GetPosKey(tA, PosA) ;
        GetPosKey(tB, PosB) ;
        BlendLinear(PosA, PosB, Frac, vPos) ;
    }
    else
    // Single key?
    if (m_nPosKeys == 1)
    {
        GetPosKey(0, vPos) ;
    }
    else
        vPos.Zero() ;   // Identity key
}

//==============================================================================

void key_set::GetPosKey(vector4 &vPos, s32 tA, s32 tB, f32 Frac)
{
    // For PS2 VU0 alignment
    static vector4 PosA PS2_ALIGNMENT(16) ;
    static vector4 PosB PS2_ALIGNMENT(16) ;

    // Multiple keys?
    if (m_nPosKeys > 1)
    {
        // Lookup and blend between two closest keys
        GetPosKey(tA, PosA) ;
        GetPosKey(tB, PosB) ;
        BlendLinear(PosA, PosB, Frac, vPos) ;
    }
    else
    // Single key?
    if (m_nPosKeys == 1)
    {
        GetPosKey(0, vPos) ;
    }
    else
        vPos.Zero() ;   // Identity key
}

//==============================================================================

void key_set::GetRotKey(quaternion &qRot, s32 tA, s32 tB, f32 Frac)
{
    // For PS2 VU0 alignment
    static quaternion RotA PS2_ALIGNMENT(16) ;
    static quaternion RotB PS2_ALIGNMENT(16) ;

    // Multiple keys?
    if (m_nRotKeys > 1)
    {
        // Lookup and blend between two closest keys
        GetRotKey(tA, RotA) ;
        GetRotKey(tB, RotB) ;
	    BlendFast(RotA, RotB, Frac, qRot) ;
    }
    else
    // Single key?
    if (m_nRotKeys == 1)
    {
        GetRotKey(0, qRot) ;
    }
    else
        qRot.Identity() ;   // Identity key
}

//==============================================================================

// Returns visiblity key
void key_set::GetVisKey(f32 &Vis, s32 tA, s32 tB, f32 Frac)
{
    f32 A,B ;

    // Opaque?
    if (m_nVisKeys == 0)
        Vis = 1.0f ;
    else
    // Single key?
    if (m_nVisKeys == 1)
        Vis = GetVisKey(0) ;
    else
    {
        // Blend between frames
        A = GetVisKey(tA) ;
        B = GetVisKey(tB) ;
        Vis = A + (Frac * (B - A)) ;
    }
}

//==============================================================================

void key_set::ReadWrite(shape_bin_file &File)
{
	// Read/Write attributes
	File.ReadWrite(m_Flags) ;
	File.ReadWrite(m_AnimWeight) ;

    // Read/Write rotation keys
    if (m_Flags.RotKeysCompressed)
    {
        // Is f32 info header + array of 8bit components
        u8* Data  = (u8*)m_RotKeys ;
        s32 Count = sizeof(comp_rot_key_hdr) + (m_nRotKeys * sizeof(comp_rot_key)) ;

        File.ReadWriteStructArray(Data, Count) ;

        m_RotKeys  = (byte*)Data ;
        m_nRotKeys = (Count - sizeof(comp_rot_key_hdr)) / sizeof(comp_rot_key) ;
    }
    else
    {
        // Is array of quaternions
        quaternion* Data  = (quaternion*)m_RotKeys ;
        s32         Count = m_nRotKeys ;

        File.ReadWriteStructArray(Data, Count) ;

        m_RotKeys  = (byte*)Data ;
        m_nRotKeys = Count ;
    }

    // Read/Write position keys
    if (m_Flags.PosKeysCompressed)
    {
        // Is f32 info header + array of 8bit components
        u8* Data  = (u8*)m_PosKeys ;
        s32 Count = sizeof(comp_pos_key_hdr) + (m_nPosKeys * sizeof(comp_pos_key)) ;

        File.ReadWriteStructArray(Data, Count) ;

        m_PosKeys  = (byte*)Data ;
        m_nPosKeys = (Count - sizeof(comp_pos_key_hdr)) / sizeof(comp_pos_key) ;
    }
    else
    {
        // Is array of vectors
        vector3* Data  = (vector3*)m_PosKeys ;
        s32      Count = m_nPosKeys ;

        File.ReadWriteStructArray(Data, Count) ;

        m_PosKeys  = (byte*)Data ;
        m_nPosKeys = Count ;
    }

    // Visiblity keys
    File.ReadWriteStructArray(m_VisKeys, m_nVisKeys) ;
}


//==============================================================================
// Animation class
//==============================================================================

// Constructor
anim::anim()
{
    // Must be 16 byte aligned for shape_bin_file_class
    ASSERT((sizeof(anim) & 15) == 0) ;

    // Init attributes
    x_strcpy(m_Name, "undefined") ;
    m_Index                 = -1 ;
    
    m_Type                  = 0 ;
    m_AdditiveRefType       = 0 ;
    
    m_Weight                = 1.0f ;
                            
    m_Fps                   = 30.0f ;
    m_nFrames               = 0 ;
    m_nLoopToFrame          = 0 ;

    m_Flags.Additive        = 0 ;
    m_Flags.RemoveRootTrans = 0 ;
    m_Flags.Compress        = 1 ;
    m_Flags.HasAnimatedVis  = 0 ;

	m_KeySets	            = NULL ;
	m_nKeySets	            = 0 ;
}

//==============================================================================

// Construct from a portion of another anim, given the anim info
anim::anim(anim &SourceAnim, user_data_anim_info &AnimInfo)
{
    // Copy attributes
    x_strcpy(m_Name, SourceAnim.GetName()) ;
    m_TimeStamp         = SourceAnim.GetTimeStamp() ;
                        
    m_Type              = AnimInfo.Type ;
    m_AdditiveRefType   = AnimInfo.AdditiveRefType ;

    m_Weight            = AnimInfo.Weight ;
                        
    m_Fps               = AnimInfo.Fps ;
                        
    m_nFrames           = MAX(AnimInfo.StartFrame, AnimInfo.EndFrame) - MIN(AnimInfo.StartFrame, AnimInfo.EndFrame) + 1 ;
    m_nLoopToFrame      = AnimInfo.LoopToFrame - MIN(AnimInfo.StartFrame, AnimInfo.EndFrame) ;

    ASSERT(m_nFrames >= 1) ;
    ASSERT(m_nLoopToFrame < m_nFrames) ;

    m_Flags.Additive        = AnimInfo.Additive ;
    m_Flags.RemoveRootTrans = AnimInfo.RemoveRootTrans ;
    m_Flags.Compress        = AnimInfo.Compress ;

    // Create new key sets
    m_nKeySets      = SourceAnim.GetKeySetCount() ;
    m_KeySets       = new key_set[m_nKeySets] ;
    ASSERT(m_KeySets) ;

    // Setup all the key sets
    for (s32 i = 0 ; i < m_nKeySets ; i++)
    {
		// Copy original key set (to get flags etc)
		m_KeySets[i] = SourceAnim.GetKeySet(i) ;

        // Create new keys
        m_KeySets[i].SetPosKeyCount(m_nFrames) ;
        m_KeySets[i].SetRotKeyCount(m_nFrames) ;
        m_KeySets[i].SetVisKeyCount(m_nFrames) ;

        // Copy the keys
        key_set& SourceKeySet = SourceAnim.GetKeySet(i) ;
        s32      SourceFrame  = AnimInfo.StartFrame ;
        for (s32 j = 0 ; j < m_nFrames ; j++)
        {
            // Get source pos key
            vector3 Pos ;
            if (SourceFrame < SourceKeySet.GetPosKeyCount())
                Pos = SourceKeySet.GetPosKey(SourceFrame) ;
            else
                Pos = SourceKeySet.GetPosKey(0) ;

            // Get source rot key
            quaternion Rot ;
            if (SourceFrame < SourceKeySet.GetRotKeyCount())
                Rot = SourceKeySet.GetRotKey(SourceFrame) ;
            else
                Rot = SourceKeySet.GetRotKey(0) ;

            // Get source vis key
            f32 Vis ;
            if (SourceFrame < SourceKeySet.GetVisKeyCount())
                Vis = SourceKeySet.GetVisKey(SourceFrame) ;
            else
                Vis = SourceKeySet.GetVisKey(0) ;

            // Store new keys
            m_KeySets[i].SetPosKey(j, Pos) ;
            m_KeySets[i].SetRotKey(j, Rot) ;
            m_KeySets[i].SetVisKey(j, Vis) ;

            // Next source frame
            if (AnimInfo.EndFrame >= AnimInfo.StartFrame)
                SourceFrame++ ;
            else
                SourceFrame-- ;
        }
    }
}

//==============================================================================

// Destructor
anim::~anim()
{
	// Cleanup
	if (m_KeySets)
		delete [] m_KeySets ;	
}

//==============================================================================

void anim::SetName(const char *Name)
{
    char    DRIVE   [X_MAX_DRIVE];
    char    DIR     [X_MAX_DIR];
    char    FNAME   [X_MAX_FNAME];
    char    EXT     [X_MAX_EXT];
    char    PATH    [X_MAX_PATH];

    // Break it up
    x_splitpath(Name,DRIVE,DIR,FNAME,EXT);

    // Build name
    x_makepath( PATH, NULL, NULL, FNAME, NULL);

    ASSERT(x_strlen(PATH) < (s32)sizeof(m_Name));
    x_strcpy(m_Name, PATH) ;
}

//==============================================================================

// Copy an animation
anim&  anim::operator = ( anim& Anim )
{
    s32 i ;

    // Can't copy self!
    ASSERT(this != &Anim) ;

    // Copy attributes
    x_strcpy(m_Name, Anim.GetName()) ;                  // Filename
    m_TimeStamp         = Anim.GetTimeStamp() ;         // Time stamp from .skel file
                        
    m_Type              = Anim.GetType() ;              // Type of animation
    m_AdditiveRefType   = Anim.GetAdditiveRefType() ;   // Reference type for additive animation

    m_Weight            = Anim.GetWeight() ;            // Weight used for choosing same types
    m_Flags             = Anim.GetFlags() ;             // Animation flags
                                                        
    m_Fps               = Anim.GetFps() ;               // Frames per sec anim was recorded at
                                                        
	m_nFrames           = Anim.GetFrameCount() ;        // Total number of frames in animation
	m_nLoopToFrame      = Anim.GetLoopToFrame() ; 	    // Frame to loop back to after completion

    // Create and copy keys
    SetKeySetCount(Anim.GetKeySetCount()) ;
    ASSERT(m_nKeySets == Anim.GetKeySetCount()) ;
    for (i = 0 ; i < m_nKeySets ; i++)
        m_KeySets[i] = Anim.GetKeySet(i) ;

    // Make sure anims are the same!
    ASSERT( MemoryUsed() == Anim.MemoryUsed() ) ;

    return *this ;
}

//==============================================================================

// Returns amount of bytes used
s32 anim::MemoryUsed()
{
    s32 Size = sizeof(anim) ;

    for (s32 i = 0 ; i < m_nKeySets ; i++)
        Size += m_KeySets[i].MemoryUsed() ;

    return Size ;
}

//==============================================================================

// Allocates key set list
void anim::SetKeySetCount(s32 nKeySets)
{
    // Delete old key sets
    if (m_KeySets)
    {
        delete [] m_KeySets ;
        m_KeySets = NULL ;
    }

    // Allocate new key sets
    if (nKeySets)
    {
        m_KeySets = new key_set[nKeySets] ;
        ASSERT(m_KeySets) ;
    }

    // Set new size
    m_nKeySets = nKeySets ;
}

//==============================================================================

// Removes key set from list
void anim::DeleteKeySet(s32 Index)
{
    s32 i ;

    // Make sure node is not used and index is valid
    ASSERT(Index >= 0) ;
    ASSERT(Index < GetKeySetCount()) ;

    // Create new key set list
    s32     nNewKeySets = GetKeySetCount()-1 ;
    key_set *NewKeySets = new key_set[nNewKeySets] ;

    // Copy before deletion key_set
    for (i = 0 ; i < Index ; i++)
        NewKeySets[i] = m_KeySets[i] ;

    // Copy after deletion key_set
    for (i = (Index+1) ; i < m_nKeySets ; i++)
        NewKeySets[i-1] = m_KeySets[i] ;

    // Delete old list of key_sets
    delete [] m_KeySets ;

    // Finally - use the new key_sets list
    m_KeySets  = NewKeySets ;
    m_nKeySets = nNewKeySets ;
}

//==============================================================================

// Applies parent keys to given child keys (in preparation for deleting the parent key set)
void anim::ApplyKeySetToChildKeySet(s32 Parent, s32 Child)
{
    s32 i ;

    // Get key sets
    key_set &ParentKeySet = GetKeySet(Parent) ;
    key_set &ChildKeySet  = GetKeySet(Child) ;

    // If parent has rot animation, and the child doesn't - we must make the child have animation!
    if ((ChildKeySet.GetRotKeyCount() == 1) && (ParentKeySet.GetRotKeyCount() > 1))
    {
        // Get child key
        quaternion ChildRot = ChildKeySet.GetRotKey(0) ;

        // Create new child keys
        ChildKeySet.SetRotKeyCount(ParentKeySet.GetRotKeyCount()) ;

        // Setup keys
        for (i = 0 ; i < ChildKeySet.GetRotKeyCount() ; i++)
            ChildKeySet.SetRotKey(i, ChildRot) ;
    }

    // If parent has pos animation, and the child doesn't - we must make the child have animation!
    if ((ChildKeySet.GetPosKeyCount() == 1) && (ParentKeySet.GetPosKeyCount() > 1))
    {
        // Get child key
        vector3 ChildPos = ChildKeySet.GetPosKey(0) ;

        // Create new child keys
        ChildKeySet.SetPosKeyCount(ParentKeySet.GetPosKeyCount()) ;

        // Setup keys
        for (i = 0 ; i < ChildKeySet.GetPosKeyCount() ; i++)
            ChildKeySet.SetPosKey(i, ChildPos) ;
    }

    // Loop over all child keys
    for (i = 0 ; i < MAX(ChildKeySet.GetPosKeyCount(), ChildKeySet.GetRotKeyCount()) ; i++)
    {
        // Get parent pos key
        vector3 ParentPos ;
        if (ParentKeySet.GetPosKeyCount() == 1)
            ParentPos = ParentKeySet.GetPosKey(0) ;
        else
            ParentPos = ParentKeySet.GetPosKey(i) ;

        // Get parent rot key
        quaternion ParentRot ;
        if (ParentKeySet.GetRotKeyCount() == 1)
            ParentRot = ParentKeySet.GetRotKey(0) ;
        else
            ParentRot = ParentKeySet.GetRotKey(i) ;

        // Get child pos key
        vector3 ChildPos ;
        if (ChildKeySet.GetPosKeyCount() == 1)
            ChildPos = ChildKeySet.GetPosKey(0) ;
        else
            ChildPos = ChildKeySet.GetPosKey(i) ;

        // Get child rot key
        quaternion ChildRot ;
        if (ChildKeySet.GetRotKeyCount() == 1)
            ChildRot = ChildKeySet.GetRotKey(0) ;
        else
            ChildRot = ChildKeySet.GetRotKey(i) ;

        // Now from 
        //  MParent = ParentRot + ParentPos ;
        //  MChild  = ChildRot + ChildPos ;
        //  mFinal = mParent * mChild ;
        // we can see that:

        // Set new child pos key
        if (i < ChildKeySet.GetPosKeyCount())
        {
            ChildPos = ParentPos + (ParentRot * ChildPos) ;
            ChildKeySet.SetPosKey(i, ChildPos) ;
        }

        // Set new child rot key
        if (i < ChildKeySet.GetRotKeyCount())
        {
            ChildRot = ParentRot * ChildRot ;
            ChildKeySet.SetRotKey(i, ChildRot) ;
        }
    }
}

//==============================================================================


// Makes animation additive by removing the first frames orientation from all the keys
// See the playback code in ShapeInstance.cpp for more details
//
// NOTE: Without baking in the removal of the first frame, the following would be needed:
//
//  Apply delta to final anim orient
//      qAnimRot = qAdditiveRot * qRefRot * qAnimRot ;
//      vAnimPos = vAdditivePos + vRefPos + vAnimPos ;
//
// Where    qAnimRot,	  vAnimPos      = current node orient
//          qAdditiveRot, vAdditivePos  = current additive anim orient
//          qRefRot,	  vRefPos       = inverse orient of first frame of additive anim
//
void anim::ConvertToAdditive(anim &RootAnim)
{
    s32 i,j ;

    // Make sure animation hierarchies match!
    ASSERT(GetKeySetCount() == RootAnim.GetKeySetCount()) ;

    // Loop through all key sets (one per node)
    for (i = 0 ; i < GetKeySetCount() ; i++)
    {
        // Get root anim key set
        key_set &RootKeySet = RootAnim.GetKeySet(i) ;
        ASSERT(RootKeySet.GetRotKeyCount()) ;
        ASSERT(RootKeySet.GetPosKeyCount()) ;
        
        // Get key set
        key_set &KeySet = GetKeySet(i) ;
        ASSERT(KeySet.GetRotKeyCount()) ;
        ASSERT(KeySet.GetPosKeyCount()) ;

        // Get first frame orientation of root anim
        quaternion  qInvRot = RootKeySet.GetRotKey(0) ;
        vector3     vInvPos = RootKeySet.GetPosKey(0) ;
        //quaternion  qInvRot = RootKeySet.GetRotKey(RootKeySet.GetRotKeyCount()-1) ;
        //vector3     vInvPos = RootKeySet.GetPosKey(RootKeySet.GetPosKeyCount()-1) ;

        // Invert
        qInvRot.Invert() ;
        vInvPos *= -1.0f ;

        // Remove first frame from rotation keys
        for (j = 0 ; j < KeySet.GetRotKeyCount() ; j++)
            KeySet.SetRotKey(j, KeySet.GetRotKey(j) * qInvRot) ;

        // Remove first frame from position keys
        for (j = 0 ; j < KeySet.GetPosKeyCount() ; j++)
            KeySet.SetPosKey(j, KeySet.GetPosKey(j) + vInvPos) ;
    }
}

//==============================================================================

// Scale anim to new fps
void anim::Scale(f32 NewFps)
{
    s32 i,j ;

    // Skip if just one frame
    if (m_nFrames == 1)
        return ;

    // Keep info
    #ifdef X_ASSERT
    s32 MemBefore = MemoryUsed();
    #endif

    // Setup sample scaler (0=no length, 1=full sample)
    f32 Scale        = NewFps / m_Fps ;
    f32 OneOverScale = 1.0f / Scale ;

    // Copy anim
    anim SourceAnim ;
    SourceAnim = *this ;

    // Scale anim info
    m_Fps           *= Scale ;

    // Scale frames
    m_nFrames       = (s32)((f32)m_nFrames * Scale) ;
    if (m_nFrames < 1)
        m_nFrames = 1 ; 

    // Scale loop to frame
	m_nLoopToFrame  = (s32)((f32)m_nLoopToFrame * Scale) ;
    if (m_nLoopToFrame >= m_nFrames)
        m_nLoopToFrame = m_nFrames - 1 ;

    // Scale keysets
    for (i = 0 ; i < m_nKeySets ; i++)
    {
        key_set& SourceKeySet = SourceAnim.GetKeySet(i) ;

        // Setup new counts
        m_KeySets[i].SetPosKeyCount(m_nFrames) ;            
        m_KeySets[i].SetRotKeyCount(m_nFrames) ;            

        // Loop through all frames
        for (j = 0 ; j < m_nFrames ; j++)
        {
            // Get time expressed as original anim time
            f32 t    = (f32)j * OneOverScale ;

            // Setup key access vars
            s32 tA, tB ;
            f32 Frac ;
            SourceAnim.SetupKeyAccessVars(t, tA, tB, Frac) ;

            // Get keys
            vector3    PosKey ;
            quaternion RotKey ;
            SourceKeySet.GetPosKey(PosKey, tA, tB, Frac) ;
            SourceKeySet.GetRotKey(RotKey, tA, tB, Frac) ;

            // Set keys in this scaled anim
            m_KeySets[i].SetPosKey(j, PosKey) ;
            m_KeySets[i].SetRotKey(j, RotKey) ;
        }
    }

    // The same keys may have been re-generated so collapse them...
    CollapseKeys() ;

    // Make sure there isn't a logic error...
    ASSERT( MemoryUsed() <= MemBefore );
}

//==============================================================================

// Collapse keys with the same value
void anim::CollapseKeys()
{
    s32 i ;

    // Collapse keys with the same value
    for (i = 0 ; i < m_nKeySets ; i++)
        m_KeySets[i].CollapseKeys() ;
}

//==============================================================================

// Returns TRUE if animation contains no movement!
xbool anim::IsStatic()
{
    s32 i ;

    // Simple case
    if (m_nFrames == 1)
        return TRUE ;

    // If any keyset contains more than one frame, then it's animated
    for (i = 0 ; i < m_nKeySets ; i++)
    {
        // Any keys animated?
        if ((m_KeySets[i].GetPosKeyCount() > 1) || (m_KeySets[i].GetRotKeyCount() > 1))
           return FALSE ;
    }

    // Animation is static
    return TRUE ;
}

//==============================================================================

// Optimize memory usage
void anim::Compress()
{
    s32 i ;

    // Keep info
    s32 MemBefore = MemoryUsed() ;

    // Scale anim
    if ((m_nFrames > 1) && (m_Fps > 10.0f))
        Scale(10.0f) ;

    // Finally, compress all keysets
    for (i = 0 ; i < m_nKeySets ; i++)
        m_KeySets[i].Compress() ;

    // Find name filename for nicer text output
    char *Name = &m_Name[x_strlen(m_Name)] ;
    while((Name != m_Name) && (Name[-1] != '/') && (Name[-1] != '\\'))
        Name-- ;

    // Show info
    s32 MemAfter = MemoryUsed() ;
    ASSERT(MemAfter <= MemBefore) ;
    shape::printf(" Compressed anim %s from %d bytes (%dK) to %d bytes (%dK) Ratio %2.1f :: 1.0\n",
                  Name,
                  MemBefore, (1023+MemBefore)/1024,
                  MemAfter,  (1023+MemAfter)/1024,
                  (f32)MemBefore / (f32)MemAfter) ;
}

//==============================================================================

// Sets up vars ready for accessing keys
void anim::SetupKeyAccessVars(f32 Frame, s32& tA, s32& tB, f32& Frac)
{
	// Setup time vars, ready for blending
	tA   = (s32)Frame ;		// Current integer frame time
	Frac = Frame - tA ;     // Get float fraction from 0->1

    // Loop frame A?
	if (tA >= m_nFrames)
		tA -= (m_nFrames - m_nLoopToFrame) ;

	// Loop frame B?
    tB = tA+1 ;			// Next integer frame time
	if (tB >= m_nFrames)
		tB -= (m_nFrames - m_nLoopToFrame) ;

    // Make sure we are within frame range...
    if (tA < 0)
        tA = 0 ;
    if (tA >= m_nFrames)
        tA = m_nFrames-1 ;

    if (tB < 0)
        tB = 0 ;
    if (tB >= m_nFrames)
        tB = m_nFrames-1 ;
}

//==============================================================================

void anim::ReadWrite(shape_bin_file &File)
{
    // Read/Write attributes
    File.ReadWrite(m_Name) ;
    File.ReadWrite(m_TimeStamp) ;
   
    File.ReadWrite(m_Index) ;
    
    File.ReadWrite(m_Type) ;
    File.ReadWrite(m_AdditiveRefType) ;

    File.ReadWrite(m_Flags) ;
    File.ReadWrite(m_Weight) ;
    File.ReadWrite(m_Fps) ;
    
    // Read/Write anim info
    File.ReadWrite(m_nFrames) ;
	File.ReadWrite(m_nLoopToFrame) ;

    // Read/Write key sets
    File.ReadWriteClassArray(m_KeySets, m_nKeySets) ;
}

//==============================================================================


//==============================================================================
// Animation state
//==============================================================================

// Constructor
anim_state::anim_state()
{
    // Reset list pointers
    m_Next = NULL ;
    m_ID   = -1 ;

	// Default to no anim
	m_pAnim = NULL ;
	m_Frame = 0.0f ;
    m_Speed = 1.0f ;
	m_Weight = 1.0f ;
    m_AnimPlayed = 0 ;
}

//==============================================================================

// Sets the current anim
void anim_state::SetAnim(anim *pAnim)
{
	// Re-start anim?
	if (m_pAnim != pAnim)
	{
		m_pAnim = pAnim ;
		m_Frame = 0.0f ;
        m_AnimPlayed = 0 ;
	}
}

//==============================================================================

// Sets up animation info structure ready for looking up key frames
void anim_state::SetupAnimInfo(anim_info& AnimInfo)
{
    // Must be anim!
    ASSERT(m_pAnim) ;

    // Setup key access vars
    m_pAnim->SetupKeyAccessVars(m_Frame, AnimInfo.tA, AnimInfo.tB, AnimInfo.Frac) ;

    // Start with first key set
    AnimInfo.KeySet = &m_pAnim->GetKeySet(0) ;
}

//==============================================================================

// Sets the current position of animation playback
void anim_state::SetFrame(f32 Frame)  
{ 
    ASSERT(m_pAnim) ;
    
    // Range check...
    if( Frame < 0.0f )
        Frame = 0.0f;
    else
    if( Frame > m_pAnim->GetFrameCount() )
        Frame = (f32)m_pAnim->GetFrameCount() ;
    
    // Must be within anim range?
    ASSERT(Frame >= 0.0f) ;
    ASSERT(Frame <= m_pAnim->GetFrameCount()) ;
    
    m_Frame = Frame ;
}

//==============================================================================

// Sets the current position of animation playback (use 0->1, where 0=start frame, 1=end frame)
void anim_state::SetFrameParametric(f32 Frame)  
{ 
    // Must have an anim
    ASSERT(m_pAnim) ;

    // Get number of frames-1
    f32 Frames = (f32)(m_pAnim->GetFrameCount() - 1) ;
    if( Frames <= 0.0f )   // Static anim?
    {
        m_Frame = 0.0f ;
        return ;
    }
    
    // Range check...
    if( Frame < 0.0f )
        Frame = 0.0f;
    else
    if( Frame > 1.0f)
        Frame = 1.0f ;
    
    // Set the frame        
    m_Frame = Frame * Frames ;
}

//==============================================================================

// Gets the current position of animation playback (use 0->1, where 0=start frame, 1=end frame)
f32 anim_state::GetFrameParametric( void )
{ 
    // Must have an anim
    ASSERT(m_pAnim) ;

    // Get number of frames-1
    f32 Frames = (f32)(m_pAnim->GetFrameCount() - 1) ;
    if( Frames <= 0.0f )   // Static anim?
        return 0.0f ;

    // Get parametric frame
    f32 ParametricFrame = m_Frame / Frames ;

    // Range check...
    if( ParametricFrame < 0.0f )
        ParametricFrame = 0.0f;
    else
    if( ParametricFrame > 1.0f)
        ParametricFrame = 1.0f ;

    // Return valid frame
    return ParametricFrame ;
}

//==============================================================================

// Advance to next frame in current anim
void anim_state::Advance(f32 DeltaTime)
{
	// Advance animation?
	if (m_pAnim)
	{
        // No frames?
        if (m_pAnim->GetFrameCount() == 0)
            m_Frame = 0 ;
        else
        {
            // Calc anim frame increment
            f32 AnimDeltaTime = m_pAnim->GetFps() * DeltaTime * m_Speed ;

		    // Goto next frame
		    m_Frame += AnimDeltaTime ;
            
		    // Playing forwards?
		    if (AnimDeltaTime > 0.0f)
		    {
                // Gone past end of anim?
			    while(m_Frame >= m_pAnim->GetFrameCount())
                {
                    // Stay on last frame or loop?
				    if (m_pAnim->GetLoopToFrame() == (m_pAnim->GetFrameCount()-1))
                        m_Frame = (f32)m_pAnim->GetLoopToFrame() ;
                    else
    				    m_Frame -= (f32)(m_pAnim->GetFrameCount() - m_pAnim->GetLoopToFrame()) ;

                    // Flag anim has looped
                    m_AnimPlayed++ ;
                }

                // Range check
                if (m_Frame < 0.0f)
                    m_Frame = 0.0f ;
		    }
		    else
		    {
                // Gone beyond front of anim?
			    while(m_Frame < 0.0f)
                {
                    // Stay on last frame or loop?
				    if (m_pAnim->GetLoopToFrame() == (m_pAnim->GetFrameCount()-1))
                        m_Frame = 0 ;
                    else
				        m_Frame += (f32)(m_pAnim->GetFrameCount() - m_pAnim->GetLoopToFrame()) ;

                    // Flag anim has looped
                    m_AnimPlayed++ ;
                }

                // Range check
                if (m_Frame >= (f32)m_pAnim->GetFrameCount())
                    m_Frame = (f32)m_pAnim->GetFrameCount() - 1.0f ;
		    }

            // Make sure frame is valid
            //ASSERT(m_Frame >= 0.0f) ;
            //ASSERT(m_Frame < m_pAnim->GetFrameCount()) ;
	    }
    }
}

//==============================================================================



/*


//==============================================================================
// Anim stream class used to compress a stream of floats
//==============================================================================

class anim_stream
{
// Data
private:
    byte*   m_Data ;            // Data stream of values
    s32     m_nSamples ;        // Total samples
    f32     m_StartPos ;        // Start value of stream

// Functions
public:

    anim_stream() ;
    ~anim_stream() ;

    u32 BitsNeeded(u32 Value) ;

    s32 F32ToS32(f32 fValue, f32 Min, f32 Scale, s32 Bits) ;
    f32 S32ToF32(s32 iValue, f32 Min, f32 Scale, s32 Bits) ;
    f32 Compress(f32 fValue, f32 Min, f32 Scale, s32 Bits) ;

    s32 Compress   (f32* Values, s32 nValues, s32 Step, s32 Bits) ;
} ;



anim_stream::anim_stream()
{
    m_Data       = NULL ;
    m_nSamples   = 0 ;
    m_StartPos   = 0 ;
}

anim_stream::~anim_stream()
{
    if (m_Data)
        delete [] m_Data ;
}

u32 anim_stream::BitsNeeded(u32 Value)
{
    u32 BitsNeeded=32 ;
    u32 BitMask=(1<<31) ;

    if (Value == 0)
        return 0 ;

    while((Value & BitMask) == 0)
    {
        BitMask >>= 1 ;
        BitsNeeded-- ;
    }

    return BitsNeeded ;
}

s32 anim_stream::F32ToS32(f32 fValue, f32 Min, f32 Scale, s32 Bits)
{   
    fValue -= Min ;
    fValue *= Scale ;
    fValue *= (f32)((1 << Bits)-1) ;
    
    return (s32)fValue ;
}

f32 anim_stream::S32ToF32(s32 iValue, f32 Min, f32 Scale, s32 Bits)
{
    f32 fValue = (f32)iValue ;

    fValue /= (f32)((1 << Bits)-1) ;
    fValue /= Scale ;
    fValue += Min ;
    
    return fValue ;
}

f32 anim_stream::Compress(f32 fValue, f32 Min, f32 Scale, s32 Bits)
{
    s32 iValue = F32ToS32(fValue, Min, Scale, Bits) ;
    return S32ToF32(iValue, Min, Scale, Bits) ;
}


s32 anim_stream::Compress(f32* Values, s32 nValues, s32 Step, s32 Bits)
{
    s32 i,j ;
    
    f32 StartPos = *Values ;

#define DERIVS  3

    f32 PosList[1024] ;
    s32 DerivCompList[DERIVS][1024] ;  // Pos+Vel+Accel
    s32 DerivMin[DERIVS], DerivMax[DERIVS], DerivRange[DERIVS], DerivBits[DERIVS] ;

    ASSERT(nValues <= 1024) ;

    // Pass 1 - grab pos info
    f32 PosMin = *Values ;
    f32 PosMax = *Values ;
    for (i = 0 ; i < nValues ; i++)
    {
        PosList[i] = *(f32*)((u32)Values + (Step * i)) ;
        PosMin     = MIN(PosMin, PosList[i]) ;
        PosMax     = MAX(PosMax, PosList[i]) ;
    }
    f32 PosRange = PosMax - PosMin ;
    f32 PosScale = 1.0f / PosRange ;

    // Calculate compressed positions (Deriv 0)
    DerivBits[0]  = Bits ;
    for (i = 0 ; i < nValues ; i++)
        DerivCompList[0][i] = F32ToS32(PosList[i], PosMin, PosScale, Bits) ;

    // Calculate rest of compressed derivs
    for (i = 1 ; i < DERIVS ; i++)
    {
        DerivMin[i] = 0 ;
        DerivMax[i] = 0 ;
        for (j = 0 ; j < nValues ; j++)
        {
            DerivCompList[i][j] = DerivCompList[i-1][(j+1) % nValues] - DerivCompList[i-1][j] ;

            DerivMin[i]           = MIN(DerivMin[i], DerivCompList[i][j]) ;
            DerivMax[i]           = MAX(DerivMax[i], DerivCompList[i][j]) ;
        }
        DerivRange[i] = DerivMax[i] - DerivMin[i] ;
        DerivBits[i]  = BitsNeeded(DerivRange[i]) ;
    }

    // Decompress last deriv stream
    s32 DerivStreamToUse = DERIVS-1 ;
    
    // Initialize deriv values
    s32 DerivValue[DERIVS] ;
    for (i = 0 ; i < DerivStreamToUse ; i++)
        DerivValue[i] = DerivCompList[i][0] ;

    // Decompress stream...
    for (i = 0 ; i < nValues ; i++)
    {
        // Just using pos stream?
        if (DerivStreamToUse == 0)
        {
            // Store pos and goto next
            *Values = S32ToF32(DerivCompList[0][i], PosMin, PosScale, Bits) ;
            Values = (f32*)((u32)Values + Step) ;
        }
        else
        {
             // Make sure calcualted position is as it should be in the compressed position stream!
            ASSERT(DerivCompList[0][i] == DerivValue[0]) ;

            // Store pos and goto next
            *Values = S32ToF32(DerivValue[0], PosMin, PosScale, Bits) ;
            Values = (f32*)((u32)Values + Step) ;
        
            // Calc next vel and next pos
            for (j = 0 ; j < (DerivStreamToUse-1) ; j++)
                DerivValue[j] += DerivValue[j+1] ;
            DerivValue[j] += DerivCompList[DerivStreamToUse][i] ;
        }
    }

    // Pass6 - calc the total bits needed!
    s32 TotalBits = 0 ;
    for (i = 0 ; i < (DerivStreamToUse-1) ; i++)
        TotalBits += DerivBits[i] ; // Initial deriv values - pos, vel etc

    for (i = 0 ; i < nValues ; i++)
        //TotalBits += 4+BitsNeeded(AccelCompList[i]) ;
        //TotalBits += 3+BitsNeeded(VelCompList[i]) ;
        //TotalBits += BitsNeeded(Bits) + BitsNeeded(PosCompList[i]) ;
        TotalBits += DerivBits[DerivStreamToUse] ;
        //TotalBits += BitsNeeded(AccelBits) + BitsNeeded(AccelCompList[i]) ;

    // Return total bytes needed
    return (TotalBits + 7) / 8 ;
}

//==============================================================================

#if 0
// Returns amount of bytes used if anim were compressed
s32 key_set::TestCompress()
{
#if 1
    anim_stream AnimStream ;

    s32 TotalSize = sizeof(key_set) ;

    TotalSize += AnimStream.Compress(&(GetPosKey(0).X), m_nPosKeys, sizeof(vector3), 8) ;
    TotalSize += AnimStream.Compress(&(GetPosKey(0).Y), m_nPosKeys, sizeof(vector3), 8) ;
    TotalSize += AnimStream.Compress(&(GetPosKey(0).Z), m_nPosKeys, sizeof(vector3), 8) ;
    
    TotalSize += AnimStream.Compress(&(GetRotKey(0).X), m_nRotKeys, sizeof(quaternion), 8) ;
    TotalSize += AnimStream.Compress(&(GetRotKey(0).Y), m_nRotKeys, sizeof(quaternion), 8) ;
    TotalSize += AnimStream.Compress(&(GetRotKey(0).Z), m_nRotKeys, sizeof(quaternion), 8) ;
    //TotalSize += AnimStream.Compress(&(GetRotKey(0)->W), m_nRotKeys, sizeof(quaternion), 8) ;

    TotalSize += (m_nRotKeys+7)/8 ;   // 1 bit for sign!

    // Validate all the quaternions
    for (s32 i = 0 ; i < m_nRotKeys ; i++)
    {
        quaternion ValidQ(GetRotKey(i)) ;
    }

    // fix up quaternions
    //for (s32 i = 0 ; i < m_nRotKeys ; i++)
        //m_RotKeys[i].Normalize() ;

    return TotalSize ;
#else


#if 1
    // OPTIONS ARE TO EITHER HAVE ALL VALUES BE 8 BIT OR USE DECOMPRESSION BUFFERS (YUCK)
    s32 i,j ;

    s32 SavedBytes = 0 ;

    // Compress positions
    for (i = 0 ; i < 3 ; i++)
    {
        // Calculate range of component across all of anim
        f32 Min, Max ;
        Min = Max = *(&(GetPosKey(0)->X) + i) ;
        for (j = 0 ; j < m_nPosKeys ; j++)
        {
            Min = MIN(Min, *(&(GetPosKey(j)->X) + i)) ;
            Max = MAX(Max, *(&(GetPosKey(j)->X) + i)) ;
        }

        f32 Range = Max - Min ;
        f32 Scale, OneOverScale ;
        if (Range != 0)
        {
            Scale = 255.0f / Range ;
            OneOverScale = 1.0f / Scale ;
        }
        else
        {
            Scale = OneOverScale = 1 ;
            SavedBytes += m_nPosKeys ;  // Can skip this stream!
        }

        // Compress component
        for (j = 0 ; j < m_nPosKeys ; j++)
        {
            // Compress component
            f32 fPos = *(&(GetPosKey(j)->X) + i) ;
            fPos -= Min ;
            fPos *= Scale ;
            u32 iPos = (u32)(fPos+0.5f) ;
            ASSERT(iPos <= 255) ;

            // Decompress component
            fPos = (f32)iPos ;
            fPos *= OneOverScale ;
            fPos += Min ;

            // Store new compressed component
            *(&(GetPosKey(j)->X) + i) = fPos ;
        }
    }

    // Compress rotations - slowest, but most accurate method
    for (i = 0 ; i < 4 ; i++)
    {
        // Calculate range of component across all of anim
        f32 Min, Max ;
        Min = Max = *(&(GetRotKey(0)->X) + i) ;
        for (j = 0 ; j < m_nRotKeys ; j++)
        {
            Min = MIN(Min, *(&(GetRotKey(j)->X) + i)) ;
            Max = MAX(Max, *(&(GetRotKey(j)->X) + i)) ;
        }

        f32 Range = Max - Min ;
        f32 Scale, OneOverScale ;
        if (Range != 0)
        {
            Scale = 255.0f / Range ;
            OneOverScale = 1.0f / Scale ;
        }
        else
        {
            Scale = OneOverScale = 1 ;
            SavedBytes += m_nPosKeys ;  // Can skip this stream!
        }

        // Compress component
        for (j = 0 ; j < m_nRotKeys ; j++)
        {
            // Compress component
            f32 fRot = *(&(GetRotKey(j)->X) + i) ;
            fRot -= Min ;
            fRot *= Scale ;
            u32 iRot = (u32)(fRot+0.5f) ;
            ASSERT(iRot <= 255) ;

            // Decompress component
            fRot = (f32)iRot ;
            fRot *= OneOverScale ;
            fRot += Min ;

            // Store new compressed component
            *(&(GetRotKey(j)->X) + i) = fRot ;
        }
    }

    // Validate all the quaternions
    for (i = 0 ; i < m_nRotKeys ; i++)
    {
        quaternion ValidQ(m_RotKeys[i]) ;
    }
#endif


    // TO TEST:
    //flattern and unflattern position components to 8 bit per component using a min+scale?
    //flattern and unflattern quaternion components to 8 bit
    //if this works - then no decompression buffers are needed!

    SavedBytes = 0 ; // only saves about 3K!

    return sizeof(key_set) - SavedBytes +
           (m_nPosKeys   * 3) + (8*3) +  // 4 for min, 4 for scale for 3 pos components
           (m_nRotKeys   * 4) + (8*4) ;  // 4 for min, 4 for scale for 4 rot components
#endif

}

#endif


*/

