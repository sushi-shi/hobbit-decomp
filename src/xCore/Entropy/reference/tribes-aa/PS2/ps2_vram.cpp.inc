//=========================================================================
//
// E_VRAM.CPP
//
//=========================================================================
#include <xCore/Entropy/e_Engine.hpp>


//=========================================================================
// DEFINES
//=========================================================================

#define DB(x) printf(x)

#define MAX_MIPS 6   


volatile xbool USE_BEST_MIPS = TRUE ;   // Turn on/off blending between mips


struct clut_ref
{
    byte*   pData;
    s16     InUse;
    s16     NColors;
    s16     BlockBase;
    s16     Bank;
    s16     TextureRef;
};

struct pixel_ref
{
    byte*   pData;
    s16     InUse;
    s16     Width;
    s16     Height;
    s16     PageWidth;
    s16     WidthLog2;
    s16     HeightLog2;
    s16     BlockBase;
    s16     Bank;
    s16     NBlocks;
    s16     TextureRef;
    s16     MipLevel;
    s16     Format;

    s16     NextRef;
    s16     PrevRef;
};

struct texture_ref
{
    s16     InUse;
    s16     Width;
    s16     Height;
    s16     WidthLog2;
    s16     HeightLog2;
    s16     BPP;
    s16     PixelFormat;
    s16     PixelRef;
    s16     ClutRef;
    s16     NMips;
    s16     MipRef[MAX_MIPS];
    sceGsTex0       tex01;
    sceGsMiptbp1    MBP1;
    sceGsMiptbp2    MBP2;
};

struct image_load
{
    dmatag          DMA;    // Header DMA and DIRECT vif cmd
    sceGsLoadImage  Header; // Transfer register values
    dmatag          DMA1;   // DMA ref to image data
};

struct image_load_ext
{
    dmatag          DMA;    // Header DMA and DIRECT vif cmd
    giftag          GIF;
    dmatag          REF;
};

typedef union 
{
    f32 v;
    struct 
    {
        u32 mant : 23;
        u32 exp  :  8;
        u32 sign :  1;
    } s;
} ieee32;


struct bank
{
    s32     ClutBaseAddr;
    s32     NClutBlocks;
    s32     ClutBlockRefCursor;
    s32     ClutBlockRef[32];

    s32     PixelBaseAddr;
    s32     PixelEndAddr;
    s32     NPixelBlocks;
    s32     PixelBlockRefCursor;
    s32     PixelBlockRefHead;

    s32     NClutRefsInBank;
    s32     NPixelRefsInBank;
    s32     NBlocksUsedInBank;
};

//=========================================================================

#define VRAM_MAX_BANKS              (4)

#define VRAM_MAX_PIXEL_REFS_IN_VRAM (128)
#define VRAM_MAX_CLUT_REFS_IN_VRAM  (32)

#define VRAM_CLUT_BASE_ADDR         (16384-(4*VRAM_MAX_CLUT_REFS_IN_VRAM))

#define VRAM_PIXEL_BASE_ADDR        (12288)
//#define VRAM_PIXEL_BASE_ADDR        (13440)
#define VRAM_PIXEL_END_ADDR         (16384-4)//(16384)//(VRAM_CLUT_BASE_ADDR)
#define VRAM_NUM_PIXEL_BLOCKS       (VRAM_PIXEL_END_ADDR-VRAM_PIXEL_BASE_ADDR)

//=========================================================================

texture_ref* s_TextureRef;
s32          s_NTextureRefs;
s32          s_NTextureRefsAllocated;

pixel_ref*   s_PixelRef;
s32          s_NPixelRefs;
s32          s_NPixelRefsAllocated;

clut_ref*    s_ClutRef;
s32          s_NClutRefs;
s32          s_NClutRefsAllocated;


bank         s_Bank[VRAM_MAX_BANKS];
s32          s_NBanks;
f32          s_MipK[2]   ={0,0};
xbool        s_MipManualMode[2]={TRUE,TRUE};

f32          s_LogTable[256];

xbool        s_VRAMClean;

image_load s_CLUTLoad4;
image_load s_CLUTLoad8;

s64 STAT_VRAM_LoadTime=0;
s64 STAT_VRAM_ActivateTime=0;

s32         VRAM_BytesLoaded=0;

xbool       s_Inited = FALSE;

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
/*
typedef struct {
    unsigned long SBP:14;
    unsigned long SBW:6;
    unsigned long SPSM:6;
    unsigned long DBP:14;
    unsigned long DBW:6;
    unsigned long DPSM:6;
} sceGsBitbltbuf;

typedef struct {
    unsigned long SSAX:11;
    unsigned long SSAY:11;
    unsigned long DSAX:11;
    unsigned long DSAY:11;
    unsigned long DIR:2;
} sceGsTrxpos;

typedef struct {
    unsigned long RRW:12;
    unsigned long RRH:12;
} sceGsTrxreg;

typedef struct {
    unsigned long XDR:2;
} sceGsTrxdir;

typedef struct {
    sceGifTag   giftag0;
    sceGsBitbltbuf  bitbltbuf;
    long        bitbltbufaddr;
    sceGsTrxpos trxpos;
    long        trxposaddr;
    sceGsTrxreg trxreg;
    long        trxregaddr;
    sceGsTrxdir trxdir;
    long        trxdiraddr;
    sceGifTag   giftag1;
} sceGsLoadImage __attribute__((aligned(16)));

    bitbltbuf.DBP = dbp
    bitbltbuf.DBW = dbw
    bitbltbuf.DPSM = dpsm
    trxpos.DSAX = x
    trxpos.DSAY = y
    trxreg.RRW = w
    trxreg.RRH = h

*/
//=========================================================================
volatile f32 VRAM_MIP_MIN_FUDGE = -0.0f;
volatile f32 VRAM_MIP_MAX_FUDGE =  0.0f;

static inline void ComputeMipRange( s32 VRAMID, f32 MinLOD, f32 MaxLOD, s32& MinMipI, s32& MaxMipI )
{
    s32 NMips = s_TextureRef[VRAMID].NMips;
        
    MinMipI = 0;
    MaxMipI = 0;

    if( NMips > 0 )
    {
        MinMipI = (s32)x_floor(MinLOD+VRAM_MIP_MIN_FUDGE);
        MaxMipI = (s32)x_ceil(MaxLOD+VRAM_MIP_MAX_FUDGE);
        MinMipI = MAX(0,MinMipI);
        MinMipI = MIN(NMips,MinMipI);
        MaxMipI = MAX(0,MaxMipI);
        MaxMipI = MIN(NMips,MaxMipI);
    }
}

//==========================================================================

inline s32 AndyGsSetDefLoadImage(sceGsLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y, short w, short h )
{
    ASSERT( dbp >= VRAM_PIXEL_BASE_ADDR );

    ((u64*)(&lp->bitbltbuf))[0] = 0;
    lp->bitbltbuf.DBP = dbp;
    lp->bitbltbuf.DBW = dbw;
    lp->bitbltbuf.DPSM = dpsm;
    lp->bitbltbufaddr  = 0x50;
    
    ((u64*)(&lp->trxpos))[0] = 0;
    lp->trxpos.DSAX = x;
    lp->trxpos.DSAY = y;
    lp->trxposaddr  = 0x51;

    ((u64*)(&lp->trxreg))[0] = 0;
    lp->trxreg.RRW = w;
    lp->trxreg.RRH = h;
    lp->trxregaddr  = 0x52;

    ((u64*)(&lp->trxdir))[0] = 0;
    lp->trxdir.XDR  = 0;
    lp->trxdiraddr  = 0x53;

    s32 ImageBytes = w*h;
    switch( dpsm )
    {
        case SCE_GS_PSMCT32:   ImageBytes <<= 2; break;
        case SCE_GS_PSMCT16:   ImageBytes <<= 1; break;
        case SCE_GS_PSMT8:     ImageBytes <<= 0; break;
        case SCE_GS_PSMT4:     ImageBytes >>= 1; break;
        default: ASSERT(FALSE); break;
    }
    VRAM_BytesLoaded += ImageBytes;

    giftag* pGT = (giftag*)(&lp->giftag0);
    pGT->BuildRegLoad(4,FALSE);

    if( (ImageBytes>>4) < 32768 )
    {
        pGT = (giftag*)(&lp->giftag1);
        pGT->BuildImageLoad( ImageBytes, TRUE );
        return 0;
    }

    return ImageBytes;

    //sceGsSetDefLoadImage(lp,dbp,dbw,dpsm,x,y,w,h);
}

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
/*

typedef struct {
    unsigned long pad00;
} sceGsTexflush;


typedef struct {
    unsigned long LCM:1;
    unsigned long pad01:1;
    unsigned long MXL:3;
    unsigned long MMAG:1;
    unsigned long MMIN:3;
    unsigned long MTBA:1;
    unsigned long pad10:9;
    unsigned long L:2;
    unsigned long pad21:11;
    unsigned long K:12;
    unsigned long pad44:20;
} sceGsTex1;

typedef struct {
    unsigned long TBP0:14;
    unsigned long TBW:6;
    unsigned long PSM:6;
    unsigned long TW:4;
    unsigned long TH:4;
    unsigned long TCC:1;
    unsigned long TFX:2;
    unsigned long CBP:14;
    unsigned long CPSM:4;
    unsigned long CSM:1;
    unsigned long CSA:5;
    unsigned long CLD:3;
} sceGsTex0;

typedef struct {
    unsigned long WMS:2;
    unsigned long WMT:2;
    unsigned long MINU:10;
    unsigned long MAXU:10;
    unsigned long MINV:10;
    unsigned long MAXV:10;
    unsigned long pad44:20;
} sceGsClamp;

typedef struct {
    sceGsTexflush   texflush;
    long        texflushaddr;
    sceGsTex1   tex11;
    long        tex11addr;
    sceGsTex0   tex01;
    long        tex01addr;
    sceGsClamp  clamp1;
    long        clamp1addr;
} sceGsTexEnv __attribute__((aligned(16)));

    tex01.TBP0 = tbp0
    tex01.TBW = tbw
    tex01.PSM = psm
    tex01.TW = w
    tex01.TH = h
    tex01.TCC = 1
    tex01.TFX = tfx
    tex01.CBP = cbp
    tex01.CPSM = cpsm
    tex01.CLD = cld
    clamp1.WMS = 1
    clamp1.WMT = 1

*/
inline void AndyGsSetDefTexEnv(sceGsTexEnv *tp, short flush, short tbp0, short tbw, short psm, short w, short h, short tfx, short cbp, short cpsm, short cld, short filter)
{
    (void)flush;

    // Setup addresses
    tp->texflushaddr = 0x3f;
    tp->tex11addr    = 0x14;
    tp->tex01addr    = 0x06;
    tp->clamp1addr   = 0x08;

    // Setup tex1 register
    ((u64*)(&tp->tex11))[0] = 0;
    tp->tex11.MMAG       = filter & 1;
    tp->tex11.MMIN       = filter;

    // Setup tex0 register
    ((u64*)(&tp->tex01))[0] = 0;
    tp->tex01.TBP0  = tbp0;
    tp->tex01.TBW   = tbw;
    tp->tex01.PSM   = psm;
    tp->tex01.TW    = w;
    tp->tex01.TH    = h;
    tp->tex01.TCC   = 1;
    tp->tex01.TFX   = tfx;
    tp->tex01.CBP   = cbp;
    tp->tex01.CPSM  = cpsm;
    tp->tex01.CLD   = cld;

    // Setup clamp register
    ((u64*)(&tp->clamp1))[0] = 0;
    tp->clamp1.WMS = 1;
    tp->clamp1.WMT = 1;

    //sceGsSetDefTexEnv(tp,flush,tbp0,tbw,psm,w,h,tfx,cbp,cpsm,cld,filter);
}

