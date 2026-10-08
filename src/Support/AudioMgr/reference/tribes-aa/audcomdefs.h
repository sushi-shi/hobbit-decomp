#ifndef __AUDCOMDEFS_H
#define __AUDCOMDEFS_H

//
// This file contains all the structures and definitions common to the iop and
// EE side of the audio driver

typedef struct s_channel_info
{
    s32 ChannelId;
    s32 SampleId;
    s16 Flags;                  // Flags such as LOOPED,PERSISTENT etc
    s16 Status;                 // Status info such as playing, delayed etc
    s16 LeftVolume;
    s16 RightVolume;
    s16 RearVolume;
    s16 Pitch;                  // 0..32767 - AUD_BASE_PITCH is the original
    s8  FadeTarget;             // 0..100
    s8  FadeRate;               // 0..100
} channel_info;


#endif // __AUDCOMDEFS_H
