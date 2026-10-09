//==============================================================================
//
//  SatchelCharge.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "SatchelCharge.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS/Globals.hpp"
#include "ParticleObject.hpp"
#include "Objects/Player/PlayerObject.hpp"
#include "GameMgr/GameMgr.hpp"
#include "netlib/bitstream.hpp"


//==============================================================================
//  DEFINES
//==============================================================================

#define LAUNCH_SPEED            20.0f       // Launch speed
#define ARM_TIME                4           // Time it takes to arm
#define DETONATE_TIME           1           // Time it takes to detonate

#define THRESHOLD_DAMAGE         0.25f
#define THRESHOLD_FORCE         30.0f


#define DIRTY_BIT_STATE         (1<<0)
#define DIRTY_BIT_SURFACE_INFO  (1<<1)
#define STATE_TIME_MIN          0
#define STATE_TIME_MAX          4
#define STATE_TIME_BITS         6

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   SatchelChargeCreator;

obj_type_info   SatchelChargeTypeInfo( object::TYPE_SATCHEL_CHARGE, 
                                       "SatchelCharge", 
                                       SatchelChargeCreator, 
                                       object::ATTR_DAMAGEABLE );


f32 SatchelChargeRunFactor  =   0.6f;
f32 SatchelChargeRiseFactor =   0.35f;
f32 SatchelChargeDragSpeed  =  15.0f;


//==============================================================================
//  FUNCTIONS
//==============================================================================

object* SatchelChargeCreator( void )
{
    return( (object*)(new satchel_charge) );
}

//==============================================================================

void satchel_charge::Initialize( const vector3&   Position,      
                                 const vector3&   Direction,     
                                       object::id OriginID,      
                                       u32        TeamBits,      
                                       f32        ExtraSpeed /* = 0 */ )
{
    // Use the ancestor (arcing) to do much of the work.
    vector3 Velocity = (Direction * LAUNCH_SPEED) + (Direction * ExtraSpeed);
    arcing::Initialize( Position, Velocity, OriginID, OriginID, TeamBits );

    // Just collide with buildings, terrain, assets, and that damn vehicle pad!
    m_CollisionAttr = ATTR_SOLID_STATIC | ATTR_ASSET | ATTR_VPAD ;

    // Take care of the stuff particular to the satchel_charge.
    m_RunFactor  = SatchelChargeRunFactor;
    m_RiseFactor = SatchelChargeRiseFactor;
    m_DragSpeed  = SatchelChargeDragSpeed;

    // Setup state
    m_State         = STATE_MOVING ;        // Current state
    m_StateTime     = 0 ;                   // Current state time
    m_ExplosionType = EXPLOSION_BIG ;       // Type of explosion to do

    m_SurfaceObjectID = obj_mgr::NullID ;   // Object ID of surface attached to

    quaternion Rot ;                            // Rotation of surface satchel is on (world or local if on node)
    Rot.Setup(vector3(0,0,-1), vector3(0,1,0)) ;
    m_SurfaceRot = Rot.GetRotation() ;
    
    m_SurfacePos.Zero() ;                   // Position on surface (world or local if on node)
    m_SurfaceNode     = -1 ;                // Node of surface or -1 if none
    m_SurfaceCollID   = 0 ;                 // Surface coll id

    m_SettleNormal( 0, 1, 0 );

    // Setup instance
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_DEPLOY_PACK_SATCHEL_CHARGE ) );
    m_Shape.SetPos(m_WorldPos) ;
    m_Shape.SetRot(m_SurfaceRot) ;

    // Setup bbox
    m_WorldBBox = m_Shape.GetWorldBounds() ;
}


//==============================================================================

void satchel_charge::OnAdd( void )
{
    // Call base class
    arcing::OnAdd();

    // Play throw sound
    vector3 Pos = GetRenderPos();
    audio_Play(SFX_WEAPONS_GRENADE_THROW, &Pos) ;
}

//==============================================================================

void satchel_charge::OnProvideInit( bitstream& BitStream )
{
    // Do ancestor stuff.
    arcing::OnProvideInit( BitStream ) ;

    // State
    BitStream.WriteRangedU32((u32)m_State, STATE_START, STATE_END) ;
    BitStream.WriteRangedF32(m_StateTime, STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX) ;
    
    // Explosion type
    BitStream.WriteRangedU32((u32)m_ExplosionType, EXPLOSION_BIG, EXPLOSION_SMALL) ;

    // Surface info
    BitStream.WriteObjectID     (m_SurfaceObjectID) ;
    BitStream.WriteRangedRadian3(m_SurfaceRot, 8) ;
    BitStream.WriteVector       (m_SurfacePos) ;
    BitStream.WriteS32          (m_SurfaceNode, 8) ;
    BitStream.WriteS32          (m_SurfaceCollID, 8) ;
}

