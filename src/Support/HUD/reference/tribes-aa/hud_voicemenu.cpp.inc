//==============================================================================
//
//  hud_VoiceMenu.cpp
//
//==============================================================================

#include "hud_manager.hpp"
#include "hud_voicemenu.hpp"
//#include "ui_manager.hpp"
#include "hud_voice_ids.hpp"



//==============================================================================
//  Macros
//==============================================================================

#define DEF_STATE(_state_,_title_)  voice_table     vc_##_state_ = { _title_, {
#define END_STATE                   { 0, 0, 0, 0, HUD_VID_NULL} } };

#define DEF_VOICE(_pos_,_label_,_text_,_id_,_vid_)  {_pos_,_label_,_text_,_id_,_vid_},
#define DEF_TRANS(_pos_,_label_,_state_)            {_pos_,_label_, (const char*)&vc_##_state_, -1, HUD_VID_NULL},



//==============================================================================

//==============================================================================
//  Root:Root4:Warning
//==============================================================================

DEF_STATE( Warning, "Warning" )
    DEF_VOICE(  5, "Bomber",                "Incoming bomber!",                             130,    HUD_VID_NULL )
    DEF_VOICE(  7, "Enemies",               "Incoming hostiles!",                           131,    HUD_VID_NULL )
    DEF_VOICE( 11, "Vehicles",              "Incoming vehicles!",                           132,    HUD_VID_NULL )
    DEF_VOICE(  3, "Watch fire",            "Watch where you're shooting!",                 133,    HUD_VID_NULL )
END_STATE

//==============================================================================
//  Root:Root4:Target
//==============================================================================

DEF_STATE( Target1, "Target1" )
    DEF_VOICE(  5, "Base",                  "Target the enemy base, I'm in position.",      111,    HUD_VID_NULL )
    DEF_VOICE(  7, "Destroyed",             "Target destroyed.",                            112,    HUD_VID_NULL )
    DEF_VOICE( 11, "Flag",                  "Target their flag, I'm in position.",          113,    HUD_VID_NULL )
END_STATE

DEF_STATE( Target2, "Target2" )
    DEF_VOICE(  7, "Fire on",               "Fire on my target.",                           116,    HUD_VID_NULL )
    DEF_VOICE( 11, "Need",                  "Need a target painted.",                       114,    HUD_VID_NULL )
END_STATE

DEF_STATE( Target3, "Target3" )
    DEF_VOICE( 11, "Sensors",               "Target their sensors, I'm in position.",       115,    HUD_VID_NULL )
    DEF_VOICE(  3, "Turret",                "Target their turrets, I'm in position.",       117,    HUD_VID_NULL )
    DEF_VOICE(  5, "Wait",                  "Wait, I'll be in range soon.",                 118,    HUD_VID_NULL )
END_STATE

DEF_STATE( Target, "Target" )
    DEF_VOICE(  3, "Acquired",              "Target acquired.",                             110,    HUD_VID_NULL )
    DEF_TRANS(  4, "Base",                  Target1 )
    DEF_TRANS(  5, "Destroyed",             Target1 )
    DEF_TRANS(  6, "Flag",                  Target1 )
    DEF_TRANS(  7, "Fire on",               Target2 )
    DEF_TRANS(  8, "Need",                  Target2 )
    DEF_TRANS( 10, "Sensors",               Target3 )
    DEF_TRANS( 11, "Turret",                Target3 )
    DEF_TRANS( 12, "Wait",                  Target3 )
END_STATE

//==============================================================================
//  Root:Root4:Self
//==============================================================================

