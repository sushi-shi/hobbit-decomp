//==============================================================================
//  
//  hud_manager.hpp
//  
//==============================================================================

#ifndef HUD_MANAGER_HPP
#define HUD_MANAGER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef X_TYPES_HPP
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#endif

#include "Objects/Player/PlayerObject.hpp"
#include "hud_voicemenu.hpp"

//==============================================================================
//  Defines
//==============================================================================

enum hud_weapons
{
    HUD_WEAPON_NONE             = player_object::INVENT_TYPE_NONE                    - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_DISK_LAUNCHER    = player_object::INVENT_TYPE_WEAPON_DISK_LAUNCHER    - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_PLASMA_GUN       = player_object::INVENT_TYPE_WEAPON_PLASMA_GUN       - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_SNIPER_RIFLE     = player_object::INVENT_TYPE_WEAPON_SNIPER_RIFLE     - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_MORTAR_GUN       = player_object::INVENT_TYPE_WEAPON_MORTAR_GUN       - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_GRENADE_LAUNCHER = player_object::INVENT_TYPE_WEAPON_GRENADE_LAUNCHER - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_CHAIN_GUN        = player_object::INVENT_TYPE_WEAPON_CHAIN_GUN        - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_BLASTER          = player_object::INVENT_TYPE_WEAPON_BLASTER          - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_ELF_GUN          = player_object::INVENT_TYPE_WEAPON_ELF_GUN          - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_MISSILE_LAUNCHER = player_object::INVENT_TYPE_WEAPON_MISSILE_LAUNCHER - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_REPAIR_GUN       = player_object::INVENT_TYPE_WEAPON_REPAIR_GUN       - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_SHOCKLANCE       = player_object::INVENT_TYPE_WEAPON_SHOCKLANCE       - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_TARGETING_LASER  = player_object::INVENT_TYPE_WEAPON_TARGETING_LASER  - player_object::INVENT_TYPE_WEAPON_START,
    HUD_WEAPON_SHRIKE           = player_object::INVENT_TYPE_WEAPON_END + 1,
    HUD_WEAPON_ASSAULT_GUN      = player_object::INVENT_TYPE_WEAPON_END + 2,
    HUD_WEAPON_ASSAULT_SHELL    = player_object::INVENT_TYPE_WEAPON_END + 3,

    HUD_WEAPON_END
};

#define HUD_NUM_WEAPONS (player_object::INVENT_TYPE_WEAPON_END + 3 - player_object::INVENT_TYPE_WEAPON_START + 1)

enum hud_bitmaps
{
    HUD_BMP_START               = 0,

    HUD_BMP_HEALTH_METER        = HUD_BMP_START,
    HUD_BMP_1P_STATBAR,
    HUD_BMP_1P_SCOREBOARD,
    HUD_BMP_1P_SCOREBOARD_B,
    HUD_BMP_1P_SCOREBOARD_C,
    HUD_BMP_2P_SCOREBOARD00,
    HUD_BMP_2P_SCOREBOARD01,
    HUD_BMP_2P_SCOREBOARD02,
    HUD_BMP_2P_SCOREBOARD03,
    HUD_BMP_2P_SCOREBOARD04,
    HUD_BMP_2P_STATBAR,
    
    HUD_BMP_HEALTH_METER_G,
    HUD_BMP_1P_SCOREBOARD_G,
    HUD_BMP_1P_STATBAR_G,
    HUD_BMP_RETICLE_G,
    
    HUD_BMP_LED_GLOW,
    HUD_BMP_BEACON,
    HUD_BMP_GRENADE,
    HUD_BMP_GRENADE_FRAG,
    HUD_BMP_GRENADE_FLARE,
    HUD_BMP_GRENADE_CONCUSS,
    HUD_BMP_MINE,
    HUD_BMP_HEALTHKIT,
    HUD_BMP_PACK_AMMO,
    HUD_BMP_PACK_ENERGY,
    HUD_BMP_PACK_INVENTORY,
    HUD_BMP_PACK_REPAIR,
    HUD_BMP_PACK_SATCHEL,
    HUD_BMP_PACK_SENSOR,
    HUD_BMP_PACK_SHIELD,
    HUD_BMP_PACK_TURRET,
    HUD_BMP_PACK_BARREL,
    
    HUD_BMP_VOTE_PIPE,
    HUD_BMP_VOTE_SLAT_TOP,
    HUD_BMP_VOTE_SLAT_BOT,
    HUD_BMP_VOTE_TRACK,
    HUD_BMP_CHAT_GLASS,

    HUD_BMP_VEH_HEALTH1,
    HUD_BMP_VEH_HEALTH2,
    HUD_BMP_VEH_STATBAR,

