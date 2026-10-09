//==============================================================================
//
//  Sensor.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Sensor.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "GameMgr\GameMgr.hpp"
#include "NetLib\BitStream.hpp"
#include "ParticleObject.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Damage/Damage.hpp"

#include "Objects\Projectiles\Debris.hpp"

#include <Support/StringMgr/StringMgr.hpp>
#include "AADS/Data/UI/ui_strings.h"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   SensorCreator;

//==============================================================================

obj_type_info   SensorLargeTypeInfo ( object::TYPE_SENSOR_LARGE, 
                                      "Large Sensor", 
                                      SensorCreator, 
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_PERMANENT     |
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_DAMAGEABLE    |
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE       |
                                      object::ATTR_HOT ); 

//==============================================================================

obj_type_info   SensorMediumTypeInfo( object::TYPE_SENSOR_MEDIUM, 
                                      "Medium Sensor", 
                                      SensorCreator, 
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_PERMANENT     |
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_DAMAGEABLE    |
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE       |
                                      object::ATTR_HOT ); 

//==============================================================================

obj_type_info   SensorRemoteTypeInfo( object::TYPE_SENSOR_REMOTE, 
                                      "Remote Sensor", 
                                      SensorCreator, 
                                      object::ATTR_SOLID_DYNAMIC | 
                                      object::ATTR_LABELED       |
                                      object::ATTR_ASSET         | 
                                      object::ATTR_DAMAGEABLE    |
                                      object::ATTR_REPAIRABLE    | 
                                      object::ATTR_ELFABLE ); 

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* SensorCreator( void )
{
    return( (object*)(new sensor) );
}

//==============================================================================

