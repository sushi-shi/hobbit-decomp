//=========================================================================
//
// Linux_DRAW.CPP - Just a bunch of not much!
//
//=========================================================================
#include <xCore/Entropy/e_Engine.hpp>

#define OFFSET_X    (2048-(512/2))
#define OFFSET_Y    (2048-(512/2))

// Fixup for big screen shots
#define RES_X    512
#define RES_Y    448
#define OFFSET_X_3D    ( (2048-(512/2)) - (RES_X/2) + ((RES_X/2) * eng_ScreenShotSize()) )
#define OFFSET_Y_3D    ( (2048-(512/2)) - (RES_Y/2) + ((RES_Y/2) * eng_ScreenShotSize()) )


//==========================================================================
//#define FORCE_CLIPPING

#define NUM_VERTICES                (16384/sizeof(vert))            // Number of vertices in spad buffer
#define TRIGGER_POINTS              ((NUM_VERTICES/1)*1)            // Vertex indicies to trigger buffer dispatch
#define TRIGGER_LINES               ((NUM_VERTICES/2)*2)            // ...
#define TRIGGER_LINE_STRIPS         ((NUM_VERTICES/2)*2)            // ...
#define TRIGGER_TRIANGLES           ((NUM_VERTICES/3)*3)            // ...
#define TRIGGER_TRIANGLE_STRIPS     ((NUM_VERTICES/3)*3)            // ...
#define TRIGGER_QUADS               ((NUM_VERTICES/4)*4)            // ...
#define TRIGGER_RECTS               ((NUM_VERTICES/4))              // ...
#define NUM_SPRITES                 (16384/sizeof(sprite))

typedef void (*fnptr_dispatch)( void );
#define ALIGN __attribute__((aligned (16)))

//==========================================================================
// Structures
//==========================================================================


struct vert
{
    vector2 UV;
    vector3 Pos;
    u32     Color;
} ALIGN;

struct sprite
{
    vector3     Position;
    vector2     WH;
    xcolor      Color;
    vector2     UV0;
    vector2     UV1;
    radian      Rotation;
    xbool       IsRotated;
} ALIGN;

//==========================================================================
// Data
//==========================================================================

static vert*            s_Buffer = (vert*)(0x70000000);        
static vert*            s_BufferPos;
static fnptr_dispatch   s_pfnDispatch;          // Dispatch Function
static s32              s_nVertsInBuffer;       // Num of verts currently in
static s32              s_nTrigger;             // Num of verts to trigger dispatch

static sprite*          s_SpriteBuffer = (sprite*)(0x70000000);
static s32              s_nSprites;

static xbool            s_Initialized = TRUE;  // Becomes TRUE when Initialized
static xbool            s_bBegin = FALSE;       // TRUE when between begin/end

static draw_primitive   s_Primitive;            // Primitive Type, see enum draw_primitive
static u32              s_Flags;                // Flags, see defines
static xbool            s_Is2D;                 // TRUE for 2D mode
static xbool            s_IsTextured;           // TRUE for Textured mode
static xbool            s_IsAlpha;              // TRUE when using alpha
static xbool            s_IsWire;               // TRUE when using wireframe
static xbool            s_IsSTQ;                // TRUE when needs stq in transform
static xbool            s_IsZBuffered;

static matrix4          s_L2W ALIGN;                  // L2W matrix for draw
static matrix4          s_W2S[ENG_MAX_VIEWS] ALIGN;
static matrix4          s_L2S[ENG_MAX_VIEWS] ALIGN;
static matrix4          s_W2V[ENG_MAX_VIEWS] ALIGN;
static matrix4          s_V2S[ENG_MAX_VIEWS] ALIGN;

static const vector2*   s_pUVs;                 // Pointer to UV array
static s32              s_nUVs;                 // Number of elements
static s32              s_sUVs;                 // Stride of elements

static const xcolor*    s_pColors;              // Pointer to Color array
static s32              s_nColors;              // Number of elements
static s32              s_sColors;              // Stride of elements

static const vector3*   s_pVerts;               // Poitner to vertex array
static s32              s_nVerts;               // Number of elements
static s32              s_sVerts;               // Stride of elements

static vector2          s_UV;                   // Current UV
static u32              s_Color;                // Current Color in ps2 format
static s32              s_ViewID;               // Current view in use

