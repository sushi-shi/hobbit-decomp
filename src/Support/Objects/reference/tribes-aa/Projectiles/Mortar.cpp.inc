//==============================================================================
//
//  Mortar.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Mortar.hpp"
#include "LabelSets/Tribes2Types.hpp"
#include "AADS\Globals.hpp"
#include "ParticleObject.hpp"
#include "objects\player\playerobject.hpp"
#include "AudioMgr/Audio.hpp"


//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   MortarCreator;

obj_type_info   MortarTypeInfo( object::TYPE_MORTAR, 
                                "Mortar", 
                                MortarCreator, 
                                0 );

f32 MortarRunFactor  =   0.2f;
f32 MortarRiseFactor =   0.1f;
f32 MortarDragSpeed  =  45.0f;
f32 MortarArmTime    =   2.0f;

//==============================================================================
//  FUNCTIONS
//==============================================================================

object* MortarCreator( void )
{
    return( (object*)(new mortar) );
}

//==============================================================================

void mortar::Initialize( const vector3&   Position,      
                         const vector3&   Direction,     
                               object::id OriginID,
                               object::id ExcludeID,
                               u32        TeamBits,
                               s32        Type )
{
    // Use the ancestor (arcing) to do much of the work.
    const player_object::weapon_info& WeaponInfo = player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_MORTAR_GUN );

    vector3 Velocity = Direction;

    if( Type )  Velocity *= 100.0f;
    else        Velocity *= WeaponInfo.MuzzleSpeed;

    arcing::Initialize( Position, Velocity, OriginID, ExcludeID, TeamBits );

    m_HasExploded = FALSE;

    // Take care of the stuff particular to the mortar.

    m_RunFactor  = MortarRunFactor;
    m_RiseFactor = MortarRiseFactor;
    m_DragSpeed  = MortarDragSpeed;
    m_SoundID    = SFX_WEAPONS_MORTAR_PROJECTILE_LOOP;

    // Create the smoke trail.
    m_SmokeEffect.Create( &tgl.ParticlePool, 
                          &tgl.ParticleLibrary, 
                          PARTICLE_TYPE_MORT_SMOKE, 
                          GetRenderPos(), 
                          vector3(0,0,1), 
                          NULL, 
                          NULL );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_MORTAR ) );

    m_Type = Type;
}

//==============================================================================

void mortar::OnAcceptInit( const bitstream& BitStream )
{
    arcing::OnAcceptInit( BitStream );

    // Take care of the stuff particular to the mortar.
    m_HasExploded = FALSE;

    m_RunFactor  = MortarRunFactor;
    m_RiseFactor = MortarRiseFactor;
    m_DragSpeed  = MortarDragSpeed;
    m_SoundID    = SFX_WEAPONS_MORTAR_PROJECTILE_LOOP;

    m_Type = 0;

    // Create the smoke trail.
    m_SmokeEffect.Create( &tgl.ParticlePool, 
                          &tgl.ParticleLibrary, 
                          PARTICLE_TYPE_MORT_SMOKE, 
                          GetRenderPos(), 
                          vector3(0,0,1), 
                          NULL, 
                          NULL );

    m_Shape.SetShape( tgl.GameShapeManager.GetShapeByType( SHAPE_TYPE_PROJ_MORTAR ) );
}

//==============================================================================