//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================

s32 VRAM_AllocTextureRef( void )
{
    s32 IncreaseSize = 128;
    static s32 AllocCursor=0;

    // Check if we need to reallocate a larger set
    if( s_NTextureRefs == s_NTextureRefsAllocated )
    {
        s_NTextureRefsAllocated += IncreaseSize;
        s_TextureRef = (texture_ref*)x_realloc(s_TextureRef,sizeof(texture_ref)*s_NTextureRefsAllocated);
        x_memset( s_TextureRef+s_NTextureRefs, 0, sizeof(texture_ref)*IncreaseSize );
        AllocCursor = s_NTextureRefs;
    }

    // Do linear search for empty slot starting with last allocated
    for( s32 i=0; i<s_NTextureRefsAllocated; i++ )
    {
        if( s_TextureRef[AllocCursor].InUse == 0 )
        {
            s_TextureRef[AllocCursor].InUse = 1;
            s_NTextureRefs++;
            return AllocCursor;
        }

        AllocCursor = (AllocCursor+1)%s_NTextureRefsAllocated;
    }

    ASSERT(FALSE);
    return -1;
}

//==========================================================================

s32 VRAM_AllocPixelRef( void )
{
    s32 IncreaseSize = 512;
    static s32 AllocCursor=0;

    // Check if we need to reallocate a larger set
    if( s_NPixelRefs == s_NPixelRefsAllocated )
    {
        s_NPixelRefsAllocated += IncreaseSize;
        s_PixelRef = (pixel_ref*)x_realloc(s_PixelRef,sizeof(pixel_ref)*s_NPixelRefsAllocated);
        x_memset( s_PixelRef+s_NPixelRefs, 0, sizeof(pixel_ref)*IncreaseSize );
        AllocCursor = s_NPixelRefs;
    }

    // Do linear search for empty slot starting with last allocated
    for( s32 i=0; i<s_NPixelRefsAllocated; i++ )
    {
        if( s_PixelRef[AllocCursor].InUse == 0 )
        {
            s_PixelRef[AllocCursor].InUse = 1;
            s_NPixelRefs++;
            return AllocCursor;
        }

        AllocCursor = (AllocCursor+1)%s_NPixelRefsAllocated;
    }

    ASSERT(FALSE);
    return -1;
}

//==========================================================================

s32 VRAM_AllocClutRef( void )
{
    s32 IncreaseSize = 128;
    static s32 AllocCursor=0;

    // Check if we need to reallocate a larger set
    if( s_NClutRefs == s_NClutRefsAllocated )
    {
        s_NClutRefsAllocated += IncreaseSize;
        s_ClutRef = (clut_ref*)x_realloc(s_ClutRef,sizeof(clut_ref)*s_NClutRefsAllocated);
        x_memset( s_ClutRef+s_NClutRefs, 0, sizeof(clut_ref)*IncreaseSize );
        AllocCursor = s_NClutRefs;
    }

    // Do linear search for empty slot starting with last allocated
    for( s32 i=0; i<s_NClutRefsAllocated; i++ )
    {
        if( s_ClutRef[AllocCursor].InUse == 0 )
        {
            s_ClutRef[AllocCursor].InUse = 1;
            s_NClutRefs++;
            return AllocCursor;
        }

        AllocCursor = (AllocCursor+1)%s_NClutRefsAllocated;
    }

    ASSERT(FALSE);
    return -1;
}

//==========================================================================

void VRAM_DeallocTextureRef( s32 ID )
{
    if( ID==-1) return;
    ASSERT( (ID>=0) && (ID<s_NTextureRefsAllocated) );
    ASSERT( s_TextureRef[ID].InUse == 1 );
    s_TextureRef[ID].InUse = 0;
    s_NTextureRefs--;
}

//==========================================================================

void VRAM_DeallocPixelRef( s32 ID )
{
    if( ID==-1) return;
    ASSERT( (ID>=0) && (ID<s_NPixelRefsAllocated) );
    ASSERT( s_PixelRef[ID].InUse == 1 );
    s_PixelRef[ID].InUse = 0;
    s_NPixelRefs--;
}

//==========================================================================

void VRAM_DeallocClutRef( s32 ID )
{
    if( ID==-1) return;
    ASSERT( (ID>=0) && (ID<s_NClutRefsAllocated) );
    ASSERT( s_ClutRef[ID].InUse == 1 );
    s_ClutRef[ID].InUse = 0;
    s_NClutRefs--;
}

//==========================================================================

void SetupBank0( s32 StartAddr )
{
    // Setup bank 0
    s_Bank[0].ClutBaseAddr       = StartAddr;
    s_Bank[0].NClutBlocks        = 32;
    s_Bank[0].ClutBlockRefCursor = -1;
    s_Bank[0].PixelBaseAddr      = s_Bank[0].ClutBaseAddr + (4*s_Bank[0].NClutBlocks);
    s_Bank[0].NPixelBlocks       = VRAM_PIXEL_END_ADDR - s_Bank[0].PixelBaseAddr;
    s_Bank[0].PixelEndAddr       = s_Bank[0].PixelBaseAddr + s_Bank[0].NPixelBlocks;
    s_Bank[0].PixelBlockRefCursor = s_Bank[0].PixelBlockRefHead;
    ASSERT( s_Bank[0].PixelEndAddr == VRAM_PIXEL_END_ADDR );

    pixel_ref* pPRef = s_PixelRef + s_Bank[0].PixelBlockRefHead;
    pPRef->BlockBase = s_Bank[0].PixelBaseAddr;
    pPRef->NBlocks   = 0;
    pPRef->NextRef   = -1;
    pPRef->PrevRef   = -1;
}

//==========================================================================

void vram_Reset( void )
{
    s32 i,j;

    // Init banks
    for( i=0; i<VRAM_MAX_BANKS; i++ )
    {
        // Clear clut info
        s_Bank[i].ClutBaseAddr = -1;
        s_Bank[i].NClutBlocks  = -1;
        for( j=0; j<VRAM_MAX_CLUT_REFS_IN_VRAM; j++ )
            s_Bank[i].ClutBlockRef[j] = -1;
        s_Bank[i].ClutBlockRefCursor = -1;

        // Clear pixel info
        s_Bank[i].PixelBaseAddr = -1;
        s_Bank[i].NPixelBlocks  = -1;
        s_Bank[i].PixelBlockRefCursor = -1;

        s_Bank[i].NClutRefsInBank   = 0;
        s_Bank[i].NPixelRefsInBank  = 0;
        s_Bank[i].NBlocksUsedInBank = 0;

        pixel_ref* pPRef = s_PixelRef + s_Bank[i].PixelBlockRefHead;
        pPRef->BlockBase = -1;
        pPRef->NBlocks   = -1;
        pPRef->NextRef   = -1;
        pPRef->PrevRef   = -1;

    }

    SetupBank0( VRAM_PIXEL_BASE_ADDR );

    s_NBanks = 1;
    s_VRAMClean = TRUE;

/*
    s_NPixelRefsInVRAM = 0;
    s_NClutRefsInVRAM  = 0;
    s_NBlocksUsedInVRAM = 0;
*/
}

//==========================================================================

