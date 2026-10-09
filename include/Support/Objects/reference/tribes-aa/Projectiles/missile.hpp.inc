//==============================================================================
//
//  Missile.hpp
//
//==============================================================================

#ifndef Missile_HPP
#define Missile_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     MissileCreator        ( void ); 

//==============================================================================
//  TYPES
//==============================================================================

class missile : public object
{

public:

    update_code     AdvanceLogic    ( f32 DeltaTime );
    update_code     OnAdvanceLogic  ( f32 DeltaTime );
    void            OnAcceptInit    ( const bitstream& BitStream );
    void            OnProvideInit   (       bitstream& BitStream );
    update_code     OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void            OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );

    void            OnAdd           ( void );
    void            OnRemove        ( void );
                    
    void            Initialize      ( const matrix4&   L2W,
                                            u32        TeamBits,
                                            object::id OriginID,
                                            object::id TargetID,
                                            s32        Type = 0 );
                            
                                            // Type: 0 = Missile Launcher
                                            //       1 = Missile Turret
                    
    void            BeginRender     ( void );
    void            Render          ( void );
    void            RenderParticles ( void );
    void            EndRender       ( void );

    void            UnbindOriginID  ( object::id OriginID );
    void            UnbindTargetID  ( object::id TargetID );

protected:

    vector3         m_TargetPt;             // final destination if no target object
    object::id      m_TargetID;             // targeted object if not dumb firing
    object::id      m_OriginID;
    
    enum stage
    {
        FIRST_STAGE,                        // just after initial launch
        SECOND_STAGE,                       // casing explodes, new projectile flies away
        CONTACT
    };

    stage           m_CurStage;                     
                    
    vector3         m_LastPos;
    vector3         m_BlendPos;
    xbool           m_UpdateRecvd;

    quaternion      m_Hdg;
    f32             m_Vel;                  // current velocity
    f32             m_Acc;                  // acceleration
    f32             m_Age;
    f32             m_Update;
    
    s32             m_Type;                 // 0 = missile launcher, 1 = missile turret

    s32             m_SfxID;                

    xbool           m_HasLOS;               // we have line-of-sight, no more long distance collision checks req'd
    xbool           m_GiveUp;               // can't find or maintain LOS...just fly to the target and hope for the best
    xbool           m_HasExploded;          // keep processing smoke until no more smoke is seen
    xbool           m_Diverted;             // true once missile has locked onto flare

    shape_instance  m_RocketShape;
    shape_instance  m_CasingShape;

    pfx_effect      m_SmokeEffect;  
};

//==============================================================================
#endif
//==============================================================================
