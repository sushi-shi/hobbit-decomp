#ifndef __audio_HPP
#define __audio_HPP

#include "x_types.hpp"
#include "e_engine.hpp"
#include "audiodefs.hpp"

//
// We will never have a handle, or sound effect, index with id 0. If we have this
// passed in, we just ignore it and return immediately.
//
#define SOUNDTYPE_NONE          (0)

//
// This struct contains all data that only exists on the EE side with reference to this
// channel.
//
typedef struct s_audio_channel
{
    cfx_state       m_State;
    cfx_info        *m_pInfo;
    s16             m_Flags;
    audstat         m_Status;
    u32             m_AttachedId;
    f32             m_Volume;
    f32             m_Pitch;
    f32             m_Pan;
    f32             m_RearPan;
    vector3         m_Position;
} audio_channel;


//
// Global audio functions
//
void    audio_Init(void);
void    audio_Kill(void);
s32     audio_LoadContainer(char *pFilename);
void    audio_UnloadContainer(s32 ContainerId);
void    audio_SetContainerVolume(s32 ContainerId,f32 Volume);
f32     audio_GetContainerVolume(s32 ContainerId);
void    audio_Update(void);
void    audio_SetMasterVolume(f32 left,f32 right);
void    audio_SetMasterVolume(f32 vol);
void    audio_SetSurround(xbool mode);
void    audio_LockChannelList(void);
void    audio_UnlockChannelList(void);
void    audio_SetEarView(view *pView);
void    audio_SetEarView(view *pView,f32 nearclip,f32 falloff,f32 farclip,f32 clipvol);
void    audio_AddEarView(view *pView);
void    audio_SetClip(f32 nearclip,f32 falloff,f32 farclip,f32 clipvol);
void    audio_QueueUpdateRequest(audio_channel *pChannel);

//
// These are "internal" functions just for the movie streamer. They may be
// modified in the future for streaming from EE memory.
void    audio_InitStream(f32 pitch);
void    audio_SendStream(void *pData,s32 length,s32 Index);
xbool   audio_WaitStream(s32 Index);
void    audio_KillStream(void);

//
// Channel based functions
//

u32     audio_Allocate      (void);                   // Allocates a channel. Doesn't start it but makes it persistent
void    audio_Free          (u32 ChannelId);              // Free's a persistent channel

s32     audio_Play          (u32 SampleId);
s32		audio_Play			(u32 SampleId, s32 Flags);
s32     audio_Play          (u32 SampleId, vector3 *pPosition);
s32     audio_Play          (u32 SampleId, vector3 *pPosition,s32 Flags);
void    audio_StopAll       (void);
void    audio_Stop          (u32 ChannelId);
void    audio_Pause         (u32 ChannelId);
void    audio_SetVolume     (u32 ChannelId,f32 Volume);
void    audio_SetPan        (u32 ChannelId,f32 Pan);
void    audio_SetRearPan    (u32 ChannelId,f32 Pan);
void    audio_SetPitch      (u32 ChannelId,f32 Pitch);
void    audio_SetPosition   (u32 ChannelId,vector3 *pPosition);
void    audio_SetFlags      (u32 ChannelId,s32 Flags);
xbool   audio_IsPlaying     (u32 ChannelId);

s32     audio_GetStatus     (u32 ChannelId);
f32     audio_GetVolume     (u32 ChannelId);
f32     audio_GetPan        (u32 ChannelId);
f32     audio_GetRearPan    (u32 ChannelId);
f32     audio_GetPitch      (u32 ChannelId);
s32     audio_GetChannelsInUse(void);
void    audio_SetLooping    (u32 ChannelId);

// Looping sound util functions - they take care of updating the ChannelId, and sound position
// NOTE: A channel id of SOUNDTYPE_NONE (0), means the sound is not playing.
void    audio_InitLooping ( s32& ChannelId ) ;
void    audio_PlayLooping ( s32& ChannelId, u32 SampleId ) ;
void    audio_PlayLooping ( s32& ChannelId, u32 SampleId, vector3* pPos ) ;
void    audio_StopLooping ( s32& ChannelId ) ;






//
// This is actually an internal function. You should NOT
// access this independantly of the audio driver.
//
void        audio_UpdateParams(audio_channel *pChannel,f32 Modifier);
cfx_info    *audio_FindInfo(s32 SampleId);
//

#endif // __AUDIO_HPP



    