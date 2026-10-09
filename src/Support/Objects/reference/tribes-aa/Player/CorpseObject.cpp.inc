//==============================================================================
//
//  CorpseObject.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "CorpseObject.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib/BitStream.hpp"
#include "AADS/Globals.hpp" 
#include "LabelSets/Tribes2Types.hpp"

//==============================================================================
// DEFINES
//==============================================================================

// Misc                  
#define MAX_ACTIVE_CORPSES          8       // Max corpses that can appear in the world!

// State defines
#define CORPSE_MOVE_STUCK_TIME      8       // Max movement time before going to idle state
#define CORPSE_IDLE_TIME            30      // Total time (secs) dead on the floor before fading out
#define CORPSE_FADE_OUT_TIME        8       // Time (secs) for alpha fade
                  
// Physics defines
#define MOVING_EPSILON				0.001f			        // Velocity must be bigger than this to move the player
#define MAX_STEP_HEIGHT             1.0f                    // Max height of step that a player can get over
#define VERTICAL_STEP_DOT           0.173f   // 80
#define MIN_FACE_DISTANCE           0.01f
#define TRACTION_DISTANCE           0.03f
#define NORMAL_ELASTICITY           0.01f
#define MOVE_RETRY_COUNT            4   //5
#define FALLING_THRESHOLD           -10
#define JUMP_SKIP_CONTACTS_MAX      (8.0f / 30.0f)

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   CorpseObjectCreator;

obj_type_info   CorpseObjectTypeInfo( object::TYPE_CORPSE, 
                                      "CorpseObject", 
                                      CorpseObjectCreator, 
                                      0 ) ;
                               
//==============================================================================
//  GLOBAL FUNCTIONS
//==============================================================================

object* CorpseObjectCreator( void )
{
    return( (object*)(new corpse_object) );
}

//==============================================================================
//  STATIC CLASS DATA
//==============================================================================

s32  corpse_object::s_ActiveCount = 0;     // Current # of active corpses      

//==============================================================================
//  INITIALIZATION FUNCTIONS
//==============================================================================

// Constructor
corpse_object::corpse_object( void )
{
}

//==============================================================================

// Destructor
corpse_object::~corpse_object()
{
}

//==============================================================================

void corpse_object::Initialize( const player_object& Player )
{
    anim* pAnim ;

    // Setup movement vars
    m_WorldPos  = Player.m_DrawPos ;
    m_Vel       = Player.m_Vel ;
    m_Rot       = radian3(0.0f, Player.m_DrawRot.Yaw, 0.0f) ;  // Just keep yaw rotation
    m_GroundRot = Player.m_GroundRot ;
    m_AnimDeltaPos.Zero() ;
    m_OnSurface = FALSE ;

    // Make sure vels are not crazy
    ASSERT(ABS(m_Vel.X) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Y) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Z) < 1000.0f) ;

    // Setup character vars
    m_IsDeadBot = (Player.GetType() == TYPE_BOT) ;
    m_CharType  = Player.m_CharacterType ;
    m_ArmorType = Player.m_ArmorType ;

    // Lookup move info
    m_MoveInfo = Player.m_MoveInfo ;

    // Grab default bbox
    m_WorldBBox = m_MoveInfo->BOUNDING_BOX ;

    // If it's a bot, setup the bbox like bots do...
    if (m_IsDeadBot)
    {
        m_WorldBBox.Min.X = m_WorldBBox.Min.Z = -0.5f ;
        m_WorldBBox.Max.X = m_WorldBBox.Max.Z = 0.5f ;
    }

    // Put into world
    m_WorldBBox.Translate(m_WorldPos) ;

    // Set player shape and anim
    m_PlayerInstance = Player.m_PlayerInstance ;

    // Remove root xz only
    m_PlayerInstance.SetRemoveRootNodePosX(TRUE) ;
    m_PlayerInstance.SetRemoveRootNodePosY(FALSE) ;
    m_PlayerInstance.SetRemoveRootNodePosZ(TRUE) ;
    
    // Remove masked + additive anims otherwise only the feet will animate!
    m_PlayerInstance.DeleteAllAdditiveAnims() ;
    m_PlayerInstance.DeleteAllMaskedAnims() ;

    // Keep color
    m_Color = XCOLOR_WHITE ;

    // Setup state
    SetupState(CORPSE_STATE_MOVE, FALSE) ;

    // Reset age
    m_Age = 0.0f ;

    // Make sure animation is running!
    // (when changing teams the anim speed can be 0 leaving players stood up!)
    m_PlayerInstance.GetCurrentAnimState().SetSpeed(1) ;
    m_PlayerInstance.GetBlendAnimState().SetSpeed(1) ;

    // Is current anim a death anim?
    pAnim = m_PlayerInstance.GetCurrentAnimState().GetAnim() ;
    if ((pAnim) && (pAnim->GetIndex() == Player.m_DeathAnimIndex))
        return ;

    // Are we blending to a death anim?
    pAnim = m_PlayerInstance.GetBlendAnimState().GetAnim() ;
    if ((pAnim) && (pAnim->GetIndex() == Player.m_DeathAnimIndex))
        return ;

    // Okay - no death anim was set 
    // (this can happen when changing teams due to the player re-spawning immediately)
    if (Player.m_DeathAnimIndex != -1)
        m_PlayerInstance.SetAnimByIndex(Player.m_DeathAnimIndex, 0.1f) ;
}

