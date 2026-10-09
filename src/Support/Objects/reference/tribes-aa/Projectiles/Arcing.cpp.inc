//==============================================================================
//
//  Arcing.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Arcing.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "AADS\Globals.hpp"
#include "AADS\GameClient.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define DEFAULT_RADIUS  0.125f

//==============================================================================
//  STORAGE
//==============================================================================

static xbool s_ArcingLog = FALSE;
static X_FILE* pLogFile  = NULL;

static f32 Drag    = 20.0f;

static collider ArcCollider;

f32 ARCING_BLEND_TIME = 0.5f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

arcing::arcing( void )
{
    if( tgl.ServerPresent )
        m_CollisionAttr = ATTR_SOLID | ATTR_DAMAGEABLE ;
    else
        m_CollisionAttr = ATTR_SOLID_STATIC;

    m_SfxHandle = 0;
    m_SoundID   = SOUNDTYPE_NONE;
}

//==============================================================================

object::update_code arcing::OnAdvanceLogic( f32 DeltaTime )
{    
    // Add in any unused time from previous logic runs.
    DeltaTime += m_Pending;
    m_Pending  = 0.0f;

    if( m_State != STATE_SETTLED )
    {
        // Update the time since last update.
        m_Update += DeltaTime;

        // Time for an update?
        if( m_Update > m_Interval )
        {
            m_DirtyBits |= 0x01;
            m_Update     = 0.0f;
        }
    }

    // Take care of that logic stuff.
    return( AdvanceLogic( DeltaTime ) );
}       

//==============================================================================

object::update_code arcing::AdvanceLogic( f32 DeltaTime )
{
    // IDEA: Solve for velocity at time of impact.  Needs above idea to prevent
    //       "inverted" bounces.
    //
    // NOTE: WorldPos and WorldBBox might separate...

    object::update_code Result = UPDATE;

    m_BlendReady = FALSE;

    if( s_ArcingLog && !pLogFile )
    {
        pLogFile = x_fopen( "ArcLog.txt", "wt" );
        ASSERT( pLogFile );
    }

    // Maybe clear the ExcludeID.
    if( m_ExcludeID.Slot != -1 )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_ExcludeID );
        if( pObject )
        {
            bbox BBox = pObject->GetBBox();
            if( ((m_WorldBBox.Max.X + 2.0f) < BBox.Min.X) ||
                ((m_WorldBBox.Max.Y + 2.0f) < BBox.Min.Y) ||
                ((m_WorldBBox.Max.Z + 2.0f) < BBox.Min.Z) ||
                ((m_WorldBBox.Min.X - 2.0f) > BBox.Max.X) ||
                ((m_WorldBBox.Min.Y - 2.0f) > BBox.Max.Y) ||
                ((m_WorldBBox.Min.Z - 2.0f) > BBox.Max.Z) )
            {
                m_ExcludeID = obj_mgr::NullID;
            }
        }
    }

    if( m_State == STATE_SETTLED )
    {    
        // Age gracefully, and go on with life.
        m_Age += DeltaTime;
        return( NO_UPDATE );
    }                       

    if( (m_State != STATE_SETTLED) && s_ArcingLog && pLogFile )
    {
        x_fprintf( pLogFile, "\n===Begin Logic===\n" );
        x_fprintf( pLogFile, "State: %s\n", (m_State == STATE_FALLING) ? "FALLING" : "SLIDING" );
    }

    // Fly thru air, bounce, slide, settle, all that good stuff.
    if( m_State != STATE_SETTLED )
    {
        Result = AdvancePhysics( DeltaTime );
    }
               
    if( (m_State != STATE_SETTLED) && s_ArcingLog && pLogFile )
    {
        x_fprintf( pLogFile, "State: %s\n", (m_State == STATE_FALLING) ? "FALLING" : "SLIDING" );
        x_fprintf( pLogFile, "===End Logic===\n\n" );
        x_fflush( pLogFile );
    }

    // Update the looping sound.
    audio_SetPosition( m_SfxHandle, &m_WorldPos );

    // Return the resulting update code.
    return( Result );
}

//==============================================================================

object::update_code arcing::AdvancePhysics( f32 DeltaTime )
{
    vector3 OriginalPosition;
    bbox    OriginalBBox;

    vector3 NewPos;
    vector3 DeltaPos;
    vector3 Rise, Run;
    vector3 Normal[3];
    s32     Iteration = -1;

