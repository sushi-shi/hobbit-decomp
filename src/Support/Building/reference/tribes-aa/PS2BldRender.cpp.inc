
//=============================================================================
//  INCLUDES
//=============================================================================

#include "BldRender.hpp"
#include <xCore/Entropy/e_VRAM.hpp>
#include "mcode\memlayout.vu"

//=============================================================================
//  VARIABLES
//=============================================================================

volatile f32 PS2BUILDING_GLOBALK = 0.9f;

static s32 s_BuildingToggle =  0;
static s32 s_MCodeHandle    = -1;
static s32 s_CurrentMipCont =  0;
static s32 DListNum         =  0;
static s32 VU_JumpAdr[4]    =  { 4, 6, 14, 6 };

static vector3 s_LEP;
static matrix4 s_MAT;

static void DrawList( const building::dlist& DList, xbool clip );

//=============================================================================
//  EXTERNALS
//=============================================================================

extern u32 VUM_BUILDING_CODE_START __attribute__(( section( ".vudata" )));
extern u32 VUM_BUILDING_CODE_END   __attribute__(( section( ".vudata" )));

//=============================================================================
//  FUNCTIONS
//=============================================================================

void BLDRD_Initialize( void )
{
    //
    // Register microcode
    //
    
    if( s_MCodeHandle == -1 )
    {
        u32 Start = (u32)(&VUM_BUILDING_CODE_START);
        u32 End   = (u32)(&VUM_BUILDING_CODE_END);
        s_MCodeHandle = eng_RegisterMicrocode("BUILDING",&VUM_BUILDING_CODE_START,End-Start );
    }
}

//=========================================================================

