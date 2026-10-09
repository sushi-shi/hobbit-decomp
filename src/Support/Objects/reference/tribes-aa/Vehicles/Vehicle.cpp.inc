//==============================================================================
//
//  vehicle.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================
#include "../AADS/Globals.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "NetLib/BitStream.hpp"
#include "Vehicle.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "Damage/Damage.hpp"
#include "Audiomgr/audio.hpp"
#include "Objects/Bot/BotObject.hpp"
#include "GameMgr/GameMgr.hpp"
#include "GameMgr/MsgMgr.hpp"
#include "GameMgr/CTF_Logic.hpp"
#include "AADS\Data\UI\Messages.h"
#include "AADS\GameServer.hpp"

static vector3 PassEyeOffset_3rd( 0.0f, 2.25f, -3.75f );
static vector3 PassEyeOffset_1st( 0.0f, 1.5f, 0.15f );
static vector3 PilotEyeOffset_3rd( 0.0f, 3.0f, -12.0f );

//==============================================================================
//  DEBUG STUFF
//==============================================================================

struct veh
{
    xbool   DebugRender;
    xbool   ShowBBox;
    xbool   ShowPainInfo;
    xbool   ShowCollision;
    xbool   ShowSeats;
};

static veh Veh = { FALSE, FALSE, FALSE, FALSE, FALSE, };

//==============================================================================
//  FUNCTIONS
//==============================================================================

vehicle::vehicle( void )
{
    m_CheckTime = 0.0f;
    m_TrueTime = 0.0f;

    // Average velocity vars
    m_VelocityID = 0 ;                       // Current slot in velocity table
    m_AverageVelocity.Zero() ;               // The computed average velocity
    for (s32 i = 0 ; i < MAX_VELS ; i++)
        m_Velocities[i].Zero() ;            // Table of previous velocities (circular list)
}

//==============================================================================
// INITIALIZATION FUNCTIONS
//==============================================================================

void vehicle::Initialize ( const s32      GeomShapeType,    // Shape to user for geometry
                           const s32      CollShapeType,    // Shape to use for collision
                           const vector3& Pos,              // Initial position
                           const radian   InitHdg,          // Initial heading
                           const f32      SeatRadius,       // Radius of seats
                                 u32      TeamBits )
{
    // Setup instance shapes
    m_GeomInstance.SetShape( tgl.GameShapeManager.GetShapeByType( GeomShapeType ) );
    m_CollInstance.SetShape( tgl.GameShapeManager.GetShapeByType( CollShapeType ) );
    m_GeomInstance.SetColor(xcolor(0,0,0,0)) ;

    // Make the geometry use frame 0 reflection map
    m_GeomInstance.SetTextureAnimFPS(0) ;
    m_GeomInstance.SetTextureFrame(0) ;

    // Set scale
    vector3 Scale;
    Scale.Set( 1.0f, 1.0f, 1.0f );
    m_GeomInstance.SetScale( Scale );
    m_CollInstance.SetScale( Scale );

    // Grab bbox that surrounds all geometry equally, no matter what the rotation
    // Also - take all collision models into account eg. mpb
    f32  Radius = 0 ;
    shape* CollShape = m_CollInstance.GetShape() ;
    ASSERT(CollShape) ;
    for (s32 i = 0 ; i < CollShape->GetModelCount() ; i++)
    {
        bbox ModelBBox = CollShape->GetModelByIndex(i)->GetBounds() ;
        Radius = MAX(Radius, ABS(ModelBBox.Min.X)) ;
        Radius = MAX(Radius, ABS(ModelBBox.Min.Y)) ;
        Radius = MAX(Radius, ABS(ModelBBox.Min.Z)) ;
        Radius = MAX(Radius, ABS(ModelBBox.Max.X)) ;
        Radius = MAX(Radius, ABS(ModelBBox.Max.Y)) ;
        Radius = MAX(Radius, ABS(ModelBBox.Max.Z)) ;
    }

    // SB.
    // Don't mess with this (John)!!!
    // If you want to change the bounds with terrain, buildings etc,
    // you must do it in the collision function (apply-physics) otherwise
    // you will totally break the collision!
    // eg. In collision do:
    // BBox MyCollBBox = m_WorldBBox ;
    // Scale MyCollBBox etc..etc..
    // Collider.Extrude(....,MyCollBBox,...)
    m_GeomBBox.Set(vector3(0,0,0), Radius + 0.001f) ;  

    // Setup position
    m_Rot = radian3(0, InitHdg, 0) ;    // Current rotation
    m_TurretRot.Zero() ;
    m_WorldPos = Pos;
    m_WorldBBox = m_GeomBBox ;
    m_WorldBBox.Translate(m_WorldPos) ;

    // Setup render vars
    m_BlendMove.Clear() ;               // Offset to blend drawing
    m_DrawPos = m_WorldPos ;            // Current position vehicle is drawn at
    m_DrawRot = m_Rot ;                 // Current rotation vehicle is drawn at

    // Turret vars
    m_bHasTurret = FALSE ;              // TRUE if vehicle has a turret onboard
    m_TurretSpeedScaler = 1 ;           // Controls input speed
    m_TurretMinPitch = -R_89 ;          // Min pitch of turret (keep at 89 for quaternion::GetRotation() to work!)
    m_TurretMaxPitch = R_89 ;           // Max pitch of turret (keep at 89 for quaternion::GetRotation() to work!)

    // Prepare those seats
    SetupSeats(SeatRadius) ;
    SetAllowEntry( FALSE );             // Dont allow players into vehicle whilst its spawning

    // Setup state
    m_Health    = 100.0f ;
    m_Energy    = 100.0f ;
    m_WeaponEnergy = 1.0f;
    m_Vel.Zero() ;

    // Firing delays
    m_Fire1Delay = 0 ;
    m_Fire2Delay = 0 ;

    m_TeamBits = TeamBits;

    // Default draw flags
    m_IsVisible = 0 ;
    m_DrawFlags = 0 ;

    // Special render vars
    m_DamageTexture      = NULL ;       // Damage texture to use (or NULL)
    m_DamageClutHandle   = -1 ;         // Damage clut to use (or -1)
    m_RenderMaterialFunc = NULL ;       // Special fx render material (or NULL)
}