static s32              s_ZBias=0;
static s32              ZBIAS_SCALAR = 1024;

s32 STAT_DrawLines=0;
s32 STAT_DrawPoints=0;

xtimer DrawTransformTime;
xtimer DrawDispatchTime;

//==========================================================================

void draw_SetZBias( s32 Bias )
{
    ASSERT( (Bias>=0) && (Bias<=16) );
    s_ZBias = (Bias*ZBIAS_SCALAR);
}

//==========================================================================

static
void draw_UpdateMatrices( void )
{
    const view* pView = eng_GetView( s_ViewID );
    s_W2V[s_ViewID] = pView->GetW2V();
    s_V2S[s_ViewID] = pView->GetV2S();
    s_W2S[s_ViewID] = pView->GetW2S();
    s_L2S[s_ViewID] = s_W2S[s_ViewID]*s_L2W;
}

//==========================================================================

static inline void LoadMatrixIntoRegisters(matrix4& m0)
{
}

//==========================================================================

static
void draw_TransformVerts( vert* pBuffer, matrix4& M, s32 NVerts )
{
}
//==========================================================================

static inline void LoadSpriteMatrixIntoRegisters(matrix4& m0, matrix4& m1)
{
}

//==========================================================================

static inline void draw_sincos( radian Angle, f32& Sine, f32& Cosine )
{
}

//==========================================================================
s32 SpriteZShift = 22;
void draw_DispatchSprites( void )
{
}


//==========================================================================

static
void draw_DispatchPoints( void )
{
}

//==============================================================================

inline xcolor ColorLerp( const xcolor& C0, const xcolor& C1, f32 T )
{
    xcolor XC;
    ASSERT( (T>=0.0f) && (T<=1.0f) );
    XC.R = (u8)((f32)C0.R + T*((f32)C1.R-(f32)C0.R));
    XC.G = (u8)((f32)C0.G + T*((f32)C1.G-(f32)C0.G));
    XC.B = (u8)((f32)C0.B + T*((f32)C1.B-(f32)C0.B));
    XC.A = (u8)((f32)C0.A + T*((f32)C1.A-(f32)C0.A));
    return XC;
}

//==========================================================================

xbool ClipLine( vert* pInV, vert* pOutV )
{
    // Transform from local to world
    pOutV[0].Pos = s_L2W.Transform(pInV[0].Pos);
    pOutV[1].Pos = s_L2W.Transform(pInV[1].Pos);
    pOutV[0].Color = pInV[0].Color;
    pOutV[1].Color = pInV[1].Color;

    // Get view planes
    const view* pView = eng_GetView(s_ViewID);
    const plane* pPlane = pView->GetViewPlanes(view::WORLD);

    for( s32 i=0; i<6; i++ )
    {
        f32 D0,D1;
        D0 = pPlane[i].Distance(pOutV[0].Pos);
        D1 = pPlane[i].Distance(pOutV[1].Pos);
        if( (D0<0) && (D1<0) ) return FALSE;
        if( (D0>=0) && (D1>=0) ) continue;
        f32 T = (0-D0)/(D1-D0);
        ASSERT( (T>=0.0f) && (T<=1.0f) );
        if( D0>D1 )
        {
            pOutV[1].Pos = pOutV[0].Pos + T*(pOutV[1].Pos-pOutV[0].Pos);
            pOutV[1].Color = ColorLerp( pOutV[0].Color, pOutV[1].Color, T );        
        }
        else
        {
            pOutV[0].Pos = pOutV[0].Pos + T*(pOutV[1].Pos-pOutV[0].Pos);
            pOutV[0].Color = ColorLerp( pOutV[0].Color, pOutV[1].Color, T );        
        }
    }

    // Transform and build verts
    draw_TransformVerts( pOutV, s_W2S[s_ViewID], 2 );

    return TRUE;
}

//==========================================================================

