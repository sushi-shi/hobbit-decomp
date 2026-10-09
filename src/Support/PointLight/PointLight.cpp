//=========================================================================
//
// POINTLIGHT.CPP
//
//=========================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <Support/PointLight/PointLight.hpp>
#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "Objects\Terrain\terrain.hpp"
#include "Building\BuildingObj.hpp"
#include "..\..\AADS\globals.hpp"

#include "Objects\Player\PlayerObject.hpp"
#include "Objects\Player\CorpseObject.hpp"
#include "Objects\Bot\BotObject.hpp"

#define MAX_VERTS               144      // MUST be divisible by 3


//#define POINTLIGHT_TIMERS

//=========================================================================
//=========================================================================
// FACE CACHE
//=========================================================================
//=========================================================================

#define MAX_FCACHE_FACES        1024//2048
#define FCACHE_HASH_BITS        10
#define FCACHE_HASH_ENTRIES     (1<<FCACHE_HASH_BITS)     

struct fcache_hash
{
    s32         First;
    s32         NFaces;
};

fcache_face*    s_Face;
fcache_hash*    s_Hash;
s32             s_FirstFreeFace;

s32             s_FaceCacheBytes;

s32             STAT_KeysCollected;
s32             STAT_FacesConstructed;

#ifdef TARGET_PS2
xbool           ptlight_UseDraw  = FALSE;
#else
xbool           ptlight_UseDraw  = TRUE;
#endif
xbool           ptlight_UseAlpha = TRUE;
xbool           ptlight_NoRender = FALSE;

//=========================================================================

void fcache_Flush( void )
{
    s32 i;
    ASSERT( s_Face && s_Hash );

    // Clear hash entries
    for( i=0; i<FCACHE_HASH_ENTRIES; i++ )
    {
        s_Hash[i].NFaces = 0;
        s_Hash[i].First  = -1;
    }

    // Clear face entries and sew together
    for( i=0; i<MAX_FCACHE_FACES; i++ )
    {
        s_Face[i].Key = 0xFFFFFFFF;
        s_Face[i].NVerts = 0;
        s_Face[i].Next = i+1;
        s_Face[i].Prev = i-1;
    }
    s_Face[MAX_FCACHE_FACES-1].Next = -1;

    s_FirstFreeFace = 0;
}

//=========================================================================

void fcache_Init( void )
{
    s_Face = (fcache_face*)x_malloc(sizeof(fcache_face)*MAX_FCACHE_FACES);
    ASSERT(s_Face);

    s_Hash = (fcache_hash*)x_malloc(sizeof(fcache_hash)*FCACHE_HASH_ENTRIES);
    ASSERT(s_Hash);

    s_FaceCacheBytes = sizeof(fcache_face)*MAX_FCACHE_FACES + 
                       sizeof(fcache_hash)*FCACHE_HASH_ENTRIES;
    //x_DebugMsg("Face cache memory usage: %1d\n",s_FaceCacheBytes);

    fcache_Flush();
}

//=========================================================================

void fcache_Kill( void )
{
    x_free(s_Face);
    x_free(s_Hash);
    s_Face = NULL;
    s_Hash = NULL;
    s_FirstFreeFace = -1;
}

//=========================================================================

inline s32 fcache_ComputeHash( fcache_key Key )
{
    s32 Hash = 0;

    Hash ^= (Key>>(FCACHE_HASH_BITS*0)) & (FCACHE_HASH_ENTRIES-1);
    Hash ^= (Key>>(FCACHE_HASH_BITS*1)) & (FCACHE_HASH_ENTRIES-1);
    Hash ^= (Key>>(FCACHE_HASH_BITS*2)) & (FCACHE_HASH_ENTRIES-1);
    Hash ^= (Key>>(FCACHE_HASH_BITS*3)) & (FCACHE_HASH_ENTRIES-1);

    return Hash;
}

//=========================================================================

s32 fcache_AllocAndHashFace( fcache_key Key )
{
    s32 FI = -1;
    ASSERT( (s_FirstFreeFace>=-1) && (s_FirstFreeFace<MAX_FCACHE_FACES) );

    // Check if there is a face in the free list
    if( s_FirstFreeFace != -1 )
    {
        FI = s_FirstFreeFace;
        if( s_Face[FI].Next != -1 )
            s_Face[s_Face[FI].Next].Prev = -1;
        s_FirstFreeFace = s_Face[FI].Next;
    }
    else
    {
        // Pull a face currently in use
        FI = x_irand(0,MAX_FCACHE_FACES-1);
        ASSERT( s_Face[FI].NVerts > 0 );

        // Remove from hash entry
        s32 Hash = fcache_ComputeHash( s_Face[FI].Key );
        s_Hash[Hash].NFaces--;

        if( s_Face[FI].Prev != -1 )
            s_Face[s_Face[FI].Prev].Next = s_Face[FI].Next;
        
        if( s_Face[FI].Next != -1 )
            s_Face[s_Face[FI].Next].Prev = s_Face[FI].Prev;

        if( s_Hash[Hash].First == FI )
            s_Hash[Hash].First = s_Face[FI].Next;
    }

    // Clear face for new use 
    s_Face[FI].Key = Key;
    s_Face[FI].NVerts = 0;

    // Hook face into hash table
    s32 Hash = fcache_ComputeHash( Key );
    s_Face[FI].Next = s_Hash[Hash].First;
    s_Face[FI].Prev = -1;
    if( s_Hash[Hash].First != -1 )
        s_Face[s_Hash[Hash].First].Prev = FI;
    s_Hash[Hash].First = FI;
    s_Hash[Hash].NFaces++;

    // Return face
    ASSERT( (s_FirstFreeFace>=-1) && (s_FirstFreeFace<MAX_FCACHE_FACES) );
    return FI;
}

//=========================================================================

const fcache_face& fcache_GetFace( fcache_key Key )
{
    // Search for face in hash table
    s32 Hash = fcache_ComputeHash(Key);
    s32 FI = s_Hash[Hash].First;
    s32 Count=100;
    while( FI != -1 )
    {
        if( s_Face[FI].Key == Key )
            return s_Face[FI];
        FI = s_Face[FI].Next;

        // Safety valve for infinite loops
        if( Count-- == 0 )
            break;
    }

    // Could not find face in cache so we need to construct it
    FI = fcache_AllocAndHashFace( Key );

    // Build face from correct object
    s32 ObjectSlot;
    s32 PrivateInfo;
    fcache_DestructKey( Key, ObjectSlot, PrivateInfo );
    object* pObj = ObjMgr.GetObjectFromSlot( ObjectSlot );
    ASSERT( pObj );
    if( pObj->GetType() == object::TYPE_TERRAIN )
    {
        ((terrain*)pObj)->ConstructFace( s_Face[FI] );
    }
    else
    if( pObj->GetType() == object::TYPE_BUILDING )
    {
        ((building_obj*)pObj)->ConstructFace( s_Face[FI] );
    }
    else
    {
        ASSERT(FALSE);
    }
    STAT_FacesConstructed++;

    // Return face
    return s_Face[FI];
}