DEF_STATE( Attack_Self1, "Attack1" )
    DEF_VOICE(  5, "Generator",             "I'll attack the enemy generator!",          84,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Sensors",               "I'll attack the enemy sensors!",            85,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Turrets",               "I'll attack the enemy turrets!",            86,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Attack_Self2, "Attack2" )
    DEF_VOICE( 11, "Attack",                "I'll attack!",                              81,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Base",                  "I'll attack the enemy base!",               82,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Attack_Self, "Attack" )
    DEF_TRANS( 11, "Attack",                Attack_Self2 )
    DEF_TRANS( 12, "Base",                  Attack_Self2 )
    DEF_VOICE(  3, "Flag",                  "I'll go for the enemy flag!",               83,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Generator",             Attack_Self1 )
    DEF_TRANS(  5, "Sensors",               Attack_Self1 )
    DEF_TRANS(  6, "Turrets",               Attack_Self1 )
    DEF_VOICE(  7, "Vehicle bay",           "I'll attack the enemy vehicle station!",    87,    HUD_VID_NULL  )
END_STATE

//==============================================================================

DEF_STATE( Task_Self1, "Task1" )
    DEF_VOICE(  5, "Sensors",               "I'll deploy sensors.",                         107,    HUD_VID_NULL )
    DEF_VOICE(  7, "Turrets",               "I'll deploy turrets.",                         108,    HUD_VID_NULL )
    DEF_VOICE( 11, "Vehicle",               "I'll get a vehicle ready.",                    109,    HUD_VID_NULL )
END_STATE

DEF_STATE( Task_Self2, "Task2" )
    DEF_VOICE( 11, "Set Defense",           "I'll deploy sensors.",                         104,    HUD_VID_NULL )
    DEF_VOICE(  3, "On it",                 "I'm on it.",                                   105,    HUD_VID_NULL )
END_STATE

DEF_STATE( Task_Self, "Self - Task" )
    DEF_VOICE(  7, "Covering",              "I'll cover you.",                              103,    HUD_VID_NULL )
    DEF_TRANS( 11, "Set Defense",           Task_Self2 )
    DEF_TRANS( 12, "On it",                 Task_Self2 )
    DEF_VOICE(  3, "Remotes",               "I'll deploy remote equipment.",                106,    HUD_VID_NULL )
    DEF_TRANS(  4, "Sensors",               Task_Self1 )
    DEF_TRANS(  5, "Turrets",               Task_Self1 )
    DEF_TRANS(  6, "Vehicle",               Task_Self1 )
END_STATE

//==============================================================================

DEF_STATE( Repair_Self1, "Repair1" )
    DEF_VOICE(  5, "Base",                  "I'll repair our base.",                  96,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Equipment",             "I'll repair our equipment.",             97,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Generator",             "I'll repair our generators.",            98,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Repair_Self2, "Repair2" )
    DEF_VOICE( 11, "Sensors",               "I'll repair our sensors.",               100,    HUD_VID_NULL )
    DEF_VOICE(  3, "Turrets",               "I'll repair our turrets.",               101,    HUD_VID_NULL )
END_STATE

DEF_STATE( Repair_Self, "Repair" )
    DEF_TRANS(  4, "Base",                  Repair_Self1 )
    DEF_TRANS(  5, "Equipment",             Repair_Self1 )
    DEF_TRANS(  6, "Generator",             Repair_Self1 )
    DEF_VOICE(  7, "Repairs",               "I'm on repairs.",                        99,    HUD_VID_NULL  )
    DEF_TRANS( 11, "Sensors",               Repair_Self2 )
    DEF_TRANS( 12, "Turrets",               Repair_Self2 )
    DEF_VOICE(  3, "Vehicle bay",           "I'll repair our vehicle station.",       102,    HUD_VID_NULL )
END_STATE

//==============================================================================

DEF_STATE( Defend_Self1, "Defend1" )
    DEF_VOICE(  5, "Defending",             "I'm defending.",                            89,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Flag",                  "I'll defend our flag.",                     90,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Generator",             "I'll defend our generator.",                91,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Defend_Self2, "Defend2" )
    DEF_VOICE( 11, "Turrets",               "I'll defend our turrets.",                  94,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Vehicle bay",           "I'll defend our vehicle station.",          95,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Defend_Self, "Defend" )
    DEF_VOICE(  3, "Base",                  "I'll defend our base.",                     88,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Defending",             Defend_Self1 )
    DEF_TRANS(  5, "Flag",                  Defend_Self1 )
    DEF_TRANS(  6, "Generator",             Defend_Self1 )
//    DEF_VOICE(  7, "I'll defend\nnexus",       "I'll defend the nexus.",                    92,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Sensors",               "I'll defend our sensors.",                  93,    HUD_VID_NULL  )
    DEF_TRANS( 11, "Turrets",               Defend_Self2 )
    DEF_TRANS( 12, "Vehicle bay",           Defend_Self2 )
END_STATE

//==============================================================================

DEF_STATE( Self, "Self" )
    DEF_TRANS( 11, "Attack",                Attack_Self )
    DEF_TRANS(  3, "Defend",                Defend_Self )
    DEF_TRANS(  5, "Repair",                Repair_Self )
    DEF_TRANS(  7, "Task",                  Task_Self   )
END_STATE

//==============================================================================
//  Root:Root3:Quick
//==============================================================================

DEF_STATE( Quick1, "Quick1" )
    DEF_VOICE(  3, "Yes",                   "Yes.",                                         129,    HUD_VID_NULL )
    DEF_VOICE(  5, "No",                    "No.",                                          125,    HUD_VID_NULL )
END_STATE

DEF_STATE( Quick2, "Quick2" )
    DEF_VOICE(  5, "Anytime",               "Anytime.",                                     119,    HUD_VID_NULL )
    DEF_VOICE(  7, "Secure?",               "Is our base secure?",                          120,    HUD_VID_NULL )
    DEF_VOICE( 11, "Cease fire",            "Cease fire!",                                  121,    HUD_VID_NULL )
END_STATE

DEF_STATE( Quick3, "Quick3" )
    DEF_VOICE(  7, "Don't know",            "No clue.",                                     122,    HUD_VID_NULL )
    DEF_VOICE( 11, "HELP!",                 "Help!",                                        123,    HUD_VID_NULL )
    DEF_VOICE(  3, "Move",                  "Move! Move! Move!",                            124,    HUD_VID_NULL )
END_STATE

DEF_STATE( Quick4, "Quick4" )
    DEF_VOICE( 11, "Sorry",                 "Sorry.",                                       126,    HUD_VID_NULL )
    DEF_VOICE(  3, "Thanks",                "Thanks.",                                      127,    HUD_VID_NULL )
    DEF_VOICE(  5, "Wait",                  "Wait up.",                                     128,    HUD_VID_NULL )
END_STATE

DEF_STATE( Quick, "Quick" )
    DEF_TRANS(  2, "Yes",                   Quick1 )
    DEF_TRANS(  3, "No",                    Quick1 )
    DEF_TRANS(  4, "Anytime",               Quick2 )
    DEF_TRANS(  5, "Secure?",               Quick2 )
    DEF_TRANS(  6, "Cease fire",            Quick2 )
    DEF_TRANS(  7, "Don't know",            Quick3 )
    DEF_TRANS(  8, "HELP!",                 Quick3 )
    DEF_TRANS(  9, "Move",                  Quick3 )
    DEF_TRANS( 10, "Sorry",                 Quick4 )
    DEF_TRANS( 11, "Thanks",                Quick4 )
    DEF_TRANS( 12, "Wait",                  Quick4 )
END_STATE

//==============================================================================
//  Root:Root3:Need
//==============================================================================

DEF_STATE( Need1, "Need1" )
    DEF_VOICE(  3, "Escort",                "Vehicle needs escort.",                       67,    HUD_VID_NULL  )
    DEF_VOICE(  5, "Gunship",               "Gunship ready, need a ride?",                 68,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Need2, "Need2" )
    DEF_VOICE(  5, "Hold vehicle",          "Hold that vehicle, I'm coming!",              69,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Pilot",                 "Need a pilot for turbo grav!",                70,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Ride",                  "I need a ride!",                              71,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Need3, "Need3" )
    DEF_VOICE(  7, "Where to?",             "Where to?",                                   74,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Support",               "Need vehicle support.",                       72,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Tailgunner",            "Need a tailgunner!",                          73,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Need4, "Need4" )
    DEF_VOICE( 11, "Bombardier",            "Need a bombardier.",                          64,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Cover",                 "Need covering fire.",                         65,    HUD_VID_NULL  )
    DEF_VOICE(  5, "Driver",                "Need a driver for ground vehicle.",           66,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Need, "Need" )
    DEF_TRANS( 10, "Bombardier",            Need4 )
    DEF_TRANS( 11, "Cover",                 Need4 )
    DEF_TRANS( 12, "Driver",                Need4 )
    DEF_TRANS(  2, "Escort",                Need1 )
    DEF_TRANS(  3, "Gunship",               Need1 )
    DEF_TRANS(  4, "Hold vehicle",          Need2 )
    DEF_TRANS(  5, "Pilot",                 Need2 )
    DEF_TRANS(  6, "Ride",                  Need2 )
    DEF_TRANS(  7, "Where to?",             Need3 )
    DEF_TRANS(  8, "Support",               Need3 )
    DEF_TRANS(  9, "Tailgunner",            Need3 )
END_STATE

//==============================================================================
//  Root:Root3:Flag
//==============================================================================

DEF_STATE( Flag1, "Flag1" )
    DEF_VOICE(  5, "Retrieve",              "Retrieve our flag.",                          41,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Secure",                "Our flag is secure.",                         42,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Take flag",             "Take the flag from me.",                      43,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Flag, "Flag" )
    DEF_VOICE(  7, "Defend",                "Defend our flag.",                            21,    HUD_VID_NULL  )
    DEF_VOICE( 11, "I have",                "I have the enemy flag.",                      37,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Give me",               "Give me the flag.",                           38,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Retrieve",              Flag1 )
    DEF_TRANS(  5, "Secure",                Flag1 )
    DEF_TRANS(  6, "Take flag",             Flag1 )
//    DEF_VOICE( "Give me\nyour flags",    "Give your flags to me.",                          39,    HUD_VID_NULL  )
//    DEF_VOICE( "Take my\nflags",         "Take my flags.",                                  40,    HUD_VID_NULL  )
END_STATE

//==============================================================================
//  Root:Root2:Enemy
//==============================================================================

DEF_STATE( Enemy1, "Enemy1" )
    DEF_VOICE(  5, "Remotes",               "Enemy remote equipment destroyed.",           33,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Sensors",               "Enemy sensors destroyed.",                    34,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Turrets",               "Enemy turrets destroyed.",                    35,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Enemy2, "Enemy2" )
    DEF_VOICE( 11, "Base",                  "Enemy base disabled.",                        30,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Disarray",              "Enemy is disrupted. Attack!",                 31,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Enemy, "Enemy" )
    DEF_TRANS( 11, "Base",                  Enemy2 )
    DEF_TRANS( 12, "Disarray",              Enemy2 )
    DEF_VOICE(  3, "Generator",             "Enemy generator destroyed.",                  32,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Remotes",               Enemy1 )
    DEF_TRANS(  5, "Sensors",               Enemy1 )
    DEF_TRANS(  6, "Turrets",               Enemy1 )
    DEF_VOICE(  7, "Vehicle bay",           "Enemy vehicle station destroyed.",            36,    HUD_VID_NULL  )
END_STATE

//==============================================================================
//  Root:Root2:Base
//==============================================================================

DEF_STATE( Base1, "Base1" )
    DEF_VOICE(  5, "Secure",                "Our base is secure.",                          16,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Taken",                 "Our base is taken!",                           17,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Base, "Base" )
    DEF_VOICE(  7, "Clear",                 "Our base is clear.",                           13,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Enemy in",              "Enemy is in our base!",                        14,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Retake",                "Retake our base!",                             15,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Secure",                Base1 )
    DEF_TRANS(  5, "Taken",                 Base1 )
END_STATE

//==============================================================================
//  Root:Root2:Repair
//==============================================================================

DEF_STATE( Repair1, "Repair1" )
    DEF_VOICE(  5, "Base",                  "Repair our base!",                             75,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Generator",             "Repair our generator!",                        76,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Repair me",             "Repair me!",                                   77,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Repair, "Repair" )
    DEF_TRANS(  4, "Base",                  Repair1 )
    DEF_TRANS(  5, "Generator",             Repair1 )
    DEF_TRANS(  6, "Repair me",             Repair1 )
    DEF_VOICE(  7, "Sensors",               "Repair our sensors!",                          78,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Turrets",               "Repair our turrets!",                          79,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Vehicle bay",           "Repair our vehicle station!",                  80,    HUD_VID_NULL  )
END_STATE

//==============================================================================
//  Root:Root1:Global
//==============================================================================

DEF_STATE( Respond, "Respond" )
    DEF_VOICE(  3, "Anytime",               "Anytime.",                                     VM_GLOBAL | 44,    HUD_VID_NULL  )
    DEF_VOICE(  5, "Don't know",            "Don't know.",                                  VM_GLOBAL | 47,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Thanks",                "Thanks.",                                      VM_GLOBAL | 60,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Wait",                  "Wait up.",                                     VM_GLOBAL | 61,    HUD_VID_NULL  )
END_STATE

//==============================================================================

DEF_STATE( Animations1, "Animations1" )
    DEF_VOICE(  5, "Hi",                    "Hi.",                                          VM_GLOBAL | 49,    HUD_VID_ANIMS1_HI        )
    DEF_VOICE(  7, "Take that",             "HAHAHAHA!",                                    VM_GLOBAL | 57,    HUD_VID_ANIMS1_TAKE_THAT )
    DEF_VOICE( 11, "Awww...",               "Awww...too bad.",                              VM_GLOBAL | 56,    HUD_VID_ANIMS1_AWWW      )
END_STATE

DEF_STATE( Animations2, "Animations2" )
    DEF_VOICE( 11, "Move",                  "Move! Move! Move!",                            VM_GLOBAL | 124,   HUD_VID_ANIMS2_MOVE      )
    DEF_VOICE(  3, "I'm Great",             "I got your number! I got all your numbers!",   VM_GLOBAL | 58,    HUD_VID_ANIMS2_IM_GREAT  )
    DEF_VOICE(  5, "Woohoo",                "Woohoo!",                                      VM_GLOBAL | 62,    HUD_VID_ANIMS2_WOOHOO    )
END_STATE

DEF_STATE( Animations, "Animations" )
    DEF_TRANS( 10, "Move",                  Animations2 )
    DEF_TRANS( 11, "I'm Great",             Animations2 )
    DEF_TRANS( 12, "Woohoo",                Animations2 )
    DEF_VOICE(  3, "I'm on it",             "I'm on it!",                                   VM_GLOBAL | 105,   HUD_VID_ANIMS_IM_ON_IT )
    DEF_TRANS(  4, "Hi",                    Animations1  )
    DEF_TRANS(  5, "Take that",             Animations1  )
    DEF_TRANS(  6, "Awww...",               Animations1  )
    DEF_VOICE(  7, "Awesome",               "I love it!",                                   VM_GLOBAL | 45,    HUD_VID_ANIMS_AWESOME  )
END_STATE

//==============================================================================

DEF_STATE( Taunt, "Taunt" )
    DEF_VOICE(  3, "HaHa!",                 "HAHAHAHA!",                                    VM_GLOBAL | 57,    HUD_VID_TAUNT_HAHA        )
    DEF_VOICE(  5, "Got Number",            "I got your number! I got all your numbers!",   VM_GLOBAL | 58,    HUD_VID_TAUNT_GOT_NUMBER  )
    DEF_VOICE(  7, "Eat Plasma",            "Eat my plasma!",                               VM_GLOBAL | 59,    HUD_VID_TAUNT_EAT_PLASMA  )
    DEF_VOICE( 11, "Awww...",               "Awww...too bad.",                              VM_GLOBAL | 56,    HUD_VID_TAUNT_AWWW        )
END_STATE

//==============================================================================

DEF_STATE( Compliment, "Compliment" )
    DEF_VOICE(  3, "Good game!",            "Good game!",                                   VM_GLOBAL | 48,    HUD_VID_NULL )
    DEF_VOICE(  5, "You rock!",             "You rock!",                                    VM_GLOBAL | 53,    HUD_VID_NULL )
    DEF_VOICE(  7, "Great shot!",           "Great shot!",                                  VM_GLOBAL | 55,    HUD_VID_NULL )
    DEF_VOICE( 11, "Awesome!",              "I love it!",                                   VM_GLOBAL | 45,    HUD_VID_NULL )
END_STATE

//==============================================================================

DEF_STATE( Global1, "Global1" )
    DEF_VOICE(  3, "Yes",                   "Yes.",                                         63,    HUD_VID_NULL )
    DEF_VOICE(  5, "Hi",                    "Hi.",                                          49,    HUD_VID_NULL )
    DEF_VOICE(  7, "No",                    "No.",                                          50,    HUD_VID_NULL )
END_STATE

DEF_STATE( Global2, "Global2" )
    DEF_VOICE(  5, "Bye",                   "Bye.",                                         46,    HUD_VID_NULL )
    DEF_VOICE(  7, "Oops",                  "Oops.",                                        51,    HUD_VID_NULL )
    DEF_VOICE( 11, "Quiet!",                "Quiet!",                                       52,    HUD_VID_NULL )
END_STATE

DEF_STATE( Global3, "Global3" )
    DEF_VOICE(  7, "Shazbot!",              "Shazbot!",                                     54,    HUD_VID_NULL )
    DEF_VOICE( 11, "Woohoo!",               "Woohoo!",                                      62,    HUD_VID_NULL )
    DEF_TRANS(  3, "Respond",               Respond )
END_STATE

DEF_STATE( Global4, "Global4" )
    DEF_TRANS( 11, "Animations",            Animations )
    DEF_TRANS(  3, "Taunt",                 Taunt      )
    DEF_TRANS(  5, "Compliment",            Compliment )
END_STATE

DEF_STATE( Global, "Global" )
    DEF_TRANS(  1, "Yes",                   Global1 )
    DEF_TRANS(  2, "Hi",                    Global1 )
    DEF_TRANS(  3, "No",                    Global1 )
    DEF_TRANS(  4, "Bye",                   Global2 )
    DEF_TRANS(  5, "Oops",                  Global2 )
    DEF_TRANS(  6, "Quiet!",                Global2 )
    DEF_TRANS(  7, "Shazbot!",              Global3 )
    DEF_TRANS(  8, "Woohoo!",               Global3 )
    DEF_TRANS(  9, "Respond",               Global3 )
    DEF_TRANS( 10, "Animations",            Global4 )
    DEF_TRANS( 11, "Taunt",                 Global4 )
    DEF_TRANS( 12, "Compliment",            Global4 )
END_STATE

//==============================================================================
//  Root:Root1:Attack
//==============================================================================

DEF_STATE( Attack1, "Attack1" )
    DEF_VOICE(  5, "Attack!",               "Attack!",                                      1,    HUD_VID_NULL   )
    DEF_VOICE(  7, "Attack base",           "Attack the enemy base!",                       2,    HUD_VID_NULL   )
    DEF_VOICE( 11, "Chase flag",            "Recover our flag!",                            3,    HUD_VID_NULL   )
END_STATE

DEF_STATE( Attack2, "Attack2" )
    DEF_VOICE(  7, "Disrupt",               "Disrupt the enemy defense!",                   4,    HUD_VID_NULL   )
    DEF_VOICE( 11, "Get flag",              "Get the enemy flag!",                          5,    HUD_VID_NULL   )
    DEF_VOICE(  3, "Generator",             "Destroy the enemy generator!",                 6,    HUD_VID_NULL   )
END_STATE

DEF_STATE( Attack3, "Attack3" )
    DEF_VOICE( 11, "Reinforce",             "Reinforce the offense!",                       8,    HUD_VID_NULL   )
    DEF_VOICE(  3, "Sensors",               "Destroy enemy sensors!",                       9,    HUD_VID_NULL   )
    DEF_VOICE(  5, "Turrets",               "Destroy enemy turrets!",                       10,   HUD_VID_NULL  )
END_STATE

DEF_STATE( Attack, "Attack" )
    DEF_TRANS(  4, "Attack!",               Attack1 )
    DEF_TRANS(  5, "Attack base",           Attack1 )
    DEF_TRANS(  6, "Chase flag",            Attack1 )
    DEF_TRANS(  7, "Disrupt",               Attack2 )
    DEF_TRANS(  8, "Get flag",              Attack2 )
    DEF_TRANS(  9, "Generator",             Attack2 )
//    DEF_VOICE( "Destroy\nobjective",    "Destroy the objective!",                           7,    HUD_VID_NULL   )
    DEF_TRANS( 10, "Reinforce",             Attack3 )
    DEF_TRANS( 11, "Sensors",               Attack3 )
//    DEF_VOICE( "Wait for\nmy signal",   "Wait for my signal before attacking.",             12,    HUD_VID_NULL  )
    DEF_TRANS( 12, "Turrets",               Attack3 )
    DEF_VOICE(  3, "Vehicle",               "Destroy enemy vehicle!",                       11,    HUD_VID_NULL  )
END_STATE

//==============================================================================
//  Root:Root1:Defend
//==============================================================================

DEF_STATE( Defend1, "Defend1" )
    DEF_VOICE(  5, "Sensors",               "Defend our sensors!",                          27,    HUD_VID_NULL  )
    DEF_VOICE(  7, "Turrets",               "Defend our turrets!",                          28,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Vehicle",               "Defend our vehicle!",                          29,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Defend2, "Defend2" )
    DEF_VOICE(  7, "Base",                  "Defend our base!",                             18,    HUD_VID_NULL  )
    DEF_VOICE( 11, "Flag carrier",          "Cover our flag carrier!",                      19,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Entrances",             "Defend our entrances!",                        20,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Defend3, "Defend3" )
    DEF_VOICE( 11, "Flag",                  "Defend our flag!",                             21,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Generator",             "Defend our generator!",                        22,    HUD_VID_NULL  )
    DEF_VOICE(  5, "Cover me!",             "Cover me!",                                    23,    HUD_VID_NULL  )
END_STATE

DEF_STATE( Defend, "Defend" )
    DEF_TRANS(  7, "Base",                  Defend2 )
    DEF_TRANS(  8, "Flag carrier",          Defend2 )
    DEF_TRANS(  9, "Entrances",             Defend2 )
    DEF_TRANS( 10, "Flag",                  Defend3 )
    DEF_TRANS( 11, "Generator",             Defend3 )
    DEF_TRANS( 12, "Cover me!",             Defend3 )
//    DEF_VOICE( "Defend\nthe nexus",     "Defend the nexus!",                            24,    HUD_VID_NULL  )
//    DEF_VOICE( "Defend\nobjective",     "Defend the objective!",                        25,    HUD_VID_NULL  )
    DEF_VOICE(  3, "Reinforce",             "Reinforce our defense!",                       26,    HUD_VID_NULL  )
    DEF_TRANS(  4, "Sensors",               Defend1 )
    DEF_TRANS(  5, "Turrets",               Defend1 )
    DEF_TRANS(  6, "Vehicle",               Defend1 )
END_STATE

//==============================================================================
//  Root
//==============================================================================

DEF_STATE( Root1, "" )
    DEF_TRANS(  3, "Global",                Global  )
    DEF_TRANS(  5, "Attack",                Attack  )
    DEF_TRANS(  7, "Defend",                Defend  )
END_STATE

DEF_STATE( Root2, "" )
    DEF_TRANS(  5, "Repair",                Repair  )
    DEF_TRANS(  7, "Base",                  Base    )
    DEF_TRANS( 11, "Enemy",                 Enemy   )
END_STATE

DEF_STATE( Root3, "" )
    DEF_TRANS(  7, "Flag",                  Flag    )
    DEF_TRANS( 11, "Need",                  Need    )
    DEF_TRANS(  3, "Quick",                 Quick   )
END_STATE

DEF_STATE( Root4, "" )
    DEF_TRANS( 11, "Self",                  Self    )
    DEF_TRANS(  3, "Target",                Target  )
    DEF_TRANS(  5, "Warning",               Warning )
END_STATE

DEF_STATE( Root, "" )
    DEF_TRANS(  1, "Global",                Root1 )
    DEF_TRANS(  2, "Attack",                Root1 )
    DEF_TRANS(  3, "Defend",                Root1 )
    DEF_TRANS(  4, "Repair",                Root2 )
    DEF_TRANS(  5, "Base",                  Root2 )
    DEF_TRANS(  6, "Enemy",                 Root2 )
    DEF_TRANS(  7, "Flag",                  Root3 )
    DEF_TRANS(  8, "Need",                  Root3 )
    DEF_TRANS(  9, "Quick",                 Root3 )
    DEF_TRANS( 10, "Self",                  Root4 )
    DEF_TRANS( 11, "Target",                Root4 )
    DEF_TRANS( 12, "Warning",               Root4 )
END_STATE

//==============================================================================
//  Voice Command Helper Functions
//==============================================================================

static void VoiceMenu_Decompress( voice_table& Dst, voice_table& Src )
{
    s32 i = 0;

    // Clear Dst
    for( i=0 ; i<VM_NUM_ENTRIES ; i++ )
    {
        Dst.Entries[i].Pos    = 0;
        Dst.Entries[i].pLabel = 0;
        Dst.Entries[i].pText  = 0;
        Dst.Entries[i].ID     = 0;
    }

    // Decompress Src
    i = 0;
    while( (i < VM_NUM_ENTRIES) && (Src.Entries[i].Pos != 0) )
    {
        s32 Pos = Src.Entries[i].Pos - 1;
        Dst.Entries[Pos] = Src.Entries[i];
        i++;
    }
}

//==============================================================================
//  Voice Command Functions
//==============================================================================

void VoiceMenu_Init( voice_table& VoiceMenu )
{
    // Decompress Root menu into VoiceTable
    VoiceMenu_Decompress( VoiceMenu, vc_Root );
}

s32 VoiceMenu_Navigate( voice_table& VoiceMenu, s32 Direction, s32& HudVoiceID  )
{
    static s32  IndexTable[4] = {3,5,7,11};
    s32 Result = -1;
    s32 Index  = -1;

    // Default
    HudVoiceID = HUD_VID_NULL ;

    ASSERT( (Direction >= 0) && (Direction < 4) );

    // Get Index into Menu from direction of navigation
    Index = IndexTable[Direction]-1;

    // Decompress Next Menu in sequence
    if( Index != -1 )
    {
        Result     = VoiceMenu.Entries[Index].ID;
        HudVoiceID = VoiceMenu.Entries[Index].HudVoiceID;
        if( Result == -1 )
            VoiceMenu_Decompress( VoiceMenu, *(voice_table*)VoiceMenu.Entries[Index].pText );
    }

    return Result;
}

//==============================================================================
