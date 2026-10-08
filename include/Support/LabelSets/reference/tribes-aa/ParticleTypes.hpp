#ifndef __PARTICLE_TYPES_H__
#define __PARTICLE_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************

		
// Particle types
BEGIN_LABEL_SET(ParticleTypes)
    LABEL(PARTICLE_TYPE_NONE,           "<un-assigned>"             )
    LABEL(PARTICLE_TYPE_EXP_DISC,       "Disc Explosion"            )
    LABEL(PARTICLE_TYPE_EXP_MORTAR,     "Mortar Explosion"          )
    LABEL(PARTICLE_TYPE_JETPACK,        "Jetpack"                   )
    LABEL(PARTICLE_TYPE_EXP_GRENADE,    "Grenade Explosion"         )
    LABEL(PARTICLE_TYPE_EXP_PLASMA,     "Plasma Explosion"          )
    LABEL(PARTICLE_TYPE_EXP_BULLET,     "Bullet Explosion"          )
    LABEL(PARTICLE_TYPE_GREN_SMOKE,     "Grenade Smoke Trail"       )
    LABEL(PARTICLE_TYPE_MORT_SMOKE,     "Mortar Smoke Trail"        )
    LABEL(PARTICLE_TYPE_EXP_LASER,      "Laser Explosion"           )
    LABEL(PARTICLE_TYPE_EXP_BLASTER,    "Blaster Explosion"         )
    LABEL(PARTICLE_TYPE_VEHICLE_EX1,    "Vehicle Exhaust 1"         )
    LABEL(PARTICLE_TYPE_MISSILE_SMOKE,  "Missile Smoke Trail"       )
    LABEL(PARTICLE_TYPE_CONC_EXP,       "Concussion Explosion"      )
    LABEL(PARTICLE_TYPE_FLASH_EXP,      "Flash Explosion"           )
    LABEL(PARTICLE_TYPE_FLARE_SPARKS,   "Flare Sparks"              )
    LABEL(PARTICLE_TYPE_DEBRIS_SMOKE,   "Debris Smoke Trail"        )
    LABEL(PARTICLE_TYPE_EXP_BOMB,       "Bomb Explosion"            )
    LABEL(PARTICLE_TYPE_EXP_SATCHEL,    "Satchel Explosion"         )
    LABEL(PARTICLE_TYPE_HOVER_SMOKE,    "Hovering Vehicle Smoke"    )
    LABEL(PARTICLE_TYPE_HOVER_SMOKE2,   "Larger Hovering Smoke"     )
    LABEL(PARTICLE_TYPE_HOVER_SMOKE3,   "Really Large Smoke"        )
    LABEL(PARTICLE_TYPE_DAMAGE_SMOKE,   "Damaged Vehicle Smoke"     )
    LABEL(PARTICLE_TYPE_DAMAGE_SMOKE2,  "Larger Damage Smoke"       )
    LABEL(PARTICLE_TYPE_DAMAGE_SMOKE3,  "Huge Damage Smoke"         )
    LABEL(PARTICLE_TYPE_EXP_MISSILE,    "Missile Explosion"         )
    LABEL(PARTICLE_TYPE_PLASMA_TRAIL,   "Plasma Trail"              )
    LABEL(PARTICLE_TYPE_EXP_TURRET,     "Turret Explosion"          )
    LABEL(PARTICLE_TYPE_SATCHEL_SMALL,  "Small Satchel Explosion"   )



END_LABEL_SET(ParticleTypes)


#endif	// __PARTICLE_TYPES_H__