//==============================================================================
// MISC FUNCTIONS
//==============================================================================

f32 vehicle::GetHealth( void ) const
{
    return( m_Health / 100.0f );
}

//==============================================================================

const xwchar* vehicle::GetLabel( void ) const
{
    return m_Label;
}

//==============================================================================

//==============================================================================
// LOGIC FUNCTIONS
//==============================================================================

void vehicle::ComputeAverageVelocity( void )
{
    s32 i;

    m_Velocities[ m_VelocityID ]  = m_Vel;

    m_VelocityID++;
    if( m_VelocityID >= MAX_VELS )
        m_VelocityID = 0;

    m_AverageVelocity( 0, 0, 0 );

    for( i=0; i<MAX_VELS; i++ )
    {
        m_AverageVelocity += m_Velocities[i];
    }

    m_AverageVelocity /= MAX_VELS;
}

//==============================================================================
// RENDER FUNCTIONS
//==============================================================================

void vehicle::UpdateInstances( void )
{
    // Update graphical instance
    m_GeomInstance.SetPos( GetDrawPos() );
    m_GeomInstance.SetRot( GetDrawRot() );

    // Update collision instance
    m_CollInstance.SetPos( GetDrawPos() );
    m_CollInstance.SetRot( GetDrawRot() );
}

//==============================================================================

void vehicle::Render( void )
{

    // Adjust the transparency myself if need be
    if ( m_TrueTime > 10.0f )       // for when the destroy condition timer is TRUE more than 10 secs.
    {
        xcolor C = m_GeomInstance.GetColor();
        C.A = (u8)(255 * (1.0f - (m_TrueTime - 10.0f) / 5.0f));
        m_GeomInstance.SetColor( C );
    }

    // Totally transparent?
    if (m_GeomInstance.GetColor().A == 0)
    {
        m_IsVisible = view::VISIBLE_NONE ;
        return ;
    }

    // Check to see if camera can see vehicle
    m_IsVisible = IsVisible( m_WorldBBox );
    if(!m_IsVisible)
        return ;

    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Lookup damage texture + clut
    tgl.Damage.GetTexture( GetHealth(), m_DamageTexture, m_DamageClutHandle ) ;

    // Setup draw flags
    m_DrawFlags = shape::DRAW_FLAG_FOG ;

    // Clip?
    if (m_IsVisible == view::VISIBLE_PARTIAL)
        m_DrawFlags |= shape::DRAW_FLAG_CLIP ;

    // Prime z buffer if transparent
    if (m_GeomInstance.GetColor().A != 255)
        m_DrawFlags |= shape::DRAW_FLAG_PRIME_Z_BUFFER | shape::DRAW_FLAG_SORT  ;

    // render the geometry shape instance
    m_GeomInstance.SetFogColor( tgl.Fog.ComputeFog(m_WorldPos) );
    m_GeomInstance.Draw( m_DrawFlags, m_DamageTexture, m_DamageClutHandle, m_RenderMaterialFunc, (void*)this );

    // draw hot points etc
    //m_GeomInstance.DrawFeatures() ;

    // draw collision shape
    //m_CollInstance.Draw( shape::DRAW_FLAG_CLIP | shape::DRAW_FLAG_FOG );
    //m_CollInstance.DrawCollisionModel() ;

    if( Veh.DebugRender )
        DebugRender();
}

