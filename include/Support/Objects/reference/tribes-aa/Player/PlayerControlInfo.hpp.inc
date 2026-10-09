//==============================================================================
//
//  PlayerControlInfo.hpp
//
//  Defines player control keys for the struct player_control_info
//
//==============================================================================



//==============================================================================
// PS2 CONTROLLER INPUT
//==============================================================================

//-----------------------------------------------------------------------------------------------------------------------------------------
//                          | ACTION                      | ACTIVATE KEY              |  SHIFT KEY (if any)    |  SHIFT KEY STATE (if any)
//-----------------------------------------------------------------------------------------------------------------------------------------
                        
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_MOVE_LEFT_RIGHT,          INPUT_PS2_STICK_LEFT_X,     INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_MOVE_FORWARD_BACKWARD,    INPUT_PS2_STICK_LEFT_Y,     INPUT_UNDEFINED,        0 )
                            
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_LOOK_UP_DOWN,             INPUT_PS2_STICK_RIGHT_Y,    INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_F32        ( PS2_LOOK_UP_DOWN_SPEED,       0.3                                                   )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_LOOK_LEFT_RIGHT,          INPUT_PS2_STICK_RIGHT_X,    INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_F32        ( PS2_LOOK_LEFT_RIGHT_SPEED,    0.5                                                   )
DEFINE_ATTRIBUTE_F32        ( PS2_LOOK_ZOOM_SPEED_SCALAR,   0.4                                                   )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_FREE_LOOK,			    INPUT_PS2_BTN_R_STICK,      INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_JUMP,                     INPUT_PS2_BTN_L1,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_FIRE,                     INPUT_PS2_BTN_L2,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_JET,                      INPUT_PS2_BTN_R1,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_ZOOM,                     INPUT_PS2_BTN_R2,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_NEXT_WEAPON,              INPUT_PS2_BTN_CROSS,        INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_GRENADE,                  INPUT_PS2_BTN_TRIANGLE,     INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_VOICE_MENU,               INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_MINE,                     INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_USE_PACK,                 INPUT_PS2_BTN_CIRCLE,       INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_HEALTH_KIT,               INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_TARGETING_LASER,          INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_DROP_WEAPON,		    	INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_DROP_PACK,		    	INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_DROP_BEACON,		    	INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_DROP_FLAG,		    	INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_SUICIDE,                  INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_OPTIONS,			        INPUT_PS2_BTN_START,        INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_VIEW_CHANGE,			    INPUT_UNDEFINED,            INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_ZOOM_IN,			        INPUT_PS2_BTN_L_UP,         INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_ZOOM_OUT,			        INPUT_PS2_BTN_L_DOWN,       INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_TARGET_LOCK,		        INPUT_PS2_BTN_SQUARE,       INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_COMPLIMENT,		        INPUT_PS2_BTN_L_LEFT,       INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_TAUNT,    		        INPUT_PS2_BTN_L_RIGHT,      INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_EXCHANGE, 		        INPUT_PS2_BTN_SELECT,       INPUT_UNDEFINED,        0 )

// For debugging...
DEFINE_ATTRIBUTE_GAME_KEY   ( PS2_FOCUS,				    INPUT_PS2_BTN_L_STICK,      INPUT_PS2_BTN_R_STICK,  1 )



//==============================================================================
// KEYBOARD INPUT
//==============================================================================

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_MOVE_FORWARD,             INPUT_KBD_E,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_MOVE_BACKWARD,            INPUT_KBD_D,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_MOVE_LEFT,                INPUT_KBD_S,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_MOVE_RIGHT,               INPUT_KBD_F,                INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_LOOK_LEFT,				INPUT_KBD_LEFT,             INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_LOOK_RIGHT,				INPUT_KBD_RIGHT,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_LOOK_UP,					INPUT_KBD_UP,               INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_LOOK_DOWN,				INPUT_KBD_DOWN,             INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_F32        ( KBD_LOOK_UP_DOWN_SPEED,       0.2                                                   )
DEFINE_ATTRIBUTE_F32        ( KBD_LOOK_LEFT_RIGHT_SPEED,    0.4                                                   )
DEFINE_ATTRIBUTE_F32        ( KBD_LOOK_ZOOM_SPEED_SCALAR,   0.4                                                   )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_FREE_LOOK,			    INPUT_KBD_Z,                INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_JUMP,                     INPUT_KBD_SPACE,            INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_FIRE,                     INPUT_KBD_RETURN,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_JET,                      INPUT_KBD_RSHIFT,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_ZOOM,                     INPUT_KBD_A,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_NEXT_WEAPON,              INPUT_KBD_W,                INPUT_KBD_LSHIFT,       0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_PREV_WEAPON,              INPUT_KBD_W,                INPUT_KBD_LSHIFT,       1 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_GRENADE,                  INPUT_KBD_G,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_VOICE_MENU,               INPUT_KBD_V,                INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_MINE,                     INPUT_KBD_B,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_USE_PACK,                 INPUT_KBD_R,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_HEALTH_KIT,               INPUT_KBD_Q,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_TARGETING_LASER,          INPUT_KBD_L,                INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_DROP_WEAPON,              INPUT_KBD_W,                INPUT_KBD_LCONTROL,     1 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_DROP_PACK,                INPUT_KBD_R,                INPUT_KBD_LCONTROL,     1 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_DROP_BEACON,              INPUT_KBD_H,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_DROP_FLAG,                INPUT_KBD_N,                INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_SUICIDE,                  INPUT_KBD_BACK,             INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_OPTIONS,                  INPUT_KBD_TAB,              INPUT_KBD_LSHIFT,       0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_VIEW_CHANGE,     			INPUT_KBD_P,                INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_ZOOM_IN,			        INPUT_KBD_T,				INPUT_KBD_LSHIFT,       0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_ZOOM_OUT,			        INPUT_KBD_T,				INPUT_KBD_LSHIFT,       1 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_TARGET_LOCK,		        INPUT_KBD_K,				INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_COMPLIMENT,		        INPUT_KBD_C,				INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_TAUNT,    		        INPUT_KBD_I,				INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_EXCHANGE, 		        INPUT_KBD_X,				INPUT_UNDEFINED,        0 )

// For debugging...
DEFINE_ATTRIBUTE_GAME_KEY   ( KBD_FOCUS,					INPUT_KBD_GRAVE,            INPUT_UNDEFINED,        0 )


//==============================================================================
// MOUSE INPUT
//==============================================================================

DEFINE_ATTRIBUTE_GAME_KEY   ( MSE_FIRE,                    INPUT_MOUSE_BTN_L,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( MSE_JET,                     INPUT_MOUSE_BTN_R,           INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_GAME_KEY   ( MSE_LOOK_LEFT_RIGHT,         INPUT_MOUSE_X_REL,           INPUT_UNDEFINED,        0 )
DEFINE_ATTRIBUTE_GAME_KEY   ( MSE_LOOK_UP_DOWN,            INPUT_MOUSE_Y_REL,           INPUT_UNDEFINED,        0 )

DEFINE_ATTRIBUTE_F32        ( MSE_LOOK_UP_DOWN_SPEED,      0.04                                                   )
DEFINE_ATTRIBUTE_F32        ( MSE_LOOK_LEFT_RIGHT_SPEED,   0.04                                                   )

DEFINE_ATTRIBUTE_GAME_KEY   ( MSE_NEXT_WEAPON,             INPUT_MOUSE_WHEEL_REL,       INPUT_UNDEFINED,        0 )

