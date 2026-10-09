//=========================================================================
//
// PS2_DRAW.CPP
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

struct header
{
    dmatag   DMA;       // DMA tag
    giftag   PRIMGIF;   // GIF for setting PRIM register
    s64      prim ;     // PRIM register
    s64      pad ;
    giftag   GIF;       // GIF for list of primitives
};

struct gsvert
{
    s64     T;  // Texture coordinates
    s64     C;  // Color
    s64     P;  // Position
};

struct vert
{
    gsvert  GS;
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
    asm __volatile__ ("
        lqc2 vf4, 0(%0)
        lqc2 vf5,16(%0)
        lqc2 vf6,32(%0)
        lqc2 vf7,48(%0)
        " : : "r" (&m0) );
}

//==========================================================================

struct veci4
{
    s32 X,Y,Z;
    f32 W;
} ALIGN;

static
void draw_TransformVerts( vert* pBuffer, matrix4& M, s32 NVerts )
{

    if( s_Is2D )
    {
        s32      C = NVerts;
        vert*    pV = pBuffer;

        while( C-- )
        {
            // Build binary u64s for GS
            s32 SPX = (s32)((pV->Pos.X+OFFSET_X)*16);
            s32 SPY = (s32)((pV->Pos.Y+OFFSET_Y)*16);
            s32 SPZ = (s32)pV->Pos.Z ;

            // Keep vert z? (NOTE: 0x01 means no clipping needed)
            if (s_Flags & DRAW_2D_KEEP_Z)
                pV->GS.P = SCE_GS_SET_XYZ( SPX, SPY, SPZ) | 0x01 ;
            else        
                pV->GS.P = SCE_GS_SET_XYZ( SPX, SPY, 0x00FFFFFF ) | 0x01;

            //pV->GS.C = pV->Color;
            //pV->GS.T = SCE_GS_SET_UV( ((s32)(pV->UV.X*16)), ((s32)(pV->UV.Y*16)) );
        
            // Build texturing STQ
            ((f32*)(&pV->GS.T))[0] = pV->UV.X; 
            ((f32*)(&pV->GS.T))[1] = pV->UV.Y; 

            // Build color and Q
            ((s32*)(&pV->GS.C))[0] = pV->Color;
            ((f32*)(&pV->GS.C))[1] = 1.0f;

            if( (SPX|SPY)&0xFFFF0000 )
                pV->GS.P &= ~((u64)1);

            #ifdef FORCE_CLIPPING
            pV->GS.P &= ~((u64)1);
            #endif

            pV++;
        }
    }
    else
    {
        s32      C;
        vert*    pV;
        volatile veci4    SP;

        pV   = pBuffer;
        C    = NVerts;
        LoadMatrixIntoRegisters( M );

        if( !s_IsSTQ )
        {
            while( C-- )
            {
                asm __volatile__ ("
                    lqc2            vf8, 0(%0)
                    vmulaw.xyzw     ACC, vf7,vf0
                    vmadday.xyzw    ACC, vf5,vf8
                    vmaddaz.xyzw    ACC, vf6,vf8
                    vmaddx.xyzw     vf12,vf4,vf8
                    vdiv            Q,vf0w,vf12w
	                vwaitq
	                vmulq.xyz       vf12,vf12,Q
                    vftoi4.xyz      vf12,vf12
                    sqc2            vf12,0(%1)
                    ": : "r" (&pV[0].Pos), "r" (&SP));


                // Build binary u64s for GS
                pV->GS.C = pV->Color;
                pV->GS.P = SCE_GS_SET_XYZ( SP.X, SP.Y, SP.Z+s_ZBias ) | 0x01;
            
                if( ((SP.X|SP.Y)&0xFFFF0000) || (SP.Z<0) )
                    pV->GS.P &= ~((u64)1);

                #ifdef FORCE_CLIPPING
                pV->GS.P &= ~((u64)1);
                #endif

                pV++;
            }
        }
        else
        {
            while( C-- )
            {
                asm __volatile__ ("
                    lqc2            vf8, 0(%0)
                    vmulaw.xyzw     ACC, vf7,vf0
                    vmadday.xyzw    ACC, vf5,vf8
                    vmaddaz.xyzw    ACC, vf6,vf8
                    vmaddx.xyzw     vf12,vf4,vf8
                    vdiv            Q,vf0w,vf12w
	                vwaitq
	                vmulq.xyz       vf12,vf12,Q
                    vmulq.w         vf12,vf0,Q
                    vftoi4.xyz      vf12,vf12
                    sqc2            vf12,0(%1)
                    ": : "r" (&pV[0].Pos), "r" (&SP) );

                // Build texturing STQ
                ((f32*)(&pV->GS.T))[0] = pV->UV.X * SP.W; 
                ((f32*)(&pV->GS.T))[1] = pV->UV.Y * SP.W; 

                // Build color and Q
                ((s32*)(&pV->GS.C))[0] = pV->Color;
                ((f32*)(&pV->GS.C))[1] = SP.W;

                // Build position
                pV->GS.P = SCE_GS_SET_XYZ( SP.X, SP.Y, SP.Z+s_ZBias ) | 0x01;
            
                if( ((SP.X|SP.Y)&0xFFFF0000) || (SP.Z<0) )
                    pV->GS.P &= ~((u64)1);

                #ifdef FORCE_CLIPPING
                pV->GS.P &= ~((u64)1);
                #endif

                // Move to next vert
                pV++;
            }
        }
    }
}
//==========================================================================

static inline void LoadSpriteMatrixIntoRegisters(matrix4& m0, matrix4& m1)
{
    asm __volatile__ ("
        lqc2 vf4,  0(%0)
        lqc2 vf5, 16(%0)
        lqc2 vf6, 32(%0)
        lqc2 vf7, 48(%0)
        lqc2 vf8,  0(%1)
        lqc2 vf9, 16(%1)
        lqc2 vf10,32(%1)
        lqc2 vf11,48(%1)
        " : : "r" (&m0), "r" (&m1) );
}

//==========================================================================

static inline void draw_sincos( radian Angle, f32& Sine, f32& Cosine )
{
    #define I_360   (1<<16)
    #define I_90    (I_360/4)
    #define I_180   (I_360/2)
    #define I_270   (I_90*3)

    s32 IAngle = ((s32)(Angle*(I_360/R_360)))&(I_360-1);
    s32 si,ci;

    if( IAngle >= I_180 )
    {
        if( IAngle >= I_270 ) { si =  IAngle - I_360; ci =  IAngle - I_270; }
        else                  { si = -IAngle + I_180; ci =  IAngle - I_270; }
    }
    else
    {
        if( IAngle >= I_90 )  { si = -IAngle + I_180; ci = -IAngle + I_90;  }
        else                  { si = IAngle;          ci = -IAngle + I_90;  }
    }

    f32 sq  = si*(R_360/I_360);
    f32 cq  = ci*(R_360/I_360);
    f32 sq2 = sq*sq;
    f32 cq2 = cq*cq;
    Sine   = (((0.00813767f*sq2) - 0.1666666f)*sq2 + 1)*sq;
    Cosine = (((0.00813767f*cq2) - 0.1666666f)*cq2 + 1)*cq;

    #undef I_360
    #undef I_90
    #undef I_180
    #undef I_270
}

//==========================================================================
s32 SpriteZShift = 22;
void draw_DispatchSprites( void )
{
    s32 i,j;

    if( s_nSprites==0 )
        return;

    header* pH = DLStruct(header); 
    s32 Count=0;

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_TRIANGLESTRIP,
                               1,               // shading method (flat, gouraud)
                               s_IsTextured,    // texture mapping (off, on)
                               0,               // fogging (off, on)
                               s_IsAlpha,       // alpha blending (off, on)
                               0,               // 1 pass anti-aliasing (off, on)
                               0,          // tex-coord spec method (STQ, UV)
                               0,               // context (1 or 2)
                               0);              // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 12, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    if( s_Is2D )
    {
        for( j=0; j<s_nSprites; j++ )
        {

            sprite* pSprite = s_SpriteBuffer + j;
            f32 w = pSprite->WH.X*16*0.5f;
            f32 h = pSprite->WH.Y*16*0.5f;
            s32 ScreenX = (s32)((pSprite->Position.X+OFFSET_X)*16);
            s32 ScreenY = (s32)((pSprite->Position.Y+OFFSET_Y)*16);

            s32 V0X,V1X;
            s32 V0Y,V1Y;

            // Construct points v0 and v1
            if( pSprite->IsRotated )
            {
                f32 s,c;
                draw_sincos( -pSprite->Rotation, s, c );
                V0X = (s32)(c*w - s*h);
                V0Y = (s32)(s*w + c*h);
                V1X = (s32)(c*w + s*h);
                V1Y = (s32)(s*w - c*h);
            }
            else
            {
                ScreenX += (s32)w;
                ScreenY += (s32)h;
                V0X = (s32)w;
                V0Y = (s32)h;
                V1X = (s32)w;
                V1Y = (s32)-h;
            }

            s32 SX[4];
            s32 SY[4];
            SX[0] = (s32)(ScreenX - V0X);
            SY[0] = (s32)(ScreenY - V0Y);
            SX[1] = (s32)(ScreenX - V1X); 
            SY[1] = (s32)(ScreenY - V1Y);
            SX[2] = (s32)(ScreenX + V0X);
            SY[2] = (s32)(ScreenY + V0Y);
            SX[3] = (s32)(ScreenX + V1X);
            SY[3] = (s32)(ScreenY + V1Y);

            s32 Clip = SX[0] | SY[0] | SX[1] | SY[1] | SX[2] | SY[2] | SX[3] | SY[3];
            if( Clip & 0xFFFF0000 )
                continue;

            u64 C = (u64)pSprite->Color | (((u64)0x3f800000)<<32);

            {
                u64* pOut = (u64*)(((u32)DLAlloc(8*12)) | 0x00000000);

                if (s_Flags & DRAW_2D_KEEP_Z)
                {
                    s32 SPZ = (s32)pSprite->Position.Z ;
                    pOut[ 2] = SCE_GS_SET_XYZ(SX[0],SY[0],SPZ);
                    pOut[ 5] = SCE_GS_SET_XYZ(SX[1],SY[1],SPZ);
                    pOut[ 8] = SCE_GS_SET_XYZ(SX[3],SY[3],SPZ);
                    pOut[11] = SCE_GS_SET_XYZ(SX[2],SY[2],SPZ);
                }
                else
                {
                    pOut[ 2] = SCE_GS_SET_XYZ(SX[0],SY[0],0x00FFFFFF);
                    pOut[ 5] = SCE_GS_SET_XYZ(SX[1],SY[1],0x00FFFFFF);
                    pOut[ 8] = SCE_GS_SET_XYZ(SX[3],SY[3],0x00FFFFFF);
                    pOut[11] = SCE_GS_SET_XYZ(SX[2],SY[2],0x00FFFFFF);
                }

                pOut[ 1] = C;
                pOut[ 4] = C;
                pOut[ 7] = C;
                pOut[10] = C;

                ((f32*)&pOut[ 0])[0] = pSprite->UV0.X;
                ((f32*)&pOut[ 0])[1] = pSprite->UV0.Y;
                ((f32*)&pOut[ 3])[0] = pSprite->UV0.X;
                ((f32*)&pOut[ 3])[1] = pSprite->UV1.Y;
                ((f32*)&pOut[ 6])[0] = pSprite->UV1.X;
                ((f32*)&pOut[ 6])[1] = pSprite->UV0.Y;
                ((f32*)&pOut[ 9])[0] = pSprite->UV1.X;
                ((f32*)&pOut[ 9])[1] = pSprite->UV1.Y;
                Count++;
            }
        }
    }
    else
    {
        // Loop through active views
        u32 ViewMask = eng_GetActiveViewMask();
        for( i=0 ; i<eng_GetNActiveViews() ; i++ )
        if( ViewMask & (1<<i) )
        {
            f32     ProjX0,ProjX1;
            f32     ProjY0,ProjY1;
            f32     NearZ,FarZ,OneOverNFZ;
            matrix4 V2S;
            s_ViewID = i;

            // For multi-screen shot fixup
            s32 OffsetX = OFFSET_X_3D ;
            s32 OffsetY = OFFSET_Y_3D ;

            // Be sure matrices are up to date
            draw_UpdateMatrices();
            LoadSpriteMatrixIntoRegisters( s_V2S[s_ViewID], s_W2V[s_ViewID] );
            const view* pView = eng_GetView(s_ViewID);
            V2S = pView->GetV2S();
            f32 ZConst0 = 16.0f*V2S(2,2);
            f32 ZConst1 = 16.0f*V2S(3,2);
            pView->GetProjection(ProjX0,ProjX1,ProjY0,ProjY1);
            ProjX0 = (ProjX0 + OffsetX)*16.0f;
            ProjY0 = (ProjY0 + OffsetY)*16.0f;
            pView->GetZLimits(NearZ,FarZ);
            OneOverNFZ = 1.0f / (FarZ-NearZ);

            s32     V0X,V0Y;
            s32     V1X,V1Y;
            volatile vector3 Center ALIGN;

            for( j=0; j<s_nSprites; j++ )
            {

                sprite* pSprite = s_SpriteBuffer + j;

                // Transform sprite center into viewspace
                asm __volatile__ ("
                    lqc2            vf12, 0(%0)
                    vmulaw.xyzw     ACC, vf11,vf0
                    vmadday.xyzw    ACC, vf9,vf12
                    vmaddaz.xyzw    ACC, vf10,vf12
                    vmaddx.xyzw     vf12,vf8,vf12
                    sqc2            vf12,0(%1)
                    ": : "r" (&pSprite->Position), "r" (&Center) );

                // Check if in front of view
                if( (Center.Z<NearZ) || (Center.Z>FarZ) ) 
                    continue;

                // Project pos onto screen
                f32 OneOverZ = 1.0f / Center.Z;
                f32 ProjOneOverZ = (16.0f*ProjX1) * OneOverZ;
                s32 ScreenX = (s32)(ProjX0 + Center.X*ProjOneOverZ);
                s32 ScreenY = (s32)(ProjY0 + Center.Y*ProjOneOverZ);
                ProjOneOverZ *= 0.5f;
                f32 w       = pSprite->WH.X * ProjOneOverZ;
                f32 h       = pSprite->WH.Y * ProjOneOverZ;
                //s32 Z       = (s32)(16.0f*V2S(2,2) + (16*V2S(3,2)*OneOverZ));
                //s32 Z = (s32)(((16.0f*Center.Z*V2S(2,2))+(16.0f*V2S(3,2))) * OneOverZ );
                s32 Z = (s32)( ZConst0 + ZConst1*OneOverZ ) + s_ZBias;

                // Construct points v0 and v1
                if( pSprite->IsRotated )
                {
                    f32 s,c;
                    draw_sincos( -pSprite->Rotation, s, c );
                    V0X = (s32)(c*w - s*h);
                    V0Y = (s32)(s*w + c*h);
                    V1X = (s32)(c*w + s*h);
                    V1Y = (s32)(s*w - c*h);
                }
                else
                {
                    V0X = (s32)w;
                    V0Y = (s32)h;
                    V1X = (s32)w;
                    V1Y = (s32)-h;
                }

                s32 SX[4];
                s32 SY[4];
                SX[0] = ScreenX + V0X;
                SY[0] = ScreenY + V0Y;
                SX[1] = ScreenX + V1X; 
                SY[1] = ScreenY + V1Y;
                SX[2] = ScreenX - V0X;
                SY[2] = ScreenY - V0Y;
                SX[3] = ScreenX - V1X;
                SY[3] = ScreenY - V1Y;

                s32 Clip = SX[0] | SY[0] | SX[1] | SY[1] | SX[2] | SY[2] | SX[3] | SY[3];
                if( Clip & 0xFFFF0000 )
                    continue;

                u64 C = (u64)pSprite->Color | (((u64)0x3f800000)<<32);

                // Check if we have a valid sprite
                {
                    u64* pOut = (u64*)(((u32)DLAlloc(8*12)) | 0x00000000);
                    //pOut = (u64*)(((u32)pOut) | 0x30000000);

                    pOut[ 2] = SCE_GS_SET_XYZ(SX[0],SY[0],Z);
                    pOut[ 5] = SCE_GS_SET_XYZ(SX[1],SY[1],Z);
                    pOut[ 8] = SCE_GS_SET_XYZ(SX[3],SY[3],Z);
                    pOut[11] = SCE_GS_SET_XYZ(SX[2],SY[2],Z);

                    pOut[ 1] = C;
                    pOut[ 4] = C;
                    pOut[ 7] = C;
                    pOut[10] = C;

                    ((f32*)&pOut[ 0])[0] = pSprite->UV0.X;
                    ((f32*)&pOut[ 0])[1] = pSprite->UV0.Y;
                    ((f32*)&pOut[ 3])[0] = pSprite->UV0.X;
                    ((f32*)&pOut[ 3])[1] = pSprite->UV1.Y;
                    ((f32*)&pOut[ 6])[0] = pSprite->UV1.X;
                    ((f32*)&pOut[ 6])[1] = pSprite->UV0.Y;
                    ((f32*)&pOut[ 9])[0] = pSprite->UV1.X;
                    ((f32*)&pOut[ 9])[1] = pSprite->UV1.Y;
                    Count++;
                }
           
            }
        }
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*12)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;

    s_nSprites = 0;
}

//==========================================================================
/*
void draw_DispatchSprites( void )
{
    s32 i,j,k;

    header* pH = DLStruct(header); 
    s32 Count=0;

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_TRIANGLESTRIP,
                               1,               // shading method (flat, gouraud)
                               s_IsTextured,    // texture mapping (off, on)
                               0,               // fogging (off, on)
                               s_IsAlpha,       // alpha blending (off, on)
                               0,               // 1 pass anti-aliasing (off, on)
                               s_Is2D,          // tex-coord spec method (STQ, UV)
                               0,               // context (1 or 2)
                               0);              // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 12, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    if( s_Is2D )
    {
    }
    else
    {
        // Loop through active views
        u32 ViewMask = eng_GetActiveViewMask();
        for( i=0 ; i<eng_GetNActiveViews() ; i++ )
        if( ViewMask & (1<<i) )
        {
            s_ViewID = i;

            // Be sure matrices are up to date
            draw_UpdateMatrices();
            LoadSpriteMatrixIntoRegisters( s_V2S[s_ViewID], s_W2V[s_ViewID] );


            vert    Vert[4];
            vector3 v0;
            vector3 v1;
            vector3 Center ALIGN;

            for( j=0; j<s_nSprites; j++ )
            {

                sprite* pSprite = s_SpriteBuffer + j;
                f32     w     = pSprite->WH.X * 0.5f;
                f32     h     = pSprite->WH.Y * 0.5f;

                // Get location in view space of sprite
                s_W2V[s_ViewID].Transform( &Center, &pSprite->Position, 1 );

                // Construct points v0 and v1
                if( pSprite->IsRotated )
                {
                    f32 s,c;
                    draw_sincos( -pSprite->Rotation, s, c );
                    v0.X = c*w - s*h;
                    v0.Y = s*w + c*h;
                    v0.Z = 0.0f;
                    v1.X = c*w + s*h;
                    v1.Y = s*w - c*h;
                    v1.Z = 0.0f;
                }
                else
                {
                    v0.X = w;
                    v0.Y = h;
                    v0.Z = 0;
                    v1.X = w;
                    v1.Y = -h;
                    v1.Z = 0;
                }

                Vert[0].Pos   = Center + v0;
                Vert[1].Pos   = Center + v1;
                Vert[2].Pos   = Center - v0;
                Vert[3].Pos   = Center - v1;
                Vert[0].Color = pSprite->Color;
                Vert[1].Color = pSprite->Color;
                Vert[2].Color = pSprite->Color;
                Vert[3].Color = pSprite->Color;
                Vert[0].UV    = vector2(pSprite->UV0.X,pSprite->UV0.Y);
                Vert[1].UV    = vector2(pSprite->UV0.X,pSprite->UV1.Y);
                Vert[2].UV    = vector2(pSprite->UV1.X,pSprite->UV1.Y);
                Vert[3].UV    = vector2(pSprite->UV1.X,pSprite->UV0.Y);


                // Transform sprite verts
                vert*           pV = Vert;
                volatile veci4  SP;

                for( k=0; k<4; k++ )
                {
                    asm __volatile__ ("
                        lqc2            vf8, 0(%0)
                        vmulaw.xyzw     ACC, vf7,vf0
                        vmadday.xyzw    ACC, vf5,vf8
                        vmaddaz.xyzw    ACC, vf6,vf8
                        vmaddx.xyzw     vf12,vf4,vf8
                        vdiv            Q,vf0w,vf12w
	                    vwaitq
	                    vmulq.xyz       vf12,vf12,Q
                        vmulq.w         vf12,vf0,Q
                        vftoi4.xyz      vf12,vf12
                        sqc2            vf12,0(%1)
                        ": : "r" (&pV[0].Pos), "r" (&SP) );

                    // Build texturing STQ
                    ((f32*)(&pV->GS.T))[0] = pV->UV.X * SP.W; 
                    ((f32*)(&pV->GS.T))[1] = pV->UV.Y * SP.W; 

                    // Build color and Q
                    ((s32*)(&pV->GS.C))[0] = pV->Color;
                    ((f32*)(&pV->GS.C))[1] = SP.W;

                    // Build position
                    pV->GS.P = SCE_GS_SET_XYZ( SP.X, SP.Y, SP.Z ) | 0x01;
            
                    if( ((SP.X|SP.Y)&0xFFFF0000) || (SP.Z<0) )
                        break;

                    // Move to next vert
                    pV++;
                }

                // Check if we have a valid sprite
                
                if( k==4 )
                {
                    u64* pOut = (u64*)DLAlloc(8*12);
                    pOut[ 0] = Vert[0].GS.T;
                    pOut[ 1] = Vert[0].GS.C;
                    pOut[ 2] = Vert[0].GS.P;
                    pOut[ 3] = Vert[1].GS.T;
                    pOut[ 4] = Vert[1].GS.C;
                    pOut[ 5] = Vert[1].GS.P;
                    pOut[ 6] = Vert[3].GS.T;
                    pOut[ 7] = Vert[3].GS.C;
                    pOut[ 8] = Vert[3].GS.P;
                    pOut[ 9] = Vert[2].GS.T;
                    pOut[10] = Vert[2].GS.C;
                    pOut[11] = Vert[2].GS.P;
                    Count++;
                }
            }
        }
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*12)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;

    s_nSprites = 0;
}*/

//==========================================================================

static
void draw_DispatchPoints( void )
{
    header* pH = DLStruct(header); 
    s32 Count=0;

    STAT_DrawPoints += s_nVertsInBuffer;

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_POINT,
                               0,    // shading method (flat, gouraud)
                               0,    // texture mapping (off, on)
                               0,    // fogging (off, on)
                               0,    // alpha blending (off, on)
                               0,    // 1 pass anti-aliasing (off, on)
                               0,    // tex-coord spec method (STQ, UV)
                               0,    // context (1 or 2)
                               0);  // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 2, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_RGBAQ, GIF_REG_XYZ2 ) ;

    // Copy data from spad to output
    vert* pV  = s_Buffer;
    s32 C = s_nVertsInBuffer;
    while( C-- )
    {
        if( pV->GS.P & 0x01 )
        {
            u64* pOut = (u64*)DLAlloc(16);
            pOut[0] = pV->GS.C;
            pOut[1] = pV->GS.P;
            Count++;
        }
        pV++;
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*2)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;
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
    header* pH = DLStruct(header); 
    s32 Count=0;

    STAT_DrawLines += (s_nVertsInBuffer/2);

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_LINE,
                               1,    // shading method (flat, gouraud)
                               0,    // texture mapping (off, on)
                               0,    // fogging (off, on)
                               s_IsAlpha,   // alpha blending (off, on)
                               0,    // 1 pass anti-aliasing (off, on)
                               0,    // tex-coord spec method (STQ, UV)
                               0,    // context (1 or 2)
                               0);  // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 2, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_RGBAQ, GIF_REG_XYZ2 ) ;

    // Copy data from spad to output
    vert* pV  = s_Buffer;
    s32 C = s_nVertsInBuffer;
    while( C>=2 )
    {
        if( (u32)pV[0].GS.P & (u32)pV[1].GS.P & 0x01 )
        {
            u64* pOut = (u64*)DLAlloc(16*2);
            pOut[0] = pV[0].GS.C;
            pOut[1] = pV[0].GS.P;
            pOut[2] = pV[1].GS.C;
            pOut[3] = pV[1].GS.P;
            Count++;
        }
        else
        {
            vert V[2];
            if( ClipLine(pV,V) )
            {
                u64* pOut = (u64*)DLAlloc(16*2);
                pOut[0] = V[0].GS.C;
                pOut[1] = V[0].GS.P;
                pOut[2] = V[1].GS.C;
                pOut[3] = V[1].GS.P;
                Count++;
            }
        }
        pV+=2;
        C-=2;
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*2)*(Count*2)) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = (Count*2);
}

//==========================================================================

static
void draw_DispatchQuadsAndRects( void )
{
    header* pH = DLStruct(header); 
    s32 Count=0;

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_TRIANGLESTRIP,
                               1,               // shading method (flat, gouraud)
                               s_IsTextured,    // texture mapping (off, on)
                               0,               // fogging (off, on)
                               s_IsAlpha,       // alpha blending (off, on)
                               0,               // 1 pass anti-aliasing (off, on)
                               0,          // tex-coord spec method (STQ, UV)
                               0,               // context (1 or 2)
                               0);              // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 12, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2 );


