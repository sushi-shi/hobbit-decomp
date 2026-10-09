//==============================================================================
//
//  Asset.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Asset.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Entropy/e_View.hpp>
#include "GameMgr\GameMgr.hpp"
#include "AudioMgr/Audio.hpp"
#include "NetLib\BitStream.hpp"
#include "ParticleObject.hpp"
#include "..\AADS\Globals.hpp"
#include "Bubble.hpp"
#include <Support/ObjectMgr/ColliderCannon.hpp>

//==============================================================================
//  LOCAL STORAGE
//==============================================================================

debug_asset g_DebugAsset = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, };

//==============================================================================

volatile f32 ASSET_AMBIENT_HUE = 1.0f;
volatile f32 ASSET_AMBIENT_INT = 0.5f;

volatile f32 ASSET_DIRECT_HUE  = 0.5f;
volatile f32 ASSET_DIRECT_INT  = 0.1f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

asset::asset( void )
{
    // Ambient sound is left to the descendant classes.
    m_AmbientID     = 0;
    m_AmbientHandle = 0;

    // Default shield parameters.  Descendants may (should!) modify.
    m_BubbleOffset( 0, 0, 0 );
    m_BubbleScale ( 1, 1, 1 );

    // Draw variables.
    m_DmgTexture     = NULL;
    m_DmgClutHandle  = -1; 
    m_ShapeDrawFlags =  0;

    m_EnableLevel    = 0.25f;
}

//==============================================================================

object::update_code asset::OnAdvanceLogic( f32 DeltaTime )
{    
    xbool Power = GameMgr.GetPower( m_Power );

    // Update team bits based on the state of the switch circuit.
    if( m_Switch != -1 )
        m_TeamBits = GameMgr.GetSwitchBits( m_Switch );

    // Check for change in power.
    if(  m_PowerOn && !Power )      PowerOff();
    else
    if( !m_PowerOn &&  Power )      PowerOn();

    // If there is power, regenerate the energy, else take it all away.
    if( m_PowerOn )
    {
        if( (!m_Disabled) && (m_Energy < 1.0f) )
        {
            m_Energy = MIN( 1.0f, m_Energy + (DeltaTime * m_RechargeRate) );
            m_DirtyBits |= 0x80000000;
        }
    }
    else
    {
        if( m_Energy > 0.0f )
        {
            m_Energy = 0.0f;
            m_DirtyBits |= 0x80000000;
        }
    }

    return( NO_UPDATE );
}

//==============================================================================

s32 asset::RenderPrep( void )
{
    // Is asset visible?
    s32 Visible = IsVisibleInZone( m_ZoneSet, m_WorldBBox );
    if( !Visible )
        return( 0 );

    // Setup draw flags.
    m_ShapeDrawFlags = shape::DRAW_FLAG_FOG;
    if( Visible == view::VISIBLE_PARTIAL )
        m_ShapeDrawFlags |= shape::DRAW_FLAG_CLIP;

    // Set a color to indicate "health" of asset.
    { 
        f32    Value = MIN( 1.0f, m_Health / m_EnableLevel );
        u8     Comp  = (u8)(127 + (Value * 128.0f));
        xcolor Color( Comp, Comp, Comp );       

        m_Shape.SetColor( Color );
    }

    // Use the "destroyed" model?
    if( m_Destroyed )
    {
        m_Shape.SetModelByIndex( 1 );
    }
    else
    {
        m_Shape.SetModelByIndex( 0 );
    }

    // Lookup damage texture and clut.
    tgl.Damage.GetTexture( GetHealth(), m_DmgTexture, m_DmgClutHandle );

    // Setup fog color.
    m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );

    // Flag it's visible.
    return( Visible );
}

//==============================================================================

void asset::DebugRender( void )
{
    if( !g_DebugAsset.DebugRender )
        return;
/*
    if( g_DebugAsset.ShowLabel )
        draw_Label( m_WorldPos, XCOLOR_YELLOW, m_Label );
*/
    if( g_DebugAsset.ShowStats )
    {
        draw_Label( m_WorldPos, XCOLOR_YELLOW, 
                    xfs( "\n\n\n%d/%d", (s32)(m_Health * 100.0f), 
                                        (s32)(m_Energy * 100.0f) ) );
    }

    if( g_DebugAsset.ShowBBox )
        draw_BBox( m_WorldBBox, XCOLOR_BLACK );

    if( g_DebugAsset.ShowCollision )
        m_Collision.DrawCollisionModel();

    if( g_DebugAsset.ShowPainInfo )
    {
        draw_Point ( GetBlendPainPoint(), XCOLOR_RED );
        draw_Sphere( GetBlendPainPoint(), GetPainRadius(), XCOLOR_RED );
    }
}

