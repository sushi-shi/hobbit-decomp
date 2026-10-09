//==============================================================================
// INCLUDES
//==============================================================================

#include <Support/Shape/MCode/PS2_Shape.hpp>
#include "..\Shape.hpp"


//==============================================================================
// FUNCTIONS
//==============================================================================

void shape::Init( void )
{
    // Register microcode
    if( s_hMicroCode == -1 )
    {
        s32 Start = (s32)(&SHAPE_MICRO_CODE_START);
        s32 End   = (s32)(&SHAPE_MICRO_CODE_END);
        s_hMicroCode = eng_RegisterMicrocode("SHAPE", (void *)Start, End-Start );
    }

    // Flag that we are initialized
    s_Initialized = TRUE ;
}

//==============================================================================

void shape::Kill( void )
{
}

//==============================================================================

//==============================================================================
// Upon calling:
//
// PS2Material_Begin
// PS2Material_AddPass * TotalPasses
// PS2Material_End
//
// Dislay list memory will be setup as follows:
//
//  struct ps2_material                     - dma all info, transfers data to vu memory
//  struct ps2_material_pass * TotalPasses  - data
//  struct ps2_execute                      - executes vu material load
//
//==============================================================================


// Allocates material on display list and sets up basic params
ps2_material* PS2Material_Begin( const material_draw_info& DrawInfo, s32 PassCount )
{
    // Calculate amount to allocate
    s32 AllocSize = sizeof(ps2_material) + (sizeof(ps2_material_pass) * PassCount) + sizeof(ps2_material_execute) ;

    //==========================================================================
    // Setup material
    //==========================================================================

    // Allocate material
    ps2_material* pMaterial = (ps2_material*)DLAlloc(AllocSize) ;
    ASSERT(pMaterial) ;

    // Fill in transfer
    pMaterial->DMA.SetCont(AllocSize - sizeof(dmatag)) ;
    pMaterial->DMA.PAD[0] = SCE_VIF1_SET_FLUSHE(0) ;               // wait for vu to finish
    pMaterial->DMA.PAD[1] = SCE_VIF1_SET_STCYCL(4, 4, 0) ;
    pMaterial->PAD[0] = SCE_VIF1_SET_NOP(0) ;
    pMaterial->PAD[1] = SCE_VIF1_SET_NOP(0) ;
    pMaterial->PAD[2] = SCE_VIF1_SET_NOP(0) ;
    pMaterial->UNPACK     = VIF_Unpack(SHAPE_MATERIAL_ADDR,         // DestVUAddr          
                            MATERIAL_SIZE(PassCount),               // NVectors
                            VIF_V4_32,                              // Format
                            FALSE,                                  // Signed
                            FALSE,                                  // Masked
                            TRUE) ;                                 // AbsoluteAddress

    // Clear all flags
    pMaterial->Flags = 0 ;

    // Grab verts pre scale
    pMaterial->VertsPreScale = DrawInfo.VertsPreScale ;
    
    // Clip?
    if (DrawInfo.Flags & shape::DRAW_FLAG_CLIP)
    {
        pMaterial->Flags |= MATERIAL_FLAG_CLIP ;
        pMaterial->VertsPreTrans = 0 ;
    }
    else
    {
        // Used in combination with the VIFAdd to auto convert verts from s32 to f32
        pMaterial->VertsPreTrans = -1.5f * (1 << (23-0)) ; // where 0 = int frac bits
    }

    // Clear passes
    pMaterial->PassCount = 0 ;


    //==========================================================================
    // Setup vu execute structure in display list
    //==========================================================================
    
    // Calc execute struct address (at end of material + passes)
    ps2_material_execute* pExecute = (ps2_material_execute*)((u32)pMaterial + sizeof(ps2_material) + (sizeof(ps2_material_pass) * PassCount)) ;
    ASSERT(pExecute) ;

    // Setup ROW registers
    pExecute->STROW = SCE_VIF1_SET_STROW(0) ;
    
    // Setup VIF add mode to convert positions from s32 to f32 for free!
    if (pMaterial->Flags & MATERIAL_FLAG_CLIP)
    {
        // Clipping micro code cannot handle the floating point inaccuracy, so turn off
        pExecute->R0 = 0 ;
        pExecute->R1 = 0 ;
        pExecute->R2 = 0 ;
        pExecute->R3 = 0.0f ;               // To clear W component in color
    }
    else
    {
        u32 FracBits = 0 ;
        u32 VIFAdd   = ((((23-FracBits) + 0x7f)<<23) + 0x400000) ;

        pExecute->R0 = VIFAdd ;
        pExecute->R1 = VIFAdd ;
        pExecute->R2 = VIFAdd ;
        pExecute->R3 = 0.0f ;       // To clear W component in color
    }

    // Init micro code
    pExecute->MSCAL = SCE_VIF1_SET_MSCAL(SHAPE_MICRO_CODE_LOAD_MATERIAL, 0) ;

    // Align for dma
    pExecute->NOP[0] = SCE_VIF1_SET_NOP(0) ;
    pExecute->NOP[1] = SCE_VIF1_SET_NOP(0) ;

    return pMaterial ;
}

