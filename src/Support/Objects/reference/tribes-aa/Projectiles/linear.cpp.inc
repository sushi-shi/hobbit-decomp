//==============================================================================
//
//  Linear.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"
#include "ParticleObject.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AudioMgr/Audio.hpp"
#include "netlib/bitstream.hpp"
#include "AADS\GameClient.hpp"

#include "objects\player\playerobject.hpp"
#include <Support/PointLight/PointLight.hpp>

#define MIN_MOVEMENT_SPEED    0.0f
#define MAX_MOVEMENT_SPEED  500.0f

//==============================================================================

linear::linear( void )
{
    m_ShotIndex = 0;
}

//==============================================================================

object::update_code linear::OnAdvanceLogic( f32 DeltaTime )
{
    // Take care of that logic stuff.
    return( AdvanceLogic( DeltaTime ) );
}

//==============================================================================

object::update_code linear::AdvanceLogic( f32 DeltaTime )
{
    xbool   HitSomething = FALSE;

    // Update the age.
    m_Age += DeltaTime;
    m_BlendReady = FALSE;

    // Get the end points of this update's movement segment.
    vector3 OldPos = m_WorldPos;
    vector3 NewPos = m_Start + (m_Movement * m_Age);

    // If not enough movement, just return.
    if( (NewPos - OldPos).LengthSquared() < 0.0001f )
        return( NO_UPDATE );

    // Look for collision.
    // Do NOT test static things like terrain and buildings.
    collider            Ray;
    collider::collision Collision; 
    Ray.RaySetup    ( this, OldPos, NewPos, m_ExcludeID.GetAsS32() );
    ObjMgr.Collide  ( ATTR_SOLID_DYNAMIC | ATTR_DAMAGEABLE, Ray );
    Ray.GetCollision( Collision );

    // Collision with dynamic object?
    if( (Collision.Collided) &&
        (m_Age - (DeltaTime * (1.0f-Collision.T)) < m_Limit) )
    {
        m_Age = (m_Age - DeltaTime) + (DeltaTime * Collision.T);
        m_ImpactPoint      = Collision.Point;
        m_ImpactPlane      = Collision.Plane;
        m_ImpactObjectID   = ((object*)(Collision.pObjectHit))->GetObjectID();
        m_ImpactObjectAttr = ((object*)(Collision.pObjectHit))->GetAttrBits();
        m_ImpactObjectType = ((object*)(Collision.pObjectHit))->GetType();
        HitSomething       = TRUE;
    }
    else
    // Collision with static environment?
    if( m_Age > m_Limit )
    {
        m_ImpactObjectAttr = 0;
        HitSomething = TRUE;
        m_Age = m_Limit;
    }
    else
    // Fizzle?
    if( m_Age > m_Life )
    {
        // On fizzle, destroy self on server, and hide on client.
        if( tgl.ServerPresent )
            return( DESTROY );
        else
        {
            m_Hidden = TRUE;
            if( m_PointLightID != -1 )
            {
                ptlight_Destroy( m_PointLightID );
                m_PointLightID = -1;
            }
        }
    }

    // If we hit something, react.
    if( HitSomething )
    {
        object::update_code Code;

        // See what the descendant class has to say and do.
        Code = Impact( m_ImpactPoint, m_ImpactPlane.Normal );

        // If the collision is fatal, destroy self on server, and hide on client.
        if( Code == DESTROY )
        {
            if( tgl.ServerPresent )
                return( Code );
            else
            {
                m_Hidden = TRUE;
                if( m_PointLightID != -1 )
                {
                    ptlight_Destroy( m_PointLightID );
                    m_PointLightID = -1;
                }
            }
        }
        else
        {
            NewPos = m_Start;
        }
    }

    // The linear still exists (hidden or not), so move it.
    m_WorldPos = NewPos;

    // Update the bounding box.
    vector3 Size( 0.05f, 0.05f, 0.05f );
    m_WorldBBox( m_WorldPos - Size, m_WorldPos + Size );

    // Update the looping sound
    audio_SetPosition( m_SfxHandle, &m_WorldPos );

