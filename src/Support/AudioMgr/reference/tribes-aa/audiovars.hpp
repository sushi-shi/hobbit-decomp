#ifndef __AUDIOVARS_HPP
#define __AUDIOVARS_HPP
#include "x_types.hpp"
#include "x_threads.hpp"
#include "audiodefs.hpp"
#include "audio.hpp"
//
// This contains all the variables and structures used on the EE side of the audio
// driver. There should be no equivalents on the IOP of the same fields.
// All common structs and defines should be in audiodefs.hpp
//

typedef struct s_audio_container
{
    struct s_audio_container *pNext;
    char            Filename[64];
    s32             Id;
    f32             Volume;
    s32             SampleCount;
    cfx_info        CfxInfo[1];   
} audio_container;

typedef struct s_audio_channel_list
{
    struct s_audio_channel_list     *m_pNext;
    audio_channel                   m_Channels[AUD_MAX_CHANNELS];
    s32                             m_Available;
    s32                             m_InUse;
    s32                             m_LastAllocated;
    s32                             m_BaseIndex;
} audio_channel_list;


typedef struct s_audio_vars
{
    audio_global_status     m_GlobalStatus;

    audio_channel_list*     m_pChannelList;
    audio_container*        m_pContainerList;
    xmutex*                 m_pChannelLock;
    audio_channel*          m_UpdateRequests[AUD_MAX_CHANNELS+10];
    xbool                   m_SurroundEnabled;
    s32                     m_UpdateRequestIndex;
    u32                     m_ChannelLoopId;
    u32                     m_UniqueId;
    s32                     m_ChannelsInUse;
    view*                   m_pEarView[AUD_MAX_EAR_VIEWS];
    s32                     m_EarViewCount;
    f32                     m_NearClip;
    f32                     m_FallOff;
    f32                     m_FarClip;
    xmesgq*                 m_pqReadyAudioUpdate;
    xmesgq*                 m_pqDelayedSamples;
    u32                     m_DelayedChannelId;
    f32                     m_ClipVolume;
    byte                    m_CommandBuffer[PACKAGE_OUT_BUFFER_SIZE];
    byte*                   m_pCommand;

} audio_vars;

extern audio_vars *g_Audio;


#endif // AUDIOVARS_HPP
