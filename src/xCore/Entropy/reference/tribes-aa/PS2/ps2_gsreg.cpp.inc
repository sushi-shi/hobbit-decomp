//=========================================================================
//
// ps2_gsreg.cpp
//
//=========================================================================

#include <xCore/Entropy/e_Engine.hpp>

//=========================================================================

struct gsreg
{
    u64 Data;
    u64 Addr;
};

struct gsreg_header
{
    dmatag  DMA;
    giftag  GIF;
};

static s32              s_NRegs=0;
static gsreg_header*    s_Header;
static xbool            s_Inside=FALSE;
static u64              s_RegData  PS2_ALIGNMENT(16) ;


//=========================================================================

void gsreg_Begin( void )
{
    ASSERT( !s_Inside );
    s_Inside = TRUE;
    s_Header = DLStruct(gsreg_header);
    s_NRegs=0;
}

//=========================================================================

void gsreg_End( void )
{
    ASSERT( s_Inside );
    s_Inside = FALSE;

    // Build the dma command and giftag
	s_Header->DMA.SetCont( sizeof(gsreg_header) - sizeof(dmatag) + (s_NRegs*16) );
	s_Header->DMA.MakeDirect();
    s_Header->GIF.BuildRegLoad(s_NRegs,TRUE);
}

//=========================================================================

void gsreg_Set( s32 Addr, u64 Data )
{
    gsreg* pReg;
    ASSERT(s_Inside);
    pReg = DLStruct(gsreg);
    pReg->Data = Data;
    pReg->Addr = (u64)Addr;
    s_NRegs++;
}

//=========================================================================

// Set mip equation
void gsreg_SetMipEquation( xbool Manual, f32 MipK, s32 NMips, s32 MagBlend, s32 MinBlend  )
{
    sceGsTex1* pTex1 = (sceGsTex1*)&s_RegData ;
    s_RegData = 0 ;

    ASSERT( (MagBlend>=0) && (MagBlend<=1) );
    ASSERT( (MinBlend>=0) && (MinBlend<=5) );

    pTex1->MMAG  = MagBlend;
    pTex1->MMIN  = MinBlend;
    pTex1->MTBA  = 0;
    pTex1->L     = 0;
    pTex1->LCM   = (Manual)?(1):(0);
    pTex1->MXL   = NMips;
    pTex1->K     = (s32)(MipK*16.0f);

    gsreg_Set( (SCE_GS_TEX1_1 + eng_GetGSContext()), s_RegData ) ;
}

//=========================================================================

void gsreg_SetAlphaBlend  ( s32 Mode, s32 FixedAlpha )
{
    // Check if we are turning blending off
    if( Mode==-1 )
    {
        Mode = ALPHA_BLEND_MODE(C_ZERO,C_ZERO,A_SRC,C_SRC);
    }

    s_RegData = ((u64)Mode) | (((u64)FixedAlpha)<<32) ;

    gsreg_Set( (SCE_GS_ALPHA_1+eng_GetGSContext()), s_RegData ) ;
}

//=========================================================================