    // Update the point light.
    if( m_PointLightID != -1 )
    {
        ptlight_SetPosition( m_PointLightID, m_WorldPos );
    }

    // Update object in the world.
    return( UPDATE );
}

//==============================================================================

void linear::Initialize( const matrix4&   L2W, 
                               u32        TeamBits,
                               object::id OriginID,
                               object::id ExcludeID )
{
    vector3 Size( 0.05f, 0.05f, 0.05f );

    m_Limit = 0.0f;

    // Setup IDs
    m_OriginID   = OriginID;
    m_ScoreID    = OriginID;
    m_ExcludeID  = (ExcludeID.Slot==-1) ? (OriginID) : (ExcludeID);

    m_Start = L2W.GetTranslation();
    m_Orient.Identity();
    m_Orient.Rotate( L2W.GetRotation() );

    m_Movement( 0, 0, m_Speed );
    m_Movement   = m_Orient * m_Movement;

    m_Age        = 0.0f;
    m_Hidden     = FALSE;
    m_TeamBits   = TeamBits;
    m_WorldPos   = m_Start;
    m_WorldBBox( m_Start - Size, m_Start + Size );

    m_PointLightID = -1;

    m_Blending = FALSE;
    m_BlendVector(0,0,0);
}

//==============================================================================

void linear::OnAdd( void )
{
    vector3 End = m_Start + (m_Movement * m_Life);

    // See if there is any static collision in this linear's future.
    collider            Ray;
    collider::collision Collision;

    Ray.RaySetup    ( this, m_Start, End );
    ObjMgr.Collide  ( ATTR_SOLID_STATIC, Ray );
    Ray.GetCollision( Collision );

    if( Collision.Collided )
    {
        m_Limit = m_Life * Collision.T;
        m_ImpactPlane      = Collision.Plane;
        m_ImpactPoint      = Collision.Point;
        m_ImpactObjectID   = ((object*)(Collision.pObjectHit))->GetObjectID();
        m_ImpactObjectType = ((object*)(Collision.pObjectHit))->GetType();
    }
    else
    {
        m_Limit = m_Life * 2.0f;
        m_ImpactPoint = End;
        m_ImpactPlane.Setup( End, vector3(0,1,0) );
    }

    // Start the looping sound.
    m_SfxHandle = audio_Play( m_SoundID, &m_WorldPos );

    // Turn down the volume for the guy who shot it.
    object* pObject = ObjMgr.GetObjectFromID( m_OriginID );
    if( (pObject) && (pObject->GetType() == TYPE_PLAYER) )
    {
        player_object* pPlayer = (player_object*)pObject;
        if( pPlayer->GetHasInputFocus() )
            audio_SetVolume( m_SfxHandle, 0.5f );
    }

    // Reset draw vars
    m_ShapeDrawFlags = 0;
}

//==============================================================================

void linear::OnRemove( void )
{
    // Stop the looping sound.
    audio_Stop( m_SfxHandle );

    // Remove the point light.
    if( m_PointLightID != -1 )
    {
        ptlight_Destroy( m_PointLightID );
        m_PointLightID = -1;
    }
}

//==============================================================================