void sensor::Render( void )
{
    // Special case.
    if( (m_Size == 0) && (m_Health == 0.0f) )
        return;

    // Visible?
    if( !RenderPrep() )
        return;

    // Draw sensor shape.
    m_Shape.Draw( m_ShapeDrawFlags, m_DmgTexture, m_DmgClutHandle );

    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

f32 sensor::GetSenseRadius( void ) const
{
    f32 SenseRadius[3] = { 150, 225, 300, };
    if( (m_PowerOn) && (!m_Disabled) )
        return( SenseRadius[m_Size] );
    else
        return( 0.0f );
}

//==============================================================================

void sensor::Destroyed( object::id OriginID )
{
    pain::type PainType[] = 
    { 
        pain::EXPLOSION_SENSOR_REMOTE,
        pain::EXPLOSION_SENSOR_MEDIUM,
        pain::EXPLOSION_SENSOR_LARGE,
    };

    if( m_Explode )
    {
        SpawnExplosion( PainType[m_Size],
                        m_WorldBBox.GetCenter(),
                        vector3(0,1,0),
                        OriginID );
    }
    asset::Destroyed( OriginID );

//  SpawnDebris( 6, m_WorldBBox.GetCenter(), vector3(0,1,0), R_45, 25 );
}

//==============================================================================

object::update_code sensor::OnAdvanceLogic( f32 DeltaTime )
{
    if( (m_Health == 0.0f) && (m_Size == 0) )
    {
        return( DESTROY );
    }

    if( (m_PowerOn) && (!m_Disabled) )
    {
        m_Shape.Advance( DeltaTime );
        m_Timer += DeltaTime;

        if( m_Timer > 30.0f )
        {
            m_DirtyBits |= 0x00000001;
            m_Timer = x_frand( 0.0f, 5.0f );
        }
    }

    return( asset::OnAdvanceLogic( DeltaTime ) );
}

//==============================================================================

void sensor::Initialize( const vector3& WorldPos,
                         const radian3& WorldRot,
                               s32      Switch,  
                               s32      Power,
                         const xwchar*  pLabel )
{   
    asset::Initialize( WorldPos, WorldRot, Switch, Power, pLabel );
    CommonInit();
}

//==============================================================================

void sensor::Initialize( const vector3&   WorldPos,
                         const vector3&   Normal,
                               u32        TeamBits )
{   
    radian3 WorldRot;
    Normal.GetPitchYaw( WorldRot.Pitch, WorldRot.Yaw );
    WorldRot.Roll   = R_0;
    WorldRot.Pitch += R_90;

    asset::Initialize( WorldPos, WorldRot, -1, 0, StringMgr( "ui", IDS_REMOTE_SENSOR ) );
    CommonInit();

    m_TeamBits = TeamBits;
    audio_Play( SFX_PACK_SENSOR_DEPLOY, &m_WorldPos );
}

//==============================================================================

void sensor::OnAcceptInit( const bitstream& BitStream )
{
    f32 Frame;

    asset::OnAcceptInit( BitStream );
    BitStream.ReadRangedS32( m_Size, 0, 2 );
    BitStream.ReadRangedF32( Frame, 10, 0.0f, 1.0f );

    x_wstrcpy( m_Label, StringMgr( "ui", IDS_REMOTE_SENSOR ) );

    CommonInit();

    m_Shape.GetCurrentAnimState().SetFrameParametric( Frame );

    audio_Play( SFX_PACK_SENSOR_DEPLOY, &m_WorldPos );
}

//==============================================================================

void sensor::CommonInit( void )
{
    m_Size = m_pTypeInfo->Type - TYPE_SENSOR_REMOTE;
    ASSERT( IN_RANGE( 0, m_Size, 2 ) );

    s32 ShapeType[3] = 
    {
        SHAPE_TYPE_ASSET_SENSOR_DEPLOYED,
        SHAPE_TYPE_ASSET_SENSOR_MEDIUM,
        SHAPE_TYPE_ASSET_SENSOR_LARGE,
    };

    s32 CollisionType[3] = 
    {
        SHAPE_TYPE_ASSET_SENSOR_DEPLOYED_COLL,
        SHAPE_TYPE_ASSET_SENSOR_MEDIUM_COLL,
        SHAPE_TYPE_ASSET_SENSOR_LARGE_COLL,
    };

    f32 RechargeRate[3] = { 0.1f, 0.05f, 0.05f };

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( ShapeType[m_Size] ) );
    ASSERT(m_Shape.GetShape()) ;
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );

    m_Collision.SetShape( tgl.GameShapeManager.GetShapeByType( CollisionType[m_Size] ) );
    ASSERT(m_Collision.GetShape()) ;
    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    // Set bbox.
    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;

    ComputeZoneSet( m_ZoneSet, m_WorldBBox, TRUE );

    // Prepare for ambient audio.
    m_AmbientID = SFX_POWER_SENSOR_HUMM_LOOP;

    // Clear timer.
    m_Timer = 0.0f;

    // Recharge rate.
    m_RechargeRate = RechargeRate[m_Size];

    // Set up the bubble effects values.
    switch( m_Size )
    {
    case 0:
        m_BubbleOffset( 0.0f, 0.3f, 0.0f );
        m_BubbleScale ( 1.5f, 1.5f, 1.5f );
        break;
    case 1:
        m_BubbleOffset( 0.0f, 2.5f, 0.0f );
        m_BubbleScale ( 7.0f, 8.0f, 7.0f );
        break;
    case 2:
        m_BubbleOffset( 0.0f, 3.5f, 0.0f );
        m_BubbleScale ( 7.5f,12.0f, 7.5f );
        break;
    }
}

//==============================================================================

void sensor::OnProvideInit( bitstream& BitStream )
{
    asset::OnProvideInit( BitStream );
    BitStream.WriteRangedS32( m_Size, 0, 2 );
    BitStream.WriteRangedF32( m_Shape.GetCurrentAnimState().GetFrameParametric(),
                              10, 0.0f, 1.0f );
}

//==============================================================================

void sensor::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    if( DirtyBits == 0xFFFFFFFF )
        DirtyBits =  0x80000001;

    if( BitStream.WriteFlag( DirtyBits & 0x00000001 ) )
    {
        f32 Frame  = m_Shape.GetCurrentAnimState().GetFrameParametric();
        Frame      = MINMAX( 0.0f, Frame, 1.0f );
        DirtyBits &= ~0x00000001;
        BitStream.WriteRangedF32( Frame, 10, 0.0f, 1.0f );
    }

    asset::OnProvideUpdate( BitStream, DirtyBits, Priority );
}

//==============================================================================

object::update_code sensor::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    if( BitStream.ReadFlag() )
    {
        f32 Frame;
        BitStream.ReadRangedF32( Frame, 10, 0.0f, 1.0f );
        m_Shape.GetCurrentAnimState().SetFrameParametric( Frame );
        if( (m_PowerOn) && (!m_Disabled) )
            m_Shape.Advance( SecInPast );
    }

    return( asset::OnAcceptUpdate( BitStream, SecInPast ) );
}

//==============================================================================