//==============================================================================

void asset::Render( void )
{
    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

object::update_code asset::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    xbool Flag;

    if( BitStream.ReadFlag() )
    {
        Flag = BitStream.ReadFlag();
        if(  Flag && !m_Disabled )  Disabled( ObjMgr.NullID );
        if( !Flag &&  m_Disabled )  Enabled ( ObjMgr.NullID );
        m_Disabled = Flag;
        if( m_Disabled )    m_Energy = 0.0f;
        else                BitStream.ReadRangedF32( m_Energy, 10, 0.0f, 1.0f );

        Flag = BitStream.ReadFlag();
        if( Flag && !m_Destroyed )  Destroyed( ObjMgr.NullID );
        m_Destroyed = Flag;
        if( m_Destroyed )   m_Health = 0.0f;
        else                BitStream.ReadRangedF32( m_Health, 10, 0.0f, 1.0f );
    }

    return( OnAdvanceLogic( SecInPast ) );
}

//==============================================================================

void asset::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( BitStream.WriteFlag( DirtyBits & 0x80000000 ) )
    {
        BitStream.WriteFlag( m_Disabled );
        if( !m_Disabled )   
            BitStream.WriteRangedF32( m_Energy, 10, 0.0f, 1.0f );
        
        BitStream.WriteFlag( m_Destroyed );
        if( !m_Destroyed )   
            BitStream.WriteRangedF32( m_Health, 10, 0.0f, 1.0f );

        DirtyBits &= ~0x80000000;
    }
}

//==============================================================================

void asset::Initialize( const vector3& WorldPos,
                        const radian3& WorldRot,
                              s32      Switch,  
                              s32      Power,
                        const xwchar*  pLabel )
{   
    // Set rotation and position.
    m_WorldRot = WorldRot;
    m_WorldPos = WorldPos;

    // Set up a place holder bbox.
    m_WorldBBox( vector3(-1,0,-1), vector3(1,2,1) );
    m_WorldBBox.Translate( m_WorldPos );

    ComputeZoneSet( m_ZoneSet, m_WorldBBox, TRUE );

    // Set switch and power circuits.
    m_Switch = Switch;
    m_Power  = Power;

    // Other values.
    if( pLabel )
    {
        x_wstrncpy( m_Label, pLabel, 31 );
        m_Label[31] = '\0';
    }
    else 
    {    
        m_Label[ 0] = '\0';
    }

    // Full power!
    m_Health    = 1.0f;
    m_Energy    = 1.0f;
    m_Destroyed = FALSE;
    m_Disabled  = FALSE;
    m_Explode   = TRUE;

    CommonInit();
}

//==============================================================================

void asset::OnAcceptInit( const bitstream& BitStream )
{
    BitStream.ReadVector   ( m_WorldPos );
    BitStream.ReadRadian3  ( m_WorldRot );
    BitStream.ReadRangedS32( m_Switch, -1, 15 );
    BitStream.ReadRangedS32( m_Power,  -1, 15 );

    if( m_Switch == -1 )
        BitStream.ReadTeamBits( m_TeamBits );

    m_Disabled = BitStream.ReadFlag();
    if( m_Disabled )    m_Energy = 0.0f;
    else                BitStream.ReadRangedF32( m_Energy, 10, 0.0f, 1.0f );

    m_Destroyed = BitStream.ReadFlag();
    if( m_Destroyed )   m_Health = 0.0f;
    else                BitStream.ReadRangedF32( m_Health, 10, 0.0f, 1.0f );

    m_WorldBBox( vector3(-1,0,-1), vector3(1,2,1) );
    m_WorldBBox.Translate( m_WorldPos );

    CommonInit();
}

//==============================================================================

void asset::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteVector   ( m_WorldPos );
    BitStream.WriteRadian3  ( m_WorldRot );
    BitStream.WriteRangedS32( m_Switch, -1, 15 );
    BitStream.WriteRangedS32( m_Power,  -1, 15 );

    if( m_Switch == -1 )    
        BitStream.WriteTeamBits( m_TeamBits );

    if( !BitStream.WriteFlag( m_Disabled ) )
        BitStream.WriteRangedF32( m_Energy, 10, 0.0f, 1.0f );

    if( !BitStream.WriteFlag( m_Destroyed ) )
        BitStream.WriteRangedF32( m_Health, 10, 0.0f, 1.0f );
}