//=========================================================================

void fcache_ConstructKey ( fcache_key& Key, s32 ObjectSlot, s32  PrivateInfo )
{
    ASSERT( ((u32)PrivateInfo & (((u32)0xFFFFFFFF)>>10)) == (u32)PrivateInfo );
    Key = (((u32)ObjectSlot) << 22) | (PrivateInfo & (((u32)0xFFFFFFFF)>>10));
}

//=========================================================================

void fcache_DestructKey  ( fcache_key  Key, s32& ObjectSlot, s32& PrivateInfo )
{
    ObjectSlot = (((u32)Key)>>22) & 1023;
    PrivateInfo = ((u32)Key) & (((u32)0xFFFFFFFF)>>10);
}


//=========================================================================
//=========================================================================
// Dummies
//=========================================================================
//=========================================================================

vector3 s_Dummy[32];
s32     s_NDummies=0;
xbitmap s_DummyBMP;

void AddDummy( const vector3& Pos )
{
    ASSERT( s_NDummies < 32 );
    s_Dummy[s_NDummies] = Pos;
    s_NDummies++;
}

//=========================================================================
f32 DUMMY_WIDTH = 0.75f;
f32 DUMMY_HEIGHT = 1.0f;
xbool SHOW_DUMMIES = FALSE;

void RenderDummies( void )
{
#ifdef TARGET_PS2
    vector3* pV;
    vector2* pT;
    ps2color* pC;
    s32 i,j,k;

    vector3 DV[12] = 
    {
        vector3(0,0,0),             vector3(DUMMY_WIDTH,DUMMY_HEIGHT,0), vector3(-DUMMY_WIDTH,DUMMY_HEIGHT,0),
        vector3(0,0,0),             vector3(0,DUMMY_HEIGHT,DUMMY_WIDTH), vector3(0,DUMMY_HEIGHT,-DUMMY_WIDTH),
        vector3(0,DUMMY_HEIGHT*2,0),   vector3(DUMMY_WIDTH,DUMMY_HEIGHT,0),           vector3(-DUMMY_WIDTH,DUMMY_HEIGHT,0),
        vector3(0,DUMMY_HEIGHT*2,0),   vector3(0,DUMMY_HEIGHT,DUMMY_WIDTH),           vector3(0,DUMMY_HEIGHT,-DUMMY_WIDTH),
    };

    vector2 TV[12] =
    {
        vector2(0,0),   vector2(0,1),       vector2(1,0),
        vector2(0,0),   vector2(0.5f,1),    vector2(1,0),
        vector2(0,0),   vector2(0,1),       vector2(1,0),
        vector2(0,0),   vector2(0.5f,1),    vector2(1,0),
    };

    if( s_NDummies == 0 )
        return;

    #ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    gsreg_End();
    #endif

    poly_Begin(POLY_USE_ALPHA);

    vram_Activate(s_DummyBMP);

    // Allocate data
    pV = (vector3*) smem_BufferAlloc( sizeof(vector3)  * s_NDummies * 12 );
    pT = (vector2*) smem_BufferAlloc( sizeof(vector2)  * s_NDummies * 12 );
    pC = (ps2color*)smem_BufferAlloc( sizeof(ps2color) * s_NDummies * 12 );

    // Check if allocation failed
    if( (!pV) || (!pT) || (!pC) )
    {
        poly_End();
        return;
    }

    k=0;
    for( i=0; i<s_NDummies; i++ )
    {
        vector3 P = s_Dummy[i];
        xcolor FC = tgl.Fog.ComputeFog( P );
        ps2color C = ps2color(128,128,128,128-(FC.A>>1));

        for( j=0; j<12; j++ )
        {
            pV[k] = P + DV[j];
            pT[k] = TV[j];
            pC[k] = C;
            k++;
        }
    }

    poly_Render( pV, pT, pC, s_NDummies * 12 );
    poly_End();
    #ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_OFF );
    gsreg_End();
    #endif


    if( SHOW_DUMMIES )
    {
        for( i=0; i<s_NDummies; i++ )
        {
            draw_BBox( bbox(s_Dummy[i],2), XCOLOR_RED );
        }
    }
#endif
    s_NDummies = 0;
}

//=========================================================================
//=========================================================================
// Bullet holes
//=========================================================================
//=========================================================================

#define MAX_BULLET_HOLES    256
#define BULLET_HOLE_AGE     3500
#define BULLET_FADEOUT      500
#define BULLET_HOLE_RADIUS  0.1f

struct bullet_hole
{
    vector3 Pos[4];
    s32     Start;
};

static bullet_hole  s_BHole[ MAX_BULLET_HOLES ];
static s32          s_BHoleCursor;
static s32          s_BHoleStart;
static xbitmap      s_BHoleBMP;

//=========================================================================

void AddBulletHole( const vector3& Pos, const vector3& Normal )
{
    bullet_hole* pB = &s_BHole[s_BHoleCursor];
    vector3 VX,VY;
    plane Plane;
    Plane.Setup(Pos,Normal);
    Plane.GetOrthoVectors( VX, VY );
    VX *= BULLET_HOLE_RADIUS;
    VY *= BULLET_HOLE_RADIUS;

    pB->Start  = s_BHoleStart;
    pB->Pos[0] = Pos - VX - VY;
    pB->Pos[1] = Pos - VX + VY;
    pB->Pos[2] = Pos + VX + VY;
    pB->Pos[3] = Pos + VX - VY;

    s_BHoleCursor = (s_BHoleCursor+1)%MAX_BULLET_HOLES;
}

//=========================================================================

void RenderBulletHoles( void )
{
    xcolor C = XCOLOR_WHITE;
    draw_SetZBias(0);
    draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );

#ifdef TARGET_PS2

    const view* pView = eng_GetActiveView(0);
    vram_SetMipEquation( vram_ComputeMipK(0.005f, *pView) );
    gsreg_Begin();
    gsreg_SetZBufferUpdate( FALSE );
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    gsreg_End();

#endif

    draw_SetTexture( s_BHoleBMP );

    // Start with cursor
    s32 I = (s_BHoleCursor+MAX_BULLET_HOLES-1)%MAX_BULLET_HOLES;
	s32 count;

	count = 0;
    while( s_BHole[I].Start != -1 )
    {
        f32 TimeLeft = (f32)(BULLET_HOLE_AGE - (s_BHoleStart - s_BHole[I].Start));

        // Check if old
        if( TimeLeft <= 0.0f )
        {
            s_BHole[I].Start = -1;
        }
        else
        {
            if( TimeLeft > BULLET_FADEOUT )
                C.A = 255;
            else
                C.A = (byte)(255.0f*TimeLeft/(f32)BULLET_FADEOUT);

            draw_Color( C );
            draw_UV( 0.0f, 0.0f ); draw_Vertex( s_BHole[I].Pos[0] );
            draw_UV( 0.0f, 1.0f ); draw_Vertex( s_BHole[I].Pos[1] );
            draw_UV( 1.0f, 1.0f ); draw_Vertex( s_BHole[I].Pos[2] );
            draw_UV( 0.0f, 0.0f ); draw_Vertex( s_BHole[I].Pos[0] );
            draw_UV( 1.0f, 1.0f ); draw_Vertex( s_BHole[I].Pos[2] );
            draw_UV( 1.0f, 0.0f ); draw_Vertex( s_BHole[I].Pos[3] );
        }

        // Go to next hole
        I = I-1;
        if( I<0 ) I = MAX_BULLET_HOLES-1;
		count++;
		if (count > MAX_BULLET_HOLES)
			break;
    }

    draw_End();
    draw_SetZBias(0);