//==============================================================================

// Keep number of corpses lying around low
void corpse_object::OnAdd( void )
{
    // Update active count
    // Too many active?
    if (++s_ActiveCount > MAX_ACTIVE_CORPSES)
    {
        // Find oldest active corspe
        f32             OldestAge     = 0 ;
        corpse_object*  pOldestCorpse = NULL ;

        // Search for oldest active corpse
        ObjMgr.StartTypeLoop( object::TYPE_CORPSE );
        corpse_object* pCorpse ;
        while( (pCorpse = (corpse_object*)ObjMgr.GetNextInType()) )
        {
            // Is this the oldest?
            if ((pCorpse->m_State == CORPSE_STATE_IDLE) && (pCorpse->m_Age >= OldestAge))
            {
                // Keep record
                OldestAge     = pCorpse->m_Age ;
                pOldestCorpse = pCorpse ;
            }
        }
        ObjMgr.EndTypeLoop();

        // Make oldest corpse fade away
        if (pOldestCorpse)
            pOldestCorpse->SetupState(CORPSE_STATE_FADE_OUT) ;
    }
}

//==============================================================================

object::update_code corpse_object::OnAdvanceLogic( f32 DeltaTime )
{
    // Adance state
    AdvanceState(DeltaTime) ;

    // Update age
    m_Age += DeltaTime ;

    // Kill?
    if( (m_State == CORPSE_STATE_FADE_OUT) && (m_StateTime >= CORPSE_FADE_OUT_TIME) )
        return DESTROY;
    else
        return UPDATE;
}

//==============================================================================

//==============================================================================
//  RENDER FUNCTIONS
//==============================================================================

void corpse_object::Render( void )
{
    // Don't draw if not in active view for now...
	s32 InView = IsVisible(m_PlayerInstance.GetWorldBounds()) ;
    if (!InView)
        return ;

    // Get colors
    xcolor FogColor = tgl.Fog.ComputeFog(m_WorldPos) ;
    xcolor Color    = XCOLOR_WHITE ;

    // Fading out?
    if (m_State == CORPSE_STATE_FADE_OUT)
        Color.A = (u8)(255.0f * (1.0f - MIN(1.0f, m_StateTime / CORPSE_FADE_OUT_TIME))) ;

	// Draw the instance
    if (Color.A != 0)
    {
        ASSERT(m_PlayerInstance.GetShape()) ;

        // Lookup damage texture + clut for zero health
        texture* DamageTexture ;
        s32      DamageClutHandle ;
        tgl.Damage.GetTexture( 0, DamageTexture, DamageClutHandle ) ;
        
        // Setup draw flags
        u32 DrawFlags = shape::DRAW_FLAG_FOG | shape::DRAW_FLAG_TURN_ON_ANIM_MAT_DETAIL ;
        if (InView == view::VISIBLE_PARTIAL)
            DrawFlags |= shape::DRAW_FLAG_CLIP ;

        // Prime Z buffer if transparent
        if (Color.A != 255)
            DrawFlags |= shape::DRAW_FLAG_PRIME_Z_BUFFER | shape::DRAW_FLAG_SORT ;
        
        m_PlayerInstance.SetColor(Color) ;
        m_PlayerInstance.SetFogColor(FogColor) ;
        m_PlayerInstance.Draw( DrawFlags, DamageTexture, DamageClutHandle ) ;
    }
}

//==============================================================================

void corpse_object::DebugRender( void )
{
}


//==============================================================================
//  STATE FUNCTIONS
//==============================================================================