void vram_Init( void )
{
    s32 i;

    ASSERT( s_Inited == FALSE );
    s_Inited = TRUE;

    // Build load image instructions for cluts
    AndyGsSetDefLoadImage( &s_CLUTLoad4.Header, VRAM_PIXEL_BASE_ADDR, 1, SCE_GS_PSMCT32, 0, 0,  8,  2 );
    AndyGsSetDefLoadImage( &s_CLUTLoad8.Header, VRAM_PIXEL_BASE_ADDR, 1, SCE_GS_PSMCT32, 0, 0, 16, 16 );
    s_CLUTLoad4.DMA.SetCont( sizeof(sceGsLoadImage) );
    s_CLUTLoad4.DMA.MakeDirect();
    s_CLUTLoad8.DMA.SetCont( sizeof(sceGsLoadImage) );
    s_CLUTLoad8.DMA.MakeDirect();

    // Init log table
    s_LogTable[0] = -127.0f;
    for( i=1; i<256; i++ )
    {
        ieee32 F;
        F.s.sign = 0;
        F.s.exp  = 127;
        F.s.mant = i<<15;
        s_LogTable[i] = x_log2(F.v) - 127.0f;
    }

    // Init vram lists
    s_NTextureRefs = 0;
    s_NTextureRefsAllocated = 128;
    s_TextureRef = (texture_ref*)x_malloc(sizeof(texture_ref)*s_NTextureRefsAllocated);
    ASSERT(s_TextureRef);
    x_memset(s_TextureRef,0,sizeof(texture_ref)*s_NTextureRefsAllocated);

    s_NPixelRefs = 0;
    s_NPixelRefsAllocated = 512;
    s_PixelRef = (pixel_ref*)x_malloc(sizeof(pixel_ref)*s_NPixelRefsAllocated);
    ASSERT(s_PixelRef);
    x_memset(s_PixelRef,0,sizeof(pixel_ref)*s_NPixelRefsAllocated);
    
    s_NClutRefs = 0;
    s_NClutRefsAllocated = 128;
    s_ClutRef = (clut_ref*)x_malloc(sizeof(clut_ref)*s_NClutRefsAllocated);
    ASSERT(s_ClutRef);
    x_memset(s_ClutRef,0,sizeof(clut_ref)*s_NClutRefsAllocated);
/*
struct bank
{
    s32     ClutBaseAddr;
    s32     NClutBlocks;
    s32     ClutBlockRefCursor;
    s32     ClutBlockRef[32];

    s32     PixelBaseAddr;
    s32     NPixelBlocks;
    s32     PixelBlockRefCursor;
    s32     PixelBlockRefHead;
};
*/
    // Init banks
    for( i=0; i<VRAM_MAX_BANKS; i++ )
    {
        x_memset(&s_Bank[i],0,sizeof(bank));
        s_Bank[i].PixelBlockRefHead = VRAM_AllocPixelRef();
    }
/*
    s_NPixelRefsInVRAM = 0;
    s_NClutRefsInVRAM  = 0;
    s_NBlocksUsedInVRAM = 0;
*/
    vram_Reset();

    //DB("VRAM: Initialized\n");
}

//==========================================================================

void VRAM_Kill( void )
{
    ASSERT( s_Inited == TRUE );
    s_Inited = FALSE;

    x_free( s_TextureRef );
    x_free( s_PixelRef );
    x_free( s_ClutRef );
    s_TextureRef = NULL;
    s_PixelRef   = NULL;
    s_ClutRef    = NULL;
}

//==========================================================================

s32 vram_GetLog2( s32 N )
{
    s32 j;
    ASSERT( N<=1024 );
    for( j=1; j<=10; j++ ) if( (1<<j)==N ) break;
    return j;
}

//==========================================================================

byte s_8BitBlocks[8*4] = 
{
    0,  1,  4,  5,  16, 17, 20, 21,
    2,  3,  6,  7,  18, 19, 22, 23,
    8,  9,  12, 13, 24, 25, 28, 29,
    10, 11, 14, 15, 26, 27, 30, 31,
};

//==========================================================================

void Compute8BitLayout( s32 W, s32 H, s16& PageWidth, s16& NBlocks )
{
    // If we fit entirely in a page
    if( (W<=128) && (H<=64) )
    {
        PageWidth = 0;
        NBlocks   = 0;

        // Compute number of blocks 
        s32 BW = (W+8)/16;
        s32 BH = (H+8)/16;
        ASSERT((BW<=8) && (BH<=4));

        // Search for largest block
        for( s32 y=0; y<BH; y++ )
        for( s32 x=0; x<BW; x++ )
        {
            NBlocks = MAX(NBlocks,s_8BitBlocks[x+(y<<3)]);
        }
        // Increment number of blocks
        NBlocks++;
        NBlocks = ALIGN_2(NBlocks);
        ASSERT(NBlocks<=32);

        //printf("(%1d,%1d) %1d %1d\n",W,H,PageWidth,NBlocks);

        return;
    }

    // We don't fit in a page so determine our page width

    // Determine page width
    s32 PH = MAX(1,H/64);
    s32 PW = ALIGN_2(MAX(1,(W/128)));
    PageWidth = PW*2;

    NBlocks = ALIGN_256(PW*PH*(128*64))/256;
    NBlocks = ALIGN_2(NBlocks);

    // Check if we can get back one page...
    if( PW > MAX(1,(W/128)) )
        NBlocks -= 32;

    //printf("(%1d,%1d) %1d %1d\n",W,H,PageWidth,NBlocks);

}

//==========================================================================

s32 VRAM_SetupPixelRef( byte* pData, s32 Width, s32 Height, s32 BPP, s32 TextureRef, s32 Format, s32 MipLevel )
{
    s32 PixelRef;
    pixel_ref* pPRef;

    PixelRef = VRAM_AllocPixelRef();
    pPRef = s_PixelRef + PixelRef;

    pPRef->pData        = pData;
    pPRef->Width        = (s16)Width;
    pPRef->Height       = (s16)Height;
    pPRef->WidthLog2    = vram_GetLog2(pPRef->Width);        
    pPRef->HeightLog2   = vram_GetLog2(pPRef->Height);
    pPRef->MipLevel     = (s16)MipLevel;
    pPRef->TextureRef   = (s16)TextureRef;
    pPRef->Format       = (s16)Format;
    pPRef->Bank         = -1;

    pPRef->BlockBase    = -1;
    pPRef->NextRef      = -1;
    pPRef->PrevRef      = -1;

    // Determine page width and number of blocks
    if( BPP<=8 )
    {
        Compute8BitLayout(Width,Height,pPRef->PageWidth,pPRef->NBlocks);
    }
    else
    {
        //ASSERT( (Width==Height) && "Tell Andy!!!" );
        pPRef->PageWidth    = MAX(1,(Width/64));
        pPRef->NBlocks      = ALIGN_256( ((Width*Height)*BPP)/8 ) / 256;
        pPRef->NBlocks      = MAX(1,pPRef->NBlocks);
    }

    return PixelRef;
}

//==========================================================================

s32 VRAM_SetupClutRef( byte* pData, s32 NColors, s32 TextureRef )
{
    s32 ClutRef;
    clut_ref* pCRef;

    ClutRef = VRAM_AllocClutRef();
    pCRef = s_ClutRef + ClutRef;

    pCRef->pData        = pData;
    pCRef->TextureRef   = (s16)TextureRef;
    pCRef->NColors      = (s16)NColors;
    pCRef->BlockBase    = -1;

    return ClutRef;
}

//==========================================================================

s32 vram_Register( const xbitmap& BMP )
{
    s32 TextureRef;
    texture_ref* pTRef;
    //s32 j;
    
    // Be sure BMP has not already been registered
    ASSERT( BMP.GetVRAMID() == 0 );

    // Confirm it is a valid format
    switch( BMP.GetFormat() )
    {
        case xbitmap::FMT_32_ABGR_8888:
        case xbitmap::FMT_32_UBGR_8888:
        case xbitmap::FMT_16_ABGR_1555:
        case xbitmap::FMT_16_UBGR_1555:
        case xbitmap::FMT_P8_ABGR_8888:
        case xbitmap::FMT_P8_UBGR_8888:
        case xbitmap::FMT_P4_ABGR_8888:
        case xbitmap::FMT_P4_UBGR_8888:
            break;
        default:
            ASSERTS(FALSE,"Bad bitmap format for ps2");
    }

    TextureRef = VRAM_AllocTextureRef();
    pTRef = s_TextureRef + TextureRef;

    // Setup width and height
    pTRef->Width = BMP.GetWidth();
    pTRef->Height = BMP.GetHeight();
    pTRef->WidthLog2 = vram_GetLog2(pTRef->Width);        
    pTRef->HeightLog2 = vram_GetLog2(pTRef->Height);
    ASSERT( pTRef->Width  >= 8 );
    ASSERT( pTRef->Height >= 8 );


    // Determine pixel format
    xbitmap::format_info Format = BMP.GetFormatInfo();
    pTRef->BPP = Format.BPP;
    switch( pTRef->BPP )
    {
        case 32:    pTRef->PixelFormat = SCE_GS_PSMCT32;
                    break;
        case 16:    pTRef->PixelFormat = SCE_GS_PSMCT16;
                    break;
        case 8:     pTRef->PixelFormat = SCE_GS_PSMT8;
                    break;
        case 4:     pTRef->PixelFormat = SCE_GS_PSMT4;
                    break;
        default:    ASSERT(FALSE);
                    break;                    
    }

    // Setup pixel ref for texture
    pTRef->PixelRef = VRAM_SetupPixelRef( (byte*)BMP.GetPixelData(0),
                                     BMP.GetWidth(),
                                     BMP.GetHeight(),
                                     Format.BPP,
                                     TextureRef,
                                     pTRef->PixelFormat,
                                     0 );
    
    // Setup clut ref for texture
    if( pTRef->BPP <= 8 )
        pTRef->ClutRef = VRAM_SetupClutRef( (byte*)BMP.GetClutData(), (pTRef->BPP==8)?(256):(16), TextureRef );
    else
        pTRef->ClutRef = -1;

    // Check if mipmaps
    pTRef->NMips = BMP.GetNMips();
    pTRef->NMips = MAX(pTRef->NMips,0);
    for( s32 i=0; i<MAX_MIPS; i++ ) pTRef->MipRef[i] = -1;
    for( s32 i=0; i<pTRef->NMips; i++ )
    {
        pTRef->MipRef[i] = VRAM_SetupPixelRef( (byte*)BMP.GetPixelData(i+1),
                                          BMP.GetWidth(i+1),
                                          BMP.GetHeight(i+1),
                                          pTRef->BPP,
                                          TextureRef,
                                          pTRef->PixelFormat,
                                          i+1 );
    }

    // Build tex01 reference
    if( pTRef->BPP <= 8 )
    {
        // Setup tex0 register
        pTRef->tex01.TCC   = 1;
        pTRef->tex01.TFX   = 0;
        pTRef->tex01.CPSM  = SCE_GS_PSMCT32;
        pTRef->tex01.CLD   = 1;
        pTRef->tex01.CSM   = 0;
        pTRef->tex01.CSA   = 0;
        pTRef->tex01.TBP0  = 0;//s_PixelRef[ pTRef->PixelRef ].BlockBase;
        pTRef->tex01.TBW   = s_PixelRef[ pTRef->PixelRef ].PageWidth;
        pTRef->tex01.PSM   = pTRef->PixelFormat;
        pTRef->tex01.TW    = pTRef->WidthLog2;
        pTRef->tex01.TH    = pTRef->HeightLog2;
        pTRef->tex01.CBP   = 0;//s_ClutRef[ pTRef->ClutRef ].BlockBase;
    }
    else
    {
        // Setup tex0 register
        pTRef->tex01.TCC   = 1;
        pTRef->tex01.TFX   = 0;
        pTRef->tex01.CBP   = 0;
        pTRef->tex01.CPSM  = 0;
        pTRef->tex01.CLD   = 0;
        pTRef->tex01.CSM   = 0;
        pTRef->tex01.CSA   = 0;
        pTRef->tex01.TBP0  = 0;//s_PixelRef[ pTRef->PixelRef ].BlockBase;
        pTRef->tex01.TBW   = s_PixelRef[ pTRef->PixelRef ].PageWidth;
        pTRef->tex01.PSM   = pTRef->PixelFormat;
        pTRef->tex01.TW    = pTRef->WidthLog2;
        pTRef->tex01.TH    = pTRef->HeightLog2;

    }

    if( pTRef->NMips >= 1 ) pTRef->MBP1.TBW1  = s_PixelRef[ pTRef->MipRef[0] ].PageWidth;
    if( pTRef->NMips >= 2 ) pTRef->MBP1.TBW2  = s_PixelRef[ pTRef->MipRef[1] ].PageWidth;
    if( pTRef->NMips >= 3 ) pTRef->MBP1.TBW3  = s_PixelRef[ pTRef->MipRef[2] ].PageWidth;
    if( pTRef->NMips >= 4 ) pTRef->MBP2.TBW4  = s_PixelRef[ pTRef->MipRef[3] ].PageWidth;
    if( pTRef->NMips >= 5 ) pTRef->MBP2.TBW5  = s_PixelRef[ pTRef->MipRef[4] ].PageWidth;
    if( pTRef->NMips >= 6 ) pTRef->MBP2.TBW6  = s_PixelRef[ pTRef->MipRef[5] ].PageWidth;

    // Load texture reference into BMP
    BMP.SetVRAMID( TextureRef+1 );

    return TextureRef+1;

    //DB(fs("VRAM: New texture %1d\n",TextureRef));
}