    OriginalPosition = m_WorldPos;
    OriginalBBox     = m_WorldBBox;

    if( s_ArcingLog && pLogFile )
    {
        x_fprintf( pLogFile, "Begin Physics\n" );
        x_fprintf( pLogFile, "DeltaTime: %g\n", DeltaTime );
        x_fprintf( pLogFile, "Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
        x_fflush( pLogFile );
    }

    // Alter the velocity by applying gravity.
    if( m_State == STATE_FALLING )
    {
        m_Velocity.Y -= m_Gravity * DeltaTime;
    }
    else
    {
        vector3 GravityVec( 0, -m_Gravity * DeltaTime, 0 );
        vector3 IntoGround  = m_SettleNormal * (m_SettleNormal.Y * -m_Gravity * DeltaTime);
        vector3 AlongGround = GravityVec - IntoGround;

        m_Velocity += AlongGround;

        if( s_ArcingLog && pLogFile )
        {
            x_fprintf( pLogFile, "Sliding Accel: %g %g %g | %g (%g)\n", AlongGround.X, AlongGround.Y, AlongGround.Z, AlongGround.Length(), AlongGround.LengthSquared() );
            x_fprintf( pLogFile, "Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
            x_fflush( pLogFile );
        }

        // Apply drag here.
        f32 DragAmount = m_Drag * DeltaTime;

        if( s_ArcingLog && pLogFile )
        {
            x_fprintf( pLogFile, "Adding drag: %g\n", DragAmount );
            x_fflush( pLogFile );
        }

        if( (DragAmount * DragAmount) > m_Velocity.LengthSquared() )
        {
            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Settled from drag!\n" );
                x_fflush( pLogFile );
            }
            m_State    = STATE_SETTLED;
            m_Accurate = TRUE;
            return( UPDATE );
        }
        else
        {
            m_Velocity.NormalizeAndScale( m_Velocity.Length() - DragAmount );
        }

        // See if we are sliding slowly.  
        // And we have been doing so for too long, settle.
        if( m_Velocity.Length() < 0.5f )
        {
            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Slow movement time: %g\n", m_SlowSlideTime );
                x_fflush( pLogFile );
            }

            m_SlowSlideTime += DeltaTime;
            if( m_SlowSlideTime > 0.5f )
            {
                if( s_ArcingLog && pLogFile )
                {
                    x_fprintf( pLogFile, "Settled from slow sliding!\n" );
                    x_fflush( pLogFile );
                }
                m_State    = STATE_SETTLED;
                m_Accurate = TRUE;
                return( UPDATE );
            }
        }
    }

    if( s_ArcingLog && pLogFile )
    {
        x_fprintf( pLogFile, "Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
        x_fflush( pLogFile );
    }

    // Assume we can travel without collision, and propose new position.
    NewPos = m_WorldPos + (m_Velocity * DeltaTime);
    if( (m_State == STATE_FALLING) && 
        IN_RANGE( 0, NewPos.X, 2048 ) &&
        IN_RANGE( 0, NewPos.Y, 1024 ) &&
        IN_RANGE( 0, NewPos.Z, 2048 ) )
    {
        m_Accurate = FALSE;
    }
    else
    {
        m_Accurate = TRUE;
    }

    while( TRUE )
    {
        Iteration++;

        if( Iteration == 3 )
        {
            Normal[0] = Normal[1];
            Normal[1] = Normal[2];
            Normal[2]( 0,1,0 );
            Iteration = 2;
        }

        if( s_ArcingLog && pLogFile )
        {
            x_fprintf( pLogFile, "Iteration: %d\n", Iteration );
            x_fprintf( pLogFile, "OldPos: %g %g %g\n", m_WorldPos.X, m_WorldPos.Y, m_WorldPos.Z );
            x_fprintf( pLogFile, "NewPos: %g %g %g\n", NewPos.X, NewPos.Y, NewPos.Z );
            x_fflush( pLogFile );
        }   

        // If not enough movement, then store the unused time for later.
        DeltaPos = NewPos - m_WorldPos;
        if( DeltaPos.LengthSquared() < 0.0001f )
        {
            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Not enough movement, skipping for now\n" );
                x_fflush( pLogFile );
            }
            m_Pending += DeltaTime;
            return( UPDATE );
        }  

        // Check for collision.
        ArcCollider.ExtSetup( this, m_WorldBBox, DeltaPos, m_ExcludeID.GetAsS32() );
        ObjMgr.Collide( m_CollisionAttr, ArcCollider );

        // No collision?
        if( ArcCollider.HasCollided() == FALSE )
        {
            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "No collision, moved in full\n" );
                x_fflush( pLogFile );
            }
            // We had no collision, so all of DeltaTime is consumed.
            m_WorldPos += DeltaPos;
            m_WorldBBox.Translate( DeltaPos );
            m_Age += DeltaTime;
            if( m_Blending )
                m_BlendTimer -= DeltaTime;