void corpse_object::AdvanceState( f32 DeltaTime )
{
	// Update state time
    m_StateTime += DeltaTime ;

	// Call advance function
	switch(m_State)
	{
        case CORPSE_STATE_MOVE:		    Advance_MOVE 			(DeltaTime) ;    break ;
        case CORPSE_STATE_IDLE:		    Advance_IDLE 			(DeltaTime) ;    break ;
        case CORPSE_STATE_FADE_OUT:	    Advance_FADE_OUT		(DeltaTime) ;    break ;
	}
}

//==============================================================================

void corpse_object::SetupState( corpse_state State, xbool SkipIfAlreadyInState /* = TRUE */)
{
    // State changed?
    if ((m_State != State) || (!SkipIfAlreadyInState))
    {
		// Record new state now, before calling setup state, so that the setup state
        // function can put the player in another state if need be!
        // (eg. setup land - damages player, so setup death is needed)
        m_State     = State ;

        // Setup logic
        switch(State)
        {
            case CORPSE_STATE_MOVE:		    Setup_MOVE			() ;    break ;
            case CORPSE_STATE_IDLE:		    Setup_IDLE			() ;    break ;
            case CORPSE_STATE_FADE_OUT:		Setup_FADE_OUT		() ;    break ;
        }

		// Reset state time
		m_StateTime = 0.0f ;
    }
}

//==============================================================================

void corpse_object::ApplyPhysics( f32 DeltaTime )
{
    collider                Collider ;
	collider::collision     Collision ;
    vector3                 Move ;
    bbox                    CollBBox ;

    //f32     mWaterCoverage = 0 ;
    f32     mDrag=0;
    f32     mBuoyancy=0 ;
    f32     mGravityMod=0 ;

    // Desired move direction & speed
    vector3 moveVec;
    f32     moveSpeed;
    moveVec.Set(0,0,0);
    moveSpeed = 0;

    // Acceleration due to gravity
    vector3 acc(0, player_object::GetCommonInfo().GRAVITY * DeltaTime, 0) ;

    // Determin ground contact normal. Only look for contacts if
    // we can move.
    #define TRACTION_DISTANCE   0.03f

    // Start collision slightly above ground
    CollBBox = m_WorldBBox ;
    CollBBox.Min.Y += TRACTION_DISTANCE ;
    CollBBox.Max.Y += TRACTION_DISTANCE ;

    // Call extrusion collision to find the ground
    Move = vector3(0.0f, -TRACTION_DISTANCE * 2.0f, 0.0f ) ; // Check below player
    Collider.ExtSetup( this, CollBBox, Move ) ;
    ObjMgr.Collide(object::ATTR_PERMANENT, Collider ) ;

    // Get results
    Collider.GetCollision( Collision );

    // Make sure vels are not crazy
    ASSERT(ABS(m_Vel.X) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Y) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Z) < 1000.0f) ;

    // Hit something?
    if (Collision.Collided)
    {
        // On run surface?
        m_OnSurface = Collision.Plane.Normal.Y > x_cos(m_MoveInfo->RUN_SURFACE_ANGLE) ;

        // Skip players when updating ground rot!
        object* pObject = (object*)Collision.pObjectHit ;
        ASSERT(pObject) ;
        if (!(pObject->GetAttrBits() & object::ATTR_PLAYER))
        {
            // Update ground rot
            vector3     Up       = vector3(0,1,0) ;
            vector3     Axis     = v3_Cross(Up, Collision.Plane.Normal) ;
            
            if( !Axis.Normalize() )
                Axis = vector3(1,0,0);

            f32         CosTheta = v3_Dot(Up, Collision.Plane.Normal) ;
            radian      Theta    = x_acos(CosTheta) ;
            quaternion  GroundRot(Axis, Theta) ;
            m_GroundRot = Blend( m_GroundRot, GroundRot, 0.2f ) ;
            m_GroundRot.Normalize() ;
        }

        // Remove acc into contact surface (should only be gravity)
        // Clear out floating point acc errors, this will allow
        // the player to "rest" on the ground.
        f32 vd = -v3_Dot(acc, Collision.Plane.Normal);
        if (vd > 0)
        {
            vector3 dv = Collision.Plane.Normal * (vd + 0.002f);
            acc += dv;
            if (acc.Length() < 0.0001f)
                acc.Set(0,0,0);
        }
        
        // Convert to acceleration
        vector3 runAcc   = -(m_Vel + acc) ;
        f32     runSpeed = runAcc.Length();

        // Clamp acceleratin, player also accelerates faster when
        // in his hard landing recover state.
        f32 maxAcc = (m_MoveInfo->RUN_FORCE / m_MoveInfo->MASS) * DeltaTime ;
        if (runSpeed > maxAcc)
            runAcc *= maxAcc / runSpeed;
        acc += runAcc ;
    }
    else
        m_OnSurface = FALSE ;

    // Adjust velocity with all the move & gravity acceleration
    m_Vel += acc;

    // apply horizontal air resistance
    f32 hvel = x_sqrt((m_Vel.X * m_Vel.X) + (m_Vel.Z * m_Vel.Z));
    if(hvel > m_MoveInfo->HORIZ_RESIST_SPEED)
    {
        f32 speedCap = hvel;
        if(speedCap > m_MoveInfo->HORIZ_MAX_SPEED)
            speedCap = m_MoveInfo->HORIZ_MAX_SPEED;
        speedCap -= m_MoveInfo->HORIZ_RESIST_FACTOR * DeltaTime * (speedCap - m_MoveInfo->HORIZ_RESIST_SPEED);
        f32 scale = speedCap / hvel;
        m_Vel.X *= scale;
        m_Vel.Z *= scale;
    }
    if(m_Vel.Y > m_MoveInfo->UP_RESIST_SPEED)
    {
        if(m_Vel.Y > m_MoveInfo->UP_MAX_SPEED)
            m_Vel.Y = m_MoveInfo->UP_MAX_SPEED;
        m_Vel.Y -= m_MoveInfo->UP_RESIST_FACTOR * DeltaTime * (m_Vel.Y - m_MoveInfo->UP_RESIST_SPEED);
    }
   
    // Container buoyancy & drag
    if (mBuoyancy != 0) 
    {     // Applying buoyancy when standing still causing some jitters- 
        if (mBuoyancy > 1.0 || !m_Vel.LengthSquared() == 0 || !m_OnSurface)
            m_Vel.Y -= mBuoyancy * player_object::GetCommonInfo().GRAVITY * mGravityMod * DeltaTime;
    }
    m_Vel -= m_Vel * mDrag * DeltaTime;

    // Make sure vels are not crazy
    ASSERT(ABS(m_Vel.X) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Y) < 1000.0f) ;
    ASSERT(ABS(m_Vel.Z) < 1000.0f) ;
}

