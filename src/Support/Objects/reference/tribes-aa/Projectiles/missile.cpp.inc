//==============================================================================
//
//  Missile.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/Entropy.hpp>
#include "ParticleObject.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AudioMgr/Audio.hpp"
#include "netlib/bitstream.hpp"
#include "GameMgr/GameMgr.hpp"
#include "..\AADS\Data\UI\Messages.h"

#include "Objects\Player\PlayerObject.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "grenade.hpp"
#include "Objects\Vehicles\Vehicle.hpp"

#include "Missile.hpp"
#include "textures.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define DIRTY_POS       ( 1 << 0 )
#define DIRTY_TARGET    ( 1 << 1 )
#define MAX_VERT_DIFF   200.0f

#define MAX_VEL         200                 // m/s maximum velocity
#define INIT_VEL         30                 // start # m/s vel
#define ACCEL            30                 // m/s acceleration
#define LIFE             10                 // seconds max lifetime
#define BOOM_DIST       (  2.0f *  2.0f )   // Length squared
#define LOCK_DIST       ( 50.0f * 50.0f )   // Length squared

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   MissileCreator;

obj_type_info   MissileClassInfo( object::TYPE_MISSILE, 
                                  "Missile", 
                                  MissileCreator, 
                                  object::ATTR_UNBIND_ORIGIN |
                                  object::ATTR_UNBIND_TARGET );

static f32      FLARE_DIST = SQR(200.0f);
static f32      FLARE_ANG  = R_45;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* MissileCreator( void )
{
    return( (object*)(new missile) );
}

//==============================================================================

object::update_code missile::OnAdvanceLogic( f32 DeltaTime )
{
    ASSERT( m_WorldPos.InRange( -20000, 20000 ) );
    ASSERT( m_TargetPt.InRange( -20000, 20000 ) );

    // Increment the time since last update.
    if( tgl.ServerPresent )
    {
        m_Update += DeltaTime;

        // Time for an update?
        if( m_Update > 0.25f )
        {
            m_DirtyBits |= DIRTY_POS;
            m_Update     = 0.0f;
        }
    }

    // Take care of that logic stuff.
    return( AdvanceLogic( DeltaTime ) );
}

//==============================================================================

object::update_code missile::AdvanceLogic( f32 DeltaTime )
{
    collider                Segment;
    collider::collision     Collision;
    vector3                 Interp;
    radian                  Ang;
    f32                     Dist;

    pain::type PainType[] = { pain::WEAPON_MISSILE,
                              pain::TURRET_MISSILE };

    // first, adjust missile towards blendpos
    if( (!tgl.ServerPresent) && (m_UpdateRecvd) )
    {
        vector3 tmp = (m_BlendPos - m_WorldPos) * 0.5f * DeltaTime;
        m_WorldPos += tmp;
    }

    // to prevent crashes (div/0 and such)
    if( DeltaTime == 0.0f )
        return NO_UPDATE;

    // Update position of smoke emitter.
    if( m_SmokeEffect.IsCreated() )
    {
        m_SmokeEffect.SetVelocity( (m_WorldPos - m_SmokeEffect.GetPosition()) / DeltaTime );
        m_SmokeEffect.UpdateEffect( DeltaTime );
    }

    // no further processing once we've exploded, but keep updating smoke til we're done
    if( m_HasExploded && tgl.ServerPresent )
    {
        if( !m_SmokeEffect.IsActive() )
            return( DESTROY );
        else
            return( NO_UPDATE );
    }

    // update the audio location
    audio_SetPosition( m_SfxID, &m_WorldPos );

    // calculate target trajectory
    if( m_TargetID != obj_mgr::NullID )
    {
        object* pObj;

        pObj = ObjMgr.GetObjectFromID( m_TargetID );

        // is object still in the world of the living?
        if( pObj == NULL )
        {
            // no?  No target ID then, just keep going forward
            m_TargetID = obj_mgr::NullID;
        }
        else
        {
            // get the location of the target
            m_TargetPt = pObj->GetPainPoint();
        }
    }

    // advance the motion
    vector3 Delta;
    Delta.Set( 0.0f, 0.0f, m_Vel * DeltaTime );
    
    // transform delta movement into world space
    Delta = m_Hdg * Delta;
    m_Hdg.Normalize();