//==============================================================================

void vehicle::DebugRender( void )
{
    if( Veh.ShowBBox )
    {
        draw_BBox( m_WorldBBox, XCOLOR_WHITE );
    }

    if( Veh.ShowPainInfo )
    {
        draw_Point ( GetBlendPainPoint(), XCOLOR_RED );
        draw_Sphere( GetBlendPainPoint(), GetPainRadius(), XCOLOR_RED );
    }

    if( Veh.ShowCollision )
    {
        m_CollInstance.DrawCollisionModel();
    }

    if( Veh.ShowSeats )
    {
        // Draw all seats bboxes.
        for( s32 i = 0; i < m_nSeats; i++ )
        {
            seat& Seat = m_Seats[i];

            // Get seat position.
            ASSERT( Seat.HotPoint );
            //vector3 SeatPos = m_GeomInstance.GetHotPointWorldPos( Seat.HotPoint );
            vector3 SeatPos = GetSeatPos( i );

            // Create a trigger box around the seat position.
            bbox TBox = Seat.LocalBBox;
            TBox.Translate( SeatPos );

            draw_BBox( TBox, XCOLOR_BLUE );
            draw_Point( SeatPos, XCOLOR_RED );
        }
    }
}

//==============================================================================

xbool vehicle::HasEnemyOccupant( u32 TeamBits )
{
    s32 i;

    for( i = 0; i < m_nSeats; i++ )
    {
        object* pObject = ObjMgr.GetObjectFromID( m_Seats[i].PlayerID );
        if( pObject && ((pObject->GetTeamBits() & TeamBits) == 0) )
            return( TRUE );
    }

    return( FALSE );
}

//==============================================================================

void vehicle::Get3rdPersonView ( s32 Seat, vector3& Pos, radian3& Rot )
{
    // Special case yaw
    radian3 FreeLookYaw = Rot ;

    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Different seats have different views
    const seat& SeatRef = GetSeat(Seat) ;
    switch(SeatRef.Type)
    {
        case SEAT_TYPE_PASSENGER:
            Pos = PassEyeOffset_3rd;
            Pos.Rotate( m_Rot );
            Pos += GetSeatPos( Seat );
            Rot += m_Rot;
            break;
        
        default:
            // Extract hot point info and use rotation of object
            ASSERT( m_GeomInstance.GetHotPointByType(HOT_POINT_TYPE_VEHICLE_EYE_POS) ) ;
            Pos = PilotEyeOffset_3rd;
            Pos.Rotate( m_Rot );
            Pos.Rotate( FreeLookYaw );
            Pos += m_GeomInstance.GetHotPointWorldPos(HOT_POINT_TYPE_VEHICLE_EYE_POS) ;
            Rot += m_Rot ;
    }
}

//==============================================================================

void vehicle::Get1stPersonView ( s32 Seat, vector3& Pos, radian3& Rot )
{
    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Different seats have different views
    const seat& SeatRef = GetSeat(Seat) ;
    switch(SeatRef.Type)
    {
        case SEAT_TYPE_PASSENGER:
            Pos = PassEyeOffset_1st;
            Pos.Rotate( m_Rot );
            Pos += GetSeatPos( Seat );
            Rot += m_Rot;
            break;

        default:
            // Extract hot point info and use rotation of object
            ASSERT( m_GeomInstance.GetHotPointByType(HOT_POINT_TYPE_VEHICLE_EYE_POS) ) ;
            Pos = m_GeomInstance.GetHotPointWorldPos(HOT_POINT_TYPE_VEHICLE_EYE_POS);
            Rot = m_Rot ;
    }
}


