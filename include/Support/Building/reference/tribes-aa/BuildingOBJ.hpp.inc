#ifndef BUILDING_OBJECT_HPP
#define BUILDING_OBJECT_HPP

//==============================================================================
// INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include "BldInstance.hpp"
#include <Support/PointLight/PointLight.hpp>

//==============================================================================
// TYPES
//==============================================================================

class building_obj : public object
{
public:

                   ~building_obj        ( void );
    void            Initialize          ( const char* pFileName, const matrix4& L2W, s32 Power );    
    update_code     OnAdvanceLogic      ( f32 DeltaTime );
    void            OnCollide           ( u32 AttrBits, collider& Collider );
    void            RenderGrid          ( void );
    void            RenderGridNGons     ( void );
    void            RenderRayNGons      ( void );
    void            RenderCellNGons     ( const vector3& Pos );
    void            GetLighting         ( const vector3& Pos,
                                          const vector3& RayDir,
                                          xcolor&        C,
                                          vector3&       BuildingPt );

    void            GetSphereFaceKeys   ( const vector3&        EyePos,
                                          const vector3&        Pos,
                                          f32                   Radius,
                                          s32&                  NKeys,
                                          fcache_key*           pKey );

    void            GetPillFaceKeys     ( const vector3&    StartPos,
                                          const vector3&    EndPos,
                                                f32         Radius,
                                                s32         ObjectSlot,
                                                s32&        NKeys,
                                                fcache_key* pKey );
    
    void            ConstructFace       ( fcache_face& Face );
    const char*     GetBuildingName     ( void );
    matrix4         GetW2L              ( void );
    matrix4         GetL2W              ( void );
    s32             FindZoneFromPoint   ( const vector3& Point );
    
    void            SetAlarmLighting    ( xbool State );

//protected:

    bld_instance    m_Instance;
    s32             m_Power;

protected:


    void            RayCollide          ( collider&                             Collider, 
                                          const building::building_grid&        Grid );

    void            RayCollideWithCell  ( collider&                             Collider, 
                                          const building::building_grid&        Grid,
                                          s32                                   GridCell,
                                          const vector3&                        P0,
                                          const vector3&                        P1,
                                          f32                                   t0, 
                                          f32                                   t1 );

};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BuildingCreator     ( void );
void    LightBuildings      ( void );
void    RenderBuildingDebug ( void );

//==============================================================================
#endif // BUILDING_OBJECT_HPP
//==============================================================================
