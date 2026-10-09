//=========================================================================
//
//  hud_manager.cpp
//
//=========================================================================

/*
#include <xCore/Entropy/Entropy.hpp>
#include "AADS/Globals.hpp"

#include "GameMgr/GameMgr.hpp"
#include <Support/ObjectMgr/ObjectMgr.hpp>

#include <Support/UI/ui_manager.hpp>
#include <Support/UI/ui_font.hpp>
#include "demo1\data\ui\ui_strings.h"
#include "demo1\data\missions\campaign.h"

#include <Support/StringMgr/StringMgr.hpp>

#include "objects/vehicles/vehicle.hpp"
#include "objects/vehicles/bomber.hpp"
#include "objects/projectiles/satchelcharge.hpp"

#include "../../AADS/fe_colors.hpp"
*/

#include "hud_manager.hpp"

//=========================================================================
//  Defines
//=========================================================================

// Health Energy Display
/*
const s32 HealthEnergyWidth     = 128;      // Real Width:   91
const s32 HealthEnergyHeight    =  64;      // Real Height:  34
const s32 HealthMeterOffsetX    =   0;
const s32 HealthMeterOffsetY    =  18;

const s32 HealthBarOffsetX      =  17;
const s32 HealthBarOffsetY      =   5;
const s32 HealthBarWidth        =  60;
const s32 HealthBarHeight       =   6;

const s32 EnergyBarOffsetX      =  17;
const s32 EnergyBarOffsetY      =  16;
const s32 EnergyBarWidth        =  60;
const s32 EnergyBarHeight       =   6;

const s32 HeatBarOffsetX        =  17;
const s32 HeatBarOffsetY        =  24;
const s32 HeatBarWidth          =  60;
const s32 HeatBarHeight         =   4;

// 1 Player Score Board

const s32 ScoreBoardWidth       =  86;
const s32 ScoreBoardHeight      = 116;
const s32 ScoreBoardTextWidth   =  54;
const s32 ScoreBoardTextHeight  =  11;
const s32 ScoreBoardOffsetY     =   8;
const s32 ScoreLabelOffsetX     =  18;
const s32 ScoreLabelOffsetY     =   6;
const s32 ScoreValueOffsetY     =  23;
const s32 FlagsLabelOffsetY     =  40;
const s32 FlagsValueOffsetY     =  57;
const s32 TimerValueOffsetX     =  10;      // Position of clock relative to top left corner of scoreboard
const s32 TimerValueOffsetY     =  80;
const s32 TimerValueOffsetCX    =  26;      // Position of clock relative to top left corner of clock piece
const s32 TimerValueOffsetCY    =  13;

// 2 Player Score Board

const s32 ScoreBoard00Width     =  79;
const s32 ScoreBoard02Width     =  93;
const s32 ScoreBoard00Height    =  47;
const s32 ScoreBoard01Height    =  33;
const s32 ScoreBoard02Height    =  26;
const s32 ScoreBoard03Height    =  33;
const s32 ScoreBoard04Height    =  50;

const s32 ScoreLabel1OffsetX    =  13;      // Player 1 score positions
const s32 ScoreLabel1OffsetY    =  13;
const s32 ScoreValue1OffsetX    =   5;
const s32 ScoreValue1OffsetY    =  29;
const s32 FlagsLabel1OffsetX    =  13;
const s32 FlagsLabel1OffsetY    =   0;
const s32 FlagsValue1OffsetX    =   5;
const s32 FlagsValue1OffsetY    =  15;

const s32 ScoreLabel2OffsetX    =  13;      // Player 2 score positions
const s32 ScoreLabel2OffsetY    =  -1;
const s32 ScoreValue2OffsetX    =   5;
const s32 ScoreValue2OffsetY    =  15;
const s32 FlagsLabel2OffsetX    =  13;
const s32 FlagsLabel2OffsetY    =  -1;
const s32 FlagsValue2OffsetX    =   5;
const s32 FlagsValue2OffsetY    =  15;

const s32 TimerValue1OffsetX    =  34;      // Position of clock relative to top left corner of middle piece
const s32 TimerValue1OffsetY    =   5;

const s32 LedGlowWidth          =  68;      // Scale glow up a bit (an extra 4 pixels)
const s32 LedGlowHeight         =  64;
const s32 LedGlowRealHeight     =  50;

// Inventory

const s32 StatBarOffsetX        =  19;      // Position of statbar relative to top left corner of scoreboard
const s32 StatBarOffsetY        = 106;
const s32 StatBarOffsetCX       =  19;      // Position of statbar relative to top left corner of clock piece
const s32 StatBarOffsetCY       =  39;

const s32 StatBar1OffsetX       =  12;      // Position of statbar relative to top left corner of scoreboard00 piece
const s32 StatBar1OffsetY       =  10;
const s32 StatBar2OffsetX       =  12;      // Position of statbar relative to top left corner of scoreboard04 piece
const s32 StatBar2OffsetY       =  39;
const s32 StatBarWidth          = 128;
const s32 StatBarHeight         = 256;
const s32 StatBarRealHeight     = 180;
const s32 StatBarMaxIcons       =   5;

const s32 InventoryOffsetX      =  11;      // Position of icon rect relative to top left corner of statbar
const s32 InventoryOffsetY      =  24;
const s32 Inventory1OffsetY     =-128;      // Offset to top icon for player 1 in split screen
const s32 Inventory2OffsetY     =  29;      // Offset to top icon for player 2 in split screen
const s32 InventoryIconShiftX   =  10;      // Amount to move icons relative to text value
const s32 InventoryIconShiftY   =   8;
const s32 InventoryIconAddY     =  31;      // Number of lines separating the top of icons
      
const s32 FlagIconTextX         =  16;      // Offset to position text under icon
const s32 FlagIconTextY         =  29;

// Voting

const s32 VotePipeWidth         =  256;
const s32 VotePipeHeight        =   16;
const s32 VotePipeOffsetY       =  -32;     // Offset from bottom of screen (1 player)
const s32 VotePipeLStop         =   32;     // Minimum width of left pipe   (2 player)
const s32 VotePipeRStop         =  112;     // Minimum width of right pipe  (2 player)
const f32 VotePipeAccel         = 8192;
const s32 VoteFadeSpeed         =    6;
const s32 VoteTrackRealWidth    =   16;
const s32 VoteTrackWidth        =    3;
const s32 VoteSlatHeight        =    4;
const s32 VoteSlatRealHeight    =   16;
const f32 VoteSlatTime          =    0.1f;
const f32 VoteBoxHeight         =   16;
const f32 VotePauseTime         =    0.1f;
const s32 VotesNeededBarWidth   =    1;
const s32 VoteCountOffsetX      =   10;

// Vehicle

const s32 VehiclePanelOffsetX   =    0;
const s32 VehiclePanelOffsetY   =   15;
const s32 VehiclePanelAddY      =    8;
const s32 VehicleGlass1OffsetX  =    2;
const s32 VehicleGlass1OffsetY  =   32;
const s32 VehicleGlass2OffsetX  =    2;
const s32 VehicleGlass2OffsetY  =   42;
const s32 VehicleIconOffsetX    =   14;
const s32 VehicleIconOffsetY    =    0;

const s32 VehicleHealthOffsetX  =   18;
const s32 VehicleHealthOffsetY  =    8;
const s32 VehicleEnergyOffsetX  =   18;
const s32 VehicleEnergyOffsetY  =   18;
const s32 VehicleWeaponOffsetX  =   18;
const s32 VehicleWeaponOffsetY  =   28;
const s32 VehicleBarWidth       =   58;
const s32 VehicleBarHeight      =    5;

const s32 VehiclePanelWidth     =  128;
const f32 VehiclePanelTime      =    0.75f;

// HUD Training Glows

const s32 HealthMeterGlowX      =  -17;
const s32 HealthMeterGlowY      =  -16;
const s32 ScoreBoardGlowX       =  -42;
const s32 ScoreBoardGlowY       =   -1;
const s32 StatBarGlowX          =  -28;
const s32 StatBarGlowY          =  -36;
const s32 ReticleGlowX          =  -64;
const s32 ReticleGlowY          =  -64;
*/
//=========================================================================
//  Structs
//=========================================================================