#ifdef TARGET_PS2
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_OFF );
    gsreg_SetZBufferUpdate( TRUE );
    gsreg_End();
#endif

}

//=========================================================================
//=========================================================================
// POINT LIGHTS
//=========================================================================
//=========================================================================

#define MAX_PT_LIGHTS   128

#define PTLIGHT_TYPE_UNUSED     0
#define PTLIGHT_TYPE_DYNAMIC    1
#define PTLIGHT_TYPE_STATIC     2

struct ptlight
{
    s32     Type;
    vector3 Pos;
    f32     Radius;
    f32     OneOverRadius;
    bbox    BBox;
    xcolor  Color;
    xbool   IncludeBldgs;
    s32     Next;
    s32     Prev;

    // For static
    f32     Age;
    f32     UserRadius;
    f32     UserTime[4];
};

static xbool    s_Inited = FALSE;
static ptlight  s_Light[MAX_PT_LIGHTS];
static s32      s_First;
static s32      s_FirstFree;
static s32      s_NLights;
static xbitmap  s_BMP;
//static xbitmap  s_ShadowBMP;

//=========================================================================

void ptlight_Init( void )
{
    s32 i;
    ASSERT( !s_Inited );
    s_Inited = TRUE;
    
    s_First = -1;

    for( i=0; i<MAX_PT_LIGHTS; i++ )
    {
        s_Light[i].Type = PTLIGHT_TYPE_UNUSED;
        s_Light[i].Pos.Zero();
        s_Light[i].Color.Set(0,0,0,0);
        s_Light[i].Radius = 0;
        s_Light[i].Next = i+1;
        s_Light[i].Prev = i-1;
    }
    s_Light[MAX_PT_LIGHTS-1].Next = -1;

    s_FirstFree = 0;
    s_NLights = 0;

    fcache_Init();

    VERIFY( auxbmp_LoadNative( s_BMP, "Data/ptlight.bmp" ) );
    vram_Register( s_BMP );

    //VERIFY( auxbmp_LoadNative( s_ShadowBMP, "Data/shadow.tga" ) );
    //vram_Register( s_ShadowBMP );

    s_BHoleCursor=0;
    s_BHoleStart=0;
    VERIFY( auxbmp_LoadNative( s_BHoleBMP, "Data/bullethole.tga" ) );
    s_BHoleBMP.BuildMips();
    vram_Register( s_BHoleBMP );
    for( i=0; i<MAX_BULLET_HOLES; i++ )
        s_BHole[i].Start = -1;

    s_NDummies = 0;
    VERIFY( auxbmp_LoadNative( s_DummyBMP, "Data/dummy.bmp" ) );
    vram_Register( s_DummyBMP );
}


//=========================================================================

void ptlight_Kill( void )
{
    ASSERT( s_Inited );
    s_Inited = FALSE;

    vram_Unregister( s_BHoleBMP );
    vram_Unregister( s_BMP );
    //vram_Unregister( s_ShadowBMP );

    s_BHoleBMP.Kill();
    s_BMP.Kill();
    //s_ShadowBMP.Kill();

    fcache_Kill();
}

//=========================================================================

static
s32 ptlight_Alloc( void )
{
    ASSERT( s_Inited );

    if( s_FirstFree == -1 )
        return -1;

    s32 ID = s_FirstFree;
    ASSERT( ID != -1 );

    // Take light out of free list
    if( s_Light[s_FirstFree].Next != -1 )
        s_Light[s_Light[s_FirstFree].Next].Prev = -1;
    s_FirstFree = s_Light[s_FirstFree].Next;

    // Add light to used list
    s_Light[ID].Next = s_First;
    s_Light[ID].Prev = -1;
    if( s_First != -1 )
        s_Light[s_First].Prev = ID;
    s_First = ID;
    s_NLights++;

    return ID;
}


//=========================================================================

s32  ptlight_Create( const vector3& Pos, f32 Radius, xcolor Color, xbool IncludeBldgs )
{
    s32 ID = ptlight_Alloc();
    if( ID==-1 )
        return -1;

    // Setup values
    s_Light[ID].Type = PTLIGHT_TYPE_DYNAMIC;
    s_Light[ID].Pos = Pos;
    s_Light[ID].Radius = Radius;
    s_Light[ID].OneOverRadius = 1.0f / s_Light[ID].Radius;
    s_Light[ID].Color = Color;
    s_Light[ID].BBox = bbox( Pos, Radius );
    s_Light[ID].IncludeBldgs = IncludeBldgs;

    return ID;
}

//=========================================================================

void ptlight_Destroy( s32 Handle )
{
    if( Handle==-1 ) 
        return;

    ASSERT( s_Inited );
    ASSERT( (Handle>=0) && (Handle<MAX_PT_LIGHTS) );
    ASSERT( s_Light[Handle].Type != PTLIGHT_TYPE_UNUSED );

    // Remove from used list
    if( s_Light[Handle].Prev != -1 )
        s_Light[s_Light[Handle].Prev].Next = s_Light[Handle].Next;
    if( s_Light[Handle].Next != -1 )
        s_Light[s_Light[Handle].Next].Prev = s_Light[Handle].Prev;
    if( s_First == Handle )
        s_First = s_Light[Handle].Next;

    // Add to free list
    if( s_FirstFree != -1 )
        s_Light[s_FirstFree].Prev = Handle;
    s_Light[Handle].Prev = -1;
    s_Light[Handle].Next = s_FirstFree;
    s_FirstFree = Handle;

    // Mark as not used
    s_Light[Handle].Type = PTLIGHT_TYPE_UNUSED;

    s_NLights--;
}

//=========================================================================

void ptlight_SetPosition( s32 Handle, const vector3& Pos )
{
    if( Handle==-1 ) 
        return;

    ASSERT( s_Inited );
    ASSERT( (Handle>=0) && (Handle<MAX_PT_LIGHTS) );
    ASSERT( s_Light[Handle].Type == PTLIGHT_TYPE_DYNAMIC );
    s_Light[Handle].Pos = Pos;
    s_Light[Handle].BBox = bbox( s_Light[Handle].Pos, s_Light[Handle].Radius );
}

//=========================================================================

void ptlight_SetRadius( s32 Handle, f32 Radius )
{
    if( Handle==-1 ) 
        return;

    ASSERT( s_Inited );
    ASSERT( (Handle>=0) && (Handle<MAX_PT_LIGHTS) );
    ASSERT( s_Light[Handle].Type == PTLIGHT_TYPE_DYNAMIC );
    s_Light[Handle].Radius = Radius;
    s_Light[Handle].OneOverRadius = 1.0f / s_Light[Handle].Radius;
    s_Light[Handle].BBox = bbox( s_Light[Handle].Pos, s_Light[Handle].Radius );
}