            // If we are sliding, make sure there is still something below.
            if( m_State == STATE_SLIDING )
            {
                DeltaPos = vector3(0,-1.0f,0) ;
                ArcCollider.ExtSetup( this, m_WorldBBox, DeltaPos, m_ExcludeID.GetAsS32() );
                ObjMgr.Collide( m_CollisionAttr, ArcCollider );
                if( !ArcCollider.HasCollided() || (ArcCollider.GetCollisionT() > 0.05f) )
                {
                    if( s_ArcingLog && pLogFile )
                    {
                        x_fprintf( pLogFile, "Slide surface not found, falling!\n" );
                    }
                    m_State = STATE_FALLING;
                }
            }

            // We're done here.
            return( UPDATE );
        }

        // If we are here, then there is a collision.
        collider::collision Collision;
        ArcCollider.GetCollision( Collision );
        m_SettleNormal    = Collision.Plane.Normal;
        Normal[Iteration] = Collision.Plane.Normal;

        if( s_ArcingLog && pLogFile )
        {
            x_fprintf( pLogFile, "Collision!\n" );
            x_fprintf( pLogFile, "T: %g\n", Collision.T );
            x_fprintf( pLogFile, "Point: %g %g %g\n", Collision.Point.X, Collision.Point.Y, Collision.Point.Z );
            x_fprintf( pLogFile, "Plane: %g %g %g  %g\n", Collision.Plane.Normal.X, Collision.Plane.Normal.Y, Collision.Plane.Normal.Z, Collision.Plane.D );
            x_fprintf( pLogFile, "Object Hit: %s\n", ((object*)Collision.pObjectHit)->GetTypeName() );
            x_fflush( pLogFile );
        }

        // Upon collision, it is a guaranteed (accurate) update.
        m_DirtyBits |= 0x01;
        m_Update     = 0.0f;
        m_Accurate   = TRUE;

        // Figure out when it took place, and update as needed.
        vector3 BackOff = DeltaPos;
        BackOff.NormalizeAndScale( 0.005f );
        f32 TimeUsed = DeltaTime * Collision.T;
        DeltaTime  -= TimeUsed;
        m_Age      += TimeUsed;
        DeltaPos   *= Collision.T;
        DeltaPos   -= BackOff;
        m_WorldPos += DeltaPos;
        m_WorldBBox.Translate( DeltaPos );

        // Clear the exclusion upon impact if outside of ExcludeID's bbox.
        if( m_ExcludeID.Slot != -1 )
        {
            object* pObject = ObjMgr.GetObjectFromID( m_ExcludeID );
            if( pObject && !(pObject->GetBBox().Intersect( m_WorldBBox )) )
            {
                m_ExcludeID = obj_mgr::NullID;
            }
        }        

        if( s_ArcingLog && pLogFile )
        {
            x_fprintf( pLogFile, "Time Used: %g\n", TimeUsed );
            x_fprintf( pLogFile, "DeltaPos: %g %g %g\n", DeltaPos.X, DeltaPos.Y, DeltaPos.Z );
            x_fprintf( pLogFile, "WorldPos: %g %g %g\n", m_WorldPos.X, m_WorldPos.Y, m_WorldPos.Z );            
            x_fflush( pLogFile );
        }                    

        // This could be the end of this humble object.  It's up to the 
        // descendant class to decide.
        if( Impact( Collision ) )
        {
            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Descendant class signaled for destruction\n" );
                x_fflush( pLogFile );
            }       

            m_Hidden = TRUE;
            return( DESTROY );
        }

        // If we are here, then the object is not dead.  React to collision.
        // How we react depends on how many iterations we have done during this
        // function invokation.

        if( Iteration == 0 )
        {
            // OK.  Nothting to get too excited about.  This is the first 
            // iteration, and the first collision.  Bounce or slide.

            Collision.Plane.GetComponents( m_Velocity, Run, Rise );
            if( Rise.Dot( Collision.Plane.Normal ) < 0.0f )
                Rise.Negate();
            Rise *= m_RiseFactor;
            Run  *= m_RunFactor;

            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "RiseVelocity: %g %g %g | %g (%g)\n", Rise.X, Rise.Y, Rise.Z, Rise.Length(), Rise.LengthSquared() );
                x_fprintf( pLogFile, "Run Velocity: %g %g %g | %g (%g)\n", Run.X, Run.Y, Run.Z, Run.Length(), Run.LengthSquared() );
                x_fflush( pLogFile );
            }                    

            // Should we just slide?
            if( (Collision.Plane.Normal.Y > 0.0001f) &&
                (Rise.LengthSquared() < 5.0f) )
            {
                if( m_State != STATE_SLIDING )
                {
                    m_State = STATE_SLIDING;
                    m_SlowSlideTime = 0.0f;
                }
                m_Velocity = Run;

                if( s_ArcingLog && pLogFile )
                {
                    x_fprintf( pLogFile, "Sliding state activated\n" );
                    x_fprintf( pLogFile, "New Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
                    x_fprintf( pLogFile, "Surface Normal: %g %g %g\n", Collision.Plane.Normal.X, Collision.Plane.Normal.Y, Collision.Plane.Normal.Z );
                    x_fflush( pLogFile );
                }                    
            }
            else
            {
                m_Velocity = Rise + Run;
                if( s_ArcingLog && pLogFile )
                {
                    x_fprintf( pLogFile, "Bounce\n" );
                    x_fprintf( pLogFile, "New Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
                    x_fprintf( pLogFile, "Surface Normal: %g %g %g\n", Collision.Plane.Normal.X, Collision.Plane.Normal.Y, Collision.Plane.Normal.Z );
                    x_fflush( pLogFile );
                }                    
            }

            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Looping for more movement\n" );
                x_fflush( pLogFile );
            }

            // Propose new position.  
            // Add in some offset to pull away from surface from the collision.
            NewPos = m_WorldPos + (m_Velocity * DeltaTime) + (Collision.Plane.Normal * 0.005f);
        }

        if( Iteration == 1 )
        {
            // Hmm.  This is the second collision.  This could be tricky. 
            // Attempt to slide along the crevice formed by the two surfaces.

            vector3 CreviceNormal;

            CreviceNormal = Normal[0] + Normal[1];

            if( CreviceNormal.LengthSquared() < 0.5f )
            {
/*
#ifdef X_DEBUG
                x_DebugMsg( "======================================================\n" );
                x_DebugMsg( "WARNING: Probable double sided triangle hit by arcing.\n" );
                x_DebugMsg( "Object Hit: %s at (%g,%g,%g)\n", 
                            ((object*)Collision.pObjectHit)->GetTypeName(),
                            ((object*)Collision.pObjectHit)->GetPosition().X, 
                            ((object*)Collision.pObjectHit)->GetPosition().Y, 
                            ((object*)Collision.pObjectHit)->GetPosition().Z );
                x_DebugMsg( "======================================================\n" );
#endif
*/
                m_State = STATE_SETTLED;
                return( UPDATE );
            }

            CreviceNormal.Normalize();
            m_SettleNormal = CreviceNormal;

            // Go into a slide state.  (Probably already sliding, though.)
            // The new velocity is the old projected onto the crevice axis.
            if( m_State != STATE_SLIDING )
            {
                m_State = STATE_SLIDING;
                m_SlowSlideTime = 0.0f;
            }

            vector3 CreviceAxis;

            CreviceAxis = v3_Cross( Normal[0], Normal[1] );
            if( CreviceAxis.Normalize() )
            {
                m_Velocity = CreviceAxis * v3_Dot( CreviceAxis, m_Velocity );
            }

            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Sliding along crevice!\n" );
                x_fprintf( pLogFile, "New Velocity: %g %g %g | %g (%g)\n", m_Velocity.X, m_Velocity.Y, m_Velocity.Z, m_Velocity.Length(), m_Velocity.LengthSquared() );
                x_fflush( pLogFile );
            }                    

            // Propose new position.  
            // Add in some offset to pull away from surface from the collision.
            NewPos = m_WorldPos + (m_Velocity * DeltaTime) + (CreviceNormal * 0.005f);
        }

        if( Iteration == 2 )
        {
            // Ah, crap!  This is the THIRD collision!  Looks like we have hit a
            // "dead-end".  Consider the orientation of the 3 surfaces we have 
            // hit.
            // 
            // If they average to a downward direction:
            //  - Don't move the object.
            //  - Switch to a falling state.
            //  - Let m_Gravity (hopefully) pull the object out.
            //
            // If they average to a upward direction:
            //  - Settle the object where it lies.

            if( s_ArcingLog && pLogFile )
            {
                x_fprintf( pLogFile, "Hit dead end!\n" );
            }

            vector3 AvgNormal = (Normal[0] + Normal[1] + Normal[2]);
            if( AvgNormal.Normalize() )
            {
                m_SettleNormal = AvgNormal;
            }

            if( AvgNormal.Y >= 0.0f )
            {
                // Settle.
                if( s_ArcingLog && pLogFile )
                {
                    x_fprintf( pLogFile, "Settled from dead end!\n" );
                    x_fflush( pLogFile );
                }
                m_State = STATE_SETTLED;
            }
            else
            {
                // Stop and fall.
                if( s_ArcingLog && pLogFile )
                {
                    x_fprintf( pLogFile, "Falling out!\n" );
                    x_fflush( pLogFile );
                }
                m_WorldPos  = OriginalPosition;
                m_WorldBBox = OriginalBBox;
                m_Velocity( 0, 0, 0 );
                m_State = STATE_FALLING;
            }

            return( UPDATE );
        }
    }

    // Should not get down here.
    ASSERT( FALSE );
    return( UPDATE );
} 

