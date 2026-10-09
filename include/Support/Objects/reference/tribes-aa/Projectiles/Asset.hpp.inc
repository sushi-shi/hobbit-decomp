//==============================================================================
//
//  Asset.hpp
//
//==============================================================================

#ifndef ASSET_HPP
#define ASSET_HPP

//==============================================================================
//  DEFINITIONS
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <Support/Shape/ShapeInstance.hpp>
#include "..\AADS\Globals.hpp"

//==============================================================================
//  TYPES
//==============================================================================

struct debug_asset
{
    xbool   DebugRender;
    xbool   ShowBBox;
    xbool   ShowLabel;
    xbool   ShowStats;
    xbool   ShowPainInfo;
    xbool   ShowCollision;
    xbool   ShowExtra;
};

extern debug_asset g_DebugAsset;

//==============================================================================

class vehicle_pad;
class mpb;

//==============================================================================

class asset : public object
{

//------------------------------------------------------------------------------
//  Public Functions

public:
                asset           ( void );

    update_code OnAdvanceLogic  ( f32 DeltaTime );
    update_code OnAcceptUpdate  ( const bitstream& BitStream, f32 SecInPast );
    void        OnProvideUpdate ( bitstream& BitStream, u32& DirtyBits, f32 Priority );
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    void        OnCollide       ( u32 AttrBits, collider& Collider );
    void        OnPain          ( const pain& Pain );
    void        OnAdd           ( void );
    void        OnRemove        ( void );
                                        
    void        DebugRender     ( void );
    void        Render          ( void );

    void        Initialize      ( const vector3& WorldPos,
                                  const radian3& WorldRot,
                                        s32      Switch,
                                        s32      Power,
                                  const xwchar*  pLabel = NULL );

    xbool       GetEnabled      ( void ) const;
    f32         GetHealth       ( void ) const;     // 0.0 thru 1.0
    f32         GetEnergy       ( void ) const;     // 0.0 thru 1.0
    xbool       IsShielded      ( void ) const;
    const xwchar* GetLabel      ( void ) const;

    s32         GetSwitchCircuit( void ) const;
    s32         GetPowerCircuit ( void ) const;

    radian3     GetRotation     ( void ) const { return( m_WorldRot ); }

virtual void    MPBSetL2W       ( const matrix4& L2W );
virtual void    MPBSetColors    ( const shape_instance& Source );
virtual void    ForceDestroyed  ( void );
virtual void    ForceDisabled   ( void );

//------------------------------------------------------------------------------
//  Private Functions

protected:

virtual void    Disabled        ( object::id OriginID );
virtual void    Destroyed       ( object::id OriginID );
virtual void    Enabled         ( object::id OriginID );
virtual void    Repaired        ( object::id OriginID );

virtual void    PowerOff        ( void );
virtual void    PowerOn         ( void );

        s32     RenderPrep      ( void );

        void    CommonInit      ( void );

        void    ResetEnvColor   ( void );

//------------------------------------------------------------------------------
//  Private Data

protected:

    shape_instance  m_Shape; 
    shape_instance  m_Collision;
    radian3         m_WorldRot;
    s32             m_Switch;
    s32             m_Power;
    f32             m_Health;
    f32             m_Energy;
    f32             m_RechargeRate;
    f32             m_EnableLevel;
    u32             m_AmbientID;
    s32             m_AmbientHandle;
    xbool           m_PowerOn;
    xbool           m_Disabled;
    xbool           m_Destroyed;
    xbool           m_Explode;
    xwchar          m_Label[32];
    id              m_RepairedByID;

    // Draw variables
    texture*        m_DmgTexture;
    s32             m_DmgClutHandle; 
    u32             m_ShapeDrawFlags;

    vector3         m_BubbleOffset;
    vector3         m_BubbleScale;

    zone_set        m_ZoneSet;

    static  xbool   s_AssetDebugRender;
    static  xbool   s_AssetShowCollision;

friend vehicle_pad;
friend mpb;

};

//==============================================================================
#endif // ASSET_HPP
//==============================================================================