//=========================================================================

void ptlight_SetColor( s32 Handle, xcolor Color )
{
    if( Handle==-1 ) 
        return;

    ASSERT( s_Inited );
    ASSERT( (Handle>=0) && (Handle<MAX_PT_LIGHTS) );
    ASSERT( s_Light[Handle].Type == PTLIGHT_TYPE_DYNAMIC );
    s_Light[Handle].Color = Color;
}

//=========================================================================

void ptlight_AdvanceLogic( f32 DeltaSec )
{
    s_BHoleStart += (s32)(DeltaSec*1000);

    // Loop through lights and advance logic

    s32 ID = s_First;
    while( ID != -1 )
    {
        s32 Next = s_Light[ID].Next;

        // Only apply logic to static lights
        if( s_Light[ID].Type == PTLIGHT_TYPE_STATIC )
        {
            // Update age
            s_Light[ID].Age += DeltaSec;

            // Check if it should be destroyed
            if( s_Light[ID].Age >= s_Light[ID].UserTime[3] )
            {
                ptlight_Destroy( ID );
            }
            else
            {
                s32 i;

                // Compute the current radius
                for( i=1; i<4; i++ )
                if( s_Light[ID].Age < s_Light[ID].UserTime[i] )
                    break;
                ASSERT( i!=4 );

                // Decide which stage we are in
                if( i==1 )
                {
                    f32 T = s_Light[ID].Age / s_Light[ID].UserTime[1];
                    s_Light[ID].Radius = T*s_Light[ID].UserRadius;
                }
                else
                if( i==2 )
                {
                    s_Light[ID].Radius = s_Light[ID].UserRadius;
                }
                else
                if( i==3 )
                {
                    f32 T = (s_Light[ID].Age         - s_Light[ID].UserTime[2]) / 
                            (s_Light[ID].UserTime[3] - s_Light[ID].UserTime[2]);
                    s_Light[ID].Radius = (1-T)*s_Light[ID].UserRadius;
                }

                s_Light[ID].OneOverRadius = 1.0f / s_Light[ID].Radius;
                s_Light[ID].BBox = bbox( s_Light[ID].Pos, s_Light[ID].Radius );
            }
        }

        ID = Next;
    }
}

//=========================================================================

#define MAX_RENDER_KEYS     2048
#define MIN_PT_LIGHT_RADIUS 0.1f

static fcache_key  s_RenderKey[MAX_RENDER_KEYS];
volatile xbool SHOW_PT_LIGHT_SPHERES = FALSE;
volatile xbool SHOW_PT_LIGHT_STATS = FALSE;
volatile f32   POINT_LIGHT_ALPHA = 1.0f;
s32    POINT_LIGHT_ZBIAS = 3;

void ptlight_Render( void )
{
    s32         TotalKeys;
    const view* pView;

#ifdef POINTLIGHT_TIMERS
    xtimer      TotalTime;
    xtimer      KeyTime;
#endif

    s32 UseDraw = ptlight_UseDraw;  // so we can toggle in debugger without poly_Begin/End Asserts
    
    RenderBulletHoles();
    RenderDummies();

    pView = eng_GetActiveView(0);

#ifdef POINTLIGHT_TIMERS
    TotalTime.Start();
#endif

    STAT_KeysCollected = 0;
    STAT_FacesConstructed = 0;

    ASSERT( s_Inited );

    vector3 EyePos = pView->GetPosition();

    s32 ID = s_First;
    s32 NumVerts = 0;

    vector3*  pVert = NULL;
    vector3*  pV    = NULL;
    vector2*  pTex  = NULL;  
    vector2*  pT    = NULL;
    ps2color* pCol  = NULL;  
    ps2color* pC    = NULL;
    xbool     OutOfMemory = FALSE;

    if( !UseDraw )
    {
        poly_SetZBias( POINT_LIGHT_ZBIAS );
        poly_Begin( ptlight_UseAlpha ? POLY_USE_ALPHA : 0 );
        vram_Activate( s_BMP );
    }
    else
    {
        draw_SetZBias( POINT_LIGHT_ZBIAS );
        draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | ( ptlight_UseAlpha ? DRAW_USE_ALPHA : 0 ));
        draw_SetTexture( s_BMP );
    }

#ifdef TARGET_PS2
    
    gsreg_Begin();
    gsreg_SetClamping( TRUE );
    gsreg_SetZBufferUpdate( FALSE );
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
    gsreg_End();

#endif
    
#ifdef TARGET_PC
    
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_ONE );