    // Copy data from spad to output
    vert* pV  = s_Buffer;
    s32 C = s_nVertsInBuffer;
    if( s_Primitive == DRAW_RECTS )
    {
        while( C>=2 )
        {
            if( (u32)pV[0].GS.P & (u32)pV[1].GS.P & 0x01 )
            {
                u64* pOut = (u64*)DLAlloc(8*12);
                s32 X0 = (pV[0].GS.P>> 0)&0xFFFF;
                s32 Y0 = (pV[0].GS.P>>16)&0xFFFF;
                s32 X1 = (pV[1].GS.P>> 0)&0xFFFF;
                s32 Y1 = (pV[1].GS.P>>16)&0xFFFF;
                s32 U0 = (pV[0].GS.T>> 0)&0xFFFF;
                s32 V0 = (pV[0].GS.T>>16)&0xFFFF;
                s32 U1 = (pV[1].GS.T>> 0)&0xFFFF;
                s32 V1 = (pV[1].GS.T>>16)&0xFFFF;
                pOut[ 0] = SCE_GS_SET_UV(U0,V0);
                pOut[ 1] = pV[0].GS.C;
                pOut[ 2] = SCE_GS_SET_XYZ(X0,Y0,0);
                pOut[ 3] = SCE_GS_SET_UV(U0,V1);
                pOut[ 4] = pV[0].GS.C;
                pOut[ 5] = SCE_GS_SET_XYZ(X0,Y1,0);
                pOut[ 6] = SCE_GS_SET_UV(U1,V0);
                pOut[ 7] = pV[0].GS.C;
                pOut[ 8] = SCE_GS_SET_XYZ(X1,Y0,0);
                pOut[ 9] = SCE_GS_SET_UV(U1,V1);
                pOut[10] = pV[0].GS.C;
                pOut[11] = SCE_GS_SET_XYZ(X1,Y1,0);
                Count++;
            }
            pV+=2;
            C-=2;
        }
    }
    else
    if( s_Primitive == DRAW_QUADS )
    {
        while( C>=4 )
        {
            if( (u32)pV[0].GS.P & (u32)pV[1].GS.P & (u32)pV[2].GS.P & 0x01 )
            {
                u64* pOut = (u64*)DLAlloc(8*12);
                pOut[ 0] = pV[0].GS.T;
                pOut[ 1] = pV[0].GS.C;
                pOut[ 2] = pV[0].GS.P;
                pOut[ 3] = pV[1].GS.T;
                pOut[ 4] = pV[1].GS.C;
                pOut[ 5] = pV[1].GS.P;
                pOut[ 6] = pV[3].GS.T;
                pOut[ 7] = pV[3].GS.C;
                pOut[ 8] = pV[3].GS.P;
                pOut[ 9] = pV[2].GS.T;
                pOut[10] = pV[2].GS.C;
                pOut[11] = pV[2].GS.P;
                Count++;
            }
            pV+=4;
            C-=4;
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*12)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;
}

//==========================================================================

static
void draw_DispatchWireTriangles( void )
{
    header* pH = DLStruct(header); 
    s32 Count=0;

    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_LINE,
                               1,    // shading method (flat, gouraud)
                               0,    // texture mapping (off, on)
                               0,    // fogging (off, on)
                               s_IsAlpha,   // alpha blending (off, on)
                               0,    // 1 pass anti-aliasing (off, on)
                               0,    // tex-coord spec method (STQ, UV)
                               0,    // context (1 or 2)
                               0);  // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 4, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_RGBAQ, GIF_REG_XYZ2, GIF_REG_RGBAQ, GIF_REG_XYZ2 ) ;

