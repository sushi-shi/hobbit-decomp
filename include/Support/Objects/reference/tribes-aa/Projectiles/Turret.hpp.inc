//==============================================================================
//
//  Turret.hpp
//
//==============================================================================

#ifndef TURRET_HPP
#define TURRET_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Asset.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class turret : public asset
{

//------------------------------------------------------------------------------
//  Public Types

public:

    enum kind
    {
        AA,
        ELF,
        MISSILE,
        MORTAR,
        PLASMA,
        SENTRY,
        DEPLOYED,
        TAIL,
    };

//------------------------------------------------------------------------------
//  Public Functions

public:

                turret          ( void );

    void        Initialize      ( const vector3&   WorldPos,
                                  const radian3&   WorldRot,
                                        s32        Switch,
                                        s32        Power,
                                        kind       Kind,
                                  const xwchar*    pLabel );

    void        Initialize      ( const vector3&   WorldPos,
                                  const vector3&   Normal,
                                  const object::id PlayerID,
                                        u32        TeamBits );

    void        MPBInitialize   ( const matrix4    L2W,
                                        u32        TeamBits );

    void        OnRemove        ( void );

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );

    void        DebugRender     ( void );
    void        Render          ( void );

    void        RenderEffects   ( void );

    void        UpdateInstance  ( void );

    vector3     GetPainPoint    ( void ) const;

    xbool       UsesBarrel      ( void );
    xbool       ChangeBarrel    ( kind Kind );
    kind        GetKind         ( void ) const;

    f32         GetSenseRadius  ( void ) const;

    vector3     GetBlendFirePos ( s32 Index = 0 );
    
virtual void    UnbindOriginID  ( object::id OriginID );
virtual void    UnbindTargetID  ( object::id TargetID );
virtual void    MPBSetL2W       ( const matrix4& L2W );

    xbool       IsMPB           ( void )    { return m_MPB; }

//------------------------------------------------------------------------------
//  Private Types

protected:

    enum state
    {
        STATE_UNDEFINED,
        STATE_IDLE,
        STATE_ACTIVATING,
        STATE_AIMING,
        STATE_COOLING,
        STATE_UNAIMING,
        STATE_DEACTIVATING,
        STATE_BARREL_CHANGING,
        STATE_DEPLOYING,
    };

//------------------------------------------------------------------------------
//  Private Functions

protected:

virtual void    Disabled        ( object::id OriginID );
virtual void    Destroyed       ( object::id OriginID );
virtual void    Enabled         ( object::id OriginID );

    void        CommonInit      ( void );

    update_code ServerLogic     ( f32 DeltaTime );
    update_code ClientLogic     ( f32 DeltaTime );

    s32         ServerTrack     ( f32 DeltaTime );
    void        ClientTrack     ( f32 DeltaTime );

    void        TrackTarget     ( const object* pObject, radian& Pitch, radian& Yaw );

    void        EnterState      ( state State );

    xbool       LookForTarget   ( void );
    xbool       UpdateAim       ( f32 DeltaTime, radian Pitch, radian Yaw );
    xbool       CheckLOS        ( object* pObject );
    xbool       CheckLOS        ( void );

    void        Fire            ( f32 DeltaTime );

//------------------------------------------------------------------------------
//  Private Data

protected:

    matrix4         m_W2L;
    state           m_State;
    state           m_PreChangeState;
    object::id      m_TargetID;
    object::id      m_OriginID;
    s32             m_TargetRank;
    s32             m_ShotCount;
    f32             m_Timer;
    f32             m_Reload;
    kind            m_Kind;
    radian          m_Pitch;
    radian          m_Yaw;
    vector3         m_PainPoint;
    shape_instance  m_Barrel;
    xbool           m_ELFing;
    s32             m_ELFAudioHandle;
    xbool           m_MPB;
    xbool           m_MissileTrack;
};

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     TurretCreator( void );

//==============================================================================
#endif // TURRET_HPP
//==============================================================================