void BLDRD_Begin( void )
{
    struct building_boot_dma
    {
        struct vifdata
        {
            giftag  DrawBaseTris;   // GIFTag for the base  triangles
            giftag  DrawLightTris;  // GIFTag for the light triangles
        };
        
        dmatag  DMA;                // DMA packet info
        u32     VIF_Boot[12];
        vifdata Data;
        u32     VIF_Run[4];
    };

    vram_Sync();
    
    //
    // Load the microcode
    //
    
    ASSERT( s_MCodeHandle != -1 );
    eng_ActivateMicrocode( s_MCodeHandle );

    // Reset the building toggle
    
    s_BuildingToggle = 0;

    //
    // Set the masks
    //
    
    gsreg_Begin();
    gsreg_SetFBMASK( 0 );
    gsreg_End();

    //
    // Set Blend equation
    //
    
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE( C_ZERO, C_ZERO, A_FIX, C_SRC ));
    gsreg_End();

    building_boot_dma* pPack = DLStruct( building_boot_dma );
    building_boot_dma::vifdata& Data = pPack->Data;

    //
    // Set the GIFTag for base texture rendering
    //
    
    Data.DrawBaseTris.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_TEXTURE );
    Data.DrawBaseTris.Reg( GIF_REG_ST, GIF_REG_NOP, GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    //
    // Set the GIFTag for lightmap texture rendering
    //
    
    Data.DrawLightTris.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA | GIF_FLAG_CONTEXT | GIF_FLAG_TEXTURE );
    Data.DrawLightTris.Reg( GIF_REG_NOP, GIF_REG_ST, GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    //
    // Set the Base and Offset registers
    //

    pPack->DMA.SetCont( sizeof( building_boot_dma ) - sizeof( dmatag ));
    pPack->DMA.PAD[0] = SCE_VIF1_SET_BASE  ( 0, 0 );
    pPack->DMA.PAD[1] = SCE_VIF1_SET_OFFSET( 0, 0 );

    //
    // Prepare the UVs for the unpacking
    //
    
    VIF_Mask( &pPack->VIF_Boot[0],  VIF_ASIS, VIF_ASIS, VIF_ROW, VIF_ROW,
                                    VIF_ASIS, VIF_ASIS, VIF_ROW, VIF_ROW,
                                    VIF_ASIS, VIF_ASIS, VIF_ROW, VIF_ROW,
                                    VIF_ASIS, VIF_ASIS, VIF_ROW, VIF_ROW );

    float temp = 1.0f;
    pPack->VIF_Boot[2] = SCE_VIF1_SET_STROW( 0 );
    pPack->VIF_Boot[3] = *((u32*)&temp);
    pPack->VIF_Boot[4] = pPack->VIF_Boot[3];
    pPack->VIF_Boot[5] = pPack->VIF_Boot[3];
    pPack->VIF_Boot[6] = pPack->VIF_Boot[3];
    pPack->VIF_Boot[7] = 0;

    //
    // Start VU and copy GIFTags to VU memory
    //
    
    pPack->VIF_Boot[8]  = SCE_VIF1_SET_MSCAL( 0, 0 );
    pPack->VIF_Boot[9]  = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Boot[10] = VIF_SkipWrite( 1, 0 );
    pPack->VIF_Boot[11] = VIF_Unpack( 0, sizeof( building_boot_dma::vifdata ) / 16, VIF_V4_32, FALSE, FALSE, FALSE );
    
    //
    // Tell the VU to run
    //
    
    pPack->VIF_Run[0] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[1] = SCE_VIF1_SET_MSCAL( 2, 0 );
    pPack->VIF_Run[2] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[3] = 0;

    //
    // Set the blending equation for the lightmap
    //

    #if !ONEPASS
    eng_PushGSContext( 1 );
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE( C_DST, C_ZERO, A_SRC, C_ZERO ));
    gsreg_SetAlphaAndZBufferTests( FALSE,                   // AlphaTest
                                   ALPHA_TEST_ALWAYS,       // AlphaTestMethod
                                   64,                      // AlphaRef
                                   ALPHA_TEST_FAIL_KEEP,    // AlphaTestFail
                                   TRUE,                    // DestAlphaTest
                                   DEST_ALPHA_TEST_0,       // DestAlphaTestMethod
                                   TRUE,                    // ZBufferTest
                                   ZBUFFER_TEST_GEQUAL ) ;  // ZBufferTestMethod
    gsreg_SetAlphaCorrection(TRUE) ;
    gsreg_End();
    eng_PopGSContext();
    #endif

    //
    // Set the blending equation for the base
    //
    
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE( C_ZERO, C_ZERO, A_FIX, C_SRC ));
    gsreg_End();

    //
    // Initialize the VRAM
    //
    
    if( g_BldDebug.FlushVRAM )
    {
        vram_Flush();

        //
        // Set the bank for the lightmaps (256*256*2) / 8192 = 17 round up
        //
    
        #if PALETTED
        vram_AllocateBank( 17, 1 );
        #else
        vram_AllocateBank( 16*4, 1 );
        #endif

        //
        // Set the bank for the fogmap
        //
        
        vram_AllocateBank(( 256 * 256 ) / 8192, 1 );
    }
}

//=========================================================================

void BLDRD_End( void )
{
    //
    // Reset the skip write and wait until we are done with the building
    //
    
    dmatag* pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[0] = SCE_VIF1_SET_FLUSHA( 0 );
    pDma->PAD[1] = VIF_SkipWrite( 1, 0 );

    //
    // Clear the frame mask register
    //
    
    eng_PushGSContext( 1 );
    gsreg_Begin();
    gsreg_SetFBMASK( 0 );
    gsreg_End();
    eng_PopGSContext();

    //
    // Set a standard method of rendering
    //
    
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE( C_ZERO, C_ZERO, A_FIX, C_SRC ));
    gsreg_End();
}

//=========================================================================

void BLDRD_Delay(void)
{
    dmatag* pDma;
    
    pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[0] = SCE_VIF1_SET_FLUSH( 0 );
}

void BLDRD_SetMark( s32 Num )
{
    dmatag* pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[1] = SCE_VIF1_SET_MARK( Num, 0 );
}

//=========================================================================