s32 ClipTriangle( vert* pInV, vert* pOutV )
{
    vert TempV[10];
    vert* pSrc = pOutV;
    vert* pDst = TempV;
    s32   nSrcPoints;
    s32   nDstPoints;
    s32 i;

    // Transform from local to world
    nSrcPoints = 3;
    for( i=0; i<3; i++ )
    {
        pSrc[i].Pos     = s_L2W.Transform( pInV[i].Pos );
        pSrc[i].Color   = pInV[i].Color;
        pSrc[i].UV      = pInV[i].UV;
    }

    // Get view planes
    const view* pView = eng_GetView(s_ViewID);
    const plane* pPlane = pView->GetViewPlanes(view::WORLD);

    for( s32 i=0; i<6; i++ )
    {
        s32 p0,p1;
        f32 D0,D1;
        nDstPoints = 0;

        // Loop through points in this ngon
        for( p0=0; p0<nSrcPoints; p0++ )
        {
            p1 = (p0+1)%nSrcPoints;
            D0 = pPlane[i].Distance(pSrc[p0].Pos);
            D1 = pPlane[i].Distance(pSrc[p1].Pos);

            // Check if we need to add point to front
            if( D0>=0 )
            {
                pDst[nDstPoints].Pos    = pSrc[p0].Pos;
                pDst[nDstPoints].Color  = pSrc[p0].Color;
                pDst[nDstPoints].UV     = pSrc[p0].UV;
                nDstPoints++;
            }

            // Check if edge spans clipping plane
            if( ((D0>=0)&&(D1<0)) || ((D0<0)&&(D1>=0)) )
            {
                // Compute point at crossing and add
                f32 t = (0-D0)/(D1-D0);
                ASSERT( (t>=0.0f) && (t<=1.0f) );
                pDst[nDstPoints].Pos    = pSrc[p0].Pos + t*(pSrc[p1].Pos-pSrc[p0].Pos);
                pDst[nDstPoints].UV     = pSrc[p0].UV  + t*(pSrc[p1].UV-pSrc[p0].UV);
                pDst[nDstPoints].Color  = ColorLerp( pSrc[p0].Color, pSrc[p1].Color, t );
                nDstPoints++;
            }
        }

        if( nDstPoints==0 ) return 0;
        vert* pTemp = pSrc;
        pSrc = pDst;
        pDst = pTemp;
        nSrcPoints = nDstPoints;
    }

    // Transform and build verts
    draw_TransformVerts( pOutV, s_W2S[s_ViewID], nSrcPoints );

    return nSrcPoints;
}

//==========================================================================

static
void draw_DispatchLines( void )
{
}

//==========================================================================

static
void draw_DispatchQuadsAndRects( void )
{
}

//==========================================================================

static
void draw_DispatchWireTriangles( void )
{
}
//==========================================================================

static
void draw_DispatchTriangles( void )
{
}

//==========================================================================

static
void draw_MainDispatch( void )
{
}

//==========================================================================

void draw_Begin( draw_primitive Primitive, u32 Flags )
{
	(void)Primitive;
	(void)Flags;
}


void draw_End( void )
{
}

//==========================================================================

void draw_SetL2W( const matrix4& L2W )
{
    s_L2W = L2W;
}

//==========================================================================

void draw_ClearL2W( void )
{
    s_L2W.Identity();
}

//==========================================================================

void draw_SetTexture( const xbitmap& Bitmap )
{
}

//==========================================================================

void draw_SetTexture( void )
{
}

//==========================================================================

void draw_DisableBilinear( void )
{
}

//==========================================================================

void draw_UV( const vector2& UV )
{
}

void draw_UV( f32 U, f32 V )
{
}

//==========================================================================

void draw_Color( const xcolor& Color )
{
}

void draw_Color( f32 R, f32 G, f32 B, f32 A )
{
}

//==========================================================================

void draw_Vertex( const vector3& Vertex )
{
}

void draw_Vertex( f32 X, f32 Y, f32 Z )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    // Setup vertex in buffer
    s_BufferPos->UV    = s_UV;
    s_BufferPos->Color = s_Color;
    s_BufferPos->Pos.Set(X,Y,Z);

    // Advance buffer pointer
    s_BufferPos++;
    s_nVertsInBuffer++;

    // Check if it is time to dispatch this buffer
    if( s_nVertsInBuffer == s_nTrigger )
        draw_MainDispatch();
}
//==========================================================================

void draw_UVs( const vector2* pUVs,    s32 Count, s32 Stride = sizeof(vector2) )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    s_pUVs = pUVs;
    s_nUVs = Count;
    s_sUVs = Stride;
}

//==========================================================================

void draw_Colors( const xcolor*  pColors, s32 Count, s32 Stride = sizeof(xcolor ) )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    s_pColors = pColors;
    s_nColors = Count;
    s_sColors = Stride;
}

//==========================================================================

