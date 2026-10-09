//==============================================================================
//
//  Beacon.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Beacon.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib\BitStream.hpp"
#include "..\AADS\Globals.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Objects\Player\PlayerObject.hpp"
#include "ParticleObject.hpp"
#include "GameMgr\GameMgr.hpp"
#include "Aimer.hpp"
#include "HUD\hud_Icons.hpp"

//==============================================================================
//  DEFINES
//==============================================================================


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   BeaconCreator;

obj_type_info   BeaconTypeInfo( object::TYPE_BEACON, 
                                "Beacon", 
                                BeaconCreator, 
                                object::ATTR_SOLID_DYNAMIC |
                                object::ATTR_BEACON        |
                                object::ATTR_DAMAGEABLE );

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* BeaconCreator( void )
{
    return( (object*)(new beacon) );
}

//==============================================================================

object::update_code beacon::OnAdvanceLogic( f32 DeltaTime )
{    
    m_Shape.Advance( DeltaTime );

    if( m_Mode == 2 )
        m_AttrBits &= ~object::ATTR_HOT;
    else
        m_AttrBits |=  object::ATTR_HOT;

    if( m_Dead )
        return( DESTROY );
    else
        return( m_Mode == 0 ? UPDATE : NO_UPDATE );
}

//==============================================================================

void beacon::Initialize( const vector3& WorldPos,
                         const vector3& Normal,
                               u32      TeamBits )
{   
    m_WorldPos = WorldPos;
    m_Normal   = Normal;

    Normal.GetPitchYaw( m_WorldRot.Pitch, m_WorldRot.Yaw );
    m_WorldRot.Roll   = R_0;
    m_WorldRot.Pitch += R_90;

    // Setup the shape instance.
    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_BEACON ) );
    m_Shape.SetPos( m_WorldPos );
    m_Shape.SetRot( m_WorldRot );
    m_Shape.SetTextureAnimFPS( 8.0f );

    m_Mode      = 2;            // Marker mode.
    m_Dead      = FALSE;
    m_WorldBBox = m_Shape.GetWorldBounds();
    m_TeamBits  = TeamBits;
    m_Owner     = obj_mgr::NullID;
}

//==============================================================================

void beacon::Initialize( const vector3&   WorldPos,
                               u32        TeamBits,
                               object::id Owner  )    
{
    m_WorldRot.Zero();
    m_WorldPos  = WorldPos;
    m_Normal(0,0,0);

    m_Mode      = 0;            // Laser pointer mode.  No shape needed.
    m_Dead      = FALSE;                                     
    m_AttrBits &= ~ATTR_DAMAGEABLE;
    m_AttrBits &= ~ATTR_SOLID_DYNAMIC;
    m_TeamBits  = TeamBits;
    m_Owner     = Owner;

    m_WorldBBox( m_WorldPos, m_WorldPos );
}

//==============================================================================

void beacon::OnAdd( void )
{
    // Item deployed message.
    pGameLogic->ItemDeployed( m_ObjectID, obj_mgr::NullID );
}

//==============================================================================

void beacon::ToggleMode( void )
{
    if ( m_Mode != 0 )
    {
        m_Mode = 3 - m_Mode;
        m_DirtyBits |= 0x01;
    }
}

//==============================================================================

void beacon::SetPosition( const vector3& Position )
{
    m_WorldPos = Position;
    m_WorldBBox( m_WorldPos, m_WorldPos );
    m_DirtyBits |= 0x02;
}

//==============================================================================

void beacon::DebugRender( void )
{
    draw_Point( m_WorldPos, XCOLOR_YELLOW );
}

//==============================================================================

