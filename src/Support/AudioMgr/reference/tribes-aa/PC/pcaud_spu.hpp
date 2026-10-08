#ifndef __IOPSPU_H
#define __IOPSPU_H

#include "pcaud_host.hpp"

#define SPU_MEM_SIZE 0x200000
#define SPU_MEM_BASE 0x006000
#define SPU_BASE_SAMPLE_RATE 48000

typedef struct s_spu_state
{
    s16 LeftVolume;
    s16 RightVolume;
    s16 Pitch;
    s32 Envelope;
    s32 Current;
    s32 Finished;
} spu_state;


typedef struct s_spu_master_state
{
    audio_global_status     m_State;
    u32                     m_Keyon0;
    u32                     m_Keyon1;
    u32                     m_Keyoff0;
    u32                     m_Keyoff1;
} spu_master_state;

void    spu_Init(void);

s32     spu_Alloc       (s32 size,char* pLabel);
s32     spu_Free        (s32 Base);
s32     spu_MemFree     (void);
s32     spu_Memtop      (void);

#define SPUTRANS_WRITE  (0)
#define SPUTRANS_READ   (1)


struct s_audio_voice;

void    spu_Transfer    (void* pBase,s32 Dest,s32 length,s32 direction);

void    spu_InitVoice   (struct s_audio_voice* pVoice);
void    spu_ReadVoice   (struct s_audio_voice* pVoice,spu_state* pState);
void    spu_WriteVoice  (struct s_audio_voice* pVoice,spu_state* pState);

void    spu_ReadMaster  (spu_master_state* pState);
void    spu_WriteMaster (spu_master_state* pState);
void    spu_SetKey      (struct s_audio_voice* pVoice,s32 on);

#endif // __IOPSPU_H