void draw_Verts( const vector3* pVerts,  s32 Count, s32 Stride = sizeof(vector3) )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    s_pVerts = pVerts;
    s_nVerts = Count;
    s_sVerts = Stride;
}

//==========================================================================

void draw_Index( s32 Index )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    if( Index == -1 )
    {
        draw_MainDispatch();
    }
    else
    {
        // Setup vertex in buffer
        if( s_pUVs )
        {
            ASSERT( Index < s_nUVs );
            s_BufferPos->UV = s_pUVs[Index];
        }
        else
        {
            s_BufferPos->UV = s_UV;
        }

        // Get Color
        if( s_pColors )
        {
            ASSERT( Index < s_nColors );
            xcolor C = s_pColors[Index];
            
            C.R ^= C.B;
            C.B ^= C.R;
            C.R ^= C.B;

            if( s_IsTextured )
            {
                C.R >>= 1;
                C.G >>= 1;
                C.B >>= 1;
            }
            C.A >>= 1;

            s_BufferPos->Color = C;
        }
        else
        {
            s_BufferPos->Color = s_Color;
        }

        // Setup vertex in buffer
        ASSERT( Index < s_nVerts );
        s_BufferPos->Pos = s_pVerts[Index];

        // Advance buffer pointer
        s_BufferPos++;
        s_nVertsInBuffer++;

        // Check if it is time to dispatch this buffer
        if( s_nVertsInBuffer == s_nTrigger )
            draw_MainDispatch();
    }
}

//==========================================================================

void draw_Execute( const s16* pIndices, s32 NIndices )
{
    s32     i;

    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    // Loop through indices supplied
    for( i=0; i<NIndices; i++ )
    {
        draw_Index( pIndices[i] );
    }
}

//==========================================================================

void draw_Init( void )
{
    s_Initialized = TRUE;
    x_DebugMsg("Can fit %1d sprites\n",NUM_SPRITES);
}

//==========================================================================

void draw_Kill( void )
{
    s_Initialized = FALSE;
}

//==========================================================================

void    draw_Sprite     ( const vector3& Position,  // Hot spot (2D Left-Top), (3D Center)
                          const vector2& WH,        // (2D pixel W&H), (3D World W&H)
                          const xcolor&  Color )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_SPRITES );

    sprite* p = &s_SpriteBuffer[s_nSprites];


    p->IsRotated = FALSE;
    p->Position  = Position;
    p->WH        = WH;
    p->Color.R   = Color.B >>1;
    p->Color.G   = Color.G >>1;
    p->Color.B   = Color.R >>1;
    p->Color.A   = Color.A >>1;
    p->Rotation  = 0.0f;
    p->UV0.Set( 0.0f, 0.0f );
    p->UV1.Set( 1.0f, 1.0f );

    s_nSprites++;
    if( s_nSprites == NUM_SPRITES )
        draw_DispatchSprites();
}

//==========================================================================

void    draw_SpriteUV   ( const vector3& Position,  // Hot spot (2D Left-Top), (3D Center)
                          const vector2& WH,        // (2D pixel W&H), (3D World W&H)
                          const vector2& UV0,       // Upper Left   UV  [0.0 - 1.0]
                          const vector2& UV1,       // Bottom Right UV  [0.0 - 1.0]
                          const xcolor&  Color )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_SPRITES );

    sprite* p = &s_SpriteBuffer[s_nSprites];

    p->IsRotated = FALSE;
    p->Position  = Position;
    p->WH        = WH;
    p->UV0       = UV0;
    p->UV1       = UV1;
    p->Color.R   = Color.B >>1;
    p->Color.G   = Color.G >>1;
    p->Color.B   = Color.R >>1;
    p->Color.A   = Color.A >>1;
    p->Rotation  = 0.0f;

    s_nSprites++;
    if( s_nSprites == NUM_SPRITES )
        draw_DispatchSprites();
}

//==========================================================================