//==============================================================================
// SEAT FUNCTIONS
//==============================================================================

// Override for your vehicle!
void vehicle::SetupSeat( s32 Index, seat& Seat ) 
{
    (void)Index ;
    (void)Seat ;
}

void vehicle::SetupSeats( f32 SeatRadius )
{
    s32 i, h, a ;

    // Make sure instance is setup
    ASSERT(m_GeomInstance.GetShape()) ;
    ASSERT(m_GeomInstance.GetModel()) ;

    // Clear count
    m_nSeats = 0 ;

    // Clear all seats incase none are found
    for (i = 0 ; i < MAX_SEATS ; i++)
    {
        // Setup seat
        seat& Seat = m_Seats[i] ;
        Seat.Type     = SEAT_TYPE_PASSENGER ;
        Seat.HotPoint = NULL ;

        // Setup player types that can sit here (default to none)
        for (a = 0 ; a < player_object::ARMOR_TYPE_TOTAL ; a++)
            Seat.PlayerTypes[a] = FALSE ;

        // Default player to idle anim
        Seat.PlayerAnimType    = ANIM_TYPE_CHARACTER_IDLE ;

        // Do not allow looking or shooting
        Seat.CanLookAndShoot   = FALSE ;

        // No player there right now...
        Seat.PlayerID = obj_mgr::NullID ;

        // Setup default size
        Seat.LocalBBox.Set(vector3(0,0,0), SeatRadius) ;

        // Make twice as high so it's easier to get in
        Seat.LocalBBox.Max.Y += SeatRadius ;
    }

    // Setup seats in order from pilot->passenger
    for (i = SEAT_TYPE_START ; i <= SEAT_TYPE_END ; i++)
    {
        // Convert seat type to hot point type
        s32 HotPointType = -1 ;
        switch(i)
        {
            default: ASSERT(0) ;
            case SEAT_TYPE_PILOT:       HotPointType = HOT_POINT_TYPE_VEHICLE_SEAT_PILOT ;  break ;
//          case SEAT_TYPE_BOMBER:      HotPointType = HOT_POINT_TYPE_VEHICLE_SEAT_BOMBER ; break ;
//          case SEAT_TYPE_GUNNER:      HotPointType = HOT_POINT_TYPE_VEHICLE_SEAT_GUNNER ; break ;
            case SEAT_TYPE_PASSENGER:   HotPointType = HOT_POINT_TYPE_VEHICLE_SEAT_PASS ;   break ;
        }

        // Loop through all model hot points
        model* Model = m_GeomInstance.GetModel() ;
        ASSERT(Model) ;
        for (h = 0 ; h < Model->GetHotPointCount() ; h++)
        {
            hot_point* HotPoint = Model->GetHotPointByIndex(h) ;
            ASSERT(HotPoint) ;

            // Found this seat type?
            if (HotPoint->GetType() == HotPointType)
            {
                // Increase MAX_SEATS if this asserts...
                ASSERT(m_nSeats < MAX_SEATS) ;

                // Setup seat
                seat& Seat = m_Seats[m_nSeats] ;
                Seat.Type     = (seat_type)i ;
                Seat.HotPoint = HotPoint ;
                Seat.PlayerID = obj_mgr::NullID ;

                // Call setup seat function
                SetupSeat(m_nSeats, Seat) ;

                // Update # of seats
                m_nSeats++ ;
            }
        }
    }

    // Make all seats available for players to hop into
    m_nSeatsFree = m_nSeats ;
}

//==============================================================================

const vehicle::seat&  vehicle::GetSeat( s32 Index ) const
{
    // Within range?
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nSeats) ;

    return m_Seats[Index] ;
}

//==============================================================================

vector3 vehicle::GetSeatPos( s32 Index )
{
    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Get seat
    const seat& Seat = GetSeat(Index) ;

    vector3 SeatPos = vector3(0.0f, 0.1f, 0.15f) ;
    SeatPos.Rotate(m_GeomInstance.GetRot()) ;

    SeatPos += m_GeomInstance.GetHotPointWorldPos(Seat.HotPoint) ;

    return SeatPos ;
}

//==============================================================================

