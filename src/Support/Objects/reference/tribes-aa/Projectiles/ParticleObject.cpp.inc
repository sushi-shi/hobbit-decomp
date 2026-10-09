//==============================================================================
//
//  ParticleObject.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include "ParticleObject.hpp"
#include "debris.hpp"
#include <xCore/Entropy/Entropy.hpp>
#include "..\AADS\Globals.hpp"
#include "AudioMgr\Audio.hpp"
#include "LabelSets\Tribes2Types.hpp"
#include <Support/PointLight/PointLight.hpp>
#include "netlib/bitstream.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  TYPE
//==============================================================================

struct explosion_attributes
{
    pain::type      PainType;
    s32             ParticleType;
    effect_initproc EffectInitFn;
    effect_proc     EffectFn;
    u8              R,G,B,A;
    f32             PLRadius;
    f32             PLUpTime;
    f32             PLHoldTime;
    f32             PLDownTime;
    xbool           PLBuildings;
    s16             Debris[6];
    f32             DebrisVel;
    xbool           Debris360;
    u32             SoundID;
};

//==============================================================================
//  STORAGE
//==============================================================================

obj_create_fn   ParticleObjectCreator;

obj_type_info   ParticleObjectTypeInfo( object::TYPE_PARTICLE_EFFECT, 
                                        "ParticleEffect", 
                                        ParticleObjectCreator, 
                                        0 );

//==============================================================================
//  FUNCTION ANNOUNCEMENTS
//==============================================================================

void        SpawnDebris     (       s32*        pCount,
                                    f32         Velocity,
                                    xbool       Full360,
                              const vector3&    WorldPos );
            
void        SpawnDebris     (       pain::type  PainType,
                              const vector3&    WorldPos );

//==============================================================================
//  EFFECT INIT FUNCTIONS
//==============================================================================

static
void InitRotateShapes( pfx_effect& Effect )
{
    s32 nEmit = Effect.GetNumEmitters();
    s32 i;

    // Create random rotations if appropriate for the emitters.
    for( i = 0; i < nEmit; i++ )
    {
        pfx_emitter& Emitter = Effect.GetEmitter(i);

        if( Emitter.GetType() == pfx_emitter_def::SHAPE )
        {
            Emitter.SetRotation( x_frand( R_0, R_360 ) );
        }
    }        
}

//==============================================================================

static
void InitGrenadeExplosion( pfx_effect& Effect )
{
    s32     nEmit = Effect.GetNumEmitters();
    s32     i;
    plane   P;
    vector3 A, B;

    // nobody should mess with the grenade explosion definition
    ASSERT( nEmit > 2 );
    vector3 Normal = Effect.GetNormal();
    P.Setup( Effect.GetPosition(), Effect.GetNormal() );
    P.GetOrthoVectors( A, B );

    // determine angular range per tendril
    s32 nTendrils = nEmit - 2;
    ASSERT( nTendrils > 0 );

    f32 AngStart = ((2 * PI) / nTendrils);
    f32 AngRange = AngStart / 2.0f;

    // create a random rotation for the explosion, and offset it a random amount
    for ( i = 2; i < nEmit; i++ )
    {
        radian  Pitch = x_frand( -R_70, -R_20 );
        radian  Yaw   = x_frand( AngStart * i, AngStart * i + AngRange );
        vector3 Vel( Pitch, Yaw );

        f32 speed = x_frand( 10.0f, 30.0f );

        Vel *= speed;

        pfx_emitter& Emitter = Effect.GetEmitter(i);

        // rotate the vector into plane space
        vector3 NewVel = (A * Vel.X) + (Normal * Vel.Y) + (B * Vel.Z);
        vector3 Decel  = NewVel * -0.1f;

        Emitter.SetVelocity( NewVel );
        Emitter.SetAcceleration( vector3( 0.0f, -19.0f, 0.0f ) + Decel );
    } 
}

//==============================================================================

