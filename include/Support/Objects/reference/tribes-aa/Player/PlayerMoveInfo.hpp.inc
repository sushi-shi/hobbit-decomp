//==============================================================================
//
//  PlayerMoveInfo.hpp
//
//  Defines control speeds etc for the struct player_move_info
//
//==============================================================================


//=========================================================================
// MOVEMENT DEFINES (LIGHT ARMOR DEFAULTS)
//=========================================================================
  
DEFINE_ATTRIBUTE_F32 (MASS,                             90      )
DEFINE_ATTRIBUTE_F32 (DRAG,                             0.3     )
DEFINE_ATTRIBUTE_F32 (MAX_DRAG,                         0.4     )
DEFINE_ATTRIBUTE_F32 (DENSITY,                          10      )
DEFINE_ATTRIBUTE_F32 (MAX_DAMAGE,                       0.66    )
DEFINE_ATTRIBUTE_F32 (MAX_ENERGY,                       60      )
DEFINE_ATTRIBUTE_F32 (REPAIR_RATE,                      0.0033  )
DEFINE_ATTRIBUTE_F32 (ENERGY_PER_DAMAGE_POINT,          75.0    ) // Shield energy required to block one point of damage

DEFINE_ATTRIBUTE_F32 (RECHARGE_RATE,                    0.1365  )
DEFINE_ATTRIBUTE_F32 (RECHARGE_RATE_WITH_ENERGY_PACK,   0.2165  )
DEFINE_ATTRIBUTE_F32 (JET_FORCE,                        2538    )     // 28.2 * 90
DEFINE_ATTRIBUTE_F32 (UNDERWATER_JET_FORCE,             4716    )     // 26.2 * 90 * 2.0
DEFINE_ATTRIBUTE_F32 (UNDERWATER_VERT_JET_FACTOR,       1.5     )
DEFINE_ATTRIBUTE_F32 (JET_ENERGY_DRAIN,                 0.8     )
DEFINE_ATTRIBUTE_F32 (UNDERWATER_JET_ENERGY_DRAIN,      0.5     )
DEFINE_ATTRIBUTE_F32 (MIN_JET_ENERGY,                   1       )
DEFINE_ATTRIBUTE_F32 (MAX_JET_HORIZONTAL_PERCENTAGE,    0.8     )

DEFINE_ATTRIBUTE_F32 (RUN_FORCE,                        4968    ) //   55.20 * 90
DEFINE_ATTRIBUTE_F32 (MAX_FORWARD_SPEED,                15      )
DEFINE_ATTRIBUTE_F32 (MAX_BACKWARD_SPEED,               14      )
DEFINE_ATTRIBUTE_F32 (MAX_SIDE_SPEED,                   14      )

DEFINE_ATTRIBUTE_F32 (MAX_UNDERWATER_FORWARD_SPEED,     8.4     )
DEFINE_ATTRIBUTE_F32 (MAX_UNDERWATER_BACKWARD_SPEED,    7.8     )
DEFINE_ATTRIBUTE_F32 (MAX_UNDERWATER_SIDE_SPEED,        7.8     )


DEFINE_ATTRIBUTE_F32 (JUMP_FORCE,                       747     )   // 8.3 * 90

DEFINE_ATTRIBUTE_F32 (RECOVER_DELAY,                    9 )
DEFINE_ATTRIBUTE_F32 (RECOVER_RUN_FORCE_SCALE,          1.2 )

DEFINE_ATTRIBUTE_F32 (MIN_IMPACT_SPEED,                 45 )
DEFINE_ATTRIBUTE_F32 (SPEED_DAMAGE_SCALE,               0.004 )

DEFINE_ATTRIBUTE_V3   (SCALE,                           1,1,1 )
DEFINE_ATTRIBUTE_BBOX (BOUNDING_BOX,                    1.2, 2.3, 1.2 )
DEFINE_ATTRIBUTE_F32  (PICKUP_RADIUS,                   0.75 )

