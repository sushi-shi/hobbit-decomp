//==============================================================================
//
//  Station.hpp
//
//==============================================================================

#ifndef STATION_HPP
#define STATION_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Objects\Projectiles\Asset.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class station : public asset
{

//------------------------------------------------------------------------------
//  Public data types

public:
    
    enum kind
    {
        FIXED    = TYPE_STATION_FIXED    - TYPE_STATION_FIXED,
        VEHICLE  = TYPE_STATION_VEHICLE  - TYPE_STATION_FIXED,
        DEPLOYED = TYPE_STATION_DEPLOYED - TYPE_STATION_FIXED,
    };

//------------------------------------------------------------------------------
//  Public functions

public:
    
                station         ( void );

    void        Initialize      ( const vector3&   Position,          
                                  const radian3&   Rot,
                                        s32        Power,
                                        s32        Switch,
                                  const xwchar*    pLabel );

    void        Initialize      ( const vector3&   Position,          
                                        radian     Yaw,
                                  const object::id PlayerID,
                                        u32        TeamBits );

    void        MPBInitialize   ( const matrix4    L2W,
                                        u32        TeamBits );

    void        OnRemove        ( void );

    update_code OnAdvanceLogic  ( f32 DeltaTime );

    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate (       bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    void        OnCollide       ( u32 AttrBits, collider& Collider );

    void        Render          ( void );
    void        DebugRender     ( void );

    object::id  OccupiedBy      ( void );

    void        MPBSetL2W       ( const matrix4& L2W );
    kind        GetKind         ( void ) const { return( m_Kind ); }
    vector3     GetPainPoint    ( void ) const;

    void        DeployBeacon    ( void ) const;    

//------------------------------------------------------------------------------
//  Private data types

    enum state
    {
        STATE_UNDEFINED,
        STATE_IDLE,
        STATE_PLAYER_EQUIP,
        STATE_PLAYER_REPAIR,
        STATE_PRE_VEHICLE_REQUEST,
        STATE_VEHICLE_REQUEST,
        STATE_POST_VEHICLE_REQUEST,
        STATE_DEPLOYING,
        STATE_COOL_DOWN,
    };

//------------------------------------------------------------------------------
//  Private functions

protected:

    update_code ServerLogic     ( f32 DeltaTime );
    update_code ClientLogic     ( f32 DeltaTime );

    void        CommonInit      ( void );
    void        SetupTriggers   ( void );
    void        EnterState      ( state State );
    s32         LookForPlayer   ( void );    

    void        Destroyed       ( object::id OriginID );

    void        Disabled        ( object::id OriginID );
    void        Enabled         ( object::id OriginID );

//------------------------------------------------------------------------------
//  Private data

protected:

    state           m_State;
    state           m_PreChangeState;
    bbox            m_TriggerBBox;
    bbox            m_ReleaseBBox;
    vector3         m_SnapPosition;
    f32             m_Timer;
    f32             m_Activate;
    object::id      m_PlayerID;
    s32             m_Power;
    s32             m_Switch;
    s32             m_Handle;
    object::id      m_VPadID;
    kind            m_Kind;     // 0:Fixed  1:Vehicle  2:Deployed
    xbool           m_MPB;
};

//==============================================================================
#endif // STATION_HPP
//==============================================================================