    // adjust velocity
    m_Vel += (m_Acc * DeltaTime);

    // cap it
    if( m_Vel > MAX_VEL )
        m_Vel = MAX_VEL;

    // if movement is too small to be useful, just return UPDATE
    if( Delta.LengthSquared() < SQR(0.0001f) )
        return UPDATE;

    // get a vector to our target
    vector3 TarDir = m_TargetPt - m_WorldPos;

    // Find the distance to the target (squared)
    Dist = TarDir.LengthSquared();

    // check for collision with world
    // did we come within a minimum reasonable distance to the player?
    // This should all be safe to run on the client, because only UPDATE is being returned
    if( Dist < BOOM_DIST )
    {
        vector3 Offset = TarDir;
        Offset.NormalizeAndScale( 0.75f );

        SpawnExplosion( PainType[m_Type],
                        m_TargetPt + Offset,
                        vector3(0,1.0f,0),
                        m_OriginID,
                        m_TargetID );

        m_HasExploded = TRUE;
        m_SmokeEffect.SetEnabled( FALSE );

        // explode flare if applicable
        if( tgl.ServerPresent && (m_TargetID != obj_mgr::NullID) )
        {
            object* pObj = ObjMgr.GetObjectFromID( m_TargetID );

            if( pObj && (pObj->GetType() == object::TYPE_GRENADE) )
            {
                grenade* pGren = (grenade*)pObj;
                pGren->DestroyFlare();
            }
        }

        return( UPDATE );  // not destroy, because our smoke trail is still here!
    }
    else
    {
        Segment.RaySetup( this, m_WorldPos, m_WorldPos + Delta );

        ObjMgr.Collide( object::ATTR_SOLID, Segment );

        // if collided, make an explosion and destroy the missile object
        if( Segment.HasCollided() )
        {
            Segment.GetCollision( Collision );

            // spawn explosion!!!
            if( ((object*)Collision.pObjectHit)->GetObjectID().Slot != m_OriginID.Slot )
            {
                SpawnExplosion( PainType[m_Type],
                                Collision.Point, 
                                Collision.Plane.Normal,
                                m_OriginID,
                                ((object*)Collision.pObjectHit)->GetObjectID() );

                m_HasExploded = TRUE;
                m_SmokeEffect.SetEnabled( FALSE );

                return( UPDATE );  // not destroy, because our smoke trail is still here!
            }
        }
    }

    // translation was succesful, update the world position
    m_WorldPos += Delta;

    // refresh our vector to our target
    TarDir = m_TargetPt - m_WorldPos;

    // Find the distance to the target (squared)
    Dist = TarDir.LengthSquared();

    // missile has not yet collided, so guide it towards it's target
    // keep looking for it unless we've given up
    if( (m_TargetID != obj_mgr::NullID) && (Dist > SQR(0.001)) )
    {
        if( m_GiveUp == FALSE )
        {
            Segment.RaySetup( this, m_WorldPos, m_TargetPt );
            ObjMgr.Collide( object::ATTR_SOLID_STATIC, Segment );

            if( Segment.HasCollided() )
            {
                Segment.GetCollision( Collision );

                // make sure we're not simply colliding with something near the target point
                if( Collision.pObjectHit == ObjMgr.GetObjectFromID( m_TargetID ) ) 
                    m_HasLOS = TRUE;        // we have line of sight
                else
                {
                    m_HasLOS = FALSE;

                    if( ( m_WorldPos.Y - m_TargetPt.Y ) > MAX_VERT_DIFF )
                    { 
                        m_GiveUp = TRUE;
                        m_HasLOS = TRUE;
                    }
                    else
                    {
                        // climb, ya bastard
                        quaternion Up;
                        Interp = v3_Cross( Delta, vector3( 0.0f, 1.0f, 0.0f ) );
                        Interp.Normalize();
                        Ang = v3_AngleBetween( Delta, vector3( 0.0f, 1.0f, 0.0f ) );

                        if( x_abs(Ang) > 0.0f )
                        {
                            Up.Setup( Interp, Ang * 0.033f /*DeltaTime*/ * 4.0f );
                            m_Hdg = Up * m_Hdg;
                            m_Hdg.Normalize();
                        }
                    }
                }
            }
        }
    }