void beacon::Render( void )
{
    // Get visibility of the beacon.
    s32 Visible = IsInViewVolume( m_WorldBBox );

    // Render the geometry.
    if( m_Mode != 0 )
    {
        // Setup draw flags.
        u32 ShapeDrawFlags = shape::DRAW_FLAG_FOG;
        if( Visible == view::VISIBLE_PARTIAL )
            ShapeDrawFlags |= shape::DRAW_FLAG_CLIP;

        m_Shape.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
        m_Shape.Draw( ShapeDrawFlags );
    }

    // Now render the HUD portion.

    u32 ContextBits = 0x00;
    object::id PlayerID( tgl.PC[tgl.WC].PlayerIndex, -1 );

    // Get the rendering context team bits.
    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    if( !pPlayer )
        return;
    
    ContextBits = pPlayer->GetTeamBits();
    if( !(ContextBits & m_TeamBits) )
        return;

    if( m_Mode == 2 )
    {
        HudIcons.Add( hud_icons::BEACON_MARK, m_WorldPos );
        return;
    }

/* Original code back when beacons existed in 
** Target Mode and on the end of the target laser.

    // If Mode Zero, find the shooter point
    vector3 ThePoint;

    if ( m_Owner != obj_mgr::NullID )
    {
        player_object* pOwner = (player_object*)ObjMgr.GetObjectFromID( m_Owner );
        
        // did the guy die or something?
        if ( !pOwner )
            return;

        ThePoint = pOwner->GetLocalWeaponTargetPt();
    }
    else
        ThePoint = m_WorldPos;

                        
    // Figure out where to draw the other point.  You have a pPlayer pointer.
    // Also, draw the line between the m_WorldPos and your point.
    player_object::invent_type  Type;
    xbool                       IsArcing = FALSE;
    xbool                       CanHit   = FALSE;
    vector3                     TargetVec;
    vector3                     TargetPoint;

    // Get weapon type
    Type  = pPlayer->GetWeaponCurrentType();

    // Sepcial case - player might not be holding a weapon!
    if (Type == player_object::INVENT_TYPE_NONE)
    {
        // Only see a target "aim at" point.
        HudIcons.Add( hud_icons::BEACON_TARGET_AIM, ThePoint );
        return;
    }

    // Linear without motion compensation?
    if( (Type == player_object::INVENT_TYPE_WEAPON_REPAIR_GUN) ||
        (Type == player_object::INVENT_TYPE_WEAPON_TARGETING_LASER) ||
        (Type == player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER) )
    {
        // Only see a target "aim at" point.
        HudIcons.Add( hud_icons::BEACON_TARGET_AIM, ThePoint );
        return;
    }

    // Arcing?
    IsArcing = ( (Type == player_object::INVENT_TYPE_WEAPON_MORTAR_GUN) || 
                 (Type == player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER) );

    // Get the other particulars for the current weapon.
    const player_object::weapon_info& WeaponInfo = player_object::GetWeaponInfo( Type );
    const view* pView = eng_GetActiveView(0);

    if( IsArcing )
    {
        CanHit = CalculateArcingAimDirection( m_WorldPos, vector3(0,0,0),
                                              pView->GetPosition(), pPlayer->GetVel(),
                                              WeaponInfo.VelocityInheritance,
                                              WeaponInfo.MuzzleSpeed,
                                              WeaponInfo.MaxAge,
                                              TargetVec, 
                                              TargetPoint );
    }
    else
    {
        CanHit = CalculateLinearAimDirection( m_WorldPos, vector3(0,0,0),
                                              pView->GetPosition(), pPlayer->GetVel(),
                                              WeaponInfo.VelocityInheritance,
                                              WeaponInfo.MuzzleSpeed,
                                              WeaponInfo.MaxAge,
                                              TargetVec,
                                              TargetPoint );
    }

    // Submit the HUD elements to be rendered.
    if( CanHit )
    {
        HudIcons.Add( hud_icons::BEACON_TARGET_BASE, ThePoint );
        HudIcons.Add( hud_icons::BEACON_TARGET_AIM,  TargetPoint );
        HudIcons.Add( hud_icons::BEACON_TARGET_LINE );
    }
    else
    {
        HudIcons.Add( hud_icons::BEACON_TARGET_AIM,  ThePoint );
    }
**
*/
}

//==============================================================================

object::update_code beacon::OnAcceptUpdate( const bitstream& BitStream, f32 SecInPast )
{
    (void)SecInPast;

    if( BitStream.ReadFlag() )
    {
        BitStream.ReadRangedS32( m_Mode, 0, 2 );
    }

    if( BitStream.ReadFlag() )
    {
        BitStream.ReadWorldPosCM( m_WorldPos );
        m_WorldBBox( m_WorldPos, m_WorldPos );
    }

    return( m_Mode == 0 ? UPDATE : NO_UPDATE );
}

//==============================================================================

void beacon::OnProvideUpdate( bitstream& BitStream, u32& DirtyBits, f32 Priority )
{
    (void)Priority;

    if( BitStream.WriteFlag( DirtyBits & 0x01 ) )
    {
        BitStream.WriteRangedS32( m_Mode, 0, 2 );
    }

    if( BitStream.WriteFlag( DirtyBits & 0x02 ) )
    {
        BitStream.WriteWorldPosCM( m_WorldPos );
    }

    DirtyBits = 0x00;
}

//==============================================================================

void beacon::OnAcceptInit( const bitstream& BitStream )
{
    BitStream.ReadVector      ( m_WorldPos );
    BitStream.ReadRadian3     ( m_WorldRot );
    BitStream.ReadRangedVector( m_Normal, 6, -1.1f, 1.1f );
    BitStream.ReadRangedS32   ( m_Mode, 0, 2 );
    BitStream.ReadTeamBits    ( m_TeamBits );
    BitStream.ReadObjectID    ( m_Owner    );
                             
    if( m_Mode != 0 )
    {
        // Setup the shape instance.
        m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_MISC_BEACON ) );
        m_Shape.SetPos( m_WorldPos );
        m_Shape.SetRot( m_WorldRot );
        m_Shape.SetTextureAnimFPS( 8.0f );

        // Set bbox.
        m_WorldBBox = m_Shape.GetWorldBounds();
    }
    else
    {
        m_WorldBBox( m_WorldPos, m_WorldPos );
    }

    m_Dead = FALSE;
}

//==============================================================================

void beacon::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteVector      ( m_WorldPos );
    BitStream.WriteRadian3     ( m_WorldRot );
    BitStream.WriteRangedVector( m_Normal, 6, -1.1f, 1.1f );
    BitStream.WriteRangedS32   ( m_Mode, 0, 2 );
    BitStream.WriteTeamBits    ( m_TeamBits );
    BitStream.WriteObjectID    ( m_Owner    );
}

//==============================================================================

f32 beacon::GetPainRadius( void ) const
{
    return( 0.1f );
}

//==============================================================================

vector3 beacon::GetPainPoint( void ) const
{
    return( m_WorldPos + (m_Normal * 0.1f) );
}

//==============================================================================

void beacon::OnPain( const pain& Pain )
{
    if( m_Dead )
        return;

    if( ((Pain.DirectHit)   && (Pain.MetalDamage > 0.00f)) ||
        ((Pain.LineOfSight) && (Pain.MetalDamage > 0.75f)) )
    {
        m_Dead = TRUE;
        SpawnExplosion( pain::EXPLOSION_BEACON, 
                        m_WorldPos, 
                        m_Normal, 
                        Pain.OriginID );
    }
}

//==============================================================================

void beacon::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;
    m_Shape.ApplyCollision( Collider );
}

//==============================================================================
