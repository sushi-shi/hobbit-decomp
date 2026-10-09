//==============================================================================
//
//  AutoAim.cpp
//
//==============================================================================

// TODO: Compute "quality" of shot.
// TODO: Visual indicatation of high quality shots.
// TODO: Prioritization of which aim help to use considers quality.

// TODO: Better prediction for arcing.  Consider fuse time.

//==============================================================================
//  INCLUDES
//==============================================================================

#include "AutoAim.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "GameMgr/GameMgr.hpp"
#include "Objects/Vehicles/Vehicle.hpp"
#include "Aimer.hpp"
#include "HUD\hud_Icons.hpp"
#include "..\AADS\Globals.hpp"

//==============================================================================
//  STORAGE
//==============================================================================

xbool UseAutoAimHelp = TRUE;
xbool UseAutoAimHint = TRUE;

static f32 MaxHelpDist   = 100.0f; // max distance

// Random time offsets
static f32 AUTO_AIM_RAND_MIN = -1.0f;
static f32 AUTO_AIM_RAND_MAX = +1.0f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

void DrawAlphaLine( const vector3& P0, const vector3& P1, xcolor C0, xcolor C1 )
{
    draw_Begin( DRAW_LINES, DRAW_USE_ALPHA );
    draw_Color ( C0 );
    draw_Vertex( P0 );
    draw_Color ( C1 );
    draw_Vertex( P1 );
    draw_End();
}

//==============================================================================

void DrawAlphaPoint( const vector3& Pos, xcolor Color )
{
    const view* pView = eng_GetActiveView( 0 );

    vector3 ScreenPos;
    ScreenPos = pView->PointToScreen( Pos );

    if( ScreenPos.Z < 0 )
        return;

    draw_Begin( DRAW_QUADS, DRAW_2D | DRAW_USE_ALPHA );
    {
        draw_Color( Color );
        draw_Vertex( ScreenPos.X-2,  ScreenPos.Y-2, 0 );
        draw_Vertex( ScreenPos.X-2,  ScreenPos.Y+2, 0 );
        draw_Vertex( ScreenPos.X+2,  ScreenPos.Y+2, 0 );
        draw_Vertex( ScreenPos.X+2,  ScreenPos.Y-2, 0 );
    }
    draw_End();
}

//==============================================================================