static
void GrenadeExCB ( pfx_effect& Effect, pfx_emitter& Emitter )
{
    f32 Alpha;

    for ( s32 i = 2; i < Effect.GetNumEmitters(); i++ )
    {
        if ( (&Emitter) == &Effect.GetEmitter(i) )
        {
            Alpha = 1.0f - ( Emitter.GetAge() / Emitter.GetLife() );
            Alpha = MIN( 1.0f, Alpha );
            Alpha = MAX( 0.0f, Alpha );

            Emitter.SetMaxAlpha( Alpha );
            return;
        }
    }
}

//==============================================================================

static
void InitRotateAll( pfx_effect& Effect )
{
    s32 nEmit = Effect.GetNumEmitters();
    s32 i;

    // Randomly rotate each emitter.
    for( i = 0; i < nEmit; i++ )
    {
        pfx_emitter& Emitter = Effect.GetEmitter(i);
        Emitter.SetRotation( x_frand( R_0, R_360 ) );
    }        
}

//==============================================================================

static
void InitSatchelExplosion( pfx_effect& Effect )
{
    s32 i;
    s32 nEmit  = Effect.GetNumEmitters();
    f32 Offset = 3.0f;

    // Randomly rotate and offset each emitter.
    for( i = 1; i < nEmit; i++ )
    {
        pfx_emitter& Emitter = Effect.GetEmitter(i);
        Emitter.SetRotation( x_frand( R_0, R_360 ) );
        vector3 Pos = Emitter.GetPosition();
        Pos += vector3( x_frand(-Offset, Offset), x_frand(0.0f, Offset), x_frand(-Offset, Offset) );
        Emitter.SetPosition( Pos );
    }        
}

//==============================================================================
//  EXPLOSION ATTRIBUTES
//==============================================================================