//==============================================================================

void corpse_object::DoMovement( f32 DeltaTime )
{
    collider                Collider ;
	collider::collision     Collision ;
    vector3                 Move ;

    // Start with all of time
    f32     RemainTime = DeltaTime ;
    s32     CollisionCount ;
    vector3 FirstNormal(0,1,0) ;
    vector3 StartPos  = m_WorldPos ;

    // Try a number of attempts to do all of movement
    for (CollisionCount = 0 ; CollisionCount < MOVE_RETRY_COUNT ; CollisionCount++)
    {
        // Make sure vels are not crazy
        ASSERT(ABS(m_Vel.X) < 1000.0f) ;
        ASSERT(ABS(m_Vel.Y) < 1000.0f) ;
        ASSERT(ABS(m_Vel.Z) < 1000.0f) ;

        // Calculate amount of movement to take
        Move = (m_Vel * RemainTime) ;

        // Apply anim motion
        Move += (m_AnimDeltaPos * RemainTime) / DeltaTime ;
        
        // Fall out early due to not enough movement?
        if (Move.LengthSquared() < (MOVING_EPSILON*MOVING_EPSILON))
            break ;

        // Prepare for an extrusion (ouch!)
		Collider.ExtSetup( this, m_WorldBBox, Move ) ;

        // Do the collision!
        ObjMgr.Collide(object::ATTR_SOLID, Collider ) ;

        // Get results
        Collider.GetCollision( Collision );

        //======================================================================
        // Process a collision
        //======================================================================
        if (Collision.Collided)
        {
            //==================================================================
            // Apply motion so far...
            //==================================================================

            // Back off from collision plane a tad...
            Collision.T -= MIN_FACE_DISTANCE / Move.Length() ; 
            Collision.T = MAX(0, Collision.T) ;
            ASSERT(Collision.T >= 0) ;
            ASSERT(Collision.T <= 1) ;

            // Put at collision point
            vector3 Motion = Move * Collision.T ;
            m_WorldPos  += Motion ;
            m_WorldBBox.Translate(Motion) ;

            // Update remaining time
            RemainTime *= 1 - Collision.T ;

            // Record impact info
            f32 ImpactSpeed = -v3_Dot(m_Vel, Collision.Plane.Normal) ;

            // Calculate slide + bounce delta vectors
            vector3 Slide  = Collision.Plane.Normal * (ImpactSpeed + NORMAL_ELASTICITY) ;
            vector3 Bounce = Collision.Plane.ReflectVector(m_Vel) - m_Vel ;
            
            // Slide or bounce?
            object* pObjectHit = (object*)Collision.pObjectHit ;
            ASSERT(pObjectHit) ;
            switch(pObjectHit->GetType())
            {
                case TYPE_STATION_FIXED:
                case TYPE_STATION_VEHICLE:
                case TYPE_VPAD:
                case TYPE_BUILDING:
                case TYPE_TERRAIN:
                    // Just slide as normal
                    m_Vel += Slide ; 
                    break ;

                default:
                    // Slide 50%
                    m_Vel += Slide * 0.5f ;

                    // Bounce 50%
                    m_Vel += Bounce * 0.5f ;
                    break ;
            }

            // Crease check
            if (CollisionCount == 0)
            {
                FirstNormal = Collision.Plane.Normal ;
            }
            else
            {
                // Hit a crease?
                if (CollisionCount == 1)
                {
                    // Re-orient velocity along the creases
                    if (v3_Dot(Slide, FirstNormal) < 0 &&
                       v3_Dot(Collision.Plane.Normal, FirstNormal) < 0)
                    {
                        vector3 nv;
                        nv = v3_Cross(Collision.Plane.Normal, FirstNormal) ;
                        f32 nvl = nv.Length() ;
                        if (nvl > 0)
                        {
                            if (v3_Dot(nv, m_Vel) < 0)
                                nvl = -nvl ;

                            nv     *= m_Vel.Length() / nvl ;
                            m_Vel  = nv ;
                        }
                    }
                }
            }
        }
        else
        {
            // No collision - put at end of movement
            m_WorldPos += Move ;
            m_WorldBBox.Translate(Move) ;
            break ;
        }
    }

    // If max iterations was reached, we hit a crease, so put the player back to their start
    // pos so that they do not get stuck!
    if (CollisionCount == MOVE_RETRY_COUNT)
    {
        m_WorldPos  = StartPos ;
        m_Vel.Zero() ;
    }                

    // Setup world bounds
    m_WorldBBox = m_MoveInfo->BOUNDING_BOX ;
    m_WorldBBox.Translate(m_WorldPos) ;
}