xbool ComputeAutoAim( const player_object* pPlayer, 
                      const object*        pTarget,
                            xbool          Blend,
                            f32&           Score,
                            vector3&       TargetCenter,
                            vector3&       AimDir,
                            vector3&       AimPos,
                            vector3&       ArcPivot )
{
    Score = -1.0f;

    // Don't bother if target is already dead.
    if( pTarget->GetHealth() == 0.0f )
        return( FALSE );

    // If target is a vehicle, it must either be an enemy vehicle, or have an
    // enemy occupant.
    if( pTarget->GetAttrBits() & object::ATTR_VEHICLE )
    {                   
        vehicle* pVehicle = (vehicle*)pTarget;

        if( pVehicle->GetTeamBits() & pPlayer->GetTeamBits() )
        {
            // Friendly vehicle.  Must have an enemy occupant!
            xbool          Enemy  = FALSE;
            s32            Seats  = pVehicle->GetNumberOfSeats();
            player_object* pOccupant;

            for( s32 i = 0; i < Seats; i++ )
            {
                pOccupant = pVehicle->GetSeatPlayer( i );
                if( (pOccupant) && 
                    ((pOccupant->GetTeamBits() & pPlayer->GetTeamBits()) == 0) )
                {
                    Enemy = TRUE;
                    break;
                }
            }

            if( !Enemy )
                return( FALSE );
        }
        else
        {
            // Enemy vehicle.  No problem.  Just proceed.
        }
    }

    player_object::invent_type Type = pPlayer->GetWeaponCurrentType();

    // Some weapons do not have auto aim.
    if( (Type == player_object::INVENT_TYPE_WEAPON_SNIPER_RIFLE     ) ||
        (Type == player_object::INVENT_TYPE_WEAPON_ELF_GUN          ) ||
        (Type == player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER ) ||
        (Type == player_object::INVENT_TYPE_WEAPON_REPAIR_GUN       ) ||
        (Type == player_object::INVENT_TYPE_WEAPON_TARGETING_LASER  ) ||
        (Type == player_object::INVENT_TYPE_NONE                    ) )
        return( FALSE );

    vector3     ViewZ;
    vector3     ViewPos;
    radian3     ViewRot;
    f32         ViewFOV;

    // Get line of sight.
    pPlayer->GetView( ViewPos, ViewRot, ViewFOV );
    ViewZ.Set( ViewRot.Pitch, ViewRot.Yaw );

    // Is projectile arcing?  (Rather than linear.)
    xbool IsArcing = (Type == player_object::INVENT_TYPE_WEAPON_MORTAR_GUN) ||
                     (Type == player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER);

    // Get position of the weapon muzzle.
    const player_object::weapon_info& WeaponInfo = player_object::GetWeaponInfo( Type );
    const vector3&                    WeaponPos  = pPlayer->GetWeaponFirePos();

    if( Blend )
        TargetCenter = pTarget->GetBlendPainPoint();
    else
        TargetCenter = pTarget->GetPainPoint();

    vector3 TargetVelocity;

    // Lookup delta time
    f32 DeltaTime = tgl.DeltaLogicTime ;

    // Lookup target player
    player_object* pTargetPlayer = NULL ;
    if (pTarget->GetAttrBits() & object::ATTR_PLAYER)
        pTargetPlayer = (player_object*)pTarget ;

    // Get accurate delta time for players (TO DO - for controlled vehicles too?)
    if (pTargetPlayer)
        DeltaTime = pTargetPlayer->GetMove().DeltaTime ;

    // Get velocity.
    TargetVelocity = pTarget->GetAverageVelocity();

    // Reject when behind player.
    if( ViewZ.Dot( TargetCenter - WeaponPos ) < 0.001f )
        return( FALSE );

    // Adjust accuracy if auto aiming for a projectile and not the hud icon
    if (!Blend)
    {
        // If logic hasn't happend to the target object yet, then predict another frame
        if (!(pTarget->GetAttrBits() & object::ATTR_LOGIC_APPLIED))
            TargetCenter += TargetVelocity * DeltaTime ;

        // Take into account the # of moves remaining
        if (pTargetPlayer)
            TargetCenter += TargetVelocity * (f32)pTargetPlayer->GetMovesRemaining() * pTargetPlayer->GetMove().DeltaTime ;

        // Add some time randomness incase taret logic is advanced after targetting
        // (this gives all players are more even ratio)
        TargetCenter += TargetVelocity * x_frand(AUTO_AIM_RAND_MIN, AUTO_AIM_RAND_MAX) * DeltaTime ;
    }

    // Do initial calculation. 
    xbool Hit;
    if( IsArcing )
    {
        Hit = CalculateArcingAimDirection( TargetCenter,                  
                                           TargetVelocity,                
                                           WeaponPos,                     
                                           pPlayer->GetVel(),             
                                           WeaponInfo.VelocityInheritance,
                                           WeaponInfo.MuzzleSpeed,        
                                           WeaponInfo.MaxAge,             
                                           AimDir,                        
                                           AimPos );                      
    }
    else
    {
        Hit = CalculateLinearAimDirection( TargetCenter,                  
                                           TargetVelocity,                
                                           WeaponPos,                     
                                           pPlayer->GetVel(),
                                           WeaponInfo.VelocityInheritance,
                                           WeaponInfo.MuzzleSpeed,        
                                           WeaponInfo.MaxAge,             
                                           AimDir,                        
                                           AimPos );                      
    }    

    // If we couldn't compute a shot here, then we're done.
    if( !Hit )
        return( FALSE );

    // Compute the arc pivot point.
    if( IsArcing )
    {
        ArcPivot  = TargetCenter;
        ArcPivot += (TargetVelocity * GetLastArcingTime());
    }

    // Now, refine the prediction.
    //  - If the prediction is underground, then see where target hits
    //    and account for one deflection.
    //  - If the active weapon is linear, then we want line of sight.
    // 

    // Fire a ray along the linear path the target is following.  If it collides
    // with something, redirect the movement accordingly and run the prediction
    // again.  We will do this only once at most.

    vector3  End;
    vector3  Move;
    collider Ray;

    if( IsArcing )  End = ArcPivot;
    else            End = AimPos;

    Move = End - TargetCenter;

    if( Move.LengthSquared() > 0.01f )
    {
        Ray.RaySetup( NULL, TargetCenter, End );
        ObjMgr.Collide( object::ATTR_PERMANENT, Ray );
        if( Ray.HasCollided() )
        {
            collider::collision  Collision;
            vector3 Rise;
            vector3 Run;
            Ray.GetCollision( Collision );
            Collision.Plane.GetComponents( Move, Run, Rise );
            Move  = Rise * Collision.T;
            Move += Run * ((Collision.T * 0.75f) + 0.25f);  // Use some friction
            End   = TargetCenter + Move;

            if( IsArcing )
            {
                Hit = CalculateArcingAimDirection(
                        End,
                        vector3(0,0,0),
                        WeaponPos,
                        pPlayer->GetVel(),
                        WeaponInfo.VelocityInheritance,
                        WeaponInfo.MuzzleSpeed,
                        WeaponInfo.MaxAge,
                        AimDir,
                        AimPos );

                if( !Hit )
                    return( FALSE );

                ArcPivot = End;
            }
            else
            {
                AimPos = End;
                AimDir = End - WeaponPos;
                AimDir.Normalize();
            }  
        }
    }

    // Aim position must be within "help" range.
    if( pTarget->GetAttrBits() & object::ATTR_PLAYER )
    {
        if( (AimPos - pPlayer->GetPosition()).LengthSquared() > SQR(MaxHelpDist*2.0f) )
            return( FALSE );
    }
    else
    {
        if( (AimPos - pPlayer->GetPosition()).LengthSquared() > SQR(MaxHelpDist) )
            return( FALSE );
    }

    // Need line of sight to target.
    Move = ViewPos - TargetCenter;
    if( Move.LengthSquared() < 0.01f )
        return( FALSE );
    Ray.RaySetup( NULL, ViewPos, TargetCenter, -1, TRUE );
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
    if( Ray.HasCollided() )
        return( FALSE );

    // Need line of sight to aim position.
    Move = ViewPos - AimPos;
    if( Move.LengthSquared() < 0.01f )
        return( FALSE );
    Ray.RaySetup( NULL, ViewPos, AimPos, -1, TRUE );
    ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
    if( Ray.HasCollided() )
        return( FALSE );

    // For arcing, need line of sight from aim position to pivot.
    if( IsArcing )
    {
        Move = ArcPivot - AimPos;
        if( Move.LengthSquared() < 0.01f )
            return( FALSE );
        Ray.RaySetup( NULL, AimPos, ArcPivot, -1, TRUE );
        ObjMgr.Collide( object::ATTR_SOLID_STATIC, Ray );
        if( Ray.HasCollided() )
            return( FALSE );
    }

    // Looks like we have a shot.  Now decide on a score.
    {
        // Our scoring uses the current FOV.  This handles zoom situations.
        // First, find out the "score" of the worst shot for which we will 
        // correct.  This is expressed as a percentage of the FOV.  Then
        // score the candidate shot and subtract the score of the worst 
        // allowable shot.  If the candidate is worse than the worst shot, then
        // the result will be less than zero and will not be considered.

        // Default is 20%.
        f32 FOVPercentage = 0.20f;

        // Override certain cases.

        switch( Type )
        {
        case player_object::INVENT_TYPE_WEAPON_SHOCKLANCE:
        case player_object::INVENT_TYPE_WEAPON_MORTAR_GUN:
            FOVPercentage = 0.05f;
            break;
        }

        f32 Worst = x_cos( ViewFOV * FOVPercentage );
        f32 Dot   = v3_Dot( AimDir, ViewZ );

        Score = Dot - Worst;
    }

    return( TRUE );
}                            