//==============================================================================

void arcing::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    // Check exclude ID?  This fixes satchel vs laser rifle collision!
    if( m_ExcludeID != obj_mgr::NullID )
    {
        object* pObject = (object*)Collider.GetMovingObject();
        if( (pObject) && (pObject->GetObjectID() == m_ExcludeID) )
            return;
    }

    if( m_AttrBits & ATTR_PERMEABLE )
        Collider.ApplyPermeableBBox( m_WorldBBox );
    else
        Collider.ApplyBBox( m_WorldBBox );
}

//==============================================================================

void arcing::Initialize( const vector3&   Position,
                         const vector3&   StartPos,
                         const vector3&   Velocity,         
                               object::id OriginID,          
                               object::id ExcludeID,
                               u32        TeamBits )
{
    ASSERT( Velocity.LengthSquared() < SQR(1000.0f) );

    m_TeamBits   = TeamBits;
    m_WorldPos   = Position;
    m_StartPos   = StartPos;
    m_Velocity   = Velocity;
    m_OriginID   = OriginID;
    m_ExcludeID  = ExcludeID;
    m_Age        =  0.0f;
    m_Update     =  0.0f;
    m_Pending    =  0.0f;
    m_Interval   =  1.0f;
    m_RunFactor  =  0.5f;
    m_RiseFactor =  0.2f;
    m_DragSpeed  = Drag;
    m_State      = STATE_FALLING;
    m_Hidden     = FALSE;
    m_Accurate   = FALSE;
    m_SoundID    = SOUNDTYPE_NONE;
    m_WorldBBox.Set( m_WorldPos, DEFAULT_RADIUS );
    m_ShapeDrawFlags = 0x00;
    m_Blending       = FALSE;
}