void gsreg_SetFBMASK ( u32 Mask )
{
    sceGsFrame* pFrame = (sceGsFrame*)&s_RegData ;

    s_RegData = eng_GetFRAMEReg();
    pFrame->FBMSK = Mask;

    gsreg_Set( (SCE_GS_FRAME_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================
void FB_GetFrameBufferPositions( s32& FB0, s32& FB1, s32& ZB );

void gsreg_SetZBuffer( xbool On )
{
    sceGsTest* pTest = (sceGsTest*)&s_RegData;
    s_RegData = 0 ;

    pTest->ATE  = 0;
    pTest->ATST = 1;
    pTest->AREF = 0;
    pTest->AFAIL= 0;
    pTest->DATE = 0;
    pTest->DATM = 0;
    pTest->ZTE  = (On)?(1):(0);
    pTest->ZTST = (On)?(2):(1);
    gsreg_Set( (SCE_GS_TEST_1+eng_GetGSContext()), s_RegData );

    s32 FB0,FB1,ZB;
    sceGsZbuf* pZBuf = (sceGsZbuf*)&s_RegData ;
    s_RegData = 0 ;

    FB_GetFrameBufferPositions(FB0,FB1,ZB);
    pZBuf->ZBP  = ZB;
    pZBuf->PSM  = SCE_GS_PSMZ32;
    pZBuf->ZMSK = (On)?(0):(1);

    gsreg_Set( (SCE_GS_ZBUF_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetZBufferTest( s32 TestMethod )
{
    sceGsTest* pTest = (sceGsTest*)&s_RegData ;
    s_RegData = 0 ;

    pTest->ATE  = 0;
    pTest->ATST = 1;
    pTest->AREF = 0;
    pTest->AFAIL= 0;
    pTest->DATE = 0;
    pTest->DATM = 0;
    pTest->ZTE  = 1;
    pTest->ZTST = TestMethod;

    gsreg_Set( (SCE_GS_TEST_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetZBufferUpdate( xbool On )
{
    s32 FB0,FB1,ZB;
    sceGsZbuf* pZBuf = (sceGsZbuf*)&s_RegData;
    s_RegData = 0 ;

    FB_GetFrameBufferPositions(FB0,FB1,ZB);

    pZBuf->ZBP  = ZB;
    pZBuf->PSM  = SCE_GS_PSMZ32;
    pZBuf->ZMSK = (On)?(0):(1);

    gsreg_Set( (SCE_GS_ZBUF_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

// General purpose alpha, dest alpha, and z buffer test setting
void gsreg_SetAlphaAndZBufferTests ( xbool  AlphaTest,
                                     s32    AlphaTestMethod,
                                     u32    AlphaRef,
                                     s32    AlphaTestFail,
                                     xbool  DestAlphaTest,
                                     s32    DestAlphaTestMethod,
                                     xbool  ZBufferTest,
                                     s32    ZBufferTestMethod )
{
    sceGsTest* pTest = (sceGsTest*)&s_RegData ;
    s_RegData = 0 ;

    pTest->ATE   = AlphaTest ;
    pTest->ATST  = AlphaTestMethod ;
    pTest->AREF  = AlphaRef ;
    pTest->AFAIL = AlphaTestFail ;
    
    pTest->DATE  = DestAlphaTest ;
    pTest->DATM  = DestAlphaTestMethod ;

    pTest->ZTE   = ZBufferTest ;
    pTest->ZTST  = ZBufferTestMethod ;

    gsreg_Set( (SCE_GS_TEST_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetAlphaCorrection ( xbool On )
{
    // Set alpha OR correction value to 1
    sceGsFba* pFba = (sceGsFba*)&s_RegData ;
    s_RegData = 0 ;
    
    pFba->FBA = On ;
    
    gsreg_Set( (SCE_GS_FBA_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetClamping( xbool On )
{
    sceGsClamp* pClamp = (sceGsClamp*)&s_RegData;
    s_RegData = 0 ;

    pClamp->WMS = (On)?(1):(0);
    pClamp->WMT = (On)?(1):(0);

    gsreg_Set( (SCE_GS_CLAMP_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetClamping( xbool S, xbool T )
{
    sceGsClamp* pClamp = (sceGsClamp*)&s_RegData;
    s_RegData = 0 ;

    pClamp->WMS = (S)?(1):(0);
    pClamp->WMT = (T)?(1):(0);

    gsreg_Set( (SCE_GS_CLAMP_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

void gsreg_SetScissor( s32 X0, s32 Y0, s32 X1, s32 Y1 )
{
    sceGsScissor* pScissor = (sceGsScissor*)&s_RegData;
    s_RegData = 0 ;

    pScissor->SCAX0 = X0;
    pScissor->SCAX1 = X1-1;
    pScissor->SCAY0 = Y0;
    pScissor->SCAY1 = Y1-1;

    gsreg_Set( (SCE_GS_SCISSOR_1+eng_GetGSContext()), s_RegData );
}

//=========================================================================

// Set color and percentage
void gsreg_SetFogColor( u8 R, u8 G, u8 B, u8 A )
{
    // Set fog color
    gsreg_Set( SCE_GS_FOGCOL, SCE_GS_SET_FOGCOL(R,G,B) ) ;

    // Set fog percentage
    gsreg_Set( SCE_GS_FOG, SCE_GS_SET_FOG(A) ) ;
}

//=========================================================================



