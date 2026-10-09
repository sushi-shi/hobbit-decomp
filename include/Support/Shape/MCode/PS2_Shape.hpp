#ifndef __PS2_SHAPE_HPP__
#define __PS2_SHAPE_HPP__



#ifdef __cplusplus

//==============================================================================
// Includes
//==============================================================================
#include <xCore/Entropy/Entropy.hpp>


//==============================================================================
// Forward declarations
//==============================================================================
struct material_draw_info ;

#endif


//==============================================================================
// MICRO CODE ADDRESSED
//==============================================================================


// Available micro code
#if defined(__cplusplus) && defined(TARGET_PS2)

extern u32 SHAPE_MICRO_CODE_START   __attribute__((section(".vudata")));
extern u32 SHAPE_MICRO_CODE_END     __attribute__((section(".vudata")));

#endif

//==============================================================================
// MICRO CODE ADDRESSES (these are the addresses of the jump table)
//==============================================================================
#define SHAPE_MICRO_CODE_INIT                           0x00                                       
#define SHAPE_MICRO_CODE_LOAD_MATERIAL                  0x02                     
#define SHAPE_MICRO_CODE_LOAD_NODE                      0x04               
#define SHAPE_MICRO_CODE_DRAW                           0x06               

#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_X       0x08
#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_Y       0x0A
#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_POS_Z       0x0C

#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_X       0x0E
#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_Y       0x10
#define SHAPE_MICRO_CODE_TEX_DIF_CLIP_PLANE_NEG_Z       0x12

#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_X   0x14
#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_Y   0x16
#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_POS_Z   0x18

#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_X   0x1A
#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_Y   0x1C
#define SHAPE_MICRO_CODE_TEX_DIF_REF_CLIP_PLANE_NEG_Z   0x1E


//==============================================================================
// CLIP FLAGS
//==============================================================================
#define CLIP_POS_X_FLAG       (1<<0)
#define CLIP_NEG_X_FLAG       (1<<1)
#define CLIP_POS_Y_FLAG       (1<<2)
#define CLIP_NEG_Y_FLAG       (1<<3)
#define CLIP_POS_Z_FLAG       (1<<4)
#define CLIP_NEG_Z_FLAG       (1<<5)


//==============================================================================
// VU1 MEMORY STRUCTURE EQUATES
//==============================================================================

//==============================================================================
// MATRICES
//==============================================================================
#define MATRIX4_ROW0                    0
#define MATRIX4_ROW1                    1
#define MATRIX4_ROW2                    2
#define MATRIX4_ROW3                    3
#define MATRIX4_SIZE                    4
                                        
#define MATRIX3_ROW0                    0
#define MATRIX3_ROW1                    1
#define MATRIX3_ROW2                    2
#define MATRIX3_SIZE                    3


//==============================================================================
// VERTS
//==============================================================================

#define VERT_UV                         0
#define VERT_REF_UV                     1
#define VERT_NORM                       2
#define VERT_POS                        3
#define VERT_SIZE                       4


//==============================================================================
// LIGHT
//==============================================================================
                                        
#define LIGHT_DIR                       0
#define LIGHT_COL                       1
#define LIGHT_AMB                       2
#define LIGHT_SIZE                      3


//==============================================================================
// MATERIAL_PASS
//==============================================================================

#define MATERIAL_PASS_SETUP             0   // GS registers info
#define MATERIAL_PASS_GIF_TAG           8   // Gif tag for rendering verts
#define MATERIAL_PASS_SIZE              9   // 7 GS registers + 2 gif tags

#define MATERIAL_MAX_PASSES             5   // Diffuse, Lum, Alpha, Reflect, Detail

//==============================================================================
// MATERIAL
//==============================================================================

#define MATERIAL_INFO                               0                   // Flags, VertsPreScale, VertsPreTrans, PassCount 
#define MATERIAL_COL                                (MATERIAL_INFO   + 1)            
#define MATERIAL_PASSES                             (MATERIAL_COL    + 1)
#define MATERIAL_SIZE(__passes__)                   (MATERIAL_PASSES + ((__passes__) * MATERIAL_PASS_SIZE))

//==============================================================================
// MATERIAL_ FLAGS
//==============================================================================

#define MATERIAL_FLAG_CLIP                          (1<<0)
#define MATERIAL_FLAG_SELF_ILLUM                    (1<<1)
#define MATERIAL_FLAG_CALC_REFLECT_UVS              (1<<2)
#define MATERIAL_FLAG_REFLECT_IN_WORLD_SPACE        (1<<3)
#define MATERIAL_FLAG_REFLECT_RANDOM                (1<<4)


//==============================================================================
// NODE
//==============================================================================

#define NODE_L2W_MATRIX                             0
#define NODE_LIGHT                                  (NODE_L2W_MATRIX + MATRIX4_SIZE)
#define NODE_SIZE                                   (NODE_LIGHT + LIGHT_SIZE)



//==============================================================================
// VU1 MEMORY LAYOUT
//==============================================================================

#define SHAPE_CLIPPER_PLANE_TABLE_SIZE              7                    // (ClipMask, JumpAddr, 0 |0 for each plane)
#define SHAPE_CLIPPER_BUFFER_SIZE                   (9 * VERT_SIZE)      // (3 verts through 6 planes, max 9)
#define SHAPE_CLIPPER_STRIP_BUFFER_SIZE             ((12 * VERT_SIZE)+1) // (9 fan verts = 12 strip verts, + gif tag = 12)

#define SHAPE_REGISTER_STORAGE_SIZE                 15