//==============================================================================

void asset::CommonInit( void )
{
    // Power up.
    m_PowerOn = GameMgr.GetPower( m_Power );

    // Since collision instance never moves, declare it static for some speedup.
    m_Collision.SetFlag( shape_instance::FLAG_IS_STATIC, TRUE );

    // Texture animation.
    if( m_Disabled )
    {
        m_Shape.SetTextureAnimFPS( 0.0f );
        m_Shape.SetTextureFrame( 0 );
    }
    else
    {
        m_Shape.SetTextureAnimFPS( 8.0f );
    }

    // Setup environmental lighting.
    ResetEnvColor();
}

//==============================================================================

void asset::ResetEnvColor( void )
{
    xcolor Color;

    vector3 RayDir( 0, -10.0f, 0 );
    RayDir.Rotate( m_WorldRot );

    Color = GetAmbientLighting( m_WorldPos, RayDir, ASSET_DIRECT_HUE, ASSET_DIRECT_INT );
    m_Shape.SetLightColor( Color );

    Color = GetAmbientLighting( m_WorldPos, RayDir, ASSET_AMBIENT_HUE, ASSET_AMBIENT_INT );
    m_Shape.SetLightAmbientColor( Color );
}

//==============================================================================

void asset::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    if( Collider.GetType() == collider::RAY )
    {
        m_Shape.ApplyCollision( Collider );
    }
    else
    {
        if( m_Collision.GetShape() )
            m_Collision.ApplyCollision( Collider );
        else
            Collider.ApplyBBox( m_WorldBBox );
    }
}

//==============================================================================

void asset::OnPain( const pain& Pain )
{
    // Special case:  Assets (specifically turrets) cannot damage themselves.
    // Special case:  Fixed inventory stations are invulnerable if:
    //                  (a) available to all teams, or 
    //                  (b) it is a non-team based game.    
    if( (Pain.OriginID == this->m_ObjectID) ||       
        ( (m_pTypeInfo->Type == TYPE_STATION_FIXED) &&
          ( (m_TeamBits == 0xFFFFFFFF) ||
            (!GameMgr.IsTeamBasedGame()) ) ) )
    {
        if( Pain.LineOfSight )
        {
            CreateShieldEffect( m_ObjectID, 1.0f );
        }
        return;
    }

    if( (Pain.LineOfSight) && 
        ((m_Health > 0.0f) || (Pain.MetalDamage < 0.0f)) )
    {
        f32 OldHealth = m_Health;

        m_Energy -= Pain.EnergyDamage;
        m_Health -= Pain.MetalDamage;

        m_Energy  = MINMAX( 0.0f, m_Energy, 1.0f );
        m_Health  = MINMAX( 0.0f, m_Health, 1.0f );
        
        if( m_Disabled )
        {
            // Enabled?
            if( m_Health >= m_EnableLevel )
                Enabled( Pain.OriginID );
        }

        // Disabled?
        // This disabled code must be able to work even if there is no power
        // (in which case the asset IS ALREADY disabled).  This allows scores
        // to be assigned to players when they damage a non-powered asset to
        // the disable threshold.
        if( (m_Health < m_EnableLevel) && (OldHealth >= m_EnableLevel) )
            Disabled( Pain.OriginID );

        if( m_Destroyed )
        {
            // "Undestroyed"?
            if( m_Health > 0.0f )
                m_Destroyed = FALSE;
        }
        else
        {
            // Destroyed?
            if( m_Health <= 0.0f )
                Destroyed( Pain.OriginID );
        }

        // Fully repaired?
        if( (OldHealth < 1.0f) && (m_Health == 1.0f) )
            Repaired( Pain.OriginID );

        // Show a shield effect?
        if( (m_Energy > 0.0f) && (Pain.EnergyDamage > 0.0f) )
        {
            CreateShieldEffect( m_ObjectID, m_Energy );
        }

        // If damaged by the "repaired by" player, clear the "repaired by" field.
        if( (Pain.MetalDamage > 0.0f) && (m_RepairedByID == Pain.OriginID) )
            m_RepairedByID = obj_mgr::NullID;

        // If repaired by a player other than the one in the "repaired by" field,
        // clear the field.
        if( (Pain.MetalDamage < 0.0f) && (m_RepairedByID != Pain.OriginID) )
            m_RepairedByID = obj_mgr::NullID;

        m_DirtyBits |= 0x80000000;
    }
}

//==============================================================================