void    draw_SpriteUV   ( const vector3& Position,  // Hot spot (3D Center)
                          const vector2& WH,        // (3D World W&H)
                          const vector2& UV0,       // Upper Left   UV  [0.0 - 1.0]
                          const vector2& UV1,       // Bottom Right UV  [0.0 - 1.0]
                          const xcolor&  Color,     //
                                radian   Rotate )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_SPRITES );

    sprite* p = &s_SpriteBuffer[s_nSprites];

    p->IsRotated = TRUE;
    p->Position  = Position;
    p->WH        = WH;
    p->UV0       = UV0;
    p->UV1       = UV1;
    p->Color.R   = Color.B >>1;
    p->Color.G   = Color.G >>1;
    p->Color.B   = Color.R >>1;
    p->Color.A   = Color.A >>1;
    p->Rotation  = Rotate;

    s_nSprites++;
    if( s_nSprites == NUM_SPRITES )
        draw_DispatchSprites();
}


//==========================================================================

void    draw_OrientedQuad(const vector3& Pos0,
                          const vector3& Pos1,
                          const vector2& UV0,
                          const vector2& UV1,
                          const xcolor&  Color0,
                          const xcolor&  Color1,
                                f32      Radius )
{
    (void)Color0;
    (void)Color1;

    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_TRIANGLES );

    vector3 Dir = Pos1 - Pos0;
    if( !Dir.Normalize() )
        return;

    vector3 CrossDir = Dir.Cross( eng_GetActiveView(0)->GetPosition() - Pos0 );
    if( !CrossDir.Normalize() )
        return;

    CrossDir *= Radius;

//    draw_Color( Color1 );
    s_Color  = ((s32)(Color1.R/2))<< 0;
    s_Color |= ((s32)(Color1.G/2))<< 8;
    s_Color |= ((s32)(Color1.B/2))<<16;
    s_Color |= ((s32)(Color1.A/2))<<24;

    draw_UV( UV1.X, UV1.Y );    draw_Vertex( Pos1 + CrossDir );
    draw_UV( UV1.X, UV0.Y );    draw_Vertex( Pos1 - CrossDir );

    //    draw_Color( Color0 );
    s_Color  = ((s32)(Color0.R/2))<< 0;
    s_Color |= ((s32)(Color0.G/2))<< 8;
    s_Color |= ((s32)(Color0.B/2))<<16;
    s_Color |= ((s32)(Color0.A/2))<<24;

    draw_UV( UV0.X, UV0.Y );    draw_Vertex( Pos0 - CrossDir );
    draw_UV( UV0.X, UV0.Y );    draw_Vertex( Pos0 - CrossDir );
    draw_UV( UV0.X, UV1.Y );    draw_Vertex( Pos0 + CrossDir );

//    draw_Color( Color1 );
    s_Color  = ((s32)(Color1.R/2))<< 0;
    s_Color |= ((s32)(Color1.G/2))<< 8;
    s_Color |= ((s32)(Color1.B/2))<<16;
    s_Color |= ((s32)(Color1.A/2))<<24;

    draw_UV( UV1.X, UV1.Y );    draw_Vertex( Pos1 + CrossDir );
}

//==========================================================================

void    draw_OrientedQuad(const vector3& Pos0,
                          const vector3& Pos1,
                          const vector2& UV0,
                          const vector2& UV1,
                          const xcolor&  Color0,
                          const xcolor&  Color1,
                                f32      Radius0,
                                f32      Radius1)
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_TRIANGLES );

    vector3 Dir = Pos1 - Pos0;
    if( !Dir.Normalize() )
        return;

    vector3 CrossDir = Dir.Cross( eng_GetActiveView(0)->GetPosition() - Pos0 );
    if( !CrossDir.Normalize() )
        return;

    vector3 Cross0 = CrossDir * Radius0;
    vector3 Cross1 = CrossDir * Radius1;

//    draw_Color( Color1 );
    s_Color  = ((s32)(Color1.R/2))<< 0;
    s_Color |= ((s32)(Color1.G/2))<< 8;
    s_Color |= ((s32)(Color1.B/2))<<16;
    s_Color |= ((s32)(Color1.A/2))<<24;

    draw_UV( UV1.X, UV1.Y );    draw_Vertex( Pos1 + Cross1 );
    draw_UV( UV1.X, UV0.Y );    draw_Vertex( Pos1 - Cross1 );

//    draw_Color( Color0 );
    s_Color  = ((s32)(Color0.R/2))<< 0;
    s_Color |= ((s32)(Color0.G/2))<< 8;
    s_Color |= ((s32)(Color0.B/2))<<16;
    s_Color |= ((s32)(Color0.A/2))<<24;

    draw_UV( UV0.X, UV0.Y );    draw_Vertex( Pos0 - Cross0 );
    draw_UV( UV0.X, UV0.Y );    draw_Vertex( Pos0 - Cross0 );
    draw_UV( UV0.X, UV1.Y );    draw_Vertex( Pos0 + Cross0 );