void vehicle::FreeSeat( s32 Index )
{
    // Can only get out on server!
    if (!tgl.pServer)
        return ;

    // Within range?
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nSeats) ;

    seat& Seat = m_Seats[Index] ;

    // Make sure seat is taken!
    ASSERT(Seat.PlayerID != obj_mgr::NullID) ;

    player_object* pPlayer = GetSeatPlayer( Index );

    // play the sound
    if ( pPlayer->GetProcessInput() )
        audio_Play( SFX_VEHICLE_DISMOUNT );
    

    // Free seat
    Seat.PlayerID = obj_mgr::NullID ;

    // Update free count
    m_nSeatsFree++ ;
    ASSERT(m_nSeatsFree <= MAX_SEATS) ;

    // Flag that seats have been updated
    m_DirtyBits |= VEHICLE_DIRTY_BIT_SEAT_STATUS ;
}

//==============================================================================

player_object* vehicle::GetSeatPlayer(s32 Index)
{
    ASSERT(Index >= 0) ;
    ASSERT(Index < m_nSeats) ;

    // Get seat
    seat& Seat = m_Seats[Index] ;

    // If player is in seat, try get at him...
    if (Seat.PlayerID != obj_mgr::NullID)
    {
        // Get player object in pilot seat
        object* Object = ObjMgr.GetObjectFromSlot(Seat.PlayerID.Slot) ;
        if (Object)
        {
            // Is this a player or bot?
            if(( Object->GetType() == object::TYPE_PLAYER ) || ( Object->GetType() == object::TYPE_BOT ))
            {
                // Return player
                player_object* pPlayer = (player_object*)Object ;
                return pPlayer ;
            }
        }
    }

    // No player there
    return NULL ;
}

//==============================================================================

void vehicle::SetAllowEntry( xbool State )
{
    m_AllowEntry = State;
}

//==============================================================================

player_object* vehicle::IsPlayerControlled()
{
    // Pilot is always in seat0
    return GetSeatPlayer(0) ;
}

//==============================================================================



//==============================================================================
// COLLISION FUNCTIONS
//==============================================================================

void vehicle::OnCollide( u32 AttrBits, collider& Collider )
{
    (void)AttrBits;

    // Make sure shape instances are in sync with actual object
    UpdateInstances() ;

    // Collide with geometry or collision model polys?
    object* pMovingObject = (object*)Collider.GetMovingObject() ;
    if( pMovingObject )
    {
        switch(pMovingObject->GetType())
        {
            // Collide with geometry for perfect node collision
            case object::TYPE_SATCHEL_CHARGE:
                m_GeomInstance.ApplyCollision(Collider) ;
                return;
                break ;
        }
    }

    // Collide with fast collision model polys
    m_CollInstance.ApplyCollision( Collider );
}

//==============================================================================

