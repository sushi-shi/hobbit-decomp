//==============================================================================
//
//  Mine.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Mine.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "Particles/ParticleEffect.hpp"
#include "ParticleObject.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "netlib/bitstream.hpp"
#include "GameMgr/GameMgr.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   MineCreator;

obj_type_info   MineTypeInfo( object::TYPE_MINE, 
                              "Mine", 
                              MineCreator, 
                              object::ATTR_ELFABLE |
                              object::ATTR_UNBIND_ORIGIN );

f32 MineRunFactor  =  0.60f;
f32 MineRiseFactor =  0.35f;
f32 MineDragSpeed  = 15.00f;

static bbox FlyingBBox( vector3( -0.05f,  0.0f, -0.05f ), 
                        vector3(  0.05f,  0.2f,  0.05f ) );

static bbox ArmedBBox ( vector3( -2.50f, -0.5f, -2.50f ), 
                        vector3(  2.50f,  2.5f,  2.50f ) );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* MineCreator( void )
{
    return( (object*)(new mine) );
}

//==============================================================================

mine::mine( void )
{
    m_CollisionAttr = object::ATTR_SOLID;
}

//==============================================================================

void mine::Initialize( const vector3&   Position,      
                       const vector3&   Direction,     
                       const vector3&   OriginVel,     
                             object::id OriginID,      
                             u32        TeamBits,      
                             f32        ExtraSpeed )
{
    // Use the ancestor (arcing) to do much of the work.
    vector3 Velocity = OriginVel + (Direction * ExtraSpeed);
    arcing::Initialize( Position, Velocity, OriginID, OriginID, TeamBits );

    // Take care of the stuff particular to the mine.

    m_WorldBBox( Position + FlyingBBox.Min,
                 Position + FlyingBBox.Max );

    m_State = FLYING;

    m_RunFactor  = MineRunFactor;
    m_RiseFactor = MineRiseFactor;
    m_DragSpeed  = MineDragSpeed;

    m_InvalidSurface = FALSE;

    m_SettleNormal( 0, 1, 0 );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_MINE ) );
    m_Shape.SetAnimByType( ANIM_TYPE_MINE_IDLE );
}

//==============================================================================

void mine::OnAdd( void )
{
    // Call base class
    arcing::OnAdd();

    // Play throw sound
    vector3 Pos = GetRenderPos();
    audio_Play(SFX_WEAPONS_GRENADE_THROW, &Pos) ;
}

//==============================================================================

void mine::OnAcceptInit( const bitstream& BitStream )
{
    // Do ancestor stuff.
    arcing::OnAcceptInit( BitStream );

    // Take care of the stuff particular to the mine.

    m_SettleNormal( 0, 1, 0 );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_MINE ) );

    s32 Int;
    BitStream.ReadRangedS32( Int, FLYING, GONE );    
    m_State = (state)Int;

    m_Shape.SetAnimByType( ANIM_TYPE_MINE_IDLE );

    // If drilling, we need age and settle normal.
    if( m_State == DRILLING )
    {
        BitStream.ReadF32( m_Age );
        BitStream.ReadVector( m_SettleNormal );
        m_Shape.SetAnimByType( ANIM_TYPE_MINE_DEPLOY );
        anim_state& DrillAnimState = m_Shape.GetCurrentAnimState();
        DrillAnimState.SetFrameParametric( m_Age );
    }

    // If armed, we need settle normal.
    if( m_State == ARMED )
    {
        quaternion Q;
        BitStream.ReadVector( m_SettleNormal );
        Q.Setup( vector3(0,1,0), m_SettleNormal );
        m_Shape.SetRot( Q.GetRotation() );
        m_Shape.SetAnimByType( ANIM_TYPE_MINE_DEPLOY );
        anim_state& DrillAnimState = m_Shape.GetCurrentAnimState();
        DrillAnimState.SetFrameParametric( 1.0f );
    }

    // Set the bounding box.
    if( m_State == ARMED )
    {
        m_WorldBBox( m_WorldPos + ArmedBBox.Min,
                     m_WorldPos + ArmedBBox.Max );
    }
    else
    {
        m_WorldBBox( m_WorldPos + FlyingBBox.Min,
                     m_WorldPos + FlyingBBox.Max );
    }

    m_RunFactor  = MineRunFactor;
    m_RiseFactor = MineRiseFactor;

    m_InvalidSurface = FALSE;
}

//==============================================================================