static void SetupDListPacket( void* pData, int nBytes, xbool NeedClipping )
{
    if( !g_BldDebug.LoadVerts )
        return;

    ASSERT( pData );
    ASSERT( nBytes > 0 );
    ASSERT( ALIGN_16(   nBytes   ) ==   nBytes   );
    ASSERT( ALIGN_16( (s32)pData ) == (s32)pData );

    //
    // Setup DMA to upload dlist data into VU memory
    //

    static u32 Buff[] = { 0, VU_DOUBLE_BUFFER_SIZE, VU_DOUBLE_BUFFER_SIZE * 2 };
    
    dmatag* pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[0] = SCE_VIF1_SET_BASE  ( Buff[ s_BuildingToggle ], 0 );
    pDma->PAD[1] = SCE_VIF1_SET_OFFSET( 0, 0 );

    pDma = DLStruct( dmatag ); 
    pDma->SetRef( nBytes, (u32)pData );
    
    pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[0] = SCE_VIF1_SET_FLUSH( 0 );

    //
    // Setup DMA to kick microcode to either clip or not clip
    //
    
    pDma = DLStruct( dmatag );
    pDma->SetCont( 0 );
    pDma->PAD[0] = SCE_VIF1_SET_MSCAL( VU_JumpAdr[ NeedClipping ], 0 );
    if( !g_BldDebug.DoubleBuffer )
    {
        pDma->PAD[1] = SCE_VIF1_SET_FLUSH( 0 );
    }

    if( !g_BldDebug.ExecVU )
        pDma->PAD[0] = 0;

    s_BuildingToggle++;
    if( s_BuildingToggle > 2 ) s_BuildingToggle = 0;
}

//=========================================================================

void BLDRD_RenderDList( const building::dlist& DList, xbool DoClip )
{
    //
    // Render block
    //
    
    if( g_BldDebug.UseVU )
    {
        SetupDListPacket( DList.pData, DList.SizeData, DoClip );
    }
    else
    {
        DrawList( DList, DoClip );
    }
}

//=========================================================================

void BLDRD_UpdaloadPassMask( u16 PassMask )
{
    BLDRD_Delay();
    
    struct set_mask
    {
        dmatag      DMA;            // DMA packet info
        u32         PassMask[4];
        u32         VIF_Run[4];     // Run microcode
    };

    set_mask* pPack = DLStruct( set_mask );

    u16 mask = PassMask;

    // Mask the pass bits
    
    PassMask &= (( g_BldDebug.BasePass & 1) << 0 ) | ( mask & 0x0E );
    PassMask &= (( g_BldDebug.LMapPass & 1) << 1 ) | ( mask & 0x0D );
    PassMask &= (( g_BldDebug.MonoPass & 1) << 2 ) | ( mask & 0x0B );
    PassMask &= (( g_BldDebug.FogPass  & 1) << 3 ) | ( mask & 0x07 );
    
    pPack->DMA.SetCont( sizeof( set_mask ) - sizeof( dmatag ));
    pPack->DMA.PAD[0] = VIF_SkipWrite( 1, 0 );
    pPack->DMA.PAD[1] = VIF_Unpack( 0, 4, VIF_S_32, FALSE, FALSE, FALSE );

    pPack->PassMask[0] = PassMask;

    pPack->VIF_Run[0] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[1] = SCE_VIF1_SET_MSCAL( 12, 0 );
    pPack->VIF_Run[2] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[3] = 0;
}

//=========================================================================

