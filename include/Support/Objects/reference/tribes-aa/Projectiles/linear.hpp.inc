//==============================================================================
//
//  Linear.hpp
//
//==============================================================================

#ifndef LINEAR_HPP
#define LINEAR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class linear : public object
{

public:
    
                        linear          ( void );

        update_code     AdvanceLogic    ( f32 DeltaTime );
                                        
        update_code     OnAdvanceLogic  ( f32 DeltaTime );
        virtual void    OnAcceptInit    ( const bitstream& BitStream );
        virtual void    OnProvideInit   (       bitstream& BitStream );
        void            OnAdd           ( void );
        void            OnRemove        ( void );
                                        
        virtual void    Initialize      ( const matrix4&   L2W, 
                                                u32        TeamBits,
                                                object::id OriginID,
                                                object::id ExcludeID = obj_mgr::NullID );

        void            SetScoreID      ( const object::id ScoreID );
        void            SetShotIndex    ( s32 ShotIndex );    

virtual update_code     Impact          ( vector3& Pos, vector3& Norm ) = 0;

        s32             RenderPrep      ( void );
const   vector3&        GetRenderPos    ( void );    

protected:

    matrix4             m_Orient;
    vector3             m_Start;
    vector3             m_Movement;
    xbool               m_Blending;
    vector3             m_BlendVector;
    vector3             m_BlendPos;
    xbool               m_BlendReady;
    f32                 m_Speed;
    f32                 m_Age;
    f32                 m_Update;           // Time since last update
    f32                 m_Limit;            // Time of any static impact
    f32                 m_Life;             // Time of fizzle (maximum age)
    plane               m_ImpactPlane;
    vector3             m_ImpactPoint;
    xbool               m_Hidden;
    object::id          m_OriginID;         // What object created the linear
    object::id          m_ScoreID;          // What object gets points if any
    object::id          m_ExcludeID;        // What object should not be hit
    s32                 m_SfxHandle;
    u32                 m_SoundID;
    object::id          m_ImpactObjectID;
    object::type        m_ImpactObjectType;
    u32                 m_ImpactObjectAttr;
    s32                 m_ShotIndex;
    s32                 m_PointLightID;
    u32                 m_ShapeDrawFlags;
};

//==============================================================================
#endif // LINEAR_HPP
//==============================================================================