    // Look for flares grenades
    if( tgl.ServerPresent && (!m_Diverted) )
    {
        grenade* pGrenade;
        radian   BestAngle  = R_180;
        grenade* pNewTarget = NULL;

        ObjMgr.StartTypeLoop( object::TYPE_GRENADE );
        while( (pGrenade = (grenade*)ObjMgr.GetNextInType()) )
        {
            if( pGrenade->GetAttrBits() & ATTR_HOT )
            {
                Segment.RaySetup( NULL, m_WorldPos, pGrenade->GetPosition(), -1, TRUE );
                ObjMgr.Collide( ATTR_SOLID_STATIC, Segment );
                if( !Segment.HasCollided() )
                {
                    vector3 FlareDir = (pGrenade->GetPosition() - m_WorldPos);
                    radian  Angle    = v3_AngleBetween( FlareDir, TarDir );

                    if( (FlareDir.LengthSquared() < x_sqr( FLARE_DIST )  ) && 
                        (Angle < BestAngle) && 
                        (Angle < FLARE_ANG ) )
                    {
                        BestAngle  = Angle;
                        pNewTarget = pGrenade;
                    }
                }
            }
        }
        ObjMgr.EndTypeLoop();

        if( pNewTarget )
        {
            object::id FlareOriginID = pNewTarget->GetOriginID();
            object*    pFlareOrigin  = ObjMgr.GetObjectFromID( FlareOriginID );
            object*    pOldTarget    = ObjMgr.GetObjectFromID( m_TargetID );

            // Flare assist bonus?
            if( (FlareOriginID != m_TargetID) && pFlareOrigin && pOldTarget )
            {   
                u32 FlareTeamBits     = pNewTarget->GetTeamBits();
                u32 OldTargetTeamBits = pOldTarget->GetTeamBits();

                // Flare origin and original target must be on same team for bonus.
                if( FlareTeamBits & OldTargetTeamBits )
                {
                    xbool Bonus = TRUE;

                    // For vehicle targets, only give bonus if the flare 
                    // thrower is not the pilot.
                    if( pOldTarget->GetAttrBits() & ATTR_VEHICLE )
                    {
                        object* pPilot = ((vehicle*)pOldTarget)->GetSeatPlayer(0);
                        if( pPilot && (pPilot == pFlareOrigin) )
                            Bonus = FALSE;
                    }
                    
                    if( Bonus )
                    {
                        GameMgr.ScoreForPlayer( FlareOriginID, 
                                                SCORE_FLARE_MISSILE,
                                                MSG_SCORE_FLARE_MISSILE );
                    }
                }
            }

            // turn off "heat" so no other missile will lock onto it
            pNewTarget->m_AttrBits &= ~ATTR_HOT;

            // set the new target
            m_TargetID   = pNewTarget->GetObjectID();
            m_DirtyBits |= DIRTY_TARGET;
            m_Diverted   = TRUE;
        }
    }

    // blend towards target
    Ang = v3_AngleBetween( Delta, TarDir );

    {
        quaternion TarQuat;
        Interp = v3_Cross( Delta, TarDir );

        f32 L = Interp.Length();
        if( L < 0.0001f )
        {
            Interp( 0, 1, 0 );
        }
        else
        {
            Interp.Normalize();
        }

        if( Dist < LOCK_DIST )
            TarQuat.Setup( Interp, Ang );
        else
            TarQuat.Setup( Interp, Ang * DeltaTime * 3.0f );

        m_Hdg = TarQuat * m_Hdg;
        m_Hdg.Normalize();
    }

    m_Age += DeltaTime;

//    // TEMP TEMP TEMP TEMP
    //eng_Begin( "Missile Debug" );
    //draw_Marker( m_TargetPt, XCOLOR_YELLOW );
    //draw_Line( m_WorldPos, m_WorldPos + TarDir, XCOLOR_WHITE );
    //eng_End();

    // dying of old age?
    if( m_Age > LIFE  )
    {
        // fake it out
        m_HasExploded = TRUE;
        m_SmokeEffect.SetEnabled( FALSE );
    }

    // Warn the victim.  (It's more sporting that way.)
    if( tgl.ServerPresent && !m_HasExploded && !m_Diverted )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_TargetID );
        if( pObject )
        {
            pObject->MissileLock();
        }
    }

    return( UPDATE );
}

//==============================================================================