void BLDRD_UpdaloadMatrices( const matrix4& L2C, 
                             const matrix4& C2S, 
                             const matrix4& L2S,
                             const matrix4& L2W,
                             const vector3& WorldEyePos,
                             const vector3& LocalEyePos,
                             const vector4& FogMul,
                             const vector4& FogAdd )
{
    (void)LocalEyePos;

    BLDRD_Delay();

    if( !g_BldDebug.LoadMatrices )
        return;

    struct set_matrices
    {
        dmatag      DMA;            // DMA packet info
        matrix4     L2S;            // Local 2 screen martrix
        matrix4     L2C;            // Local 2 clip
        matrix4     C2S;            // Clip 2 screen    
        matrix4     FOG;            // FOG matrix
        vector4     FogMul;
        vector4     FogAdd;
        vector4     EyeLocal;
        vector4     EyeWorld;
        u32         VIF_Run[4];     // Run microcode
    };

    set_matrices* pPack = DLStruct( set_matrices );

    pPack->DMA.SetCont( sizeof( set_matrices ) - sizeof( dmatag ));
    pPack->DMA.PAD[0] = VIF_SkipWrite( 1, 0 );       
    pPack->DMA.PAD[1] = VIF_Unpack( 0, ( 4 * 4 ) + 4, VIF_V4_32, FALSE, FALSE, FALSE );

    pPack->L2S = L2S;
    pPack->L2C = L2C;
    pPack->C2S = C2S;
    pPack->FOG = L2W;
    pPack->FogMul = FogMul;
    pPack->FogAdd = FogAdd;
    pPack->EyeLocal.X = LocalEyePos.X;
    pPack->EyeLocal.Y = LocalEyePos.Y;
    pPack->EyeLocal.Z = LocalEyePos.Z;

    pPack->EyeWorld.X = WorldEyePos.X;
    pPack->EyeWorld.Y = WorldEyePos.Y;
    pPack->EyeWorld.Z = WorldEyePos.Z;

    pPack->VIF_Run[0] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[1] = SCE_VIF1_SET_MSCAL( 10, 0);
    pPack->VIF_Run[2] = SCE_VIF1_SET_FLUSH( 0 );
    pPack->VIF_Run[3] = 0;

    // Set these for the debug renderer

    s_LEP = LocalEyePos;
    s_MAT = L2W;
}

//=========================================================================

void BLDRD_UpdaloadBase( const xbitmap& Bitmap, f32 MinLOD, f32 MaxLOD )
{
    BLDRD_Delay();

    if( g_BldDebug.LoadBase )
    {
        vram_ActivateMips( Bitmap, MinLOD, MaxLOD, 0 );
    }

    struct base0_pass
    {
        giftag          VertGiftag;
        giftag          PassGiftag;
        u64             TexFlush;
        u64             TexFlushAddr;
        sceGsTex1       Tex1;
        u64             Tex1Addr;
        sceGsTex0       Tex0;
        u64             Tex0Addr;
        sceGsMiptbp1    Mip1;
        u64             Mip1Addr;
        sceGsMiptbp2    Mip2;
        u64             Mip2Addr;
        u64             Alpha;
        u64             AlphaAddr;
        sceGsFrame      FrameMask;
        u64             FrameMaskAddr;
        sceGsClamp      Clamp;
        u64             ClampAddr;
    };

    dmatag&     DMA  = *DLStruct( dmatag );
    base0_pass& Pass = *DLStruct( base0_pass );

    //
    // Setup DMA to upload pass data
    //
    
    DMA.SetCont( sizeof( base0_pass ));
    DMA.PAD[0] = VIF_SkipWrite( 1, 0 );
    DMA.PAD[1] = VIF_Unpack( VUMEM_BASE0_PASS, 
                             sizeof( base0_pass ) / 16,
                             VIF_V4_32,
                             FALSE, FALSE, TRUE );
    //
    // Setup vert GIFTtag
    //
    
    if( g_BldDebug.BaseTextured )
    {
        Pass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_TEXTURE );
        Pass.VertGiftag.Reg( GIF_REG_ST, GIF_REG_NOP, GIF_REG_RGBAQ, GIF_REG_XYZ2 );
    }
    else
    {
        Pass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE );
        Pass.VertGiftag.Reg( GIF_REG_ST, GIF_REG_NOP, GIF_REG_RGBAQ, GIF_REG_XYZ2  );
    }

    //
    // Setup pass data giftag
    //
    
    Pass.PassGiftag.BuildRegLoad(( sizeof( base0_pass ) / 16 ) - 2, TRUE );

    //
    // Get registers from vram manager
    //
    
    vram_registers Reg;
    vram_GetRegisters( Reg, Bitmap );

    //
    // Setup pass registers
    //

    Pass.TexFlush     = 0;
    Pass.TexFlushAddr = 0x3f;

    Pass.Tex0      = Reg.Tex0;
    Pass.Tex0.TFX  = 1;                  // Setup decal mode
    Pass.Tex0.TCC  = 1;
    Pass.Tex0Addr  = Reg.Tex0Addr;

    Pass.Tex1      = Reg.Tex1;
    Pass.Tex1Addr  = Reg.Tex1Addr;

    Pass.Mip1      = Reg.Mip1;
    Pass.Mip2      = Reg.Mip2;
    Pass.Mip1Addr  = Reg.Mip1Addr;
    Pass.Mip2Addr  = Reg.Mip2Addr;

    Pass.Alpha     = ALPHA_BLEND_MODE( C_ZERO, C_ZERO, A_FIX, C_SRC );
    Pass.AlphaAddr = ( SCE_GS_ALPHA_1 + eng_GetGSContext() );

    ((u64*)&Pass.FrameMask)[0] = eng_GetFRAMEReg();
    Pass.FrameMask.FBMSK       = 0;
    Pass.FrameMaskAddr         = ( SCE_GS_FRAME_1 + eng_GetGSContext() );

    Pass.Clamp.WMS = 0;
    Pass.Clamp.WMT = 0;
    Pass.ClampAddr = SCE_GS_CLAMP_1;

    //
    // Remember number of mips
    // 
    
    s_CurrentMipCont = Bitmap.GetNMips();
}