//==============================================================================

void RenderAutoAimHint( void )
{
    object::id     PlayerID( tgl.PC[ tgl.WC ].PlayerIndex, -1 );
    player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID( PlayerID );
    ASSERT(pPlayer->GetType() == object::TYPE_PLAYER) ;

    object*        pTarget;
    vehicle*       pVehicle;

    player_object::invent_type Type = pPlayer->GetWeaponCurrentType();

    f32     BestScore = 0.0f;
    xbool   GotOne    = FALSE;
    vector3 Position;

    ObjMgr.Select( object::ATTR_PLAYER | object::ATTR_TURRET | object::ATTR_VEHICLE );
    while( (pTarget = ObjMgr.GetNextSelected()) )
    {
        pVehicle = NULL;

        // Only attempt to shoot at enemies.
        if( pPlayer->GetTeamBits() & pTarget->GetTeamBits() )
            continue;

        // If target is a player in a vehicle, then shoot at the vehicle itself.
        if( pTarget->GetAttrBits() & object::ATTR_PLAYER )
        {
            player_object* pTargetPlayer = (player_object*)pTarget;
            pVehicle = pTargetPlayer->IsAttachedToVehicle();
            if (pVehicle)
                pTarget = pVehicle ;
        }
        
        //
        // Render.
        //

        xbool IsArcing = (Type == player_object::INVENT_TYPE_WEAPON_MORTAR_GUN) ||
                         (Type == player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER);   

        xbool       Valid;
        f32         Score;
        vector3     AimPos;
        vector3     AimDir;
        vector3     ArcPivot;
        vector3     TargetCenter;
        xcolor      Color1;
        xcolor      Color2;
        object::id  Locked;

        // Try auto aim
        Valid = ComputeAutoAim( pPlayer, 
                                pTarget,
                                TRUE,
                                Score, 
                                TargetCenter, 
                                AimDir, 
                                AimPos, 
                                ArcPivot );

        // Not found?
        if( !Valid )
            continue;

        // If player's view is locked onto target, then use the player view vars for predictor pos icon
        if (pPlayer->GetAutoAimTargetID() == pTarget->GetObjectID())
        {
            // Lookup predictor pos since this is where the hud target icon will be drawn
            Score        = pPlayer->GetAutoAimScore() ;
            AimDir       = pPlayer->GetAutoAimDir() ;
            AimPos       = pPlayer->GetAutoAimPos() ;
            ArcPivot     = pPlayer->GetAutoAimArcPivot() ;
        }

        Locked = pPlayer->GetLockedTarget();

        // If the Locked target is the current target, OR
        // the current target is in a vehicle AND that vehicle is the Locked target...

        if( (Locked == pTarget->GetObjectID()) || 
            ((pVehicle) && (Locked == pVehicle->GetObjectID())) )
        {
            HudIcons.Add( hud_icons::AUTOAIM_TARGET, AimPos, hud_icons::YELLOW, 127 );
            Color1.Set( 191, 191,   0, 127 );
            Color2.Set( 191, 191,   0, 255 );
            Score += 1000.0f;
        }
        else
        {
            Color1.Set( 191,   0,   0, 127 );
            Color2.Set( 191,   0,   0,  63 );
        }

        if( IsArcing )
        {
            DrawAlphaLine( TargetCenter, ArcPivot, Color1, Color1 );
            DrawAlphaLine( ArcPivot,     AimPos,   Color1, Color2 );
        }
        else
        {
            DrawAlphaLine( TargetCenter, AimPos,   Color1, Color2 );
        }

        // Look for the best.
        if( Score > BestScore )
        {
            BestScore = Score;
            GotOne    = TRUE;
            Position  = AimPos;
        }

        // Debug render
/*
        if( FALSE )
        {
            vector3     ViewZ;
            vector3     ViewPos;
            radian3     ViewRot;
            f32         ViewFOV;
            f32         Dot;

            // Get line of sight.
            pPlayer->GetView( ViewPos, ViewRot, ViewFOV );
            ViewZ.Set( ViewRot.Pitch, ViewRot.Yaw );

            Dot = v3_Dot( ViewZ, AimDir );
            if( Dot > HelpAngleCos )
                DrawAlphaPoint( AimPos, XCOLOR_WHITE );
        }
*/
    }
    ObjMgr.ClearSelection();

    if( GotOne )// AND NOT TARGET LOCKED
    {
        DrawAlphaPoint( Position, xcolor( 191, 0, 0, 127 ) );
    }
}