//==========================================================================


void VRAM_BuildTexturePacket(   s32   Width,
                                s32   Height,
                                s32   PageWidth,
                                byte* pPixel, 
                                s32   PixelFormat,
                                s32   VRAMPixelAddr )
{
    image_load* pIL = DLStruct(image_load);
    s32 QWImageSize;
    s32 ImageBytes;

    ASSERT( VRAMPixelAddr >= VRAM_PIXEL_BASE_ADDR );
    
    // Setup initial dma
    pIL->DMA.SetCont( sizeof(sceGsLoadImage) );
    pIL->DMA.MakeDirect();

    // Setup transfer header info
    ImageBytes = AndyGsSetDefLoadImage( &pIL->Header, 
                                        (s16)VRAMPixelAddr, 
                                        (s16)PageWidth,
                                        (s16)PixelFormat, 
                                        0, 0, 
                                        (s16)Width, 
                                        (s16)Height);
    if( ImageBytes == 0 )
    {
        // Setup DMA Ref to pixel data
        QWImageSize  = MIN(32767,pIL->Header.giftag1.NLOOP);
        pIL->DMA1.SetRef( QWImageSize<<4, (u32)pPixel );
        pIL->DMA1.MakeDirect();
        return;
    }

    // Data needs to be sent in multiple transfers
    s32 NQWords = ImageBytes>>4;
    s32 Offset = 0;

    while( NQWords > 0 )
    {
        s32 NQWordsToSend = MIN(32767,NQWords);
        giftag* pGT;
        dmatag* pDMA;

        // Check if we can use previous giftag
        if( Offset==0 ) 
        {
            pGT  = (giftag*)(&pIL->Header.giftag1);
            pDMA = (dmatag*)(&pIL->DMA1);

            // Setup giftag for transfer
            pGT->BuildImageLoad( NQWordsToSend<<4, 1 );

            // Setup dma ref
            pDMA->SetRef( NQWordsToSend<<4, (u32)(pPixel) );
            pDMA->MakeDirect();
        }
        else            
        {
            image_load_ext* pExt = DLStruct(image_load_ext);

            pExt->DMA.SetCont(sizeof(giftag));
            pExt->DMA.MakeDirect();

            // Setup giftag for transfer
            pExt->GIF.BuildImageLoad( NQWordsToSend<<4, 1 );

            // Setup dma ref
            pExt->REF.SetRef( NQWordsToSend<<4, (u32)(pPixel+Offset) );
            pExt->REF.MakeDirect();

        }

        Offset += (NQWordsToSend<<4);
        NQWords -= NQWordsToSend;
    }
    
}

//==========================================================================

void VRAM_UnloadPixelRef( s32 Ref )
{
    if( s_PixelRef[Ref].BlockBase == -1 )
        return;

    // Determine which bank the texture is loaded in
    ASSERT( s_PixelRef[Ref].Bank != -1 );
    bank* pB = &s_Bank[s_PixelRef[Ref].Bank];

    // Check if it was last-added cursor
    if( Ref == pB->PixelBlockRefCursor )
        pB->PixelBlockRefCursor = pB->PixelBlockRefHead;

    pB->NPixelRefsInBank--;
    pB->NBlocksUsedInBank -= s_PixelRef[Ref].NBlocks;

    // Remove from linked list
    s_PixelRef[Ref].BlockBase = -1;
    if( s_PixelRef[Ref].PrevRef!=-1 )
        s_PixelRef[s_PixelRef[Ref].PrevRef].NextRef = s_PixelRef[Ref].NextRef;
    if( s_PixelRef[Ref].NextRef!=-1 )
        s_PixelRef[s_PixelRef[Ref].NextRef].PrevRef = s_PixelRef[Ref].PrevRef;
    s_PixelRef[Ref].NextRef = -1;
    s_PixelRef[Ref].PrevRef = -1;
}

//==========================================================================
void CheckMovieValid(void);

void VRAM_LoadPixelRef( s32 BI, s32 Ref )
{
    pixel_ref* pPRef;
    pixel_ref* pPCRef;
    bank* pB = s_Bank + BI;

	CheckMovieValid();
    s_VRAMClean = FALSE;

    // Get ptr to pixel_ref
    pPRef  = s_PixelRef + Ref;

    // Decide if we want this aligned toa page boundry
    xbool DoAlign = ( pPRef->NBlocks >= 96 )?(1):(0);
    s32   NBlocksNeeded = pPRef->NBlocks;

    if( DoAlign )
    {
        s32 Padding = s_PixelRef[pB->PixelBlockRefCursor].BlockBase+
                      s_PixelRef[pB->PixelBlockRefCursor].NBlocks;

        Padding = ALIGN_32(Padding) - Padding;
        NBlocksNeeded += Padding;
    }

    // Confirm that we are guaranteed to find a fit
    ASSERT( pB->NPixelBlocks >= NBlocksNeeded );

    // Check if pixels will fit and reset to beginning of list if it doesn't
    s32 BlocksRemaining = pB->PixelEndAddr -
                          (s_PixelRef[pB->PixelBlockRefCursor].BlockBase +
                           s_PixelRef[pB->PixelBlockRefCursor].NBlocks);

    if( BlocksRemaining < NBlocksNeeded )
    {
        pB->PixelBlockRefCursor = pB->PixelBlockRefHead;
        NBlocksNeeded = pPRef->NBlocks;
        if( DoAlign )
        {
            s32 Padding = s_PixelRef[pB->PixelBlockRefCursor].BlockBase+
                          s_PixelRef[pB->PixelBlockRefCursor].NBlocks;

            Padding = ALIGN_32(Padding) - Padding;
            NBlocksNeeded += Padding;
        }
    }

    // Get ptr to cursor
    pPCRef = s_PixelRef + pB->PixelBlockRefCursor;

    // Loop and remove remove all blocks until there is enough room
    while( 1 )
    {
        s32 NBlocks;
        s32 NextRef;
        
        // Compute NBlocks between cursor and next
        NextRef = pPCRef->NextRef;
        if( NextRef==-1 )
            NBlocks = pB->PixelEndAddr - (pPCRef->BlockBase + pPCRef->NBlocks);
        else
            NBlocks = s_PixelRef[NextRef].BlockBase - (pPCRef->BlockBase + pPCRef->NBlocks);

        // If enough blocks bail
        if( NBlocks >= NBlocksNeeded )
            break;

        // Still not enough blocks so unload NextRef
        //ASSERT( s_PixelRef[NextRef].TextureRef != pPRef->TextureRef );
        VRAM_UnloadPixelRef(NextRef);
    }

    // There is enough room so add our block after cursor

    // Link our ref into list behind cursor
    pPRef->NextRef = pPCRef->NextRef;
    pPRef->PrevRef = pB->PixelBlockRefCursor;
    if( pPCRef->NextRef != -1 )
        s_PixelRef[pPCRef->NextRef].PrevRef = Ref;
    pPCRef->NextRef = Ref;

    // Setup memory location
    pPRef->BlockBase = pPCRef->BlockBase + pPCRef->NBlocks + (NBlocksNeeded-pPRef->NBlocks);
    pPRef->Bank = BI;

    // Setup our newly added ref as the new cursor
    pB->PixelBlockRefCursor = Ref;
    pB->NPixelRefsInBank++;    
    pB->NBlocksUsedInBank += NBlocksNeeded;

    ASSERT( (pPRef->BlockBase) < pB->PixelEndAddr );
    ASSERT( (pPRef->BlockBase + pPRef->NBlocks) <= pB->PixelEndAddr );

    ASSERT( (pPRef->BlockBase) >= pB->PixelBaseAddr );
    ASSERT( (pPRef->BlockBase + pPRef->NBlocks) > pB->PixelBaseAddr );

    // Add commands to dma up pixel data
    VRAM_BuildTexturePacket( pPRef->Width, pPRef->Height, pPRef->PageWidth, pPRef->pData, pPRef->Format, pPRef->BlockBase );

    //DB(fs("VRAM: %1d Load Pixels %1d at %1d\n",s_NPixelRefsInVRAM,Ref,pPRef->BlockBase));
}