//=========================================================================

void UpdateFullColorLightMap( const xbitmap& Bitmap, s32* pClutHandle ) 
{
    #if !PALETTED
    (void)pClutHandle;
    #endif

    #if ONEPASS
    #define NUM_PASSES  1
    #else
    #define NUM_PASSES  3
    #endif
    
    struct lm_pass
    {
        struct pass
        {
            giftag          PassGiftag;
            sceGsTex0       Tex0;
            u64             Tex0Addr;
            
            #if !ONEPASS
            sceGsFrame      FrameMask;
            u64             FrameMaskAddr;
            #else
            u64             Alpha;
            u64             AlphaAddr;
            #endif
        };

        giftag          VertGiftag;
        pass            Pass[NUM_PASSES];
    };

    vram_registers  Reg;
    dmatag&         DMA     = *DLStruct( dmatag );
    lm_pass&        LMPass  = *((lm_pass*)DLAlloc( sizeof( lm_pass )));

    vram_GetRegisters( Reg, Bitmap );

    //
    // Setup DMA to upload pass data
    //
    
    DMA.SetCont( sizeof( lm_pass ));
    DMA.PAD[0] = VIF_SkipWrite( 1, 0 );
    DMA.PAD[1] = VIF_Unpack( VUMEM_LIGHTMAP_FULLCOLOR_PASS, 
                             sizeof( lm_pass ) / 16,
                             VIF_V4_32,
                             FALSE, FALSE, TRUE );
    //
    // Setup vert GIFTag
    //
    
    if( g_BldDebug.LMapTextured )
    {
        if( g_BldDebug.AlphaBlending )
        {
            // ORIGINAL!
            LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA | GIF_FLAG_CONTEXT | GIF_FLAG_TEXTURE );
        }
        else
        {
            LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_CONTEXT | GIF_FLAG_TEXTURE );
        }
    }
    else    
    {
        LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_CONTEXT );
    }
    
    LMPass.VertGiftag.Reg( GIF_REG_NOP, GIF_REG_ST, GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    //
    // Setup pass registers
    //

    for( s32 i=0; i<NUM_PASSES; i++ )
    {
        #if !ONEPASS
        static u32 Mask[] = { 0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF };
        #endif

        LMPass.Pass[i].PassGiftag.BuildRegLoad(( sizeof( lm_pass::pass ) / 16 ) - 1, TRUE );

        // Setup clut activation
        
        LMPass.Pass[i].Tex0       = Reg.Tex0;
        LMPass.Pass[i].Tex0Addr   = Reg.Tex0Addr;

        #if PALETTED
        if( g_BldDebug.LoadLightmap )
            LMPass.Pass[i].Tex0.CBP = vram_GetClutBaseAddr( pClutHandle[i] );
        #endif
        
        LMPass.Pass[i].Tex0.TFX   = 1;  // Setup decal mode
        LMPass.Pass[i].Tex0.TCC   = 1;  

        #if !ONEPASS
        
        // Setup the mask for each pass
        
        ((u64*)&LMPass.Pass[i].FrameMask)[0] = eng_GetFRAMEReg();
        LMPass.Pass[i].FrameMask.FBMSK       = Mask[i];
        LMPass.Pass[i].FrameMaskAddr         = ( SCE_GS_FRAME_1 + eng_GetGSContext() );
        
        #else   
        
        LMPass.Pass[i].Alpha     = ALPHA_BLEND_MODE( C_DST, C_SRC, A_SRC, C_ZERO );
        LMPass.Pass[i].AlphaAddr = SCE_GS_ALPHA_2;
        
        #endif
    }
}