//==============================================================================

void corpse_object::Setup_MOVE( void )
{
}

//==============================================================================

void corpse_object::Advance_MOVE( f32 DeltaTime )
{
    // Advance anim
    m_PlayerInstance.Advance( DeltaTime, &m_AnimDeltaPos ) ;
    
    // Do movement
    ApplyPhysics( DeltaTime ) ;
    DoMovement( DeltaTime ) ;

    // Set instance position
    m_PlayerInstance.SetPos(m_WorldPos) ;

    // Set instance rotation
    quaternion Rot = m_GroundRot * quaternion(m_Rot) ;

    m_PlayerInstance.SetRot(Rot.GetRotation()) ;
    // Always wait for anim to finish first
    if (m_PlayerInstance.GetCurrentAnimState().GetAnimPlayed())
    {
        // Settled on a surface or been here too long?
        if ((m_OnSurface) && (m_Vel.Length() < MOVING_EPSILON))
            SetupState(CORPSE_STATE_IDLE) ;
        else
        // Been here too long?
        if( m_StateTime > CORPSE_MOVE_STUCK_TIME )
            SetupState(CORPSE_STATE_IDLE) ;
    }
}

//==============================================================================

void corpse_object::Setup_IDLE( void )
{
}

//==============================================================================

void corpse_object::Advance_IDLE( f32 DeltaTime )
{
    (void)DeltaTime ;

    // Fade out yet?
    if (m_StateTime > CORPSE_IDLE_TIME)
        SetupState(CORPSE_STATE_FADE_OUT) ;
}
                                 
//==============================================================================

void corpse_object::Setup_FADE_OUT( void )
{
    // Update active count
    s_ActiveCount-- ;
}

//==============================================================================

void corpse_object::Advance_FADE_OUT( f32 DeltaTime )
{
    (void)DeltaTime ;
}

//==============================================================================

