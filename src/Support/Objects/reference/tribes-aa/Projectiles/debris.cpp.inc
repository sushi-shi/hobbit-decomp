//==============================================================================
//
//  Debris.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "arcing.hpp"

#include "Debris.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

static f32    DEBRIS_LIFE  =  20.0f;
static f32    DEBRIS_FADE  =   5.0f;

static radian DELTA_ROLL   = RADIAN(360);
static radian DELTA_PITCH  = RADIAN(360);
static radian DELTA_YAW    = RADIAN(360);

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   DebrisCreator;

obj_type_info   DebrisTypeInfo( object::TYPE_DEBRIS, 
                                "Debris", 
                                DebrisCreator, 
                                0 );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* DebrisCreator( void )
{
    return( (object*)(new debris) );
}

//==============================================================================

object::update_code debris::AdvanceLogic( f32 DeltaTime )
{
    // Update position of smoke emitter.
    if( m_SmokeEffect.IsCreated() )
    {
        m_SmokeEffect.SetVelocity( (m_WorldPos - m_SmokeEffect.GetPosition()) / DeltaTime );
        m_SmokeEffect.UpdateEffect( DeltaTime );
    }

    // Update rotations.
    if( m_State == STATE_FALLING )
    {
        m_Rot += (m_DeltaRot * DeltaTime);
        m_Rot.ModAngle();
    }

    // Age faster when sliding.
    if( (m_Type <= SMALL) && (m_State == STATE_SLIDING) )
    {
        // (Age will also be advanced in the arcing::AdvanceLogic call below.)
        m_Age += (DeltaTime * 3.0f);
    }

    // Age slower if settled.
    if( m_State == STATE_SETTLED )
    {
        m_Age -= (DeltaTime * 0.5f);
    }

    // Age faster when there are too many debris.
    s32 Count = ObjMgr.GetInstanceCount( object::TYPE_DEBRIS );
    if( (m_Type <= SMALL) && (Count > 20) )
    {
        m_Age += (Count - 20) * 0.25f * DeltaTime;
    }

    // Kill it when it gets too old.
    if( m_Age > DEBRIS_LIFE )
    {
        if ( m_SmokeEffect.IsCreated() )
            m_SmokeEffect.SetEnabled( FALSE );

        return DESTROY;
    }

    // return with call to base class AdvanceLogic
    return( arcing::AdvanceLogic( DeltaTime ) );
}

//==============================================================================

static f32 VelOffset = 100.0f;

xbool debris::Impact( const collider::collision& Collision )
{
    (void)Collision;

    f32 VelocitySquared = m_Velocity.LengthSquared();
    f32 Cap = MINMAX( 0.0f, (VelocitySquared - VelOffset) / VelOffset, 1.0f );

    // Reduce rotation.
    m_DeltaRot.Roll  *= Cap;
    m_DeltaRot.Pitch *= Cap;
    m_DeltaRot.Yaw   *= Cap;
/*
    // Clatter.
    vector3 Pos = Collision.Point;
    s32     snd = audio_Play( MapTypeToAudio[m_Type] + x_irand(0,2), &Pos );
    f32     vol = MIN( 1.0f, m_Velocity.LengthSquared() / Cap );
    audio_SetVolume( snd, vol );
*/
    // No smoking!
    if( m_SmokeEffect.IsCreated() )
        m_SmokeEffect.SetEnabled( FALSE );

    return( FALSE );
}

//==============================================================================

static s32 MapTypeToShape[] = 
{
    SHAPE_TYPE_DEBRIS_CHUNK_LARGE1,             // BIG,
    SHAPE_TYPE_DEBRIS_CHUNK_MEDIUM1,            // MEDIUM,
    SHAPE_TYPE_DEBRIS_CHUNK_SMALL1,             // SMALL,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT1,         // VEHICLE_GRAVCYCLE1,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT2,         // VEHICLE_GRAVCYCLE2,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT3,         // VEHICLE_GRAVCYCLE3,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT4,         // VEHICLE_GRAVCYCLE4,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT5,         // VEHICLE_GRAVCYCLE5,
    SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT6,         // VEHICLE_GRAVCYCLE6,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE1,          // VEHICLE_SHRIKE1,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE2,          // VEHICLE_SHRIKE2,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE3,          // VEHICLE_SHRIKE3,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE4,          // VEHICLE_SHRIKE4,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE5,          // VEHICLE_SHRIKE5,
    SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE6,          // VEHICLE_SHRIKE6,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF1,         // VEHICLE_TANK1,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF2,         // VEHICLE_TANK2,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF3,         // VEHICLE_TANK3,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF4,         // VEHICLE_TANK4,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF5,         // VEHICLE_TANK5,
    SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF6,         // VEHICLE_TANK6,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD1,          // VEHICLE_BOMBER1,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD2,          // VEHICLE_BOMBER2,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD3,          // VEHICLE_BOMBER3,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD4,          // VEHICLE_BOMBER4,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD5,          // VEHICLE_BOMBER5,
    SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD6,          // VEHICLE_BOMBER6,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC1,           // VEHICLE_TRANSPORT1,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC2,           // VEHICLE_TRANSPORT2,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC3,           // VEHICLE_TRANSPORT3,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC4,           // VEHICLE_TRANSPORT4,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC5,           // VEHICLE_TRANSPORT5,
    SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC6,           // VEHICLE_TRANSPORT6,
    SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO1,         // VEHICLE_MPB1,
    SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO2,         // VEHICLE_MPB2,
    SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO3,         // VEHICLE_MPB3
};