void linear::OnAcceptInit( const bitstream& BitStream )
{
    vector3 Size( 0.05f, 0.05f, 0.05f );
    f32     Speed;

    BitStream.ReadWorldPosCM( m_Start );
    if( BitStream.ReadFlag() )
    {
        BitStream.ReadF32( Speed );
    }
    else
    {
        BitStream.ReadRangedF32( Speed, 7, MIN_MOVEMENT_SPEED, MAX_MOVEMENT_SPEED );
    }

    BitStream.ReadUnitVector( m_Movement, 28 );
    m_Movement *= Speed;

    //x_DebugMsg("LINEAR: (%1.3f,%1.3f,%1.3f) (%1.3f,%1.3f,%1.3f)\n",
    //    m_Start.X,m_Start.Y,m_Start.Z,
    //    m_Movement.X,m_Movement.Y,m_Movement.Z);

    BitStream.ReadObjectID( m_OriginID );

    if( BitStream.ReadFlag() )
        BitStream.ReadObjectID( m_ExcludeID );
    else
        m_ExcludeID = m_OriginID;

    // orient the object
    radian Pitch, Yaw;
    m_Movement.GetPitchYaw( Pitch, Yaw );
    m_Orient.SetRotation( radian3( Pitch, Yaw, 0.0f ) );
    m_Orient.ZeroTranslation();

    m_Age    =  0.0f;
    m_Limit  = m_Life * 2.0f;
    m_Hidden = FALSE;

    m_ImpactObjectID = obj_mgr::NullID;

    m_WorldPos = m_Start;
    m_WorldBBox( m_Start - Size, m_Start + Size );

    m_PointLightID = -1;

    // Set up the blending.
    m_Blending = FALSE;
    m_BlendVector(0,0,0);
    {
        object* pOrigin = ObjMgr.GetObjectFromID( m_OriginID );
        if( pOrigin )
        {
            m_BlendPos    = pOrigin->GetBlendFirePos( m_ShotIndex );
            m_BlendVector = m_BlendPos - m_WorldPos;
            m_BlendReady  = FALSE;
            m_Blending    = TRUE;
        }
    }
}

//==============================================================================

void linear::OnProvideInit( bitstream& BitStream )
{
    //x_DebugMsg("LINEAR: (%1.3f,%1.3f,%1.3f) (%1.3f,%1.3f,%1.3f)\n",
    //    m_Start.X,m_Start.Y,m_Start.Z,
    //    m_Movement.X,m_Movement.Y,m_Movement.Z);

    BitStream.WriteWorldPosCM( m_Start );
    f32 Speed = m_Movement.Length();
    
    if( BitStream.WriteFlag( (Speed<MIN_MOVEMENT_SPEED) || (Speed>MAX_MOVEMENT_SPEED) ) )
    {
        BitStream.WriteF32( Speed );
    }
    else
    {
        BitStream.WriteRangedF32( Speed, 7, MIN_MOVEMENT_SPEED, MAX_MOVEMENT_SPEED );
    }

    vector3 N = m_Movement * (1.0f/Speed);
    BitStream.WriteUnitVector( N, 28 );

    BitStream.WriteObjectID( m_OriginID );
    if( BitStream.WriteFlag( m_OriginID.Slot != m_ExcludeID.Slot ) )
        BitStream.WriteObjectID( m_ExcludeID );
}

//==============================================================================

s32 linear::RenderPrep( void )
{
    // Hidden?
    if( m_Hidden )
        return( 0 );

    // Is object in view?
    s32 Visible = IsInViewVolume( m_WorldBBox );
    if( !Visible )
        return( 0 );

    // Setup draw flags
    if( Visible == view::VISIBLE_PARTIAL )
        m_ShapeDrawFlags = shape::DRAW_FLAG_CLIP | shape::DRAW_FLAG_FOG;
    else
        m_ShapeDrawFlags = shape::DRAW_FLAG_FOG;

    return( Visible );
}

//==============================================================================

f32 LINEAR_BLEND_TIME = 1.0f;

const vector3& linear::GetRenderPos( void )
{
    // See if blending should be complete.
    if( m_Blending && ((m_Age > LINEAR_BLEND_TIME) || (m_Age > m_Limit)) )
    {
        m_Blending = FALSE;
        m_BlendVector(0,0,0);
    }

    // Blending?
    if( m_Blending )
    {
        // Out of date?
        if( !m_BlendReady )
        {
            f32 Factor;

            if( m_Limit < LINEAR_BLEND_TIME )
            {
                Factor = m_Age / m_Limit;
            }
            else
            {
                Factor = m_Age / LINEAR_BLEND_TIME;
            }

            m_BlendPos   = m_WorldPos + m_BlendVector * (1.0f - Factor);
            m_BlendReady = TRUE;
        }

        return( m_BlendPos );
    }

    return( m_WorldPos );
}

//==============================================================================

void linear::SetScoreID( const object::id ScoreID )
{
    m_ScoreID = ScoreID;
}

//==============================================================================

void linear::SetShotIndex( s32 ShotIndex )
{
    m_ShotIndex = ShotIndex;
}

//==============================================================================