//=========================================================================

void UpdateMonocromoLightMap( const xbitmap& Bitmap, s32* pClutHandle ) 
{
    #if !PALETTED
    (void)pClutHandle;
    #endif
    
    struct lm_pass
    {
        struct pass
        {
            giftag          PassGiftag;
            sceGsTex0       Tex0;
            u64             Tex0Addr;

            #if !ONEPASS
            sceGsFrame      FrameMask;
            u64             FrameMaskAddr;
            #else
            u64             Alpha;
            u64             AlphaAddr;
            #endif
        };

        giftag          VertGiftag;
        pass            Pass;
    };

    vram_registers  Reg;
    dmatag&         DMA     = *DLStruct( dmatag );
    lm_pass&        LMPass  = *((lm_pass*)DLAlloc( sizeof( lm_pass )));

    vram_GetRegisters( Reg, Bitmap );

    //
    // Setup DMA to upload pass data
    //
    
    DMA.SetCont( sizeof( lm_pass ));
    DMA.PAD[0] = VIF_SkipWrite( 1, 0 );
    DMA.PAD[1] = VIF_Unpack( VUMEM_LIGHTMAP_MONOCOLOR_PASS, 
                             sizeof( lm_pass ) / 16,
                             VIF_V4_32,
                             FALSE, FALSE, TRUE );

    //
    // Setup vert GIFTag
    //
    
    if( g_BldDebug.LMapTextured )
    {
        if( g_BldDebug.AlphaBlending )
        {
            LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA | GIF_FLAG_CONTEXT | GIF_FLAG_TEXTURE );
        }
        else
        {
            LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_CONTEXT | GIF_FLAG_TEXTURE );
        }
    }
    else
    {
        LMPass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA | GIF_FLAG_CONTEXT );
    }
    LMPass.VertGiftag.Reg( GIF_REG_NOP, GIF_REG_ST, GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    //
    // Setup pass registers
    //
    
    LMPass.Pass.PassGiftag.BuildRegLoad(( sizeof( lm_pass::pass ) / 16 ) - 1, TRUE );

    LMPass.Pass.Tex0       = Reg.Tex0;
    LMPass.Pass.Tex0Addr   = Reg.Tex0Addr;
    #if PALETTED
    if( g_BldDebug.LoadLightmap )
        LMPass.Pass.Tex0.CBP = vram_GetClutBaseAddr( pClutHandle[0] );
    #endif
    LMPass.Pass.Tex0.TFX   = 1;  // Setup decal mode
    LMPass.Pass.Tex0.TCC   = 1;  


    #if !ONEPASS
    
    // Setup the mask for each pass
    
    ((u64*)&LMPass.Pass.FrameMask)[0]   = eng_GetFRAMEReg();
    LMPass.Pass.FrameMask.FBMSK         = 0;
    LMPass.Pass.FrameMaskAddr           = (SCE_GS_FRAME_1+eng_GetGSContext());
    #else
    LMPass.Pass.Alpha     = ALPHA_BLEND_MODE(C_DST, C_ZERO, A_SRC, C_ZERO);
    LMPass.Pass.AlphaAddr = SCE_GS_ALPHA_2;
    #endif
}