#endif

    s32 NPointLightsRendered=0;
    while( (ID != -1) && (!OutOfMemory) )
    {
        // Check if point light is not in view
        if( (s_Light[ID].Radius > MIN_PT_LIGHT_RADIUS) &&
            pView->SphereInView( s_Light[ID].Pos, s_Light[ID].Radius ) )
        {
            // Decide on fog value
            xcolor FogColor = tgl.Fog.ComputeFog( s_Light[ID].Pos );
            ps2color C;
            s32 Alpha = (s32)((f32)s_Light[ID].Color.A + ((f32)(2*FogColor.A/255.0f)*(0-s_Light[ID].Color.A)));
            if( Alpha < 0 ) Alpha = 0;
            if( Alpha > 0 )
            {
                C.R = s_Light[ID].Color.R;
                C.G = s_Light[ID].Color.G;
                C.B = s_Light[ID].Color.B;
                NPointLightsRendered++;
                //x_printfxy(0,NPointLightsRendered,"%d %d %d %d %d",(s32)FogColor.R,(s32)FogColor.G,(s32)FogColor.B,(s32)FogColor.A,Alpha);


                // Collect keys of faces that need to be rendered
#ifdef POINTLIGHT_TIMERS
                KeyTime.Start();
#endif
                TotalKeys = 0;

                object* pObj;
                ObjMgr.Select( object::ATTR_SOLID_STATIC, s_Light[ID].BBox );
                while( (pObj = ObjMgr.GetNextSelected()) )
                {
                    if( pObj->GetType() == object::TYPE_TERRAIN )
                    {
                        terrain* pTerrain = (terrain*)pObj;
                        s32 NKeys = MAX_RENDER_KEYS - TotalKeys;
                        pTerrain->GetSphereFaceKeys( EyePos, s_Light[ID].Pos, s_Light[ID].Radius, NKeys, s_RenderKey+TotalKeys );
                        TotalKeys += NKeys;
                    }

                    if( s_Light[ID].IncludeBldgs && ( pObj->GetType() == object::TYPE_BUILDING ) )
                    {
                        building_obj* pBuilding = (building_obj*)pObj;
                        s32 NKeys = MAX_RENDER_KEYS - TotalKeys;
                        pBuilding->GetSphereFaceKeys( EyePos, s_Light[ID].Pos, s_Light[ID].Radius, NKeys, s_RenderKey+TotalKeys );
                        TotalKeys += NKeys;
                    }
                }
                ASSERT( TotalKeys < MAX_RENDER_KEYS );
                ObjMgr.ClearSelection();

                STAT_KeysCollected += TotalKeys;
#ifdef POINTLIGHT_TIMERS
                KeyTime.Stop();
#endif

                if( !UseDraw )
                {
                    C.R >>= 1;
                    C.G >>= 1;
                    C.B >>= 1;
                }
            
                // Render faces
                for( s32 i=0; i<TotalKeys; i++ )
                {
                    const fcache_face& F = fcache_GetFace( s_RenderKey[i] );
                    f32 D = F.Plane.Distance( s_Light[ID].Pos );
                    if( D<0 ) D = -D;

                    if( D<s_Light[ID].Radius )
                    {
                        // Compute uvs
                        vector3 Center  = s_Light[ID].Pos - (F.Plane.Normal * D);
                        f32 ConstR = 0.5f / x_sqrt( s_Light[ID].Radius*s_Light[ID].Radius - D*D );
                        f32 ConstS = 0.5f - F.OrthoS.Dot( Center ) * ConstR;
                        f32 ConstT = 0.5f - F.OrthoT.Dot( Center ) * ConstR;
                        f32 Factor = (s_Light[ID].Radius - D) * s_Light[ID].OneOverRadius;
                        ASSERT( (Factor>=0.0f) && (Factor<=1.0f) );

                        C.A = (byte)(Alpha*Factor*POINT_LIGHT_ALPHA);
                    
                        if( !UseDraw )
                        {
                            C.A >>= 1;

                            if( !NumVerts )
                            {
                                pVert = pV = (vector3*) smem_BufferAlloc( sizeof(vector3)  * MAX_VERTS );
                                pTex  = pT = (vector2*) smem_BufferAlloc( sizeof(vector2)  * MAX_VERTS );
                                pCol  = pC = (ps2color*)smem_BufferAlloc( sizeof(ps2color) * MAX_VERTS );
                                if( (!pV) || (!pT) || (!pC) )
                                {
                                    NumVerts    = 0;
                                    OutOfMemory = TRUE;
                                    //x_DebugMsg("Could not allocate point light verts!!!\n");
                                    break;
                                }
                            }
                        
                            pVert[0] = F.Pos[0];
                            pVert[1] = F.Pos[1];
                            pVert[2] = F.Pos[2];
                        
                            pTex[0]  = vector2( F.OrthoST[0].X * ConstR + ConstS, F.OrthoST[0].Y * ConstR + ConstT );
                            pTex[1]  = vector2( F.OrthoST[1].X * ConstR + ConstS, F.OrthoST[1].Y * ConstR + ConstT );
                            pTex[2]  = vector2( F.OrthoST[2].X * ConstR + ConstS, F.OrthoST[2].Y * ConstR + ConstT );
    
                            pCol[0]  = C;
                            pCol[1]  = C;
                            pCol[2]  = C;
                        
                            pVert += 3;
                            pTex  += 3;
                            pCol  += 3;
    
                            NumVerts += 3;
                        
                            if( NumVerts == MAX_VERTS )
                            {
                                if( !ptlight_NoRender )
                                {
                                    poly_Render( pV, pT, pC, MAX_VERTS );
                                }
    
                                pVert = pV;
                            
                                NumVerts = 0;
                            }
                        }
                        else
                        {
                            xcolor XC = xcolor( C.R, C.G, C.B, C.A );
                    
                            draw_Color( XC );
                    
                            //vector3 Fudge = F.Plane.Normal * 0.03f;

                            draw_UV( F.OrthoST[0].X * ConstR + ConstS, F.OrthoST[0].Y * ConstR + ConstT );
                            draw_Vertex( F.Pos[0] );
                            draw_UV( F.OrthoST[1].X * ConstR + ConstS, F.OrthoST[1].Y * ConstR + ConstT );
                            draw_Vertex( F.Pos[1] );
                            draw_UV( F.OrthoST[2].X * ConstR + ConstS, F.OrthoST[2].Y * ConstR + ConstT );
                            draw_Vertex( F.Pos[2] );
                        }

                    }
                }
            }
        }

        ID = s_Light[ID].Next;
    }

    if( !UseDraw )
    {
        if( NumVerts )
        {
            ASSERT( NumVerts <= MAX_VERTS );
        
            if( !ptlight_NoRender )
            {
                poly_Render( pV, pT, pC, NumVerts );
            }
        }
    
        poly_End();
        poly_SetZBias( 0 );
    }
    else
    {
        draw_End();
        draw_SetZBias(0);
    }

#ifdef TARGET_PS2
    
    gsreg_Begin();
    gsreg_SetClamping( FALSE );
    gsreg_SetZBufferUpdate( TRUE );
    gsreg_SetAlphaBlend( ALPHA_BLEND_OFF );
    gsreg_End();

#endif

#ifdef TARGET_PC
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
#endif

#ifdef POINTLIGHT_TIMERS
    TotalTime.Stop();
#endif

    if( SHOW_PT_LIGHT_STATS )
    {
#ifdef POINTLIGHT_TIMERS
        x_printfxy(0,5,"NLs:%3d Ks:%3d Fs:%3d TT:%1.2f KT:%1.2f\n",
            s_NLights,STAT_KeysCollected,STAT_FacesConstructed,
            TotalTime.ReadMs(),
            KeyTime.ReadMs());
#endif
    }

    // Render debug spheres
    if( SHOW_PT_LIGHT_SPHERES )
    {
        ID = s_First;
        while( ID != -1 )
        {
            draw_Sphere( s_Light[ID].Pos, s_Light[ID].Radius, s_Light[ID].Color );
            ID = s_Light[ID].Next;
        }
    }
}

//=========================================================================

void ptlight_Create     ( const vector3& Pos,
                                f32      Radius,
                                xcolor   Color,
                                f32      RampUpTime,
                                f32      HoldTime,
                                f32      RampDownTime,
                                xbool    IncludeBldgs )
{
    s32 ID = ptlight_Alloc();

    if( ID == -1 )
        return;

    // Setup values
    s_Light[ID].Type = PTLIGHT_TYPE_STATIC;
    s_Light[ID].Pos = Pos;
    s_Light[ID].Radius = 0;
    s_Light[ID].OneOverRadius = 0;
    s_Light[ID].Color = Color;
    s_Light[ID].BBox = bbox( Pos, Radius );

    s_Light[ID].Age = 0.0f;
    s_Light[ID].UserRadius = Radius;
    s_Light[ID].UserTime[0] = 0.0f;
    s_Light[ID].UserTime[1] = RampUpTime;
    s_Light[ID].UserTime[2] = RampUpTime+HoldTime;
    s_Light[ID].UserTime[3] = RampUpTime+HoldTime+RampDownTime;
    s_Light[ID].IncludeBldgs = IncludeBldgs;
}


//=========================================================================
//=========================================================================
//=========================================================================
// SHADOWS
//=========================================================================
//=========================================================================
//=========================================================================