//    draw_Color( Color1 );
    s_Color  = ((s32)(Color1.R/2))<< 0;
    s_Color |= ((s32)(Color1.G/2))<< 8;
    s_Color |= ((s32)(Color1.B/2))<<16;
    s_Color |= ((s32)(Color1.A/2))<<24;

    draw_UV( UV1.X, UV1.Y );    draw_Vertex( Pos1 + Cross1 );
}

//==========================================================================

void    draw_OrientedStrand (const vector3* pPosData,
                                   s32      NumPts,
                             const vector2& UV0,
                             const vector2& UV1,
                             const xcolor&  Color0,
                             const xcolor&  Color1,
                                   f32      Radius )
{
    s32 i;
    vector3 quad[6];        //  storage for a quad, plus an extra edge for averaging
    vector2 uv0, uv1;

    uv0 = UV0;
    uv1 = UV1;

    ASSERT( s_bBegin );
    ASSERT( s_Primitive == DRAW_TRIANGLES );

    for ( i = 0; i < NumPts - 1; i++ )
    {
        vector3 Dir = pPosData[i+1] - pPosData[i];
        if( !Dir.Normalize() )
            Dir(0,1,0);
        
        vector3 CrossDir = Dir.Cross( eng_GetActiveView(0)->GetPosition() - pPosData[i] );
        if( !CrossDir.Normalize() )
            CrossDir(1,0,0);

        CrossDir *= Radius;

        if ( i == 0 )
        {
            // first set, no point averaging necessary
            quad[ 0 ] =     pPosData[ i ] + CrossDir;
            quad[ 1 ] =     pPosData[ i ] - CrossDir;
            quad[ 2 ] =     pPosData[ i + 1 ] + CrossDir;
            quad[ 3 ] =     pPosData[ i + 1 ] - CrossDir;
        }
        else
        {
            vector3 tq1, tq2;

            tq1 = pPosData[ i ] + CrossDir;
            tq2 = pPosData[ i ] - CrossDir;

            // second set...average verts
            quad[ 2 ] =     ( quad[2] + tq1 ) / 2.0f;
            quad[ 3 ] =     ( quad[3] + tq2 ) / 2.0f;
            quad[ 4 ] =     pPosData[ i + 1 ] + CrossDir;
            quad[ 5 ] =     pPosData[ i + 1 ] - CrossDir;            
        }

        // render q0, q1, q2, and q3 then shift all of them down
        if ( i > 0 )
        {
            draw_Color( Color1 );
            draw_UV( uv1.X, uv1.Y );    draw_Vertex( quad[2] );
            draw_UV( uv1.X, uv0.Y );    draw_Vertex( quad[3] );
            draw_Color( Color0 );
            draw_UV( uv0.X, uv0.Y );    draw_Vertex( quad[1] );
            draw_UV( uv0.X, uv0.Y );    draw_Vertex( quad[1] );
            draw_UV( uv0.X, uv1.Y );    draw_Vertex( quad[0] );
            draw_Color( Color1 );
            draw_UV( uv1.X, uv1.Y );    draw_Vertex( quad[2] );

            // cycle the UV's
            uv0.X = uv1.X;
            uv1.X += ( UV1.X - UV0.X );

            quad[0] = quad[2];
            quad[1] = quad[3];
            quad[2] = quad[4];
            quad[3] = quad[5];
        }
        
    }

    // last edge...
    draw_Color( Color1 );
    draw_UV( UV1.X, UV1.Y );    draw_Vertex( quad[2] );
    draw_UV( UV1.X, UV0.Y );    draw_Vertex( quad[3] );
    draw_Color( Color0 );
    draw_UV( UV0.X, UV0.Y );    draw_Vertex( quad[1] );
    draw_UV( UV0.X, UV0.Y );    draw_Vertex( quad[1] );
    draw_UV( UV0.X, UV1.Y );    draw_Vertex( quad[0] );
    draw_Color( Color1 );
    draw_UV( UV1.X, UV1.Y );    draw_Vertex( quad[2] );

}

//==============================================================================

void draw_ClearZBuffer( const irect& Rect )
{
}

//==============================================================================