#define SHAPE_SYNC_GIF_ADDR                         0
#define SHAPE_W2V_ADDR                              1
#define SHAPE_W2C_ADDR                              (SHAPE_W2V_ADDR + MATRIX4_SIZE)
#define SHAPE_C2S_ADDR                              (SHAPE_W2C_ADDR + MATRIX4_SIZE)
#define SHAPE_W2S_ADDR                              (SHAPE_C2S_ADDR + MATRIX4_SIZE)
#define SHAPE_PARAMS_ADDR                           (SHAPE_W2S_ADDR + MATRIX4_SIZE)
#define SHAPE_MATERIAL_ADDR                         (SHAPE_PARAMS_ADDR + 1)
#define SHAPE_REGISTER_STORAGE                      (SHAPE_MATERIAL_ADDR + MATERIAL_SIZE(MATERIAL_MAX_PASSES))
#define SHAPE_CLIPPER_PLANE_TABLE                   (SHAPE_REGISTER_STORAGE + SHAPE_REGISTER_STORAGE_SIZE)
#define SHAPE_CLIPPER_BUFFER0                       (SHAPE_CLIPPER_PLANE_TABLE + SHAPE_CLIPPER_PLANE_TABLE_SIZE)
#define SHAPE_CLIPPER_BUFFER1                       (SHAPE_CLIPPER_BUFFER0 + SHAPE_CLIPPER_BUFFER_SIZE)
#define SHAPE_CLIPPER_STRIP_BUFFER0                 (SHAPE_CLIPPER_BUFFER1 + SHAPE_CLIPPER_BUFFER_SIZE)
#define SHAPE_CLIPPER_STRIP_BUFFER1                 (SHAPE_CLIPPER_STRIP_BUFFER0 + SHAPE_CLIPPER_STRIP_BUFFER_SIZE)
#define SHAPE_NODE_ADDR                             (SHAPE_CLIPPER_STRIP_BUFFER1 + SHAPE_CLIPPER_STRIP_BUFFER_SIZE)
#define SHAPE_BASE_ADDR(__nodes__)                  (SHAPE_NODE_ADDR + (NODE_SIZE * __nodes__))
#define SHAPE_BUFFER_SIZE(__base__, __buffers__)    ((1024 - __base__)/__buffers__)

// Relative to tops address for buffering...
#define SHAPE_GIF_ADDR                0
#define SHAPE_VERTS_ADDR              (SHAPE_GIF_ADDR + 1)


//==============================================================================
// C++ STRUCTURES
//==============================================================================

#ifdef __cplusplus

// Vert position format
struct ps2_pos
{
    s16 x,y,z,adc ;
} ;

// Vert texture co-ord format
struct ps2_tex_coord
{
    s16 u,v ;
} ;

// Vert normal format
struct ps2_norm
{
    s8  x,y,z ;
} ;


#ifdef TARGET_PS2

// Info used to setup a pass
struct ps2_pass_info
{
    // Bitmap
    const xbitmap*  BMP ; 
    s32             ClutHandle ;

    // Context switching
    s32             Context ;

    // Alpha blending
    s32             AlphaBlendMode ;
    u32             FixedAlpha ;

    // Frame buffer masking
    u32             FBMask ;

    // Primitive gif tag
    giftag          GifTag PS2_ALIGNMENT(16);
} ;


// A material pass
struct ps2_material_pass
{
    // Members
    giftag          GSGifTag;       // QWord1 - GS load gif tag

    u64             TexFlush;       // QWord2 - GS Regload1
    u64             TexFlushAddr;
    
    sceGsTex1       Tex1;           // QWord3 - GS Regload2
    u64             Tex1Addr;
    
    sceGsTex0       Tex0;           // QWord4 - GS Regload3
    u64             Tex0Addr;
    
    sceGsMiptbp1    Mip1;           // QWord5 - GS Regload4
    u64             Mip1Addr;
    
    sceGsMiptbp2    Mip2;           // QWord6 - GS Regload5
    u64             Mip2Addr;
    
    u64             Alpha;          // QWord7 - GS Regload6
    u64             AlphaAddr;
    
    sceGsFrame      FrameMask;      // QWord8 - GS Regload7
    u64             FrameMaskAddr ;

    giftag          PrimGifTag ;    // QWord10 - Primitive draw gif tag
} ;

// Material execute data
struct ps2_material_execute
{
    // VU execution and data transfer setup
    s32                 MSCAL ;
    s32                 STROW ;
    s32                 R0 ;
    s32                 R1 ;
    s32                 R2 ;
    f32                 R3 ;
    s32                 NOP[2] ;
} ;


// Material pass
struct ps2_material
{
    // Dma pack
    dmatag              DMA ;                       // DMA header
    s32                 PAD[3] ;                    // Pad before unpack
    s32                 UNPACK ;                    // Transfer material data cmnd

    // Material data
    s32                 Flags ;                     // Flags
    f32                 VertsPreScale ;             // Scales verts
    f32                 VertsPreTrans ;             // Used with VIF for int->float
    s32                 PassCount ;                 // # of passes
    f32                 Col[4] ;                    // Diffuse color
} ;

// Allocates material on display list
ps2_material* PS2Material_Begin( const material_draw_info& DrawInfo, s32 PassCount ) ;

// Adds pass to material
void PS2Material_AddPass ( ps2_material* pMaterial, const ps2_pass_info& Info ) ;

// End material in display list (just verifies that all is good!)
inline void PS2Material_End( ps2_material* pMaterial, s32 PassCount )
{
    (void)pMaterial ;
    (void)PassCount ;
    ASSERT(pMaterial->PassCount <= MATERIAL_MAX_PASSES) ;
    ASSERT(pMaterial->PassCount == PassCount) ;
}
   
//==============================================================================


#endif  //#ifdef TARGET_PS2


#endif  //#ifdef __cplusplus




#endif  // #define __PS2_SHAPE_HPP__