f32     PLAYER_SHADOW_RADIUS = 1.0f;
f32     PLAYER_SHADOW_DIST   = 20.0f;
s32     NShadowKeys;
s32     NShadows;
xbool   SHOW_SHADOW_INFO = FALSE;

#ifdef POINTLIGHT_TIMERS
xtimer  ShadowClipTime;
xtimer  ShadowKeyTime;
xtimer  ShadowDrawTime;
#endif


#define MAX_SHADOW_FRAGMENT_VERTS   16
#define MAX_SHADOW_FRAGMENTS        128
#define MAX_SHADOW_PLANE_SETS       64

struct shad_fragment
{
    vector3 P[MAX_SHADOW_FRAGMENT_VERTS];
    s32     NPoints;
};

struct shad_plane_set
{
    plane Plane[4];
};

shad_fragment   s_ShadFrag[MAX_SHADOW_FRAGMENTS*2];
shad_plane_set  s_ShadPlane[MAX_SHADOW_PLANE_SETS];
s32             s_NShadPlaneSets;


//=========================================================================

f32 TriArea( const vector3& P0, const vector3& P1, const vector3& P2 )
{
    vector3 N = v3_Cross(P1-P0,P2-P0);
    return N.LengthSquared();
}

//=========================================================================

void ShadClip( const shad_fragment& Frag,
               const plane&         Plane,
                     shad_fragment& Front,
                     shad_fragment& Back )
{
    const f32 EPSILON = 0.0f;
    ASSERT( Frag.NPoints >= 3 );
    Front.NPoints = 0;
    Back.NPoints  = 0;

    s32     p0,p1;
    vector3 P0,P1;
    f32     D0,D1;

    // Loop through points in this ngon
    for( p0=0; p0<Frag.NPoints; p0++ )
    {
        p1 = (p0+1);
        if( p1 == Frag.NPoints ) p1 = 0;
        P0 = Frag.P[p0];
        P1 = Frag.P[p1];
        D0 = Plane.Distance(P0);
        D1 = Plane.Distance(P1);

        // Check if we need to add point to front or back
        if( D0 > EPSILON ) 
        { 
            if( Front.NPoints < MAX_SHADOW_FRAGMENT_VERTS )
            {
                Front.P[Front.NPoints] = P0; 
                Front.NPoints++; 
            }
        }

        if( D0 <= EPSILON ) 
        { 
            if( Back.NPoints < MAX_SHADOW_FRAGMENT_VERTS )
            {
                Back.P[Back.NPoints] = P0; 
                Back.NPoints++;    
            }
        }

        // Check if edge spans clipping plane
        if( ((D0>EPSILON)&&(D1<EPSILON)) || 
            ((D0<EPSILON)&&(D1>EPSILON)) )
        {
            // Compute point at crossing and add to both
            f32 t = (0-D0)/(D1-D0);
            if( t<0 ) t=0;
            if( t>1 ) t=1;
            vector3 P;
            P.X = (P0.X + t*(P1.X-P0.X));
            P.Y = (P0.Y + t*(P1.Y-P0.Y));
            P.Z = (P0.Z + t*(P1.Z-P0.Z));

            if( Front.NPoints < MAX_SHADOW_FRAGMENT_VERTS )
            {
                Front.P[Front.NPoints] = P; 
                Front.NPoints++; 
            }

            if( Back.NPoints < MAX_SHADOW_FRAGMENT_VERTS )
            {
                Back.P[Back.NPoints] = P; 
                Back.NPoints++;    
            }
        }
    }

    if( Front.NPoints < 3 )
        Front.NPoints = 0;

    if( Back.NPoints < 3 )
        Back.NPoints = 0;

    if( Front.NPoints >= 3 )
    {
        if( TriArea( Front.P[0], Front.P[1], Front.P[2] ) < 0.001f )
        {
            Front.NPoints = 0;
            //BREAK;
        }
    }

    if( Back.NPoints >= 3 )
    {
        if( TriArea( Back.P[0], Back.P[1], Back.P[2] ) < 0.001f )
        {
            Back.NPoints = 0;
            //BREAK;
        }
    }
}

//=========================================================================

void draw_Plane( const plane&   Plane, 
                 const vector3& Center,
                       f32      Radius,
                       xcolor   Color=XCOLOR_WHITE,
                       xbool    DoWire=FALSE)
{
    f32 UScale[8] = {1.000f,0.333f,-0.333f,-1.000f,-1.000f,-0.333f,0.333f,1.000f};
    f32 VScale[8] = {0.333f,1.000f,1.000f,0.333f,-0.333f,-1.000f,-1.000f,-0.333f};
    vector3 P[8];
    vector3 UDot;
    vector3 VDot;
    s32 i;

    Plane.GetOrthoVectors( UDot, VDot );

    for( i=0; i<8; i++ )
        P[i] = Center + (UDot*UScale[i] + VDot*VScale[i])*Radius;

    if( DoWire )
    {
        s32 p = 7;
        draw_Begin(DRAW_LINES,DRAW_USE_ALPHA);
            draw_Color(Color);
            for( i=0; i<8; i++ )
            {
                draw_Vertex(P[p]);
                draw_Vertex(P[i]);
                p = i;
            }
        draw_End();
    }
    else
    {
        draw_Begin(DRAW_TRIANGLES,DRAW_USE_ALPHA);
            draw_Color(Color);
            for( i=1; i<7; i++ )
            {
                draw_Vertex(P[0]);
                draw_Vertex(P[i]);
                draw_Vertex(P[i+1]);
            }
        draw_End();
    }

    draw_Begin(DRAW_LINES);
        draw_Color(XCOLOR_BLACK);   draw_Vertex(Center);
        draw_Color(Color);          draw_Vertex(Center+Plane.Normal*Radius);
    draw_End();
}