    // Copy data from spad to output
    vert* pV  = s_Buffer;
    s32 C = s_nVertsInBuffer;
    while( C>=3 )
    {
        if( (u32)pV[0].GS.P & (u32)pV[1].GS.P & (u32)pV[2].GS.P & 0x01 )
        {
            u64* pOut = (u64*)DLAlloc(8*4*3);
            
            pOut[ 0] = pV[0].GS.C;
            pOut[ 1] = pV[0].GS.P;
            pOut[ 2] = pV[1].GS.C;
            pOut[ 3] = pV[1].GS.P;

            pOut[ 4] = pV[1].GS.C;
            pOut[ 5] = pV[1].GS.P;
            pOut[ 6] = pV[2].GS.C;
            pOut[ 7] = pV[2].GS.P;

            pOut[ 8] = pV[2].GS.C;
            pOut[ 9] = pV[2].GS.P;
            pOut[10] = pV[0].GS.C;
            pOut[11] = pV[0].GS.P;

            Count+=3;
        }
        else
        {
            vert V[10];
            s32 NVerts;
            NVerts = ClipTriangle(pV,V);
            if( NVerts )
            {
                s32 OldI = NVerts-1;
                for( s32 i=0; i<NVerts; i++ )
                {
                    u64* pOut = (u64*)DLAlloc(8*4);
                    pOut[0] = V[OldI].GS.C;
                    pOut[1] = V[OldI].GS.P;
                    pOut[2] = V[i].GS.C;
                    pOut[3] = V[i].GS.P;
                    OldI = i;
                    Count++;
                }
            }
        }
        pV+=3;
        C-=3;
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*4)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;
}
//==========================================================================

