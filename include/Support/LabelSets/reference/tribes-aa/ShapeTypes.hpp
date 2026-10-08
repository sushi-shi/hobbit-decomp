#ifndef __SHAPE_TYPES_H__
#define __SHAPE_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************



// Shape type
BEGIN_LABEL_SET(ShapeTypes)
    LABEL(SHAPE_TYPE_NONE,                          "<un-assigned>"                             )
                                                                                                
    // Characters                                                                               
    LABEL(SHAPE_TYPE_CHARACTER_LIGHT_MALE,          "Character - Light male"                    )
    LABEL(SHAPE_TYPE_CHARACTER_MEDIUM_MALE,         "Character - Medium male"                   )
    LABEL(SHAPE_TYPE_CHARACTER_HEAVY_MALE,          "Character - Heavy male"                    )
                                                                                                
    LABEL(SHAPE_TYPE_CHARACTER_LIGHT_FEMALE,        "Character - Light female"                  )
    LABEL(SHAPE_TYPE_CHARACTER_MEDIUM_FEMALE,       "Character - Medium female"                 )
    LABEL(SHAPE_TYPE_CHARACTER_HEAVY_FEMALE,        "Character - Heavy female"                  )
                                                                                                
    LABEL(SHAPE_TYPE_CHARACTER_LIGHT_BIO,           "Character - Light bio"                     )
    LABEL(SHAPE_TYPE_CHARACTER_MEDIUM_BIO,          "Character - Medium bio"                    )
    LABEL(SHAPE_TYPE_CHARACTER_HEAVY_BIO,           "Character - Heavy bio"                     )
                                                                                                
    // Weapons                                                                                  
    LABEL(SHAPE_TYPE_WEAPON_CHAIN_GUN,              "Weapon - Chain gun"                        )
    LABEL(SHAPE_TYPE_WEAPON_DISC_LAUNCHER,          "Weapon - Disc launcher"                    )
    LABEL(SHAPE_TYPE_WEAPON_ELF,                    "Weapon - Elf"                              )
    LABEL(SHAPE_TYPE_WEAPON_ENERGY,                 "Weapon - Energy"                           )
    LABEL(SHAPE_TYPE_WEAPON_GRENADE_LAUNCHER,       "Weapon - Grenade launcher"                 )
    LABEL(SHAPE_TYPE_WEAPON_MISSILE_LAUNCHER,       "Weapon - Missle launcher"                  )
    LABEL(SHAPE_TYPE_WEAPON_MORTAR_LAUNCHER,        "Weapon - Mortar launcher"                  )
    LABEL(SHAPE_TYPE_WEAPON_PLASMA_GUN,             "Weapon - Plasma gun"                       )
    LABEL(SHAPE_TYPE_WEAPON_PLASMA_THROWER,         "Weapon - Plasma thrower"                   )
    LABEL(SHAPE_TYPE_WEAPON_REPAIR,                 "Weapon - Repair"                           )
    LABEL(SHAPE_TYPE_WEAPON_SHOCKLANCE,             "Weapon - Shocklance"                       )
    LABEL(SHAPE_TYPE_WEAPON_SNIPER_RIFLE,           "Weapon - Sniper rifle"                     )
    LABEL(SHAPE_TYPE_WEAPON_TARGETING,              "Weapon - Targeting"                        )
    LABEL(SHAPE_TYPE_WEAPON_BLASTER,                "Weapon - Blaster"                          )
                                                                                                
    // Assets                                                                                   
    LABEL(SHAPE_TYPE_ASSET_GENERATOR,               "Asset - Generator"                         )
    LABEL(SHAPE_TYPE_ASSET_GENERATOR_COLL,          "Asset - Generator COLLISION"               )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_LARGE,            "Asset - Sensor Large"                      )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_LARGE_COLL,       "Asset - Sensor Large COLLISION"            )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_MEDIUM,           "Asset - Sensor Medium"                     )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_MEDIUM_COLL,      "Asset - Sensor Medium COLLISION"           )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_MOTION,           "Asset - Sensor Motion"                     )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_MOTION_COLL,      "Asset - Sensor Motion COLLISION"           )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_PULSE,            "Asset - Sensor Pulse"                      )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_PULSE_COLL,       "Asset - Sensor Pulse COLLISION"            )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_DEPLOYED,         "Asset - Sensor Deployed"                   )
    LABEL(SHAPE_TYPE_ASSET_SENSOR_DEPLOYED_COLL,    "Asset - Sensor Deployed COLLISION"         )

    // Turrets                                                                                  
    LABEL(SHAPE_TYPE_TURRET_DEPLOYED,               "Turret - Deployed"                         )
    LABEL(SHAPE_TYPE_TURRET_DEPLOYED_COLL,          "Turret - Deployed COLLISION"               )
    LABEL(SHAPE_TYPE_TURRET_SENTRY,                 "Turret - Sentry"                           )
    LABEL(SHAPE_TYPE_TURRET_SENTRY_COLL,            "Turret - Sentry COLLISION"                 )
    LABEL(SHAPE_TYPE_TURRET_FIXED_BASE,             "Turret - Fixed Base"                       )
    LABEL(SHAPE_TYPE_TURRET_FIXED_BASE_COLL,        "Turret - Fixed Base COLLISION"             )

    LABEL(SHAPE_TYPE_TURRET_BARREL_AA,              "Turret - Barrel AA"                        )
    LABEL(SHAPE_TYPE_TURRET_BARREL_ELF,             "Turret - Barrel ELF"                       )
    LABEL(SHAPE_TYPE_TURRET_BARREL_MISSILE,         "Turret - Barrel Missile"                   )
    LABEL(SHAPE_TYPE_TURRET_BARREL_PLASMA,          "Turret - Barrel Plasma"                    )
    LABEL(SHAPE_TYPE_TURRET_BARREL_MORTAR,          "Turret - Barrel Mortar"                    )

    LABEL(SHAPE_TYPE_TURRET_TANK_BASE,              "Turret - Tank Base"                        )
    LABEL(SHAPE_TYPE_TURRET_TANK_BARREL_MORTAR,     "Turret - Tank Barrel Mortar"               )
    LABEL(SHAPE_TYPE_TURRET_TANK_BARREL_CHAIN,      "Turret - Tank Barrel Chain"                )

    LABEL(SHAPE_TYPE_TURRET_BELLY_BASE,             "Turret - Belly Base"                       )
    LABEL(SHAPE_TYPE_TURRET_BELLY_BARREL_LEFT,      "Turret - Belly Barrel Left"                )
    LABEL(SHAPE_TYPE_TURRET_BELLY_BARREL_RIGHT,     "Turret - Belly Barrel Right"               )

    LABEL(SHAPE_TYPE_TURRET_TRANSPORT,              "Turret - Transport vehicle"                )
    LABEL(SHAPE_TYPE_TURRET_TRANSPORT_COLL,         "Turret - Transport vehicle COLLISION"      )

    // GUI screens                                                                              
    LABEL(SHAPE_TYPE_GUI_MAIN_SCREEN,               "GUI - Main screen"                         )
    LABEL(SHAPE_TYPE_GUI_OPTIONS_SCREEN,            "GUI - Options screen"                      )
                                                                                                
    // Particles                                                                                
    LABEL(SHAPE_TYPE_PARTICLE_HEMISPHERE,           "Particle - Hemisphere"                     )
    LABEL(SHAPE_TYPE_EXPLOSION_SURFACE,             "Particle - Explosion Surface"              )
    LABEL(SHAPE_TYPE_MORTAR_EXPLOSION_SURFACE,      "Particle - Mortar Explosion Surface"       )
    LABEL(SHAPE_TYPE_PLASMA_EXPLOSION_SURFACE,      "Particle - Plasma Explosion Surface"       )
    LABEL(SHAPE_TYPE_EXPLOSION_SHOCKWAVE,           "Particle - Explosion Shockwave"            )


    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_LARGE1,           "Particle - Large Debris v1"                )
    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_LARGE2,           "Particle - Large Debris v2"                )
    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_MEDIUM1,          "Particle - Medium Debris v1"               )
    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_MEDIUM2,          "Particle - Medium Debris v2"               )
    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_SMALL1,           "Particle - Small Debris v1"                )
    LABEL(SHAPE_TYPE_DEBRIS_CHUNK_SMALL2,           "Particle - Small Debris v2"                )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT1,       "Particle - Vehicle Wildcat Debris 1"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT2,       "Particle - Vehicle Wildcat Debris 2"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT3,       "Particle - Vehicle Wildcat Debris 3"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT4,       "Particle - Vehicle Wildcat Debris 4"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT5,       "Particle - Vehicle Wildcat Debris 5"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_WILDCAT6,       "Particle - Vehicle Wildcat Debris 6"       )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE1,        "Particle - Vehicle Shrike Debris 1"        )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE2,        "Particle - Vehicle Shrike Debris 2"        )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE3,        "Particle - Vehicle Shrike Debris 3"        )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE4,        "Particle - Vehicle Shrike Debris 4"        )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE5,        "Particle - Vehicle Shrike Debris 5"        )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_SHRIKE6,        "Particle - Vehicle Shrike Debris 6"        )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD1,        "Particle - Vehicle Thundersword Debris 1"  )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD2,        "Particle - Vehicle Thundersword Debris 2"  )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD3,        "Particle - Vehicle Thundersword Debris 3"  )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD4,        "Particle - Vehicle Thundersword Debris 4"  )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD5,        "Particle - Vehicle Thundersword Debris 5"  )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_TSWORD6,        "Particle - Vehicle Thundersword Debris 6"  )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC1,         "Particle - Vehicle Havoc Debris 1"         )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC2,         "Particle - Vehicle Havoc Debris 2"         )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC3,         "Particle - Vehicle Havoc Debris 3"         )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC4,         "Particle - Vehicle Havoc Debris 4"         )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC5,         "Particle - Vehicle Havoc Debris 5"         )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_HAVOC6,         "Particle - Vehicle Havoc Debris 6"         )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF1,       "Particle - Vehicle Beowulf Debris 1"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF2,       "Particle - Vehicle Beowulf Debris 2"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF3,       "Particle - Vehicle Beowulf Debris 3"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF4,       "Particle - Vehicle Beowulf Debris 4"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF5,       "Particle - Vehicle Beowulf Debris 5"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_BEOWULF6,       "Particle - Vehicle Beowulf Debris 6"       )

    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO1,       "Particle - Vehicle Jericho Debris 1"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO2,       "Particle - Vehicle Jericho Debris 2"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO3,       "Particle - Vehicle Jericho Debris 3"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO4,       "Particle - Vehicle Jericho Debris 4"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO5,       "Particle - Vehicle Jericho Debris 5"       )
    LABEL(SHAPE_TYPE_DEBRIS_VEHICLE_JERICHO6,       "Particle - Vehicle Jericho Debris 6"       )


    // Pickups                                                                                  
    LABEL(SHAPE_TYPE_PICKUP_AMMO_DISC,              "Pickup - Ammo disc"                        )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_CHAIN_GUN,         "Pickup - Ammo chaingun"                    )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_PLASMA,            "Pickup - Ammo plasma"                      )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_GRENADE_LAUNCHER,  "Pickup - Ammo grenade launcher"            )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_GRENADE_BASIC,     "Pickup - Ammo grenade basic"               )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_MINE,              "Pickup - Ammo mine"                        )
    LABEL(SHAPE_TYPE_PICKUP_AMMO_BOX,               "Pickup - Ammo box"                         )

    LABEL(SHAPE_TYPE_PICKUP_HEALTH_PATCH,           "Pickup - Health patch"                     )
    LABEL(SHAPE_TYPE_PICKUP_HEALTH_KIT,             "Pickup - Health kit"                       )
                                                                                                
    // Projectiles                                                                              
    LABEL(SHAPE_TYPE_PROJ_DISC_CORE,                "Projectile - Disc Core"                    )
    LABEL(SHAPE_TYPE_PROJ_DISC_EFFECT,              "Projectile - Disc Effect"                  )
    LABEL(SHAPE_TYPE_PROJ_GRENADE,                  "Projectile - Grenade"                      )
    LABEL(SHAPE_TYPE_PROJ_HANDGRENADE,              "Projectile - Hand Grenade"                 )
    LABEL(SHAPE_TYPE_PROJ_MORTAR,                   "Projectile - Mortar"                       )
    LABEL(SHAPE_TYPE_PROJ_MISSILE,                  "Projectile - Missile"                      )
    LABEL(SHAPE_TYPE_PROJ_MISSILE_CASING,           "Projectile - Missile Casing"               )
    LABEL(SHAPE_TYPE_PROJ_BOMB,                     "Projectile - Bomb"                         )
    LABEL(SHAPE_TYPE_PROJ_MINE,                     "Projectile - Mine"                         )
                                                                                                
    // Misc                                                                                     
    LABEL(SHAPE_TYPE_MISC_FLAG,                     "Misc - Flag"                               )
    LABEL(SHAPE_TYPE_MISC_FLAG_STAND,               "Misc - Flag Stand"                         )
    LABEL(SHAPE_TYPE_MISC_SHIELD,                   "Misc - Shield"                             )
    LABEL(SHAPE_TYPE_MISC_SWITCH,                   "Misc - Switch (flip flop)"                 )
    LABEL(SHAPE_TYPE_MISC_BEACON,                   "Misc - Beacon"                             )
    LABEL(SHAPE_TYPE_MISC_LOGO_PROJECTOR,           "Misc - Logo projector"                     )
    LABEL(SHAPE_TYPE_MISC_LOGO_STORM,               "Misc - Logo storm"                         )
    LABEL(SHAPE_TYPE_MISC_LOGO_INFERNO,             "Misc - Logo inferno"                       )
    LABEL(SHAPE_TYPE_MISC_LOGO_NEUTRAL,             "Misc - Logo neutral"                       )
    LABEL(SHAPE_TYPE_MISC_FORCE_FIELD,              "Misc - Force field"                        )
                                                                                                
    // Inventory stations                                                                       
    LABEL(SHAPE_TYPE_INVENT_STATION_HUMAN,          "Inventory station - Human"                 )
    LABEL(SHAPE_TYPE_INVENT_STATION_HUMAN_COLL,     "Inventory station - Human COLLISION"       )
    LABEL(SHAPE_TYPE_INVENT_STATION_VEHICLE,        "Inventory station - Vehicle"               )
    LABEL(SHAPE_TYPE_INVENT_STATION_VEHICLE_COLL,   "Inventory station - Vehicle COLLISION"     )
    LABEL(SHAPE_TYPE_INVENT_STATION_DEPLOY,         "Inventory station - Deploy"                )
    LABEL(SHAPE_TYPE_INVENT_STATION_DEPLOY_COLL,    "Inventory station - Deploy COLLISION"      )

    // Vehicle related shapes
    LABEL(SHAPE_TYPE_VEHICLE_PAD,                   "Vehicle - Vehicle pad"                             )
    LABEL(SHAPE_TYPE_VEHICLE_PAD_COLL,              "Vehicle - Vehicle pad COLLISION"                   )

    LABEL(SHAPE_TYPE_VEHICLE_SHRIKE,                "Vehicle - Shrike Fighter"                          )
    LABEL(SHAPE_TYPE_VEHICLE_SHRIKE_COLL,           "Vehicle - Shrike Fighter COLLISION"                )
    LABEL(SHAPE_TYPE_VEHICLE_THUNDERSWORD,          "Vehicle - Thundersword Bomber"                     )
    LABEL(SHAPE_TYPE_VEHICLE_THUNDERSWORD_COLL,     "Vehicle - Thundersword Bomber COLLISION"           )
    LABEL(SHAPE_TYPE_VEHICLE_HAVOC,                 "Vehicle - Havoc Transport"                         )
    LABEL(SHAPE_TYPE_VEHICLE_HAVOC_COLL,            "Vehicle - Havoc Transport COLLISION"               )
    LABEL(SHAPE_TYPE_VEHICLE_WILDCAT,               "Vehicle - Wildcat GravCycle"                       )
    LABEL(SHAPE_TYPE_VEHICLE_WILDCAT_COLL,          "Vehicle - Wildcat GravCycle COLLISION"             )
    LABEL(SHAPE_TYPE_VEHICLE_BEOWULF,               "Vehicle - Beowulf Assault Tank"                    )
    LABEL(SHAPE_TYPE_VEHICLE_BEOWULF_COLL,          "Vehicle - Beowulf Assault Tank COLLISION"          )
    LABEL(SHAPE_TYPE_VEHICLE_JERICHO2,              "Vehicle - Jericho II Mobile Point Base"            )
    LABEL(SHAPE_TYPE_VEHICLE_JERICHO2_COLL,         "Vehicle - Jericho II Mobile Point Base COLLISION"  )

    // Armor packs
    LABEL(SHAPE_TYPE_ARMOR_PACK_AMMO,               "Armor Pack - Ammo"                         )
    LABEL(SHAPE_TYPE_ARMOR_PACK_CLOAKING,           "Armor Pack - Cloaking"                     )
    LABEL(SHAPE_TYPE_ARMOR_PACK_ENERGY,             "Armor Pack - Energy"                       )
    LABEL(SHAPE_TYPE_ARMOR_PACK_REPAIR,             "Armor Pack - Repair"                       )
    LABEL(SHAPE_TYPE_ARMOR_PACK_SENSOR_JAMMER,      "Armor Pack - Sensor jammer"                )
    LABEL(SHAPE_TYPE_ARMOR_PACK_SHIELD,             "Armor Pack - Shield"                       )
                                                                                                
    // Deployable packs                                                                         
    LABEL(SHAPE_TYPE_DEPLOY_PACK_BARREL_AA,         "Deployable Pack - Barrel AA"               )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_BARREL_ELF,        "Deployable Pack - Barrel Elf"              )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_BARREL_MORTAR,     "Deployable Pack - Barrel Mortar"           )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_BARREL_MISSILE,    "Deployable Pack - Barrel Missile"          )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_BARREL_PLASMA,     "Deployable Pack - Barrel Plasma"           )
                                                                                                
    LABEL(SHAPE_TYPE_DEPLOY_PACK_INVENT_STATION,    "Deployable Pack - Invent station"          )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_MOTION_SENSOR,     "Deployable Pack - Motion sensor"           )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_PULSE_SENSOR,      "Deployable Pack - Pulse sensor"            )
                                                                                                
    LABEL(SHAPE_TYPE_DEPLOY_PACK_TURRET_INDOOR,     "Deployable Pack - Turret indoor"           )
    LABEL(SHAPE_TYPE_DEPLOY_PACK_TURRET_OUTDOOR,    "Deployable Pack - Turret outdoor"          )
                                                                                                
    LABEL(SHAPE_TYPE_DEPLOY_PACK_SATCHEL_CHARGE,    "Deployable Pack - Satchel charge"          )
                                                                                                
    // Scenery     
    
    // LushShapes.txt
    LABEL(SHAPE_TYPE_SCENERY_BMISCF,                "Scenery - bmiscf"                          )

    LABEL(SHAPE_TYPE_SCENERY_BORG1,                 "Scenery - borg1"                           )
    LABEL(SHAPE_TYPE_SCENERY_BORG5,                 "Scenery - borg5"                           )
    LABEL(SHAPE_TYPE_SCENERY_BORG13,                "Scenery - borg13"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG16,                "Scenery - borg16"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG17,                "Scenery - borg17"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG18,                "Scenery - borg18"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG19,                "Scenery - borg19"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG20,                "Scenery - borg20"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG33,                "Scenery - borg33"                          )
    LABEL(SHAPE_TYPE_SCENERY_BORG34,                "Scenery - borg34"                          )

    LABEL(SHAPE_TYPE_SCENERY_BROCK6,                "Scenery - brock6"                          )
    LABEL(SHAPE_TYPE_SCENERY_BROCK7,                "Scenery - brock7"                          )
    LABEL(SHAPE_TYPE_SCENERY_BROCK8,                "Scenery - brock8"                          )
    LABEL(SHAPE_TYPE_SCENERY_BROCKA,                "Scenery - brocka"                          )
    LABEL(SHAPE_TYPE_SCENERY_BROCKC,                "Scenery - brockc"                          )

    LABEL(SHAPE_TYPE_SCENERY_BSPIR1,                "Scenery - bspir1"                          )
    LABEL(SHAPE_TYPE_SCENERY_BSPIR2,                "Scenery - bspir2"                          )
    LABEL(SHAPE_TYPE_SCENERY_BSPIR3,                "Scenery - bspir3"                          )
    LABEL(SHAPE_TYPE_SCENERY_BSPIR4,                "Scenery - bspir4"                          )
    LABEL(SHAPE_TYPE_SCENERY_BSPIR5,                "Scenery - bspir5"                          )

    // LavaShapes.txt
    LABEL(SHAPE_TYPE_SCENERY_DORG16,                "Scenery - dorg16"                          )
    LABEL(SHAPE_TYPE_SCENERY_DORG18,                "Scenery - dorg18"                          )
    LABEL(SHAPE_TYPE_SCENERY_DORG19,                "Scenery - dorg19"                          )
    
    LABEL(SHAPE_TYPE_SCENERY_DROCK8,                "Scenery - drock8"                          )


    // DesertShapes.txt
    LABEL(SHAPE_TYPE_SCENERY_PMISCF,                "Scenery - pmiscf"                          )

    LABEL(SHAPE_TYPE_SCENERY_PORG1,                 "Scenery - porg1"                           )
    LABEL(SHAPE_TYPE_SCENERY_PORG2,                 "Scenery - porg2"                           )
    LABEL(SHAPE_TYPE_SCENERY_PORG3,                 "Scenery - porg3"                           )
    LABEL(SHAPE_TYPE_SCENERY_PORG5,                 "Scenery - porg5"                           )
    LABEL(SHAPE_TYPE_SCENERY_PORG6,                 "Scenery - porg6"                           )
    LABEL(SHAPE_TYPE_SCENERY_PORG20,                "Scenery - porg20"                          )

    LABEL(SHAPE_TYPE_SCENERY_PROCK6,                "Scenery - prock6"                          )
    LABEL(SHAPE_TYPE_SCENERY_PROCK7,                "Scenery - prock7"                          )
    LABEL(SHAPE_TYPE_SCENERY_PROCK8,                "Scenery - prock8"                          )
    LABEL(SHAPE_TYPE_SCENERY_PROCKA,                "Scenery - procka"                          )
    LABEL(SHAPE_TYPE_SCENERY_PROCKB,                "Scenery - prockb"                          )
    LABEL(SHAPE_TYPE_SCENERY_PROCKC,                "Scenery - prockc"                          )

    LABEL(SHAPE_TYPE_SCENERY_PSPIR1,                "Scenery - pspir1"                          )
    LABEL(SHAPE_TYPE_SCENERY_PSPIR2,                "Scenery - pspir2"                          )
    LABEL(SHAPE_TYPE_SCENERY_PSPIR3,                "Scenery - pspir3"                          )
    LABEL(SHAPE_TYPE_SCENERY_PSPIR4,                "Scenery - pspir4"                          )
    LABEL(SHAPE_TYPE_SCENERY_PSPIR5,                "Scenery - pspir5"                          )

    
    // IceShapes.txt
    LABEL(SHAPE_TYPE_SCENERY_SROCK6,                "Scenery - srock6"                          )
    LABEL(SHAPE_TYPE_SCENERY_SROCK7,                "Scenery - srock7"                          )
    LABEL(SHAPE_TYPE_SCENERY_SROCK8,                "Scenery - srock8"                          )
    
    LABEL(SHAPE_TYPE_SCENERY_SSPIR1,                "Scenery - sspir1"                          )
    LABEL(SHAPE_TYPE_SCENERY_SSPIR2,                "Scenery - sspir2"                          )
    LABEL(SHAPE_TYPE_SCENERY_SSPIR3,                "Scenery - sspir3"                          )
    LABEL(SHAPE_TYPE_SCENERY_SSPIR4,                "Scenery - sspir4"                          )

    LABEL(SHAPE_TYPE_SCENERY_SORG20,                "Scenery - sorg20"                          )
    LABEL(SHAPE_TYPE_SCENERY_SORG22,                "Scenery - sorg22"                          )


    // GameShapes.txt (GLOBAL)
    LABEL(SHAPE_TYPE_SCENERY_NEXUS_BASE,            "Scenery - Nexus base"                      )
    LABEL(SHAPE_TYPE_SCENERY_NEXUS_CAP,             "Scenery - Nexus cap"                       )
    LABEL(SHAPE_TYPE_SCENERY_NEXUS_EFFECT,          "Scenery - Nexus effect"                    )

    LABEL(SHAPE_TYPE_SCENERY_STACKABLE1L,           "Scenery - stackable1l"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE2L,           "Scenery - stackable2l"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE3L,           "Scenery - stackable3l"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE4L,           "Scenery - stackable4l"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE5L,           "Scenery - stackable5l"                     )

    LABEL(SHAPE_TYPE_SCENERY_STACKABLE1M,           "Scenery - stackable1m"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE2M,           "Scenery - stackable2m"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE3M,           "Scenery - stackable3m"                     )
    
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE1S,           "Scenery - stackable1s"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE2S,           "Scenery - stackable2s"                     )
    LABEL(SHAPE_TYPE_SCENERY_STACKABLE3S,           "Scenery - stackable3s"                     )

    LABEL(SHAPE_TYPE_SCENERY_STATUE_BASE,           "Scenery - statue base"                     )
    LABEL(SHAPE_TYPE_SCENERY_STATUE_HMALE,          "Scenery - statue hmale"                    )
    
    LABEL(SHAPE_TYPE_SCENERY_VEHICLE_LAND_ASSAULT_WRECK, "Scenery - vehicle land assault wreck" )
    LABEL(SHAPE_TYPE_SCENERY_VEHICLE_AIR_SCOUT_WRECK,    "Scenery - vehicle air scout wreck"    )


END_LABEL_SET(ShapeTypes)

#endif	// __SHAPE_TYPES_H__
