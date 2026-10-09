//==============================================================================
//
//  PlayerCommonInfo.hpp
//
//  Defines common player values for the struct player_common_info
//
//==============================================================================


//=========================================================================
// VIEW DEFINES
//=========================================================================

DEFINE_ATTRIBUTE_F32 ( VIEW_3RD_PERSON_HEIGHT,      1.8f    )       // Dist to put above persons feet
DEFINE_ATTRIBUTE_F32 ( VIEW_3RD_PERSON_DIST,        3.0f    )       // Dist to pull away from player
DEFINE_ATTRIBUTE_F32 ( VIEW_BACKOFF_DIST,           0.15f   )       // Dist to pull away from walls
DEFINE_ATTRIBUTE_F32 ( VIEW_BLEND_SPEED,            1.0f    )       // Blend time between views
DEFINE_ATTRIBUTE_R32 ( VIEW_MAX_PITCH,              80.0f   )       // Max looking down angle
DEFINE_ATTRIBUTE_R32 ( VIEW_MIN_PITCH,              -80.0f  )       // Min looking up angle
DEFINE_ATTRIBUTE_R32 ( VIEW_X1_FOV,                 70.0f   )       // Field of view for x1 zoom
DEFINE_ATTRIBUTE_F32 ( VIEW_SHOW_ZOOM_CHANGE_TIME,  3.0f    )       // Time to show view radicals
                                    


//=========================================================================
// MOVEMENT DEFINES
//=========================================================================

DEFINE_ATTRIBUTE_F32 ( GRAVITY,                     -26.0f  )       // Gravity in ms^2
DEFINE_ATTRIBUTE_F32 ( GROUND_FRICTION,             5.0f    )
DEFINE_ATTRIBUTE_F32 ( AIR_FRICTION,                0.400f  )

DEFINE_ATTRIBUTE_F32 ( SKI_JUMP_SPEED,              20.0f   )       // Speed for when ski-ing can happen
DEFINE_ATTRIBUTE_F32 ( SHOW_HURT_TIME,              3.0f    )       // Time health bar is displayed for if player is hurt
DEFINE_ATTRIBUTE_F32 ( MAX_HURT_SCREEN_FLASH_TIME,  2.0f    )       // Max time screen can flash red



//=========================================================================