static
void draw_DispatchTriangles( void )
{
    header* pH = DLStruct(header); 
    s32 Count=0;
 
    // Build header info
    pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
    pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
    pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_TRIANGLE,
                               1,               // shading method (flat, gouraud)
                               s_IsTextured,    // texture mapping (off, on)
                               0,               // fogging (off, on)
                               s_IsAlpha,       // alpha blending (off, on)
                               0,               // 1 pass anti-aliasing (off, on)
                               0,          // tex-coord spec method (STQ, UV)
                               0,               // context (1 or 2)
                               0);              // fragment value control (normal, fixed)
    pH->GIF.Build( GIF_MODE_REGLIST, 10, 1, 0, 0, 0, 1 );
    pH->GIF.Reg( GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                 GIF_REG_NOP );

    // Copy data from spad to output
    vert* pV  = s_Buffer;
    s32 C = s_nVertsInBuffer;
    while( C>=3 )
    {
        if( (u32)pV[0].GS.P & (u32)pV[1].GS.P & (u32)pV[2].GS.P & 0x01 )
        {
            u64* pOut = (u64*)DLAlloc(8*10);
            pOut[0] = pV[0].GS.T;
            pOut[1] = pV[0].GS.C;
            pOut[2] = pV[0].GS.P;
            pOut[3] = pV[1].GS.T;
            pOut[4] = pV[1].GS.C;
            pOut[5] = pV[1].GS.P;
            pOut[6] = pV[2].GS.T;
            pOut[7] = pV[2].GS.C;
            pOut[8] = pV[2].GS.P;
            pOut[9] = 0;
            Count++;
        }
        else
        {
            vert V[10];
            s32 NVerts;
            NVerts = ClipTriangle(pV,V);
            if( NVerts )
            {
                for( s32 i=1; i<NVerts-1; i++ )
                {
                    u64* pOut = (u64*)DLAlloc(8*10);
                    pOut[0] = V[0].GS.T;
                    pOut[1] = V[0].GS.C;
                    pOut[2] = V[0].GS.P;
                    pOut[3] = V[i].GS.T;
                    pOut[4] = V[i].GS.C;
                    pOut[5] = V[i].GS.P;
                    pOut[6] = V[i+1].GS.T;
                    pOut[7] = V[i+1].GS.C;
                    pOut[8] = V[i+1].GS.P;
                    pOut[9] = 0;
                    Count++;
                }
            }

        }
        pV+=3;
        C-=3;
    }

    pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*10)*Count) ) ;
    pH->DMA.MakeDirect();
    pH->GIF.NLOOP = Count;
}