static explosion_attributes ExpAttr[] = 
{ //                                                                                                                 ----------------------Point Light-----------------------      ----------Debris-----------------------
  //                                                                                                                 ------Color-------              Up    Hold   Down    On    
  //  Pain Type ------------------------  Particle Type                Effect Init             Effect Call Back        R    G    B    A    Radius   Time   Time   Time  Bldgs?     S       M       L          Vel    360     Sound
    { pain::WEAPON_DISC,                  PARTICLE_TYPE_EXP_DISC,      InitRotateShapes,       NULL,                 192, 192, 255,  64,     5.0f,  0.0f,  0.0f,  2.5f,  TRUE,  { -2,  0,  0,  0,  0,  0, }, 30.0f,  FALSE,  SFX_WEAPONS_SPINFUSOR_IMPACT         }, 
    { pain::WEAPON_CHAINGUN,              PARTICLE_TYPE_EXP_BULLET,    NULL,                   NULL,                 255, 255, 107,  64,     0.8f,  0.0f,  0.1f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
    { pain::WEAPON_BLASTER,               PARTICLE_TYPE_EXP_BLASTER,   NULL,                   NULL,                 255, 192, 192,  64,     3.0f,  0.0f,  0.1f,  0.4f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_BLASTER_IMPACT           },
    { pain::WEAPON_PLASMA,                PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_PLASMA_PROJECTILE_IMPACT },
    { pain::WEAPON_GRENADE,               PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   GrenadeExCB,          255, 255,   0,  64,     2.0f,  0.0f,  0.1f,  0.2f,  TRUE,  {  2,  2,  0,  0,  0,  0, }, 30.0f,   TRUE,  SFX_WEAPONS_GRENADE_IMPACT           },
    { pain::WEAPON_MORTAR,                PARTICLE_TYPE_EXP_MORTAR,    InitRotateAll,          NULL,                 192, 255, 192, 128,    25.0f,  0.0f,  0.0f,  2.0f, FALSE,  {  5,  4,  0,  0,  0,  0, }, 40.0f,   TRUE,  SFX_WEAPONS_MORTAR_IMPACT            },
    { pain::WEAPON_MISSILE,               PARTICLE_TYPE_EXP_MISSILE,   InitRotateAll,          NULL,                 255, 255, 128, 128,     5.0f,  0.0f,  0.0f,  2.0f,  TRUE,  {  2,  6,  0,  0,  1,  1, }, 30.0f,   TRUE,  SFX_TURRET_MISSILE_IMPACT            },
    { pain::WEAPON_ELF,                   PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
    { pain::WEAPON_LASER,                 PARTICLE_TYPE_EXP_LASER,     NULL,                   NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_SNIPERRIFLE_IMPACT       },
    { pain::WEAPON_LASER_HEAD_SHOT,       PARTICLE_TYPE_EXP_LASER,     NULL,                   NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_SNIPERRIFLE_IMPACT       },
    { pain::WEAPON_GRENADE_HAND,          PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   GrenadeExCB,          255, 255,   0,  64,     2.0f,  0.0f,  0.1f,  0.2f,  TRUE,  {  3,  3,  0,  0,  0,  0, }, 30.0f,  FALSE,  SFX_WEAPONS_GRENADE_IMPACT           },
    { pain::WEAPON_GRENADE_FLASH,         PARTICLE_TYPE_FLASH_EXP,     InitRotateShapes,       NULL,                 255, 255, 255, 128,     5.0f,  1.0f,  1.0f,  1.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_GRENADE_FLASH_EXPLODE    },
    { pain::WEAPON_GRENADE_CONCUSSION,    PARTICLE_TYPE_CONC_EXP,      NULL,                   NULL,                 178, 228, 255,  64,     5.0f,  0.0f,  0.1f,  0.4f,  TRUE,  { -6,  0,  0,  0,  0,  0, }, 50.0f,  FALSE,  SFX_WEAPONS_GRENADE_FLASH_EXPLODE    },
    { pain::WEAPON_MINE,                  PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                 255, 225,   0,  64,     3.0f,  0.0f,  0.2f,  3.0f,  TRUE,  {  2,  1,  0,  0,  0,  0, }, 30.0f,  FALSE,  SFX_WEAPONS_MINE_DETONATE            },
/**/{ pain::WEAPON_MINE_FIZZLE,           PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                 255, 225,   0,  64,     3.0f,  0.0f,  0.2f,  3.0f,  TRUE,  {  1,  1,  0,  0,  0,  0, }, 25.0f,  FALSE,  SFX_WEAPONS_MINE_FIZZLE              },
/**/{ pain::WEAPON_SATCHEL,               PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  { 10, 10,  1,  1,  1,  1, }, 60.0f,  FALSE,  SFX_PACK_SATCHEL_EXPLOSION           },
/**/{ pain::WEAPON_SATCHEL_SMALL,         PARTICLE_TYPE_SATCHEL_SMALL, InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  {  6,  6,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_PACK_SATCHEL_EXPLOSION           },
/**/{ pain::WEAPON_SATCHEL_FIZZLE,        PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  2,  2,  0,  0,  0,  0, }, 20.0f,  FALSE,  SFX_WEAPONS_MINE_FIZZLE              },
/**/{ pain::WEAPON_TANK_GUN,              PARTICLE_TYPE_EXP_BULLET,    NULL,                   NULL,                 255, 255, 107,  64,     0.8f,  0.0f,  0.1f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::WEAPON_TANK_MORTAR,           PARTICLE_TYPE_EXP_MORTAR,    InitRotateAll,          NULL,                 192, 255, 192, 128,    25.0f,  0.0f,  0.0f,  2.0f, FALSE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::WEAPON_SHRIKE_GUN,            PARTICLE_TYPE_EXP_BLASTER,   NULL,                   NULL,                 255, 192, 192,  64,     3.0f,  0.0f,  0.1f,  0.4f,  TRUE,  {  2,  0,  0,  0,  0,  0, }, 20.0f,  FALSE,  SFX_WEAPONS_BLASTER_IMPACT           },
/**/{ pain::WEAPON_BOMBER_GUN,            PARTICLE_TYPE_EXP_BULLET,    NULL,                   NULL,                 255, 255, 107,  64,     0.8f,  0.0f,  0.1f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_BLASTER_IMPACT           },
/**/{ pain::WEAPON_BOMBER_BOMB,           PARTICLE_TYPE_EXP_BOMB,      InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 10, 10,  0,  0,  0,  0, }, 60.0f,  FALSE,  SFX_TURRET_MORTAR_IMPACT   /*DMT*/   },
/**/{ pain::TURRET_AA,                    PARTICLE_TYPE_EXP_BLASTER,   NULL,                   NULL,                 255, 192, 192,  64,     3.0f,  0.0f,  0.1f,  0.4f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_TURRET_AATURRET_IMPACT           },
/**/{ pain::TURRET_ELF,                   PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::TURRET_PLASMA,                PARTICLE_TYPE_EXP_DISC,      InitRotateShapes,       NULL,                 192, 192, 255,  64,     5.0f,  0.0f,  0.0f,  2.5f,  TRUE,  {  0,  6,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_TURRET_PLASMA_IMPACT             },
/**/{ pain::TURRET_MISSILE,               PARTICLE_TYPE_EXP_MISSILE,   InitRotateAll,          NULL,                 255, 255, 128, 128,     5.0f,  0.0f,  0.0f,  2.0f,  TRUE,  {  2,  2,  0,  0,  2,  2, }, 50.0f,   TRUE,  SFX_TURRET_MISSILE_IMPACT            },
/**/{ pain::TURRET_MORTAR,                PARTICLE_TYPE_EXP_MORTAR,    InitRotateAll,          NULL,                 192, 255, 192, 128,    25.0f,  0.0f,  0.0f,  2.0f, FALSE,  { 10,  8,  0,  0,  0,  0, }, 60.0f,  FALSE,  SFX_VEHICLE_THUNDERSWORD_BOMB_EXPLOSION /*DMT*/ },
/**/{ pain::TURRET_SENTRY,                PARTICLE_TYPE_EXP_BLASTER,   NULL,                   NULL,                 255, 192, 192,  64,     3.0f,  0.0f,  0.1f,  0.4f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_TURRET_SENTRY_IMPACT             },
/**/{ pain::TURRET_CLAMP,                 PARTICLE_TYPE_EXP_BLASTER,   NULL,                   NULL,                 255, 192, 192,  64,     3.0f,  0.0f,  0.1f,  0.4f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_TURRET_SPIDER_IMPACT             },
/**/{ pain::EXPLOSION_GENERATOR,          PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 18, 18,  6,  6, 12, 12, }, 50.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           }, 
/**/{ pain::EXPLOSION_SENSOR_LARGE,       PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 18, 18,  6,  6, 12, 12, }, 50.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_SENSOR_MEDIUM,      PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 12, 12,  2,  2,  8,  8, }, 40.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_SENSOR_REMOTE,      PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                 255, 225,   0,  64,     3.0f,  0.0f,  0.2f,  3.0f,  TRUE,  {  4,  2,  0,  0,  0,  0, }, 30.0f,  FALSE,  SFX_WEAPONS_MINE_FIZZLE              },
/**/{ pain::EXPLOSION_TURRET_FIXED,       PARTICLE_TYPE_EXP_TURRET,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 18, 18,  6,  6, 12, 12, }, 50.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION03           },
/**/{ pain::EXPLOSION_TURRET_SENTRY,      PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  {  2,  4,  0,  0,  1,  1, }, 40.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_TURRET_CLAMP,       PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                 255, 225,   0,  64,     3.0f,  0.0f,  0.2f,  3.0f,  TRUE,  {  4,  2,  0,  0,  1,  1, }, 30.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_STATION_INVEN,      PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 12, 12,  2,  2,  8,  8, }, 50.0f,  FALSE,  SFX_EXPLOSIONS_EXPLOSION03           },
/**/{ pain::EXPLOSION_STATION_VEHICLE,    PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 12, 12,  2,  2,  8,  8, }, 50.0f,  FALSE,  SFX_EXPLOSIONS_EXPLOSION03           },
/**/{ pain::EXPLOSION_STATION_DEPLOYED,   PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  { 12, 12,  2,  2,  8,  8, }, 30.0f,  FALSE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_BEACON,             PARTICLE_TYPE_EXP_PLASMA,    InitRotateAll,          NULL,                 255, 172,   0,  64,     3.0f,  0.0f,  1.0f,  2.5f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  SFX_WEAPONS_MINE_FIZZLE              },
/**/{ pain::EXPLOSION_GRAV_CYCLE,         PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  { 12, 12,  1,  1,  6,  6, }, 40.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_TANK,               PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::EXPLOSION_MPB,                PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::EXPLOSION_SHRIKE,             PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  { 16, 16,  2,  2,  8,  8, }, 50.0f,   TRUE,  SFX_EXPLOSIONS_EXPLOSION02           },
/**/{ pain::EXPLOSION_BOMBER,             PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  { 18, 18,  6,  6, 10, 10, }, 50.0f,   TRUE,  SFX_PACK_SATCHEL_EXPLOSION           },
/**/{ pain::EXPLOSION_TRANSPORT,          PARTICLE_TYPE_EXP_SATCHEL,   InitSatchelExplosion,   NULL,                 255, 172,   0,  64,    20.0f,  0.0f,  1.0f,  2.5f, FALSE,  { 20, 20,  6,  6, 12, 12, }, 50.0f,   TRUE,  SFX_PACK_SATCHEL_EXPLOSION           },
/**/{ pain::HIT_BY_ARMOR_LIGHT,           PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::HIT_BY_ARMOR_MEDIUM,          PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::HIT_BY_ARMOR_HEAVY,           PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::HIT_BY_GRAV_CYCLE,            PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  2,  0,  0,  0,  1,  0, }, 20.0f,   TRUE,  0                                    },
/**/{ pain::HIT_BY_TANK,                  PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,   TRUE,  0                                    },
/**/{ pain::HIT_BY_MPB,                   PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,   TRUE,  0                                    },
/**/{ pain::HIT_BY_SHRIKE,                PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  2,  0,  0,  0,  1,  0, }, 20.0f,   TRUE,  0                                    },
/**/{ pain::HIT_BY_BOMBER,                PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  2,  0,  0,  0,  1,  0, }, 20.0f,   TRUE,  0                                    },
/**/{ pain::HIT_BY_TRANSPORT,             PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  2,  0,  0,  0,  1,  0, }, 20.0f,   TRUE,  0                                    },
/**/{ pain::HIT_IMMOVEABLE,               PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_VEHICLE_SPAWN,           PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  { 10, 10,  0,  0,  5,  5, }, 40.0f,   TRUE,  SFX_WEAPONS_MINE_FIZZLE              },
/**/{ pain::MISC_VEHICLE_SPAWN_FORCE,     PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_FORCE_FIELD,             PARTICLE_TYPE_EXP_GRENADE,   InitGrenadeExplosion,   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  { 10, 10,  0,  0,  5,  5, }, 30.0f,   TRUE,  SFX_WEAPONS_MINE_FIZZLE              },        
/**/{ pain::MISC_BOUNDS_HURT,             PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_BOUNDS_DEATH,            PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_FLIP_FLOP_FORCE,         PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_FLAG_TO_VEHICLE,         PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::MISC_PLAYER_SUICIDE,          PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::REPAIR_VIA_PACK,              PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::REPAIR_VIA_FIXED_INVEN,       PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
/**/{ pain::REPAIR_VIA_DEPLOYED_INVEN,    PARTICLE_TYPE_NONE,          NULL,                   NULL,                   0,   0,   0, 255,     0.0f,  0.0f,  0.0f,  0.0f,  TRUE,  {  0,  0,  0,  0,  0,  0, },  0.0f,  FALSE,  0                                    },
};                                                                                                                                 