//=========================================================================


void VRAM_BuildPalPacket( image_load* pIL,
                     byte* pClut,
                     s32   NColors,
                     s32   ClutAddr)
{
/*
    s32 QWImageSize;

    // Setup dma of clut header and vif DIRECT cmd
    pIL->DMA.SetCont( sizeof(sceGsLoadImage) );
    pIL->DMA.MakeDirect();
    
    // Setup pixel transfer header info
    if( NColors <= 16 )
    {
        AndyGsSetDefLoadImage(   &pIL->Header, 
                                (s16)ClutAddr, 
                                (s16)1, 
                                (s16)SCE_GS_PSMCT32, 
                                0, 0, 
                                (s16)8, 
                                (s16)2);
    }
    else                                
    if( NColors <= 256 )
    {
        AndyGsSetDefLoadImage(   &pIL->Header, 
                                (s16)ClutAddr, 
                                (s16)1, 
                                (s16)SCE_GS_PSMCT32, 
                                0, 0, 
                                (s16)16, 
                                (s16)16);
    }
    else
    {
        ASSERT(FALSE);
    }

    // Setup DMA Ref to clut data
    QWImageSize  = pIL->Header.giftag1.NLOOP;
    pIL->DMA1.SetRef( QWImageSize<<4, (u32)pClut );
    pIL->DMA1.MakeDirect();
*/

    if( NColors <= 16 )
    {
        *pIL = s_CLUTLoad4;
        VRAM_BytesLoaded += 16*4;
    }
    else
    {
        *pIL = s_CLUTLoad8;
        VRAM_BytesLoaded += 256*4;
    }

    ASSERT( ClutAddr >= VRAM_PIXEL_BASE_ADDR );
    ASSERT( ClutAddr <= (VRAM_PIXEL_END_ADDR-4) );

    pIL->Header.bitbltbuf.DBP = ClutAddr;

    pIL->DMA1.SetRef( pIL->Header.giftag1.NLOOP<<4, (u32)pClut );
    pIL->DMA1.MakeDirect();

}

//==========================================================================

void VRAM_UnloadClutRef( s32 Ref )
{
    s32 BI;

    if( Ref==-1 )
        return;

    if( s_ClutRef[Ref].BlockBase == -1 )
        return;

    // Determine which bank clut is in
    BI = s_ClutRef[Ref].Bank;
    ASSERT( (BI>=0) && (BI<s_NBanks) );

    // Clear Block location
    s_ClutRef[Ref].BlockBase = -1;
    s_ClutRef[Ref].Bank      = -1;

    s_Bank[BI].NClutRefsInBank--;
}

//==========================================================================

void VRAM_LoadClutRef( s32 BI, s32 Ref )
{
    bank* pB = s_Bank + BI;

    s_VRAMClean = FALSE;

    // Pick random location
    //s_ClutBlockRefCursor = x_irand(0,VRAM_MAX_CLUT_REFS_IN_VRAM-1);

    // Increment Clut cursor to next
    pB->ClutBlockRefCursor = (pB->ClutBlockRefCursor+1)%pB->NClutBlocks;

    // If anybody is here...remove them
    if( pB->ClutBlockRef[pB->ClutBlockRefCursor] != -1 )
    {
        VRAM_UnloadClutRef( pB->ClutBlockRef[pB->ClutBlockRefCursor] );
        //s_ClutRef[ s_ClutBlockRef[s_ClutBlockRefCursor] ].BlockBase = -1;
    }

    // Store location
    pB->ClutBlockRef[pB->ClutBlockRefCursor] = Ref;
    s_ClutRef[Ref].BlockBase = pB->ClutBaseAddr + 4*pB->ClutBlockRefCursor;
    s_ClutRef[Ref].Bank = BI;

    pB->NClutRefsInBank++;


    ASSERT( (s_ClutRef[Ref].BlockBase) < pB->PixelBaseAddr );
    ASSERT( (s_ClutRef[Ref].BlockBase + 4) <= pB->PixelBaseAddr );

    ASSERT( (s_ClutRef[Ref].BlockBase) >= pB->ClutBaseAddr );
    ASSERT( (s_ClutRef[Ref].BlockBase + 4) > pB->ClutBaseAddr );

    // Load clut into location
    image_load* pIL = DLStruct(image_load);
    VRAM_BuildPalPacket( pIL, s_ClutRef[Ref].pData, s_ClutRef[Ref].NColors, s_ClutRef[Ref].BlockBase );

    //DB(fs("VRAM: Load Clut %1d at %1d\n",Ref,s_ClutRef[Ref].BlockBase));
}

//==========================================================================

void FlushBank( bank& B )
{

    if( B.NClutRefsInBank > 0 )
    {
        // Flush all cluts
        for( s32 i=0; i<B.NClutBlocks; i++ )
        {
            if( B.ClutBlockRef[i] != -1 )
                VRAM_UnloadClutRef( B.ClutBlockRef[i] );
            B.ClutBlockRef[i] = -1;
        }
        B.ClutBlockRefCursor = -1;
    }

    if( B.NPixelRefsInBank==0 )
        return;

    // Flush all pixel references 
    while( s_PixelRef[B.PixelBlockRefHead].NextRef != -1 )
    {
        VRAM_UnloadPixelRef( s_PixelRef[B.PixelBlockRefHead].NextRef );
    }

    // Reset initial pixel references
    {
        B.PixelBlockRefCursor = B.PixelBlockRefHead;
        pixel_ref* pPRef = s_PixelRef + B.PixelBlockRefHead;
        pPRef->BlockBase = B.PixelBaseAddr;
        pPRef->NBlocks   = 0;
        pPRef->NextRef   = -1;
        pPRef->PrevRef   = -1;
    }
}

//=========================================================================
/*
struct mip_packet
{
    dmatag          DMA;
    giftag          GIF;
    sceGsTex1       TEX1;
    u64             TEX1Addr;
    sceGsMiptbp1    MBP1;
    u64             MBP1Addr;
    sceGsMiptbp2    MBP2;
    u64             MBP2Addr;
};

void BuildMipPacket( s32 TextureRef )
{
    // Get packet from display list
    mip_packet* pPacket = DLStruct(mip_packet);

    // Setup DMA through VIF1 to GIF
    pPacket->DMA.SetCont( sizeof(mip_packet) - sizeof(sceDmaTag) );
    pPacket->DMA.MakeDirect();

    // Setup giftag for manual addresses
    pPacket->GIF.BuildRegLoad(3,TRUE);

    // Set addresses
    s32 Context = ENG_GetGSContext();
    pPacket->TEX1Addr   = Context + 0x14;
    pPacket->MBP1Addr   = Context + 0x34;
    pPacket->MBP2Addr   = Context + 0x36;

    // Get ptr to texture ref
    texture_ref* pTRef = s_TextureRef + TextureRef;

    if( pTRef->NMips >= 1 )
    {
        pPacket->MBP1.TBP1  = s_PixelRef[ pTRef->MipRef[0] ].BlockBase;
        pPacket->MBP1.TBW1  = s_PixelRef[ pTRef->MipRef[0] ].PageWidth;
    }

    if( pTRef->NMips >= 2 )
    {
        pPacket->MBP1.TBP2  = s_PixelRef[ pTRef->MipRef[1] ].BlockBase;
        pPacket->MBP1.TBW2  = s_PixelRef[ pTRef->MipRef[1] ].PageWidth;
    }

    if( pTRef->NMips >= 3 )
    {
        pPacket->MBP1.TBP3  = s_PixelRef[ pTRef->MipRef[2] ].BlockBase;
        pPacket->MBP1.TBW3  = s_PixelRef[ pTRef->MipRef[2] ].PageWidth;
    }

    if( pTRef->NMips >= 4 )
    {
        pPacket->MBP2.TBP4  = s_PixelRef[ pTRef->MipRef[3] ].BlockBase;
        pPacket->MBP2.TBW4  = s_PixelRef[ pTRef->MipRef[3] ].PageWidth;
    }

    if( pTRef->NMips >= 5 )
    {
        pPacket->MBP2.TBP5  = s_PixelRef[ pTRef->MipRef[4] ].BlockBase;
        pPacket->MBP2.TBW5  = s_PixelRef[ pTRef->MipRef[4] ].PageWidth;
    }

    if( pTRef->NMips >= 6 )
    {
        pPacket->MBP2.TBP6  = s_PixelRef[ pTRef->MipRef[5] ].BlockBase;
        pPacket->MBP2.TBW6  = s_PixelRef[ pTRef->MipRef[5] ].PageWidth;
    }

    pPacket->TEX1.LCM   = (s_MipManualMode)?(1):(0);
    pPacket->TEX1.MXL   = pTRef->NMips;
    pPacket->TEX1.MMAG  = 1;
    pPacket->TEX1.MMIN  = 4;
    pPacket->TEX1.MTBA  = 0;
    pPacket->TEX1.L     = 0;
    pPacket->TEX1.K     = (s32)(s_MipK*16.0f);
}
*/
//==========================================================================

extern volatile f32 MIPK;

