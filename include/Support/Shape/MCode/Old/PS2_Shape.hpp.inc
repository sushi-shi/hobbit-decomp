#ifndef __PS2_SHAPE_HPP__
#define __PS2_SHAPE_HPP__


//==============================================================================
// C++ STRUCS
//==============================================================================

#ifdef __cplusplus

#endif


//==============================================================================
// MICRO CODE ADDRESSED
//==============================================================================


// Available micro code
#if defined(__cplusplus) && defined(TARGET_PS2)

extern u32 SHAPE_MICRO_CODE_START                             __attribute__((section(".vudata")));
extern u32 SHAPE_MICRO_CODE_END                               __attribute__((section(".vudata")));

#endif

//==============================================================================
// MICRO CODE ADDRESSES (these are the addresses of the jump table)
//==============================================================================
#define SHAPE_MICRO_CODE_INIT               0x00                                       
#define SHAPE_MICRO_CODE_LOAD_MATERIAL      0x02                     
#define SHAPE_MICRO_CODE_DIF_LOAD_NODE      0x04               
#define SHAPE_MICRO_CODE_DIF_REF_LOAD_NODE  0x06               
#define SHAPE_MICRO_CODE_TEX_DIF_DRAW       0x08               
#define SHAPE_MICRO_CODE_TEX_DIF_REF_DRAW   0x0a               
#define SHAPE_MICRO_CODE_DIF_DRAW           0x0c               
#define SHAPE_MICRO_CODE_DIF_REF_DRAW       0x0e               






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
                                        
#define DIF_VERT_NORM                   0
#define DIF_VERT_POS                    1
#define DIF_VERT_SIZE                   2
                                        
#define DIF_REF_VERT_REF_UV             0
#define DIF_REF_VERT_NORM               1
#define DIF_REF_VERT_POS                2
#define DIF_REF_VERT_SIZE               3
                                        
#define TEX_DIF_VERT_UV                 0
#define TEX_DIF_VERT_NORM               1
#define TEX_DIF_VERT_POS                2
#define TEX_DIF_VERT_SIZE               3
                                        
#define TEX_DIF_REF_VERT_UV             0
#define TEX_DIF_REF_VERT_REF_UV         1
#define TEX_DIF_REF_VERT_NORM           2
#define TEX_DIF_REF_VERT_POS            3
#define TEX_DIF_REF_VERT_SIZE           4

//==============================================================================
// LIGHT
//==============================================================================
                                        
#define LIGHT_DIR                       0
#define LIGHT_COL                       1
#define LIGHT_AMB                       2
#define LIGHT_SIZE                      3

//==============================================================================
// MATERIAL
//==============================================================================

#define MATERIAL_PASS_SIZE              8

#define MATERIAL_FLAGS                  0
#define MATERIAL_COL                    (MATERIAL_FLAGS + 1)
#define MATERIAL_GIF_TAG_ALPHA_PASS     (MATERIAL_COL + 1)
#define MATERIAL_GIF_TAG_ALPHA_PRIM     (MATERIAL_GIF_TAG_ALPHA_PASS + MATERIAL_PASS_SIZE)
#define MATERIAL_GIF_TAG_DIFFUSE_PASS   (MATERIAL_GIF_TAG_ALPHA_PRIM + 1)
#define MATERIAL_GIF_TAG_DIFFUSE_PRIM   (MATERIAL_GIF_TAG_DIFFUSE_PASS + MATERIAL_PASS_SIZE)
#define MATERIAL_GIF_TAG_DETAIL_PASS    (MATERIAL_GIF_TAG_DIFFUSE_PRIM + 1)
#define MATERIAL_GIF_TAG_DETAIL_PRIM    (MATERIAL_GIF_TAG_DETAIL_PASS + MATERIAL_PASS_SIZE)
#define MATERIAL_GIF_TAG_REFLECT_PRIM   (MATERIAL_GIF_TAG_DETAIL_PRIM + 1)

#define MATERIAL_SIZE                   (MATERIAL_GIF_TAG_REFLECT_PRIM + 1)

//==============================================================================
// MATERIAL_ FLAGS
//==============================================================================

#define MATERIAL_FLAG_CLIP              (1<<0)
#define MATERIAL_FLAG_SELF_ILLUM        (1<<1)
#define MATERIAL_FLAG_ALPHA_TEX_SETUP   (1<<2)
#define MATERIAL_FLAG_DIFFUSE_TEX_SETUP (1<<3)
#define MATERIAL_FLAG_REFLECT_DRAW      (1<<4)
#define MATERIAL_FLAG_DETAIL_DRAW       (1<<5)


//==============================================================================
// NODE
//==============================================================================

#define NODE_LIGHT                      0
#define NODE_PROJ_MATRIX                (NODE_LIGHT + LIGHT_SIZE)
#define NODE_ENV_MATRIX                 (NODE_PROJ_MATRIX + MATRIX4_SIZE)
#define NODE_SIZE                       (NODE_ENV_MATRIX + MATRIX3_SIZE)


//==============================================================================
// VU1 MEMORY LAYOUT
//==============================================================================

#define SHAPE_SYNC_GIF_ADDR                         0
#define SHAPE_PARAMS_ADDR                           1
#define SHAPE_MATERIAL_ADDR                         2
#define SHAPE_NODE_ADDR                             (SHAPE_MATERIAL_ADDR + MATERIAL_SIZE)
#define SHAPE_BASE_ADDR(__nodes__)                  (SHAPE_NODE_ADDR + (NODE_SIZE * __nodes__))
#define SHAPE_BUFFER_SIZE(__base__, __buffers__)    ((1024 - __base__)/__buffers__)

// Relative to tops address for buffering...
#define SHAPE_GIF_ADDR                0
#define SHAPE_VERTS_ADDR              (SHAPE_GIF_ADDR + 1)





#endif  // #define __PS2_SHAPE_HPP__