//==============================================================================
//  UTILITY FUNCTIONS
//==============================================================================

object* ParticleObjectCreator( void )
{
    return( (object*)(new particle_object) );
}

//==============================================================================
//  CLASS FUNCTIONS
//==============================================================================

object::update_code particle_object::OnAdvanceLogic( f32 DeltaTime )
{
    // Update the age.
    m_Age += DeltaTime;

    if( (m_CreateVisuals) && (!m_DebrisCreated) && (m_Age > 0.125f) )
    {
        explosion_attributes& Attr = ExpAttr[m_PainType];

        m_DebrisCreated = TRUE;

        if( Attr.Debris[0] != 0 )
        {   
            // Double number of small bits when terrain is hit.
            // If number is negative, then only make bits when terrain is hit.
            // Vary number of bits, too.

            s32 C[6] = { Attr.Debris[0], Attr.Debris[1], 
                         Attr.Debris[2], Attr.Debris[3], 
                         Attr.Debris[4], Attr.Debris[5] };

            if( m_ObjectHitType == object::TYPE_TERRAIN )
            {
                if( C[0] < 0 )      // If number is negative (and on terrain)...
                    C[0] = -C[0];   
                C[0] <<= 1;
                C[1] <<= 1;
            }
            else
            {
                if( C[0] < 0 )      // If number is negative (and NOT on terrain)...
                    C[0] = 0;
            }

            C[0] = x_irand( C[0] >> 2, C[0] );
            C[1] = x_irand( C[1] >> 2, C[1] );
            C[2] = x_irand( C[2] >> 2, C[2] );
            C[3] = x_irand( C[3] >> 2, C[3] );
            C[4] = x_irand( C[4] >> 2, C[4] );
            C[5] = x_irand( C[5] >> 2, C[5] );

            SpawnDebris( C,
                         Attr.DebrisVel,
                         Attr.Debris360,
                         m_WorldPos );
        }
    }