// Controls over slope of runnable/jumpable surfaces
DEFINE_ATTRIBUTE_R32 (RUN_SURFACE_ANGLE ,               70 )
DEFINE_ATTRIBUTE_R32 (JUMP_SURFACE_ANGLE,               80 )

DEFINE_ATTRIBUTE_F32 (MIN_JUMP_SPEED,                   20 )
DEFINE_ATTRIBUTE_F32 (MAX_JUMP_SPEED,                   30 )

DEFINE_ATTRIBUTE_F32 (HORIZ_MAX_SPEED,                  68 )
DEFINE_ATTRIBUTE_F32 (HORIZ_RESIST_SPEED,               33 )
DEFINE_ATTRIBUTE_F32 (HORIZ_RESIST_FACTOR,              0.35 )
DEFINE_ATTRIBUTE_F32 (MAX_JET_FORWARD_SPEED,            30 )

DEFINE_ATTRIBUTE_F32 (UP_MAX_SPEED,                     80 )
DEFINE_ATTRIBUTE_F32 (UP_RESIST_SPEED,                  25 )
DEFINE_ATTRIBUTE_F32 (UP_RESIST_FACTOR,                 0.3 )

// Heat inc'ers and dec'ers
DEFINE_ATTRIBUTE_F32 (HEAT_DECAY_PER_SEC,               1.0 / 3.0 ) // Takes 3 seconds to clear heat sig.
DEFINE_ATTRIBUTE_F32 (HEAT_INCREASE_PER_SEC,            1.0 / 3.0 ) // Takes 3.0 seconds of constant jet to get full heat sig.

DEFINE_ATTRIBUTE_S32 (MAX_WEAPONS,                      3 )           // Max number of different weapons the player can have
DEFINE_ATTRIBUTE_S32 (MAX_GRENADES,                     1 )          // Max number of different grenades the player can have
DEFINE_ATTRIBUTE_S32 (MAX_MINES,                        1 )             // Max number of different mines the player can have


// Inventory restrictions
DEFINE_ATTRIBUTE_ARRAY_S32 (INVENT_MAX,   INVENT_TYPE_TOTAL)

// Weapons
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_DISK_LAUNCHER],         0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_PLASMA_GUN],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_SNIPER_RIFLE],          0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_MORTAR_GUN],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_GRENADE_LAUNCHER],      0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_CHAIN_GUN],             0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_BLASTER],               0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_ELF_GUN],               0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_MISSILE_LAUNCHER],      0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_REPAIR_GUN],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_SHOCKLANCE],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_TARGETING_LASER],       0   )

                                                                                        
// Weapon ammo                                                                          
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_DISK],             0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_BULLET],           0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_GRENADE],          0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_PLASMA],           0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_MORTAR],           0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_WEAPON_AMMO_MISSILE],          0   )
                                                                                        
// Secondary weapons                                                                    
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_GRENADE_BASIC],                0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_GRENADE_FLASH],                0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_GRENADE_CONCUSSION],           0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_GRENADE_FLARE],                0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_MINE],                         0   )
                                                                                        
// Armor packs                                                                          
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_AMMO],              0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_CLOAKING],          0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_ENERGY],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_REPAIR],            0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_SENSOR_JAMMER],     0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_ARMOR_PACK_SHIELD],            0   )

// Deployable packs
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_BARREL_AA],        0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_BARREL_ELF],       0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_BARREL_MORTAR],    0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_BARREL_MISSILE],    0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_BARREL_PLASMA],    0   )
                                                                                        
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_INVENT_STATION],   0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_MOTION_SENSOR],    0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_PULSE_SENSOR],     0   )
                                                                                        
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_TURRET_INDOOR],    0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_TURRET_OUTDOOR],   0   )
                                                                                        
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_DEPLOY_PACK_SATCHEL_CHARGE],   0   )
                                                                                        
// Belt gear                                                                            
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_BELT_GEAR_BEACON],             0   )
DEFINE_ATTRIBUTE_ARRAY_S32_ENTRY( INVENT_MAX[INVENT_TYPE_BELT_GEAR_HEALTH_KIT],         0   )



//=========================================================================

