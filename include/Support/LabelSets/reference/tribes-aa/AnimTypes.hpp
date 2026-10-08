#ifndef __ANIM_TYPES_H__
#define __ANIM_TYPES_H__


//******************************************************************************
// NOTE - Unless you add types to the end only, you MUST rebuild your binary
//        shape files, otherwise they will become out of sync with these types!
//******************************************************************************


// Animation types - assigned one per animation
BEGIN_LABEL_SET(AnimTypes)
    LABEL(ANIM_TYPE_NONE,                           "<un-assigned>"                         )
    
    // Character main animations
    LABEL(ANIM_TYPE_CHARACTER_IDLE_GAME_START,      "Character - Idle game start"           )
    LABEL(ANIM_TYPE_CHARACTER_IDLE,                 "Character - Idle"                      )
                                                                                            
    LABEL(ANIM_TYPE_CHARACTER_FORWARD,              "Character - Forward"                   )
    LABEL(ANIM_TYPE_CHARACTER_BACKWARD,             "Character - Backward"                  )
    LABEL(ANIM_TYPE_CHARACTER_SIDE,                 "Character - Side"                      )
                                                                                            
    LABEL(ANIM_TYPE_CHARACTER_JET,                  "Character - Jet"                       )
    LABEL(ANIM_TYPE_CHARACTER_JUMP_UP,              "Character - Jump up"                   )
    LABEL(ANIM_TYPE_CHARACTER_FALL,                 "Character - Fall"                      )
    LABEL(ANIM_TYPE_CHARACTER_LAND,                 "Character - Land"                      )

    // Special vehicle animations
    LABEL(ANIM_TYPE_CHARACTER_PILOT_GRAV_CYCLE,     "Character - Pilot grav cycle"          )
    LABEL(ANIM_TYPE_CHARACTER_SITTING,              "Character - Sitting"                   )

    // Special pose animations
    LABEL(ANIM_TYPE_CHARACTER_SNIPER_POSE,          "Character - Sniper pose"               )
    LABEL(ANIM_TYPE_CHARACTER_ROCKET_POSE,          "Character - Rocket pose"               )
    LABEL(ANIM_TYPE_CHARACTER_BIND_POSE,            "Character - bind pose"                 )

    // Celebrate animations
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE,            "Character - Celebrate generic"         )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_BOW,        "Character - Celebrate bow"             )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_DANCE,      "Character - Celebrate dance"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_SALUTE,     "Character - Celebrate salute"          )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_WAVE,       "Character - Celebrate wave"            )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_DISCO,      "Character - Celebrate disco"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_FLEX,       "Character - Celebrate flex"            )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_ROCKY,      "Character - Celebrate rocky"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_TAUNT,      "Character - Celebrate taunt"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_JUMP,       "Character - Celebrate jump"            )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_GORILLA,    "Character - Celebrate gorilla"         )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_GUN,        "Character - Celebrate gun"             )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_ROAR,       "Character - Celebrate roar"            )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_SHOOT,      "Character - Celebrate shoot"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_SWIPE,      "Character - Celebrate swipe"           )
    LABEL(ANIM_TYPE_CHARACTER_CELEBRATE_YEAH,       "Character - Celebrate yeah"            )

    // Taunt animations
    LABEL(ANIM_TYPE_CHARACTER_TAUNT,                "Character - Taunt generic"             )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_BEST,           "Character - Taunt best"                )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_BUTT,           "Character - Taunt butt"                )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_IMP,            "Character - Taunt imp"                 )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_KISS,           "Character - Taunt kiss"                )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_BULL,           "Character - Taunt bull"                )
    LABEL(ANIM_TYPE_CHARACTER_TAUNT_HEAD,           "Character - Taunt head"                )

    // Death animations
    LABEL(ANIM_TYPE_CHARACTER_DIE_FORWARD,          "Character - Die forward"               )
    LABEL(ANIM_TYPE_CHARACTER_DIE_BACKWARD,         "Character - Die backward"              )
    LABEL(ANIM_TYPE_CHARACTER_DIE_SIDE,             "Character - Die side"                  )
    LABEL(ANIM_TYPE_CHARACTER_DIE_CHEST,            "Character - Die chest"                 )
    LABEL(ANIM_TYPE_CHARACTER_DIE_HEAD,             "Character - Die head"                  )
    LABEL(ANIM_TYPE_CHARACTER_DIE_KNEES,            "Character - Die knees"                 )
    LABEL(ANIM_TYPE_CHARACTER_DIE_LEFT_LEG,         "Character - Die left leg"              )
    LABEL(ANIM_TYPE_CHARACTER_DIE_RIGHT_LEG,        "Character - Die right leg"             )
    LABEL(ANIM_TYPE_CHARACTER_DIE_LEFT_SIDE,        "Character - Die left side"             )
    LABEL(ANIM_TYPE_CHARACTER_DIE_RIGHT_SIDE,       "Character - Die right side"            )
    LABEL(ANIM_TYPE_CHARACTER_DIE_SLUMP,            "Character - Die slump"                 )
    LABEL(ANIM_TYPE_CHARACTER_DIE_SPIN,             "Character - Die spin"                  )

    // Character additive layered animations
    LABEL(ANIM_TYPE_CHARACTER_RECOIL,                   "Character - Additive recoil"                   )
    LABEL(ANIM_TYPE_CHARACTER_LOOK_SOUTH_NORTH,         "Character - Additive look south north"         )
    LABEL(ANIM_TYPE_CHARACTER_SNIPER_LOOK_SOUTH_NORTH,  "Character - Additive sniper look south north"  )
    LABEL(ANIM_TYPE_CHARACTER_ROCKET_LOOK_SOUTH_NORTH,  "Character - Additive rocket look south north"  )
    LABEL(ANIM_TYPE_CHARACTER_HEAD_UP_DOWN,             "Character - Additive head up down"             )
    LABEL(ANIM_TYPE_CHARACTER_HEAD_LEFT_RIGHT,          "Character - Additive head left right"          )

    // Weapon animations
    LABEL(ANIM_TYPE_WEAPON_ACTIVATE,                "Weapon - Activate"                     )
    LABEL(ANIM_TYPE_WEAPON_IDLE,                    "Weapon - Idle"                         )
    LABEL(ANIM_TYPE_WEAPON_FIRE,                    "Weapon - Fire"                         )
    LABEL(ANIM_TYPE_WEAPON_RELOAD,                  "Weapon - Reload"                       )
    LABEL(ANIM_TYPE_WEAPON_DEACTIVATE,              "Weapon - Deactivate"                   )

    // GUI animations
    LABEL(ANIM_TYPE_GUI_MAINSCR_INTRO,              "GUI - Main Screen Intro"               )
    LABEL(ANIM_TYPE_GUI_MAINSCR_HOST,               "GUI - Main Screen->Host Game"          )
    LABEL(ANIM_TYPE_GUI_MAINSCR_JOIN,               "GUI - Main Screen->Join Game"          )
    LABEL(ANIM_TYPE_GUI_HOSTSCR_DONE,               "GUI - Host Game->Outro"  	 	        )
    LABEL(ANIM_TYPE_GUI_JOINSCR_DONE,               "GUI - Join Game->Outro"                )

    // Asset general animation
    LABEL(ANIM_TYPE_ASSET_IDLE,                     "Asset - Idle"                          )
    LABEL(ANIM_TYPE_ASSET_ACTIVE,                   "Asset - Active"                        )
    LABEL(ANIM_TYPE_ASSET_DESTROYED,                "Asset - Destroyed"                     )
    
    // Turret animations
    LABEL(ANIM_TYPE_TURRET_IDLE,                    "Turret - Idle"                         )
    LABEL(ANIM_TYPE_TURRET_DEPLOY,                  "Turret - Deploy"                       )
    LABEL(ANIM_TYPE_TURRET_ACTIVATE,                "Turret - Activate"                     )
    LABEL(ANIM_TYPE_TURRET_DEACTIVATE,              "Turret - Deactivate"                   )
    LABEL(ANIM_TYPE_TURRET_FIRE,                    "Turret - Fire"                         )
    LABEL(ANIM_TYPE_TURRET_PITCH,                   "Turret - Pitch"                        )
    LABEL(ANIM_TYPE_TURRET_YAW,                     "Turret - Yaw"                          )
    LABEL(ANIM_TYPE_TURRET_DESTROYED,               "Turret - Destroyed"                    )

    // Inventory station animations
    LABEL(ANIM_TYPE_INVENT_DEPLOY,                  "Inventory - Deploy"                    )
    LABEL(ANIM_TYPE_INVENT_IDLE,                    "Inventory - Idle"                      )
    LABEL(ANIM_TYPE_INVENT_ACTIVATE,                "Inventory - Activate"                  )
    LABEL(ANIM_TYPE_INVENT_DEACTIVATE,              "Inventory - Deactivate"                )
    LABEL(ANIM_TYPE_INVENT_DESTROYED,               "Inventory - Destroyed"                 )

    // Mine animations
    LABEL(ANIM_TYPE_MINE_IDLE,                      "Mine - Idle"                           )
    LABEL(ANIM_TYPE_MINE_DEPLOY,                    "Mine - Deploy"                         )

    // Vehicle animations
    LABEL(ANIM_TYPE_VEHICLE_SPAWN,                  "Vehicle - Spawn"                       )
    LABEL(ANIM_TYPE_VEHICLE_IDLE,                   "Vehicle - Idle"                        )
    LABEL(ANIM_TYPE_VEHICLE_DEPLOY,                 "Vehicle - Deploy"                      )
    LABEL(ANIM_TYPE_VEHICLE_DESTROYED,              "Vehicle - Destroyed"                   )

END_LABEL_SET(AnimTypes)


#endif	// __ANIM_TYPES_H__