//==========================================================================

static
void draw_MainDispatch( void )
{
    s32     i;

    if( s_nVertsInBuffer == 0 )
        return;

    if( 1 )
    {
        // Loop through active views
        u32 ViewMask = eng_GetActiveViewMask();
        for( i=0 ; i<eng_GetNActiveViews() ; i++ )
        if( ViewMask & (1<<i) )
        {
            s_ViewID = i;

            // Be sure matrices are up to date
            draw_UpdateMatrices();

            // Transform verts
            DrawTransformTime.Start();
            draw_TransformVerts( s_Buffer, s_L2S[s_ViewID], s_nVertsInBuffer );
            DrawTransformTime.Stop();

            // Do actual dispatch
            DrawDispatchTime.Start();
            s_pfnDispatch();
            DrawDispatchTime.Stop();
        }
    }

    // Clear vert buffer
    s_BufferPos = s_Buffer;
    s_nVertsInBuffer = 0;
}

//==========================================================================

void draw_Begin( draw_primitive Primitive, u32 Flags )
{
    ASSERT( s_Initialized );
    ASSERT( !s_bBegin );

    // Confirm we are in a render context
    ASSERT( eng_InBeginEnd() );

    // Save primitive and flags
    s_Primitive  = Primitive;
    s_Flags      = Flags;
    s_Is2D       = (Flags & DRAW_2D)?(TRUE):(FALSE);
    s_IsTextured = (Flags & DRAW_TEXTURED)?(TRUE):(FALSE);
    s_IsAlpha    = (Flags & DRAW_USE_ALPHA)?(TRUE):(FALSE);
    s_IsWire     = (Flags & DRAW_WIRE_FRAME)?(TRUE):(FALSE);
    s_IsZBuffered= (Flags & DRAW_NO_ZBUFFER)?(FALSE):(TRUE);
    s_IsSTQ  = FALSE;

    if( s_IsWire )
        s_IsTextured = FALSE;

    // Set internal state from primitive type
    switch( s_Primitive )
    {
    case DRAW_POINTS:
        s_pfnDispatch = draw_DispatchPoints;
        s_nTrigger = TRIGGER_POINTS;
        break;
    case DRAW_LINES:
        s_pfnDispatch = draw_DispatchLines;
        s_nTrigger = TRIGGER_LINES;
        break;
    case DRAW_RECTS:
        s_pfnDispatch = draw_DispatchQuadsAndRects;
        s_nTrigger = TRIGGER_RECTS;
        ASSERT( s_Is2D );
        break;
    case DRAW_QUADS:
        s_pfnDispatch = draw_DispatchQuadsAndRects;
        s_nTrigger = TRIGGER_QUADS;
        break;
    case DRAW_TRIANGLES:
        if( s_IsWire )
        {
            s_pfnDispatch = draw_DispatchWireTriangles;
            s_nTrigger = TRIGGER_TRIANGLES;
            s_IsSTQ = FALSE;
        }
        else
        {
            s_pfnDispatch = draw_DispatchTriangles;
            s_nTrigger = TRIGGER_TRIANGLES;
            s_IsSTQ = s_IsTextured;
        }
        break;
    case DRAW_SPRITES:
        s_nSprites = 0;
        break;
    default:
        ASSERT( FALSE && "Primitive not implemented");
        break;
    }


    gsreg_Begin();

    // Set render states for ALPHA
    if( s_IsAlpha )
    {
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    }
    else
    {
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_ZERO,C_ZERO,A_SRC,C_SRC) );
    }
    
    // Set render states for ZBUFFER
    if( !s_IsZBuffered )
    {
        gsreg_SetZBuffer(FALSE);
    }
    else
    {
//        gsreg_SetZBuffer(TRUE);
    }

    // Set D3D render states for WIREFRAME
    if( Flags & DRAW_WIRE_FRAME )
    {
    }
    else
    {
    }

    // Clear Texture if not in textured mode
    if( !s_IsTextured )
    {
    }

    gsreg_End();

    // Set in begin state
    s_bBegin = TRUE;

    // Clear vert buffer
    s_BufferPos = s_Buffer;
    s_nVertsInBuffer = 0;

    // Clear initial values
    s_UV.Zero();
    s_Color = XCOLOR_WHITE;

    s_nVerts = 0;
    s_nUVs = 0;
    s_nColors = 0;
    s_pVerts = NULL;
    s_pUVs = NULL;
    s_pColors = NULL;
}


