//=========================================================================
//
// TERR.H
//
//=========================================================================
#ifndef TERR_H
#define TERR_H

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/Entropy/e_View.hpp>
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include <Support/ObjectMgr/Collider.hpp>
#include <Support/Fog/Fog.hpp>
#include <Support/PointLight/PointLight.hpp>
//#include "extrusion.hpp"

//#include "fog.hpp"

//=========================================================================
#ifdef TARGET_PS2
#define TERR_MAX_BASE_PASSES    4
#endif

#ifdef TARGET_PC
#define TERR_MAX_BASE_PASSES    1
#endif

#ifdef TARGET_LINUX
#define TERR_MAX_BASE_PASSES    1
#endif


class terr
{
private:

    struct terr_material
    {
        char        Name[64];
//        xbitmap     BMP;
    };

    struct terr_stack_node
    {
        vector3 Offset;
        s32     MinZ,MaxZ;
        s32     MinX,MaxX;
        f32     MinY,MaxY;
        s32     ClipMask;
        s32     Level;
    };

    struct block_alpha
    {
        byte Alpha[81];
        byte Pad[15];
    };

    struct block
    {
        s16         Height[81];
        byte        Pad[6+8];
        u64         SquareMask;
        s32         NPasses;
        u32         PassMask;
        block_alpha Alpha[TERR_MAX_BASE_PASSES];
    } PS2_ALIGNMENT(16);

    struct dlist_fragment
    {
        byte        DList[16*16];
    } PS2_ALIGNMENT(16);

    block           m_Block[32*32];
    s16             m_MinY[21845];
    s16             m_MaxY[21845];
    s16             m_MinMaxOffset[9];
    terr_material   m_Material[TERR_MAX_BASE_PASSES];
    terr_material   m_DetailMat;
    dlist_fragment  m_DListActivate[TERR_MAX_BASE_PASSES];
    xcolor          m_DetailMipColor;
    char            m_FileName[32];
    s32             m_NPasses;
    f32             m_DetailDist;

    f32             m_BaseWorldPixelSize;
    f32             m_DetailWorldPixelSize;
    f32             m_BaseMipK;
    f32             m_DetailMipK;
    f32             m_DetailMipFade[5];

    xbool           m_DoFog;
    xbool           m_DoDetail;

    #define TERR_NUM_PLANES 11
    plane           m_ClipPlane[TERR_NUM_PLANES]; // L,R,B,T,N,F,CL,CR,CB,CT,CN
    s32             m_ClipMinIndex[TERR_NUM_PLANES*3];
    s32             m_ClipMaxIndex[TERR_NUM_PLANES*3];
    f32             m_MaxHeight;
    f32             m_MinHeight;
    vector3         m_EyePos;
    vector3         m_OccPos;
    s32             m_SuperTextureID;

    fog*            m_Fog;

    s32             IsBoxInView( const bbox& BBox, s32 Mask);
    void            EmitBlocks( void );
    f32             NearestDistSquared( const vector3& Min, const vector3& Max );
    void            ShipBlockNoClip( s32 BZ, s32 BX, const vector3& Min, const vector3& Max, f32 NDist );
    void            ShipBlockClip( s32 BZ, s32 BX, const vector3& Min, const vector3& Max, f32 NDist );
    void            BootAndLoadMatrices( const view* pView );
    void            BuildLighting( char* pFileName );
    void            LoadDetailMap( void );
    void            OpenD3DPass(void);
    void            CloseD3DPass(void);
    xbool           RayBlockCollision(       collider& Collider,
                                       const vector3& P0,
                                       const vector3& P1);

    void            RayCollide( collider& Collider );
    void            ExtCollide( collider& Collider );

    xbool           CrudeRay( s32 X0, s32 Y0, f32 Z0, s32 X1, s32 Y1, f32 Z1 );
    xbool           OcclusionScanX(const vector3& edge0, const vector3& edge1, const vector3& localCamPos);
    xbool           OcclusionScanY(const vector3& edge0, const vector3& edge1, const vector3& localCamPos);

public:
            terr            ( void );
           ~terr            ( void );
    void    Load            ( const char* pFileName );
    void    Kill            ( void );
    void    Render          ( xbool DoFog, xbool DoDetail );
    f32     GetVertLight    ( s32 VZ, s32 VX );
    f32     GetVertHeight   ( s32 VZ, s32 VX );
    f32     GetHeight       ( f32 PZ, f32 PX );
    void    GetQuadHeights  ( s32 VZ, s32 VX, f32* pH );
    void    GetNormal       ( f32 PZ, f32 PX, vector3& Normal );
    void    GetQuadBounds   ( s32 PZ, s32 PX, s32 Level, f32& Min, f32& Max );
    char*   GetName         ( void ) {return m_FileName;}
    void    SetFog          ( const fog* pFog );
    void    GetMinMaxHeight ( f32& MinY, f32& MaxY );

    void    Collide         ( collider& Collider );

    void    BuildLightingData( const vector3&    LightDir,
                                     f32         ShadowFalloffDist,
                                     f32         MinShadow,
                                     f32         MaxShadow,
                                     f32         MinDiffuse,
                                     f32         MaxDiffuse,
                                     f32         MinGlobal,
                                     f32         MaxGlobal,
                               const char*       pFileName );

    void    SetEmptySquare  ( s32 SQ );
    xbool   IsSquareEmpty   ( s32 Z, s32 X );
    xbool   IsModSquareEmpty( s32 Z, s32 X );

    void    GetLighting     ( f32 Z, f32 X, xcolor& C, f32& Y );

    void    GetSphereFaceKeys ( const vector3&      Pos,
                                      f32           Radius,
                                      s32           ObjectSlot,
                                      s32&          NKeys,
                                      fcache_key*   pKey );

    void    GetPillFaceKeys ( const vector3&    StartPos,
                              const vector3&    EndPos,
                                    f32         Radius,
                                    s32         ObjectSlot,
                                    s32&        NKeys,
                                    fcache_key* pKey );
                
    void    ConstructFace   ( fcache_face& Face );

    xbool   IsOccluded  ( const bbox&     BBox,
                          const vector3&  EyePos );
};


//=========================================================================
#endif
//=========================================================================