xbool mortar::Impact( const collider::collision& Collision )
{   
    vector3 Pos         = Collision.Point ;
    object* pObjectHit  = (object*)Collision.pObjectHit ;

    s32     HSound;

    arcing::Impact( Collision );

    if( !m_HasExploded )
    {
        if( (m_Age > MortarArmTime) || 
            (pObjectHit->GetAttrBits() & object::ATTR_DAMAGEABLE) )
            // (m_Type == 1) )  // Adding fuse to turret
        {
            pain::type PainType[] = { pain::WEAPON_MORTAR, 
                                      pain::TURRET_MORTAR };

            SpawnExplosion( PainType[m_Type],
                            m_WorldPos, 
                            Collision.Plane.Normal,
                            m_OriginID );

            m_HasExploded = TRUE;

            // Stop smoking now!  (not in 30 days)
            m_SmokeEffect.SetEnabled( FALSE );

            return( FALSE );
        }
    }

    // Play the TINK sound.
    switch( pObjectHit->GetType() )
    {
        case TYPE_BUILDING:
        case TYPE_SCENIC:
            HSound = audio_Play( SFX_WEAPONS_MORTARSHELL_IMPACT_HARD, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;

        default:
            HSound = audio_Play( SFX_WEAPONS_MORTARSHELL_IMPACT_SOFT, &Pos );
            audio_SetVolume( HSound, MIN(1.0f, (m_Velocity.Length() * 0.05f)) );
            break;
    }

    return( FALSE );
}

//==============================================================================

void mortar::OnAdd( void )
{
    s32 SoundID[2] = { SFX_WEAPONS_MORTAR_FIRE, SFX_TURRET_MORTAR_FIRE };

    arcing::OnAdd();

    vector3 Pos = GetRenderPos();

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
        audio_Play( SoundID[m_Type] );
    else
        audio_Play( SoundID[m_Type], &Pos );
}

//==============================================================================

object::update_code mortar::AdvanceLogic( f32 DeltaTime )
{
    object::update_code RetCode = arcing::AdvanceLogic( DeltaTime );
    vector3 Pos = GetRenderPos();

    // Update smoke effect.
    m_SmokeEffect.SetVelocity( (Pos - m_SmokeEffect.GetPosition()) / DeltaTime );
    m_SmokeEffect.UpdateEffect( DeltaTime );

    // Time to blow up after sitting there?
    if( (!m_HasExploded) && 
        ((m_State == STATE_SETTLED) || (m_State == STATE_SLIDING)) && 
        (m_Age > MortarArmTime) )
    {
        m_HasExploded = TRUE;
        m_SmokeEffect.SetEnabled( FALSE );

        pain::type PainType[] = 
        { 
            pain::WEAPON_MORTAR, 
            pain::TURRET_MORTAR 
        };
        
        SpawnExplosion( PainType[m_Type],
                        Pos, 
                        m_SettleNormal,
                        m_OriginID,
                        m_ObjectHitID );
    }

    // Destroy self.
    if( m_HasExploded )
    {
        if( m_SmokeEffect.IsCreated() )
        {
            if( !m_SmokeEffect.IsActive() )
                return( DESTROY );
            else
                RetCode = UPDATE;
        }
        else
            return( DESTROY );
    }

    // Time to take out of the world "just in case"?
    if( m_Age > player_object::GetWeaponInfo( player_object::INVENT_TYPE_WEAPON_MORTAR_GUN ).MaxAge )
    {
        return( DESTROY );
    }

    // Let the ancestor class handle it from here.
    return( RetCode );
}

//==============================================================================

void mortar::Render( void )
{
    if ( !m_HasExploded )
    {
        vector3 Pos = GetRenderPos();

        // Setup shape ready for render prep
        m_Shape.SetPos( Pos );
        m_Shape.SetRot( radian3( 0.0f, m_Age * (PI*4.0f), 0.0f ) );

        // Visible?
        if (!RenderPrep())
            return ;

        // Draw it
        m_Shape.Draw( m_ShapeDrawFlags ) ;

        // Render Glow
        tgl.PostFX.AddFX( Pos, vector2( 6.0f, 6.0f ), xcolor(128,255,128,38) );
        tgl.PostFX.AddFX( Pos, vector2( 4.0f, 4.0f ), xcolor(255,255,255,44) );
    }
}

//==============================================================================

void mortar::RenderParticles( void )
{
    // Render smoke trail.
    m_SmokeEffect.RenderEffect();
}

//==============================================================================