//=========================================================================
//  Data
//=========================================================================
/*
volatile s32 OVERLAY_ALPHA = 160;

u32 s_ReticleColors[] =
{
    ARGB(224,161,211,245),       // HUD_WEAPON_NONE
    ARGB(224,161,211,245),       // HUD_WEAPON_DISK_LAUNCHER
    ARGB(224,161,211,245),       // HUD_WEAPON_PLASMA_GUN
    ARGB(224,255,  0,  0),       // HUD_WEAPON_SNIPER_RIFLE
    ARGB(224,161,211,245),       // HUD_WEAPON_MORTAR_GUN
    ARGB(224,161,211,245),       // HUD_WEAPON_GRENADE_LAUNCHER
    ARGB(224,161,211,245),       // HUD_WEAPON_CHAIN_GUN
    ARGB(224,161,211,245),       // HUD_WEAPON_BLASTER
    ARGB(224,161,211,245),       // HUD_WEAPON_ELF_GUN
    ARGB(224,161,211,245),       // HUD_WEAPON_MISSILE_LAUNCHER
    ARGB(224,161,211,245),       // HUD_WEAPON_REPAIR_GUN
    ARGB(224,161,211,245),       // HUD_WEAPON_SHOCKLANCE
    ARGB(224,  0,255,  0),       // HUD_WEAPON_TARGETING_LASER
    ARGB(224,238,156,  0),       // HUD_WEAPON_SHRIKE
    ARGB(224,238,156,  0),       // HUD_WEAPON_ASSAULT_GUN
    ARGB(224,238,156,  0),       // HUD_WEAPON_ASSAULT_SHELL
};

xbool   HUD_DebugInfo   = FALSE;
xbool   HUD_HideHUD     = FALSE;
xcolor  HUD_DebugColor(255,255,255,255);

xcolor  HUD_HealthCol    (  42, 178,  46, 255 );
xcolor  HUD_EnergyCol    (  55,  57, 178, 255 );
xcolor  HUD_HeatCol      ( 149,  47,  47, 255 );
xcolor  HUD_StatBarCol   ( 255, 255, 255,  70 );
xcolor  HUD_ScoreLabelCol( 128, 192, 255, 192 );
xcolor  HUD_ScoreValueCol(   0,   0,   0, 255 );
xcolor  HUD_TimerValueCol( 255, 255, 255, 192 );
xcolor  HUD_NormalGlowCol( 160, 192, 255, 255 );
xcolor  HUD_TargetGlowCol( 255, 255,  80, 255 );
xcolor  HUD_LockedGlowCol( 255,  80,  80, 255 );
xcolor  HUD_IconGlowCol  ( 180, 220, 255, 255 );
xcolor  HUD_ZeroGlowCol  ( 255, 100, 100, 255 );
xcolor  HUD_VoteTextCol  ( 255, 255, 255, 224 );
xcolor  HUD_VoteBoxCol   (   0,   0,   0, 255 );
xcolor  HUD_VotePosCol   (  63, 127,  63, 255 );
xcolor  HUD_VoteNegCol   ( 127,  31,  31, 255 );
xcolor  HUD_PulseCol     ( 150, 255, 200, 255 );
xcolor  HUD_FlagTextCol  ( 100, 255, 255, 255 );
xcolor  HUD_ShieldedCol  ( 100, 150, 200, 255 );

static s32 s_VoteStates[2][11]=
{
    {
        VOTE_OPEN_FULL,
        VOTE_CLOSING,
        VOTE_PAUSE,
        VOTE_OPENING_VOTEBOX,
        VOTE_OPENING_SLATS,
        VOTE_VOTING,
        VOTE_CLOSING_SLATS,
        VOTE_CLOSING_VOTEBOX,
        VOTE_PAUSE,
        VOTE_OPENING,
        VOTE_DONE,
    },
    {
        VOTE_CLOSED,
        VOTE_OPENING_VOTEBOX,
        VOTE_OPENING_SLATS,
        VOTE_VOTING,
        VOTE_CLOSING_SLATS,
        VOTE_CLOSING_VOTEBOX,
        VOTE_DONE,
    }
};

xbool TestVoting = FALSE;
xbool TestChat   = FALSE;
xbool DrawGuard  = FALSE;
*/
//=========================================================================
//  Helpers
//=========================================================================