void missile::OnAcceptInit( const bitstream& BitStream )
{
    BitStream.ReadWorldPosCM( m_WorldPos );
    m_BlendPos = m_WorldPos;

    radian3 Hdg;
    BitStream.ReadRangedRadian3( Hdg, 10 );
    m_Hdg.Setup( Hdg );

    BitStream.ReadObjectID( m_TargetID );
    BitStream.ReadObjectID( m_OriginID );

    m_Type = 0;
}

//==============================================================================

void missile::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteWorldPosCM( m_WorldPos );
    
    BitStream.WriteRangedRadian3( m_Hdg.GetRotation(), 10 );

    BitStream.WriteObjectID( m_TargetID );
    BitStream.WriteObjectID( m_OriginID );
}

//==============================================================================

object::update_code missile::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{       
    ASSERT( m_TargetPt.InRange( -20000, 20000 ) );
    ASSERT( SecInPast < 10.0f );

    if( BitStream.ReadFlag() )
    {
        s32 Tag;
        vector3 V;

        BitStream.ReadWorldPosCM( m_BlendPos );
        BitStream.ReadVector    ( V );
        BitStream.ReadRangedS32( Tag, 0, 255 );
/*
        x_DebugMsg( "MISSILE(%02X)(1): (%7.2f,%7.2f,%7.2f) <%08X,%08X,%08X>\n",
                    Tag, 
                    m_BlendPos.X, m_BlendPos.Y, m_BlendPos.Z, 
                    *(u32*)(&m_BlendPos.X), *(u32*)(&m_BlendPos.Y), *(u32*)(&m_BlendPos.Z) );
        x_DebugMsg( "MISSILE(%02X)(2): (%7.2f,%7.2f,%7.2f) <%08X,%08X,%08X>\n",
                    Tag, 
                    V.X, V.Y, V.Z, 
                    *(u32*)(&V.X), *(u32*)(&V.Y), *(u32*)(&V.Z) );
*/
    }

    if( BitStream.ReadFlag() )
        BitStream.ReadObjectID( m_TargetID );

    ASSERT( m_BlendPos.InRange( -20000, 20000 ) );

    // figure out where it should be
    vector3 tmp = m_TargetPt - m_BlendPos;
    tmp.Normalize();
    tmp *= ( m_Vel * SecInPast );

    ASSERT( tmp.InRange( -20000, 20000 ) );

    // Set BlendPos to the correct (server version) location in space
    m_BlendPos += tmp;     

    ASSERT( m_BlendPos.InRange( -20000, 20000 ) );

    m_UpdateRecvd = TRUE;

    return( AdvanceLogic( SecInPast ) );
}

//==============================================================================

random R;

void missile::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    ASSERT( m_WorldPos.InRange( -20000, 20000 ) );

    if( BitStream.WriteFlag( DirtyBits & DIRTY_POS ) )
    {
        s32 Tag = R.irand( 0, 255 );

        DirtyBits &= ~DIRTY_POS;
        BitStream.WriteWorldPosCM( m_WorldPos );
        BitStream.WriteVector    ( m_WorldPos );
        BitStream.WriteRangedS32( Tag, 0, 255 );
/*
        x_DebugMsg( "MISSILE(%02X): (%7.2f,%7.2f,%7.2f) <%08X,%08X,%08X>\n",
                    Tag, 
                    m_WorldPos.X, m_WorldPos.Y, m_WorldPos.Z, 
                    *(u32*)(&m_WorldPos.X), *(u32*)(&m_WorldPos.Y), *(u32*)(&m_WorldPos.Z) );
*/
    }

    if( BitStream.WriteFlag( DirtyBits & DIRTY_TARGET ) )
    {
        DirtyBits &= ~DIRTY_TARGET;
        BitStream.WriteObjectID( m_TargetID );
    }
}

//==============================================================================

