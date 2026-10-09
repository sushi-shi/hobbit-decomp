//==============================================================================
//
//  Generator.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Generator.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "Debris.hpp"
#include "GameMgr\GameMgr.hpp"
#include "AudioMgr/Audio.hpp"
#include "NetLib\BitStream.hpp"
#include "ParticleObject.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Damage/Damage.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define DIRTY_SOUND     0x01

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   GeneratorCreator;

obj_type_info   GeneratorTypeInfo( object::TYPE_GENERATOR, 
                                   "Generator", 
                                   GeneratorCreator, 
                                   object::ATTR_SOLID_DYNAMIC | 
                                   object::ATTR_PERMANENT     |
                                   object::ATTR_LABELED       |
                                   object::ATTR_ASSET         | 
                                   object::ATTR_REPAIRABLE    | 
                                   object::ATTR_ELFABLE       |
                                   object::ATTR_DAMAGEABLE ); 

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* GeneratorCreator( void )
{
    return( (object*)(new generator) );
}

//==============================================================================

generator::generator( void )
{
    m_RechargeRate = 0.05f;
}

//==============================================================================

void generator::Initialize( const vector3& WorldPos,
                            const radian3& WorldRot,
                                  s32      Switch,  
                                  s32      Power, 
                            const xwchar*  pLabel )
{   
    asset::Initialize( WorldPos, WorldRot, Switch, Power, pLabel );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_ASSET_GENERATOR ) );
    ASSERT(m_Shape.GetShape()) ;
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );

    m_Collision.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_ASSET_GENERATOR_COLL ) );
    ASSERT(m_Collision.GetShape()) ;
    m_Collision.SetPos( m_WorldPos );
    m_Collision.SetRot( m_WorldRot );

    // Set final bbox.
    m_WorldBBox  = m_Shape.GetWorldBounds();
    m_WorldBBox += m_Collision.GetWorldBounds() ;

    ComputeZoneSet( m_ZoneSet, m_WorldBBox, TRUE );

    // Prepare for ambient audio.
    m_AmbientID = SFX_POWER_GENERATOR_HUMM_LOOP;

    // Set up the bubble effects values.
    m_BubbleOffset( 0.0f,  1.5f, -1.75f );
    m_BubbleScale ( 5.0f,  5.0f,  5.00f );
}

//==============================================================================

void generator::Render( void )
{
    // Visible?
    if( !RenderPrep() )
        return;

    // Draw generator shape.
    m_Shape.Draw( m_ShapeDrawFlags, m_DmgTexture, m_DmgClutHandle );

    if( g_DebugAsset.DebugRender )
        DebugRender();
}

//==============================================================================

void generator::PowerOff( void )
{
}

//==============================================================================

void generator::PowerOn( void )
{
}

//==============================================================================

void generator::Disabled( object::id OriginID )
{
    asset::Disabled( OriginID );
    GameMgr.PowerLoss( m_Power );

    if( !GameMgr.GetPower( m_Power ) )
    {
        if( tgl.ServerPresent )
        {
            audio_Play( SFX_POWER_BASE_OFF, &m_WorldPos );
            m_DirtyBits |= DIRTY_SOUND;
        }
    }
}

//==============================================================================

void generator::Destroyed( object::id OriginID )
{
    if( m_Explode )
    {
        SpawnExplosion( pain::EXPLOSION_GENERATOR,
                        m_WorldBBox.GetCenter(),
                        vector3(0,1,0),
                        OriginID );
    }

    asset::Destroyed( OriginID );
}

//==============================================================================

void generator::Enabled( object::id OriginID )
{
    m_PowerOn = TRUE;
    asset::Enabled( OriginID );

    if( !GameMgr.GetPower( m_Power ) )
    {
        if( tgl.ServerPresent )
        {
            audio_Play( SFX_POWER_BASE_ON, &m_WorldPos );
            m_DirtyBits |= DIRTY_SOUND;
        }
    }

    GameMgr.PowerGain( m_Power );
}

//==============================================================================

void generator::OnPain( const pain& Pain )
{
    asset::OnPain( Pain );
    if( (Pain.MetalDamage < 0.0f) && (m_Health >= m_EnableLevel) && !m_PowerOn )
    {
        Enabled( Pain.OriginID );
    }
}

//==============================================================================

object::update_code generator::OnAdvanceLogic( f32 DeltaTime )
{
    if( m_Health >= m_EnableLevel )
    {
        m_Shape.Advance( DeltaTime );
    }

    return( asset::OnAdvanceLogic( DeltaTime ) );
}

//==============================================================================

object::update_code generator::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    if( BitStream.ReadFlag() )
    {
        if( BitStream.ReadFlag() )
            audio_Play( SFX_POWER_BASE_ON,  &m_WorldPos );
        else
            audio_Play( SFX_POWER_BASE_OFF, &m_WorldPos );
    }

    return( asset::OnAcceptUpdate( BitStream, SecInPast ) );
}

//==============================================================================

void generator::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    if( DirtyBits == 0xFFFFFFFF )
        DirtyBits =  0x80000000;

    if( BitStream.WriteFlag( DirtyBits & DIRTY_SOUND ) )
    {
        BitStream.WriteFlag( GameMgr.GetPower( m_Power ) );
        DirtyBits &= ~DIRTY_SOUND;
    }

    asset::OnProvideUpdate( BitStream, DirtyBits, Priority );
}

//==============================================================================