void VRAM_ActivateTextureAndMips( s32 TextureRef )
{
    struct env_load
    {
        dmatag          DMA;   // DMA texture environment settings
        giftag          GIF;
        sceGsTexflush   texflush;
        u64             texflushaddr;
        sceGsTex1       tex11;
        u64             tex11addr;
        sceGsTex0       tex01;
        u64             tex01addr;
        sceGsMiptbp1    MBP1;
        u64             MBP1Addr;
        sceGsMiptbp2    MBP2;
        u64             MBP2Addr;
    };

    env_load* pE = DLStruct(env_load);

    // Setup dma                        
    pE->DMA.SetCont(sizeof(env_load)-sizeof(dmatag));
    pE->DMA.MakeDirect();

    // Setup giftag to process register loads
    pE->GIF.BuildRegLoad(5,1);
    
    texture_ref* pTRef = s_TextureRef + TextureRef;

    pE->tex01 = pTRef->tex01;
    pE->tex01.TBP0  = s_PixelRef[ pTRef->PixelRef ].BlockBase;
    if( pTRef->BPP <= 8 )
    {
        pE->tex01.CBP   = s_ClutRef[ pTRef->ClutRef ].BlockBase;
    }

    //
    // Setup addresses in packet
    //
    s32 Context = eng_GetGSContext();
    pE->texflushaddr    = 0x3f;
    pE->tex11addr       = Context + 0x14;
    pE->tex01addr       = Context + 0x06;
    pE->MBP1Addr        = Context + 0x34;
    pE->MBP2Addr        = Context + 0x36;

    //
    // Setup mipmap registers
    //
    pE->MBP1 = pTRef->MBP1;
    pE->MBP2 = pTRef->MBP2;
    if( pTRef->NMips >= 1 ) pE->MBP1.TBP1  = s_PixelRef[ pTRef->MipRef[0] ].BlockBase;
    if( pTRef->NMips >= 2 ) pE->MBP1.TBP2  = s_PixelRef[ pTRef->MipRef[1] ].BlockBase;
    if( pTRef->NMips >= 3 ) pE->MBP1.TBP3  = s_PixelRef[ pTRef->MipRef[2] ].BlockBase;
    if( pTRef->NMips >= 4 ) pE->MBP2.TBP4  = s_PixelRef[ pTRef->MipRef[3] ].BlockBase;
    if( pTRef->NMips >= 5 ) pE->MBP2.TBP5  = s_PixelRef[ pTRef->MipRef[4] ].BlockBase;
    if( pTRef->NMips >= 6 ) pE->MBP2.TBP6  = s_PixelRef[ pTRef->MipRef[5] ].BlockBase;

    pE->tex11.MMAG  = 1;
    pE->tex11.MMIN  = 4;
    pE->tex11.MTBA  = 0;
    pE->tex11.L     = 0;
    pE->tex11.LCM   = (s_MipManualMode[Context])?(1):(0);
    pE->tex11.MXL   = pTRef->NMips;

    //pE->tex11.K     = (s32)(MIPK*16.0f);
    pE->tex11.K     = (s32)(s_MipK[Context]*16.0f);
}

//==========================================================================

void vram_SetMipEquation    ( f32 MipK, xbool Manual )
{
    s32 C = eng_GetGSContext();
    s_MipK[C] = MipK;
    s_MipManualMode[C] = Manual;
}

//==========================================================================
/*
typedef struct {
    sceGsTexflush   texflush;
    long        texflushaddr;
    sceGsTex1   tex11;
    long        tex11addr;
    sceGsTex0   tex01;
    long        tex01addr;
    sceGsClamp  clamp1;
    long        clamp1addr;
} sceGsTexEnv __attribute__((aligned(16)));
*/


void VRAM_ActivateTexture( s32 TextureRef )
{
    struct env_load
    {
        dmatag          DMA;   // DMA texture environment settings
        giftag          GIF;
        sceGsTexflush   texflush;
        u64             texflushaddr;
        sceGsTex1       tex11;
        u64             tex11addr;
        sceGsTex0       tex01;
        u64             tex01addr;
    };

	CheckMovieValid();
    env_load* pE = DLStruct(env_load);

    // Setup dma                        
    pE->DMA.SetCont(sizeof(env_load)-sizeof(dmatag));
    pE->DMA.MakeDirect();

    // Setup giftag to process register loads
    pE->GIF.BuildRegLoad(3,1);
    
    texture_ref* pTRef = s_TextureRef + TextureRef;

    pE->tex01 = pTRef->tex01;
    pE->tex01.TBP0  = s_PixelRef[ pTRef->PixelRef ].BlockBase;
    if( pTRef->BPP <= 8 )
    {
        pE->tex01.CBP   = s_ClutRef[ pTRef->ClutRef ].BlockBase;
    }

    ((u64*)(&pE->tex11))[0] = 0;
    pE->tex11.MMAG      = 1;
    pE->tex11.MMIN      = 1;

    //
    // Setup addresses in packet
    //
    s32 Context = eng_GetGSContext();
    pE->texflushaddr    = 0x3f;
    pE->tex11addr       = Context + 0x14;
    pE->tex01addr       = Context + 0x06;
}

//==========================================================================

void vram_ActivateMips( s32 VRAMID, f32 MinLOD, f32 MaxLOD, s32 BI )
{
//    TIME_Push();

    s32 TextureRef;
    texture_ref* pTRef;

	CheckMovieValid();
    // Get reference to texture
    TextureRef = VRAMID;
    ASSERT( (TextureRef>=0) && (TextureRef<s_NTextureRefsAllocated) );
    ASSERT( s_TextureRef[TextureRef].InUse == 1 );
    pTRef = s_TextureRef + TextureRef;

    // Compute integer mip range
    s32 MinMipI, MaxMipI;
    ComputeMipRange( VRAMID, MinLOD, MaxLOD, MinMipI, MaxMipI );
    //MinMipI = 0;
    //MaxMipI = pTRef->NMips;

    //x_printf("MinMipI:%1d MaxMipI:%1d\n",MinMipI,MaxMipI);

    s32 Count=0;
    while( 1 )
    {
        Count++;
        ASSERT( Count != 10 );

        // Load up pixel data if needed
        if( MinMipI==0 )
        {
            if( s_PixelRef[pTRef->PixelRef].BlockBase == -1 )
            {
                VRAM_LoadPixelRef( BI, pTRef->PixelRef );
            }
            else
            {
                ASSERT( s_PixelRef[pTRef->PixelRef].Bank == BI );
            }
        }

        // Load up clut data if needed
        if( pTRef->ClutRef!=-1) 
        {
            if( s_ClutRef[pTRef->ClutRef].BlockBase == -1 )
            {
                VRAM_LoadClutRef( BI, pTRef->ClutRef );
            }
            else
            {
                ASSERT( s_ClutRef[pTRef->ClutRef].Bank == BI );
            }
        }

        // Load up mips if needed
        if( pTRef->NMips > 0 )
        {
            for( s32 i=0; i<pTRef->NMips; i++ )
            if( (MinMipI<=(i+1)) && (MaxMipI>=(i+1)) )
            {
                if( s_PixelRef[pTRef->MipRef[i]].BlockBase == -1)
                {
                    VRAM_LoadPixelRef( BI, pTRef->MipRef[i] );
                }
                else
                {
                    ASSERT( s_PixelRef[pTRef->MipRef[i]].Bank == BI ); 
                }
            }
        }


        //
        //  Confirm placement
        //
        {
            xbool Correct = TRUE;

            if( MinMipI==0 )
            {
                if( s_PixelRef[pTRef->PixelRef].BlockBase == -1 )
                    Correct = FALSE;
            }

            if( pTRef->ClutRef != -1 )
            {
                if( s_ClutRef[pTRef->ClutRef].BlockBase == -1 )
                    Correct = FALSE;
            }

            if( pTRef->NMips > 0 )
            {
                for( s32 i=0; i<pTRef->NMips; i++ )
                if( (MinMipI<=(i+1)) && (MaxMipI>=(i+1)) )
                {
                    pixel_ref* pPRef = s_PixelRef + pTRef->MipRef[i];
                    if( pPRef->BlockBase == -1 )
                        Correct = FALSE;
                }
            }

            if( Correct )
                break;
            
            // Flush vram
            FlushBank( s_Bank[BI] );
        }
    }

    // Set GS registers to use this texture
    if( pTRef->NMips==0 )
    {
        VRAM_ActivateTexture( TextureRef );     
    }
    else
    {
        VRAM_ActivateTextureAndMips( TextureRef );     
    }
    
#ifdef X_DEBUG
    //VRAM_SanityCheck();
#endif
}

//==========================================================================

void vram_ActivateMips( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD, s32 BI )
{
    vram_ActivateMips( BMP.GetVRAMID()-1, MinLOD, MaxLOD, BI );
}

//==========================================================================

void vram_Activate( s32 VRAMID )
{

    vram_ActivateMips( VRAMID-1, 0, 100.0f, 0 );
}

//==========================================================================

void vram_Activate( const xbitmap& BMP )
{
    vram_ActivateMips( BMP, 0, 100.0f, 0 );
}

//==========================================================================

void vram_Activate( s32 VRAMID, s32 Bank )
{
    vram_ActivateMips( VRAMID-1, 0, 100.0f, Bank );
}

//==========================================================================

void vram_Activate( const xbitmap& BMP, s32 Bank )
{
    vram_ActivateMips( BMP, 0, 100.0f, Bank );
}

//==========================================================================

void vram_Activate( void )
{
}

//==========================================================================

void VRAM_Unload( const xbitmap& BMP )
{
    s32 TextureRef;
    texture_ref* pTRef;

    // Get reference to texture
    TextureRef = BMP.GetVRAMID()-1;
    ASSERT( (TextureRef>=0) && (TextureRef<s_NTextureRefsAllocated) );
    ASSERT( s_TextureRef[TextureRef].InUse == 1 );
    pTRef = s_TextureRef + TextureRef;

    // Unload pixel data
    if( s_PixelRef[pTRef->PixelRef].BlockBase != -1 )
        VRAM_UnloadPixelRef( pTRef->PixelRef );

    // Unload clut data
    if( (pTRef->ClutRef!=-1) && (s_ClutRef[pTRef->ClutRef].BlockBase != -1) )
        VRAM_UnloadClutRef( pTRef->ClutRef );

    // Unload mips
    for( s32 i=0; i<pTRef->NMips; i++ )
    if( s_PixelRef[pTRef->MipRef[i]].BlockBase != -1 )
        VRAM_UnloadPixelRef( pTRef->MipRef[i] );
}

