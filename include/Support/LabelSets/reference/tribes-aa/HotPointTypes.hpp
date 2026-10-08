#ifndef __HOT_POINT_TYPES_H__
#define __HOT_POINT_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************



// Hot points - special purpose points attached to nodes
BEGIN_LABEL_SET(HotPointTypes)
    LABEL(HOT_POINT_TYPE_NONE,                          "<un-assigned>"                             )
                                                                                                    
    // Weapon specific id's                                                                         
    LABEL(HOT_POINT_TYPE_FIRE_WEAPON,                   "Weapon - Fire position"                    )
    LABEL(HOT_POINT_TYPE_MUZZLE_FLASH,                  "Weapon - Muzzle flash"                     )
                                                                                                    
    // Mount id's (specific for player and associated object)                                       
    LABEL(HOT_POINT_TYPE_WEAPON_MOUNT,                  "Mount - Weapon"                            )
    LABEL(HOT_POINT_TYPE_CAMERA_MOUNT,                  "Mount - Camera"	                        )
    LABEL(HOT_POINT_TYPE_BACKPACK_MOUNT,                "Mount - Backpack"                          )
    LABEL(HOT_POINT_TYPE_FLAG_MOUNT,                    "Mount - Flag"                              )
    LABEL(HOT_POINT_TYPE_TURRET_MOUNT,                  "Mount - Turret"                            )
    LABEL(HOT_POINT_TYPE_TURRET_BARREL_MOUNT,           "Mount - Turret barrel"                     )
    LABEL(HOT_POINT_TYPE_INVENT_MOUNT,                  "Mount - Inventory"                         )

    // Character specific id's
    LABEL(HOT_POINT_TYPE_LEFT_FOOT,			            "Character - Left foot"			            )
    LABEL(HOT_POINT_TYPE_RIGHT_FOOT,		            "Character - Right foot"		            )
    LABEL(HOT_POINT_TYPE_BACK_JETPACK_EMITTER,	        "Character - Back jetpack emitter"	        )
    LABEL(HOT_POINT_TYPE_LEFT_FOOT_JETPACK_EMITTER,	    "Character - Left foot jetpack emitter"	    )
    LABEL(HOT_POINT_TYPE_RIGHT_FOOT_JETPACK_EMITTER,	"Character - Right foot jetpack emitter"    )
    LABEL(HOT_POINT_TYPE_BUTT,                          "Character - Butt"                          )

    // Vehicle ID's
    LABEL(HOT_POINT_TYPE_VEHICLE_EYE_POS,               "Vehicle - Eye Position"                    )
    LABEL(HOT_POINT_TYPE_VEHICLE_EXHAUST1,              "Vehicle - Exhaust 1"                       )
    LABEL(HOT_POINT_TYPE_VEHICLE_EXHAUST2,              "Vehicle - Exhaust 2"                       )
    LABEL(HOT_POINT_TYPE_VEHICLE_EXHAUST3,              "Vehicle - Exhaust 3"                       )
    LABEL(HOT_POINT_TYPE_VEHICLE_EXHAUST4,              "Vehicle - Exhaust 4"                       )
    LABEL(HOT_POINT_TYPE_VEHICLE_SEAT_PILOT,            "Vehicle - Seat Pilot"                      )
    LABEL(HOT_POINT_TYPE_VEHICLE_SEAT_BOMBER,           "Vehicle - Seat Bomber"                     )
    LABEL(HOT_POINT_TYPE_VEHICLE_SEAT_GUNNER,           "Vehicle - Seat Gunner"                     )
    LABEL(HOT_POINT_TYPE_VEHICLE_SEAT_PASS,             "Vehicle - Seat Passenger"                  )

    // Invent ID's
    LABEL(HOT_POINT_TYPE_INVENT_RAY,                    "Inventory - Ray"                           )

    LABEL(HOT_POINT_TYPE_VEHICLE_STATION_SPAWN,         "VehicleStation - Vehicle Spawn Location"   )
    LABEL(HOT_POINT_TYPE_VEHICLE_STATION_TRIGGER,       "VehicleStation - Trigger Location"         )
   
    LABEL(HOT_POINT_TYPE_INVENT_LEFT_ARM_FX,            "Inventory - Left arm fx"                   )
    LABEL(HOT_POINT_TYPE_INVENT_RIGHT_ARM_FX,           "Inventory - Right arm fx"                  )

END_LABEL_SET(HotPointTypes)


#endif	// __HOT_POINT_TYPES_H__