void mine::OnProvideInit( bitstream& BitStream )
{
    // Do ancestor stuff.
    arcing::OnProvideInit( BitStream );

    // Add our own stuff.
    BitStream.WriteRangedS32( m_State, FLYING, GONE );
    if( m_State == DRILLING )
    {
        BitStream.WriteF32( m_Age );
        BitStream.WriteVector( m_SettleNormal );
    }
    if( m_State == ARMED )
    {
        BitStream.WriteVector( m_SettleNormal );
    }
}

//==============================================================================

object::update_code mine::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    s32 Int;

    BitStream.ReadRangedS32( Int, FLYING, GONE );
    m_State = (state)Int;

    if( m_State == ARMED )
    {
        m_WorldBBox( m_WorldPos + ArmedBBox.Min,
                     m_WorldPos + ArmedBBox.Max );
    }
    else
    {
        m_WorldBBox( m_WorldPos + FlyingBBox.Min,
                     m_WorldPos + FlyingBBox.Max );
    }

    return( arcing::OnAcceptUpdate( BitStream, SecInPast ) );
}

//==============================================================================

void mine::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    if( DirtyBits & 0x01 )
    {
        BitStream.WriteRangedS32( m_State, FLYING, GONE );
        arcing::OnProvideUpdate( BitStream, DirtyBits, Priority );
    }
}

//==============================================================================

object::update_code mine::AdvanceLogic( f32 DeltaTime )
{
    object::update_code Result = arcing::AdvanceLogic( DeltaTime );

    if( arcing::m_State == STATE_FALLING )
        m_InvalidSurface = FALSE;

    // Should we switch from FLYING to SETTLING?
    if( (m_State == FLYING) && (arcing::m_State == STATE_SETTLED) )
    {
        m_State      = SETTLING;
        m_Age        = 0.0f;
        m_DirtyBits |= 0x01;
    }

    // Should we switch from SETTLING to DRILLING?
    if( (m_State == SETTLING) && (m_Age >= 1.0f) )
    {
        xbool Fizzle = m_InvalidSurface;

        // Any reason to fizzle this mine?
        //  - Another mine to close? 
        //  - Too many mines for this team?
        //  - Invalid surface?
        if( !Fizzle )
        {   
            s32     Count = 0;
            mine*   pMine;
            vector3 Span;

            ObjMgr.StartTypeLoop( object::TYPE_MINE );
            while( (pMine = (mine*)ObjMgr.GetNextInType()) )
            {
                if( pMine == this )
                    continue;

                if( (pMine->m_State != ARMED) && 
                    (pMine->m_State != DRILLING) )
                    continue;

                // Too many?
                if( pMine->m_TeamBits == m_TeamBits )
                {
                    Count++;
                    if( Count >= 50 )
                        break;
                }   
            
                // Too close to another?
                Span = m_WorldPos - pMine->m_WorldPos;
                if( Span.LengthSquared() < (6*6) )
                    break;
            }
            ObjMgr.EndTypeLoop();

            if( pMine )
                Fizzle = TRUE;
        }    

        if( Fizzle && tgl.ServerPresent )
        {
            SpawnExplosion( pain::WEAPON_MINE_FIZZLE,
                            m_WorldPos, 
                            m_SettleNormal,
                            m_OriginID,
                            m_ObjectHitID );

            m_State      = GONE;
            m_DirtyBits |= 0x01;
            return( DESTROY );
        }
                   
        m_AttrBits  |= ATTR_MINE;
        m_State      = DRILLING;
        m_Age        = 0.0f;
        m_DirtyBits |= 0x01;
        m_Shape.SetAnimByType( ANIM_TYPE_MINE_DEPLOY );
        if( !Fizzle )
            audio_Play( SFX_WEAPONS_MINE_DEPLOY, &m_WorldPos );        
    }

    // Should we switch from DRILLING to ARMED?
    if( (m_State == DRILLING) && (m_Age >= 0.8f) )
    {
        m_AttrBits  |= ATTR_DAMAGEABLE;
        m_AttrBits  |= ATTR_SOLID_DYNAMIC;
        m_State      = ARMED;
        m_DirtyBits |= 0x01;
        m_WorldBBox( m_WorldPos + ArmedBBox.Min,
                     m_WorldPos + ArmedBBox.Max );
        if( tgl.ServerPresent )
        {
            pGameLogic->ItemDeployed( m_ObjectID, m_OriginID );
        }
    }

    // Should we go BOOM?
    if( (m_State == TRIPPED) && (m_Age > 0.125f) )
    {
        if( tgl.ServerPresent )
        {
            SpawnExplosion( pain::WEAPON_MINE,
                            m_WorldPos, 
                            m_SettleNormal,
                            m_OriginID,
                            m_ObjectHitID );
        }

        m_State      = GONE;
        m_DirtyBits |= 0x01;
        Result       = DESTROY;
    }