s32 vehicle::HitByPlayer( player_object* pPlayer )
{
    ASSERT( pPlayer );
    
    s32 i ;

    // Can only connect on server!
    if (!tgl.pServer)
        return -1 ;

    // no bots, please (unless they have permission...)
    if ( pPlayer->GetType() == object::TYPE_BOT )
    {
        bot_object* pBot = (bot_object*)pPlayer;
        
        if( pBot->IsAllowedInVehicle() == FALSE )
            return -1 ;
    }
   
    // Any seats free?
    if (m_nSeatsFree == 0)
        return -1 ;

    // Can player get in vehicle?
    if (pPlayer->GetVehicleDismountTime() != 0)
        return -1 ;

    // No rides in death vehicle.
    if( m_Health == 0.0f )
        return( -1 );
    
    // Wait until vehicle has fully spawned before allowing any players in
    if( m_AllowEntry == FALSE )
        return( -1 );

    // Look for closest to player
    s32 ClosestSeat = -1 ;
    f32 ClosestDist = F32_MAX ;

    // Check all free seats
    for (i = 0 ; i < m_nSeats ; i++)
    {
        seat& Seat = m_Seats[i] ;
        
        // Empty seat?
        if( Seat.PlayerID == obj_mgr::NullID )
        {
            // Get seat pos
            ASSERT(Seat.HotPoint) ;
            //vector3 SeatPos = m_GeomInstance.GetHotPointWorldPos(Seat.HotPoint) ;
            vector3 SeatPos = GetSeatPos( i );

            // Player must be above seat
            //if (pPlayer->GetPos().Y >= SeatPos.Y)
            {
                // Create a trigger box around the seat pos
                bbox TBox = Seat.LocalBBox ;
                TBox.Translate(SeatPos) ;

                // Inside trigger box?
                if (TBox.Intersect(pPlayer->GetPos()))
                {
                    // Get dist to seat (just compare x + z)
                    vector3 DeltaPos = SeatPos - pPlayer->GetPos() ;
                    //f32 Dist = DeltaPos.LengthSquared() ;
                    f32 Dist = SQR(DeltaPos.X) + SQR(DeltaPos.Z) ;

                    // If closest so far, keep track of it
                    if (Dist < ClosestDist)
                    {
                        ClosestDist = Dist ;
                        ClosestSeat = i ;
                    }
                }
            }
        }
    }

    //   && (Seat.PlayerTypes[pPlayer->GetCharacterType()][pPlayer->GetArmorType()]) )

    // Found a seat?
    if (ClosestSeat != -1)
    {   
        // Armor restricted?
        if( !m_Seats[ClosestSeat].PlayerTypes[ pPlayer->GetArmorType() ] )
        {
            if( pPlayer->GetVehicleDismountTime() <= 0.0f )
            {
                pPlayer->ResetVehicleDismountTime();
                MsgMgr.Message( MSG_CANNOT_PILOT_ARMOR, pPlayer->GetObjectID().Slot );
            }
            return( -1 );
        }

        // Pack restricted for pilot seat?
        if( m_Seats[ClosestSeat].Type == SEAT_TYPE_PILOT )
        {
            switch( pPlayer->GetCurrentPack() )
            {
            case player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_AA:
            case player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_MORTAR:
            case player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_MISSILE:
            case player_object::INVENT_TYPE_DEPLOY_PACK_BARREL_PLASMA:
            case player_object::INVENT_TYPE_DEPLOY_PACK_INVENT_STATION:
            case player_object::INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR:
            case player_object::INVENT_TYPE_ARMOR_PACK_AMMO:
                if( pPlayer->GetVehicleDismountTime() <= 0.0f )
                {
                    pPlayer->ResetVehicleDismountTime();
                    MsgMgr.Message( MSG_CANNOT_PILOT_PACK, pPlayer->GetObjectID().Slot );
                }
                return( -1 );
            }
        }

        // Flag restricted?
        if( (m_Seats[ClosestSeat].Type == SEAT_TYPE_PILOT) &&
            (GameMgr.GetGameType() == GAME_CTF) )
        {
            vector3    Pos;
            s32        Status;
            s32        Slot = pPlayer->GetObjectID().Slot;
            ctf_logic* pCTF = (ctf_logic*)pGameLogic;

            pCTF->GetFlagStatus( 0x01, Status, Pos );
            if( Status == Slot )
            {
                if( pPlayer->GetVehicleDismountTime() <= 0.0f )
                {
                    pPlayer->ResetVehicleDismountTime();
                    tgl.pServer->SendAudio( SFX_POWER_INVENTORYSTATION_DENIED, Slot );
                }
                return( -1 );
            }

            pCTF->GetFlagStatus( 0x02, Status, Pos );
            if( Status == Slot )
            {
                if( pPlayer->GetVehicleDismountTime() <= 0.0f )
                {
                    pPlayer->ResetVehicleDismountTime();
                    tgl.pServer->SendAudio( SFX_POWER_INVENTORYSTATION_DENIED, Slot );
                }
                return( -1 );
            }
        }

        // If we make it here, then the player can take a seat!

        // Keep record
        m_Seats[ClosestSeat].PlayerID = pPlayer->GetObjectID() ;

        // play the sound
        if ( pPlayer->GetProcessInput() )
            audio_Play( SFX_VEHICLE_MOUNT );

        // Update seats free count
        m_nSeatsFree-- ;
        ASSERT(m_nSeatsFree >= 0) ;

        // Success!
        pPlayer->AttachToVehicle( this, ClosestSeat ) ;

        // Flag that seats have been updated
        m_DirtyBits |= VEHICLE_DIRTY_BIT_SEAT_STATUS ;
    }

    // Return seat (if any)
    return ClosestSeat ;
}

//==============================================================================

matrix4* vehicle::GetNodeL2W( s32 ID, s32 NodeIndex )
{
    (void)ID;

    // Make sure instances are uptodate
    UpdateInstances() ;

    // Get vehicle render node
    render_node* RenderNodes = m_GeomInstance.CalculateRenderNodes() ;
    if (RenderNodes)
        return &RenderNodes[NodeIndex].L2W ;
    else
        return NULL ;
}

//==============================================================================