//=========================================================================

void BLDRD_UpdaloadLightMap( const xbitmap& Bitmap, s32* pClutHandle )
{
    BLDRD_Delay();
    
    ASSERT( pClutHandle );

    #if !ONEPASS
    ASSERT( pClutHandle[0] != -1 );
    ASSERT( pClutHandle[1] != -1 );
    ASSERT( pClutHandle[2] != -1 );
    #endif

    //
    // Activate the lightmap
    //
    
    eng_PushGSContext( 1 );
    {
        if( g_BldDebug.LoadLightmap )
        {
            vram_Activate( Bitmap, 1 );
        
            #if PALETTED
                ASSERT( pClutHandle[0] == vram_GetClutHandle( Bitmap ));
                #if !ONEPASS
                vram_LoadClut( pClutHandle[1], 1 );
                vram_LoadClut( pClutHandle[2], 1 );
                #endif
            #endif
        }
        
        // Get registers from vram manager
        
        UpdateMonocromoLightMap( Bitmap, pClutHandle );
        UpdateFullColorLightMap( Bitmap, pClutHandle );            
    }

    //
    // Turn bilinear filtering on/off for debugging
    //
    
    if( !g_BldDebug.Bilinear )
    {
        gsreg_Begin();
        gsreg_SetMipEquation( FALSE, 1.0f, 7, MIP_MAG_POINT, MIP_MIN_POINT );
        gsreg_End();
    }
    
    eng_PopGSContext();
}

//=========================================================================

void BLDRD_UpdaloadFog( const xbitmap& Bitmap )
{
    // Activate fog texture into bank 2
    
    vram_Activate( Bitmap, 2 );

    struct fog_pass
    {
        giftag          VertGiftag;
        giftag          PassGiftag;
        u64             TexFlush;
        u64             TexFlushAddr;
        sceGsTex1       Tex1;
        u64             Tex1Addr;
        sceGsTex0       Tex0;
        u64             Tex0Addr;
        u64             Alpha;
        u64             AlphaAddr;
        sceGsFrame      FrameMask;
        u64             FrameMaskAddr;
        sceGsClamp      Clamp;
        u64             ClampAddr;
    };

    dmatag&     DMA  = *DLStruct( dmatag );
    fog_pass&   Pass = *DLStruct( fog_pass );

    //
    // Setup DMA to upload pass data
    //
    
    DMA.SetCont( sizeof( fog_pass ));
    DMA.PAD[0] = VIF_SkipWrite( 1, 0 );
    DMA.PAD[1] = VIF_Unpack( VUMEM_FOG_PASS, 
                             sizeof( fog_pass ) / 16,
                             VIF_V4_32,
                             FALSE, FALSE, TRUE );
    
    //
    // Setup vert GIFTag
    //
    
    if( g_BldDebug.FogTextured )
    {
        Pass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA | GIF_FLAG_TEXTURE );
    }
    else
    {
        Pass.VertGiftag.Build2( 4, 0, GIF_PRIM_TRIANGLESTRIP, GIF_FLAG_SMOOTHSHADE | GIF_FLAG_ALPHA );
    }
    Pass.VertGiftag.Reg( GIF_REG_ST, GIF_REG_NOP, GIF_REG_RGBAQ, GIF_REG_XYZ2 );

    // Setup pass data GIFTag
    
    Pass.PassGiftag.BuildRegLoad(( sizeof( fog_pass ) / 16 ) - 2, TRUE );

    //
    // Get registers from vram manager
    //
    
    vram_registers Reg;
    vram_GetRegisters( Reg, Bitmap );

    //
    // Setup pass registers
    //

    Pass.TexFlush     = 0;
    Pass.TexFlushAddr = 0x3f;

    Pass.Tex0      = Reg.Tex0;
    Pass.Tex0.TFX  = 1;                 // Setup decal mode
    Pass.Tex0.TCC  = 1;
    Pass.Tex0Addr  = Reg.Tex0Addr;

    Pass.Tex1      = Reg.Tex1;
    Pass.Tex1Addr  = Reg.Tex1Addr;

    Pass.Alpha     = ALPHA_BLEND_MODE( C_SRC, C_DST, A_SRC, C_DST );
    Pass.AlphaAddr = SCE_GS_ALPHA_1;

    ((u64*)&Pass.FrameMask)[0] = eng_GetFRAMEReg();
    Pass.FrameMask.FBMSK       = 0;
    Pass.FrameMaskAddr         = SCE_GS_FRAME_1;

    Pass.Clamp.WMS = 1;
    Pass.Clamp.WMT = 1;
    Pass.ClampAddr = SCE_GS_CLAMP_1;
}

