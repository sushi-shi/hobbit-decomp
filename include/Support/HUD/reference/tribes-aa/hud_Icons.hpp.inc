//==============================================================================
//
//  hud_Icons.hpp
//
//==============================================================================

#ifndef HUD_ICONS_HPP
#define HUD_ICONS_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>

//==============================================================================
//  DEFINES
//==============================================================================

#define MAX_ENTRIES 64

//==============================================================================
//  TYPES
//==============================================================================

class hud_icons
{

//------------------------------------------------------------------------------
//  Public Types

public:

    enum icon
    {
        FLAG,
        FLAG_HALO,

        WAYPOINT,
        SWITCHPOINT,
        PLAYER_INDICATOR,
/*
        BEACON_TARGET_BASE,
        BEACON_TARGET_AIM,
        BEACON_TARGET_LINE,
*/
        BEACON_MARK,

        MISSILE_TRACK,
        MISSILE_LOCK,
        
//      TARGET_LOCK?
        AUTOAIM_TARGET,
    };

    enum color
    {
        GREEN,
        RED,
        WHITE,
        YELLOW,
        HARD_CODED,
        USE_BITS,
    };

//------------------------------------------------------------------------------
//  Public Functions

public:

            hud_icons       ( void );
           ~hud_icons       ( void );

    void    Init            ( void );
    void    Kill            ( void );

    void    Add             ( icon Icon, const vector3& Position, u32 TeamBits );
    void    Add             ( icon Icon, const vector3& Position, color ColorIndex );
    void    Add             ( icon Icon, const vector3& Position, color ColorIndex, s32 Alpha );
    void    Add             ( icon Icon, const vector3& Position );
    void    Add             ( icon Icon );
    void    Render          ( void );
    void    Clear           ( void );

};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern hud_icons HudIcons;

//==============================================================================
#endif // HUD_ICONS_HPP
//==============================================================================