//==========================================================================

void vram_Flush( s32 BankIndex )
{
    if( BankIndex == -1 )
    {
        for( s32 i=0; i<s_NBanks; i++ )
            FlushBank( s_Bank[i] );

        vram_Reset();

        return;
    }
    
    ASSERT( (BankIndex>=0) && (BankIndex<s_NBanks) );
    FlushBank( s_Bank[BankIndex] );
}

//==========================================================================

void vram_Flush( void )
{
    vram_Flush(-1);
}

//=========================================================================

void VRAM_PrintStats( void )
{
    //printf("NPixelRefs:%1d   NClutRefs:%1d   PixelBytes:%1d\n",s_NPixelRefsInVRAM,s_NClutRefsInVRAM,s_NBlocksUsedInVRAM*256);
/*    
    for( s32 i=0; i<VRAM_MAX_CLUT_REFS_IN_VRAM; i++ )
    if( s_ClutBlockRef[i] != -1 )
        printf("(%1d,%1d) ",i,s_ClutBlockRef[i]);
    printf("\n");
*/
}

//=========================================================================

s32  VRAM_GetNextID( s32 VRAMID )
{

    for( s32 i=0; i<s_NTextureRefsAllocated; i++ )
    {
        ASSERT( (VRAMID>=0) && (VRAMID<s_NTextureRefsAllocated) );
        VRAMID = (VRAMID+1)%s_NTextureRefsAllocated;

        if( s_TextureRef[VRAMID].InUse )
            return VRAMID;
    }

    return -1;
}

//==========================================================================

void vram_Unregister( s32 TextureRef )
{
    ASSERT( (TextureRef>=0) && (TextureRef<s_NTextureRefsAllocated) );
    ASSERT( s_TextureRef[TextureRef].InUse == 1 );

    texture_ref* pTRef = s_TextureRef + TextureRef;

    VRAM_UnloadPixelRef ( pTRef->PixelRef );
    VRAM_DeallocPixelRef( pTRef->PixelRef );

    if( pTRef->ClutRef != -1 )
    {
        VRAM_UnloadClutRef( pTRef->ClutRef );
        VRAM_DeallocClutRef( pTRef->ClutRef );
    }

    for( s32 i=0; i<pTRef->NMips; i++ )
    {
        VRAM_UnloadPixelRef ( pTRef->MipRef[i] );
        VRAM_DeallocPixelRef( pTRef->MipRef[i] );
    }

    VRAM_DeallocTextureRef( TextureRef );

    //DB(fs("VRAM: Del texture %1d\n",TextureRef));
}

//=========================================================================

void vram_Unregister( const xbitmap& BMP )
{
    vram_Unregister( BMP.GetVRAMID()-1 );
    BMP.SetVRAMID( 0 );
}

//=========================================================================

s32  vram_RegisterClut( byte* pClut, s32 NColors )
{
    s32 ClutRef;
    ClutRef = VRAM_SetupClutRef( pClut, NColors, -1 );
    return ClutRef;
}

//=========================================================================

void vram_UnRegisterClut( s32 Handle )
{
    VRAM_UnloadClutRef( Handle );
    VRAM_DeallocClutRef( Handle );
}

//=========================================================================

void vram_LoadClut( s32 ClutRef, s32 BI )
{
    ASSERT( (ClutRef>=0) && (ClutRef<s_NClutRefsAllocated) );
    ASSERT( s_ClutRef[ClutRef].InUse == 1 );

    // Upload clut if needed
    if( s_ClutRef[ClutRef].BlockBase == -1 )
    {
        VRAM_LoadClutRef( BI, ClutRef );
    }
    else
    {
        ASSERT( s_ClutRef[ClutRef].Bank == BI );
    }
}

//=========================================================================

void vram_ActivateClut( s32 ClutRef, s32 BI )
{
    struct clut_activate
    {
        dmatag      DMA;
        giftag      GIF;
        sceGsTex2   tex21;
        u64         tex21addr;
    };

    ASSERT( (ClutRef>=0) && (ClutRef<s_NClutRefsAllocated) );
    ASSERT( s_ClutRef[ClutRef].InUse == 1 );

    // Upload clut if needed
    if( s_ClutRef[ClutRef].BlockBase == -1 )
    {
        VRAM_LoadClutRef( BI, ClutRef );
    }
    else
    {
        ASSERT( s_ClutRef[ClutRef].Bank == BI );
    }

    // Activate the clut
    clut_activate* pCA = DLStruct(clut_activate);
    pCA->DMA.SetCont( sizeof(clut_activate)-sizeof(dmatag) );
    pCA->DMA.MakeDirect();
    pCA->GIF.BuildRegLoad( 1, TRUE );
    pCA->tex21addr  = 0x16 + eng_GetGSContext();
    pCA->tex21.PSM  = (s_ClutRef[ClutRef].NColors<=16)?(SCE_GS_PSMT4):(SCE_GS_PSMT8);
    pCA->tex21.CBP  = s_ClutRef[ClutRef].BlockBase;
    pCA->tex21.CPSM = SCE_GS_PSMCT32;
    pCA->tex21.CSM  = 0;
    pCA->tex21.CSA  = 0;
    pCA->tex21.CLD  = 1;
}

//=========================================================================

s32 vram_GetClutBaseAddr( s32 Handle )
{
    ASSERT( s_ClutRef[ Handle ].BlockBase >= 0  ); 
    return s_ClutRef[ Handle ].BlockBase;
}

//=========================================================================

s32  vram_GetClutHandle( const xbitmap& BMP )
{
    s32 TextureRef = BMP.GetVRAMID()-1;
    return s_TextureRef[TextureRef].ClutRef;
}

//=========================================================================

xbool vram_IsClutActive( s32 Handle )
{
    if ( s_ClutRef[ Handle ].BlockBase == -1)
        return FALSE ;

    return TRUE ;
}

//=========================================================================

void VRAM_Display( void )
{
    s32 i;
    s32 N;
    s32 MinAddr;
    s32 MaxAddr;

    x_DebugMsg("************************************\n");
    x_DebugMsg("*********** VRAM LAYOUT ************\n");
    x_DebugMsg("************************************\n");
    x_DebugMsg("NTextureRefs %1d\n",s_NTextureRefs);
    x_DebugMsg("NPixelRefs   %1d\n",s_NPixelRefs);
    x_DebugMsg("NClutRefs    %1d\n",s_NClutRefs);

    for( s32 B=0; B<s_NBanks; B++ )
    {
        bank* pB = s_Bank + B;

        x_DebugMsg("------------------------------------\n");
        x_DebugMsg("PIXEL %1d     Refs:%1d\n",B,pB->NPixelRefsInBank);

        if( s_PixelRef[pB->PixelBlockRefHead].NextRef != -1 )
        {
            N=0;
            MinAddr = 0x7FFFFFFF;
            MaxAddr = 0x00000000;
            i = s_PixelRef[pB->PixelBlockRefHead].NextRef;
            while( i != -1 )
            {
                x_DebugMsg("%2d] %3d (%3d,%3d) %3d %05X %1c\n",
                    N,i,
                    s_PixelRef[i].Width,s_PixelRef[i].Height,
                    s_PixelRef[i].NBlocks,
                    s_PixelRef[i].BlockBase,
                    ((((s_PixelRef[i].BlockBase)%32)==0)?('*'):(' ')));
                MinAddr = MIN(s_PixelRef[i].BlockBase,MinAddr);
                MaxAddr = MAX(s_PixelRef[i].BlockBase+s_PixelRef[i].NBlocks,MaxAddr);
                i = s_PixelRef[i].NextRef;
                N++;
            }

            x_DebugMsg("------------------------------------\n");
            x_DebugMsg("VRAM USED: %1d %1c%1.1f\n",(MaxAddr-MinAddr),'%',(100*(MaxAddr-MinAddr))/(f32)pB->NPixelBlocks);
        }

        x_DebugMsg("------------------------------------\n");
        x_DebugMsg("CLUT %1d    Refs:%1d\n",B,pB->NClutRefsInBank);

        {
            N=0;
            for( i=0; i<pB->NClutBlocks; i++ )
            if( pB->ClutBlockRef[i] != -1 )
            {
                x_DebugMsg("%2d] %3d %3d\n",N,pB->ClutBlockRef[i],i);
                N++;
            }
            if( N>0 )
            {
                x_DebugMsg("------------------------------------\n");
                x_DebugMsg("VRAM USED: %1d %1c%1.1f\n",N*4,'%',(100*N)/(f32)pB->NClutBlocks);
            }
        }
        x_DebugMsg("************************************\n");
    }
}

//=========================================================================

s32 vram_AllocateBank( s32 N8kPixelPages, s32 N8ClutPages )
{

    ASSERT(s_VRAMClean);
    ASSERT(s_NBanks<3);
    ASSERT(N8ClutPages>=0);
    ASSERT(N8kPixelPages>0);
    s32 StartAddr;

    // Get ending address of previous bank
    if( s_NBanks==1 )
        StartAddr = VRAM_PIXEL_BASE_ADDR;
    else
        StartAddr = s_Bank[s_NBanks-1].PixelEndAddr;

    // Confirm that we can fit memory requested
    s32 BlocksNeeded = N8kPixelPages*32 + N8ClutPages*32;
    ASSERT( (VRAM_PIXEL_END_ADDR-StartAddr) >= BlocksNeeded );

    // Setup a new bank
    bank* pB               = s_Bank + s_NBanks;
    pB->ClutBaseAddr       = StartAddr; 
    pB->NClutBlocks        = N8ClutPages*8;
    pB->ClutBlockRefCursor = -1;
    pB->PixelBaseAddr      = pB->ClutBaseAddr + (4*pB->NClutBlocks);
    pB->NPixelBlocks       = BlocksNeeded;
    pB->PixelEndAddr       = pB->PixelBaseAddr + pB->NPixelBlocks;
    pB->PixelBlockRefCursor= pB->PixelBlockRefHead;

    pixel_ref* pPRef = s_PixelRef + pB->PixelBlockRefHead;
    pPRef->BlockBase = pB->PixelBaseAddr;
    pPRef->NBlocks   = 0;
    pPRef->NextRef   = -1;
    pPRef->PrevRef   = -1;

    // Reset bank 0
    SetupBank0( pB->PixelEndAddr );

    s_NBanks++;

    return StartAddr ;
}