//=========================================================================
//  Debug Rendering (and overdraw renderer)
//=========================================================================

static void DrawList( const building::dlist& DList, xbool clip )
{
    int numv, ngons, nvert, count, i, j;
    vector4 *pVertex, fifo[3];
    vector3 normal, vec;
    f32 dot;
    (void)clip;

    if( g_BldDebug.ShowOverdraw )
    {
        eng_SetBackColor( xcolor( 0, 0, 0 ));
        
        draw_Begin( DRAW_TRIANGLES, DRAW_USE_ALPHA );
        gsreg_Begin();
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE( C_SRC, C_ZERO, A_FIX, C_DST ), 128 );
        gsreg_SetZBuffer( FALSE );
        gsreg_End();
    }
    else
    {
        draw_Begin( DRAW_TRIANGLES, 0);
    }

    draw_SetL2W( s_MAT );
    
    pVertex = (vector4*)DList.pData;

    nvert = *(int*)&pVertex[0].Z;
    ngons = *(int*)&pVertex[0].W;
    
    pVertex += 2;
    DListNum++;
    
    numv = DList.nVertices;
    ASSERT( numv == nvert );
    
    for( i=0; i<ngons; i++ )
    {
        ASSERT(  *((u32*)&pVertex[0].W ) & ( 1 << 15 ));
        ASSERT(  *((u32*)&pVertex[1].W ) & ( 1 << 15 ));
        
        count = ((*(u32*)&pVertex[0].W ) & 0x3F ) + 1;

        ASSERT( count >= 3 );
        
        normal.X = pVertex[0].W;
        normal.Y = pVertex[1].W;
        normal.Z = pVertex[2].W;

        vec.X = s_LEP.X - pVertex[0].X;
        vec.Y = s_LEP.Y - pVertex[0].Y;
        vec.Z = s_LEP.Z - pVertex[0].Z;
        
        dot = vec.Dot( normal );

        if( g_BldDebug.ShowOverdraw )
        {
            draw_Color( xcolor( 16, 16, 16 ));
        }
        else
        {
            if( dot < 0.0f )
            {
                draw_Color( xcolor( DListNum * 1000, 0, 0, 128 ));
            }
            else
            {
                draw_Color( xcolor( 0, DListNum * 1000, 0, 128 ));
            }
        }

        //
        // Render strips
        //
        
        for( j=0; j<count; j++ )
        {
            fifo[2] = fifo[1];
            fifo[1] = fifo[0];
            fifo[0] = pVertex[0];

            if(!(*((u32*)&pVertex[0].W ) & ( 1 << 15 )))
            {
                draw_Vertex(fifo[0].X, fifo[0].Y, fifo[0].Z);
                draw_Vertex(fifo[1].X, fifo[1].Y, fifo[1].Z);
                draw_Vertex(fifo[2].X, fifo[2].Y, fifo[2].Z);
            }
                
            pVertex++;
        }
    }       

    draw_End();
    
    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_OFF );
    gsreg_SetZBuffer( TRUE );
    gsreg_End();
}

//=========================================================================