    // Update Particle Effect
    if( m_ParticleEffect.IsCreated() )
    {
        m_ParticleEffect.UpdateEffect( DeltaTime );

        // Done?
        if( !m_ParticleEffect.IsActive() && (tgl.ServerPresent) )
        {
            return( DESTROY );
        }
    }
    else
        return( DESTROY );

    return( NO_UPDATE );
}

//==============================================================================

void particle_object::Initialize( const vector3&    Position, 
                                  const vector3&    Normal,
                                        pain::type  PainType,
                                  const object::id  ObjectHitID,
                                        f32         Intensity )
{
    m_Age         = 0.0f;
    m_Normal      = Normal;
    m_PainType    = PainType;
    m_WorldPos    = Position;
    m_Intensity   = MINMAX( 0.0f, Intensity, 1.0f );
    m_WorldBBox.Set( m_WorldPos, 10.0f );

    // Store the type of the object we hit.
    object* pObj  = ObjMgr.GetObjectFromID( ObjectHitID );
    if( pObj == NULL )      
        m_ObjectHitType = TYPE_NULL;
    else                    
        m_ObjectHitType = pObj->GetType();
}

//==============================================================================

void particle_object::Render( void )
{
    // Determine if it is in view.
    const view* pView = eng_GetActiveView(0);
    if( !pView->BBoxInView( m_WorldBBox, view::WORLD ) )
        return;

    if( m_ParticleEffect.IsCreated() )
        m_ParticleEffect.RenderEffect();
}