//==============================================================================

void arcing::Initialize( const vector3&   Position,
                         const vector3&   Velocity,         
                               object::id OriginID,          
                               object::id ExcludeID,
                               u32        TeamBits )
{
    Initialize( Position, Position, Velocity, OriginID, ExcludeID, TeamBits );
}

//==============================================================================

void arcing::SetOriginID( object::id OriginID )
{
    m_OriginID = OriginID;
}

//==============================================================================

vector3 arcing::GetVelocity( void ) const
{
    return( m_Velocity );
}

//==============================================================================

void arcing::SetVelocity( const vector3& Velocity )
{
    m_Velocity   = Velocity;
    m_DirtyBits |= 0x01;
}

//==============================================================================

void arcing::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteRangedS32 ( m_State, 0, 2 );
    BitStream.WriteVector    ( m_WorldPos );
    BitStream.WriteWorldPosCM( m_StartPos );
    BitStream.WriteVector    ( m_Velocity );
    BitStream.WriteObjectID  ( m_OriginID );

    if( BitStream.WriteFlag( m_OriginID.Slot != m_ExcludeID.Slot ) )
        BitStream.WriteObjectID( m_ExcludeID );

    if( m_State == STATE_SLIDING )
        BitStream.WriteVector( m_SettleNormal );
}