void missile::OnAdd( void )
{
    collider                Segment;
    collider::collision     Collision;

    // basic initialization
    m_Vel         = INIT_VEL;
    m_Acc         = ACCEL;
    m_HasLOS      = FALSE;
    m_HasExploded = FALSE;
    m_UpdateRecvd = FALSE;
    m_GiveUp      = FALSE;
    m_Age         = 0.0f;
    m_Update      = 0.0f;
    m_Diverted    = FALSE;

    vector3 TarDir( 0.0f, 0.0f, 1000.0f );
    TarDir = m_Hdg * TarDir;

    m_TargetPt = m_WorldPos + TarDir;

    // play firing sound
    s32 Sound[2] = { SFX_WEAPONS_MISSLELAUNCHER_FIRE, SFX_TURRET_MISSILE_FIRE };

    // Determine if a local player fired this blaster
    xbool Local = FALSE;
    object* pCreator = ObjMgr.GetObjectFromID( m_OriginID );
    if( pCreator && (pCreator->GetType() == object::TYPE_PLAYER) )
    {
        player_object* pPlayer = (player_object*)pCreator;
        Local = pPlayer->IsLocal();
    }

    // Play sound
    if( Local )
        audio_Play( Sound[m_Type] );
    else
        audio_Play( Sound[m_Type], &m_WorldPos );

    // setup the shape
    m_RocketShape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_MISSILE ) );

    // if no living target, figure out where to hit
    if( m_TargetID == obj_mgr::NullID )
    {
        // calculate the target point
        // check for collision with world

        Segment.RaySetup( this, m_WorldPos, m_WorldPos + TarDir );

        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Segment );

        // if collided, make an explosion and destroy the missile object
        if( Segment.HasCollided() )
        {
            Segment.GetCollision( Collision );
            m_TargetPt = Collision.Point;
            m_DirtyBits |= DIRTY_TARGET;
        }
        else
        {
            // must be firing out into space            
            m_DirtyBits |= DIRTY_TARGET;
        }
    }

    // Create the smoke trail effect
    m_SmokeEffect.Create( &tgl.ParticlePool, 
                          &tgl.ParticleLibrary, 
                          PARTICLE_TYPE_MISSILE_SMOKE, 
                          m_WorldPos, 
                          vector3(0,0,1), 
                          NULL, 
                          NULL );

    // play the audio loop
    m_SfxID = audio_Play( SFX_WEAPONS_MISSLELAUNCHER_PROJECTILE_LOOP, &m_WorldPos );
}
                
//==============================================================================

void missile::OnRemove( void )
{
    if( m_SmokeEffect.IsCreated() )
    {
        m_SmokeEffect.SetEnabled( FALSE );
    }

    audio_Stop( m_SfxID );
}

//==============================================================================

void missile::Initialize( const matrix4&   L2W,
                                u32        TeamBits,
                                object::id OriginID,
                                object::id TargetID,
                                s32        Type )
{
    m_WorldPos = L2W.GetTranslation();
    m_Hdg.Setup( L2W.GetRotation() );

    m_OriginID = OriginID;
    m_TargetID = TargetID;
    m_TeamBits = TeamBits;
    m_Type     = Type;
}
                
//==============================================================================

void missile::BeginRender( void )
{
}

//==============================================================================

void missile::Render( void )
{
    m_RocketShape.SetPos( m_WorldPos );
    m_RocketShape.SetRot( m_Hdg.GetRotation() );

    // Visible?
    s32 Visible = IsInViewVolume( m_RocketShape.GetWorldBounds() ) ;
    if( (!Visible) || m_HasExploded )
        return;

    // Draw
    m_RocketShape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );

    // Clip?
    if( Visible == view::VISIBLE_PARTIAL )
        m_RocketShape.Draw( shape::DRAW_FLAG_FOG | shape::DRAW_FLAG_CLIP );
    else
        m_RocketShape.Draw( shape::DRAW_FLAG_FOG );

    tgl.PostFX.AddFX( m_WorldPos, vector2(4.0f, 4.0f), xcolor(255,255,192,38) );
    tgl.PostFX.AddFX( m_WorldPos, vector2(2.0f, 2.0f), xcolor(255,255,192,38) );
}

//==============================================================================

void missile::RenderParticles( void )
{
    if( m_SmokeEffect.IsCreated() )
        m_SmokeEffect.RenderEffect();
}

//==============================================================================

void missile::EndRender( void )
{
}

//==============================================================================

void missile::UnbindOriginID( object::id OriginID )
{
    if( m_OriginID == OriginID )
        m_OriginID = obj_mgr::NullID;
}

//==============================================================================

void missile::UnbindTargetID( object::id TargetID )
{
    if( m_TargetID == TargetID )
        m_TargetID = obj_mgr::NullID;
}

//==============================================================================