//==============================================================================

void particle_object::OnProvideInit( bitstream& BitStream )
{
    BitStream.WriteVector      ( m_WorldPos );
    BitStream.WriteRangedVector( m_Normal, 10, -1.0f, 1.0f );
    BitStream.WriteRangedS32   ( m_PainType, 0, pain::PAIN_END_OF_LIST );
    BitStream.WriteRangedS32   ( m_ObjectHitType, TYPE_NULL, TYPE_END_OF_LIST );
    BitStream.WriteRangedF32   ( m_Intensity, 8, 0.0f, 1.0f );
}

//==============================================================================

void particle_object::OnAcceptInit( const bitstream& BitStream )
{
    BitStream.ReadVector      ( m_WorldPos );
    BitStream.ReadRangedVector( m_Normal, 10, -1.0f, 1.0f );
    BitStream.ReadRangedS32   ( (s32&)m_PainType, 0, pain::PAIN_END_OF_LIST );
    BitStream.ReadRangedS32   ( (s32&)m_ObjectHitType, TYPE_NULL, TYPE_END_OF_LIST );
    BitStream.ReadRangedF32   ( m_Intensity, 8, 0.0f, 1.0f );

    m_Age = 0.0f;
    m_WorldBBox.Set( m_WorldPos, 10.0f );
}

//==============================================================================