//==============================================================================

void arcing::OnAcceptInit( const bitstream& BitStream )
{
    vector3     Position;
    vector3     StartPos;
    vector3     Velocity;
    s32         State;
    object::id  OriginID;
    object::id  ExcludeID;

    BitStream.ReadRangedS32 ( State, 0, 2 );
    BitStream.ReadVector    ( Position );
    BitStream.ReadWorldPosCM( StartPos );
    BitStream.ReadVector    ( Velocity );
    BitStream.ReadObjectID  ( OriginID );

    if( BitStream.ReadFlag() )
        BitStream.ReadObjectID( ExcludeID );
    else
        ExcludeID = OriginID;

    if( (state)State == STATE_SLIDING )
        BitStream.ReadVector( m_SettleNormal );

    ASSERT( Velocity.Length() < 1000.0f );

    Initialize( Position, StartPos, Velocity, OriginID, ExcludeID, 0x00 );

    // Slight HACK HACK here, setting state AFTER calling Initialize().
    m_State = (state)State;

    // Set up the blending.
    m_Blending = FALSE;
    m_BlendVector(0,0,0);
    {
        object* pOrigin = ObjMgr.GetObjectFromID( m_OriginID );
        if( pOrigin )
        {
            m_BlendPos    = pOrigin->GetBlendFirePos();
            m_BlendVector = m_BlendPos - m_StartPos;
            m_BlendReady  = FALSE;
            m_Blending    = TRUE;
            m_BlendTimer  = ARCING_BLEND_TIME;
        }
    }
}

//==============================================================================

object::update_code arcing::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    object::update_code Result;
    xbool               Accurate;
    xbool               Update = FALSE;
    vector3             OldPos = m_WorldPos;

    if( (Accurate = BitStream.ReadFlag()) )
        BitStream.ReadVector( m_WorldPos );
    else
        BitStream.ReadWorldPosCM( m_WorldPos );

    if( BitStream.ReadFlag() )
        BitStream.ReadRangedF32( m_Velocity.X, 12, -100.0f, 100.0f );
    else
        BitStream.ReadF32( m_Velocity.X );

    if( BitStream.ReadFlag() )
        BitStream.ReadRangedF32( m_Velocity.Y, 12, -100.0f, 100.0f );
    else
        BitStream.ReadF32( m_Velocity.Y );

    if( BitStream.ReadFlag() )
        BitStream.ReadRangedF32( m_Velocity.Z, 12, -100.0f, 100.0f );
    else
        BitStream.ReadF32( m_Velocity.Z );

    if( BitStream.ReadFlag() )
        BitStream.ReadRangedF32( m_Age, 10, 0.0f, 30.0f );
    else
        BitStream.ReadF32( m_Age );

    BitStream.ReadRangedS32( (s32&)m_State, 0, 2 );
    
    if( m_State == STATE_SLIDING )
        BitStream.ReadVector( m_SettleNormal );

    m_Pending = 0.0f;
    m_Hidden  = FALSE;
    m_WorldBBox.Translate( m_WorldPos - OldPos );

    if( !Accurate )
    {
        m_BlendVector = OldPos - m_WorldPos;
        m_BlendReady  = FALSE;
        m_Blending    = TRUE;
        m_BlendTimer  = ARCING_BLEND_TIME;
    }

    while( SecInPast > 0.0001f )
    {
        Result = AdvanceLogic( MIN( SecInPast, 0.032f ) );
        if( Result == UPDATE  )  Update = TRUE;
        if( Result == DESTROY )  return( DESTROY );
        SecInPast -= 0.032f;
    }

    return( Update ? UPDATE : NO_UPDATE );
}

//==============================================================================