//==============================================================================

void satchel_charge::OnAcceptInit( const bitstream& BitStream )
{
    u32 TempU32 ;

    // Do ancestor stuff.
    arcing::OnAcceptInit( BitStream );

    // State
    BitStream.ReadRangedU32(TempU32, STATE_START, STATE_END) ;
    m_State = (state)TempU32 ;
    BitStream.ReadRangedF32(m_StateTime, STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX) ;

    // Explosion type
    BitStream.ReadRangedU32(TempU32, EXPLOSION_BIG, EXPLOSION_SMALL) ;
    m_ExplosionType = (explosion_type)TempU32 ;

    // Surface info
    BitStream.ReadObjectID      (m_SurfaceObjectID) ;
    BitStream.ReadRangedRadian3 (m_SurfaceRot, 8) ;
    BitStream.ReadVector        (m_SurfacePos) ;
    BitStream.ReadS32           (m_SurfaceNode, 8) ;
    BitStream.ReadS32           (m_SurfaceCollID, 8) ;

    // Take care of the stuff particular to the satchel_charge.
    m_RunFactor  = SatchelChargeRunFactor;
    m_RiseFactor = SatchelChargeRiseFactor;
    m_DragSpeed  = SatchelChargeDragSpeed;

    // Setup instance
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_DEPLOY_PACK_SATCHEL_CHARGE ) );
    m_Shape.SetPos(m_WorldPos) ;

    // Setup bbox
    m_WorldBBox = m_Shape.GetWorldBounds() ;
}

//==============================================================================

object::update_code satchel_charge::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    u32 TempU32 ;

    // State
    if (BitStream.ReadFlag())
    {
        // Explosion type
        if (BitStream.ReadFlag())
            m_ExplosionType = EXPLOSION_BIG ;
        else
            m_ExplosionType = EXPLOSION_SMALL ;

        // State
        BitStream.ReadRangedU32(TempU32, STATE_START, STATE_END) ;

        // Call setup state so audio effects are heard on client
        SetupState((state)TempU32) ;
        BitStream.ReadRangedF32(m_StateTime, STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX) ;
    }

    // Surface info
    if ( BitStream.ReadFlag())
    {
        // Surface info
        if ( BitStream.ReadFlag() )
        {
            BitStream.ReadObjectID      (m_SurfaceObjectID) ;
            BitStream.ReadRangedRadian3 (m_SurfaceRot, 8) ;
            BitStream.ReadVector        (m_SurfacePos) ;
            BitStream.ReadS32           (m_SurfaceNode, 8) ;
            BitStream.ReadS32           (m_SurfaceCollID, 8) ;
        }
        else
            m_SurfaceObjectID = obj_mgr::NullID ;
    }

    // Call base class
    return( arcing::OnAcceptUpdate( BitStream, SecInPast ) );
}

//==============================================================================

void satchel_charge::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    // State
    if ( BitStream.WriteFlag( (DirtyBits & DIRTY_BIT_STATE) != 0 ) )
    {
        DirtyBits &= ~DIRTY_BIT_STATE ;

        // Explosion type
        BitStream.WriteFlag(m_ExplosionType == EXPLOSION_BIG) ;

        // State
        BitStream.WriteRangedU32((u32)m_State, STATE_START, STATE_END) ;
        BitStream.WriteRangedF32(m_StateTime, STATE_TIME_BITS, STATE_TIME_MIN, STATE_TIME_MAX) ;
    }

    // Surface info
    if ( BitStream.WriteFlag( (DirtyBits & DIRTY_BIT_SURFACE_INFO) != 0) )
    {
        DirtyBits &= ~DIRTY_BIT_SURFACE_INFO ;

        // Surface info
        if ( BitStream.WriteFlag(m_SurfaceObjectID != obj_mgr::NullID))
        {
            BitStream.WriteObjectID     (m_SurfaceObjectID) ;
            BitStream.WriteRangedRadian3(m_SurfaceRot, 8) ;
            BitStream.WriteVector       (m_SurfacePos) ;
            BitStream.WriteS32          (m_SurfaceNode, 8) ;
            BitStream.WriteS32          (m_SurfaceCollID, 8) ;
        }
    }

    // Call base class
    arcing::OnProvideUpdate( BitStream, DirtyBits, Priority );
}