//=========================================================================
//  hud_manager
//=========================================================================

hud_manager::hud_manager( void )
{
}

//=========================================================================

hud_manager::~hud_manager( void )
{
}

//=========================================================================

void hud_manager::Init( ui_manager* pUIManager )
{
}

//=========================================================================

void hud_manager::Kill( void )
{
}

//=========================================================================

void hud_manager::Initialize( void )
{
}

//=========================================================================

s32 hud_manager::CreateUser( player_object* pPlayer, s32 UIUserID )
{
    return( 0 );
}

//=========================================================================

void hud_manager::DeleteAllUsers( void )
{
}

//=========================================================================

void hud_manager::DeleteUser( s32 UserID )
{
}

//=========================================================================

hud_manager::user* hud_manager::GetUser( s32 UserID ) const
{
    return( NULL );
}

//=========================================================================

player_object* hud_manager::GetPlayer( s32 UserID ) const
{
    return( NULL );
}

//=========================================================================

void hud_manager::Update( f32 DeltaTime )
{
}

//=========================================================================

xbool hud_manager::ProcessInput( s32 UserID, s32 ControllerID, f32 DeltaTime )
{
    return( FALSE );
}

//=========================================================================

void hud_manager::Render( void ) const
{
}

//=========================================================================

void hud_manager::ActivateVoiceMenu( s32 UserID )
{
}