//==============================================================================
// MOVEMENT FUNCTIONS
//==============================================================================

void vehicle::SetVel( const vector3& Vel )
{
    m_Vel = Vel;
}

//==============================================================================

// Position functions
const radian3& vehicle::GetRot( void )
{
    return m_Rot ;
}

//==============================================================================
        
const radian3& vehicle::GetTurretRot( void )
{
    // Should only be calling if vehicle has a turret!
    ASSERT(m_bHasTurret) ;

    return m_TurretRot ;
}

//==============================================================================
        
void vehicle::SetTurretRot( const radian3& Rot )
{
    // Should only be calling if vehicle has a turret!
    ASSERT(m_bHasTurret) ;

    // Changed?
    if (m_TurretRot != Rot)
    {
        // Flag it's changed
        m_DirtyBits |= VEHICLE_DIRTY_BIT_ROT ;

        // Update turret
        m_TurretRot = Rot ;    
    }
}

//==============================================================================

f32 vehicle::GetTurretSpeedScaler( void )
{
    return m_TurretSpeedScaler ;
}

//==============================================================================

radian vehicle::GetTurretMinPitch   ( void )
{
    return m_TurretMinPitch ;
}

//==============================================================================

radian vehicle::GetTurretMaxPitch   ( void )
{
    return m_TurretMaxPitch ;
}

//==============================================================================
// Returns TRUE if there is an object in the way of the collision bbox

xbool vehicle::ObjectInWayOfCollision( const bbox& WorldBBox )
{
    (void)WorldBBox;
/*
    object* pObject;
    xbool   Blocked = FALSE;

    // Only move the turret if there are no players etc. inside the vehicles bbox
    // (It's a cheesy, but safe way so that I don't have to deal with rotational collision!)
    ObjMgr.Select(object::ATTR_ALL, WorldBBox) ;
    while( !Blocked && (pObject = ObjMgr.GetNextSelected()) )
    {
        // Skip checking self
        if (pObject != this)
        {
            // Skip movement if any of these objects are in the way
            switch(pObject->GetType())
            {
                case object::TYPE_PLAYER:
                case object::TYPE_BOT:
                    ASSERT(pObject->GetAttrBits() & object::ATTR_PLAYER) ;
                    // Only take into account if the player is not in a vehicle
                    // (could be pilot seat of tank etc)
                    if ( ((player_object*)pObject)->IsAttachedToVehicle() == NULL )
                        Blocked = TRUE;
                    break ;

                case object::TYPE_CORPSE:
                    Blocked = TRUE;
                    break;
            }
        }
    }
    ObjMgr.ClearSelection() ;

    return( Blocked );
*/

    // There's nothing in the way...
    return FALSE ;
}

//==============================================================================

#define SIGN(__v__)     ( (__v__) == 0 ? 0 : ((__v__ > 0) ? 1 : -1) )

void vehicle::ReturnTurretToRest( f32 DeltaTime )
{
    f32     Delta ;

    // Should only call for vehicles with turrets
    ASSERT(m_bHasTurret) ;

    // Skip if already reset
    if ((m_TurretRot.Pitch == 0) && (m_TurretRot.Yaw == 0))
        return ;

    // Base return speed off input speed scaler
    radian ReturnSpeed = m_TurretSpeedScaler * R_100 * DeltaTime ;

    // Reduce pitch first
    if (m_TurretRot.Pitch != 0)
    {
        Delta = -x_ModAngle2(m_TurretRot.Pitch) ;
        if (ABS(Delta) > ReturnSpeed)
            Delta = SIGN(Delta) * ReturnSpeed ;
        m_TurretRot.Pitch += Delta ;
    }
    else
    // Reduce yaw
    if (m_TurretRot.Yaw != 0)
    {
        Delta = -x_ModAngle2(m_TurretRot.Yaw) ;
        if (ABS(Delta) > ReturnSpeed)
            Delta = SIGN(Delta) * ReturnSpeed ;
        m_TurretRot.Yaw += Delta ;
    }
}

//==============================================================================

const vector3& vehicle::GetDrawPos( void )
{
    // Use actual pos + blend pos
    m_DrawPos = m_WorldPos + (m_BlendMove.Blend * m_BlendMove.DeltaPos) ;

    // adjust for rotation (lower in collision box if level)
    //radian MaxRot;
    //vector3 Size = m_GeomBBox.GetSize();

    //MaxRot = MAX( m_Rot.Roll, m_Rot.Pitch );
    //m_DrawPos.Y -= (( x_cos( MaxRot )) * (Size.Y / 2.0f) );

    return m_DrawPos ;
}