void particle_object::OnAdd( void )
{
    explosion_attributes& Attr = ExpAttr[m_PainType];

    ASSERT( Attr.PainType == m_PainType );

    // Sound!
    audio_Play( Attr.SoundID, &m_WorldPos );

    // Create visuals ONLY if there is a reasonable chance that the player will
    // see them.

    m_CreateVisuals = FALSE;

    for( s32 i = 0; (i < tgl.NLocalPlayers) && (!m_CreateVisuals); i++ )
    {
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromSlot( tgl.PC[i].PlayerIndex );
        if( !pPlayer )
        {
            m_CreateVisuals = TRUE;
            break;
        }
        view&   View   = *(pPlayer->GetView());
        vector3 ViewPt = View.GetPosition();
        plane   Plane;
        f32     Distance;
        Plane.Setup( ViewPt, View.GetViewZ() );
        Distance = Plane.Distance( m_WorldPos );
        if( (Distance > -50.0f) && (Distance < 750.0f) )
        {
            if( (m_WorldPos - ViewPt).LengthSquared() < SQR(750.0f) )
                m_CreateVisuals = TRUE;
        }
    }

    // Start the particle effect.
    if( Attr.ParticleType != PARTICLE_TYPE_NONE )
    {
        m_ParticleEffect.Create( &tgl.ParticlePool, 
                                 &tgl.ParticleLibrary, 
                                 Attr.ParticleType,
                                 m_WorldPos, 
                                 m_Normal, 
                                 Attr.EffectFn,
                                 Attr.EffectInitFn );
    }

    // If we are creating the other visuals, then do so.
    if( m_CreateVisuals )
    {
        // Light!
        if( Attr.PLRadius > 0.0f )
        {
            ptlight_Create( m_WorldPos + m_Normal * 0.1f,
                            Attr.PLRadius,
                            xcolor( Attr.R, Attr.G, Attr.B, Attr.A ),
                            Attr.PLUpTime,
                            Attr.PLHoldTime,
                            Attr.PLDownTime,
                            Attr.PLBuildings );
        }

        // Vehicle death!
        if( IN_RANGE( pain::EXPLOSION_GRAV_CYCLE, m_PainType, pain::EXPLOSION_TRANSPORT ) )
        {
            SpawnDebris( m_PainType, m_WorldPos );
        }
    }

    m_DebrisCreated = FALSE;

    // Special cases.
    switch( Attr.ParticleType )
    {
    case PARTICLE_TYPE_EXP_BULLET:
        // "P'tang!"
        PlayBulletSound();
        // Do not permit "floating" bullet holes!
        if( (m_ObjectHitType == TYPE_TERRAIN ) ||
            (m_ObjectHitType == TYPE_BUILDING) ||
            (m_ObjectHitType == TYPE_SCENIC  ) )
            AddBulletHole( m_WorldPos, m_Normal );
        break;

    default:
        break;
    }
}

//==============================================================================

void particle_object::PlayBulletSound( void )
{
    u32 Building[] = 
    { 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_01, 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_02, 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_03, 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_04, 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_05, 
        SFX_WEAPONS_CHAINGUN_IMPACT_BUILDING_06, 
    };

    u32 Player[] = 
    {
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_01,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_02,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_03,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_04,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_05,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_06,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_07,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_08,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_09,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_10,
        SFX_WEAPONS_CHAINGUN_IMPACT_ARMOR_11,
    };

    u32 Default[] = 
    {
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_01,
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_02,
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_03,
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_04,
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_05,
        SFX_WEAPONS_CHAINGUN_IMPACT_TERRAIN_06,
    };    

    switch( m_ObjectHitType )
    {
        case TYPE_BUILDING:
            audio_Play( Building[ x_irand( 0, (sizeof( Building ) - 1) >> 2 ) ], &m_WorldPos );
            break;

        case TYPE_PLAYER:                                          
        case TYPE_BOT:                                          
            audio_Play( Player  [ x_irand( 0, (sizeof( Player   ) - 1) >> 2 ) ], &m_WorldPos );
            break;

        default:
            audio_Play( Default [ x_irand( 0, (sizeof( Default  ) - 1) >> 2 ) ], &m_WorldPos );
            break;
    }
}