//==============================================================================

xbool satchel_charge::Detonate( explosion_type ExplosionType )
{
    // Only on server!
    if (!tgl.pServer)
        return FALSE ;

    // Must be in idle for large. Small and fizzle happend immediately
    if ((ExplosionType == EXPLOSION_BIG) && (m_State != STATE_ARMED))
        return FALSE ;

    // Disconnect the player from this satchel charge (if he's still there)
    if (m_OriginID != obj_mgr::NullID)
    {
        // Attached to a player?
        player_object* pPlayerObject = (player_object*)ObjMgr.GetObjectFromID(m_OriginID) ;
        if (        (pPlayerObject) && 
                    (       (pPlayerObject->GetType() == object::TYPE_PLAYER)
                        ||  (pPlayerObject->GetType() == object::TYPE_BOT) ) )
        {
            // Clear players reference to satchel charge
            pPlayerObject->DetachSatchelCharge(FALSE) ; // Do not me fizzle out!
        }
    }

    // Keep explosion type
    m_ExplosionType = ExplosionType ;

    // Start off the sequence
    SetupState(STATE_DETONATE) ;

    // Trigger event if the satchel is detonated
    if( ExplosionType == satchel_charge::EXPLOSION_BIG )
        pGameLogic->SatchelDetonated( m_OriginID );

    // Flag it's been triggered
    return TRUE ;
}

//==============================================================================

void satchel_charge::SetOriginID( object::id  OriginID )
{
    // Begin with base class call
    arcing::SetOriginID(OriginID) ;

    // Only on server
    if (!tgl.ServerPresent)
        return ;

    // If detaching from player, fizzle out
    if (OriginID == obj_mgr::NullID)
        Detonate(EXPLOSION_FIZZLE) ;
}

//==============================================================================

f32 satchel_charge::GetPainRadius( void ) const
{
    return( 0.1f );
}

//==============================================================================

void satchel_charge::OnPain( const pain& Pain )
{
    // Try detontate?
    if( Pain.LineOfSight && ((Pain.MetalDamage > THRESHOLD_DAMAGE) ||
                             (Pain.Force       > THRESHOLD_FORCE )) )
    {
        // Only do small explosion.
        Detonate( EXPLOSION_SMALL );
    }
}

//==============================================================================

