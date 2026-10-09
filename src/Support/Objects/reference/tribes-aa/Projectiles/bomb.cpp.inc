//==============================================================================
//
//  Bomb.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Bomb.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include "ParticleObject.hpp"
#include "Objects\Player\PlayerObject.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BombCreator;

obj_type_info   BombTypeInfo( object::TYPE_BOMB, 
                              "Bomb", 
                              BombCreator, 
                              0 );

f32 BombRunFactor  =   0.2f;
f32 BombRiseFactor =   0.1f;
f32 BombDragSpeed  =  45.0f;
f32 BombArmTime    =   2.0f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BombCreator( void )
{
    return( (object*)(new bomb) );
}

//==============================================================================

void bomb::Setup( void )
{
    m_HasExploded = FALSE;

    // Take care of the stuff particular to the bomb.

    m_RunFactor  = BombRunFactor;
    m_RiseFactor = BombRiseFactor;
    m_DragSpeed  = BombDragSpeed;
    m_SoundID    = SFX_VEHICLE_THUNDERSWORD_BOMB_DROP_LOOP;

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_BOMB ) );
}

//==============================================================================

void bomb::Initialize( const vector3&   Position,      
                             object::id OriginID,
                             object::id ExcludeID,
                             u32        TeamBits )
{
    arcing::Initialize( Position, vector3(0,-10.0f,0), OriginID, ExcludeID, TeamBits );
    Setup();
}

//==============================================================================

void bomb::OnAcceptInit( const bitstream& BitStream )
{
    arcing::OnAcceptInit( BitStream );
    Setup();
//  m_BlendTimer *= 4.0f;
}

//==============================================================================

xbool bomb::Impact( const collider::collision& Collision )
{   
    if( tgl.ServerPresent )
    {
        arcing::Impact( Collision );
        SpawnExplosion( pain::WEAPON_BOMBER_BOMB,
                        GetRenderPos(), 
                        Collision.Plane.Normal,
                        m_ScoreID, // <<---------- Not the OriginID
                        m_ObjectHitID );

        m_HasExploded = TRUE;
        return( TRUE );
    }

    return( FALSE );
}

//==============================================================================

object::update_code bomb::AdvanceLogic( f32 DeltaTime )
{
    object::update_code RetCode = arcing::AdvanceLogic( DeltaTime );

    // Destroy self.
    if( m_HasExploded )
    {
        return( DESTROY );
    }

    // Let the ancestor class handle it from here.
    return( RetCode );
}

//==============================================================================

void bomb::Render( void )
{
    // Exploded?
    if( m_HasExploded )
        return;

    // Setup shape ready for render prep
    m_Shape.SetPos( GetRenderPos() );
    m_Shape.SetRot( radian3( m_ObjectID.Slot, 
                             m_ObjectID.Slot + m_Age * PI * 0.2f, 
                             m_ObjectID.Slot + m_Age * PI ) );
    
    // Visible?
    if( !RenderPrep() )
        return;

    // Draw it
    m_Shape.Draw( m_ShapeDrawFlags );
}

//==============================================================================