void arcing::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( DirtyBits | 0x01 )
    {
        if( BitStream.WriteFlag( m_Accurate ) )
            BitStream.WriteVector( m_WorldPos );
        else
            BitStream.WriteWorldPosCM( m_WorldPos );

        if( BitStream.WriteFlag( IN_RANGE( -100.0f, m_Velocity.X, 100.0f ) ) )
            BitStream.WriteRangedF32( m_Velocity.X, 12, -100.0f, 100.0f );
        else
            BitStream.WriteF32( m_Velocity.X );

        if( BitStream.WriteFlag( IN_RANGE( -100.0f, m_Velocity.Y, 100.0f ) ) )
            BitStream.WriteRangedF32( m_Velocity.Y, 12, -100.0f, 100.0f );
        else
            BitStream.WriteF32( m_Velocity.Y );

        if( BitStream.WriteFlag( IN_RANGE( -100.0f, m_Velocity.Z, 100.0f ) ) )
            BitStream.WriteRangedF32( m_Velocity.Z, 12, -100.0f, 100.0f );
        else
            BitStream.WriteF32( m_Velocity.Z );

        if( BitStream.WriteFlag( IN_RANGE( 0.0f, m_Age, 30.0f ) ) )
            BitStream.WriteRangedF32( m_Age, 10, 0.0f, 30.0f );
        else
            BitStream.WriteF32( m_Age );

        BitStream.WriteRangedS32( m_State, 0, 2 );

        if( m_State == STATE_SLIDING )
            BitStream.WriteVector( m_SettleNormal );

        DirtyBits &= ~0x01;
    }
}

//==============================================================================

xbool arcing::Impact( const collider::collision& Collision )
{   
    if( Collision.pObjectHit )
        m_ObjectHitID = ((object*)Collision.pObjectHit)->GetObjectID();
    else
        m_ObjectHitID = obj_mgr::NullID;

    return( FALSE );
}

//==============================================================================

void arcing::OnAdd( void )
{
    // Start the looping sound.
    m_SfxHandle = audio_Play( m_SoundID, &m_WorldPos );

    // Turn down the volume for the guy who shot it.
    if( m_OriginID != obj_mgr::NullID )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_OriginID );
        if( (pObject) && (pObject->GetType() == TYPE_PLAYER) )
        {
            player_object* pPlayer = (player_object*)pObject;
            if( pPlayer->GetHasInputFocus() )
                audio_SetVolume( m_SfxHandle, 0.5f );
        }
    }

    // initialize to the default gravity
    m_Gravity = GetGravity();
    m_Drag    = Drag;
}

//==============================================================================

void arcing::OnRemove( void )
{
    // Stop the looping sound.
    audio_Stop( m_SfxHandle );
}

//==============================================================================

#define CULL_RADIUS 0.35f
#define CULL_SIZE   1.30f

s32 arcing::RenderPrep( void )
{
    s32 Visible = view::VISIBLE_NONE;

    // Hidden?
    if( m_Hidden )
        return( view::VISIBLE_NONE );

    // Use shape geometry bounds?
    if( m_Shape.GetShape() )
    {
        // Get geometry info
        bbox    GeomBBox   = m_Shape.GetWorldBounds();
        vector3 GeomCenter = GeomBBox.GetCenter();
        f32     GeomRadius = GeomBBox.GetRadius();

        // Too small to draw?
        const view* pView = eng_GetActiveView(0);
        ASSERT( pView );
        if( !pView->SphereInCone( GeomCenter, GeomRadius ) )
            return( view::VISIBLE_NONE );
        if( pView->CalcScreenSize( GeomCenter, GeomRadius ) < CULL_SIZE )
            return( view::VISIBLE_NONE );

        // Is shape in view?
        Visible = IsInViewVolume( GeomBBox );
        if( !Visible )
            return( view::VISIBLE_NONE );

        // Setup draw flags.
        m_ShapeDrawFlags = shape::DRAW_FLAG_FOG;
        if( Visible == view::VISIBLE_PARTIAL )
            m_ShapeDrawFlags |= shape::DRAW_FLAG_CLIP;

        // Setup fog color
        m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    }
    else
    {
        // Is object in view?
        Visible = IsInViewVolume( m_WorldBBox );
        if( !Visible )
            return( view::VISIBLE_NONE );
    }

    return( Visible );
}

//==============================================================================

const vector3& arcing::GetRenderPos( void )
{
    // See if blending should be complete.
    if( m_Blending && (m_BlendTimer < 0.0f) )
    {
        m_Blending = FALSE;
    }

    // Blending?
    if( m_Blending )
    {
        // Out of date?
        if( !m_BlendReady )
        {
            m_BlendPos   = m_WorldPos + m_BlendVector * (m_BlendTimer / ARCING_BLEND_TIME);
            m_BlendReady = TRUE;
        }

        return( m_BlendPos );
    }

    return( m_WorldPos );
}

//==============================================================================

void arcing::SetScoreID( const object::id ScoreID )
{
    m_ScoreID = ScoreID;
}

//==============================================================================