//==========================================================================

static
void draw_Flush( void )
{
    // Flush any drawing we have queued up
    draw_MainDispatch();

    // Watch for leftover sprites
    if( s_Primitive == DRAW_SPRITES )
        draw_DispatchSprites();
}

//==========================================================================

void draw_End( void )
{
    ASSERT( s_bBegin );

    draw_Flush();

    // Clear in begin state
    s_bBegin = FALSE;

    // Restore zbuffer if we turned it off
    if( !s_IsZBuffered )
    {
        gsreg_Begin();
        gsreg_SetZBuffer(TRUE);
        gsreg_End();
    }

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
    draw_Flush();
    vram_Activate( Bitmap );
}

//==========================================================================

void draw_SetTexture( void )
{
    vram_Activate();
}

//==========================================================================

void draw_DisableBilinear( void )
{
    gsreg_Begin();
    gsreg_SetClamping( TRUE );
    gsreg_SetMipEquation( FALSE, 1.0f, 0, MIP_MAG_POINT, MIP_MIN_POINT );
    gsreg_End();
}

//==========================================================================

void draw_UV( const vector2& UV )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    s_UV = UV;
}

void draw_UV( f32 U, f32 V )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    s_UV.X = U;
    s_UV.Y = V;
}

//==========================================================================

