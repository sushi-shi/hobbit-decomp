// Imported engine source reference: Area51 pristine431f72b9, Support/Animation/BasePlayer.cpp
// Provisional Hobbit adaptation unless an explicit PC RVA is recorded. See IMPORT-NOTES.md.
//=========================================================================
//
//  BasePlayer.CPP
//
//=========================================================================

//=========================================================================
// INCLUDES
//=========================================================================
#include <Support/Animation/BasePlayer.hpp>

#ifdef TARGET_PS2
#include <xCore/Entropy/PS2/ps2_spad.hpp>
#endif

//=========================================================================
// FUNCTIONS
//=========================================================================

anim_key* base_player::GetMixBuffer(mix_buffer MixBuffer) {
#ifdef TARGET_PS2
    // Use scratch pad
    ASSERT((MAX_ANIM_BONES * MIX_BUFFER_COUNT * sizeof(anim_key)) <= (u32)SPAD.GetUsableSize());
    anim_key* pStart = (anim_key*)SPAD.GetUsableStartAddr();
    return &pStart[MAX_ANIM_BONES * MixBuffer];
#else
    // Use main memory
    static anim_key s_MixBuffer[MAX_ANIM_BONES * MIX_BUFFER_COUNT];
    return &s_MixBuffer[MAX_ANIM_BONES * MixBuffer];
#endif
}

//=========================================================================