//=========================================================================

void vram_Sync( void )
{
    struct sync
    {
        dmatag  DMA;
        giftag  GIF;
        s64     FlushData;
        s64     FlushAddr;
    };
    sync* pS = DLStruct(sync);
    pS->DMA.SetCont(sizeof(sync)-sizeof(dmatag));
    pS->DMA.PAD[0] = SCE_VIF1_SET_FLUSHA(0);
    pS->DMA.MakeDirect();
    pS->GIF.BuildRegLoad(1,1);
    pS->FlushData = 0;
    pS->FlushAddr = 0x3f;
}

//=========================================================================

xbool vram_IsActive( const xbitmap& BMP )
{
    xbool Active = TRUE;
    s32 Ref = BMP.GetVRAMID()-1;
    texture_ref* pTRef = s_TextureRef + Ref;

    ASSERT( pTRef->InUse );

    if( s_PixelRef[pTRef->PixelRef].BlockBase == -1 )
        Active = FALSE;

    if( pTRef->ClutRef != -1 )
    {
        if( s_ClutRef[pTRef->ClutRef].BlockBase == -1 )
            Active = FALSE;
    }

    if( pTRef->NMips > 0 )
    {
        for( s32 i=0; i<pTRef->NMips; i++ )
        if( s_PixelRef[pTRef->MipRef[i]].BlockBase == -1 )
            Active = FALSE;
    }

    return Active;
}

//=========================================================================

xbool vram_IsActive( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD )
{
    xbool Active = TRUE;
    s32 Ref = BMP.GetVRAMID()-1;
    texture_ref* pTRef = s_TextureRef + Ref;

    ASSERT( pTRef->InUse );

    s32 MinMipI, MaxMipI;
    ComputeMipRange( Ref, MinLOD, MaxLOD, MinMipI, MaxMipI );

    if( MinMipI == 0 )
    {
        if( s_PixelRef[pTRef->PixelRef].BlockBase == -1 )
            Active = FALSE;
    }

    if( pTRef->ClutRef != -1 )
    {
        if( s_ClutRef[pTRef->ClutRef].BlockBase == -1 )
            Active = FALSE;
    }

    // Check mips if there are any
    if( pTRef->NMips > 0 )
    {
        for( s32 i=0; i<pTRef->NMips; i++ )
        if( (MinMipI<=(i+1)) && (MaxMipI>=(i+1)) )
        {
            if( s_PixelRef[pTRef->MipRef[i]].BlockBase == -1)
                Active = FALSE;
        }
    }

    return Active;
}
//=========================================================================

void VRAM_SanityCheck( void )
{
    s32 i;

    // Loop through all pixel refs 
    for( i=0; i<s_NPixelRefsAllocated; i++ )
    if( s_PixelRef[i].InUse )
    {
        ASSERT( (s_PixelRef[i].BlockBase == -1) || (s_PixelRef[i].BlockBase>=VRAM_PIXEL_BASE_ADDR) );
    }
}

//=========================================================================
inline 
f32 vram_Log2( f32 x )
{
    ieee32 F;
    F.v = x;
    return ( ((f32)F.s.exp) + s_LogTable[ (F.s.mant>>15)&0xFF ] );
}

//=========================================================================

f32  vram_ComputeMipK( f32 WorldPixelSize, const view& View )
{
    return( -vram_Log2( View.GetScreenDist() * WorldPixelSize ) );
    /*
    f32 XFOV,FSX,XC1;
    f32 ViewZ;

    XFOV = View.GetXFOV();
    FSX  = 1.0f / x_tan(XFOV*0.5f);
    XC1  = FSX * 512.0f * 0.5f;

    // Screen pixel size == XC1*WorldPixelSize/ViewZ
    // 1 = XC1*WPS/ViewZ
    // ViewZ = XC1*WPS

    ViewZ = XC1 * WorldPixelSize; 

    return( -vram_Log2(ViewZ) );
    */
}
//=========================================================================

void vram_ComputeMipRange( f32 MinZ, f32 MaxZ, f32 MipK, f32& MinLOD, f32& MaxLOD )
{
    if( MinZ < 0.01f ) MinZ = 0.01f;
    if( MaxZ < 0.01f ) MaxZ = 0.01f;

    MipK = (f32)((s32)(MipK*16)) / 16.0f;

    MinLOD = vram_Log2(MinZ) + MipK;
    MaxLOD = vram_Log2(MaxZ) + MipK;
}

//=========================================================================

void vram_ComputeMipRange   ( const bbox&   BBox, 
                              const view&   View,
                              f32           WorldPixelSize, 
                              f32&          MinLOD,
                              f32&          MaxLOD )
{
    f32 MinZ,MaxZ;

    // Get Z range
    View.GetMinMaxZ( BBox, MinZ, MaxZ );
    if( MinZ < 0.01f ) MinZ = 0.01f;
    if( MaxZ < 0.01f ) MaxZ = 0.01f;

    // Compute mipK
    f32 MipK = -vram_Log2( View.GetScreenDist() * WorldPixelSize );
    MipK = (f32)((s32)(MipK*16)) / 16.0f;

    // Compute LODs
    MinLOD = vram_Log2(MinZ) + MipK;
    MaxLOD = vram_Log2(MaxZ) + MipK;
}

//=========================================================================
/*
struct vram_registers
{
    sceGsTex0       Tex0;
    u64             Tex0Addr;
    sceGsTex1       Tex1;
    u64             Tex1Addr;
    sceGsMiptbp1    Mip1;
    u64             Mip1Addr;
    sceGsMiptbp2    Mip2;
    u64             Mip2Addr;
};
*/

void vram_GetRegisters( vram_registers& Reg, const xbitmap& BMP )
{
    s32 TextureRef = BMP.GetVRAMID()-1;
    s32 Context = eng_GetGSContext();

    texture_ref* pTRef = s_TextureRef + TextureRef;

    // Setup Tex0
    Reg.Tex0 = pTRef->tex01;
    Reg.Tex0.TBP0  = s_PixelRef[ pTRef->PixelRef ].BlockBase;
    if( pTRef->BPP <= 8 )
    {
        Reg.Tex0.CBP   = s_ClutRef[ pTRef->ClutRef ].BlockBase;
    }

    // Setup Tex1
    Reg.Tex1.MMAG  = 1;
    if (USE_BEST_MIPS)
        Reg.Tex1.MMIN  = MIP_MIN_INTERP_MIPMAP_BILINEAR ; // Blend between mips
    else
        Reg.Tex1.MMIN  = MIP_MIN_INTERP_MIPMAP_POINT ;    // Choose mip map
    Reg.Tex1.MTBA  = 0;
    Reg.Tex1.L     = 0;
    Reg.Tex1.LCM   = (s_MipManualMode[Context])?(1):(0);
    Reg.Tex1.MXL   = pTRef->NMips;
    //Reg.Tex1.K     = (s32)(MIPK*16.0f);;
    Reg.Tex1.K     = (s32)(s_MipK[Context]*16.0f);

    // Setup Mip registers
    Reg.Mip1 = pTRef->MBP1;
    Reg.Mip2 = pTRef->MBP2;
    if( pTRef->NMips >= 1 ) Reg.Mip1.TBP1  = s_PixelRef[ pTRef->MipRef[0] ].BlockBase;
    if( pTRef->NMips >= 2 ) Reg.Mip1.TBP2  = s_PixelRef[ pTRef->MipRef[1] ].BlockBase;
    if( pTRef->NMips >= 3 ) Reg.Mip1.TBP3  = s_PixelRef[ pTRef->MipRef[2] ].BlockBase;
    if( pTRef->NMips >= 4 ) Reg.Mip2.TBP4  = s_PixelRef[ pTRef->MipRef[3] ].BlockBase;
    if( pTRef->NMips >= 5 ) Reg.Mip2.TBP5  = s_PixelRef[ pTRef->MipRef[4] ].BlockBase;
    if( pTRef->NMips >= 6 ) Reg.Mip2.TBP6  = s_PixelRef[ pTRef->MipRef[5] ].BlockBase;

    // Setup addresses
    Reg.Tex0Addr = Context + 0x06;
    Reg.Tex1Addr = Context + 0x14;
    Reg.Mip1Addr = Context + 0x34;
    Reg.Mip2Addr = Context + 0x36;
}

//=========================================================================

s32 vram_GetUploadSize( const xbitmap& BMP, f32 MinLOD, f32 MaxLOD )
{
    s32 UploadSize = 0;
    s32 Ref = BMP.GetVRAMID()-1;
    texture_ref* pTRef = s_TextureRef + Ref;

    s32 MinMipI, MaxMipI;
    ComputeMipRange( Ref, MinLOD, MaxLOD, MinMipI, MaxMipI );

    if( MinMipI == 0 )
    {
        UploadSize += (BMP.GetWidth()*BMP.GetHeight()*BMP.GetBPP())/8;
    }

    if( pTRef->ClutRef != -1 )
    {
        UploadSize += 1024;
    }

    // Check mips if there are any
    if( pTRef->NMips > 0 )
    {
        for( s32 i=0; i<pTRef->NMips; i++ )
        if( (MinMipI<=(i+1)) && (MaxMipI>=(i+1)) )
        {
            UploadSize += (BMP.GetWidth(i+1)*BMP.GetHeight(i+1)*BMP.GetBPP())/8;
        }
    }

    return UploadSize;
}

//=========================================================================