//==============================================================================

xbool GetAutoAimPoint( const player_object* pPlayer, 
                             vector3&       Dir, 
                             vector3&       Point, 
                             object::id&    TargetID )
{
    // AutoAim is only available for human players.  Not bots allowed.
    if( pPlayer->GetType() == object::TYPE_BOT )
        return( FALSE );

    // Cycle thru enemy enemy players and pick best target.

    f32      Score;
    f32      BestScore = 0.0f;
    xbool    GotOne    = FALSE;
    vector3  AimDir;
    vector3  AimPos;
    vector3  ArcPivot;
    vector3  TargetCenter;
    object*  pTarget;
    collider Ray;

    ObjMgr.Select( object::ATTR_PLAYER | object::ATTR_TURRET | object::ATTR_VEHICLE );
    while( (pTarget = ObjMgr.GetNextSelected()) )
    {
        // Only attempt to shoot at enemy turrets and vehicles.
        if( pPlayer->GetTeamBits() & pTarget->GetTeamBits() )
            continue;

        // If target is a player in a vehicle, then shoot at the vehicle itself.
        if( pTarget->GetAttrBits() & object::ATTR_PLAYER )
        {
            player_object* pTargetPlayer = (player_object*)pTarget;
            vehicle*       pVehicle      = pTargetPlayer->IsAttachedToVehicle();

            if( pVehicle )  
                pTarget = (object*)pVehicle;
        }

        // Need an aiming solution.
        if( !ComputeAutoAim( pPlayer, 
                             pTarget, 
                             FALSE,
                             Score, 
                             TargetCenter, 
                             AimDir, 
                             AimPos, 
                             ArcPivot ) )
            continue;

        // Must be better than best so far.
        if( Score > BestScore )
        {
            GotOne    = TRUE;
            BestScore = Score;
            Dir       = AimDir;
            Point     = AimPos;
            TargetID  = pTarget->GetObjectID();
        }
    }
    ObjMgr.ClearSelection();

    return( GotOne );
}

//==============================================================================
