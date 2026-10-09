//==============================================================================
//
//  Pain.hpp
//
//==============================================================================

#ifndef PAIN_HPP
#define PAIN_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/Object.hpp>

//==============================================================================
//  TYPES
//==============================================================================

struct pain_base
{
    enum type
    {
        WEAPON_DISC,
        WEAPON_CHAINGUN,
        WEAPON_BLASTER,
        WEAPON_PLASMA,
        WEAPON_GRENADE,
        WEAPON_MORTAR,
        WEAPON_MISSILE,
        WEAPON_ELF,
        WEAPON_LASER,
        WEAPON_LASER_HEAD_SHOT,
        WEAPON_GRENADE_HAND,
        WEAPON_GRENADE_FLASH,
        WEAPON_GRENADE_CONCUSSION,
        WEAPON_MINE,
        WEAPON_MINE_FIZZLE,
        WEAPON_SATCHEL,
        WEAPON_SATCHEL_SMALL,
        WEAPON_SATCHEL_FIZZLE,
        WEAPON_TANK_GUN,
        WEAPON_TANK_MORTAR,
        WEAPON_SHRIKE_GUN,
        WEAPON_BOMBER_GUN,
        WEAPON_BOMBER_BOMB,
        TURRET_AA,
        TURRET_ELF,
        TURRET_PLASMA,
        TURRET_MISSILE,
        TURRET_MORTAR,
        TURRET_SENTRY,
        TURRET_CLAMP,
        EXPLOSION_GENERATOR,
        EXPLOSION_SENSOR_LARGE,
        EXPLOSION_SENSOR_MEDIUM,
        EXPLOSION_SENSOR_REMOTE,
        EXPLOSION_TURRET_FIXED,
        EXPLOSION_TURRET_SENTRY,
        EXPLOSION_TURRET_CLAMP,
        EXPLOSION_STATION_INVEN,
        EXPLOSION_STATION_VEHICLE,
        EXPLOSION_STATION_DEPLOYED,
        EXPLOSION_BEACON,
        EXPLOSION_GRAV_CYCLE,
        EXPLOSION_TANK,
        EXPLOSION_MPB,
        EXPLOSION_SHRIKE,
        EXPLOSION_BOMBER,
        EXPLOSION_TRANSPORT,     
        HIT_BY_ARMOR_LIGHT,
        HIT_BY_ARMOR_MEDIUM,
        HIT_BY_ARMOR_HEAVY,      
        HIT_BY_GRAV_CYCLE,
        HIT_BY_TANK,      
        HIT_BY_MPB,       
        HIT_BY_SHRIKE,    
        HIT_BY_BOMBER,    
        HIT_BY_TRANSPORT,
        HIT_IMMOVEABLE,
        MISC_VEHICLE_SPAWN,
        MISC_VEHICLE_SPAWN_FORCE,
        MISC_FORCE_FIELD,
        MISC_BOUNDS_HURT,
        MISC_BOUNDS_DEATH,
        MISC_FLIP_FLOP_FORCE,
        MISC_FLAG_TO_VEHICLE,
        MISC_PLAYER_SUICIDE,
        REPAIR_VIA_PACK,
        REPAIR_VIA_FIXED_INVEN,
        REPAIR_VIA_DEPLOYED_INVEN,

        PAIN_END_OF_LIST,
    };

    type  PainType;  
    xbool SapEnergy;
    f32   DamageR1;
    f32   DamageR0;
    f32   ForceR1;
    f32   ForceR0;
    f32   MaxForce;
};

//------------------------------------------------------------------------------

struct pain : public pain_base
{
// Data
    f32         Force;
    f32         MetalDamage;
    f32         EnergyDamage;
    f32         Distance;
    xbool       LineOfSight;
    xbool       DirectHit;

    vector3     Center;

    u32         TeamBits;
    object::id  OriginID;
    object::id  VictimID;   // Used when R0 is 0 for both Damage and Force.

// Operators/functions
    const pain& operator = ( const pain_base& PainBase );
    void Clear( void );
}; 

//==============================================================================
//  FUNCTIONS
//==============================================================================


//==============================================================================
//  EXPORTED DATA
//==============================================================================

extern f32       g_MetalDamageTable [ pain::PAIN_END_OF_LIST ][ object::TYPE_END_DAMAGEABLE+1 ];
extern f32       g_ShieldDamageTable[ pain::PAIN_END_OF_LIST ][ object::TYPE_END_DAMAGEABLE+1 ];
extern f32       g_ShieldLeakTable  [ pain::PAIN_END_OF_LIST ][ object::TYPE_END_DAMAGEABLE+1 ];
extern pain_base g_PainBase         [ pain::PAIN_END_OF_LIST ];

//==============================================================================
#endif // PAIN_HPP
//==============================================================================