xbool satchel_charge::Impact( const collider::collision& Collision )
{   
    // Play the TINK sound.
    vector3 Pos        = Collision.Point;
    object* pObjectHit = (object*)Collision.pObjectHit;
    ASSERT( pObjectHit );

    s32     HSound;
    switch( pObjectHit->GetType() )
    {
        case TYPE_BUILDING:
            HSound = audio_Play( SFX_WEAPONS_MORTARSHELL_IMPACT_HARD, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;

        default:
            HSound = audio_Play( SFX_WEAPONS_MORTARSHELL_IMPACT_SOFT, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;
    }

    // Set rotation from collision normal
    quaternion Rot;
    Rot.Setup( vector3(0,0,-1), Collision.Plane.Normal );
    m_SurfaceRot = Rot.GetRotation();

    return( FALSE );
}

//==============================================================================

void satchel_charge::SetupState( state State )
{
    // Already in this state?
    if (State == m_State)
        return ;

    // Put into new state and reset state time
    m_State     = State ;
    m_StateTime = 0 ;

    // Flag dirty bits
    m_DirtyBits |= DIRTY_BIT_STATE ;

    // Special case setups (start sounds, animating textures etc)
    switch(State)
    {
        case STATE_MOVING:
            break ;
        case STATE_ARMING:
            audio_Play( SFX_PACK_SATCHEL_ACTIVATE, &m_WorldPos );
            break ;
        case STATE_ARMED:
            pGameLogic->ItemDeployed( m_ObjectID, m_OriginID );
            break ;
        case STATE_DETONATE:
            // If it's the big explosion, make the detonate "beep beep"
            if (m_ExplosionType == EXPLOSION_BIG)
                audio_Play( SFX_PACK_SATCHEL_DETONATE, &m_WorldPos );
            break ;
    }
}

//==============================================================================

object::update_code satchel_charge::AdvanceLogic( f32 DeltaTime )
{
    // Keep for later
    vector3 OldPos = m_WorldPos ;

    // Call base class
    object::update_code RetCode = arcing::AdvanceLogic( DeltaTime );

    //======================================================================
    // Check for sticking to an object?
    //======================================================================
    if (arcing::m_State != STATE_SETTLED)
    {
        collider            Collider;
        collider::collision Collision; 

        // Get movement direction of satchel
        vector3 MoveDir    = m_WorldPos - OldPos ;
        f32     MoveLength = MoveDir.LengthSquared() ;
        
        // Moving far enough?
        if (MoveLength >= collider::MIN_RAY_DIST_SQR)
        {
            // Normalize direction
            MoveDir *= 1.0f / MoveLength ;

            // Start a few meters back
            OldPos = m_WorldPos - (6.0f * MoveDir) ;

            // Cast a ray from current pos to old pos to make sure old pos is valid
            Collider.RaySetup   ( this, m_WorldPos, OldPos ) ;
            ObjMgr.Collide      ( ATTR_SOLID_STATIC, Collider ) ;
            Collider.GetCollision( Collision );
            if (Collision.Collided)
                OldPos = Collision.Point ;

            // Cast a ray from a few meters back to current position
            MoveDir    = m_WorldPos - OldPos ;
            MoveLength = MoveDir.LengthSquared() ;
            if (MoveLength >= collider::MIN_RAY_DIST_SQR)
            {
                Collider.RaySetup   ( this, OldPos, m_WorldPos ) ;
                ObjMgr.Collide      ( ATTR_VEHICLE | ATTR_PLAYER, Collider );
                Collider.GetCollision( Collision );

                // Hit a node?
                if ((Collision.Collided) && (Collision.NodeIndex != -1))
                {
                    // Get object that was hit
                    object* pObjectHit  = (object*)Collision.pObjectHit ;
                    ASSERT(pObjectHit) ;

                    // Do not stick to origin player
                    if (pObjectHit->GetObjectID() != m_OriginID)
                    {
                        // Put at impact position
                        vector3 DeltaPos = Collision.Point - m_WorldPos ;
                        m_WorldPos += DeltaPos ;
                        m_WorldBBox.Translate(DeltaPos) ;

                        // Only stick on the server
                        if (tgl.ServerPresent)
                        {
                            // Does this object support node collision?
                            matrix4* L2W = pObjectHit->GetNodeL2W(Collision.ID, Collision.NodeIndex) ;
                            if (L2W)
                            {
                                // Calculate node world to local matrix
                                matrix4 W2L = *L2W ;
                                W2L.InvertSRT() ;

                                // Record collision in node local space
                                m_DirtyBits |= DIRTY_BIT_SURFACE_INFO ;
                                m_SurfaceObjectID = pObjectHit->GetObjectID() ;

                                // Get surface rotation in local space
                                quaternion qRot ;
                                qRot.Setup(vector3(0,0,-1), Collision.Plane.Normal) ;
                                radian3 WorldRot = qRot.GetRotation() ;
                                radian3 LocalRot = (W2L * matrix4(WorldRot)).GetRotation() ;
                                m_SurfaceRot = LocalRot ;

                                // Get surface pos in local space
                                m_SurfacePos      = W2L.Transform(Collision.Point) ;

                                // Get surface info...
                                m_SurfaceNode     = Collision.NodeIndex ;
                                ASSERT(Collision.NodeIndex <= 127) ;
                                m_SurfaceCollID   = Collision.ID ;

                                // Stick to surface
                                m_DirtyBits |= DIRTY_BIT_STATE ;
                                arcing::m_State = STATE_SETTLED ;
                                m_SettleNormal = Collision.Plane.Normal ;
                                m_Velocity.Zero() ;
                            }
                        }
                    }
                }
            }
        }
    }

    //======================================================================
    // Stuck to an object?
    //======================================================================
    if (m_SurfaceObjectID != obj_mgr::NullID)
    {
        // If the object has gone or is dead, don't move with it
        object* pObject = ObjMgr.GetObjectFromID(m_SurfaceObjectID) ;
        if ((!pObject) || (pObject->GetHealth() == 0))
            pObject = NULL ;

        // Still connected?
        if (pObject)
        {
            // Orientate to connected node if possible
            matrix4* NodeL2W = pObject->GetNodeL2W(m_SurfaceCollID, m_SurfaceNode) ;
            if (NodeL2W)
            {
                // Calc world position 
                vector3 WorldPos = NodeL2W->Transform(m_SurfacePos) ;

                // Move the satchel
                vector3 DeltaPos = WorldPos - m_WorldPos ;
                m_WorldPos  += DeltaPos ;
                m_WorldBBox.Translate(DeltaPos) ;

                // Calc world rot
                radian3 WorldRot = ((*NodeL2W) * matrix4(m_SurfaceRot)).GetRotation() ;

                // Update shape
                m_Shape.SetRot( WorldRot ) ;

                // Make sure satchel is updated
                RetCode = UPDATE ;
            }
            else
            {
                // Fizzle out
                pObject = NULL ;
            }
        }

        // If the object has gone on the server then fizzle out the satchel
        if ((tgl.ServerPresent) && (!pObject))
        {
            // Clear connection to object
            m_DirtyBits |= DIRTY_BIT_SURFACE_INFO ;
            m_SurfaceObjectID = obj_mgr::NullID ;

            // Fizzle out
            Detonate(EXPLOSION_FIZZLE) ;
        }
    }
    else
        m_Shape.SetRot( m_SurfaceRot ) ;

    //======================================================================
    // Update state logic
    //======================================================================

    // Update state time
    m_StateTime += DeltaTime ;
    if (m_StateTime > STATE_TIME_MAX)
        m_StateTime = STATE_TIME_MAX ;

    // Set position
    m_Shape.SetPos( m_WorldPos );

    // Advance the instance so the textures animate
    m_Shape.Advance(DeltaTime) ;

    // Advance state
    switch(m_State)
    {
        case STATE_MOVING:

            // Set to green light
            m_Shape.SetTextureAnimFPS(0.0f) ;
            m_Shape.SetTextureFrame(1) ;

            // Done moving?
            if (arcing::m_State == STATE_SETTLED)
                SetupState(STATE_ARMING) ;
            break ;

        case STATE_ARMING:

            // Set to green light
            m_Shape.SetTextureAnimFPS(0.0f) ;
            m_Shape.SetTextureFrame(1) ;

            // Finished arming?
            if (m_StateTime >= ARM_TIME)
                SetupState(STATE_ARMED) ;
            break ;

        case STATE_ARMED:

            // Flash green/red
            m_Shape.SetTextureAnimFPS(10.0f) ;
            m_Shape.SetTexturePlaybackType(material::PLAYBACK_TYPE_LOOP) ;
            break ;

        case STATE_DETONATE:

            // Set to red light
            m_Shape.SetTextureAnimFPS(0.0f) ;
            m_Shape.SetTextureFrame(0) ;

            // Explode? (only on server!)
            if (tgl.ServerPresent)
            {
                // Which type?
                switch(m_ExplosionType)
                {
                    // Would you like it big?
                    case EXPLOSION_BIG:
                        if (m_StateTime >= DETONATE_TIME)
                        {
                            pGameLogic->ItemDestroyed( m_ObjectID, obj_mgr::NullID );

                            SpawnExplosion( pain::WEAPON_SATCHEL,
                                            m_WorldPos, 
                                            m_SettleNormal,
                                            m_OriginID,
                                            m_ObjectHitID );

                            // Kill object
                            RetCode = DESTROY ;
                        }
                        break ;

                    // Would you like it small?
                    case EXPLOSION_SMALL:
                        {
                            pGameLogic->ItemDestroyed( m_ObjectID, obj_mgr::NullID );

                            SpawnExplosion( pain::WEAPON_SATCHEL_SMALL,
                                            m_WorldPos, 
                                            m_SettleNormal,
                                            m_OriginID,
                                            m_ObjectHitID );

                            // Kill object
                            RetCode = DESTROY ;
                        }
                        break ;

                    // Or would you like a fizzle?
                    case EXPLOSION_FIZZLE:
                        {
                            pGameLogic->ItemDestroyed( m_ObjectID, obj_mgr::NullID );

                            SpawnExplosion( pain::WEAPON_MINE_FIZZLE,
                                            m_WorldPos, 
                                            m_SettleNormal,
                                            m_OriginID,
                                            m_ObjectHitID );

                            // Kill object
                            RetCode = DESTROY ;
                        }
                        break ;
                }
            }
            break ;
    }

    // Let the ancestor class handle it from here.
    return( RetCode );
}

//==============================================================================

void satchel_charge::Render( void )
{
    // Visible?
    if (!RenderPrep())
        return ;

    // Setup fog color for this view and draw!
    m_Shape.Draw( m_ShapeDrawFlags );
}

//==============================================================================