void debris::Initialize( const vector3&    WorldPos,
                         const vector3&    Velocity,
                               chunk_type  Type,
                               s32         SmokeEffect )
{
    // Set the collision attribute to ignore anything but buildings and terrain.
    m_CollisionAttr  = ATTR_SOLID_STATIC;

    // Set up rotation deltas.
    m_DeltaRot.Roll  = x_frand( -DELTA_ROLL,  +DELTA_ROLL  );
    m_DeltaRot.Pitch = x_frand( -DELTA_PITCH, +DELTA_PITCH );
    m_DeltaRot.Yaw   = x_frand( -DELTA_YAW,   +DELTA_YAW   );

    // Faster rotations for smaller particles.
    if( Type <= SMALL )
    {
        m_DeltaRot.Roll  *= 4.0f;
        m_DeltaRot.Pitch *= 4.0f;
        m_DeltaRot.Yaw   *= 4.0f;
    }

    // Set up initial rotation.
    m_Rot.Zero();

    // Create the smoke effect.
    if( SmokeEffect )
    {
        m_SmokeEffect.Create( &tgl.ParticlePool, 
                              &tgl.ParticleLibrary, 
                              SmokeEffect, 
                              WorldPos, 
                              vector3(0,0,1), 
                              NULL, 
                              NULL );
    }

    // Chunk type.
    m_Type = Type;

    // Figure out a shape type to use.
    s32 ShapeType = MapTypeToShape[Type];

    // There is some variety available for small bits.
    if( Type <= SMALL )
    {
        ShapeType += x_rand() & 0x01;
    }

    // set the shape instance
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( ShapeType ) );

    // Tweak the size and color for small bits.
    if( Type <= SMALL )
    {
        m_Shape.SetScale( vector3( x_frand( 0.5f, 1.0f ), 
                                   x_frand( 0.5f, 1.0f ), 
                                   x_frand( 0.5f, 1.0f ) ) );
        u8 Value = (u8)(x_rand() & 0x000000FF);
        m_Shape.SetColor( xcolor( Value, Value, Value ) );
    }

    // call the base initializer
    arcing::Initialize( WorldPos, Velocity, obj_mgr::NullID, obj_mgr::NullID, 0 );

    // Override default bounding box for tiny bits.
    if( ShapeType == SHAPE_TYPE_DEBRIS_CHUNK_SMALL1 )
        m_WorldBBox.Set( m_WorldPos, 0.0625f );

    // Tweak age for random fade out.
    if( m_Type <= SMALL )
        m_Age = x_frand( 0.0f, DEBRIS_LIFE * 0.5f );
    else
        m_Age = 0.0f;

    // override default gravity
    m_Gravity = 25.0f;
}

//==============================================================================

void debris::Render( void )
{
    // Setup shape ready for render prep
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_Rot );

    // Visible?
    if( !RenderPrep() )
        return;

    // Setup color.
    f32    Alpha;
    xcolor Color = m_Shape.GetColor();

    if( m_Age > DEBRIS_FADE )
    {
        Alpha = 1.0f - ((m_Age - DEBRIS_FADE) / (DEBRIS_LIFE - DEBRIS_FADE));
        if( Alpha <= 0.0f )
            return;
    }
    else
    {
        Alpha = 1.0f;
    }

    Color.A = (u8)(255 * Alpha);
    m_Shape.SetColor( Color );

    // Setup draw flags
    u32 DrawFlags = m_ShapeDrawFlags;
    if( Color.A != 255 )
        DrawFlags |= shape::DRAW_FLAG_PRIME_Z_BUFFER | shape::DRAW_FLAG_SORT;  // for correct fading

    // Lookup max damage texture + clut
    texture* DamageTexture;
    s32      DamageClutHandle ;
    tgl.Damage.GetTexture( 0, DamageTexture, DamageClutHandle );

    // Finally, draw it
    m_Shape.Draw( DrawFlags, DamageTexture, DamageClutHandle ); 
}

//==============================================================================

void debris::RenderParticles( void )
{
    // Render smoke trail.
    if( !m_Hidden ) 
    {
        // but only if visible
        if( !IsVisible( m_WorldBBox ) )
            return;

        if( m_SmokeEffect.IsCreated() )
            m_SmokeEffect.RenderEffect();
    }
}

//==============================================================================

void debris::OnAdd( void )
{
    m_Drag = 45.0f;
    arcing::OnAdd();
}

//==============================================================================