//==============================================================================

// Adds pass to material display list
void PS2Material_AddPass ( ps2_material* pMaterial, const ps2_pass_info& Info )
{
    // Calc next pass address (after material)
    ps2_material_pass* pPass = (ps2_material_pass*)( (u32)pMaterial + sizeof(ps2_material) + (sizeof(ps2_material_pass) * pMaterial->PassCount) ) ;
    ASSERT(pPass) ;
    ASSERT(sizeof(ps2_material_pass) == (MATERIAL_PASS_SIZE*16)) ;

    // Next pass
    pMaterial->PassCount++ ;
    ASSERT(pMaterial->PassCount <= MATERIAL_MAX_PASSES) ;

    // Set GS register set
    pPass->GSGifTag.BuildRegLoad( (sizeof(ps2_material_pass) / 16)-2, TRUE );   //-2 for giftags

    // Setup flush texture register
    pPass->TexFlush     = 0;
    pPass->TexFlushAddr = SCE_GS_TEXFLUSH ;

    // Activate texture?
    if (Info.BMP)
    {
        // Setup texture load
        vram_registers Reg ;
        vram_GetRegisters( Reg, *Info.BMP ) ;

        // Force use of mip level?
        if (shape::s_DrawSettings.Mips != -1)
        {
            Reg.Tex1.LCM = 1 ;
            Reg.Tex1.K   = shape::s_DrawSettings.Mips<<4 ;
        }

        // Setup tex0
        pPass->Tex0     = Reg.Tex0;
        pPass->Tex0Addr = SCE_GS_TEX0_1 + Info.Context ;

        // Override clut activation?
        if (Info.ClutHandle != -1)
        {
            ASSERT(vram_IsClutActive(Info.ClutHandle)) ;
            pPass->Tex0.CBP = vram_GetClutBaseAddr( Info.ClutHandle ) ;
        }

        // Setup tex1
        pPass->Tex1     = Reg.Tex1;
        pPass->Tex1Addr = SCE_GS_TEX1_1 + Info.Context ;

        // Setup mips
        pPass->Mip1     = Reg.Mip1;
        pPass->Mip1Addr = SCE_GS_MIPTBP1_1 + Info.Context ;

        pPass->Mip2     = Reg.Mip2;
        pPass->Mip2Addr = SCE_GS_MIPTBP2_1 + Info.Context ;
    }
    else
    {
        // Just clear registers for non-textured render

        // Setup tex0
        *(u64*)&pPass->Tex0 = 0 ;
        pPass->Tex0Addr     = SCE_GS_TEX0_1 + Info.Context ;

        // Setup tex1
        *(u64*)&pPass->Tex1 = 0 ;
        pPass->Tex1Addr     = SCE_GS_TEX1_1 + Info.Context ;

        // Setup mips
        *(u64*)&pPass->Mip1 = 0 ;
        pPass->Mip1Addr     = SCE_GS_MIPTBP1_1 + Info.Context ;

        *(u64*)&pPass->Mip2 = 0 ;
        pPass->Mip2Addr     = SCE_GS_MIPTBP2_1 + Info.Context ;
    }

    // Setup alpha register
    pPass->Alpha     = ((u64)Info.AlphaBlendMode) | (((u64)Info.FixedAlpha)<<32) ;
    pPass->AlphaAddr = SCE_GS_ALPHA_1 + Info.Context ;

    // Setup frame mask register
    ((u64*)&pPass->FrameMask)[0] = eng_GetFRAMEReg();
    pPass->FrameMask.FBMSK       = Info.FBMask;
    pPass->FrameMaskAddr         = SCE_GS_FRAME_1 + Info.Context;

    // Setup primitive gif tag
    pPass->PrimGifTag = Info.GifTag ;
}

//==============================================================================

