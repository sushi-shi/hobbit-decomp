#include "audio.hpp"
#include "audiodefs.hpp"
#include "audiovars.hpp"




void ps2audio_Init(void);
void ps2audio_Kill(void);
void pcaudio_Init(void);
void pcaudio_Kill(void);

void audio_Init(void)
{
    ASSERT(!g_Audio);
    g_Audio = (audio_vars *)x_malloc(sizeof(audio_vars));
    g_Audio->m_ChannelLoopId = 1;

    g_Audio->m_pChannelLock = new xmutex();
    g_Audio->m_UpdateRequestIndex=0;
    g_Audio->m_GlobalStatus.m_ChannelCount = AUD_MAX_CHANNELS;
    audio_SetClip(AUD_DEFAULT_NEARCLIP,AUD_DEFAULT_FALLOFF,AUD_DEFAULT_FARCLIP,AUD_DEFAULT_CLIPVOLUME);
    g_Audio->m_EarViewCount     = 0;
    g_Audio->m_pEarView[0]      = NULL;
    g_Audio->m_pEarView[1]      = NULL;
    g_Audio->m_pChannelList     = NULL;
    g_Audio->m_pContainerList   = NULL;
    g_Audio->m_SurroundEnabled  = FALSE;
    g_Audio->m_UniqueId         = 0;

    audio_SetMasterVolume( 1.0f, 1.0f );

    g_Audio->m_pqDelayedSamples = new xmesgq(8);
    g_Audio->m_pqReadyAudioUpdate = new xmesgq(1);

#ifdef TARGET_PS2
    ps2audio_Init();
#endif

#ifdef TARGET_PC
    pcaudio_Init();
#endif

    g_Audio->m_pqReadyAudioUpdate->Send(NULL,MQ_BLOCK);
}

void audio_Kill(void)
{
    audio_channel_list  *pChannelList,*pLast;
    
    if (!g_Audio)
        return;
    audio_LockChannelList();
    pChannelList = g_Audio->m_pChannelList;
    while (pChannelList)
    {
        pLast = pChannelList;
        pChannelList = pChannelList->m_pNext;
        x_free(pLast);
    }
    delete g_Audio->m_pqReadyAudioUpdate;
    delete g_Audio->m_pChannelLock;
    delete g_Audio->m_pqDelayedSamples;

#ifdef TARGET_PS2
    ps2audio_Kill();
#endif

#ifdef TARGET_PC
    pcaudio_Kill();
#endif

    x_free(g_Audio);
    g_Audio = NULL;
}

void audio_QueueUpdateRequest(audio_channel *pChannel)
{
    if (g_Audio->m_UpdateRequestIndex >= (AUD_MAX_CHANNELS+10))
        return;
    g_Audio->m_UpdateRequests[g_Audio->m_UpdateRequestIndex] = pChannel;
    g_Audio->m_UpdateRequestIndex++;
}

void audio_LockChannelList(void)
{
    g_Audio->m_pChannelLock->Enter();
}

void audio_UnlockChannelList(void)
{
    g_Audio->m_pChannelLock->Exit();
}


void    audio_SetMasterVolume(f32 left,f32 right)
{
    g_Audio->m_GlobalStatus.m_LeftVolume = (s16)(left * AUD_MAX_VOLUME);
    g_Audio->m_GlobalStatus.m_RightVolume = (s16)(right * AUD_MAX_VOLUME);
}

void    audio_SetSurround(xbool mode)
{
    g_Audio->m_SurroundEnabled = mode;
}

void    audio_SetMasterVolume(f32 vol)
{
    audio_SetMasterVolume(vol,vol);
}

void    audio_SetEarView(view *pView)
{
    g_Audio->m_pEarView[0] = pView;
    g_Audio->m_EarViewCount=1;
}

void    audio_AddEarView(view *pView)
{
    s32 i;

    for (i=0;i<g_Audio->m_EarViewCount;i++)
    {
        ASSERT(g_Audio->m_pEarView[i] != pView);
    }
    g_Audio->m_pEarView[g_Audio->m_EarViewCount] = pView;
    g_Audio->m_EarViewCount++;
    ASSERT(g_Audio->m_EarViewCount <= AUD_MAX_EAR_VIEWS);
}

void    audio_SetEarView(view *pView,f32 nearclip,f32 falloff,f32 farclip,f32 clipvol)
{
    audio_SetEarView(pView);
    audio_SetClip(nearclip,falloff,farclip,clipvol);
}

void    audio_SetClip(f32 nearclip,f32 falloff,f32 farclip,f32 clipvol)
{
    // I know this is somewhat redundant but it's just easier to keep it in
    // fixed and floating formats
    g_Audio->m_GlobalStatus.m_FarClip = (s32)(farclip*AUD_CLIP_SCALAR);
    g_Audio->m_GlobalStatus.m_FallOff = (s32)(falloff*AUD_CLIP_SCALAR);
    g_Audio->m_GlobalStatus.m_NearClip = (s32)(nearclip*AUD_CLIP_SCALAR);

    g_Audio->m_ClipVolume = clipvol;
    g_Audio->m_NearClip = nearclip;
    g_Audio->m_FallOff = falloff;
    g_Audio->m_FarClip = farclip;
}

s32     audio_GetChannelsInUse(void)
{
    return g_Audio->m_ChannelsInUse;
}


//==============================================================================

void audio_InitLooping ( s32& ChannelId )
{
    ChannelId = 0 ;
}

//==============================================================================

void audio_PlayLooping ( s32& ChannelId, u32 SampleId )
{
    // Already started?
    if (ChannelId)
    {
        // If it's stopped, then it's not a looping sound, so keep it going!
        if ( !audio_IsPlaying( ChannelId ) )
        {
            ChannelId = audio_Play(SampleId) ;
            audio_SetLooping(ChannelId);
        }
    }
    else
    {
        // Start it
        ChannelId = audio_Play(SampleId) ;
        audio_SetLooping(ChannelId);
    }
}

//==============================================================================

void audio_PlayLooping ( s32& ChannelId, u32 SampleId, vector3* pPos )
{
    ASSERT(pPos) ;

    // Already started?
    if (ChannelId)
    {
        // If it's stopped, then it's not a looping sound, so keep it going!
        if ( !audio_IsPlaying( ChannelId ) )
        {
            ChannelId = audio_Play(SampleId, pPos) ;
            audio_SetLooping(ChannelId);
        }

        // Update the position
    	audio_SetPosition(ChannelId, pPos) ;
    }
    else
    {
        // Start it
        ChannelId = audio_Play(SampleId, pPos) ;
        audio_SetLooping(ChannelId);
    }
}

//==============================================================================

void audio_StopLooping( s32& ChannelId )
{
    // Only stop if it's going...
    if (ChannelId)
    {
        // Stop the sound
        audio_Stop(ChannelId) ;

        // Flag it's stopped
        ChannelId = 0 ;
    }
}

//==============================================================================