//==============================================================================

const radian3& vehicle::GetDrawRot( void )
{
    // Use actual rot + blend rot
    m_DrawRot = m_Rot + (m_BlendMove.Blend * m_BlendMove.DeltaRot) ;

    // Keep rot normalized
    m_DrawRot.ModAngle2() ;

    return m_DrawRot ;
}

//==============================================================================

const radian3& vehicle::GetDrawTurretRot( void )
{
    return( m_Rot );
}

//==============================================================================
// NET FUNCTIONS
//==============================================================================

void vehicle::TruncateRangedF32(f32& Value, s32 NBits, f32 Min, f32 Max)
{
    // Cap
    if (Value < Min)
        Value = Min ;
    else
    if (Value > Max)
        Value = Max ;

    f32 Mid = (Min + Max) /2 ;
    if (Value != Mid)
        bitstream::TruncateRangedF32(Value, NBits, Min, Max) ;
}

//==============================================================================

void vehicle::WriteRangedF32( bitstream& BitStream, f32 Value, s32 NBits, f32 Min, f32 Max )
{
    f32 Mid = (Min + Max) /2 ;
    if (BitStream.WriteFlag(Value != Mid))
        BitStream.WriteRangedF32(Value, NBits, Min, Max) ;
}

//==============================================================================

void vehicle::ReadRangedF32( const bitstream& BitStream, f32& Value, s32 NBits, f32 Min, f32 Max )
{
    f32 Mid = (Min + Max) /2 ;
    if (BitStream.ReadFlag())
        BitStream.ReadRangedF32(Value, NBits, Min, Max) ;
    else
        Value = Mid ;
}

//==============================================================================
// OTHER FUNCTIONS
//==============================================================================

void vehicle::AddWeaponEnergy     ( f32 Amount )
{
    m_WeaponEnergy += Amount;

    // truncate
    if ( m_WeaponEnergy > 1.0f )
        m_WeaponEnergy = 1.0f;

    if ( m_WeaponEnergy < 0.0f )
        m_WeaponEnergy = 0.0f;
}

//==============================================================================

f32 vehicle::GetWeaponEnergy     ( void )
{
    return m_WeaponEnergy;
}

//==============================================================================

f32 vehicle::GetEnergy( void ) const
{
    return( m_Energy / 100.0f );
}

//==============================================================================

void vehicle::MissileTrack( void )
{
    // Tell all the players on board the vehicle
    for (s32 i = 0 ; i < GetNumberOfSeats() ; i++)
    {
        player_object*  Player = GetSeatPlayer(i) ;
        if (Player)
            Player->MissileTrack() ;
    }
}

//==============================================================================

void vehicle::MissileLock( void )
{
    // Tell all the players on board the vehicle
    for (s32 i = 0 ; i < GetNumberOfSeats() ; i++)
    {
        player_object*  Player = GetSeatPlayer(i) ;
        if (Player)
            Player->MissileLock() ;
    }
}

//==============================================================================

void vehicle::CheckDestroyConditions( f32 DeltaTime )
{
    if ( tgl.ServerPresent )
    {
        // does not apply to MPB's
        if ( GetType() != object::TYPE_VEHICLE_JERICHO2 )
        {
            m_CheckTime += DeltaTime;

            // should we look for player's in the area?
            if ( m_CheckTime > 1.0f )
            {
                // first check to see if the original owner still owns it
                if ( m_Owner != obj_mgr::NullID )
                {
                    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( m_Owner );

                    if ( pPlayer )
                    {
                        if ( pPlayer->GetLastVehiclePurchased() != m_ObjectID )
                            m_Owner = obj_mgr::NullID;
                    }
                    else
                        m_Owner = obj_mgr::NullID;
                }
                else
                {
                    ObjMgr.Select( object::ATTR_PLAYER, 
                                    m_WorldPos,
                                    25.0f );
                    // if there's somebody there, reset the timer, otherwise, keep counting
                    if( ObjMgr.GetNextSelected() == NULL )
                        m_TrueTime += m_CheckTime;
                    else
                        m_TrueTime = 0.0f;
                    ObjMgr.ClearSelection();
                }

                // reset this regardless
                m_CheckTime = 0.0f;
            }
        }
    }
}