    HUD_BMP_VI_ASSAULT,
    HUD_BMP_VI_BOMBER,
    HUD_BMP_VI_HAPC,
    HUD_BMP_VI_MPB,
    HUD_BMP_VI_SHRIKE,
    HUD_BMP_VI_WILDCAT,
    HUD_BMP_V_SEAT,

    HUD_BMP_FLAG,

    HUD_BMP_END
};

#define HUD_NUM_BITMAPS         (HUD_BMP_END - HUD_BMP_START)

#define NUM_OBJECTIVE_LINES     3

enum vote_states
{
    VOTE_DONE,
    VOTE_OPEN_FULL,
    VOTE_OPENING_VOTEBOX,
    VOTE_OPENING_SLATS,
    VOTE_OPENING,
    VOTE_CLOSING,
    VOTE_CLOSING_VOTEBOX,
    VOTE_CLOSING_SLATS,
    VOTE_CLOSED,
    VOTE_PAUSE,
    VOTE_VOTING,
};

enum lock_state
{
    MISSILE_NO_LOCK,
    MISSILE_TRACKING,
    MISSILE_LOCKED,
};

enum hud_glow
{
    HUDGLOW_NONE,
    HUDGLOW_HEALTHMETER,
    HUDGLOW_SCOREBOARD,
    HUDGLOW_STATBAR,
    HUDGLOW_RETICLE,
    HUDGLOW_TEXT1,
    HUDGLOW_TEXT2,
    HUDGLOW_TEXT3,
};

enum inven_type
{
    INVEN_NORMAL,       // icon with default colour and no count
    INVEN_COUNT_LEFT,   // icon has a count to the left of it
    INVEN_COUNT_BELOW,  // icon has a count below it
    INVEN_PULSE1,       // fade the colour in and out  (no count)
    INVEN_PULSE2,       // blend the colour to another (no count)
};

const s32 NumChatLines       =  3;
const s32 NumChatLines2P     =  2;
const s32 MaxInventoryItems  =  5;

//==============================================================================
//  Types
//==============================================================================

//==============================================================================
//  Logging
//==============================================================================

//==============================================================================
//  Chat Defines
//==============================================================================

enum chat_area
{
};

extern s32 CHAT_AREA_CHATTER_Y;
extern s32 CHAT_AREA_PLAYER_Y;
extern s32 CHAT_AREA_POPUP_Y;

//==============================================================================
//  hud_manager
//==============================================================================

class ui_manager;
class player_object;

class hud_manager
{
public:
    //==============================================================================
    //  User Data
    //==============================================================================

    struct chat_line
    {
    };

    struct inventory
    {
    };

    struct user
    {
    };
    
//==============================================================================
//  Functions
//==============================================================================

public:
                    hud_manager         ( void );
                   ~hud_manager         ( void );

    void            Init                ( ui_manager* pUIManager );
    void            Kill                ( void );

    s32             CreateUser          ( player_object* pPlayer, s32 UIUserID );
    void            DeleteUser          ( s32 UserID );
    void            DeleteAllUsers      ( void );
    user*           GetUser             ( s32 UserID ) const;
    player_object*  GetPlayer           ( s32 UserID ) const;

    void            Update              ( f32 DeltaTime );
    xbool           ProcessInput        ( s32 UserID, s32 ControllerID, f32 DeltaTime );

    void            Render              ( void ) const;
    
    void            ActivateVoiceMenu   ( s32 UserID );
    void            DeactivateVoiceMenu ( s32 UserID );
    xbool           IsVoiceMenuActive   ( s32 UserID ) const;
/*
    // Objectives Window for single player
    void            SetObjective        ( const xwchar* pString );
    const xwchar*   GetObjective        ( void );
*/
    // Messages: Chat, errors, pop-ups, exchange.
    void            ClearChat           ( user* pUser );
    void            AddChat             ( s32 PlayerIndex, const xwchar* pString );
    void            Popup               ( s32 Region, s32 PlayerIndex, const xwchar* pString );
    void            ClearPopup          ( s32 Region, s32 PlayerIndex );

    void            Initialize          ( void );
    void            StartVote           ( const xwchar* pString );
    void            StopVote            ( void );
    void            SetVote             ( f32 Vote1, f32 Vote2 );
    void            SetForAgainst       ( s32 For, s32 Against );
    void            SetVotesNeeded      ( f32 Needed );
    xbool           IsReadyToVote       ( void );
    
    // HUD Training glows
    void            SetHUDGlow          ( hud_glow Item );
    void            SetMissileLock      ( s32 PlayerIndex, lock_state LockState );
    user*           GetUserFromPlayerID ( s32 PlayerIndex ) const;

    void            AddInventory        ( user* pUser, s32 Icon, s32 Count, inven_type Type );

protected:
    
};

//==============================================================================
#endif // HUD_MANAGER_HPP
//==============================================================================