    // Animate shape.
    m_Shape.Advance( DeltaTime );

    // Let the ancestor class handle it from here.
    return( Result );
}

//==============================================================================

f32 mine::GetPainRadius( void ) const
{
    return( 0.1f );
}

//==============================================================================

vector3 mine::GetPainPoint( void ) const
{
    return( m_WorldPos + (m_SettleNormal * 0.1f) );
}

//==============================================================================

static f32 s_MineThresholdDamage =  0.30f;
static f32 s_MineThresholdForce  = 45.00f;

void mine::OnPain( const pain& Pain )
{
    xbool Explode = FALSE;

    if( (Pain.DirectHit) && (Pain.MetalDamage > 0.0f) )
    {
        Explode = TRUE;
    }
    else
    {
        Explode = ( (Pain.PainType != pain::WEAPON_MINE) &&
                    (Pain.PainType != pain::WEAPON_MINE_FIZZLE) &&
                    (Pain.LineOfSight) &&
                    ((Pain.MetalDamage > s_MineThresholdDamage) || 
                     (Pain.Force       > s_MineThresholdForce )) );
    }

    if( Explode )
    {
        Trip();
        m_Age = 1.0f;
    }
}

//==============================================================================

void mine::DebugRender( void )
{
    draw_BBox ( m_WorldBBox, XCOLOR_RED );
    draw_Point( m_WorldPos,  XCOLOR_YELLOW );

    vector3 Normal( 0, m_State == SETTLING ? 3.0f : 1.0f, 0 );

    // Are we settling?
    if( m_State == SETTLING )
    {
        quaternion Q;
        Q.Setup( vector3(0,1,0), m_SettleNormal );
        Q = BlendToIdentity( Q, 1.0f - m_Age );
        Normal = Q * Normal;        
    }

    if( m_State > SETTLING )
    {
        Normal = m_SettleNormal;
    }

    draw_Line( m_WorldPos, m_WorldPos + Normal, XCOLOR_RED );
}

//==============================================================================

void mine::Render( void )
{
    // Setup shape orientation ready for render prep

    // Are we settling?
    if( m_State == DRILLING )
    {
        quaternion Q;

        Q.Setup( vector3(0,1,0), m_SettleNormal );
        Q = BlendToIdentity( Q, 1.0f - m_Age );

        m_Shape.SetRot( Q.GetRotation() );
    }

    m_Shape.SetPos( GetRenderPos() );

    // Visible?
    if( !RenderPrep() )
        return;

    // Draw it
    m_Shape.Draw( m_ShapeDrawFlags );

    //DebugRender();
}

//==============================================================================

void mine::Trip( void )
{
    if( m_State == ARMED )
    {
        m_Age   = 0.0f;
        m_State = TRIPPED;
        m_AttrBits &= ~ATTR_MINE;    
        m_AttrBits &= ~ATTR_DAMAGEABLE;
        m_AttrBits &= ~ATTR_SOLID_DYNAMIC;
    }
}

//==============================================================================

void mine::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    if( m_State == ARMED )
    {
        object* pObject = (object*)Collider.GetMovingObject();
        if( pObject )
        {
            u32 MovingObjAttrBits = pObject->GetAttrBits();

            // Trip if a player AND 
            //         player is not dead AND 
            //         player is not a bot on the mine's team.
            if( (MovingObjAttrBits & object::ATTR_PLAYER) &&
                !((player_object*)pObject)->IsDead() &&
                !( (pObject->GetType() == object::TYPE_BOT) && 
                   (pObject->GetTeamBits() & GetTeamBits())) )
            {
                Trip();
            }

            if( MovingObjAttrBits & object::ATTR_VEHICLE )
            {
                Trip();
            }
        }
    }

    if( Collider.GetType() == collider::RAY )
        m_Shape.ApplyCollision( Collider );
}

//==============================================================================

xbool mine::Impact( const collider::collision& Collision )
{   
    object* pObjectHit = (object*)Collision.pObjectHit;
    ASSERT( pObjectHit );

    if( !(pObjectHit->GetAttrBits() & ATTR_SOLID_STATIC) )
        m_InvalidSurface = TRUE;

    return( FALSE );
}

//==============================================================================

void mine::UnbindOriginID( object::id OriginID )
{
    if( m_OriginID.Slot == OriginID.Slot )
        m_OriginID = obj_mgr::NullID;
}

//==============================================================================