//==============================================================================

void SpawnExplosion( pain::type      PainType,
                     const vector3&  Position,
                     const vector3&  Normal,
                     object::id      OriginID,
                     object::id      VictimID,
                     f32             Scalar )
{
    if( !tgl.ServerPresent )
        return;

    if( ExpAttr[PainType].ParticleType != PARTICLE_TYPE_NONE )
    {
        // Create and add the explosion object to the world.
        particle_object* pExplosion = (particle_object*)ObjMgr.CreateObject( object::TYPE_PARTICLE_EFFECT );
        pExplosion->Initialize( Position, Normal, PainType, VictimID, Scalar );
        ObjMgr.AddObject( pExplosion, obj_mgr::AVAILABLE_SERVER_SLOT );
    }

    // Dish out some pain.
    ObjMgr.InflictPain( PainType, Position, OriginID, VictimID, Scalar );
}

//==============================================================================

void SpawnDebris(       s32*     pCount,
                        f32      Velocity,
                        xbool    Full360,
                  const vector3& WorldPos )
{
    s32 Flame[3] = { pCount[5], 
                     pCount[3], 
                     pCount[1] };
    s32 Total[3] = { pCount[5] + pCount[4], 
                     pCount[3] + pCount[2], 
                     pCount[1] + pCount[0] };
    s32     i, j;
    debris* pDebris;
    vector3 Vel;
    f32     Low = Full360 ? -Velocity : 0.0f;

    for( i = 0; i <        3; i++ )
    for( j = 0; j < Total[i]; j++ )
    {
        Vel.X = x_frand( -Velocity, +Velocity );
        Vel.Y = x_frand(       Low, +Velocity );
        Vel.Z = x_frand( -Velocity, +Velocity );

        pDebris = (debris*)ObjMgr.CreateObject( object::TYPE_DEBRIS );
        pDebris->Initialize( WorldPos, 
                             Vel,
                             (debris::chunk_type)i,
                             j < Flame[i] ? PARTICLE_TYPE_DEBRIS_SMOKE : 0 );

        ObjMgr.AddObject( pDebris, obj_mgr::AVAILABLE_CLIENT_SLOT );
    }
}

//==============================================================================

void SpawnDebris(        pain::type   PainType,
                  const  vector3&     WorldPos )
{
    debris* pDebris;
    vector3 Vel;
    f32     Velocity = ExpAttr[ PainType ].DebrisVel * 0.5f;

    s32 i;
    s32 Base;
    s32 Index = (PainType - pain::EXPLOSION_GRAV_CYCLE);
    s32 Map[] =
    {
        debris::VEHICLE_GRAVCYCLE1,      // EXPLOSION_GRAV_CYCLE,
        debris::VEHICLE_TANK1,           // EXPLOSION_TANK,      
        debris::VEHICLE_MPB1,            // EXPLOSION_MPB,       
        debris::VEHICLE_SHRIKE1,         // EXPLOSION_SHRIKE,    
        debris::VEHICLE_BOMBER1,         // EXPLOSION_BOMBER,    
        debris::VEHICLE_TRANSPORT1,      // EXPLOSION_TRANSPORT, 
    };

    Base = Map[Index];      

    for( i = 0; i < 5; i++ )
    {
        Vel.X = x_frand( -Velocity, +Velocity ); // + VehicleVelocity.X;
        Vel.Y = x_frand( -Velocity, +Velocity ); // + VehicleVelocity.Y;
        Vel.Z = x_frand( -Velocity, +Velocity ); // + VehicleVelocity.Z;

        pDebris = (debris*)ObjMgr.CreateObject( object::TYPE_DEBRIS );
        pDebris->Initialize( WorldPos, 
                             Vel,
                             (debris::chunk_type)(Base + i),
                             PARTICLE_TYPE_DAMAGE_SMOKE2 );

        ObjMgr.AddObject( pDebris, obj_mgr::AVAILABLE_CLIENT_SLOT );
    }
}

//==============================================================================