//=========================================================================
/*
s32 SHADOW_MaxPlaneSets = 0;
s32 DO_SHAD_VOLUME_CLIPPING = 4;
s32 SHADOW_ZBIAS = 1;


void shadow_RenderVolume( const bbox&    BBox,
                          const vector3& StartPos,
                          const vector3& EndPos,
                                f32      Radius,
                          const vector3& SunDir)
{
    s32 i;
    s32 TotalKeys = 0;
    plane   VolumePlane[5];
    s32 NSrcShadFragments = 0;
//  s32 NDstShadFragments = 0;
    shad_fragment* pSrcFrag = s_ShadFrag;
//  shad_fragment* pDstFrag = s_ShadFrag + MAX_SHADOW_FRAGMENTS;

    NShadows++;

    if( SHOW_SHADOW_INFO )
    {
        draw_Sphere( StartPos, Radius, XCOLOR_GREEN );
        draw_Sphere( EndPos, Radius, XCOLOR_RED );
        draw_Line( StartPos, EndPos, XCOLOR_WHITE );
    }

    ////////////////////////////
    // COLLECT KEYS
    ////////////////////////////

    // Collect keys of faces that need to be rendered
    ShadowKeyTime.Start();

    object* pObj;
    ObjMgr.Select( object::ATTR_SOLID_STATIC, BBox );
    while( (pObj = ObjMgr.GetNextSelected()) )
    {
        if( pObj->GetType() == object::TYPE_TERRAIN )
        {
            terrain* pTerrain = (terrain*)pObj;
            s32 NKeys = MAX_RENDER_KEYS - TotalKeys;
            pTerrain->GetPillFaceKeys( StartPos, EndPos, Radius+1.0f, NKeys, s_RenderKey+TotalKeys );
            TotalKeys += NKeys;
        }
    }
    ASSERT( TotalKeys < MAX_RENDER_KEYS );
    ObjMgr.ClearSelection();

    STAT_KeysCollected += TotalKeys;
    ShadowKeyTime.Stop();
    NShadowKeys += TotalKeys;

    ShadowClipTime.Start();

    ////////////////////////////
    // BUILD VOLUME PLANES
    ////////////////////////////
    {
        vector3 P[4];
        vector3 UDot;
        vector3 VDot;
        VolumePlane[0].Setup( StartPos, SunDir );
        VolumePlane[0].GetOrthoVectors( UDot, VDot );
        
        //draw_Plane( VolumePlane[0], StartPos, Radius, XCOLOR_WHITE, TRUE );

        UDot *= Radius*2.00f;
        VDot *= Radius*2.00f;
        P[0] = StartPos - UDot - VDot;
        P[1] = StartPos - UDot + VDot;
        P[2] = StartPos + UDot + VDot;
        P[3] = StartPos + UDot - VDot;
        VolumePlane[1].Setup( P[0]+SunDir, P[0], P[1] );
        VolumePlane[2].Setup( P[1]+SunDir, P[1], P[2] );
        VolumePlane[3].Setup( P[2]+SunDir, P[2], P[3] );
        VolumePlane[4].Setup( P[3]+SunDir, P[3], P[0] );
        //draw_Plane( VolumePlane[1], P[0], Radius, XCOLOR_RED, TRUE );
        //draw_Plane( VolumePlane[2], P[1], Radius, XCOLOR_GREEN, TRUE );
        //draw_Plane( VolumePlane[3], P[2], Radius, XCOLOR_BLUE, TRUE );
        //draw_Plane( VolumePlane[4], P[3], Radius, XCOLOR_YELLOW, TRUE );
    }

    ////////////////////////////
    // BUILD SHADOW PLANE SETS AND INITIAL FRAGMENTS
    ////////////////////////////
    s_NShadPlaneSets = 0;
    for( i=0; i<TotalKeys; i++ )
    {
        s32 j;
        shad_fragment Frag;

        // Get face and check if backfacing sun direction
        const fcache_face& F = fcache_GetFace( s_RenderKey[i] );
        if( F.Plane.Normal.Dot( SunDir ) >= 0.0f )
            continue;

        // Clip fragment to volume planes
        {
            shad_fragment KeepFrag[2];
            shad_fragment Back;

            KeepFrag[0].P[0] = F.Pos[0];
            KeepFrag[0].P[1] = F.Pos[1];
            KeepFrag[0].P[2] = F.Pos[2];
            KeepFrag[0].NPoints = 3;

            s32 k=0;
            if( DO_SHAD_VOLUME_CLIPPING )
            {
                for( j=0; j<DO_SHAD_VOLUME_CLIPPING; j++ )
                {
                    ShadClip( KeepFrag[k], VolumePlane[j], KeepFrag[(k+1)&0x01], Back );
                    k = (k+1)&0x01;
                    if( KeepFrag[k].NPoints == 0 )
                        break;
                }
            }

            if( KeepFrag[k].NPoints == 0 )
                continue;

            Frag = KeepFrag[k];
        }

        if( s_NShadPlaneSets == MAX_SHADOW_PLANE_SETS )
            break;

        // Allocate a new shadow plane set
        shad_plane_set* pP = &s_ShadPlane[s_NShadPlaneSets];
        s_NShadPlaneSets++;

        // Create planes 
        pP->Plane[0] = F.Plane;
        pP->Plane[0].Negate();
        pP->Plane[1].Setup( F.Pos[0] + SunDir, F.Pos[0], F.Pos[1] );
        pP->Plane[2].Setup( F.Pos[1] + SunDir, F.Pos[1], F.Pos[2] );
        pP->Plane[3].Setup( F.Pos[2] + SunDir, F.Pos[2], F.Pos[0] );

        // Add fragment
        if( NSrcShadFragments == MAX_SHADOW_FRAGMENTS )
            break;

        pSrcFrag[NSrcShadFragments] = Frag;
        NSrcShadFragments++;
    }

    ////////////////////////////
    // CLIP FACES
    ////////////////////////////
    {
        if( SHOW_SHADOW_INFO )
        {
            random Rand;
            draw_SetZBias(SHADOW_ZBIAS);
            draw_Begin( DRAW_TRIANGLES, DRAW_WIRE_FRAME );

            for( i=0; i<NSrcShadFragments; i++ )
            {
                ASSERT( pSrcFrag[i].NPoints >= 3 );

                draw_Color( Rand.color() );
                for( s32 j=1; j<(pSrcFrag[i].NPoints-1); j++ )
                {
                    vector3 P[3];
                    P[0] = pSrcFrag[i].P[0];
                    P[1] = pSrcFrag[i].P[j];
                    P[2] = pSrcFrag[i].P[j+1];
                    P[0].Y += i*0.10f + 3.0f;
                    P[1].Y += i*0.10f + 3.0f;
                    P[2].Y += i*0.10f + 3.0f;
                    draw_Vertex( P[0] );
                    draw_Vertex( P[1] );
                    draw_Vertex( P[2] );
                }
            }

            draw_End();
            draw_SetZBias(0);
        }

        if( SHOW_SHADOW_INFO )
        {
            for( i=0; i<NSrcShadFragments; i++ )
            {
                ASSERT( pSrcFrag[i].NPoints >= 3 );
                vector3 P = (pSrcFrag[i].P[0] + pSrcFrag[i].P[1] + pSrcFrag[i].P[2])/3.0f;
                P.Y += i*0.10f + 3.0f;
                draw_Label( P, XCOLOR_BLACK, "%1d",i);
            }
        }
sfasdf
        s_NShadPlaneSets = MIN(SHADOW_MaxPlaneSets,s_NShadPlaneSets);

        // Loop through planesets
        for( s32 j=0; j<s_NShadPlaneSets; j++ )
        {
            NDstShadFragments = 0;

            // Loop through src fragments
            for( s32 k=0; k<NSrcShadFragments; k++ )
            {
                if( NDstShadFragments==MAX_SHADOW_FRAGMENTS )
                    break;

                shad_fragment KeepFrag[2];
                shad_fragment StoreFrag;

                KeepFrag[0] = pSrcFrag[k];

                if( TriArea( KeepFrag[0].P[0], KeepFrag[0].P[1], KeepFrag[0].P[2] ) < 0.001f )
                {
                    BREAK;
                }

                // Loop through planes
                for( s32 l=0; l<4; l++ )
                {
                    ShadClip( KeepFrag[l&0x01], s_ShadPlane[j].Plane[l], KeepFrag[(l+1)&0x01], StoreFrag );

                    if( StoreFrag.NPoints > 0 )
                    {
                        if( NDstShadFragments==MAX_SHADOW_FRAGMENTS )
                            break;

                        pDstFrag[NDstShadFragments] = StoreFrag;
                        NDstShadFragments++;
                    }

                    if( KeepFrag[(l+1)&0x01].NPoints == 0 )
                        break;
                }
            }

            // Swap buffers
            {
                shad_fragment* pTemp    = pSrcFrag;
                pSrcFrag                = pDstFrag;
                pDstFrag                = pTemp;
                NSrcShadFragments       = NDstShadFragments;
            }
        }
asdfas
    }
    ShadowClipTime.Stop();


    ////////////////////////////
    // RENDER FACES
    ////////////////////////////
    ShadowDrawTime.Start();

    vector3 UDot;
    vector3 VDot;
    plane   ProjPlane;
    ProjPlane.Setup( StartPos, SunDir );
    ProjPlane.GetOrthoVectors( UDot, VDot );
    UDot *= 0.5f / Radius;
    VDot *= 0.5f / Radius;

    //x_printf("NF:%1d\n",NSrcShadFragments);
    if( 0 )
    {
        random Rand;
        draw_SetZBias(4);
        draw_Begin( DRAW_TRIANGLES, DRAW_USE_ALPHA );

        for( i=0; i<NSrcShadFragments; i++ )
        {
            ASSERT( pSrcFrag[i].NPoints >= 3 );

            xcolor C = Rand.color();
            C.A = 64;
            draw_Color( C );
            for( s32 j=1; j<(pSrcFrag[i].NPoints-1); j++ )
            {
                vector3 P[3];
                P[0] = pSrcFrag[i].P[0];
                P[1] = pSrcFrag[i].P[j];
                P[2] = pSrcFrag[i].P[j+1];
                //P[0].Y += i*0.10f;
                //P[1].Y += i*0.10f;
                //P[2].Y += i*0.10f;
                draw_Vertex( P[0] );
                draw_Vertex( P[1] );
                draw_Vertex( P[2] );
            }
        }

        draw_End();
        draw_SetZBias(0);
    }


    if( 1 )
    {

    #ifdef TARGET_PS2
        draw_SetZBias(8);
        draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
        draw_SetTexture( s_ShadowBMP );
        gsreg_Begin();
        gsreg_SetClamping( TRUE );
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_DST,C_SRC,A_SRC,C_SRC) );
        gsreg_End();
    #endif

    #ifdef TARGET_PC
        draw_SetZBias(1);
        draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED | DRAW_USE_ALPHA );
        draw_SetTexture( s_ShadowBMP );
        g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ZERO );
        g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCCOLOR );
    #endif

        // Render faces
        for( i=0; i<NSrcShadFragments; i++ )
        {
            s32 j;

            ASSERT( pSrcFrag[i].NPoints >= 3 );

            vector3 RP[MAX_SHADOW_FRAGMENT_VERTS];
            f32     CI[MAX_SHADOW_FRAGMENT_VERTS];
            f32      U[MAX_SHADOW_FRAGMENT_VERTS];
            f32      V[MAX_SHADOW_FRAGMENT_VERTS];

            for( j=0; j<pSrcFrag[i].NPoints; j++ )
            {
                RP[j] = pSrcFrag[i].P[j] - StartPos;
                f32 I = SunDir.Dot(RP[j]) * (1.0f/PLAYER_SHADOW_DIST);
                if( I<0 ) I=0.0f;
                if( I>1.0f) I=1.0f;
                CI[j] = 1.0f;
                

                U[j] = UDot.Dot(RP[j]) + 0.5f;
                V[j] = VDot.Dot(RP[j]) + 0.5f;
            }

            for( j=1; j<pSrcFrag[i].NPoints-1; j++ )
            {
                draw_Color( 1.0f, 1.0f, 1.0f, CI[0] );
                draw_UV( U[0], V[0] );
                draw_Vertex( pSrcFrag[i].P[0] );

                draw_Color( 1.0f, 1.0f, 1.0f, CI[j] );
                draw_UV( U[j], V[j] );
                draw_Vertex( pSrcFrag[i].P[j] );

                draw_Color( 1.0f, 1.0f, 1.0f, CI[j+1] );
                draw_UV( U[j+1], V[j+1] );
                draw_Vertex( pSrcFrag[i].P[j+1] );
            }
        }

        draw_End();
        draw_SetZBias(0);

    #ifdef TARGET_PS2
        gsreg_Begin();
        gsreg_SetClamping( FALSE );
        gsreg_SetAlphaBlend( ALPHA_BLEND_OFF );
        gsreg_End();
    #endif

    #ifdef TARGET_PC
        g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
        g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
    #endif
    }

    ShadowDrawTime.Stop();
}


//=========================================================================

void shadow_Render( const vector3& SunDir )
{
    ShadowClipTime.Reset();
    ShadowKeyTime.Reset();
    ShadowDrawTime.Reset();
    NShadowKeys = 0;
    NShadows=0;

    const view* pView;
    pView = eng_GetActiveView(0);

    // Loop through players
    player_object *pPlayer;
    ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
    while( (pPlayer = (player_object*)ObjMgr.GetNextInType()) )
    {
        // Decide on pill
        vector3 StartPos = pPlayer->GetDrawPos() + vector3(0,0.1f,0);
        vector3 EndPos   = StartPos + SunDir*PLAYER_SHADOW_DIST;

        // Get bbox for pill and check if in view
        bbox BBox(StartPos,EndPos);
        if( pView->BBoxInView( BBox ) )
        {
            shadow_RenderVolume( BBox, StartPos-SunDir*1.0f, EndPos, PLAYER_SHADOW_RADIUS, SunDir );
        }
    }
    ObjMgr.EndTypeLoop();

    // Loop through bots
    ObjMgr.StartTypeLoop( object::TYPE_BOT );
    while( (pPlayer = (player_object*)ObjMgr.GetNextInType()) )
    {
        // Decide on pill
        vector3 StartPos = pPlayer->GetDrawPos() + vector3(0,0.1f,0);
        vector3 EndPos   = StartPos + SunDir*PLAYER_SHADOW_DIST;

        // Get bbox for pill and check if in view
        bbox BBox(StartPos,EndPos);
        if( pView->BBoxInView( BBox ) )
        {
            shadow_RenderVolume( BBox, StartPos-SunDir*1.0f, EndPos, PLAYER_SHADOW_RADIUS, SunDir );
        }
    }
    ObjMgr.EndTypeLoop();

    if( SHOW_SHADOW_INFO )
    {
        x_printfxy(0, 9,"NShad     %d",NShadows);
        x_printfxy(0,10,"ShadKeyT  %4.2f",ShadowKeyTime.ReadMs() );
        x_printfxy(0,11,"NShadKeys %d",NShadowKeys);
        x_printfxy(0,12,"ShadDrawT %4.2f",ShadowDrawTime.ReadMs() );
        x_printfxy(0,13,"ShadClipT %4.2f",ShadowClipTime.ReadMs() );
    }
}

//=========================================================================
*/




