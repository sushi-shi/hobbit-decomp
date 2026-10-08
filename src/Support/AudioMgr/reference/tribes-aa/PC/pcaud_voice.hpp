#ifndef __IOPVOICE_H
#define __IOPVOICE_H

#include "x_types.hpp"
#include "pcaud_cfx.hpp"
#include "pcaud_spu.hpp"
#include "pcaud_element.hpp"
#include "pcaud_stream.hpp"
#include "Miles.h"

enum
{
    VSTAT_FREE = 0,
    VSTAT_IDLE,
    VSTAT_STARTING,
    VSTAT_PLAYING,
    VSTAT_STOPPING,
    VSTAT_PAUSED,
};

#define VOICE_ALIGNMENT (16)

struct s_cfx_element;
struct s_voice_stream;

typedef struct s_audio_voice
{
    s16                     m_Id;                   // Current voice number 0..23
    s16                     m_Type;                 // Element, stream or hybrid.
    s16                     m_Status;
    s16                     m_ExpireDelay;
    s16                     m_Age;
    s32                     m_SpuCurrent;           // Current SPU address
    void*                   m_pSysVoice;            // System allocated voice control block
    struct s_cfx_element*   m_pOwner;
    struct s_audio_voice*   m_pSibling;            // Surround voice
    spu_state               m_Hardware;             // Current status of the hardware for this channel
    s16                     m_UniqueSampleId;       // Used by Miles, every sample has a unique Id.
} audio_voice;

extern Audio g_Miles;

void        voice_Init  (void);
audio_voice*voice_Alloc (cfx_element* pElement);
void        voice_Free  (audio_voice* pVoice);
s32         voice_Cull  (s32 NumberToCull);
xbool       voice_Update(audio_voice* pVoice,cfx_state* pState,f32 DeltaTime);

#endif

