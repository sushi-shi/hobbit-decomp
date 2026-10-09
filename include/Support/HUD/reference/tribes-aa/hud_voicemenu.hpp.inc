//==============================================================================
//
//  hud_VoiceMenu.hpp
//
//==============================================================================

#ifndef HUD_VOICEMENU_HPP
#define HUD_VOICEMENU_HPP

//==============================================================================
//  Structures
//==============================================================================

#define VM_GLOBAL       0x80000000
#define VM_NUM_ENTRIES  13

struct voice_cmd
{
    s32         Pos;
    const char* pLabel;
    const char* pText;
    s32         ID;
    s32         HudVoiceID ;
};

struct voice_table
{
    const char* pTitle;
    voice_cmd   Entries[VM_NUM_ENTRIES];
};

//==============================================================================
//  Functions
//==============================================================================

void    VoiceMenu_Init      ( voice_table& VoiceMenu );
s32     VoiceMenu_Navigate  ( voice_table& VoiceMenu, s32 Direction, s32& HudVoiceID );

//==============================================================================
#endif // HUD_VOICEMENU_HPP
//==============================================================================