//=========================================================================

void hud_manager::DeactivateVoiceMenu( s32 UserID )
{
}

//=========================================================================

xbool hud_manager::IsVoiceMenuActive( s32 UserID ) const
{
    return( FALSE );
}

//=========================================================================
/*
void hud_manager::SetObjective( const xwchar* pString )
{
    m_pObjective = pString;
}

//=========================================================================

const xwchar* hud_manager::GetObjective( void )
{
    return( m_pObjective );
}

//=========================================================================

void hud_manager::UpdateVote( f32 DeltaTime )
{
}

//=========================================================================

void hud_manager::NextState( void )
{
}

//=========================================================================

s32 hud_manager::GetCurrentState( void ) const
{
    return( 0 );
}
*/
//=========================================================================

void hud_manager::StartVote( const xwchar* pString )
{
}

//=========================================================================

void hud_manager::StopVote( void )
{
}

//=========================================================================

void hud_manager::SetVote( f32 Vote1, f32 Vote2 )
{
}

//=========================================================================

void hud_manager::SetVotesNeeded( f32 Needed )
{
}

//=========================================================================

void hud_manager::SetForAgainst( s32 For, s32 Against )
{
}

//=========================================================================

xbool hud_manager::IsReadyToVote( void )
{
    return( FALSE );
}

//=========================================================================

void hud_manager::SetHUDGlow( hud_glow Item )
{
}

//=========================================================================

void hud_manager::SetMissileLock( s32 PlayerIndex, lock_state LockState )
{
}

//=========================================================================

hud_manager::user* hud_manager::GetUserFromPlayerID( s32 PlayerIndex ) const
{
    return( NULL );
}

//=========================================================================