xbool asset::GetEnabled( void ) const
{
    return( !m_Disabled );
}

//==============================================================================

f32 asset::GetHealth( void ) const
{
    return( m_Health );
}

//==============================================================================

f32 asset::GetEnergy( void ) const
{
    return( m_Energy );
}

//==============================================================================

xbool asset::IsShielded( void ) const
{
    return( TRUE );
}

//==============================================================================

const xwchar* asset::GetLabel( void ) const
{
    return( m_Label );
}

//==============================================================================

s32 asset::GetSwitchCircuit( void ) const
{
    return( m_Switch );
}

//==============================================================================

s32 asset::GetPowerCircuit( void ) const
{
    return( m_Power );
}

//==============================================================================

void asset::Disabled( object::id OriginID )
{
    m_Disabled   = TRUE;
    m_Energy     = 0.0f;
    m_DirtyBits |= 0x80000000;

    pGameLogic->ItemDisabled( m_ObjectID, OriginID );

    m_Shape.SetTextureAnimFPS( 0.0f );
    m_Shape.SetTextureFrame( 0 );

    audio_Stop( m_AmbientHandle );
    m_AmbientHandle = 0;
}

//==============================================================================

void asset::Destroyed( object::id OriginID )
{
    if( !m_Disabled )   Disabled( OriginID );

    if( m_Explode )
        pGameLogic->ItemDestroyed( m_ObjectID, OriginID );

    m_Destroyed  = TRUE;
    m_Explode    = FALSE;
    m_Health     = 0.0f;
    m_DirtyBits |= 0x80000000;    
}

//==============================================================================

void asset::Enabled( object::id OriginID )
{
    if( m_PowerOn )
    {
        m_Disabled   = FALSE;
        m_Energy     = 0.0f;
        m_DirtyBits |= 0x80000000;

        pGameLogic->ItemEnabled( m_ObjectID, OriginID );

        m_Shape.SetTextureAnimFPS( 8.0f );

        if( m_AmbientHandle == 0 )
        {
            m_AmbientHandle = audio_Play( m_AmbientID,
                                          &m_WorldPos,
                                          AUDFLAG_PERSISTENT );
        }

        m_Explode = TRUE;
    }

    // Set the "repaired by" field.  If the same player completes the repairs
    // to 100% health, then that player gets bonus points. 
    m_RepairedByID = OriginID;
}

//==============================================================================

void asset::Repaired( object::id OriginID )
{
    m_Health     = 1.0f;
    m_DirtyBits |= 0x80000000;
    pGameLogic->ItemRepaired( m_ObjectID, OriginID, (OriginID == m_RepairedByID) );
}

//==============================================================================

void asset::PowerOff( void )
{
    m_PowerOn = FALSE;
    if( m_Health >= m_EnableLevel )
        Disabled( obj_mgr::NullID );
    ResetEnvColor();
}

//==============================================================================

void asset::PowerOn( void )
{
    m_PowerOn = TRUE;
    if( m_Health >= m_EnableLevel )
        Enabled( obj_mgr::NullID );
    ResetEnvColor();
}

//==============================================================================

void asset::OnAdd( void )
{
    if( (GameMgr.GetPower( m_Switch )) && (m_AmbientHandle == 0) )
    {
        m_AmbientHandle = audio_Play( m_AmbientID,
                                      &m_WorldPos,
                                      AUDFLAG_PERSISTENT );
    }
}

//==============================================================================

void asset::OnRemove( void )
{
    if( m_AmbientHandle != 0 )
    {
        audio_Stop( m_AmbientHandle );
        m_AmbientHandle = 0;
    }
}

//==============================================================================

void asset::ForceDestroyed( void )
{
    Destroyed( ObjMgr.NullID );
}

//==============================================================================

void asset::ForceDisabled( void )
{
    m_Health = m_EnableLevel * 0.9f;
    Disabled( ObjMgr.NullID );
}

//==============================================================================

void asset::MPBSetL2W( const matrix4& L2W )
{
    m_WorldPos = L2W.GetTranslation();
    m_WorldRot = L2W.GetRotation();

    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );

    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;
}

//==============================================================================

void asset::MPBSetColors( const shape_instance& Source )
{
    m_Shape.SetColor( Source.GetColor() );
    m_Shape.SetFogColor( Source.GetFogColor() );
    m_Shape.SetLightAmbientColor( Source.GetLightAmbientColor() );
    m_Shape.SetLightColor( Source.GetLightColor() );
    m_Shape.SetLightDirection( Source.GetLightDirection() );
}
    
//==============================================================================