void draw_Color( const xcolor& Color )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );
    xcolor c2( Color.B, Color.G, Color.R, Color.A );
    if( s_IsTextured )
    {
        c2.R >>= 1;
        c2.G >>= 1;
        c2.B >>= 1;
    }
    c2.A >>= 1;
    s_Color = (u32)c2;
}

void draw_Color( f32 R, f32 G, f32 B, f32 A )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );
    f32 S = ( s_IsTextured )?(128):(255);
    s_Color  = ((s32)(R*S))<< 0;
    s_Color |= ((s32)(G*S))<< 8;
    s_Color |= ((s32)(B*S))<<16;
    s_Color |= ((s32)(A*128))<<24;
}

//==========================================================================

void draw_Vertex( const vector3& Vertex )
{
    ASSERT( s_bBegin );
    ASSERT( s_Primitive != DRAW_SPRITES );

    // Setup vertex in buffer
    s_BufferPos->UV    = s_UV;
    s_BufferPos->Color = s_Color;
    s_BufferPos->Pos   = Vertex;

    // Advance buffer pointer
    s_BufferPos++;
    s_nVertsInBuffer++;

    // Check if it is time to dispatch this buffer
    if( s_nVertsInBuffer == s_nTrigger )
        draw_MainDispatch();
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
    eng_PushGSContext(0);
    gsreg_Begin();
    gsreg_SetFBMASK( 0x00FFFFFF );
    gsreg_SetZBufferTest( ZBUFFER_TEST_ALWAYS );
    gsreg_SetZBufferUpdate( TRUE );
    gsreg_End();
    eng_PopGSContext();

    eng_PushGSContext(1);
    gsreg_Begin();
    gsreg_SetFBMASK( 0x00FFFFFF );
    gsreg_SetZBufferTest( ZBUFFER_TEST_ALWAYS );
    gsreg_SetZBufferUpdate( TRUE );
    gsreg_End();
    eng_PopGSContext();

    {
        header* pH = DLStruct(header); 

        // Build header info
        pH->PRIMGIF.Build( GIF_MODE_REGLIST, 1, 1, 0, 0, 0, 0 );
        pH->PRIMGIF.Reg( GIF_REG_PRIM ) ;
        pH->prim = SCE_GS_SET_PRIM(GIF_PRIM_TRIANGLESTRIP,
                                   1,               // shading method (flat, gouraud)
                                   0,               // texture mapping (off, on)
                                   0,               // fogging (off, on)
                                   0,               // alpha blending (off, on)
                                   0,               // 1 pass anti-aliasing (off, on)
                                   0,               // tex-coord spec method (STQ, UV)
                                   0,               // context (1 or 2)
                                   0);              // fragment value control (normal, fixed)
        pH->GIF.Build( GIF_MODE_REGLIST, 12, 1, 0, 0, 0, 1 );
        pH->GIF.Reg( GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                     GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ3, 
                     GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2, 
                     GIF_REG_ST,  GIF_REG_RGBAQ, GIF_REG_XYZ2 );

        u64* pOut = (u64*)DLAlloc(8*12);
        s32 X0 = ((Rect.l+OFFSET_X)*16)&0xFFFF;
        s32 Y0 = ((Rect.t+OFFSET_Y)*16)&0xFFFF;
        s32 X1 = ((Rect.r+OFFSET_X)*16)&0xFFFF;
        s32 Y1 = ((Rect.b+OFFSET_Y)*16)&0xFFFF;
        s32 U0 = 0;
        s32 V0 = 0;
        s32 U1 = 0;
        s32 V1 = 0;
        s32 C  = 0x00000000; //0xFFFFFFFF;
        pOut[ 0] = SCE_GS_SET_UV(U0,V0);
        pOut[ 1] = C;
        pOut[ 2] = SCE_GS_SET_XYZ(X0,Y0,0x00);
        pOut[ 3] = SCE_GS_SET_UV(U0,V1);
        pOut[ 4] = C;
        pOut[ 5] = SCE_GS_SET_XYZ(X0,Y1,0x00);
        pOut[ 6] = SCE_GS_SET_UV(U1,V0);
        pOut[ 7] = C;
        pOut[ 8] = SCE_GS_SET_XYZ(X1,Y0,0x00);
        pOut[ 9] = SCE_GS_SET_UV(U1,V1);
        pOut[10] = C;
        pOut[11] = SCE_GS_SET_XYZ(X1,Y1,0x00);

        pH->DMA.SetCont( sizeof(header)-sizeof(dmatag)+((8*12)*1) ) ;
        pH->DMA.MakeDirect();
        pH->GIF.NLOOP = 1;
    }

    eng_PushGSContext(0);
    gsreg_Begin();
    gsreg_SetFBMASK( 0x00000000 );
    gsreg_SetZBufferTest( ZBUFFER_TEST_GEQUAL );
    gsreg_End();
    eng_PopGSContext();

    eng_PushGSContext(1);
    gsreg_Begin();
    gsreg_SetFBMASK( 0x00000000 );
    gsreg_SetZBufferTest( ZBUFFER_TEST_GEQUAL );
    gsreg_End();
    eng_PopGSContext();
}

//==============================================================================
